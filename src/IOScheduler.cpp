#include "IOScheduler.h"
#include "IOSchedulerImp.h"
#include "logger.h"


// using namespace sdpf;


IOScheduler::IOScheduler() {
    imp_ = new IOSchedulerImp();
}

IOScheduler::~IOScheduler() {
    //LOG_TRACE("IOScheduler {} destructing", static_cast<void*>(this));
    if (imp_) {
        delete imp_;
        imp_ = nullptr;
    }
}

int IOScheduler::init() {
    return imp_->init();
}

int IOScheduler::run() {
    return imp_->run();
}

void IOScheduler::stop() {
    imp_->stop();
}

int IOScheduler::post(AsyncTask f) {
    return imp_->post(f);
}

int IOScheduler::dispatch(AsyncTask f) {
    return imp_->dispatch(f);
}

bool IOScheduler::is_init() {
    return imp_->is_init();
}

void* IOScheduler::handle() {
    return imp_->handle();
}


IOSchedulerImp::IOSchedulerImp()
            : init_(false) {
}

IOSchedulerImp::~IOSchedulerImp() {
    //LOG_TRACE("IOSchedulerImp {} destructing", static_cast<void*>(this));
}

int IOSchedulerImp::init() {
    if (init_) { // 0 != uv_is_active((uv_handle_t*)&handle_);
        return -1;
    }

    int ret = uv_loop_init(&loop_);
    if (0 != ret) {
        LOG_ERROR("uv loop init failed: %s", uv_strerror(ret));
        return -2;
    }

    handle_.data = (void*)this;
    ret = uv_async_init(&loop_, &handle_, [](uv_async_t* h){
        IOSchedulerImp* ctx = (IOSchedulerImp*)h->data;
        ctx->on_async();
    });
    if (0 != ret) {
        LOG_ERROR("uv async init failed: %s", uv_strerror(ret));
        uv_loop_close(&loop_);
        return -3;
    }

    loop_thread_id_ = std::thread::id();
    init_ = true;
    return 0;
}

int IOSchedulerImp::run() {
    if (!init_) {
        return -1;
    }
    // if (loop_thread_id_ != std::thread::id()) { // running
    //     return -2;
    // }

    LOG_INFO("IOScheduler start running");
    loop_thread_id_ = std::this_thread::get_id();

    int ret = uv_run(&loop_, UV_RUN_DEFAULT); // not reentrant, not in callback
    LOG_INFO("IOScheduler uv_run exit: %d", ret);

    ret = uv_loop_close(&loop_);
    if (0 != ret) { // return UV_EBUSY if exists active handle
        LOG_ERROR("uv_loop_close failed: %d, %s", ret, uv_strerror(ret));
    }

    init_ = false;
    loop_thread_id_ = std::thread::id();
    return 0;
}

void IOSchedulerImp::stop() {
    if (init_) {
        dispatch(std::bind(&IOSchedulerImp::on_stop, this));
        // init_ = false;
    }
}

int IOSchedulerImp::post(AsyncTask f) {
    if (!f || !init_) {
        return -1;
    }

    {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_task_.emplace_back(std::move(f));
    }

    // only one thread safe
    int ret = uv_async_send(&handle_);
    if (0 != ret) {
        LOG_ERROR("uv_async_send failed: %d, %s", ret, uv_strerror(ret));
        return -2;
    }
    return 0;
}

int IOSchedulerImp::dispatch(AsyncTask f) {
    if (!f || !init_) {
        return -1;
    }

    int ret = 0;
    if (in_loop_thread()) {
        f();
    } else {
        ret = post(f);
    }
    return ret;
}

bool IOSchedulerImp::is_init() {
    return init_;
}

uv_loop_t* IOSchedulerImp::handle() {
    return &loop_;
}

bool IOSchedulerImp::in_loop_thread() {
    return (loop_thread_id_ == std::this_thread::get_id());
}

void IOSchedulerImp::on_async() {
    std::vector<AsyncTask> temp;
    {
    std::lock_guard<std::mutex> lock(mutex_);
    temp.swap(queue_task_);
    }

    for (auto& item : temp) {
        item();
    }
}

void IOSchedulerImp::on_stop() {
    if (!init_) {
        return;
    }
    init_ = false;
    // LOG_DEBUG("IOScheduler::on_stop()");

    if (0 == (uv_is_closing((uv_handle_t*)&handle_))) {
        uv_close((uv_handle_t*)&handle_, [](uv_handle_t* h) {
            IOSchedulerImp* loop = (IOSchedulerImp*)h->data;
            loop->on_close();
        });
    } else {
        on_close();
    }
}

void IOSchedulerImp::on_close() {
    uv_stop(&loop_);

    // close remain active handle_t
    // call uv_TYPE_stop and uv_close to all active handle_t
    // uv_walk(loop, [](uv_handle_t* handle, void*) {
    //     if (0 == uv_is_closing(handle)) {
    //       uv_close(handle, nullptr);
    //     }
    // }, nullptr);
}
