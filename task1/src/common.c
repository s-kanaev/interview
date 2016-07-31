#include "common.h"
#include "log.h"

#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>

#include <net/if.h>
#include <sys/ioctl.h>

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

