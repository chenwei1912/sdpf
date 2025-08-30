#ifndef SDPF_IOCONTEXTIMP_H
#define SDPF_IOCONTEXTIMP_H

#include "uv.h"

#include <functional>
#include <thread>
#include <vector>
#include <mutex>
#include <atomic>


// namespace sdpf {

class IOContextImp
{
public:
    using AsyncTask = std::function<void()>;

    IOContextImp();
    ~IOContextImp();

    IOContextImp(const IOContextImp&) = delete;
    IOContextImp& operator=(const IOContextImp&) = delete;
    //IOContextImp(IOContextImp&&) = delete;
    //IOContextImp& operator=(IOContextImp&&) = delete;

    int init();
    int run();
    void stop();

    int post(AsyncTask f);
    int dispatch(AsyncTask f);

    bool is_init();
    uv_loop_t* handle();

private:
    bool in_loop_thread();
    void on_async();
    void on_stop();
    void on_close();

    uv_loop_t loop_;
    uv_async_t handle_;

    std::atomic_bool init_;
    std::thread::id loop_thread_id_;

    std::vector<AsyncTask> queue_task_;
    std::mutex mutex_;

};


// class EventTimer
// {
// public:
//     using TimerTask = std::function<void()>;
//     using TimerCloseCallback = std::function<void(EventTimer*)>;

//     EventTimer(IOContext* pctx);
//     ~EventTimer();

//     EventTimer(const EventTimer&) = delete;
//     EventTimer& operator=(const EventTimer&) = delete;
//     //EventTimer(EventTimer&&) = delete;
//     //EventTimer& operator=(EventTimer&&) = delete;

//     int start(TimerTask cb, size_t timeout, size_t repeat);
//     int stop(TimerCloseCallback cb = nullptr);

// private:
//     void on_start(TimerTask cb, size_t timeout, size_t repeat);
//     void on_stop(TimerCloseCallback cb);
//     void on_timer();
//     void on_close();

//     IOContext* context_;
//     uv_timer_t handle_;
//     std::atomic_bool active_;

//     TimerTask cb_;
//     size_t timeout_; // unit: millisecond
//     size_t repeat_;

//     TimerCloseCallback close_cb_;
// };


// class EventSignal
// {
// public:
//     using SignalTask = std::function<void(int)>;
//     using SignalCloseCallback = std::function<void(EventSignal*)>;

//     EventSignal(IOContext* pctx);
//     ~EventSignal();

//     EventSignal(const EventSignal&) = delete;
//     EventSignal& operator=(const EventSignal&) = delete;
//     //EventSignal(EventSignal&&) = delete;
//     //EventSignal& operator=(EventSignal&&) = delete;

//     int start(SignalTask cb, int signum);
//     int stop(SignalCloseCallback cb = nullptr);

// private:
//     void on_start(SignalTask cb, int signum);
//     void on_stop(SignalCloseCallback cb);
//     void on_signal(int signum);
//     void on_close();

//     IOContext* context_;
//     uv_signal_t handle_;
//     std::atomic_bool active_;

//     SignalTask cb_;
//     int signum_;

//     SignalCloseCallback close_cb_;
// };

// } // namespace sdpf

#endif // SDPF_IOCONTEXTIMP_H