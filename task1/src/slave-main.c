#include "slave.h"
#include "io-service.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    slave_t slave;
    io_service_t iosvc;
    char *interface = NULL;

    if (argc < 2) {
        printf("usage: %s <interface>\n", argv[0]);
        exit(0);
    }

    interface = argv[1];

    io_service_init(&iosvc);
    slave_init(&slave, &iosvc, interface);

    slave_run(&slave);

    return 0;
}