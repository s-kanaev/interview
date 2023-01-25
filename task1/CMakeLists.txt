include_directories(include)

set(master_src src/master-main.c src/master.c src/common.c)
set(slave_src src/slave.c src/common.c src/slave-main.c)

set(masterlib_src src/master-private.c)
set(protocol_src src/protocol.c)

add_library(protocol SHARED ${protocol_src})
add_library(masterlib SHARED ${masterlib_src})

add_executable(master ${master_src})
target_link_libraries(master lib protocol masterlib)

add_executable(slave ${slave_src})
target_link_libraries(slave lib protocol masterlib)
