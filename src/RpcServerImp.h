#ifndef SDPF_RPCSERVERIMP_H
#define SDPF_RPCSERVERIMP_H

#include "TcpServer.h"
#include "RpcChannel.h"

#include <unordered_set>

// namespace sdpf {
// namespace rpc {

class RpcServer;

class RpcServerImp {
public:
    using LaunchCallback = std::function<void(RpcServer*, int)>;
    using CloseCallback = std::function<void(RpcServer*)>;

    explicit RpcServerImp(RpcServer* pif, IOScheduler* pctx);
    ~RpcServerImp();

    void launch_callback(LaunchCallback cb);
    void close_callback(CloseCallback cb);

    int start(const char* strip, unsigned short port);
    int stop();

    int register_service(RpcService* svc);

private:
    void on_launch(TcpServer* svr, int status);
    TcpConnectionPtr on_alloc();
    void on_accept(TcpConnectionPtr conn, int status);
    void on_channel_connect(RpcChannelPtr chn, int status);
    void on_channel_close(RpcChannelPtr chn);
    void on_close(TcpServer* svr);

    void notify_all(RpcService* svc, RpcMessage* msg);
    void close_all();


    RpcServer* pif_;

    TcpServer tcp_server_;

    std::unordered_set<RpcChannelPtr> channels_;
    std::vector<RpcService*> services_;

    LaunchCallback launch_cb_;
    CloseCallback close_cb_;
};

// }// namespace rpc
// }// namespace sdpf

#endif // SDPF_RPCSERVERIMP_H