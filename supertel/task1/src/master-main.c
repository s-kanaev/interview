#include "master.h"
#include "io-service.h"
#include "common.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

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

    if (!wait_for_sigterm_sigint(&iosvc)) {
        LOG(LOG_LEVEL_FATAL, "Can't initiate SIGTERM awaiting: %s\n",
            strerror(errno));

        exit(1);
    }

    if (!master_init(&master, &iosvc, interface)) {
        LOG_MSG(LOG_LEVEL_FATAL, "Can't initialize master\n");

        exit(1);
    }

    master_run(&master);

    master_deinit(&master);
    io_service_deinit(&iosvc);

    return 0;
}