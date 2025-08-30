#ifndef SDPF_TCPCONNECTIONIMP_H
#define SDPF_TCPCONNECTIONIMP_H


#include "uv.h"

#include <atomic>
#include <queue>
// #include <any>


// namespace sdpf {


class TcpConnectionImp {
public:
    using ConnectCallback = std::function<void(TcpConnectionPtr, int)>;
    using RecvCallback = std::function<void(TcpConnectionPtr, IOBuf*, size_t)>;
    using SendCallback = std::function<void(TcpConnectionPtr, int, BufPtr)>;
    using CloseCallback = std::function<void(TcpConnectionPtr)>;

    TcpConnectionImp(IOContext* pctx);
    ~TcpConnectionImp();

    TcpConnectionImp(const TcpConnectionImp&) = delete;
    TcpConnectionImp& operator=(const TcpConnectionImp&) = delete;
    // TcpConnectionImp(TcpConnectionImp&&) = delete;
    // TcpConnectionImp& operator=(TcpConnectionImp&&) = delete;

    void set_shared(TcpConnectionPtr ptr);

    void connect_callback(ConnectCallback cb);
    void recv_callback(RecvCallback cb);
    void send_callback(SendCallback cb);
    void close_callback(CloseCallback cb);

    int start(const char* ip_str, uint16_t port); // start connect
    int stop();
    int send(const char* data, size_t n);
    int send(const BufPtr& buf);

    IOContext* context();
    uv_tcp_t* handle();
    bool connected();
    void data(void* data);
    void* data() const;

    // internal call by server accept
    // public wrap for on_connect
    void accept_connect(int status);

private:
    void on_start(std::shared_ptr<SocketAddr> ptr);
    void on_stop();
    void on_send(BufPtr buf); // const BufPtr&

    void on_connect(int status);
    void on_alloc(size_t s_size, uv_buf_t* buf);
    void on_read(ssize_t nread, const uv_buf_t* buf);
    void on_write(int status);
    void on_close();

    void send_buf();
    void clear_buf();


    TcpConnectionPtr pif_;

    IOContext* context_;
    uv_tcp_t handle_;
    uv_connect_t connect_req_;
    uv_write_t write_req_;
    // std::atomic_bool started_;
    std::atomic_bool connected_;

    SocketAddr addr_;

    IOBuf recv_buf_;
    std::queue<BufPtr> send_buf_;
    // std::mutex send_mutex_;

    ConnectCallback conn_cb_;
    RecvCallback recv_cb_;
    SendCallback send_cb_;
    CloseCallback close_cb_;

    void* data_; // std::any for c++17
};


// } // namespace sdpf

#endif // SDPF_TCPCONNECTIONIMP_H