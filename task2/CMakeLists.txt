include_directories(include)

set(driver_src src/driver-core.c src/unix-socket-server.c
               src/driver-main.c src/driver-payloads.c src/common.c)

add_executable(driver ${driver_src})
target_link_libraries(driver lib)