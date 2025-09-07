#ifndef SDPF_IOSIGNALIMP_H
#define SDPF_IOSIGNALIMP_H

#include "uv.h"

#include <functional>
#include <atomic>


// namespace sdpf {


class IOSignalImp {
public:
    using SignalTask = std::function<void(int)>;
    using SignalCloseCallback = std::function<void(IOSignal*)>;

    IOSignalImp(IOSignal* pif, IOScheduler* pctx);
    ~IOSignalImp();

    IOSignalImp(const IOSignalImp&) = delete;
    IOSignalImp& operator=(const IOSignalImp&) = delete;
    //IOSignalImp(IOSignalImp&&) = delete;
    //IOSignalImp& operator=(IOSignalImp&&) = delete;

    int start(SignalTask cb, int signum);
    int stop(SignalCloseCallback cb = nullptr);

private:
    void on_start(SignalTask cb, int signum);
    void on_stop(SignalCloseCallback cb);
    void on_signal(int signum);
    void on_close();


    IOSignal* pif_;

    IOScheduler* context_;
    uv_signal_t handle_;
    std::atomic_bool active_;

    SignalTask cb_;
    int signum_;

    SignalCloseCallback close_cb_;
};

// } // namespace sdpf

#endif // SDPF_IOSIGNALIMP_H