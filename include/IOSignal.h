#ifndef SDPF_IOSIGNAL_H
#define SDPF_IOSIGNAL_H

#include <functional>


// namespace sdpf {

class IOScheduler;
class IOSignalImp;

class IOSignal {
public:
    using SignalTask = std::function<void(int)>;
    using SignalCloseCallback = std::function<void(IOSignal*)>;

    IOSignal(IOScheduler* pctx);
    ~IOSignal();

    IOSignal(const IOSignal&) = delete;
    IOSignal& operator=(const IOSignal&) = delete;
    //IOSignal(IOSignal&&) = delete;
    //IOSignal& operator=(IOSignal&&) = delete;

    int start(SignalTask cb, int signum);
    int stop(SignalCloseCallback cb = nullptr);

private:
    IOSignalImp* imp_;
};

// } // namespace sdpf

#endif // SDPF_IOSIGNAL_H