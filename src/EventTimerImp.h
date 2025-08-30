#ifndef SDPF_EVENTTIMERIMP_H
#define SDPF_EVENTTIMERIMP_H

#include "uv.h"

#include <functional>
#include <atomic>


// namespace sdpf {


class EventTimerImp {
public:
    using TimerTask = std::function<void()>;
    using TimerCloseCallback = std::function<void(EventTimer*)>;

    EventTimerImp(EventTimer* pif, IOContext* pctx);
    ~EventTimerImp();

    EventTimerImp(const EventTimerImp&) = delete;
    EventTimerImp& operator=(const EventTimerImp&) = delete;
    //EventTimerImp(EventTimerImp&&) = delete;
    //EventTimerImp& operator=(EventTimerImp&&) = delete;

    int start(TimerTask cb, size_t timeout, size_t repeat);
    int stop(TimerCloseCallback cb = nullptr);

private:
    void on_start(TimerTask cb, size_t timeout, size_t repeat);
    void on_stop(TimerCloseCallback cb);
    void on_timer();
    void on_close();


    EventTimer* pif_;

    IOContext* context_;
    uv_timer_t handle_;
    std::atomic_bool active_;

    TimerTask cb_;
    size_t timeout_; // unit: millisecond
    size_t repeat_;

    TimerCloseCallback close_cb_;
};

// } // namespace sdpf

#endif // SDPF_EVENTTIMERIMP_H