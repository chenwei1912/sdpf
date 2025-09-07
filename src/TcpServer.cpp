#include "TcpServer.h"
#include "TcpServerImp.h"
#include "IOScheduler.h"
#include "logger.h"

#include <string.h>


// using namespace sdpf;

// #define DEFAULT_PORT 7000
// #define DEFAULT_BACKLOG 128
static const int _DEFAULT_BACKLOG = 128;


TcpServer::TcpServer(IOScheduler* pctx) {
    imp_ = new TcpServerImp(this, pctx);
}

TcpServer::~TcpServer() {
    // LOG_TRACE("TcpServer [{}] destructing", name_);
    if (imp_) {
        delete imp_;
        imp_ = nullptr;
    }
}

void TcpServer::launch_callback(LaunchCallback cb) {
    imp_->launch_callback(cb);
}

void TcpServer::alloc_callback(AllocCallback cb) {
    imp_->alloc_callback(cb);
}

void TcpServer::accept_callback(AcceptCallback cb) {
    imp_->accept_callback(cb);
}

void TcpServer::close_callback(CloseCallback cb) {
    imp_->close_callback(cb);
}

int TcpServer::start(const char* ip_str, uint16_t port) {
    return imp_->start(ip_str, port);
}

int TcpServer::stop() {
    return imp_->stop();
}

IOScheduler* TcpServer::context() {
    return imp_->context();
}

// void* TcpServer::handle() {
//     return imp_->handle();
// }


TcpServerImp::TcpServerImp(TcpServer* pif, IOScheduler* pctx)
    : pif_(pif), context_(pctx), started_(false) {
}

TcpServerImp::~TcpServerImp() {
    // LOG_TRACE("TcpServerImp [{}] destructing", name_);
}

void TcpServerImp::launch_callback(LaunchCallback cb) {
    launch_cb_ = cb;
}

void TcpServerImp::alloc_callback(AllocCallback cb) {
    alloc_cb_ = cb;
}

void TcpServerImp::accept_callback(AcceptCallback cb) {
    accept_cb_ = cb;
}

void TcpServerImp::close_callback(CloseCallback cb) {
    close_cb_ = cb;
}

int TcpServerImp::start(const char* ip_str, uint16_t port) {
    if (nullptr == context_ || !context_->is_init()) {
        return -1;
    }
    if (started_) { // 0 != uv_is_active((uv_handle_t*)&handle_)
        return -2;
    }

    std::shared_ptr<SocketAddr> ptr = std::make_shared<SocketAddr>();
    strncpy(ptr->ip, ip_str, sizeof(ptr->ip) - 1);
    ptr->port = port;

    int ret = context_->dispatch(std::bind(&TcpServerImp::on_start, this, ptr));
    if (0 != ret) {
        LOG_ERROR("TcpServer::start dispatch failed: %d", ret);
        return -3;
    }

    // started_ = true;
    return 0;
}

int TcpServerImp::stop() {
    if (nullptr == context_ || !context_->is_init()) {
        return -1;
    }
    if (!started_) {
        return -2;
    }

    int ret = context_->dispatch(std::bind(&TcpServerImp::on_stop, this));
    if (0 != ret) {
        LOG_ERROR("TcpServer::stop dispatch failed: %d", ret);
        return -3;
    }

    // started_ = false;
    return 0;
}

IOScheduler* TcpServerImp::context() {
    return context_;
}

// uv_tcp_t* TcpServerImp::handle() {
//     return &server_;
// }

void TcpServerImp::on_start(std::shared_ptr<SocketAddr> ptr) {
    if (started_) {
        return;
    }
    if (0 != uv_is_active((uv_handle_t*)&server_)) {
        LOG_WARN("tcp server is already active");
    }

    started_ = true;
    memset(&addr_, 0, sizeof(addr_));
    strncpy(addr_.ip, ptr->ip, sizeof(addr_.ip));
    addr_.port = ptr->port;

    int ret = 0;
    do {
        server_.data = (void*)this;
        ret = uv_tcp_init((uv_loop_t*)context_->handle(), &server_);
        if (0 != ret) {
            LOG_ERROR("tcp server init failed: %d, %s", ret, uv_strerror(ret));
            break;
        }

        struct sockaddr_in sock_addr;
        ret = uv_ip4_addr(addr_.ip, addr_.port, &sock_addr); // reverse uv_ip4_name
        if (0 != ret) {
            LOG_ERROR("tcp server ipv4 address failed: %d, %s", ret, uv_strerror(ret));
            break;
        }

        // UV_TCP_REUSEPORT UV_TCP_IPV6ONLY
        ret = uv_tcp_bind(&server_, (const struct sockaddr*)&sock_addr, 0);
        if (0 != ret) {
            LOG_ERROR("tcp server bind failed: %d, %s", ret, uv_strerror(ret));
            break;
        }

        ret = uv_listen((uv_stream_t*)&server_, _DEFAULT_BACKLOG, [](uv_stream_t* server, int status){
            TcpServerImp* s = static_cast<TcpServerImp*>(server->data);
            s->on_accept(status);
        });
        if (0 != ret) {
            LOG_ERROR("tcp server listen failed: %d, %s", ret, uv_strerror(ret));
            break;
        }
    } while (0);

    if (launch_cb_) {
        launch_cb_(pif_, ret);
    }
    if (0 == ret) {
        // LOG_DEBUG("TcpServer start success %s:%d", addr_.ip, addr_.port);
    } else {
        // LOG_ERROR("TcpServer start failed %d:%s", ret, uv_strerror(ret));
        on_stop();
    }
}

void TcpServerImp::on_stop() {
    if (!started_) {
        return;
    }

    started_ = false;
    // close_cb_ = cb;
    // LOG_DEBUG("TcpServer stop!");

    if (0 != uv_is_active((uv_handle_t*)&server_)) {
        // LOG_DEBUG("tcp server stop active!");
        uv_read_stop((uv_stream_t*)&server_);
        // LOG_DEBUG("TcpServer uv_read_stop ret: %d", ret);
    }
    if (0 == (uv_is_closing((uv_handle_t*)&server_))) {
        uv_close((uv_handle_t*)&server_, [](uv_handle_t* h) {
            TcpServerImp* s = static_cast<TcpServerImp*>(h->data);
            s->on_close();
        });
    } else {
        on_close();
    }
}

void TcpServerImp::on_accept(int status) {
    if (status < 0) {
        LOG_ERROR("TcpServer new connection error: %d", status);
        return;
    }

    // LOG_DEBUG("TcpServer new connection coming!");
    TcpConnectionPtr conn;
    if (alloc_cb_) {
        conn = alloc_cb_();
    } else {
        conn = std::make_shared<TcpConnection>(context_);
    }
    // LOG_DEBUG("TcpConnection 1 use_count:%ld\n", conn.use_count());

    int ret = 0;
    do {
        uv_tcp_t* handle = (uv_tcp_t*)conn->handle();
        // handle->data = (void*)conn.get(); // CAUTION: easy misuse
        ret = uv_tcp_init((uv_loop_t*)conn->context()->handle(), handle);
        if (0 != ret) {
            LOG_ERROR("TcpServer uv_tcp_init failed: %d, %s", ret, uv_strerror(ret));
            break;
        }

        ret = uv_accept((uv_stream_t*)&server_, (uv_stream_t*)handle);
        if (0 != ret) {
            LOG_ERROR("TcpServer uv_accept failed: %d, %s", ret, uv_strerror(ret));
            break;
        }
    } while (false);

    // conn not have connect/close callback
    // conn->connect_callback(conn_cb_);
    conn->accept_connect(ret);
    if (accept_cb_) {
        accept_cb_(conn, ret);
    }
}

void TcpServerImp::on_close() {
    if (close_cb_) {
        close_cb_(pif_);
    }
}
