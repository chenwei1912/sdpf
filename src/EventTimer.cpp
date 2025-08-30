#include "EventTimer.h"
#include "EventTimerImp.h"
#include "logger.h"

#include "IOContext.h"

// using namespace sdpf;


EventTimer::EventTimer(IOContext* pctx) {
    imp_ = new EventTimerImp(this, pctx);
}

EventTimer::~EventTimer() {
    //LOG_TRACE("EventTimer {} destructing", static_cast<void*>(this));
    if (imp_) {
        delete imp_;
        imp_ = nullptr;
    }
}

int EventTimer::start(TimerTask cb, size_t timeout, size_t repeat) {
    return imp_->start(cb, timeout, repeat);
}

int EventTimer::stop(TimerCloseCallback cb) {
    return imp_->stop(cb);
}


EventTimerImp::EventTimerImp(EventTimer* pif, IOContext* pctx)
            : pif_(pif), context_(pctx), active_(false) {
}

EventTimerImp::~EventTimerImp() {
    //LOG_TRACE("EventTimerImp {} destructing", static_cast<void*>(this));
}

int EventTimerImp::start(TimerTask cb, size_t timeout, size_t repeat) {
    if (nullptr == context_ || !context_->is_init()) {
        return -1;
    }
    if (active_) { // 0 != uv_is_active((uv_handle_t*)&handle_)
        return -2;
    }

    int ret = context_->dispatch(std::bind(&EventTimerImp::on_start, this, cb, timeout, repeat));
    if (0 != ret) {
        LOG_ERROR("EventTimer::start dispatch failed: %d", ret);
        return -3;
    }

    // active_ = true;
    return 0;
}

int EventTimerImp::stop(TimerCloseCallback cb) {
    if (nullptr == context_ || !context_->is_init()) {
        return -1;
    }
    if (!active_) {
        return -2;
    }

    int ret = context_->dispatch(std::bind(&EventTimerImp::on_stop, this, cb));
    if (0 != ret) {
        LOG_ERROR("EventTimer::stop dispatch failed: %d", ret);
        return -3;
    }

    // active_ = false;
    return 0;
}

void EventTimerImp::on_start(TimerTask cb, size_t timeout, size_t repeat) {
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
        EventTimerImp* t = (EventTimerImp*)h->data;
        t->on_timer();
    }, timeout, repeat);
    if (0 != ret) {
        LOG_ERROR("uv timer start failed: %d, %s", ret, uv_strerror(ret));
        on_stop(nullptr);
        return;
    }
}

void EventTimerImp::on_stop(TimerCloseCallback cb) {
    if (!active_) {
        return;
    }

    active_ = false;
    close_cb_ = cb;
    // LOG_DEBUG("EventTimer::on_stop()");

    if (0 != uv_is_active((uv_handle_t*)&handle_)) {
        int ret = uv_timer_stop(&handle_);
        if (0 != ret) {
            LOG_ERROR("uv timer stop failed: %d, %s", ret, uv_strerror(ret));
        }
    }
    if (0 == (uv_is_closing((uv_handle_t*)&handle_))) {
        uv_close((uv_handle_t*)&handle_, [](uv_handle_t* h) {
            EventTimerImp* t = (EventTimerImp*)h->data;
            t->on_close();
        });
    } else {
        on_close();
    }
}

void EventTimerImp::on_timer() {
    if (cb_) {
        cb_();
    }
}

void EventTimerImp::on_close() {
    if (close_cb_) {
        close_cb_(pif_);
    }
}
