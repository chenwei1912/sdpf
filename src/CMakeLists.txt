cmake_minimum_required(VERSION 3.5)

get_filename_component(PARENT_DIR ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)

project(libsdpf LANGUAGES C CXX)
message("--------------- cmake start project ${PROJECT_NAME} ---------------")

#set(CMAKE_VERBOSE_MAKEFILEON ON)
message("${PROJECT_SOURCE_DIR}")
message("${PARENT_DIR}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

aux_source_directory(./ SRC_LIST)
#message(STATUS "sdpf lib SRC_LIST: ${SRC_LIST}")


if(DEFINED CROSS) # target_link_directories since 3.13
    link_directories(sdpf ${3RDLIB_DIR}/libuv/lib)
    link_directories(sdpf ${3RDLIB_DIR}/zlog/lib)
    link_directories(sdpf ${3RDLIB_DIR}/msgpack/lib)
else()
    link_directories(sdpf ${3RDLIB_DIR}/lib)
endif()


add_library(sdpf SHARED ${SRC_LIST})


target_include_directories(sdpf PUBLIC ${PARENT_DIR}/include)
if(DEFINED CROSS)
    target_include_directories(sdpf PUBLIC ${3RDLIB_DIR}/../..)
    target_include_directories(sdpf PUBLIC ${3RDLIB_DIR}/libuv/include)
    target_include_directories(sdpf PUBLIC ${3RDLIB_DIR}/zlog/include)
    target_include_directories(sdpf PUBLIC ${3RDLIB_DIR}/msgpack/include)
else()
    target_include_directories(sdpf PUBLIC ${3RDLIB_DIR}/include)
endif()


target_link_libraries(sdpf uv msgpack-c zlog pthread)

#set_target_properties(sdpf PROPERTIES POSITION_INDEPENDENT_CODE ON)

