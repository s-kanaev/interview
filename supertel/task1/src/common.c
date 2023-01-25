#include "common.h"
#include "log.h"

#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include <net/if.h>
#include <sys/ioctl.h>
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

int allocate_udp_broadcasting_socket(const char *iface,
                                     uint16_t local_port,
                                     struct sockaddr *local_addr) {
    static const int BROADCAST = 1;
    static const int REUSE_ADDR = 1;

    size_t len;
    int sfd, ret;
    struct sockaddr_in addr;
    struct ifreq ifreq;

    sfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (sfd < 0)
        return -1;

    ret = setsockopt(sfd,
                     SOL_SOCKET, SO_REUSEADDR,
                     &REUSE_ADDR, sizeof(REUSE_ADDR));

    if (ret != 0) {
        LOG(LOG_LEVEL_WARN,
            "Can't set socket option (SO_REUSEADDR): %s\n",
            strerror(errno));

        shutdown(sfd, SHUT_RDWR);
        close(sfd);
        return -1;
    }

    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(local_port);
    addr.sin_family = AF_INET;
    if (bind(sfd, (const struct sockaddr *)&addr, sizeof(addr))) {
        LOG(LOG_LEVEL_WARN,
            "Can't bind: %s\n",
            strerror(errno));

        shutdown(sfd, SHUT_RDWR);
        close(sfd);
        return -1;
    }

    len = strlen(iface);
    ret = setsockopt(sfd,
                     SOL_SOCKET, SO_BINDTODEVICE,
                     (const int *)iface, len < IFNAMSIZ ? len : IFNAMSIZ - 1);

    if (ret != 0) {
        LOG(LOG_LEVEL_WARN,
            "Can't set socket option (SO_REUSEADDR): %s\n",
            strerror(errno));

        shutdown(sfd, SHUT_RDWR);
        close(sfd);
        return -1;
    }

    ret = setsockopt(sfd,
                     SOL_SOCKET, SO_BROADCAST,
                     &BROADCAST, sizeof(BROADCAST));

    if (ret != 0) {
        LOG(LOG_LEVEL_WARN,
            "Can't set socket option (SO_BROADCAST): %s\n",
            strerror(errno));

        shutdown(sfd, SHUT_RDWR);
        close(sfd);
        return -1;
    }

    memset(&ifreq, 0, sizeof(ifreq));
    strncpy(ifreq.ifr_name, iface, IFNAMSIZ - 1);
    ret = ioctl(sfd, SIOCGIFADDR, &ifreq);

    if (ret != 0) {
        LOG(LOG_LEVEL_WARN,
            "Can't get device local address (SIOCGIFADDR): %s\n",
            strerror(errno));

        shutdown(sfd, SHUT_RDWR);
        close(sfd);
        return -1;
    }

    memcpy(local_addr, &(ifreq.ifr_addr), sizeof(ifreq.ifr_addr));

    return sfd;
}

int fetch_broadcast_addr(int sfd, const char *iface, struct sockaddr *braddr) {
    struct ifreq ifreq;
    int ret;

    memset(&ifreq, 0, sizeof(ifreq));
    strncpy(ifreq.ifr_name, iface, IFNAMSIZ - 1);
    ret = ioctl(sfd, SIOCGIFBRDADDR, &ifreq);

    if (ret != 0)
        return -1;

    memcpy(braddr, &(ifreq.ifr_broadaddr), sizeof(*braddr));

    return 0;
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
