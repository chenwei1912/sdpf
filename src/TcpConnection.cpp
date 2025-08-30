#include "TcpConnection.h"
#include "TcpConnectionImp.h"
#include "logger.h"

#include "IOContext.h"

#include <string.h>

// using namespace sdpf;



TcpConnection::TcpConnection(IOContext* pctx) {
    imp_ = new TcpConnectionImp(pctx);
}

TcpConnection::~TcpConnection() {
    // LOG_TRACE("TcpConnection dtor!");
    if (imp_) {
        delete imp_;
        imp_ = nullptr;
    }
}

void TcpConnection::connect_callback(ConnectCallback cb) {
    imp_->connect_callback(cb);
}

void TcpConnection::recv_callback(RecvCallback cb) {
    imp_->recv_callback(cb);
}

void TcpConnection::send_callback(SendCallback cb) {
    imp_->send_callback(cb);
}

void TcpConnection::close_callback(CloseCallback cb) {
    imp_->close_callback(cb);
}

int TcpConnection::start(const char* ip_str, uint16_t port) {
    imp_->set_shared(shared_from_this()); // CAUTION: must call this for callback param
    return imp_->start(ip_str, port);
}

int TcpConnection::stop() {
    return imp_->stop();
}

int TcpConnection::send(const char* data, size_t n) {
    return imp_->send(data, n);
}

int TcpConnection::send(const BufPtr& buf) {
    return imp_->send(buf);
}

IOContext* TcpConnection::context() {
    return imp_->context();
}

void* TcpConnection::handle() {
    return imp_->handle();
}

bool TcpConnection::connected() {
    return imp_->connected();
}

void TcpConnection::data(void* data) {
    imp_->data(data);
}

void* TcpConnection::data() {
    return imp_->data();
}

void TcpConnection::accept_connect(int status) {
    imp_->set_shared(shared_from_this()); // CAUTION: must call this for callback param
    imp_->accept_connect(status);
}


static const size_t _Max_Send_Buf = 64;

TcpConnectionImp::TcpConnectionImp(IOContext* pctx) :
    context_(pctx), connected_(false) {
}

TcpConnectionImp::~TcpConnectionImp() {
    // LOG_TRACE("TcpConnectionImp dtor!");
    clear_buf();
}

void TcpConnectionImp::set_shared(TcpConnectionPtr ptr) {
    pif_ = ptr;
}

void TcpConnectionImp::connect_callback(ConnectCallback cb) {
    conn_cb_ = cb;
}

void TcpConnectionImp::recv_callback(RecvCallback cb) {
    recv_cb_ = cb;
}

void TcpConnectionImp::send_callback(SendCallback cb) {
    send_cb_ = cb;
}

void TcpConnectionImp::close_callback(CloseCallback cb) {
    close_cb_ = cb;
}

int TcpConnectionImp::start(const char* ip_str, uint16_t port) {
    if (nullptr == ip_str || 0 == port) {
        return -1;
    }
    if (nullptr == context_ || !context_->is_init()) {
        return -2;
    }
    if (connected_) { // 0 != uv_is_active((uv_handle_t*)&handle_)
        return -3;
    }

    std::shared_ptr<SocketAddr> ptr = std::make_shared<SocketAddr>();
    strncpy(ptr->ip, ip_str, sizeof(ptr->ip) - 1);
    ptr->port = port;

    int ret = context_->dispatch(std::bind(&TcpConnectionImp::on_start, this, ptr));
    if (0 != ret) {
        LOG_ERROR("TcpConnection::start dispatch failed: %d", ret);
        return -4;
    }

    // connected_ = true;
    return 0;
}

int TcpConnectionImp::stop() {
    if (nullptr == context_ || !context_->is_init()) {
        return -1;
    }
    if (!connected_) {
        return -2;
    }

    int ret = context_->dispatch(std::bind(&TcpConnectionImp::on_stop, this));
    if (0 != ret) {
        LOG_ERROR("TcpConnection::stop dispatch failed: %d", ret);
        return -3;
    }

    // connected_ = false;
    return 0;
}

int TcpConnectionImp::send(const char* data, size_t n) {
    if (nullptr == data || 0 == n) {
        return -1;
    }
    if (nullptr == context_ || !context_->is_init()) {
        return -2;
    }
    if (!connected_) {
        return -3;
    }

    // copy data to buffer because data is probably invalid when leave this function
    BufPtr buf = std::make_shared<IOBuf>(n);
    buf->write(data, n);
    return send(buf);
}

int TcpConnectionImp::send(const BufPtr& buf) {
    if (!buf || 0 == buf->readable_bytes()) {
        return -1;
    }
    if (nullptr == context_ || !context_->is_init()) {
        return -2;
    }
    if (!connected_) {
        return -3;
    }

    return context_->dispatch(std::bind(&TcpConnectionImp::on_send, this, buf));
}

IOContext* TcpConnectionImp::context() {
    return context_;
}

uv_tcp_t* TcpConnectionImp::handle() {
    return &handle_;
}

bool TcpConnectionImp::connected() {
    return connected_;
}

void TcpConnectionImp::data(void* data) {
    data_ = data;
}

void* TcpConnectionImp::data() const {
    return data_;
}

void TcpConnectionImp::accept_connect(int status) {
    // handle_.data = (void*)this;
    on_connect(status);
}

void TcpConnectionImp::on_start(std::shared_ptr<SocketAddr> ptr) {
    if (connected_) {
        return;
    }
    if (0 != uv_is_active((uv_handle_t*)&handle_)) {
        LOG_WARN("TcpConnection is already active");
    }

    connected_ = true;
    memset(&addr_, 0, sizeof(addr_));
    strncpy(addr_.ip, ptr->ip, sizeof(addr_.ip));
    addr_.port = ptr->port;

    int ret = 0;
    do {
        // handle_.data = (void*)this;
        ret = uv_tcp_init((uv_loop_t*)context_->handle(), &handle_);
        if (0 != ret) {
            LOG_ERROR("TcpConnection uv_tcp_init failed: %d, %s", ret, uv_strerror(ret));
            break;
        }

        struct sockaddr_in sock_addr;
        ret = uv_ip4_addr(addr_.ip, addr_.port, &sock_addr); // reverse uv_ip4_name
        if (0 != ret) {
            LOG_ERROR("TcpConnection uv_ip4_addr failed: %d, %s", ret, uv_strerror(ret));
            break;
        }

        connect_req_.data = (void*)this;
        ret = uv_tcp_connect(&connect_req_, &handle_, (const sockaddr*)&sock_addr, [](uv_connect_t* req, int status){
            // assert(req->handle == (&handle_));
            TcpConnectionImp* c = static_cast<TcpConnectionImp*>(req->data);
            c->on_connect(status);
        });
        if (0 != ret) {
            LOG_ERROR("TcpConnection uv_tcp_connect failed: %d, %s", ret, uv_strerror(ret));
            break;
        }

        LOG_INFO("TcpConnection start connect %s:%d", addr_.ip, addr_.port);
        return;
    } while (0);

// FAILED_START:
    on_connect(ret);
}

void TcpConnectionImp::on_stop() {
    if (!connected_) {
        return;
    }

    connected_ = false;
    // LOG_DEBUG("TcpConnection::on_stop");

    if (0 != uv_is_active((uv_handle_t*)&handle_)) {
        uv_read_stop((uv_stream_t*)&handle_); // uv_shutdown?
        // LOG_DEBUG("TcpConnection uv_read_stop ret: %d", ret);
    }
    if (0 == (uv_is_closing((uv_handle_t*)&handle_))) {
        uv_close((uv_handle_t*)&handle_, [](uv_handle_t* h) {
            TcpConnectionImp* c = static_cast<TcpConnectionImp*>(h->data);
            c->on_close();
        });
    } else {
        on_close();
    }
}

void TcpConnectionImp::on_send(BufPtr buf) {
    if (!connected_) {
        return;
    }

    if (send_buf_.size() >= _Max_Send_Buf) {
        if (send_cb_) {
            send_cb_(pif_, send_buf_.size(), buf);
        }
        return;
    }

    bool sending = !send_buf_.empty();
    send_buf_.push(buf);
    if (!sending) { // not sending, start send!
        send_buf();
    }
}

void TcpConnectionImp::on_connect(int status) {
    connected_ = true; // for server accept

    if (0 != status) {
        LOG_ERROR("TcpConnection connect failed %d:%s", status, uv_strerror(status));
        if (conn_cb_) {
            conn_cb_(pif_, status);
        }
        on_stop();
        return;
    }

    // int uv_tcp_getsockname(const uv_tcp_t* handle, struct sockaddr* name, int* namelen)
    // int uv_tcp_getpeername(const uv_tcp_t* handle, struct sockaddr* name, int* namelen)
    // LOG_INFO("TcpConnection[{}] establish: local[{}:{}] remote[{}:{}]",
    //          name_, local_ep_.address().to_v4().to_string(), local_ep_.port(),
    //          remote_ep_.address().to_v4().to_string(), remote_ep_.port());

    // int sz = 1024 * 1024 * 20;
    // uv_send_buffer_size((uv_handle_t*)&handle_, &sz);
    // uv_recv_buffer_size((uv_handle_t*)&handle_, &sz);
    // assert(0 == status);
    // assert(1 == uv_is_readable((uv_stream_t*)&handle_));
    // assert(1 == uv_is_writable((uv_stream_t*)&handle_));
    // assert(0 == uv_is_closing((uv_handle_t*)&handle_));

    int ret = uv_tcp_nodelay(&handle_, 1);
    if (0 != ret) {
        LOG_WARN("TcpConnection uv_tcp_nodelay failed %d:%s", ret, uv_strerror(ret));
    }

    // linger

    handle_.data = (void*)this;
    ret = uv_read_start((uv_stream_t*)&handle_, [](uv_handle_t* handle, size_t s_size, uv_buf_t* buf){
        TcpConnectionImp* c = static_cast<TcpConnectionImp*>(handle->data);
        c->on_alloc(s_size, buf);
    }, [](uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf){
        TcpConnectionImp* c = static_cast<TcpConnectionImp*>(stream->data);
        c->on_read(nread, buf);
    });
    if (0 != ret) {
        LOG_WARN("TcpConnection uv_read_start failed %d:%s", ret, uv_strerror(ret));
    }

    if (conn_cb_) {
        conn_cb_(pif_, status);
    }
}

void TcpConnectionImp::on_alloc(size_t suggested_size, uv_buf_t* buf) {
    if (!connected_) {
        return;
    }

    // if (suggested_size > IOBuf::InitialSize) {
    //     recv_buf_.ensure_writable(suggested_size);
    // } else {
    recv_buf_.ensure_writable(IOBuf::initialSize);
    // }
    buf->base = recv_buf_.begin_write();
    buf->len = recv_buf_.writable_bytes();
}

void TcpConnectionImp::on_read(ssize_t nread, const uv_buf_t* buf) {
    if (!connected_) {
        return;
    }

    if (nread > 0) {
        // LOG_TRACE("TcpConnection recv: %s, len: %ld, buf_len:%lu", buf->base, nread, buf->len);
        recv_buf_.has_written(nread);

        if (recv_cb_) {
            recv_cb_(pif_, &recv_buf_, nread);
        } else {
            recv_buf_.has_readed(nread);
        }
    } else {
        LOG_ERROR("TcpConnection recv error %ld:%s", nread, uv_strerror(nread));
        on_stop();
        // if (0 == nread) {
        // } else if (UV_EOF == nread) {
        // } else if (-4077 == nread || -104 == nread) { // windows & linux
        // } else {
        // }
    }
}

void TcpConnectionImp::on_write(int status) {
    if (!connected_) {
        return;
    }

    if (send_cb_) {
        send_cb_(pif_, status, send_buf_.front());
    }

    if (0 == status) {
        send_buf_.pop(); // release buffer
        if (!send_buf_.empty()) { // continue to send
            send_buf();
        }
    } else {
        LOG_ERROR("TcpConnection on_write failed %d:%s", status, uv_strerror(status));
        on_stop();
    }
}

void TcpConnectionImp::on_close() {
    clear_buf();

    if (close_cb_) {
        close_cb_(pif_);
    }
}

void TcpConnectionImp::send_buf() {
    uv_buf_t buf = uv_buf_init(const_cast<char*>(send_buf_.front()->begin_read()), send_buf_.front()->readable_bytes());
    write_req_.data = (void*)this;
    // write requests sent with uv_write will be queued
    // reuse write_req_ to call uv_write should after uv_write_cb(on_write) callback is fired
    int ret = uv_write(&write_req_, (uv_stream_t*)&handle_, &buf, 1, [](uv_write_t* req, int status){
        TcpConnectionImp* c = static_cast<TcpConnectionImp*>(req->data);
        c->on_write(status);
    });
    if (0 != ret) {
        LOG_ERROR("TcpConnection uv_write failed %d:%s", ret, uv_strerror(ret));
    }
}

void TcpConnectionImp::clear_buf() {
    while (!send_buf_.empty()) {
        send_buf_.pop();
    }
}
