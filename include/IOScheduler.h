#ifndef SDPF_IOSCHEDULER_H
#define SDPF_IOSCHEDULER_H

#include <functional>


// namespace sdpf {


class IOSchedulerImp;

class IOScheduler {
public:
    using AsyncTask = std::function<void()>;

    IOScheduler();
    ~IOScheduler();

    IOScheduler(const IOScheduler&) = delete;
    IOScheduler& operator=(const IOScheduler&) = delete;
    //IOScheduler(IOScheduler&&) = delete;
    //IOScheduler& operator=(IOScheduler&&) = delete;

    int init();
    int run();
    void stop();

    // enqueue to execute in loop thread
    int post(AsyncTask f);

    // dispatch will call it rightaway if the dispatch-caller
    // was called from loop thread(in the same loop thread),
    // but enqueue otherwise.
    int dispatch(AsyncTask f);

    bool is_init();
    void* handle();

private:
    IOSchedulerImp* imp_;
};

// } // namespace sdpf

#endif // SDPF_IOSCHEDULER_H