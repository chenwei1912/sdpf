#ifndef SDPF_RPCCHANNEL_H
#define SDPF_RPCCHANNEL_H

#include "TcpConnection.h"

#include <memory>
#include <functional>
#include <vector>

// namespace sdpf {
// namespace rpc {

class IOScheduler;
class RpcService;
class RpcMessage;
class RpcController;

class RpcChannel;
using RpcChannelPtr = std::shared_ptr<RpcChannel>;

class RpcChannelImp;

class RpcChannel : public std::enable_shared_from_this<RpcChannel> {
public:
    using ConnectCallback = std::function<void(RpcChannelPtr, int)>;
    using CloseCallback = std::function<void(RpcChannelPtr)>;

    explicit RpcChannel(IOScheduler* pctx);
    ~RpcChannel();

    RpcChannel(const RpcChannel&) = delete;
    RpcChannel& operator=(const RpcChannel&) = delete;
    // RpcChannel(RpcChannel&&) = default;
    // RpcChannel& operator=(RpcChannel&&) = default;

    void connect_callback(ConnectCallback cb);
    void close_callback(CloseCallback cb);

    int start(const char* ip_str, uint16_t port); // client side
    int stop();

    int call_method(const char* service, const char* method, RpcMessage* request, RpcMessage* response,
                    RpcController* ctrl); // client side
    int notify(RpcService* psvc, RpcMessage* msg); // server side

    TcpConnectionPtr connection();

    // use for internal, call by server
    void accept_connect();
    void set_services(std::vector<RpcService*>* services);

private:
    RpcChannelImp* imp_;
};

// } // namespace rpc
// } // namespace sdpf

#endif // SDPF_RPCCHANNEL_H