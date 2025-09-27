#ifndef SDPF_THREADPOOL_H
#define SDPF_THREADPOOL_H

#include <functional>

// namespace sdpf {

class ThreadPoolImp;

class ThreadPool {
public:
    using Task = std::function<void()>;

    ThreadPool();
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    //ThreadPool(ThreadPool&&) = delete;
    //ThreadPool& operator=(ThreadPool&&) = delete;

    int start(size_t thread_num, size_t max_task = 0, bool grow = false);
    int stop();

    bool append(const Task& task);

private:
    ThreadPoolImp* imp_;
};

// } // namespace sdpf

#endif // SDPF_THREADPOOL_H
