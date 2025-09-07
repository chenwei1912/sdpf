#include "IOTimer.h"
#include "IOTimerImp.h"
#include "logger.h"

#include "IOScheduler.h"

// using namespace sdpf;


IOTimer::IOTimer(IOScheduler* pctx) {
    imp_ = new IOTimerImp(this, pctx);
}

IOTimer::~IOTimer() {
    //LOG_TRACE("IOTimer {} destructing", static_cast<void*>(this));
    if (imp_) {
        delete imp_;
        imp_ = nullptr;
    }
}

int IOTimer::start(TimerTask cb, size_t timeout, size_t repeat) {
    return imp_->start(cb, timeout, repeat);
}

int IOTimer::stop(TimerCloseCallback cb) {
    return imp_->stop(cb);
}


IOTimerImp::IOTimerImp(IOTimer* pif, IOScheduler* pctx)
            : pif_(pif), context_(pctx), active_(false) {
}

IOTimerImp::~IOTimerImp() {
    //LOG_TRACE("IOTimerImp {} destructing", static_cast<void*>(this));
}

int IOTimerImp::start(TimerTask cb, size_t timeout, size_t repeat) {
    if (nullptr == context_ || !context_->is_init()) {
        return -1;
    }
    if (active_) { // 0 != uv_is_active((uv_handle_t*)&handle_)
        return -2;
    }

    int ret = context_->dispatch(std::bind(&IOTimerImp::on_start, this, cb, timeout, repeat));
    if (0 != ret) {
        LOG_ERROR("IOTimer::start dispatch failed: %d", ret);
        return -3;
    }

    // active_ = true;
    return 0;
}

int IOTimerImp::stop(TimerCloseCallback cb) {
    if (nullptr == context_ || !context_->is_init()) {
        return -1;
    }
    if (!active_) {
        return -2;
    }

    int ret = context_->dispatch(std::bind(&IOTimerImp::on_stop, this, cb));
    if (0 != ret) {
        LOG_ERROR("IOTimer::stop dispatch failed: %d", ret);
        return -3;
    }

    // active_ = false;
    return 0;
}

void IOTimerImp::on_start(TimerTask cb, size_t timeout, size_t repeat) {
    if (active_) {
        return;
    }
    if (0 != uv_is_active((uv_handle_t*)&handle_)) {
        LOG_WARN("timer is already started");
    }

    active_ = true;
    cb_ = cb;
    timeout_ = timeout;
    repeat_ = repeat;

    handle_.data = (void*)this;
    int ret = uv_timer_init((uv_loop_t*)context_->handle(), &handle_);
    if (0 != ret) {
        LOG_ERROR("uv timer init failed: %d, %s", ret, uv_strerror(ret));
        on_stop(nullptr);
        return;
    }
    uv_unref((uv_handle_t*)&handle_);

    ret = uv_timer_start(&handle_, [](uv_timer_t* h){
        IOTimerImp* t = (IOTimerImp*)h->data;
        t->on_timer();
    }, timeout, repeat);
    if (0 != ret) {
        LOG_ERROR("uv timer start failed: %d, %s", ret, uv_strerror(ret));
        on_stop(nullptr);
        return;
    }
}

void IOTimerImp::on_stop(TimerCloseCallback cb) {
    if (!active_) {
        return;
    }

    active_ = false;
    close_cb_ = cb;
    // LOG_DEBUG("IOTimer::on_stop()");

    if (0 != uv_is_active((uv_handle_t*)&handle_)) {
        int ret = uv_timer_stop(&handle_);
        if (0 != ret) {
            LOG_ERROR("uv timer stop failed: %d, %s", ret, uv_strerror(ret));
        }
    }
    if (0 == (uv_is_closing((uv_handle_t*)&handle_))) {
        uv_close((uv_handle_t*)&handle_, [](uv_handle_t* h) {
            IOTimerImp* t = (IOTimerImp*)h->data;
            t->on_close();
        });
    } else {
        on_close();
    }
}

void IOTimerImp::on_timer() {
    if (cb_) {
        cb_();
    }
}

void IOTimerImp::on_close() {
    if (close_cb_) {
        close_cb_(pif_);
    }
}
