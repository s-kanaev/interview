project(task1 C)
cmake_minimum_required(VERSION 2.8)

add_definitions(-g)

include_directories(include)

file(GLOB lib_src lib/*.c)

set(master_src src/master.c src/master-main.c)
set(protocol_src src/protocol.c)

add_library(lib SHARED ${lib_src})
add_library(protocol SHARED ${protocol_src})

add_executable(master ${master_src})
target_link_libraries(master lib protocol)
