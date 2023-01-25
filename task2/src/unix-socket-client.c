#include "unix-socket-client.h"
#include "common.h"

#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/socket.h>

bool unix_socket_client_init(usc_t *usc,
                             const char *name, size_t name_len,
                             io_service_t *iosvc) {
    assert(usc);

    memset(usc, 0, sizeof(*usc));

    usc->iosvc = iosvc;
    usc->connector = NULL;

    usc->fd = allocate_unix_socket(name, name_len);

    if (usc->fd < 0)
        return false;

    usc->name = (char *)malloc(name_len);
    memcpy(usc->name, name, name_len);
    usc->name_len = name_len;

    return true;
}

void unix_socket_client_deinit(usc_t *usc) {
    assert(usc);

    shutdown(usc->fd, SHUT_RDWR);
    close(usc->fd);

    unlink(usc->name);
    free(usc->name);
}

bool unix_socket_client_connect(usc_t *usc,
                                const char *name, size_t name_len,
                                usc_connector_t _connector, void *ctx) {
    /* TODO */
}

