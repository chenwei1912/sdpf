#include "ThreadPool.h"
#include "BlockingQueue.hpp"

#include <thread>
#include <vector>
// #include <atomic>


// #define POOL_MAXTHREAD_NUM 16


class ThreadPoolImp {
public:
    using Task = std::function<void()>;

    ThreadPoolImp();
    ~ThreadPoolImp();

    ThreadPoolImp(const ThreadPoolImp&) = delete;
    ThreadPoolImp& operator=(const ThreadPoolImp&) = delete;
    // ThreadPoolImp(ThreadPoolImp&&) = delete;
    // ThreadPoolImp& operator=(ThreadPoolImp&&) = delete;

    int start(size_t thread_num, size_t max_task = 0, bool grow = false);
    int stop();

    bool append(const Task& task);

private:
    void run();
    int add_thread(size_t num);

    std::vector<std::thread> threads_;
    BlockingQueue<Task> tasks_;
    // std::atomic_bool running_;

    // size_t max_thread_; // max thread number
    size_t max_task_; // max task number in queue
    // std::atomic_int idle_count_;
    bool grow_;
};

ThreadPoolImp::ThreadPoolImp()
    : max_task_(0)
    // , idle_count_(0)
    , grow_(false) {
}

ThreadPoolImp::~ThreadPoolImp() {
    stop();
}

int ThreadPoolImp::start(size_t thread_num, size_t max_task, bool grow) {
    if (thread_num < 1 || !threads_.empty()) {
        return -1;
    }

    //max_task_ = max_task;
    tasks_.init(max_task);

    //idle_count_ = 0;
    //grow_ = grow;
    return add_thread(thread_num);
}

int ThreadPoolImp::stop() {
    if (threads_.empty()) {
        return -1;
    }

    tasks_.notify_exit();
    for (auto& item : threads_) {
        if (item.joinable()) { // item.get_id() != std::thread::id()
            item.join();
        }
    }
    threads_.clear();
    return 0;
}

bool ThreadPoolImp::append(const Task& task) {
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

void ThreadPoolImp::run() {
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

int ThreadPoolImp::add_thread(size_t num) {
    for (size_t i = 0; i < num; ++i) {
        threads_.emplace_back(std::bind(&ThreadPoolImp::run, this));
        //idle_count_++;
    }
    return 0;
}


ThreadPool::ThreadPool() {
    imp_ = new ThreadPoolImp();
}

ThreadPool::~ThreadPool() {
    if (imp_) {
        delete imp_;
        imp_ = nullptr;
    }
}

int ThreadPool::start(size_t thread_num, size_t max_task, bool grow) {
    return imp_->start(thread_num, max_task, grow);
}

int ThreadPool::stop() {
    return imp_->stop();
}

bool ThreadPool::append(const Task& task) {
    return imp_->append(task);
}
