#ifndef SDPF_RPCSERVER_H
#define SDPF_RPCSERVER_H

// namespace sdpf {
// namespace rpc {

#include <functional>


class IOContext;
class RpcService;
class RpcServerImp;

class RpcServer {
public:
    using LaunchCallback = std::function<void(RpcServer*, int)>;
    using CloseCallback = std::function<void(RpcServer*)>;

    explicit RpcServer(IOContext* pctx);
    ~RpcServer();

    void launch_callback(LaunchCallback cb);
    void close_callback(CloseCallback cb);

    int start(const char* strip, unsigned short port);
    int stop();

    int register_service(RpcService* svc);

private:
    RpcServerImp* imp_;
};

// }// namespace rpc
// }// namespace sdpf

#endif // SDPF_RPCSERVER_H