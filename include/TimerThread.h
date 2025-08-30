#ifndef TIMERTHREAD_H
#define TIMERTHREAD_H

#include "EventLoop.h"

#include <unordered_set>


typedef std::function<void()> TimerCallback;

class Timer
{
public:
    Timer(TimerCallback cb, size_t timeout, size_t repeat)
        : cb_(cb)
        , timeout_(timeout)
        , repeat_(repeat) {
        handle_.data = (void*)this;
    }

    //Timer() = default;
    ~Timer() {
    }

    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;
    //Timer(Timer&&) = delete;
    //Timer& operator=(Timer&&) = delete;

//    uv_timer_t* get_handle() {
//        return &handle_;
//    }
//    size_t get_repeat() {
//        return repeat_;
//    }

    // use for internal
    void execute_cb();

    friend class TimerThread;

private:
    TimerCallback cb_;
    size_t timeout_; // unit: millisecond
    size_t repeat_;

    uv_timer_t handle_;
};


class TimerThread
{
public:
    static TimerThread* pointer() {
        static TimerThread tm_thread;
        return &tm_thread;
    }

    int start();
    int stop();
    int join();

    Timer* start_timer(TimerCallback cb, size_t timeout, size_t repeat);
    void stop_timer(Timer* t);

    // use for internal
    //void del_timer(Timer* timer);

private:
    TimerThread();
    ~TimerThread();
    TimerThread(const TimerThread&) = delete;
    TimerThread& operator=(const TimerThread&) = delete;
    //TimerThread(TimerThread&&) = delete;
    //TimerThread& operator=(TimerThread&&) = delete;

    void run();
    void clear();

    void start_timer_loop(Timer* timer);
    void stop_timer_loop(Timer* timer);

    std::thread thread_;
    EventLoop loop_;
    std::unordered_set<Timer*> timers_;

};

#endif // TIMERTHREAD_H