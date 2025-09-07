#ifndef SDPF_RPCCHANNELIMP_H
#define SDPF_RPCCHANNELIMP_H

// #include "TcpConnection.h"
#include "Dispatcher.h"

#include <atomic>
#include <mutex>
#include <unordered_map>

// namespace sdpf {
// namespace rpc {

// class RpcService;
// class RpcMessage;
// class RpcController;

// class RpcChannel;
// using RpcChannelPtr = std::shared_ptr<RpcChannel>;

class RpcChannelImp {
public:
    using ConnectCallback = std::function<void(RpcChannelPtr, int)>;
    using CloseCallback = std::function<void(RpcChannelPtr)>;

    explicit RpcChannelImp(IOScheduler* pctx);
    ~RpcChannelImp();

    RpcChannelImp(const RpcChannelImp&) = delete;
    RpcChannelImp& operator=(const RpcChannelImp&) = delete;
    // RpcChannelImp(RpcChannelImp&&) = default;
    // RpcChannelImp& operator=(RpcChannelImp&&) = default;

    void set_shared(RpcChannelPtr ptr);

    void connect_callback(ConnectCallback cb);
    void close_callback(CloseCallback cb);

    int start(const char* ip_str, uint16_t port); // client side
    int stop();

    int call_method(const char* service, const char* method, RpcMessage* request, RpcMessage* response,
                    RpcController* ctrl); // client side
    int notify(RpcService* psvc, RpcMessage* msg); // server side

    TcpConnectionPtr connection();

    // call by server
    void accept_connect();
    void set_services(std::vector<RpcService*>* services);

private:
    void on_connect(TcpConnectionPtr conn, int status);
    void on_recv(TcpConnectionPtr conn, IOBuf* pb, size_t n);
    void on_send(TcpConnectionPtr conn, int status, BufPtr spb);
    void on_close(TcpConnectionPtr conn);

    int parse(IOBuf* pb);
    void process_request(uint32_t id, const char* service, const char* method, const char* buf, size_t len);
    void process_response(uint32_t id, int code, const char* text, const char* buf, size_t len);
    void done(uint32_t id, RpcController* ctrl); // server side
    void send_resp(uint32_t id, int code, RpcMessage* response); // server side
    int send_packet(Meta_Rpc* meta, RpcMessage* msg); // send request and response

    RpcController* remove_request(uint32_t id);
    RpcService* find_svc(const char* name);


    RpcChannelPtr pif_;
    TcpConnectionPtr conn_;

    std::atomic<uint32_t> id_;

    std::unordered_map<uint32_t, void*> requests_; // client: RpcController* server: RpcService*
    std::mutex mutex_requests_;

    std::vector<RpcService*>* services_;

    ConnectCallback conn_cb_;
    CloseCallback close_cb_;

    // void* data_;
};

// } // namespace rpc
// } // namespace sdpf

#endif // SDPF_RPCCHANNELIMP_H