#include "Dispatcher.h"
#include "logger.h"

#include "MsgService_data.h"
#include "msgpack.h"



typedef struct {
    char* buf;
    size_t len;
    size_t write_len;
} user_buf;


static int msgpack_user_write(void* data, const char* buf, size_t len) {
    user_buf* pb = static_cast<user_buf*>(data);
    memcpy(pb->buf + pb->write_len, buf, len);
    pb->write_len += len;
    return 0;
}
// static int msgpack_iobuf_write(void* data, const char* buf, size_t len) {
//     IOBuf* pb = static_cast<IOBuf*>(data);
//     pb->write(buf, len);
//     return 0;
// }

int serialize_meta(Meta_Rpc* meta, char* buf, size_t len) {
    // msgpack_sbuffer* buffer = msgpack_sbuffer_new();
    user_buf tmp_buf = {buf, len, 0}; // FIXME: ensure enough space for all pack case
    msgpack_packer* pk = msgpack_packer_new(&tmp_buf, msgpack_user_write);

    if (RPCTYPE_REQUEST == meta->type) {
        msgpack_pack_array(pk, 4);
    } else if (RPCTYPE_RESPONSE == meta->type) {
        msgpack_pack_array(pk, 4);
    } else {
        msgpack_packer_free(pk);
        return -1;
    }

    msgpack_pack_int(pk, meta->type);
    msgpack_pack_uint32(pk, meta->relation_id);

    if (RPCTYPE_REQUEST == meta->type) {
        msgpack_pack_str_with_body(pk, meta->via.req.service, strlen(meta->via.req.service));
        msgpack_pack_str_with_body(pk, meta->via.req.method, strlen(meta->via.req.method));
    } else if (RPCTYPE_RESPONSE == meta->type) {
        msgpack_pack_int(pk, meta->via.rsp.code);
        msgpack_pack_str_with_body(pk, meta->via.rsp.text, strlen(meta->via.rsp.text));
    }

    // msgpack_sbuffer_free(buffer);
    msgpack_packer_free(pk);
    return tmp_buf.write_len;
}

int deserialize_meta(Meta_Rpc* meta, const char* buf, size_t len) {
    msgpack_unpacked msg_meta;
    msgpack_unpacked_init(&msg_meta);
    msgpack_unpack_return result = msgpack_unpack_next(&msg_meta, buf, len, NULL);
    if (MSGPACK_UNPACK_SUCCESS != result) {
        msgpack_unpacked_destroy(&msg_meta);
        return -1;
    }

    msgpack_object obj_meta = msg_meta.data;
    // msgpack_object_print_buffer(char *buffer, size_t buffer_size, obj_meta);

    int i = 0;
    msgpack_object* po = obj_meta.via.array.ptr + i; // MSGPACK_OBJECT_POSITIVE_INTEGER == po->type
    meta->type = (Meta_RpcType)po->via.i64;

    ++i;
    po = obj_meta.via.array.ptr + i; // MSGPACK_OBJECT_POSITIVE_INTEGER == po->type
    meta->relation_id = po->via.u64;

    if (RPCTYPE_REQUEST == meta->type) { // request 3 == obj_meta.via.array.size
        ++i;
        po = obj_meta.via.array.ptr + i; // MSGPACK_OBJECT_STR == po->type
        // if (po->via.str.size < sizeof(meta->via.req.service) - 1) {
        // }
        strncpy(meta->via.req.service, po->via.str.ptr, po->via.str.size); // po->via.str.size
        // LOG_TRACE("request service: %s", po->via.str.ptr);

        ++i;
        po = obj_meta.via.array.ptr + i; // MSGPACK_OBJECT_STR == po->type
        strncpy(meta->via.req.method, po->via.str.ptr, po->via.str.size); // po->via.str.size
        // LOG_TRACE("request method: %s", po->via.str.ptr);

        // ++i;
        // po = obj_meta.via.array.ptr + i;
        // meta->via.req.log_id = po->via.i64;
        // LOG_TRACE("request id: %u", po->via.u64);
    } else if (RPCTYPE_RESPONSE == meta->type) { // response 2 == obj_meta.via.array.size
        ++i;
        po = obj_meta.via.array.ptr + i; // MSGPACK_OBJECT_NEGATIVE_INTEGER == po->type
        meta->via.rsp.code = po->via.i64;
        // LOG_TRACE("response code: %u", po->via.i64);

        ++i;
        po = obj_meta.via.array.ptr + i; // MSGPACK_OBJECT_STR == po->type
        strncpy(meta->via.rsp.text, po->via.str.ptr, po->via.str.size); // po->via.str.size
        // LOG_TRACE("response text: %s", po->via.str.ptr);
    } else {
        return -2;
    }

    msgpack_unpacked_destroy(&msg_meta);
    return 0;
}

// Msg_Echo::~Msg_Echo() {
//     LOG_TRACE("Msg_Echo dtor!");
// }
int Msg_Echo::serialize(char* buf, size_t len) {
    // msgpack_sbuffer* buffer = msgpack_sbuffer_new();
    user_buf tmp_buf = {buf, len, 0};
    msgpack_packer* pk = msgpack_packer_new(&tmp_buf, msgpack_user_write);

    msgpack_pack_array(pk, 2);
    msgpack_pack_int(pk, this->log_id);
    msgpack_pack_bin_with_body(pk, this->msg, strlen(this->msg));
    // msgpack_pack_bin(pk, strlen(this->msg));
    // msgpack_pack_bin_body(pk, this->msg, strlen(this->msg));

    // msgpack_sbuffer_free(buffer);
    msgpack_packer_free(pk);
    return tmp_buf.write_len;
}
int Msg_Echo::deserialize(const char* buf, size_t len) {
    msgpack_unpacked msg;
    msgpack_unpacked_init(&msg);
    msgpack_unpack_return result = msgpack_unpack_next(&msg, buf, len, NULL);

    int ret = 0;
    do {
        if (MSGPACK_UNPACK_SUCCESS != result) {
            ret = -1;
            break;
        }

        msgpack_object obj = msg.data;
        // msgpack_object_print_buffer(char *buffer, size_t buffer_size, obj_meta);
        if (MSGPACK_OBJECT_ARRAY != obj.type
            /*|| 2 != obj.via.array.size*/) {
            ret = -2;
            break;
        }

        int i = 0;
        msgpack_object* item = obj.via.array.ptr + i; // item->type MSGPACK_OBJECT_POSITIVE_INTEGER MSGPACK_OBJECT_NEGATIVE_INTEGER
        this->log_id = item->via.i64;

        ++i;
        item = obj.via.array.ptr + i; // item->type MSGPACK_OBJECT_STR
        // strncpy(this->msg, item->via.str.ptr, strlen(item->via.str.ptr)); // item->via.str.size
        memcpy(this->msg, item->via.bin.ptr, item->via.bin.size);
    } while (false);

    msgpack_unpacked_destroy(&msg);
    return ret;
}

int Msg_Notify::serialize(char* buf, size_t len) {
    // msgpack_sbuffer* buffer = msgpack_sbuffer_new();
    user_buf tmp_buf = {buf, len, 0};
    msgpack_packer* pk = msgpack_packer_new(&tmp_buf, msgpack_user_write);

    msgpack_pack_array(pk, 2);
    msgpack_pack_int(pk, this->log_id);
    msgpack_pack_bin_with_body(pk, this->msg, strlen(this->msg));

    // msgpack_sbuffer_free(buffer);
    msgpack_packer_free(pk);
    return tmp_buf.write_len;
}
int Msg_Notify::deserialize(const char* buf, size_t len) {
    msgpack_unpacked msg;
    msgpack_unpacked_init(&msg);
    msgpack_unpack_return result = msgpack_unpack_next(&msg, buf, len, NULL);

    int ret = 0;
    do {
        if (MSGPACK_UNPACK_SUCCESS != result) {
            ret = -1;
            break;
        }

        msgpack_object obj = msg.data;
        // msgpack_object_print_buffer(char *buffer, size_t buffer_size, obj_meta);
        if (MSGPACK_OBJECT_ARRAY != obj.type
            /*|| 2 != obj.via.array.size*/) {
            ret = -2;
            break;
        }

        int i = 0;
        msgpack_object* item = obj.via.array.ptr + i; // item->type MSGPACK_OBJECT_POSITIVE_INTEGER MSGPACK_OBJECT_NEGATIVE_INTEGER
        this->log_id = item->via.i64;

        ++i;
        item = obj.via.array.ptr + i; // item->type MSGPACK_OBJECT_STR
        // strncpy(this->msg, item->via.str.ptr, strlen(item->via.str.ptr)); // item->via.str.size
        memcpy(this->msg, item->via.bin.ptr, item->via.bin.size);
    } while (false);

    msgpack_unpacked_destroy(&msg);
    return ret;
}

