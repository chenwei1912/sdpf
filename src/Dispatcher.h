#ifndef SDPF_DISPATCHER_H
#define SDPF_DISPATCHER_H

#include <stdint.h>
#include <stddef.h>


typedef enum {
    RPCTYPE_NIL                     = 0x00,
    RPCTYPE_REQUEST                 = 0x01,
    RPCTYPE_RESPONSE                = 0x02
} Meta_RpcType;

typedef struct {
    char service[64];
    char method[64];
    // uint32_t log_id;
} Meta_Request;

typedef struct {
    int32_t code;
    char text[64];
} Meta_Response;

typedef union {
    Meta_Request req;
    Meta_Response rsp;
} Meta_Union;

typedef struct {
    Meta_RpcType type;
    uint32_t relation_id;
    Meta_Union via;
} Meta_Rpc;


int serialize_meta(Meta_Rpc* meta, char* buf, size_t len);
int deserialize_meta(Meta_Rpc* meta, const char* buf, size_t len);
// int pack_msg(int method_id, RpcMessage* msg, IOBuf* pb);
// int unpack_msg(int method_id, RpcMessage* msg, const char* buf, size_t len);


#endif // SDPF_DISPATCHER_H