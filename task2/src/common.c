#include "common.h"
#include "log.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include <sys/socket.h>
#include <linux/un.h>
#include <sys/signalfd.h>

static
void signal_caught(int fd, io_svc_op_t op, io_service_t *iosvc) {
    struct signalfd_siginfo si;

    ssize_t rc = read(fd, &si, sizeof(si));

    if (si.ssi_signo != SIGTERM && si.ssi_signo != SIGINT)
        return;

    io_service_stop(iosvc, false);
    close(fd);
}

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

bool wait_for_sigterm_sigint(io_service_t *iosvc) {
    sigset_t sigset;
    int ret;
    int fd = -1;

    ret = sigemptyset(&sigset);
    if (ret < 0)
        return false;

    ret = sigaddset(&sigset, SIGTERM);
    if (ret < 0)
        return false;

    ret = sigaddset(&sigset, SIGINT);
    if (ret < 0)
        return false;

    ret = sigprocmask(SIG_BLOCK, &sigset, NULL);
    if (ret < 0)
        return false;

    fd = signalfd(fd, &sigset, 0);

    if (fd < 0) {
        sigprocmask(SIG_UNBLOCK, &sigset, NULL);
        return false;
    }

    io_service_post_job(iosvc, fd, IO_SVC_OP_READ, !IOSVC_JOB_ONESHOT,
                        (iosvc_job_function_t)signal_caught,
                        iosvc);

    return true;
}
