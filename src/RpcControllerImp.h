#ifndef SDPF_RPCCONTROLLERIMP_H
#define SDPF_RPCCONTROLLERIMP_H

// #include <functional>
#include <mutex>
#include <condition_variable>

// namespace sdpf {
// namespace rpc {

// class RpcMessage;

class RpcControllerImp {
public:
    using DoneCallback = std::function<void()>;

    RpcControllerImp();
    ~RpcControllerImp();
    RpcControllerImp(const RpcControllerImp&) = delete;
    RpcControllerImp& operator=(const RpcControllerImp&) = delete;
    // RpcControllerImp(RpcControllerImp&&) = default;
    // RpcControllerImp& operator=(RpcControllerImp&&) = default;

    void resp(RpcMessage* resp);
    RpcMessage* resp() const;

    void done(DoneCallback done);
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
    RpcMessage* response_;
    DoneCallback done_;

    int ec_;
    std::string text_;
    // int log_id_;

    std::mutex mutex_;
    std::condition_variable cond_;
};

// }// namespace rpc
// }// namespace sdpf

#endif // SDPF_RPCCONTROLLERIMP_H