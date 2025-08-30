#ifndef SDPF_EVENTSIGNAL_H
#define SDPF_EVENTSIGNAL_H

#include <functional>


// namespace sdpf {

class IOContext;
class EventSignalImp;

class EventSignal {
public:
    using SignalTask = std::function<void(int)>;
    using SignalCloseCallback = std::function<void(EventSignal*)>;

    EventSignal(IOContext* pctx);
    ~EventSignal();

    EventSignal(const EventSignal&) = delete;
    EventSignal& operator=(const EventSignal&) = delete;
    //EventSignal(EventSignal&&) = delete;
    //EventSignal& operator=(EventSignal&&) = delete;

    int start(SignalTask cb, int signum);
    int stop(SignalCloseCallback cb = nullptr);

private:
    EventSignalImp* imp_;
};

// } // namespace sdpf

#endif // SDPF_EVENTSIGNAL_H