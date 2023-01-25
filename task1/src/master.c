#include "master.h"
#include "master-private.h"
#include "protocol.h"

#include <assert.h>
#include <unistd.h>
#include <string.h>

#include <netdb.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <net/if.h>
#include <arpa/inet.h>

#include <sys/socket.h>
#include <sys/ioctl.h>

static
void timeout(master_t *m) {
    pr_request_t request;
    request.s.s = PR_REQUEST;

    sendto(m->udp_socket, &request, sizeof(request), 0,
           &m->bcast_addr, sizeof(m->bcast_addr));
}

static
void data_received(int fd, io_svc_op_t op, master_t *m) {
    pr_signature_t *packet = NULL;
    int pending;
    int ret;
    ssize_t bytes_read;
    ssize_t buf_size;
    uint8_t buffer[PR_MAX_SIZE];
    struct sockaddr_storage remote_addr;
    socklen_t remote_addr_len;

    /* peek */
    ret = ioctl(fd, FIONREAD, &pending);
    assert(0 == ret);

    if (pending > PR_MAX_SIZE || pending < PR_MIN_SIZE) {
        /* discard this datagram */
        /* read big chunks */
        while (pending >= PR_MAX_SIZE) {
            bytes_read = recv(fd, buffer, PR_MAX_SIZE, 0);
            assert(bytes_read > 0);

            pending -= bytes_read;
        }

        /* red the rest */
        while (pending) {
            bytes_read = recv(fd, buffer, pending, 0);
            assert(bytes_read > 0);

            pending -= bytes_read;
        }

        return;
    }

    /* read datagram */
    buf_size = 0;
    while (pending) {
        bytes_read = recvfrom(fd, (uint8_t *)(buffer) + buf_size, pending, 0,
                              (struct sockaddr *)&remote_addr, &remote_addr_len);
        assert(bytes_read >= 0);

        pending -= bytes_read;
        buf_size += bytes_read;
    }

    /* parse and act */
    packet = (pr_signature_t *)buffer;
    master_act(m, packet, fd);
}

/**************** private API ****************/
void
master_init_(master_t *m,
             io_service_t *iosvc) {
    m->iosvc = iosvc;

    timer_init(&m->tmr, m->iosvc);

    memset(&m->sum, 0, sizeof(m->sum));
    memset(&m->avg, 0, sizeof(m->avg));

    avl_tree_init(&m->slaves, true, sizeof(slave_description_t));
}

void
master_deinit_(master_t *m) {
    timer_deinit(&m->tmr);
    avl_tree_purge(&m->slaves);
}

void
master_arm_timer(master_t *m) {
    timer_set_periodic(
        &m->tmr,
        MASTER_REQUEST_TIMEOUT_MSEC / 1000,
        (MASTER_REQUEST_TIMEOUT_MSEC % 1000) * 1000000,
        (tmr_job_t)timeout, m
    );
}

void
master_disarm_timer(master_t *m) {
    timer_cancel(&m->tmr);
}

avl_tree_node_t *
master_update_slave(master_t *m,
                    uint32_t ip,
                    const slave_description_t *sd) {
    avl_tree_node_t *atn = avl_tree_get(&m->slaves, ip);
    slave_description_t *atn_sd;

    if (!atn) {
        atn = avl_tree_add(&m->slaves, ip);
        atn_sd = (slave_description_t *)atn->data;
        memset(atn_sd, 0, sizeof(*atn_sd));
    }
    else
        atn_sd = (slave_description_t *)atn->data;

    m->sum.temperature -= atn_sd->temperature;
    m->sum.illumination -= atn_sd->illumination;

    memcpy(atn_sd, sd, sizeof(*atn_sd));

    m->sum.temperature += atn_sd->temperature;
    m->sum.illumination += atn_sd->illumination;
}

void
master_calculate_averages(master_t *m) {
    m->avg.temperature = m->slaves.count
                          ? m->sum.temperature / m->slaves.count
                          : 0;
    m->avg.illumination = m->slaves.count
                          ? m->sum.illumination / m->slaves.count
                          : 0;
}

uint8_t
master_calculate_brightenss(const master_t *m) {
    return m->avg.illumination + 1;
}

void
master_act(master_t *m, const pr_signature_t *packet, int fd) {
    switch (packet->s) {
        case PR_RESPONSE:
            /* TODO update slave, calculate averages, send if needs to */
            break;
        case PR_VOTE:
            /* TODO send master reset packet */
            break;
        default:
            /* do nothing if the packet is neither response nor vote */
            break;
    }
}

void
master_set_broadcast_addr(master_t *m, struct sockaddr *bcast_addr) {
    memcpy(&m->bcast_addr, bcast_addr, sizeof(m->bcast_addr));
}

/**************** public API ****************/
void master_init(master_t *m, io_service_t *iosvc,
                 const char *local_addr) {
    static const int BROADCAST = 1;
    static const int REUSE_ADDR = 1;
    int sfd;
    int ret;
    struct addrinfo *addr_info = NULL, *cur_addr;
    struct addrinfo hint;
    struct ifreq ifreq;

    assert(m && iosvc);

    master_init_(m, iosvc);

    /* find suitable local address */
    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_DGRAM;
    hint.ai_protocol = 0;
    hint.ai_flags = AI_PASSIVE;
    ret = getaddrinfo(local_addr, MASTER_UDP_PORT_STR, &hint, &addr_info);
    assert(0 == ret);

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
        assert(0 == ret);

        ret = setsockopt(sfd,
                         SOL_SOCKET, SO_REUSEADDR,
                         &REUSE_ADDR, sizeof(REUSE_ADDR));
        assert(0 == ret);

        if (!bind(sfd, cur_addr->ai_addr, cur_addr->ai_addrlen))
            break;

        shutdown(sfd, SHUT_RDWR);
        close(sfd);
    }

    assert(NULL == cur_addr);

    m->udp_socket = sfd;
    memcpy(&m->local_addr, cur_addr->ai_addr, sizeof(cur_addr->ai_addrlen));

    /* fetch broadcast addr */
    memset(&ifreq, 0, sizeof(ifreq));
    ret = ioctl(m->udp_socket, SIOCGIFBRDADDR, &ifreq);

    assert(0 == ret);
    master_set_broadcast_addr(m, &(ifreq.ifr_broadaddr));
}

void master_deinit(master_t *m) {
    assert(m);

    master_deinit(m);

    close(m->udp_socket);
}

void master_run(master_t *m) {
    assert(m);

    master_arm_timer(m);

    io_service_post_job(m->iosvc, m->udp_socket, IO_SVC_OP_READ,
                        !IOSVC_JOB_ONESHOT,
                        (iosvc_job_function_t)data_received,
                        m);

    io_service_run(m->iosvc);
}
