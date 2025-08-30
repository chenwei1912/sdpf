#include "RpcServer.h"
#include "logger.h"

#include "MsgService.h"
#include "MsgService_data.h"

#include "IOContext.h"
#include "EventTimer.h"

#include <thread>
// #include <unordered_set>
#include <stdio.h>
#include <string.h>



void server_thread(IOContext* pctx) {
    LOG_INFO("start event loop thread!");
    // std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    pctx->run();
    LOG_INFO("exit loop thread!");
}

// void chn_close(RpcChannelPtr chn) {
//     LOG_INFO("rpc channel close!");
// }
// void chn_connect(RpcChannelPtr chn) {
//     LOG_INFO("rpc channel established!");
//     chn->close_callback(chn_close);
// }

void server_close(RpcServer* s) {
    LOG_INFO("RpcServer close!");
}
void server_start(RpcServer* s, int status) {
    if (0 == status) {
        LOG_INFO("RpcServer start success!");
        // s->channel_callback(chn_connect);
        // s->close_callback(server_close);
    } else {
        LOG_ERROR("RpcServer start failed %d", status);
    }
}

void timer_notify(MsgService* svc) {
    Msg_Notify msg;
    msg.log_id = 9;
    memset(msg.msg, 0, sizeof(msg.msg));
    strcpy(msg.msg, "notify hello");

    if (svc) {
        svc->notify(&msg);
    }
}


int main(int argc, char* argv[]) {
    int ret = log_init("gb28181");
    if (0 != ret) {
        printf("log init error\n");
        return -1;
    }
    LOG_INFO("start tcp test");

#if 1
    IOContext ctx;
    ctx.init();
    LOG_TRACE("IOContext size: %zu", sizeof(IOContext));

    MsgService svc;

    RpcServer server(&ctx);
    server.launch_callback(server_start);
    server.close_callback(server_close);
    server.register_service(&svc);
    server.start("192.168.17.17", 8341);

    EventTimer t1(&ctx);
    t1.start(std::bind(timer_notify, &svc), 3000, 1000);

    std::thread t(server_thread, &ctx);
    // std::this_thread::sleep_for(std::chrono::milliseconds(6000));
    getchar();

    t1.stop();
    server.stop();
    ctx.stop();

    t.join();
    // std::this_thread::sleep_for(std::chrono::milliseconds(1000));
#endif

#if 0

#endif

#if 0

#endif

#if 0

#endif

    log_uninit();
    return 0;
}





