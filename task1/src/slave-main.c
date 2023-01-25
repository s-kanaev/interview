#include "slave.h"
#include "io-service.h"
#include "common.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <string.h>

int main(int argc, char **argv) {
    slave_t slave;
    io_service_t iosvc;
    char *interface = NULL;

    if (argc < 2) {
        printf("usage: %s <interface>\n", argv[0]);
        exit(0);
    }

    interface = argv[1];

    srandom(time(NULL));

    io_service_init(&iosvc);

    if (!wait_for_sigterm_sigint(&iosvc)) {
        LOG(LOG_LEVEL_FATAL, "Can't initiate SIGTERM awaiting: %s\n",
            strerror(errno));

        exit(1);
    }

    if (!slave_init(&slave, &iosvc, interface)) {
        LOG_MSG(LOG_LEVEL_FATAL, "Can't initialize slave\n");

        exit(1);
    }

    slave_run(&slave);

    slave_deinit(&slave);
    io_service_deinit(&iosvc);

    return 0;
}