#ifndef SDPF_RPCMESSAGE_H
#define SDPF_RPCMESSAGE_H

#include <stddef.h>

// namespace sdpf {
// namespace rpc {


class RpcMessage {
public:
    // RpcMessage() {};
    virtual ~RpcMessage() {};
    virtual int serialize(char* buf, size_t len) = 0;
    virtual int deserialize(const char* buf, size_t len) = 0;

    int item_count;
};

// }// namespace rpc
// }// namespace sdpf

#endif // SDPF_RPCMESSAGE_H