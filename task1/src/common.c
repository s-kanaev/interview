#include "common.h"
#include "log.h"

#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>

#include <net/if.h>
#include <sys/ioctl.h>

int allocate_udp_broadcasting_socket(const char *iface,
                                     uint16_t local_port) {
    static const int BROADCAST = 1;
    static const int REUSE_ADDR = 1;

    int sfd, ret;
    struct sockaddr_in addr;

    sfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (sfd < 0)
        return -1;

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
    if (!bind(sfd, (const struct sockaddr *)&addr, sizeof(addr)))
        return sfd;

    return -1;
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

