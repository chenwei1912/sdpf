#include "MsgService.h"
#include "MsgService_data.h"
#include "RpcController.h"
#include "logger.h"

#include <string.h>



static RpcMessage* notify_new() {
    Msg_Notify* ptr = new (std::nothrow) Msg_Notify;
    ptr->log_id = 0;
    memset(ptr->msg, 0, sizeof(ptr->msg));
    return ptr;
}

static RpcMessage* test_new() {
    return nullptr;
}
static int test(RpcMessage* request, RpcMessage* response, RpcController* ctrl) {
    // LOG_TRACE("msg service test call");
    return 0;
}

static RpcMessage* echo_new() {
    Msg_Echo* ptr = new (std::nothrow) Msg_Echo;
    ptr->log_id = 0;
    memset(ptr->msg, 0, sizeof(ptr->msg));
    return ptr;
}
static int echo(RpcMessage* request, RpcMessage* response, RpcController* ctrl) {
    Msg_Echo* req = dynamic_cast<Msg_Echo*>(request);
    Msg_Echo* rsp = dynamic_cast<Msg_Echo*>(response);
    if (nullptr == req || nullptr == rsp) {
        return -1;
    }

    rsp->log_id = req->log_id;
    // memset(rsp->msg, 0, sizeof(rsp->msg));
    strcpy(rsp->msg, req->msg);
    // size_t n = strlen(rsp->msg);
    // strncpy(rsp->msg, req->msg, sizeof(rsp->msg) - 1);
    // rsp->msg[sizeof(rsp->msg) - 1] = '\0';
    return 0;
}


typedef RpcMessage* (*new_handler_t)();
// typedef void (*del_handler_t)(int method_id, RpcMessage* msg);
typedef int (*call_handler_t)(RpcMessage* request, RpcMessage* response, RpcController* ctrl);

typedef struct {
    int index;
    const char* name;
    new_handler_t new_request;
    // del_handler_t del_request;
    new_handler_t new_response;
    // del_handler_t del_response;
    call_handler_t handler;
} svc_method;

static const char* _service_name = "msg";
static const int _method_count = 3;
static const svc_method _Methods[] = {
    {0, "notify", notify_new, nullptr, nullptr}, // special for notify
    {1, "test", test_new, test_new, test},
    {2, "echo", echo_new, echo_new, echo},
};
// static const std::unordered_set<svc_method> _Methods = {
//     {0, "echo", echo_new, echo_new_response, echo},
// };


MsgService::MsgService() {
}

MsgService::~MsgService() {
}

const char* MsgService::name() {
    return _service_name;
}

int MsgService::find_method(const char* method) {
    if (nullptr == method || 0 == strlen(method)) {
        return -1;
    }

    for (int i = 0; i < _method_count; ++i) {
        if (nullptr != _Methods[i].name
            && 0 == strncmp(method, _Methods[i].name, strlen(_Methods[i].name))) {
            return i;
        }
    }

    return -2;
}

RpcMessage* MsgService::new_request(int method_id) {
    if (method_id < 0 || method_id > _method_count) {
        return nullptr;
    }

    new_handler_t handle = _Methods[method_id].new_request;
    if (handle) {
        return handle();
    }
    return nullptr;
}

RpcMessage* MsgService::new_response(int method_id) {
    if (method_id < 0 || method_id > _method_count) {
        return nullptr;
    }

    new_handler_t handle = _Methods[method_id].new_response;
    if (handle) {
        return handle();
    }
    return nullptr;
}

void MsgService::call_method(int method_id, RpcMessage* request, RpcMessage* response, RpcController* ctrl) {
    if (method_id < 0 || method_id > _method_count) {
        return;
    }

    call_handler_t handle = _Methods[method_id].handler;
    if (nullptr == handle) {
        return;
    }

    int ret = handle(request, response, ctrl);
    if (0 != ret) {
        // log error ret
        ctrl->ec(1005);
    }

    if (ctrl) {
        auto done = ctrl->done();
        if (done) {
            done();
        }
    }
}

void MsgService::notify(RpcMessage* msg) {
    if (notify_cb_) {
        notify_cb_(msg);
    }
}
