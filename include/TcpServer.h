#ifndef SDPF_TCPSERVER_H
#define SDPF_TCPSERVER_H

#include "TcpConnection.h"


// namespace sdpf {

// class IOContext;
class TcpServerImp;

class TcpServer {
public:
    using LaunchCallback = std::function<void(TcpServer*, int)>;
    using AllocCallback = std::function<TcpConnectionPtr()>;
    using AcceptCallback = std::function<void(TcpConnectionPtr, int)>;
    using CloseCallback = std::function<void(TcpServer*)>;

    explicit TcpServer(IOContext* pctx);
    ~TcpServer();

    TcpServer(const TcpServer&) = delete;
    TcpServer& operator=(const TcpServer&) = delete;
    // TcpServer(TcpServer&&) = delete;
    // TcpServer& operator=(TcpServer&&) = delete;

    void launch_callback(LaunchCallback cb);
    void alloc_callback(AllocCallback cb);
    void accept_callback(AcceptCallback cb);
    void close_callback(CloseCallback cb);

    // start listen and wait for accept new connection
    int start(const char* ip_str, uint16_t port);
    int stop();

    IOContext* context();
    // void* handle();

private:
    TcpServerImp* imp_;
};

// } // namespace sdpf

#endif // SDPF_TCPSERVER_H