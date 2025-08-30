#ifndef SDPF_RPCSERVICE_H
#define SDPF_RPCSERVICE_H

#include <functional>

// namespace sdpf {
// namespace rpc {

class RpcMessage;
class RpcController;

class RpcService {
public:
    using NotifyCallback = std::function<void(RpcMessage*)>;

    RpcService() {};
    virtual ~RpcService() {};

    virtual const char* name() = 0;
    virtual int find_method(const char* method) = 0;

    virtual RpcMessage* new_request(int method_id) = 0;
    // virtual void del_request(int method_id, RpcMessage* request) = 0;
    virtual RpcMessage* new_response(int method_id) = 0;
    // virtual void del_response(int method_id, RpcMessage* response) = 0;
    virtual void call_method(int method_id, RpcMessage* request, RpcMessage* response, RpcController* ctrl) = 0;

    virtual void notify(RpcMessage* msg) = 0;
    void bind(NotifyCallback cb) { notify_cb_ = cb; };

protected:
    NotifyCallback notify_cb_;
};

// }// namespace rpc
// }// namespace sdpf

#endif // SDPF_RPCSERVICE_H