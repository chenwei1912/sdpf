#ifndef SDPF_EVENTSIGNALIMP_H
#define SDPF_EVENTSIGNALIMP_H

#include "uv.h"

#include <functional>
#include <atomic>


// namespace sdpf {


class EventSignalImp
{
public:
    using SignalTask = std::function<void(int)>;
    using SignalCloseCallback = std::function<void(EventSignal*)>;

    EventSignalImp(EventSignal* pif, IOContext* pctx);
    ~EventSignalImp();

    EventSignalImp(const EventSignalImp&) = delete;
    EventSignalImp& operator=(const EventSignalImp&) = delete;
    //EventSignalImp(EventSignalImp&&) = delete;
    //EventSignalImp& operator=(EventSignalImp&&) = delete;

    int start(SignalTask cb, int signum);
    int stop(SignalCloseCallback cb = nullptr);

private:
    void on_start(SignalTask cb, int signum);
    void on_stop(SignalCloseCallback cb);
    void on_signal(int signum);
    void on_close();


    EventSignal* pif_;

    IOContext* context_;
    uv_signal_t handle_;
    std::atomic_bool active_;

    SignalTask cb_;
    int signum_;

    SignalCloseCallback close_cb_;
};

// } // namespace sdpf

#endif // SDPF_EVENTSIGNALIMP_H