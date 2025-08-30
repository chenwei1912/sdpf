#include "IOBuf.h"
#include "IOBufImp.h"
// #include "logger.h"

// #include <algorithm>
#include <string.h>


// using namespace sdpf;

const size_t IOBuf::initialSize = 1024;
static const size_t _Max_Size = IOBuf::initialSize * 4;



IOBuf::IOBuf(size_t init) {
    imp_ = new IOBufImp(init);
}

IOBuf::~IOBuf() {
    // LOG_TRACE("IOBuf dtor!");
    if (imp_) {
        delete imp_;
        imp_ = nullptr;
    }
}

void IOBuf::swap(IOBuf& rhs) {
    std::swap(imp_, rhs.imp_);
}

const char* IOBuf::begin_read() const {
    return imp_->begin_read();
}

size_t IOBuf::readable_bytes() const {
    return imp_->readable_bytes();
}

int IOBuf::has_readed(size_t n) {
    return imp_->has_readed(n);
}

void IOBuf::has_readall() {
    imp_->has_readall();
}

int IOBuf::read(char* data, size_t n) {
    return imp_->read(data, n);
}

char* IOBuf::begin_write() {
    return imp_->begin_write();
}

size_t IOBuf::writable_bytes() const {
    return imp_->writable_bytes();
}

int IOBuf::has_written(size_t n) {
    return imp_->has_written(n);
}

int IOBuf::write(const char* data, size_t n) {
    return imp_->write(data, n);
}

int IOBuf::ensure_writable(size_t n) {
    return imp_->ensure_writable(n);
}

void IOBuf::shrink() {
    return imp_->shrink();
}


//IOBufImp::IOBufImp()
//    : read_index_(0)
//    , write_index_(0)
//{
//memset(recv_buffer_, 0, sizeof(recv_buffer_));
//}

IOBufImp::IOBufImp(size_t init)
    : buff_(init), read_index_(0), write_index_(0) {
}

IOBufImp::~IOBufImp() {
    // LOG_TRACE("IOBufImp dtor!");
}

void IOBufImp::swap(IOBufImp& rhs) {
    buff_.swap(rhs.buff_);
    std::swap(read_index_, rhs.read_index_);
    std::swap(write_index_, rhs.write_index_);
}

const char* IOBufImp::begin_read() const {
    return begin() + read_index_;
}

size_t IOBufImp::readable_bytes() const {
    return write_index_ - read_index_;
}

int IOBufImp::has_readed(size_t n) {
    // assert(len <= readableBytes());
    if (n > readable_bytes()) {
        return -1;
    }

    read_index_ += n;
    return 0;
}

void IOBufImp::has_readall() {
    read_index_ = write_index_;
}

int IOBufImp::read(char* data, size_t n) {
    if (nullptr == data || 0 == n) {
        return -1;
    }
    if (n > readable_bytes()) {
        return -2;
    }

    memcpy(data, begin_read(), n);
    has_readed(n);
    return 0;
}

char* IOBufImp::begin_write() {
    return begin() + write_index_;
}

size_t IOBufImp::writable_bytes() const {
    return buff_.size() - write_index_;
}

int IOBufImp::has_written(size_t n) {
    if (n > writable_bytes()) {
        return -1;
    }

    write_index_ += n;
    return 0;
}

int IOBufImp::write(const char* data, size_t n) {
    if (nullptr == data || 0 == n) {
        return -1;
    }
    if (0 != ensure_writable(n)) {
        return -2;
    }

    memcpy(begin_write(), data, n); // std::copy(data, data + n, begin_write());
    has_written(n);
    return 0;
}

// void IOBufImp::write(const std::string& str) {
//     write(str.data(), str.size());
// }

// std::string IOBufImp::read_string() {
//     return read_string(readable_bytes());
// }

// std::string IOBufImp::read_string(size_t n) {
//     // assert(n <= readableBytes());
//     size_t len = readable_bytes();
//     if (n > len) {
//         n = len;
//     }
//     std::string str(begin_read(), n);
//     has_readed(n);
//     return str;
// }

int IOBufImp::ensure_writable(size_t n) {
    if (n > writable_bytes() + read_index_) { // all writable space
        return make_space(n);
    }
    if (n > writable_bytes()) {
        move_readable_data();
    }
    return 0;
}

void IOBufImp::shrink() {
    buff_.shrink_to_fit(); // c++11
}

char* IOBufImp::begin() {
    return &buff_[0];
}
const char* IOBufImp::begin() const {
    return &buff_[0];
}

int IOBufImp::make_space(size_t n) {
    // if (read_index_ + writable_bytes() < n) {
    size_t new_size = n - writable_bytes() - read_index_ + buff_.size();
    // printf("read_index: %zu, write_len: %zu, n: %zu, buff_size: %zu, new size: %zu\n",
    //     read_index_, writable_bytes(), n, buff_.size(), new_size);
    if (new_size > _Max_Size) {
        return -1;
    } else {
        size_t multiple = new_size / IOBuf::initialSize;
        size_t remain = new_size % IOBuf::initialSize;
        if (remain > 0) {
            ++multiple;
        }
        buff_.resize(IOBuf::initialSize * multiple);
        move_readable_data();
    }
    // } else {
    //     move_readable_data();
    // }
    return 0;
}

void IOBufImp::move_readable_data() {
    if (0 == read_index_) {
        return;
    }
    size_t count = readable_bytes();
    if (count > 0) {
        memcpy(begin(), begin_read(), count);
    }
    read_index_ = 0;
    write_index_ = count;
}