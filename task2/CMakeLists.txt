include_directories(include)

set(driver_src src/driver-core.c src/unix-socket-server.c
               src/driver-main.c src/driver-payloads.c src/common.c)

set(shell_src src/unix-socket-client.c src/common.c src/shell.c
              src/shell-main.c)

add_executable(driver ${driver_src})
target_link_libraries(driver lib)

add_executable(shell ${shell_src})
target_link_libraries(shell lib)
