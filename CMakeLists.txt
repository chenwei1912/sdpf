cmake_minimum_required(VERSION 3.5)

if(DEFINED CROSS) # aarch64-v01c01-linux-gnu-
    set(CROSS aarch64-v01c01-linux-gnu-)
    message("CROSS set ${CROSS}")
else()
    message("CROSS default ${CROSS}")
endif()

set(CMAKE_C_COMPILER  ${CROSS}gcc)
set(CMAKE_CXX_COMPILER ${CROSS}g++)
set(CMAKE_STRIP ${CROSS}strip)

project(sdpf LANGUAGES C CXX)

message("${PROJECT_SOURCE_DIR}")
#set(CMAKE_VERBOSE_MAKEFILEON ON)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -O2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O2") # -std=c++17
#message(STATUS "cxx flags ${CMAKE_CXX_FLAGS}")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)


if(DEFINED CROSS)
    set(3RDLIB_DIR /home/ethan/dvp/ipCamera/app/3rdlib/install_dir)
else()
    set(3RDLIB_DIR /home/ethan/dvp/libs/install)
endif()
message("3rd library path: ${3RDLIB_DIR}")


set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build)


add_subdirectory(src)
add_subdirectory(test)

