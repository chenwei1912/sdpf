#ifndef SDPF_IOTIMER_H
#define SDPF_IOTIMER_H


#include <functional>


// namespace sdpf {

class IOScheduler;
class IOTimerImp;

class IOTimer {
public:
    using TimerTask = std::function<void()>;
    using TimerCloseCallback = std::function<void(IOTimer*)>;

    IOTimer(IOScheduler* pctx);
    ~IOTimer();

    IOTimer(const IOTimer&) = delete;
    IOTimer& operator=(const IOTimer&) = delete;
    //IOTimer(IOTimer&&) = delete;
    //IOTimer& operator=(IOTimer&&) = delete;

    int start(TimerTask cb, size_t timeout, size_t repeat);
    int stop(TimerCloseCallback cb = nullptr);

private:
    IOTimerImp* imp_;
};

// } // namespace sdpf

#endif // SDPF_IOTIMER_H