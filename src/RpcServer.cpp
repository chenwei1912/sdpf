#include "RpcServer.h"
#include "RpcServerImp.h"
#include "RpcService.h"
#include "logger.h"

// #include "RpcChannel.h"
// #include "TcpServer.h"

// #include <string>
// #include <future>
// #include <chrono>
// #include <thread>

// using namespace sdpf;


RpcServer::RpcServer(IOContext* pctx) {
    imp_ = new RpcServerImp(this, pctx);
}

RpcServer::~RpcServer() {
    // LOG_TRACE("RpcServer dtor");
    if (imp_) {
        delete imp_;
        imp_ = nullptr;
    }
}

void RpcServer::launch_callback(LaunchCallback cb) {
    imp_->launch_callback(cb);
}

void RpcServer::close_callback(CloseCallback cb) {
    imp_->close_callback(cb);
}

int RpcServer::start(const char* strip, unsigned short port) {
    return imp_->start(strip, port);
}

int RpcServer::stop() {
    return imp_->stop();
}

int RpcServer::register_service(RpcService* svc) {
    return imp_->register_service(svc);
}


RpcServerImp::RpcServerImp(RpcServer* pif, IOContext* pctx)
    : pif_(pif), tcp_server_(pctx) {
}

RpcServerImp::~RpcServerImp() {
    // LOG_TRACE("RpcServerImp dtor");
    channels_.clear();
    services_.clear();
}

void RpcServerImp::launch_callback(LaunchCallback cb) {
    launch_cb_ = cb;
}

void RpcServerImp::close_callback(CloseCallback cb) {
    close_cb_ = cb;
}

int RpcServerImp::start(const char* strip, unsigned short port) {
    tcp_server_.launch_callback(std::bind(&RpcServerImp::on_launch, this, std::placeholders::_1, std::placeholders::_2));
    tcp_server_.close_callback(std::bind(&RpcServerImp::on_close, this, std::placeholders::_1));
    return tcp_server_.start(strip, port);
}

int RpcServerImp::stop() {
    close_all();
    return tcp_server_.stop();
}

int RpcServerImp::register_service(RpcService* svc) {
    services_.push_back(svc);
    svc->bind(std::bind(&RpcServerImp::notify_all, this, svc, std::placeholders::_1));
    return 0;
}

void RpcServerImp::on_launch(TcpServer* svr, int status) {
    if (0 == status) {
        svr->alloc_callback(std::bind(&RpcServerImp::on_alloc, this));
        svr->accept_callback(std::bind(&RpcServerImp::on_accept, this, std::placeholders::_1, std::placeholders::_2));
    } else {
    }

    if (launch_cb_) {
        launch_cb_(pif_, status);
    }
}

TcpConnectionPtr RpcServerImp::on_alloc() {
    RpcChannelPtr chn = std::make_shared<RpcChannel>(tcp_server_.context());
    TcpConnectionPtr conn = chn->connection();
    conn->data(chn.get());
    channels_.insert(chn);
    return conn;
}

void RpcServerImp::on_accept(TcpConnectionPtr conn, int status) {
    // RpcChannelPtr chn = std::any_cast<RpcChannelPtr>(conn->data());
    RpcChannel* ptr = reinterpret_cast<RpcChannel*>(conn->data()); // static_cast
    RpcChannelPtr chn;
    for (const auto& item : channels_) {
        if (item.get() == ptr) {
            chn = item;
        }
    }
    // LOG_DEBUG("RpcChannelPtr use count: %zu", chn.use_count());

    if (0 == status) {
        chn->connect_callback(std::bind(&RpcServerImp::on_channel_connect, this, std::placeholders::_1, std::placeholders::_2));
        chn->close_callback(std::bind(&RpcServerImp::on_channel_close, this, std::placeholders::_1));
        chn->accept_connect();
    } else {
        // LOG_ERROR("RpcServerImp channel connect failed: %d", status);
        // for (auto it = channels_.begin(); it != channels_.end(); ++it) {
        //     if (ptr == (*it).get()) {
        //         channels_.erase(it);
        //         break;
        //     }
        // }
        channels_.erase(chn);
    }
}

void RpcServerImp::on_channel_connect(RpcChannelPtr chn, int status) {
    if (0 == status) {
        chn->set_services(&services_);
    } else {
        // LOG_ERROR("RpcServer can't run this path");
        // channels_.erase(chn);
    }
}

void RpcServerImp::on_channel_close(RpcChannelPtr chn) {
    channels_.erase(chn);
}

void RpcServerImp::on_close(TcpServer* svr) {
    if (close_cb_) {
        close_cb_(pif_);
    }
}

void RpcServerImp::notify_all(RpcService* svc, RpcMessage* msg) {
    int ret = 0;
    for (const auto& chn : channels_) {
        ret = chn->notify(svc, msg);
        if (0 != ret) {
            // LOG_ERROR("RpcServer notify failed: %d", ret);
        }
    }
}

void RpcServerImp::close_all() {
    for (const auto& chn : channels_) {
        chn->stop();
    }
}

