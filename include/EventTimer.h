#ifndef SDPF_EVENTTIMER_H
#define SDPF_EVENTTIMER_H


#include <functional>


// namespace sdpf {

class IOContext;
class EventTimerImp;

class EventTimer {
public:
    using TimerTask = std::function<void()>;
    using TimerCloseCallback = std::function<void(EventTimer*)>;

    EventTimer(IOContext* pctx);
    ~EventTimer();

    EventTimer(const EventTimer&) = delete;
    EventTimer& operator=(const EventTimer&) = delete;
    //EventTimer(EventTimer&&) = delete;
    //EventTimer& operator=(EventTimer&&) = delete;

    int start(TimerTask cb, size_t timeout, size_t repeat);
    int stop(TimerCloseCallback cb = nullptr);

private:
    EventTimerImp* imp_;
};

// } // namespace sdpf

#endif // SDPF_EVENTTIMER_H