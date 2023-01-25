cmake_minimum_required(VERSION 2.8)
project(interview-satellitesoft)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -g -O0 -Wl,-Bdynamic -fPIC")

include_directories(include)

include(FindPkgConfig)
pkg_check_modules(libpqxx REQUIRED libpqxx)

add_definitions(${libpqxx_CFLAGS})

find_package(cppnetlib 0.11.0 REQUIRED)
include_directories(${CPPNETLIB_INCLUDE_DIRS})

# include_directories(cpp-netlib-0.9.4)
# include_directories(cpp-netlib-0.11.0-final)

aux_source_directory(src SRCS)

set(NEED_BOOST_LIBS boost_system-mt boost_thread-mt)

add_library(server STATIC ${SRCS})
target_link_libraries(server pthread rt ${libpqxx_LDFLAGS} ${NEED_BOOST_LIBS} ${CPPNETLIB_LIBRARIES})

add_executable(server.bin main.cpp)
target_link_libraries(server.bin server pthread rt ${libpqxx_LDFLAGS} ${NEED_BOOST_LIBS} ${CPPNETLIB_LIBRARIES})

add_executable(client.bin client.cpp)
target_link_libraries(client.bin ${NEED_BOOST_LIBS} ${CPPNETLIB_LIBRARIES})
