#ifndef SDPF_IOTIMERIMP_H
#define SDPF_IOTIMERIMP_H

#include "uv.h"

#include <functional>
#include <atomic>


// namespace sdpf {


class IOTimerImp {
public:
    using TimerTask = std::function<void()>;
    using TimerCloseCallback = std::function<void(IOTimer*)>;

    IOTimerImp(IOTimer* pif, IOScheduler* pctx);
    ~IOTimerImp();

    IOTimerImp(const IOTimerImp&) = delete;
    IOTimerImp& operator=(const IOTimerImp&) = delete;
    //IOTimerImp(IOTimerImp&&) = delete;
    //IOTimerImp& operator=(IOTimerImp&&) = delete;

    int start(TimerTask cb, size_t timeout, size_t repeat);
    int stop(TimerCloseCallback cb = nullptr);

private:
    void on_start(TimerTask cb, size_t timeout, size_t repeat);
    void on_stop(TimerCloseCallback cb);
    void on_timer();
    void on_close();


    IOTimer* pif_;

    IOScheduler* context_;
    uv_timer_t handle_;
    std::atomic_bool active_;

    TimerTask cb_;
    size_t timeout_; // unit: millisecond
    size_t repeat_;

    TimerCloseCallback close_cb_;
};

// } // namespace sdpf

#endif // SDPF_IOTIMERIMP_H