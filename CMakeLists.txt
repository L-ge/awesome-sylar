cmake_minimum_required(VERSION 3.0)

project(awesome-sylar)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -rdynamic -O0 -ggdb -std=c++11 -Wall -Werror -pthread")

include_directories(.)

set(LIB_SRC
    sylar/log.cc
    sylar/mutex.cc
    sylar/util/filestream_util.cc
    sylar/util/system_util.cc
    )

add_library(sylar SHARED ${LIB_SRC})

add_executable(test_log tests/test_log.cc)
add_dependencies(test_log sylar)
target_link_libraries(test_log sylar)


set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
