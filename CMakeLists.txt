project(interview-supertel C)
cmake_minimum_required(VERSION 2.8)

add_definitions(-g)

include_directories(include)

file(GLOB lib_src lib/*.c)

add_library(lib SHARED ${lib_src})

add_subdirectory(task1)
add_subdirectory(task2)
