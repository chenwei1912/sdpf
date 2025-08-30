#ifndef SDPF_MSGSERVICE_H
#define SDPF_MSGSERVICE_H

#include "RpcService.h"

// namespace sdpf {
// namespace rpc {


class MsgService : public RpcService {
public:
    MsgService();
    ~MsgService();

    const char* name() override;
    int find_method(const char* method) override;

    RpcMessage* new_request(int method_id) override;
    // void del_request(int method_id, RpcMessage* request) override;
    RpcMessage* new_response(int method_id) override;
    // void del_response(int method_id, RpcMessage* response) override;
    void call_method(int method_id, RpcMessage* request, RpcMessage* response, RpcController* ctrl) override;
    void notify(RpcMessage* msg) override;

};

// }// namespace rpc
// }// namespace sdpf

#endif // SDPF_MSGSERVICE_H