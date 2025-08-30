#ifndef SDPF_TCPSERVERIMP_H
#define SDPF_TCPSERVERIMP_H


#include "uv.h"

#include <atomic>


// namespace sdpf {


class TcpServerImp {
public:
    using LaunchCallback = std::function<void(TcpServer*, int)>;
    using AllocCallback = std::function<TcpConnectionPtr()>;
    using AcceptCallback = std::function<void(TcpConnectionPtr, int)>;
    using CloseCallback = std::function<void(TcpServer*)>;

    TcpServerImp(TcpServer* pif, IOContext* pctx);
    ~TcpServerImp();

    TcpServerImp(const TcpServerImp&) = delete;
    TcpServerImp& operator=(const TcpServerImp&) = delete;
    // TcpServerImp(TcpServerImp&&) = delete;
    // TcpServerImp& operator=(TcpServerImp&&) = delete;

    void launch_callback(LaunchCallback cb);
    void alloc_callback(AllocCallback cb);
    void accept_callback(AcceptCallback cb);
    void close_callback(CloseCallback cb);

    // start listen and wait for accept new connection
    int start(const char* ip_str, uint16_t port);
    int stop();

    IOContext* context();
    // uv_tcp_t* handle();

private:
    void on_start(std::shared_ptr<SocketAddr> ptr);
    void on_stop();
    void on_accept(int status);
    void on_close();


    TcpServer* pif_;

    IOContext* context_;
    uv_tcp_t server_;

    std::atomic_bool started_;
    SocketAddr addr_;

    LaunchCallback launch_cb_;
    AllocCallback alloc_cb_;
    AcceptCallback accept_cb_;
    CloseCallback close_cb_;

    // const std::string name_;
};

// } // namespace sdpf

#endif // SDPF_TCPSERVERIMP_H