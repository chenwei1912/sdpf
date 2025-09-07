#include "IOSignal.h"
#include "IOSignalImp.h"
#include "logger.h"

#include "IOScheduler.h"

// using namespace sdpf;


IOSignal::IOSignal(IOScheduler* pctx) {
    imp_ = new IOSignalImp(this, pctx);
}

IOSignal::~IOSignal() {
    //LOG_TRACE("IOSignal {} destructing", static_cast<void*>(this));
    if (imp_) {
        delete imp_;
        imp_ = nullptr;
    }
}

int IOSignal::start(SignalTask cb, int signum) {
    return imp_->start(cb, signum);
}

int IOSignal::stop(SignalCloseCallback cb) {
    return imp_->stop(cb);
}


IOSignalImp::IOSignalImp(IOSignal* pif, IOScheduler* pctx)
            : pif_(pif), context_(pctx), active_(false) {
}

IOSignalImp::~IOSignalImp() {
    //LOG_TRACE("IOSignalImp {} destructing", static_cast<void*>(this));
}

int IOSignalImp::start(SignalTask cb, int signum) {
    if (nullptr == context_ || !context_->is_init()) {
        return -1;
    }
    if (active_) { // 0 != uv_is_active((uv_handle_t*)&handle_)
        return -2;
    }

    int ret = context_->dispatch(std::bind(&IOSignalImp::on_start, this, cb, signum));
    if (0 != ret) {
        LOG_ERROR("IOSignal::start dispatch failed: %d", ret);
        return -3;
    }

    // active_ = true;
    return 0;
}

int IOSignalImp::stop(SignalCloseCallback cb) {
    if (nullptr == context_ || !context_->is_init()) {
        return -1;
    }
    if (!active_) {
        return -2;
    }

    int ret = context_->dispatch(std::bind(&IOSignalImp::on_stop, this, cb));
    if (0 != ret) {
        LOG_ERROR("IOSignal::stop dispatch failed: %d", ret);
        return -3;
    }

    // active_ = false;
    return 0;
}

void IOSignalImp::on_start(SignalTask cb, int signum) {
    if (active_) {
        return;
    }
    if (0 != uv_is_active((uv_handle_t*)&handle_)) {
        LOG_WARN("signal %d is already started", signum);
    }

    active_ = true;
    cb_ = cb;
    signum_ = signum;

    handle_.data = (void*)this;
    int ret = uv_signal_init((uv_loop_t*)context_->handle(), &handle_);
    if (0 != ret) {
        LOG_ERROR("signal %d init failed: %d, %s", signum_, ret, uv_strerror(ret));
        on_stop(nullptr);
        return;
    }
    uv_unref((uv_handle_t*)&handle_);

    ret = uv_signal_start(&handle_, [](uv_signal_t* h, int signum){
        IOSignalImp* s = (IOSignalImp*)h->data;
        s->on_signal(signum);
    }, signum_);
    if (0 != ret) {
        LOG_ERROR("signal %d start failed: %d, %s", signum_, ret, uv_strerror(ret));
        on_stop(nullptr);
        return;
    }
}

void IOSignalImp::on_stop(SignalCloseCallback cb) {
    if (!active_) {
        return;
    }

    active_ = false;
    close_cb_ = cb;
    // LOG_DEBUG("IOSignal %d stop!", signum);

    if (0 != uv_is_active((uv_handle_t*)&handle_)) {
        int ret = uv_signal_stop(&handle_);
        if (0 != ret) {
            LOG_ERROR("signal stop failed: %d, %s", ret, uv_strerror(ret));
        }
    }
    if (0 == (uv_is_closing((uv_handle_t*)&handle_))) {
        uv_close((uv_handle_t*)&handle_, [](uv_handle_t* h) {
            IOSignalImp* s = (IOSignalImp*)h->data;
            s->on_close();
        });
    } else {
        on_close();
    }
}

void IOSignalImp::on_signal(int signum) {
    if (cb_) {
        cb_(signum);
    }
}

void IOSignalImp::on_close() {
    if (close_cb_) {
        close_cb_(pif_);
    }
}
