#ifndef SDPF_IOCONTEXT_H
#define SDPF_IOCONTEXT_H

#include <functional>


// namespace sdpf {


class IOContextImp;

class IOContext {
public:
    using AsyncTask = std::function<void()>;

    IOContext();
    ~IOContext();

    IOContext(const IOContext&) = delete;
    IOContext& operator=(const IOContext&) = delete;
    //IOContext(IOContext&&) = delete;
    //IOContext& operator=(IOContext&&) = delete;

    int init();
    int run();
    void stop();

    // enqueue to execute in loop thread
    int post(AsyncTask f);

    // dispatch will call it rightaway if the dispatch-caller
    // was called from loop thread(in the same loop thread),
    // but enqueue otherwise.
    int dispatch(AsyncTask f);

    bool is_init();
    void* handle();

private:
    IOContextImp* imp_;
};

// } // namespace sdpf

#endif // SDPF_IOCONTEXT_H