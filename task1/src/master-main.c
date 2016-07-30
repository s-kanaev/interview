#include "master.h"
#include "io-service.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    master_t master;
    io_service_t iosvc;
    char *interface = NULL;

    if (argc < 2) {
        printf("usage: %s <interface>\n", argv[0]);
        exit(0);
    }

    interface = argv[1];

    io_service_init(&iosvc);
    master_init(&master, &iosvc, interface);

    master_run(&master);

    return 0;
}