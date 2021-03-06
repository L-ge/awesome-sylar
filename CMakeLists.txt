cmake_minimum_required(VERSION 3.0)

project(awesome-sylar)

set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -rdynamic -O0 -fPIC -std=c++11 -ggdb -Wall -Werror -Wno-deprecated-declarations -pthread -lyaml-cpp -ldl -lz -ljsoncpp")

include_directories(.)
include_directories(/apps/bread/include)
link_directories(/apps/bread/lib)
link_directories(/apps/bread/lib64)

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
    sylar/iomanager.cc
    sylar/timer.cc
    sylar/hook.cc
    sylar/fdmanager.cc
    sylar/address.cc
    sylar/socket.cc
    sylar/bytearray.cc
    sylar/http/http.cc
    sylar/util/string_util.cc
    sylar/http/http_parser.cc
    sylar/http/http11_parser.rl.cc
    sylar/http/httpclient_parser.rl.cc
    sylar/tcp_server.cc
    sylar/stream.cc
    sylar/streams/socket_stream.cc
    sylar/http/http_session.cc
    sylar/http/http_server.cc
    sylar/http/servlet.cc
    sylar/http/http_connection.cc
    sylar/streams/zlib_stream.cc
    sylar/uri.rl.cc
    sylar/daemon.cc
    sylar/env.cc
    sylar/application.cc
    sylar/module.cc
    sylar/library.cc
    sylar/protocol.cc
    sylar/util/json_util.cc
    sylar/util/type_util.cc
    )

add_library(sylar SHARED ${LIB_SRC})

add_executable(test_log tests/test_log.cc)
add_dependencies(test_log sylar)
target_link_libraries(test_log sylar)

add_executable(test_config tests/test_config.cc)
add_dependencies(test_config sylar)
target_link_libraries(test_config sylar)

add_executable(test_thread tests/test_thread.cc)
add_dependencies(test_thread sylar)
target_link_libraries(test_thread sylar)

add_executable(test_scheduler tests/test_scheduler.cc)
add_dependencies(test_scheduler sylar)
target_link_libraries(test_scheduler sylar)

add_executable(test_iomanager tests/test_iomanager.cc)
add_dependencies(test_iomanager sylar)
target_link_libraries(test_iomanager sylar)

add_executable(test_hook tests/test_hook.cc)
add_dependencies(test_hook sylar)
target_link_libraries(test_hook sylar)

add_executable(test_address tests/test_address.cc)
add_dependencies(test_address sylar)
target_link_libraries(test_address sylar)

add_executable(test_socket tests/test_socket.cc)
add_dependencies(test_socket sylar)
target_link_libraries(test_socket sylar)

add_executable(test_bytearray tests/test_bytearray.cc)
add_dependencies(test_bytearray sylar)
target_link_libraries(test_bytearray sylar)

add_executable(test_http tests/test_http.cc)
add_dependencies(test_http sylar)
target_link_libraries(test_http sylar)

add_executable(test_http_parser tests/test_http_parser.cc)
add_dependencies(test_http_parser sylar)
target_link_libraries(test_http_parser sylar)

add_executable(test_tcp_server tests/test_tcp_server.cc)
add_dependencies(test_tcp_server sylar)
target_link_libraries(test_tcp_server sylar)

add_executable(test_http_server tests/test_http_server.cc)
add_dependencies(test_http_server sylar)
target_link_libraries(test_http_server sylar)

add_executable(test_http_connection tests/test_http_connection.cc)
add_dependencies(test_http_connection sylar)
target_link_libraries(test_http_connection sylar)

add_executable(test_daemon tests/test_daemon.cc)
add_dependencies(test_daemon sylar)
target_link_libraries(test_daemon sylar)

add_executable(test_env tests/test_env.cc)
add_dependencies(test_env sylar)
target_link_libraries(test_env sylar)

add_executable(test_application tests/test_application.cc)
add_dependencies(test_application sylar)
target_link_libraries(test_application sylar)

add_executable(echo_server examples/echo_server.cc)
add_dependencies(echo_server sylar)
target_link_libraries(echo_server sylar)

add_executable(my_http_server examples/my_http_server.cc)
add_dependencies(my_http_server sylar)
target_link_libraries(my_http_server sylar)

add_executable(bin_sylar sylar/main.cc)
add_dependencies(bin_sylar sylar)
target_link_libraries(bin_sylar sylar)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
