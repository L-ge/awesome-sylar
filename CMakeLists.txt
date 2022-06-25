cmake_minimum_required(VERSION 3.0)

project(awesome-sylar)

set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -rdynamic -O0 -fPIC -std=c++11 -ggdb -Wall -Werror -pthread -lyaml-cpp")

include_directories(.)
include_directories(/apps/bread/include)
link_directories(/apps/bread/lib)

find_package(Boost REQUIRED)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

set(LIB_SRC
    sylar/log.cc
    sylar/mutex.cc
    sylar/thread.cc
    sylar/util/filestream_util.cc
    sylar/util/system_util.cc
    sylar/env.cc
    sylar/config.cc
    sylar/fiber.cc
    sylar/scheduler.cc
    )

add_library(sylar SHARED ${LIB_SRC})

add_executable(test_log tests/test_log.cc)
add_dependencies(test_log sylar)
target_link_libraries(test_log sylar)

add_executable(test_config tests/test_config.cc)
add_dependencies(test_config sylar)
target_link_libraries(test_config sylar)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
