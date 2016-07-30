#include "common.h"
#include "log.h"

#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>

#include <net/if.h>
#include <sys/ioctl.h>

int allocate_udp_broadcasting_socket(const char *local_addr,
                                     const char *local_port,
                                     struct addrinfo *addr) {
    static const int BROADCAST = 1;
    static const int REUSE_ADDR = 1;

    struct addrinfo *addr_info = NULL, *cur_addr;
    struct addrinfo hint;
    int ret;
    int sfd = -1;

    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_DGRAM;
    hint.ai_protocol = 0;
    hint.ai_flags = AI_PASSIVE;
    ret = getaddrinfo(local_addr, local_port, &hint, &addr_info);

    if (ret != 0) {
        LOG(LOG_LEVEL_FATAL,
            "Couldn't getaddrinfo: %s\n",
            strerror(errno));
        return -1;
    }

    for (cur_addr = addr_info; cur_addr != NULL; cur_addr = cur_addr->ai_next) {
        sfd = socket(cur_addr->ai_family,
                     cur_addr->ai_socktype | SOCK_CLOEXEC,
                     cur_addr->ai_protocol);

        sfd = socket(AF_INET, SOCK_DGRAM, 0);

        if (sfd < 0)
            continue;

        ret = setsockopt(sfd,
                         SOL_SOCKET, SO_BROADCAST,
                         &BROADCAST, sizeof(BROADCAST));

        if (ret != 0) {
            LOG(LOG_LEVEL_WARN,
                "Can't set socket option (SO_BROADCAST): %s\n",
                strerror(errno));

            continue;
        }

        ret = setsockopt(sfd,
                         SOL_SOCKET, SO_REUSEADDR,
                         &REUSE_ADDR, sizeof(REUSE_ADDR));

        if (ret != 0) {
            LOG(LOG_LEVEL_WARN,
                "Can't set socket option (SO_REUSEADDR): %s\n",
                strerror(errno));

            continue;
        }

        if (!bind(sfd, cur_addr->ai_addr, cur_addr->ai_addrlen))
            break;

        shutdown(sfd, SHUT_RDWR);
        close(sfd);
    }

    if (cur_addr == NULL) {
        LOG(LOG_LEVEL_FATAL,
            "Can't locate suitable address for %s\n",
            local_addr);

        freeaddrinfo(addr_info);
        return -1;
    }

    memcpy(addr, cur_addr, sizeof(*addr_info));
    freeaddrinfo(addr_info);

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

