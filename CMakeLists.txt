cmake_minimum_required(VERSION 2.8)
project(interview-satellitesoft)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -g -O0")

include_directories(include)
# include_directories(cpp-netlib-0.9.4)
include_directories(cpp-netlib-0.11.0-final)

aux_source_directory(src SRCS)

add_library(server SHARED ${SRCS})

add_executable(interview-satellitesoft main.cpp)
