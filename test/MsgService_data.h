#ifndef SDPF_MSGSERVICE_DATA_H
#define SDPF_MSGSERVICE_DATA_H

#include "RpcMessage.h"


class Msg_Echo : public RpcMessage {
public:
    // Msg_Echo() {};
    ~Msg_Echo() {};
    int serialize(char* buf, size_t len) override;
    int deserialize(const char* buf, size_t len) override;

    int log_id;
    char msg[64];
};

class Msg_Notify : public RpcMessage {
public:
    // Msg_Notify() {};
    ~Msg_Notify() {};
    int serialize(char* buf, size_t len) override;
    int deserialize(const char* buf, size_t len) override;

    int log_id;
    char msg[64];
};


#endif // SDPF_MSGSERVICE_DATA_H