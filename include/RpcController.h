#ifndef SDPF_RPCCONTROLLER_H
#define SDPF_RPCCONTROLLER_H

#include <functional>

// namespace sdpf {
// namespace rpc {

class RpcMessage;
class RpcControllerImp;

class RpcController {
public:
    using DoneCallback = std::function<void()>;

    RpcController();
    ~RpcController();
    RpcController(const RpcController&) = delete;
    RpcController& operator=(const RpcController&) = delete;
    // RpcController(RpcController&&) = default;
    // RpcController& operator=(RpcController&&) = default;

    void resp(RpcMessage* resp);
    RpcMessage* resp() const;

    void done(DoneCallback cb);
    DoneCallback done() const;

    void ec(int ec);
    int ec() const;

    void text(const char* str); // server side
    const char* text();

    bool failed() const; // client side

    // for internal, user can't call
    void lock();
    void unlock();
    int wait();
    void notify();

private:
    RpcControllerImp* imp_;
};

// typedef std::shared_ptr<RpcController> RpcControllerPtr;

// }// namespace rpc
// }// namespace sdpf

#endif // SDPF_RPCCONTROLLER_H