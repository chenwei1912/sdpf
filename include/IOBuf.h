#ifndef SDPF_IOBUF_H
#define SDPF_IOBUF_H

#include <memory>

// namespace sdpf {

class IOBufImp;

class IOBuf {
public:
    static const size_t initialSize;

    explicit IOBuf(size_t init = initialSize);
    ~IOBuf();
    // IOBuf(const IOBuf&) = delete;
    // IOBuf& operator=(const IOBuf&) = delete;
    // IOBuf(IOBuf&&) = delete;
    // IOBuf& operator=(IOBuf&&) = delete;

    void swap(IOBuf& rhs);

    const char* begin_read() const;
    size_t readable_bytes() const;
    int has_readed(size_t n);
    void has_readall();
    int read(char* data, size_t n);
    // int read(std::string& str);

    char* begin_write();
    size_t writable_bytes() const; // remain space from write pointer
    int has_written(size_t n);
    int write(const char* data, size_t n);
    // int write(const std::string& str);

    // std::string read_string();
    // std::string read_string(size_t n);

    int ensure_writable(size_t n);
    void shrink();

private:
    IOBufImp* imp_;
};

typedef std::shared_ptr<IOBuf> BufPtr;

// } // namespace sdpf

#endif // SDPF_IOBUF_H