#include "IOScheduler.h"
#include "TcpConnection.h"
#include "IOTimer.h"
#include "logger.h"

#include <thread>
#include <unordered_set>
#include <stdio.h>
//#include <iostream>


// TcpConnectionPtr _conn;
std::unordered_set<TcpConnectionPtr> _connections;
// std::queue<BufferPtr> _send_buf;

uint32_t _recv_bytes = 0;
uint32_t _send_bytes = 0;


void service_run(IOScheduler* pctx) {
    LOG_INFO("start event loop thread!");
    pctx->run();
    LOG_INFO("exit loop thread!");
}



void tcp_recv(TcpConnectionPtr conn, IOBuf* pb, size_t n) {
    // LOG_DEBUG("client recv: %s, len: %zu", pb->begin_read(), n);
    // _recv_bytes += n;
    pb->has_readed(n);
}
void tcp_send(TcpConnectionPtr conn, int status, BufPtr pb) {
    if (0 == status) {
        // pb->begin_read();
        // pb->readable_bytes();
        // LOG_INFO("client send bytes: %zu", pb->readable_bytes());
        // _send_bytes += bp->readable_bytes();
    } else {
        if (status > 0) {
            LOG_DEBUG("tcp send buffer full: %d", status);
        } else {
            LOG_ERROR("tcp connection send failed: %d", status);
        }
    }
}
void tcp_close(TcpConnectionPtr conn) {
    LOG_DEBUG("tcp connection close!");
    // release conn
    _connections.erase(conn);
}
void tcp_connect(TcpConnectionPtr conn, int status) {
    if (0 == status) {
        LOG_DEBUG("tcp connect success!");
        conn->recv_callback(tcp_recv);
        conn->send_callback(tcp_send);
        conn->close_callback(tcp_close);
        // conn->send("hellomedia", 11);
    } else {
        LOG_DEBUG("tcp connect failed: %d", status);
        _connections.erase(conn);
    }
}

void timer_func(IOScheduler* pctx) {
    if (_connections.size() < 200) {
        auto conn = std::make_shared<TcpConnection>(pctx);
        conn->connect_callback(tcp_connect);
        // conn->close_callback(tcp_close);
        conn->start("192.168.17.17", 7113);
        _connections.insert(conn); // hold
    }

    for (const auto& conn : _connections) {
        conn->send("hellomedia", 11);
    }
}
void timer_close(IOTimer* t) {
    LOG_DEBUG("timer close");
}
void close_all_connection() {
    LOG_DEBUG("close all tcp: %zu", _connections.size());
    for (const auto& conn : _connections) {
        conn->stop();
    }
    // _connections.clear();
}

// void timer_stat() {
//     LOG_INFO("tcp send: %u, recv: %u", _send_bytes, _recv_bytes);
// }


int main(int argc, char* argv[]) {
    int ret = log_init("gb28181");
    if (0 != ret) {
        printf("log init error\n");
        return -1;
    }
    LOG_INFO("start tcp client");

#if 1
    IOScheduler ctx;
    ctx.init();
    LOG_TRACE("IOScheduler size: %zu, AsyncTask: %zu", sizeof(IOScheduler), sizeof(IOScheduler::AsyncTask));

    IOTimer t1(&ctx);
    t1.start(std::bind(timer_func, &ctx), 0, 1000);

    // IOTimer t2(&ctx);
    // t2.start(timer_release, 0, 2000);

    // TcpConnection conn(&ctx);
    // conn.connect_callback(tcp_connect);
    // conn.close_callback(tcp_close);
    // conn.start("192.168.17.21", 7113);

    std::thread trd(service_run, &ctx);
    // std::this_thread::sleep_for(std::chrono::milliseconds(6000));
    getchar();

    // conn.stop();
    ctx.dispatch(close_all_connection);
    t1.stop(timer_close);
    // t2.stop(timer_close);
    ctx.stop();
    trd.join();

    // while (!_send_buf.empty()) {
    //     _send_buf.pop();
    // }

    // std::this_thread::sleep_for(std::chrono::milliseconds(1000));
#endif

    log_uninit();
    return 0;
}





