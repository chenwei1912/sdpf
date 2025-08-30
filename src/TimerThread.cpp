#include "TimerThread.h"
#include "logger.h"


static void timer_cb(uv_timer_t* handle) {
    Timer* t = (Timer*)handle->data;
    t->execute_cb();
//    if (0 == t->get_repeat()) {
//        TimerThread::get_timerthread()->del_timer(t); // multi timerthread?
//    }
}

static void close_cb(uv_handle_t* handle) {
    Timer* t = (Timer*)handle->data;
    LOG_DEBUG("stop_timer_loop %p",t);
    delete t;
//    if (0 == t->get_repeat()) {
//        TimerThread::get_timerthread()->del_timer(t); // multi timerthread?
//    }
}


void Timer::execute_cb() {
    cb_();

    if (0 == repeat_) {
        LOG_DEBUG("stop_timer_loop timer=%p  timeout_=%ld",this,timeout_);
        TimerThread::pointer()->stop_timer(this);
    }
}


int TimerThread::start() {
    if (thread_.joinable()) {
        return -1;
    }
    thread_ = std::thread(std::bind(&TimerThread::run, this));
    return 0;
}

int TimerThread::stop() {
    if (!thread_.joinable()) {
        return -1;
    }

    loop_.quit();
    return 0;
}

int TimerThread::join() {
    if (!thread_.joinable()) {
        return -1;
    }

    thread_.join();
    return 0;
}

Timer* TimerThread::start_timer(TimerCallback cb, size_t timeout, size_t repeat)
{
    if (!cb 
        || !thread_.joinable()) {
        return nullptr;
    }

    Timer* timer = new (std::nothrow) Timer(cb, timeout, repeat); // std::move(cb)
    if (nullptr == timer) {
        //LOG_ERROR("new Timer failed");
        return nullptr;
    }
    loop_.dispatch(std::bind(&TimerThread::start_timer_loop, this, timer));
    return timer;
}

void TimerThread::stop_timer(Timer* timer)
{
    if (nullptr == timer 
        || !thread_.joinable()) {
        return;
    }
    
    loop_.dispatch(std::bind(&TimerThread::stop_timer_loop, this, timer));
}

//void TimerThread::del_timer(Timer* timer) {
//    auto it = timers_.find(timer);
//    if (it != timers_.end()) {
//        timers_.erase(it);
//        delete timer;
//    }
//}

TimerThread::TimerThread() {
}

TimerThread::~TimerThread() {
    stop();
    join();
    clear();
}

void TimerThread::run() {
    int ret = loop_.init();
    if (0 != ret) {
        //LOG_FATAL("TimerThread run failed: %d", ret);
        return; // abort();
    }

    ret = loop_.loop();
    if (0 != ret) {
        //LOG_FATAL("TimerThread run failed: %d", ret);
        return; // abort();
    }

    clear();
}

void TimerThread::clear() {
    for (auto it = timers_.begin(); it != timers_.end(); ++it) {
        delete (*it);
    }
    timers_.clear();
}


void TimerThread::start_timer_loop(Timer* timer)
{
    uv_timer_t* h = &(timer->handle_);

    uv_timer_init(loop_.get_loop(), h);
    uv_unref((uv_handle_t*)h);

    timers_.insert(timer);

    uv_timer_start(h, timer_cb, timer->timeout_, timer->repeat_);
}

void TimerThread::stop_timer_loop(Timer* timer)
{
    
    LOG_DEBUG("stop_timer_loop add=%p timeout_=%ld repeat_=%ld type=%d closing=%d",timer,timer->timeout_,timer->repeat_,uv_handle_get_type((uv_handle_t*)&timer->handle_),uv_is_closing((uv_handle_t*)&timer->handle_));

        


   
    //del_timer(timer);
    auto it = timers_.find(timer);
    if (it != timers_.end()) {
        LOG_DEBUG("stop_timer_loop %p type=%d",timer,uv_handle_get_type((uv_handle_t*)&timer->handle_));
        uv_timer_stop(&(timer->handle_));
        uv_close((uv_handle_t*)&(timer->handle_), close_cb);

        // if (0 == uv_is_closing((uv_handle_t*)&timer->handle_))
        // {
        //     uv_close((uv_handle_t*)&(timer->handle_), NULL);
        // }
        // else
        // {
        //     LOG_DEBUG("already uv_is_closing %p",timer);
        // }

        timers_.erase(it);
        // delete timer;
    }
}


