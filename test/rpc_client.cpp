#include "RpcChannel.h"
#include "RpcController.h"
#include "MsgService_data.h"
#include "logger.h"
#include "IOContext.h"

#include <thread>
#include <unordered_set>
#include <stdio.h>
#include <string.h>


#define  CALL_SYNC


std::unordered_set<RpcChannelPtr> _channels;
RpcChannelPtr _chn;
bool _flag;

Msg_Notify* _notify_response = nullptr;
RpcController* _notify_ctrl = nullptr;


class MsgService_Stub {
public:
    static const char* service_;

    explicit MsgService_Stub(RpcChannelPtr channel)
        : chn_(channel) {
    }
    ~MsgService_Stub() {
    }

    void test() {
        // auto ctrl = std::make_unique<RpcController>();
        RpcController* ctrl = new RpcController;
        // auto done = std::bind(&MsgService_Stub::on_done, this, ctrl, nullptr);
        // ctrl->done(done);

        int ret = chn_->call_method(service_, "test", nullptr, nullptr, ctrl);
        if (0 != ret) {
            LOG_DEBUG("client call method failed: %d", ret);
        }

        // delete if sync
        // delete response;
        delete ctrl;
    }

    // void echo(Msg_Echo* request, Msg_Echo* response) {
    void echo() {
        Msg_Echo request;
        request.log_id = 7;
        memset(request.msg, 0, sizeof(request.msg));
        strcpy(request.msg, "hello");

        Msg_Echo* response = new Msg_Echo;
        memset(response->msg, 0, sizeof(response->msg));

        // auto ctrl = std::make_unique<RpcController>();
        RpcController* ctrl = new RpcController;
        // auto done = std::bind(&MsgService_Stub::on_done, this, ctrl, response);
        // ctrl->done(done);

        int ret = chn_->call_method(service_, "echo", &request, response, ctrl);
        if (0 == ret) {
            // LOG_INFO("client echo log_id: %d, result: %s", response->log_id, response->msg);
        } else {
            LOG_DEBUG("channel call method failed: %d", ret);
        }

        // delete if sync
        delete response;
        delete ctrl;
    }

    void register_notify() {
        _notify_response = new Msg_Notify;
        memset(_notify_response->msg, 0, sizeof(_notify_response->msg));

        _notify_ctrl = new RpcController;

        auto done = std::bind(&MsgService_Stub::on_notify, this, _notify_ctrl, _notify_response);
        _notify_ctrl->done(done);

        int ret = chn_->call_method(service_, "notify", nullptr, _notify_response, _notify_ctrl);
        if (0 != ret) {
            LOG_DEBUG("channel call method failed: %d", ret);
            delete _notify_response;
            _notify_response = nullptr;
            delete _notify_ctrl;
            _notify_ctrl = nullptr;
        }

    }

private:
    void on_done(RpcController* ctrl, RpcMessage* response) {
        // LOG_DEBUG("client done call, ec: %d, text: %s", ctrl->ec(), ctrl->text().c_str());
        //std::unique_ptr<RpcMessage> guard_rsp(response); // delete response
        //std::unique_ptr<RpcController> guard_ctrl(ctrl); // delete ctrl
        if (ctrl->failed()) {
            LOG_ERROR("invoke notify failed: %d", ctrl->ec());
            return;
        }

        Msg_Echo* echo = dynamic_cast<Msg_Echo*>(response);
        if (nullptr != echo) {
            // LOG_INFO("client echo log_id: %d, result: %s", echo->log_id, echo->msg);
            return;
        }

        // delete ctrl and response according to call method
    }

    void on_notify(RpcController* ctrl, RpcMessage* response) {
        // LOG_DEBUG("client recv notify, ec: %d, text: %s", ctrl->ec(), ctrl->text().c_str());
        if (ctrl->failed()) {
            LOG_ERROR("client receive notify failed: %d", ctrl->ec());
            return;
        }

        Msg_Notify* notify = dynamic_cast<Msg_Notify*>(response);
        if (nullptr != notify) {
            // LOG_INFO("notify log_id: %d, result: %s", notify->log_id, notify->msg);
            return;
        }

        // DO NOT delete ctrl and response to repeat receive notify(global var)
    }

    RpcChannelPtr chn_;

    // Msg_Notify rsp_notify_;
    // RpcController ctrl_notify_;
};
const char* MsgService_Stub::service_ = "msg";


void service_run(IOContext* pctx) {
    LOG_INFO("start event loop thread!");
    pctx->run();
    LOG_INFO("exit loop thread!");
}


void chn_notify(RpcController* ctrl, RpcMessage* response) {
    // LOG_DEBUG("client recv notify, ec: %d, text: %s", ctrl->ec(), ctrl->text().c_str());
    if (ctrl->failed()) {
        LOG_ERROR("client receive notify failed: %d", ctrl->ec());
        return;
    }

    Msg_Notify* notify = dynamic_cast<Msg_Notify*>(response);
    if (nullptr != notify) {
        // LOG_INFO("notify log_id: %d, result: %s", notify->log_id, notify->msg);
        return;
    }

    // DO NOT delete ctrl and response to repeat receive notify(global var)
}
void chn_done(RpcController* ctrl, RpcMessage* response) {
    // LOG_DEBUG("rpc client done call, ec: %d, text: %s", ctrl->ec(), ctrl->text().c_str());
    //std::unique_ptr<RpcMessage> guard_rsp(response); // delete response
    //std::unique_ptr<RpcController> guard_ctrl(ctrl); // delete ctrl
    if (ctrl->failed()) {
        LOG_ERROR("client invoke failed: %d", ctrl->ec());
        return;
    }

    Msg_Echo* echo = dynamic_cast<Msg_Echo*>(response);
    if (nullptr != echo) {
        // LOG_INFO("client echo log_id: %d, result: %s", echo->log_id, echo->msg);
        return;
    }

    // delete ctrl and response according to call method
}

void chn_close(RpcChannelPtr chn) {
    LOG_DEBUG("channel close!");
    // release conn
    _channels.erase(chn);
}
void chn_connect(RpcChannelPtr chn, int status) {
    if (0 == status) {
        LOG_DEBUG("channel connect success!");
        chn->close_callback(chn_close);
    } else {
        LOG_DEBUG("channel connect failed: %d", status);
        _channels.erase(chn);
    }
}

void chn_register_notify(RpcChannelPtr chn) {
    _notify_response = new Msg_Notify;
    memset(_notify_response->msg, 0, sizeof(_notify_response->msg));

    _notify_ctrl = new RpcController;

    auto done = std::bind(chn_notify, _notify_ctrl, _notify_response);
    _notify_ctrl->done(done);

    int ret = chn->call_method("msg", "notify", nullptr, _notify_response, _notify_ctrl);
    if (0 != ret) {
        LOG_DEBUG("channel call method failed: %d", ret);
        delete _notify_response;
        _notify_response = nullptr;
        delete _notify_ctrl;
        _notify_ctrl = nullptr;
    }

}

void chn_echo(RpcChannelPtr chn) {
    Msg_Echo request;
    request.log_id = 7;
    memset(request.msg, 0, sizeof(request.msg));
    strncpy(request.msg, "hello", 6);

    Msg_Echo* response = new Msg_Echo;
    memset(response->msg, 0, sizeof(response->msg));

    RpcController* ctrl = new RpcController;
    // auto done = std::bind(chn_done, ctrl, response);
    // ctrl->done(done);

    int ret = chn->call_method("msg", "echo", &request, response, ctrl);
    if (0 == ret) {
        // LOG_INFO("client echo log_id: %d, result: %s", response->log_id, response->msg);
    } else {
        LOG_DEBUG("channel call method failed: %d", ret);
    }

    // if sync
    delete response;
    delete ctrl;
}
void chn_test(RpcChannelPtr chn) {
    RpcController* ctrl = new RpcController;
    // auto done = std::bind(chn_done, ctrl, nullptr);
    // ctrl->done(done);

    int ret = chn->call_method("msg", "test", nullptr, nullptr, ctrl);
    if (0 != ret) {
        LOG_DEBUG("channel call method failed: %d", ret);
    }

    // if sync
    // delete response;
    delete ctrl;
}
void timer_func(IOContext* pctx) {
    if (_channels.size() < 1) {
        auto chn = std::make_shared<RpcChannel>(pctx);
        chn->connect_callback(chn_connect);
        // chn->close_callback(chn_close);
        chn->start("192.168.17.17", 8341);
        _channels.insert(chn); // hold
    }

    for (const auto& chn : _channels) {
        chn_test(chn);
        chn_echo(chn);
    }
}

void close_all_channel() {
    // LOG_DEBUG("close all channel: %zu", _channels.size());
    for (const auto& chn : _channels) {
        chn->stop();
    }
    // _connections.clear();
}

void thread_send_sync(MsgService_Stub* stub) {
    while (_flag) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        stub->test();
        stub->echo();
        stub->register_notify();
        // chn_test(_chn);
        // chn_echo(_chn);
        // chn_register_notify(_chn);
    }
}


int main(int argc, char* argv[]) {
    int ret = log_init("gb28181");
    if (0 != ret) {
        printf("log init error\n");
        return -1;
    }
    LOG_INFO("start tcp client");

    IOContext ctx;
    ctx.init();
    LOG_TRACE("RpcChannel: %zu, RpcController: %zu", sizeof(RpcChannel), sizeof(RpcController));

#ifdef CALL_SYNC
    _chn = std::make_shared<RpcChannel>(&ctx);
    MsgService_Stub client(_chn);

    _chn->connect_callback(chn_connect);
    // chn->close_callback(chn_close);
    ret = _chn->start("192.168.17.17", 8341);
    if (0 != ret) {
        LOG_ERROR("channel start failed: %d", ret);
        return -2;
    }
#endif

#ifdef CALL_SYNC
    _flag = true;
    std::thread thd_sync(thread_send_sync, &client);
#else
    EventTimer t1(&ctx);
    t1.start(std::bind(timer_func, &ctx), 0, 1000);
#endif

    std::thread trd(service_run, &ctx);
    // std::this_thread::sleep_for(std::chrono::milliseconds(6000));
    getchar();

#ifdef CALL_SYNC
    _chn->stop();
    _flag = false;
    thd_sync.join();
#else
    ctx.dispatch(close_all_channel);
    t1.stop();
#endif
    // chn->stop();
    ctx.stop();
    trd.join();
    // std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    log_uninit();
    return 0;
}





