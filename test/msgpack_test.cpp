//#include "catch.hpp"

// #include <thread>
// #include <unordered_set>
#include <stdio.h>
#include <iostream>
#include <stdint.h>
#include <any>

#include "Dispatcher.h"
#include "IOBuf.h"
#include "msgpack.h"

#include <memory>


typedef struct {
    char name[6];
    char value[12];
    int h;
} GB_DeviceInfo;


// typedef enum {
//     RPCTYPE_NIL                     = 0x00,
//     RPCTYPE_REQUEST                 = 0x01,
//     RPCTYPE_RESPONSE                = 0x02
// } Meta_RpcType;

// typedef struct {
//     char service[64];
//     char method[64];
//     // uint32_t log_id;
// } Meta_Request;

// typedef struct {
//     int32_t code;
//     char text[64];
// } Meta_Response;

// typedef union {
//     Meta_Request req;
//     Meta_Response rsp;
// } Meta_Union;

// typedef struct {
//     Meta_RpcType type;
//     uint32_t relation_id;
//     Meta_Union via;
// } Meta_Rpc;

struct person {
    int idx;
    char name[4];
    // std::string name;
    int age;

    // MSGPACK_DEFINE(idx, name, age);
    // MSGPACK_DEFINE_ARRAY(idx, name, age);
};

template <typename T>
void myprint(T last) {
    if (typeid(last) == typeid(int)) {
        std::cout << "int ";
    } else if (typeid(last) == typeid(char*)) {
        std::cout << "char* ";
    } else if (typeid(last) == typeid(const char*)) {
        std::cout << "const char* ";
    } else if (typeid(last) == typeid(float)) {
        std::cout << "float ";
    } else if (typeid(last) == typeid(double)) {
        std::cout << "double ";
    }
    std::cout << last << std::endl;
}

template <typename T, typename... Args>
void myprint(T head, Args... rest) {
    if (typeid(head) == typeid(int)) {
        std::cout << "int ";
    } else if (typeid(head) == typeid(char*)) {
        std::cout << "char* ";
    } else if (typeid(head) == typeid(const char*)) {
        std::cout << "const char* ";
    } else if (typeid(head) == typeid(float)) {
        std::cout << "float ";
    } else if (typeid(head) == typeid(double)) {
        std::cout << "double ";
    }
    std::cout << head << ", ";
    myprint(rest...);
}

class A1 {};

class A2 {
public:
    A2(){}
};

class A3 {
public:
    A3() = default;
    A3(int a3){}
};


int main(int argc, char* argv[]) {

    if (std::is_pod<A1>::value) {
        printf("A1 is pod\n");
    } else {
        printf("A1 is NOT pod\n");
    }
    if (std::is_pod<A2>::value) {
        printf("A2 is pod\n");
    } else {
        printf("A2 is NOT pod\n");
    }
    if (std::is_pod<A3>::value) {
        printf("A3 is pod\n");
    } else {
        printf("A3 is NOT pod\n");
    }

#if 1
    Dispatcher disp;
    // char* buf = new char[1024];
    // auto BufPtr = std::make_unique<char[]>(1024);
    auto pb = std::make_unique<IOBuf>();

    Meta_Rpc m1;
    memset(&m1, 0, sizeof(m1));
    m1.type = RPCTYPE_REQUEST;
    m1.relation_id = 7;
    strncpy(m1.via.req.service, "camera", 6);
    strncpy(m1.via.req.method, "ptz_move", 8);

    Meta_Rpc m2;
    memset(&m2, 0, sizeof(m2));
    m2.type = RPCTYPE_RESPONSE;
    m2.relation_id = 9;
    m2.via.rsp.code = -3;
    strncpy(m2.via.rsp.text, "404 not found", 13);

    // Meta_Rpc* meta = &m1;
    // printf("before meta request id: %u, service: %s, method: %s\n",
    //     meta->relation_id, meta->via.req.service, meta->via.req.method);

    // pb->ensure_writable(count)
    int len_pack = disp.pack_meta(&m1, pb->begin_write(), pb->writable_bytes());
    pb->has_written(len_pack);
    printf("m1 pack len: %d\n", len_pack);

    Meta_Rpc mm;
    memset(&mm, 0, sizeof(mm));
    // disp.unpack_meta(&mm, (char*)BufPtr.get(), len_pack);
    disp.unpack_meta(&mm, pb->begin_read(), pb->readable_bytes());
    pb->has_readall();

    Meta_Rpc* meta = &mm;
    if (RPCTYPE_REQUEST == meta->type) {
        printf("meta request id: %u, service: %s, method: %s\n",
            meta->relation_id, meta->via.req.service, meta->via.req.method);
    } else if (RPCTYPE_RESPONSE == meta->type) {
        printf("meta response id: %u, code: %d, text: %s\n",
            meta->relation_id, meta->via.rsp.code, meta->via.rsp.text);
    }

    // pb->ensure_writable(count)
    len_pack = disp.pack_meta(&m2, pb->begin_write(), pb->writable_bytes());
    pb->has_written(len_pack);
    printf("m2 pack len: %d\n", len_pack);

    memset(&mm, 0, sizeof(mm));
    disp.unpack_meta(&mm, pb->begin_read(), pb->readable_bytes());
    pb->has_readall();

    meta = &mm;
    if (RPCTYPE_REQUEST == meta->type) {
        printf("meta request id: %u, service: %s, method: %s\n",
            meta->relation_id, meta->via.req.service, meta->via.req.method);
    } else if (RPCTYPE_RESPONSE == meta->type) {
        printf("meta response id: %u, code: %d, text: %s\n",
            meta->relation_id, meta->via.rsp.code, meta->via.rsp.text);
    }

    // delete [] buf;
#endif

#if 0
    /* msgpack::sbuffer is a simple buffer implementation. */
    msgpack_sbuffer sbuf;
    msgpack_sbuffer_init(&sbuf);

    /* serialize values into the buffer using msgpack_sbuffer_write callback function. */
    msgpack_packer pk;
    msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 3);
    msgpack_pack_int(&pk, 1);
    msgpack_pack_true(&pk);
    msgpack_pack_str(&pk, 7);
    msgpack_pack_str_body(&pk, "example", 7);

    /* deserialize the buffer into msgpack_object instance. */
    /* deserialized object is valid during the msgpack_zone instance alive. */
    // msgpack_zone mempool;
    // msgpack_zone_init(&mempool, 2048);

    // msgpack_unpack(sbuf.data, sbuf.size, NULL, &mempool, &deserialized);
    msgpack_unpacked msg;
    msgpack_unpacked_init(&msg);
    msgpack_unpack_return ret = msgpack_unpack_next(&msg, sbuf.data, sbuf.size, NULL);

    /* print the deserialized object. */
    msgpack_object deserialized = msg.data;
    msgpack_object_print(stdout, deserialized);
    puts("");

    msgpack_unpacked_destroy(&msg);
    msgpack_sbuffer_destroy(&sbuf);
#endif

#if 0
    /* creates buffer and serializer instance. */
    msgpack_sbuffer buffer;
    msgpack_sbuffer_init(&buffer);

    msgpack_packer pk;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    // for (int i = 0; i < 20; ++i) {
        msgpack_sbuffer_clear(&buffer);

        Meta_Rpc meta;
        // meta.type = 1;
        // meta.relation_id = 3;
        // meta.via.

        msgpack_pack_array(&pk, 4);
        msgpack_pack_int(&pk, 1);
        msgpack_pack_int64(&pk, 7);
        msgpack_pack_str_with_body(&pk, "Media", 5);
        msgpack_pack_str_with_body(&pk, "hello", 5);

        // msgpack_pack_array(&pk, 3);
        // msgpack_pack_str_with_body(&pk, "Media", 5);
        // msgpack_pack_str_with_body(&pk, "hello", 5);
        // msgpack_pack_int(&pk, 7);


        /* deserializes it. */
        msgpack_unpacked msg;
        msgpack_unpacked_init(&msg);
        msgpack_unpack_return ret = msgpack_unpack_next(&msg, buffer.data, buffer.size, NULL);
        printf("deserialize ret: %d\n", ret);

        msgpack_object obj = msg.data;
        msgpack_object_print(stdout, obj);  /*=> ["Hello", "MessagePack"] */
        printf("\n");
        // printf("person : %u", sizeof(person));

        if (3 == obj.via.array.size) {
            printf("array %u is Request! %d\n", obj.via.array.size, obj.via.array.ptr->type);
            ++obj.via.array.ptr;
            printf("array %u is Request! %d\n", obj.via.array.size, obj.via.array.ptr->type);
            ++obj.via.array.ptr;
            printf("array %u is Request! %d\n", obj.via.array.size, obj.via.array.ptr->type);
        } else if (2 == obj.via.array.size) {
            printf("array %u is Reponse!\n", obj.via.array.size);
        }

        // GB_DeviceInfo info;
        // memset(&info, 0, sizeof(info));

        // if (MSGPACK_OBJECT_ARRAY == obj.type) {
        //     printf("obj type array, size: %d\n", obj.via.array.size);
        //     int i = 0;
        //     msgpack_object* ptr = obj.via.array.ptr + i;
        //     strncpy(info.name, ptr->via.str.ptr, sizeof(info.name) - 1);
        //     printf("array[%d] : %s\n", i, ptr->via.str.ptr);
        //     // printf("array[%d] : %s\n", i, info.name);

        //     ++i;
        //     ptr = obj.via.array.ptr + i;
        //     strncpy(info.value, ptr->via.str.ptr, sizeof(info.value) - 1);
        //     printf("array[%d] : %s\n", i, ptr->via.str.ptr);
        //     // printf("array[%d] : %s\n", i, info.value);
        // }

        msgpack_unpacked_destroy(&msg);
    // }

    msgpack_sbuffer_destroy(&buffer);
    // msgpack_sbuffer_free(buffer);
    // msgpack_packer_free(pk);
#endif

#if 0
    GB_DeviceInfo info;
    memset(&info, 0, sizeof(info));
    strncpy(info.name, "hello", 5);
    strncpy(info.value, "message", 7);
    info.h = 9;

    double x = 1.37f;
    myprint(1, 2, "hello", x);

    myprint(info.name, info.value, info.h);

    printf("%s, %s, %s, %s, %s\n", typeid(uint8_t).name(), typeid(uint16_t).name(), typeid(uint32_t).name(),
            typeid(uint64_t).name(), typeid(size_t).name());
    printf("%s, %s, %s, %s\n", typeid(unsigned char).name(), typeid(unsigned short).name(), typeid(unsigned int).name(),
            typeid(long long).name());
#endif

#if 0

#endif

    return 0;
}





