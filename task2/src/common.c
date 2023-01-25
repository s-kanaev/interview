#include "common.h"
#include "log.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <linux/un.h>

int allocate_unix_socket(const char *name, size_t name_len) {
    struct sockaddr_un addr;
    int sfd = -1;

    sfd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (sfd < 0) {
        LOG(LOG_LEVEL_WARN, "Can't allocate UNIX-socket: %s\n",
            strerror(errno));

        return -1;
    }

    if (!name || !name_len)
        return sfd;

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    memcpy(addr.sun_path, name,
           name_len > UNIX_PATH_MAX ? UNIX_PATH_MAX - 1 : name_len);
    addr.sun_path[UNIX_PATH_MAX - 1] = '\0';

    if (bind(sfd, (struct sockaddr *)&addr, sizeof(addr))) {
        LOG(LOG_LEVEL_DEBUG,
            "Can't bind UNIX-socket: %s\n",
            strerror(errno));

        close(sfd);

        return -1;
    }

    return sfd;
}
