//#include "catch.hpp"
#include "IOScheduler.h"
#include "TcpServer.h"
#include "logger.h"

#include <thread>
#include <unordered_set>
#include <stdio.h>
//#include <iostream>



std::unordered_set<TcpConnectionPtr> _connections;


void test_server(IOScheduler* pctx) {
    LOG_INFO("start event loop thread!");
    // std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    pctx->run();
    LOG_INFO("exit loop thread!");
}


void tcp_recv(TcpConnectionPtr conn, IOBuf* pb, size_t n) {
    // LOG_DEBUG("server recv: %s, len: %zu", pb->begin_read(), n);
    conn->send(pb->begin_read(), n); // echo
    pb->has_readed(n);
}
void tcp_send(TcpConnectionPtr conn, int status, BufPtr pb) {
    if (0 == status) {
        // pb->begin_read();
        // pb->readable_bytes();
        // LOG_INFO("tcp connection send bytes: %zu", pb->readable_bytes());
    } else {
        LOG_INFO("tcp connection send failed: %d", status);
    }
}
void tcp_close(TcpConnectionPtr conn) {
    LOG_INFO("tcp connection close!");
    _connections.erase(conn); // release conn
}

void svr_accept(TcpConnectionPtr conn, int status) {
    if (0 == status) {
        LOG_INFO("tcp connection accept success!");
        conn->recv_callback(tcp_recv);
        conn->send_callback(tcp_send);
        conn->close_callback(tcp_close);
        _connections.insert(conn); // conn created by server, resource manage
    } else {
        LOG_INFO("tcp connection accept failed!");
        // _connections.erase(conn);
        // auto destruct conn
    }
}
TcpConnectionPtr svr_alloc(IOScheduler* pctx) {
    return std::make_shared<TcpConnection>(pctx);
}
void server_close(TcpServer* s) {
    LOG_INFO("TcpServer close!");
    // _connections.clear();
}
void server_start(TcpServer* s, int status) {
    if (0 == status) {
        LOG_INFO("TcpServer start success!");
        s->alloc_callback(std::bind(svr_alloc, s->context()));
        s->accept_callback(svr_accept);
    } else {
        LOG_ERROR("TcpServer start failed %d", status);
    }
}

void close_all_connection() {
    for (const auto& conn : _connections) {
        conn->stop();
    }
}


int main(int argc, char* argv[]) {
    int ret = log_init("gb28181");
    if (0 != ret) {
        printf("log init error\n");
        return -1;
    }
    LOG_INFO("start tcp test");

#if 1
    IOScheduler ctx;
    ctx.init();
    LOG_TRACE("IOScheduler size: %zu, AsyncTask: %zu", sizeof(IOScheduler), sizeof(IOScheduler::AsyncTask));

    TcpServer server(&ctx);
    server.launch_callback(server_start);
    server.close_callback(server_close);
    server.start("192.168.17.17", 7113);

    std::thread t(test_server, &ctx);
    // std::this_thread::sleep_for(std::chrono::milliseconds(6000));
    getchar();

    ctx.dispatch(close_all_connection);
    server.stop();
    ctx.stop();

    t.join();
    // std::this_thread::sleep_for(std::chrono::milliseconds(1000));
#endif

#if 0

#endif

#if 0

#endif

#if 0

#endif

    log_uninit();
    return 0;
}





