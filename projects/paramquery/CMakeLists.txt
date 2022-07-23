cmake_minimum_required(VERSION 3.0)

project(paramquery)

set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -rdynamic -O0 -fPIC -std=c++11 -ggdb -Wall -Werror -Wno-deprecated-declarations -pthread -lyaml-cpp -ldl -lz -lhiredis -ljsoncpp")

include_directories(.)
include_directories(/apps/bread/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../..)

link_directories(/apps/bread/lib)
link_directories(/apps/bread/lib64)

find_package(Boost REQUIRED)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

set(LIB_SRC
    my_module.cc
    paramquery_servlet.cc
    aredisclient.cc
    ahandleparam.cc
    )

add_library(paramquery SHARED ${LIB_SRC})

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/../bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/../lib)

