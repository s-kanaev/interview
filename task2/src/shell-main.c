#include "shell.h"
#include "io-service.h"
#include "common.h"
#include "log.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv) {
    io_service_t iosvc;
    shell_t sh;

    io_service_init(&iosvc);

    if (!shell_init(&sh, DIR, DIR_LEN, &iosvc, STDIN_FILENO, stdout)) {
        LOG(LOG_LEVEL_FATAL, "Can't initialize shell: %s\n",
            strerror(errno));
        exit(1);
    }

    setvbuf(stdout, NULL, _IONBF, 0);

    shell_run(&sh);

    shell_deinit(&sh);

    io_service_deinit(&iosvc);

    return 0;
}