#ifndef SDPF_TCPCONNECTION_H
#define SDPF_TCPCONNECTION_H

#include "IOBuf.h"

#include <functional>
#include <memory>


// namespace sdpf {


typedef struct {
    char ip[64];
    uint16_t port;
} SocketAddr;


class IOContext;
class TcpConnectionImp;

class TcpConnection;
using TcpConnectionPtr = std::shared_ptr<TcpConnection>;

class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
public:
    using ConnectCallback = std::function<void(TcpConnectionPtr, int)>;
    using RecvCallback = std::function<void(TcpConnectionPtr, IOBuf*, size_t)>;
    using SendCallback = std::function<void(TcpConnectionPtr, int, BufPtr)>;
    using CloseCallback = std::function<void(TcpConnectionPtr)>;

    TcpConnection(IOContext* pctx);
    ~TcpConnection();

    TcpConnection(const TcpConnection&) = delete;
    TcpConnection& operator=(const TcpConnection&) = delete;
    // TcpConnection(TcpConnection&&) = delete;
    // TcpConnection& operator=(TcpConnection&&) = delete;

    void connect_callback(ConnectCallback cb);
    void recv_callback(RecvCallback cb);
    void send_callback(SendCallback cb);
    void close_callback(CloseCallback cb);

    int start(const char* ip_str, uint16_t port); // start connect
    int stop();
    int send(const char* data, size_t n);
    int send(const BufPtr& buf);

    IOContext* context();
    void* handle();
    bool connected();
    void data(void* data);
    void* data();

    // internal call by server accept
    // public wrap for on_connect
    void accept_connect(int status);

private:
    TcpConnectionImp* imp_;
};


// } // namespace sdpf

#endif // SDPF_TCPCONNECTION_H