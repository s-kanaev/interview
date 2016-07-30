#include "master.h"
#include "io-service.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    master_t master;
    io_service_t iosvc;
    char *local_addr = NULL;
    char *interface = NULL;

    if (argc < 3) {
        printf("usage: %s <ip address to watch> <interface>\n", argv[0]);
        exit(0);
    }

    local_addr = argv[1];
    interface = argv[2];

    io_service_init(&iosvc);
    master_init(&master, &iosvc, local_addr, interface);

    master_run(&master);

    return 0;
}