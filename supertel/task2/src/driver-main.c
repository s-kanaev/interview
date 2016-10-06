#include "driver-core.h"
#include "io-service.h"
#include "driver-payloads.h"
#include "common.h"
#include "log.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>

void print_usage(const char *self) {
    printf("usage: %s <driver type: one | two> <slot number>\n", self);
}

int main(int argc, char **argv) {
    io_service_t iosvc;
    driver_core_t dc;
    driver_payload_t dp;

    char *type = NULL;
    char *slot = NULL;

    if (argc < 3) {
        print_usage(argv[0]);
        exit(0);
    }

    type = argv[1];
    slot = argv[2];

    if (0 == strcmp(type, "one"))
        driver_payload_one(&dp, atoi(slot));
    else if (0 == strcmp(type, "two"))
        driver_payload_two(&dp, atoi(slot));
    else {
        print_usage(argv[0]);
        exit(2);
    }

    io_service_init(&iosvc);

    if (!wait_for_sigterm_sigint(&iosvc)) {
        LOG(LOG_LEVEL_FATAL, "Can't initiate SIGTERM awaiting: %s\n",
            strerror(errno));

        exit(1);
    }

    if (!driver_core_init(&iosvc, &dc, &dp)) {
        LOG_MSG(LOG_LEVEL_FATAL, "Can't initialzie driver core\n");
        exit(1);
    }

    driver_core_run(&dc);

    driver_core_deinit(&dc);

    io_service_deinit(&iosvc);

    return 0;
}