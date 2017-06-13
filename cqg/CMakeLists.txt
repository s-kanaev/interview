project(cqg-interview CXX)
cmake_minimum_required(VERSION 2.8)

add_definitions(-g)

include_directories(include)

file(GLOB lib_src lib/*.cpp)
file(GLOB src src/*.cpp)

add_library(checkerlib SHARED ${lib_src})

add_executable(check-parenthesis ${src})
target_link_libraries(check-parenthesis checkerlib)

