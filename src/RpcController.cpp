#include "RpcController.h"
#include "RpcControllerImp.h"
// #include "logger.h"

// using namespace sdpf;


RpcController::RpcController() {
    imp_ = new RpcControllerImp();
}

RpcController::~RpcController() {
    // LOG_TRACE("RpcController dtor");
    if (imp_) {
        delete imp_;
        imp_ = nullptr;
    }
}

void RpcController::resp(RpcMessage* resp) {
    imp_->resp(resp);
}

RpcMessage* RpcController::resp() const {
    return imp_->resp();
}

void RpcController::done(DoneCallback cb) {
    imp_->done(cb);
}

RpcController::DoneCallback RpcController::done() const {
    return imp_->done();
}

void RpcController::ec(int ec) {
    imp_->ec(ec);
}

int RpcController::ec() const {
    return imp_->ec();
}

void RpcController::text(const char* str) {
    imp_->text(str);
}

const char* RpcController::text() {
    return imp_->text();
};

bool RpcController::failed() const {
    return imp_->failed();
}

void RpcController::lock() {
    imp_->lock();
}

void RpcController::unlock() {
    imp_->unlock();
}

int RpcController::wait() {
    return imp_->wait();
}

void RpcController::notify() {
    imp_->notify();
}


RpcControllerImp::RpcControllerImp()
    : response_(nullptr)
    , done_(nullptr)
    , ec_(0) {
}

RpcControllerImp::~RpcControllerImp() {
    // LOG_TRACE("RpcControllerImp dtor");
}

// void RpcControllerImp::reset() {
//     // if (nullptr != response_) {
//     //     delete response_;
//     //     response_ = nullptr;
//     // }
//     response_ = nullptr;
//     done_ = nullptr;

//     text_.clear();
//     ec_ = 0;
//     // log_id_ = 0;
// }

void RpcControllerImp::resp(RpcMessage* resp) {
    response_ = resp;
}

RpcMessage* RpcControllerImp::resp() const {
    return response_;
}

void RpcControllerImp::done(DoneCallback done) {
    done_ = done;
}

RpcControllerImp::DoneCallback RpcControllerImp::done() const {
    return done_;
}

void RpcControllerImp::ec(int ec) {
    ec_ = ec;
}

int RpcControllerImp::ec() const {
    return ec_;
}

void RpcControllerImp::text(const char* str) {
    text_ = str;
    // ec_ = -1; // FIXME: map code?
}

const char* RpcControllerImp::text() {
    return text_.c_str();
};

bool RpcControllerImp::failed() const {
    return (0 != ec_);
}

void RpcControllerImp::lock() {
    mutex_.lock();
}

void RpcControllerImp::unlock() {
    mutex_.unlock();
}

int RpcControllerImp::wait() {
    std::unique_lock<std::mutex> lk(mutex_, std::adopt_lock); // already-locked mutex
    // cond_.wait(lock, [this](){ return (!queue_.empty() || exit_); });
    auto ret = cond_.wait_for(lk, std::chrono::milliseconds(3000));
    if (std::cv_status::timeout == ret) { // std::cv_status::no_timeout
        return -1;
    }

    return 0;
}

void RpcControllerImp::notify() {
    std::unique_lock<std::mutex> lk(mutex_);
    lk.unlock();
    cond_.notify_one();
}