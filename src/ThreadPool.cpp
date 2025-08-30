#include "ThreadPool.h"


#define POOL_MAXTHREAD_NUM 16


ThreadPool::ThreadPool()
    : max_task_(0)
    , idle_count_(0)
    , grow_(false) {
}

ThreadPool::~ThreadPool() {
    stop();
}

int ThreadPool::start(size_t thread_num, size_t max_task, bool grow) {
    if (thread_num < 1 || !threads_.empty())
        return -1;

    //max_task_ = max_task;
    tasks_.init(max_task);

    //idle_count_ = 0;
    //grow_ = grow;
    return add_thread(thread_num);
}

int ThreadPool::stop() {
    if (threads_.empty())
        return -1;

    tasks_.notify_exit();
    for (auto& item : threads_)
    {
        if (item.joinable()) // item.get_id() != std::thread::id()
            item.join();
    }
    threads_.clear();
    return 0;
}

bool ThreadPool::append(const Task& task) {
//    if (threads_.empty())
//        return false;
//    if (grow_ && idle_count_ < 1)
//        add_thread(1);

    return tasks_.push(task);
}

//// raw parameter for std::bind
//template<typename F, typename... Args>
//bool append(F&& f, Args&&... args) {
//    if (!running_)
//        return false;
//    //auto task = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
//    std::lock_guard<std::mutex> lock(mutex_);
//    if (tasks_.size() >= max_task_)
//        return false;
//    tasks_.emplace(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
//    if (idle_count_ < 1 && grow_)
//        add_thread(1);
//    
//    cond_.notify_one();
//    return true;
//}

void ThreadPool::run() {
    while (true) {
        Task task;
        if (!tasks_.pop(task)) {
            break;
        }
        if (task) {
            //idle_count_--; // and perf timing
            task();
            //idle_count_++; // and perf timing
        }
    }
}

int ThreadPool::add_thread(size_t num) {
    for (size_t i = 0; i < num; ++i) {
        threads_.emplace_back(std::bind(&ThreadPool::run, this));
        //idle_count_++;
    }
    return 0;
}

