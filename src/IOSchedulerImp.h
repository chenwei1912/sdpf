#ifndef SDPF_IOSCHEDULERIMP_H
#define SDPF_IOSCHEDULERIMP_H

#include "uv.h"

#include <functional>
#include <thread>
#include <vector>
#include <mutex>
#include <atomic>


// namespace sdpf {

class IOSchedulerImp
{
public:
    using AsyncTask = std::function<void()>;

    IOSchedulerImp();
    ~IOSchedulerImp();

    IOSchedulerImp(const IOSchedulerImp&) = delete;
    IOSchedulerImp& operator=(const IOSchedulerImp&) = delete;
    //IOSchedulerImp(IOSchedulerImp&&) = delete;
    //IOSchedulerImp& operator=(IOSchedulerImp&&) = delete;

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

// } // namespace sdpf

#endif // SDPF_IOSCHEDULERIMP_H