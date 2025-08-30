#ifndef SDPF_IOBUFIMP_H
#define SDPF_IOBUFIMP_H

#include <vector>
#include <memory>
#include <string>


// namespace sdpf {

class IOBufImp {
public:
    explicit IOBufImp(size_t init);
    ~IOBufImp();
    // IOBufImp(const IOBufImp&) = delete;
    // IOBufImp& operator=(const IOBufImp&) = delete;
    // IOBufImp(IOBufImp&&) = delete;
    // IOBufImp& operator=(IOBufImp&&) = delete;

    void swap(IOBufImp& rhs);

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
    char* begin();
    const char* begin() const;

    int make_space(size_t n);
    void move_readable_data();

    std::vector<char> buff_;
    size_t read_index_;
    size_t write_index_;
};

// } // namespace sdpf

#endif // SDPF_IOBUFIMP_H