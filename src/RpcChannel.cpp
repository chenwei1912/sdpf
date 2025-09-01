#include "RpcChannel.h"
#include "RpcChannelImp.h"

#include "RpcService.h"
#include "RpcMessage.h"
#include "RpcController.h"
// #include "Dispatcher.h"
#include "logger.h"


#if (defined WIN32) || (defined _WIN64)
#include <WinSock2.h>
#else
#include <arpa/inet.h>
#endif

#include <string.h>



RpcChannel::RpcChannel(IOContext* pctx) {
    imp_ = new RpcChannelImp(pctx);
}

RpcChannel::~RpcChannel() {
    // LOG_TRACE("RpcChannel dtor");
    if (imp_) {
        delete imp_;
        imp_ = nullptr;
    }
}

void RpcChannel::connect_callback(ConnectCallback cb) {
    imp_->connect_callback(cb);
}

void RpcChannel::close_callback(CloseCallback cb) {
    imp_->close_callback(cb);
}

int RpcChannel::start(const char* ip_str, uint16_t port) {
    imp_->set_shared(shared_from_this()); // CAUTION: must call this for callback param
    return imp_->start(ip_str, port);
}

int RpcChannel::stop() {
    return imp_->stop();
}

int RpcChannel::call_method(const char* service, const char* method, RpcMessage* request, RpcMessage* response,
                            RpcController* ctrl) {
    return imp_->call_method(service, method, request, response, ctrl);
}

int RpcChannel::notify(RpcService* psvc, RpcMessage* msg) {
    return imp_->notify(psvc, msg);
}

TcpConnectionPtr RpcChannel::connection() {
    return imp_->connection();
}

void RpcChannel::accept_connect() {
    imp_->set_shared(shared_from_this()); // CAUTION: must call this for callback param
    imp_->accept_connect();
}

void RpcChannel::set_services(std::vector<RpcService*>* services) {
    imp_->set_services(services);
}


typedef struct {
    uint8_t mark[4];    // "ERPC"
    uint16_t body_len;  // body length(network order)
    uint16_t meta_len;  // meta length(network order)
    uint8_t type;       // type
    uint8_t mode;     // mode 0:raw, 1:msgpack, 2:json, 3:protobuf
    uint8_t version;    // 0x10 = 1.0
    uint8_t reserve;    // "#"
} HeaderPacket;

static const int _ENOERROR = 0;
static const int _ENOSERVICE = 1001;
static const int _ENOMETHOD = 1002;
static const int _EREQUEST = 1003;
static const int _ERESPONSE = 1004;
static const int _EMETHODCALL = 1005;
static const int _ENETWORK = 1006;
static const int _EINTERNAL = 1007;
static const std::unordered_map<int, std::string> _CodeError = {
   { _ENOERROR, "" },
   { _ENOSERVICE, "no service" },
   { _ENOMETHOD, "no method" },
   { _EREQUEST, "invalid request" },
   { _ERESPONSE, "invalid response" },
   { _EMETHODCALL, "method call error" },
   { _ENETWORK, "network error" },
   { _EINTERNAL, "server internal error" },
};

static const char* _METHOD_NOTIFY = "notify";
static const int _ENOTIFY_MARK = 9876;


RpcChannelImp::RpcChannelImp(IOContext* pctx)
    : id_(0), services_(nullptr) {
    conn_ = std::make_shared<TcpConnection>(pctx);
}

RpcChannelImp::~RpcChannelImp() {
    // LOG_TRACE("RpcChannelImp dtor");
    if (conn_) {
        conn_.reset();
    }
    // requests_.clear();

}

void RpcChannelImp::set_shared(RpcChannelPtr ptr) {
    pif_ = ptr;
}

void RpcChannelImp::connect_callback(ConnectCallback cb) {
    conn_cb_ = cb;
}

void RpcChannelImp::close_callback(CloseCallback cb) {
    close_cb_ = cb;
}

int RpcChannelImp::start(const char* ip_str, uint16_t port) {
    if (nullptr == ip_str || 0 == port) {
        return -1;
    }
    if (!conn_ || conn_->connected()) {
        return -2;
    }
    if (nullptr != services_) { // server side
        return -2;
    }

    conn_->connect_callback(std::bind(&RpcChannelImp::on_connect, this, std::placeholders::_1, std::placeholders::_2));
    // conn_->close_callback(std::bind(&RpcChannelImp::on_close, this, std::placeholders::_1));
    return conn_->start(ip_str, port);
}

int RpcChannelImp::stop() {
    if (conn_) {
        conn_->stop();
    }
    return 0;
}

int RpcChannelImp::call_method(const char* service, const char* method, RpcMessage* request, RpcMessage* response,
                            RpcController* ctrl) {
    if (nullptr == service || nullptr == method || nullptr == ctrl) {
        return -1;
    }
    if (!conn_ || !conn_->connected()) {
        return -2;
    }
    if (nullptr != services_) { // server side
        return -3;
    }

    if (0 == strcmp(method, _METHOD_NOTIFY)) { // special for notify
        if (!ctrl->done()) {
            return -3;
        }
        ctrl->ec(_ENOTIFY_MARK); // not 0 to mark notify for not remove in response
        ctrl->text(service);
    } else {
        ctrl->ec(0);
    }

    Meta_Rpc meta;
    // std::shared_ptr<Meta_Rpc> pm = std::make_shared<Meta_Rpc>();
    memset(&meta, 0, sizeof(meta));
    meta.type = RPCTYPE_REQUEST;
    meta.relation_id = ++id_;
    strncpy(meta.via.req.service, service, sizeof(meta.via.req.service) - 1);
    strncpy(meta.via.req.method, method, sizeof(meta.via.req.method) - 1);

    ctrl->resp(response);

    {
    std::lock_guard<std::mutex> lock(mutex_requests_);
    if (0 != ctrl->ec()) { // notify request
        auto it = requests_.begin();
        for (; it != requests_.end(); ++it) {
            RpcController* ptr = static_cast<RpcController*>(it->second);
            if (0 != ptr->ec() // already have notify in same service
                && 0 == strcmp(service, ptr->text())) {
                break;
            }
        }
        if (it != requests_.end()) {
            LOG_WARN("client service: %s, repeat notify: %u, new notify: %u", service, it->first, meta.relation_id);
            return -3;
        }
    }
    requests_[meta.relation_id] = ctrl;
    // LOG_DEBUG("call wait request id: %u, size: %zu", meta.relation_id, requests_.size());
    }

    int ret = 0;
    do {
        auto done = ctrl->done();
        if (done) { // async call
            ret = send_packet(&meta, request);
            if (0 != ret) {
                LOG_ERROR("channel request send failed: %d", ret);
                ret = -4;
                break;
            }
        } else { // no callback, sync call
            ctrl->lock(); // CAUTION: carefully lock/unlock

            int ret = send_packet(&meta, request);
            if (0 != ret) {
                LOG_ERROR("channel request send failed: %d", ret);
                ctrl->unlock();
                ret = -4;
                break;
            }

            ret = ctrl->wait();
            if (0 != ret) { // timeout
                ret = -5;
                break;
            }
        }
    } while (false);

    if (0 != ret) {
        ctrl->ec(0); // reset 0 to delete
        ctrl->text("");
        remove_request(meta.relation_id);
    }
    return ret;
}

int RpcChannelImp::notify(RpcService* psvc, RpcMessage* msg) {
    if (nullptr == psvc || nullptr == msg) {
        return -1;
    }
    if (!conn_ || !conn_->connected()) {
        return -2;
    }
    if (nullptr == services_) { // client side
        return -2;
    }

    uint32_t id = 0;
    {
    std::lock_guard<std::mutex> lock(mutex_requests_);
    auto it = requests_.begin();
    for (; it != requests_.end(); ++it) {
        // RpcService* svc = static_cast<RpcService*>(it->second);
        if (it->second == psvc/* && 0 == strcmp(svc->name(), psvc->name())*/) {
            break;
        }
    }
    if (it != requests_.end()) {
        id = it->first;
    } else {
        // LOG_ERROR("service: %s notify can not find request id", psvc->name());
        return -3;
    }
    }

    send_resp(id, _ENOERROR, msg);
    return 0;
}

TcpConnectionPtr RpcChannelImp::connection() {
    return conn_;
}

void RpcChannelImp::accept_connect(/*TcpConnectionPtr conn*/) {
    // if (!conn_) {
        // accept_ = true;
        // conn_ = conn;
    on_connect(conn_, 0);
    // }
}

void RpcChannelImp::set_services(std::vector<RpcService*>* services) {
    services_ = services;
}

void RpcChannelImp::on_connect(TcpConnectionPtr conn, int status) {
    if (0 == status) {
        conn->recv_callback(std::bind(&RpcChannelImp::on_recv, this, std::placeholders::_1, std::placeholders::_2,
            std::placeholders::_3));
        conn->send_callback(std::bind(&RpcChannelImp::on_send, this, std::placeholders::_1, std::placeholders::_2,
            std::placeholders::_3));
        conn_->close_callback(std::bind(&RpcChannelImp::on_close, this, std::placeholders::_1));
    } else {
        // LOG_INFO("tcp connection establish failed!");
    }

    if (conn_cb_) {
        conn_cb_(pif_, status);
    }
}

void RpcChannelImp::on_recv(TcpConnectionPtr conn, IOBuf* pb, size_t n) {
    int ret = 0;
    do {
        ret = parse(pb);
        if (ret > 0) {
            pb->has_readed(ret); // continue to parse next message
        } else if (ret < 0) {
            conn->stop();
        } // 0 == ret, exit loop to continue recv data
    } while (ret > 0);
}

void RpcChannelImp::on_send(TcpConnectionPtr conn, int status, BufPtr spb) {
    if (0 == status) {
    } else {
        if (status > 0) {
            // send buff full, discard send data
        }
    }
}

void RpcChannelImp::on_close(TcpConnectionPtr conn) {
    {
    std::lock_guard<std::mutex> lock(mutex_requests_);
    if (nullptr == services_) { // client side
        for (auto& item : requests_) { // process all remain request
            RpcController* ctrl = static_cast<RpcController*>(item.second);
            ctrl->ec(_ENETWORK);
            auto done = ctrl->done();
            if (done) {
                done();
            } else {
                ctrl->notify();
            }
        }
    }
    requests_.clear();
    }

    services_ = nullptr;

    if (close_cb_) {
        close_cb_(pif_);
    }
}

int RpcChannelImp::parse(IOBuf* pb) {
    uint16_t pack_len = 0;
    uint16_t body_len = 0;
    uint16_t meta_len = 0;
    size_t header_len = sizeof(HeaderPacket);
    size_t remain_len = pb->readable_bytes();
    if (remain_len < header_len) { // not complete header
        return 0;
    }
    const char* buf = pb->begin_read();

    // ================= parse header ====================
    HeaderPacket* header = (HeaderPacket*)buf;
    body_len = ntohs(header->body_len);
    meta_len = ntohs(header->meta_len);

    // XXX: search "ERPC"?
    if (0 != memcmp(header->mark, "ERPC", 4)
        || 1 != header->mode
        || 0x10 != header->version) {
        LOG_ERROR("header error %c%c%c%c, body_len:%u, meta_len:%u, type:%u, mode:%u, version:%u",
            header->mark[0], header->mark[1], header->mark[2], header->mark[3],
            body_len, meta_len, header->type, header->mode, header->version);
        return -1;
    }
    // LOG_DEBUG("parse header %c%c%c%c, body_len:%u, meta_len:%u, type:%u, mode:%u, version:%.2X",
    //     header->mark[0], header->mark[1], header->mark[2], header->mark[3],
    //     body_len, meta_len, header->type, header->mode, header->version);

    pack_len = header_len + meta_len + body_len;
    if (remain_len < pack_len) { // not complete message
        return 0;
    }

    // ================= parse meta ====================
    buf += header_len;

    Meta_Rpc meta;
    memset(&meta, 0, sizeof(meta));
    meta.type = (Meta_RpcType)header->type;
    int ret = deserialize_meta(&meta, buf, meta_len);
    if (0 != ret) {
        LOG_ERROR("meta parse failed: %d", ret);
        return -2;
    }

    // ================= process body ====================
    buf += meta_len;

    if (RPCTYPE_REQUEST == meta.type) {
        process_request(meta.relation_id, meta.via.req.service, meta.via.req.method, buf, body_len);
    } else if (RPCTYPE_RESPONSE == meta.type) {
        process_response(meta.relation_id, meta.via.rsp.code, meta.via.rsp.text, buf, body_len);
    } else {
        LOG_ERROR("meta type error: %d", meta.type);
    }

    return pack_len;
}

void RpcChannelImp::process_request(uint32_t id, const char* service, const char* method, const char* buf, size_t len) {
    // LOG_DEBUG("process request id: %u, service: %s, method: %s, len: %zu", id, service, method, len);
    int ec = _ENOERROR;
    RpcMessage* request = nullptr;
    RpcMessage* response = nullptr;
    RpcController* ctrl = nullptr;

    do {
        RpcService* psvc = find_svc(service);
        if (nullptr == psvc) {
            LOG_ERROR("server not find service: %s", service);
            ec = _ENOSERVICE;
            break;
        }

        int method_id = psvc->find_method(method);
        if (method_id < 0) {
            LOG_ERROR("service no method: %s", method);
            ec = _ENOMETHOD;
            break;
        }

        if (0 == strcmp(method, _METHOD_NOTIFY) && 0 == len) { // register notify
            std::lock_guard<std::mutex> lock(mutex_requests_);
            auto it = requests_.begin();
            for (; it != requests_.end(); ++it) {
                // RpcService* svc = static_cast<RpcService*>(it->second);
                if (it->second == psvc/* && 0 == strcmp(it->second.name(), psvc.name())*/) {
                    break;
                }
            }
            if (it == requests_.end()) {
                LOG_INFO("service: %s, setting notify request id: %u", service, id);
                requests_[id] = psvc;
            } else {
                LOG_WARN("service: %s, id: %u, has repeat notify request: %u", service, id, it->first);
            }
            break; // no process for notify register
        }

        // get request from method
        request = psvc->new_request(method_id); // std::unique_ptr<RpcMessage> req(request);
        if (len > 0 && nullptr != request) {
            int ret = request->deserialize(buf, len);
            if (0 != ret) {
                LOG_ERROR("request parse failed: %d", ret);
                ec = _EREQUEST;
                break;
            }
        }

        // get response from method, delete in done callback
        response = psvc->new_response(method_id);

        // prepare control, delete in done callback
        RpcController* ctrl = new (std::nothrow) RpcController;
        if (nullptr == ctrl) {
            LOG_ERROR("RpcController memory failed");
            ec = _EINTERNAL;
            break;
        }
        ctrl->resp(response);

        // done callback
        auto cb = std::bind(&RpcChannelImp::done, this, id, ctrl);
        ctrl->done(cb);

        psvc->call_method(method_id, request, response, ctrl);
    } while (false);

    if (request){
        delete request;
    }
    if (_ENOERROR != ec) {
        // LOG_ERROR("proc request error: %d", ec);
        if (response){
            delete response;
        }
        if (ctrl){
            delete ctrl;
        }
        send_resp(id, ec, nullptr);
    }
}

void RpcChannelImp::process_response(uint32_t id, int code, const char* text, const char* buf, size_t len) {
    // LOG_DEBUG("process response id: %u, code: %d, text: %s, len: %zu", id, code, text, len);
    RpcController* ctrl = remove_request(id);
    if (nullptr == ctrl) {
        LOG_ERROR("no request find id: %u", id);
        return;
    }

    bool bnotify = false;
    if (0 != ctrl->ec()) {
        bnotify = true;
    }

    int ec = _ENOERROR;
    RpcMessage* response = ctrl->resp();
    if (len > 0 && nullptr != response) {
        int ret = response->deserialize(buf, len);
        if (0 != ret) {
            LOG_ERROR("response parse failed: %d", ret);
            ec = _ERESPONSE;
        }
    }

    if (0 == ec) { // no server error!
        // set respone code and text
        ctrl->ec(code);
        // if (_ENOERROR != code) { // 0 == ec: service name in text
        //     ctrl->text(text);
        // }
    } else {
        ctrl->ec(ec); // replace by server error
        // ctrl->text(text);
        //ctrl->text("invalid response"); // TODO: find error text
    }

    auto done = ctrl->done();
    if (done) {
        done();
    } else {
        ctrl->notify();
    }

    if (bnotify) {
        ctrl->ec(_ENOTIFY_MARK); // not 0 to mark notify for not remove in response
        // ctrl->text();
    }
}

// this callback is called by server side user(sync or async in user's thread)
void RpcChannelImp::done(uint32_t id, RpcController* ctrl) {
    // LOG_DEBUG("rpc server method done callback");
    int ec = _ENOERROR;
    RpcMessage* response = ctrl->resp();
    if (ctrl->failed()) {
        LOG_ERROR("server method result error: %s", ctrl->text());
        ec = _EMETHODCALL;
    }

    send_resp(id, ec, response);

    delete response;
    delete ctrl;
}

void RpcChannelImp::send_resp(uint32_t id, int ec, RpcMessage* response) {
    Meta_Rpc meta;
    memset(&meta, 0, sizeof(meta));
    meta.type = RPCTYPE_RESPONSE;
    meta.relation_id = id;
    meta.via.rsp.code = ec;
    if (_ENOERROR != ec) {
        // find error text
        // strncpy(meta.via.rsp.text, "404 not found", 13);
    }

    int ret = send_packet(&meta, response);
    if (0 != ret) {
        LOG_ERROR("channel response send failed: %d", ret);
    }
}

int RpcChannelImp::send_packet(Meta_Rpc* meta, RpcMessage* msg) {
    BufPtr pb = std::make_shared<IOBuf>();

    // ================= pack header ====================
    HeaderPacket* header = (HeaderPacket*)pb->begin_write();
    memcpy((char*)header->mark, "ERPC", 4);
    header->body_len = 0;
    header->meta_len = 0;
    header->type = meta->type;
    header->mode = 1;
    header->version = 0x10;
    header->reserve = '#';
    pb->has_written(sizeof(HeaderPacket));

    // ================= serialize meta ====================
    // TODO: pb->ensure_writable();
    int ret = serialize_meta(meta, pb->begin_write(), pb->writable_bytes());
    if (ret > 0) {
        // LOG_TRACE("serialize meta len: %d", ret);
        pb->has_written(ret);
        header->meta_len = htons(ret);
    } else {
        return -1;
    }
    // LOG_TRACE("channel pack header+meta len: %zu", pb->readable_bytes());

    // ================= serialize body ====================
    if (nullptr != msg) {
        // TODO: pb->ensure_writable();
        ret = msg->serialize(pb->begin_write(), pb->writable_bytes());
        if (ret > 0) {
            // LOG_TRACE("serialize message len: %d", ret);
            pb->has_written(ret);
            header->body_len = htons(ret);
        } else {
            return -2;
        }
    }

    ret = conn_->send(pb);
    if (0 != ret) {
        LOG_ERROR("send packet failed: %d", ret);
        return -3;
    }

    return 0;
}

RpcController* RpcChannelImp::remove_request(uint32_t id) {
    RpcController* ctrl = nullptr;
    std::lock_guard<std::mutex> lock(mutex_requests_);
    auto it = requests_.find(id);
    if (it != requests_.end()) {
        ctrl = static_cast<RpcController*>(it->second);
        if (0 == ctrl->ec()) { // not delete for notify
            requests_.erase(it);
        }
    }
    // LOG_DEBUG("client remove request id: %u, size: %zu", id, requests_.size());
    return ctrl;
}

RpcService* RpcChannelImp::find_svc(const char* name) {
    auto it = services_->begin();
    for (; it != services_->end(); ++it) {
        if (0 == strncmp(name, (*it)->name(), strlen(name))) {
            return *it;
        }
    }
    return nullptr;
}
