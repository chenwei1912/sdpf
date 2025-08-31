cmake_minimum_required(VERSION 3.5)

get_filename_component(PARENT_DIR ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)

project(sdpf_test LANGUAGES C CXX)
message("--------------- cmake start project ${PROJECT_NAME} ---------------")

#set(CMAKE_VERBOSE_MAKEFILEON ON)
message("${PROJECT_SOURCE_DIR}")
message("${PARENT_DIR}")


#aux_source_directory(./ SRC_LIST)


if(DEFINED CROSS) # target_link_directories since 3.13
    link_directories(sdpf ${3RDLIB_DIR}/libuv/lib)
    link_directories(sdpf ${3RDLIB_DIR}/zlog/lib)
    link_directories(sdpf ${3RDLIB_DIR}/msgpack/lib)
else()
    link_directories(sdpf ${3RDLIB_DIR}/lib)
endif()
link_directories(sdpf ${PARENT_DIR}/build)


add_executable(msgpack_test msgpack_test.cpp)
add_executable(tcp_server tcp_server.cpp)
add_executable(tcp_client tcp_client.cpp)
add_executable(rpc_server rpc_server.cpp MsgService.cpp)
add_executable(rpc_client rpc_client.cpp)


target_include_directories(sdpf PUBLIC ${PARENT_DIR}/include)
if(DEFINED CROSS)
    target_include_directories(sdpf PUBLIC ${3RDLIB_DIR}/../..)
    target_include_directories(sdpf PUBLIC ${3RDLIB_DIR}/libuv/include)
    target_include_directories(sdpf PUBLIC ${3RDLIB_DIR}/zlog/include)
    target_include_directories(sdpf PUBLIC ${3RDLIB_DIR}/msgpack/include)
else()
    target_include_directories(sdpf PUBLIC ${3RDLIB_DIR}/include)
endif()


target_link_libraries(msgpack_test PUBLIC sdpf)
target_link_libraries(tcp_server PUBLIC sdpf)
target_link_libraries(tcp_client PUBLIC sdpf)
target_link_libraries(rpc_server PUBLIC sdpf)
target_link_libraries(rpc_client PUBLIC sdpf)

