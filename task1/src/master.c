#include "master.h"
#include "master-private.h"
#include "protocol.h"
#include "log.h"
#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include <netdb.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <arpa/inet.h>

#include <sys/socket.h>
#include <sys/ioctl.h>

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
    if (ret != 0) {
        LOG(LOG_LEVEL_FATAL,
            "Can't pending datagram size with ioctl(FIONREAD): %s\n",
            strerror(errno));
        abort();
    }

    if (pending > PR_MAX_SIZE || pending < PR_MIN_SIZE) {
        /* discard this datagram */
        /* read big chunks */
        while (pending >= PR_MAX_SIZE) {
            bytes_read = recv(fd, buffer, PR_MAX_SIZE, 0);
            if (bytes_read < 0) {
                LOG(LOG_LEVEL_FATAL,
                    "Can't read data: %s\n",
                    strerror(errno));
                abort();
            }

            pending -= bytes_read;
        }

        /* red the rest */
        while (pending) {
            bytes_read = recv(fd, buffer, pending, 0);
            if (bytes_read < 0) {
                LOG(LOG_LEVEL_FATAL,
                    "Can't read data: %s\n",
                    strerror(errno));
                abort();
            }

            pending -= bytes_read;
        }

        return;
    }

    /* read datagram */
    buf_size = 0;
    while (pending) {
        bytes_read = recvfrom(fd, (uint8_t *)(buffer) + buf_size, pending, 0,
                              (struct sockaddr *)&remote_addr, &remote_addr_len);
        if (bytes_read < 0) {
            LOG(LOG_LEVEL_FATAL,
                "Can't read data: %s\n",
                strerror(errno));
            abort();
        }

        pending -= bytes_read;
        buf_size += bytes_read;
    }

    /* parse and act */
    packet = (pr_signature_t *)buffer;

    if (packet->s >= PR_COUNT) {
        /* invalid, discard*/
        LOG(LOG_LEVEL_WARN,
            "Invalid signature received: %#02x\n",
            (int)(packet->s));

        return;
    }

    if (buf_size != PR_STRUCT_EXPECTED_SIZE[packet->s]) {
        /* invalid, discard */
        LOG(LOG_LEVEL_WARN,
            "Invalid size of datagram received: %lu instead of %lu\n",
            buf_size, PR_STRUCT_EXPECTED_SIZE[packet->s]);

        return;
    }

    if (((struct sockaddr_in *)&remote_addr)->sin_addr.s_addr ==
        ((struct sockaddr_in *)&m->local_addr)->sin_addr.s_addr) /* discard */
        return;

    LOG_LN();

    master_act(m, packet, fd, (struct sockaddr_in *)&remote_addr);
}

/**************** API ****************/
void master_init(master_t *m, io_service_t *iosvc,
                 const char *iface) {
    struct sockaddr brcast_addr;

    assert(m && iosvc && iface);

    master_init_(m, iosvc);

    /* find suitable local address */
    m->udp_socket = allocate_udp_broadcasting_socket(
        iface, UDP_PORT, &m->local_addr);

    if (m->udp_socket < 0) {
        LOG(LOG_LEVEL_FATAL,
            "Can't locate suitable socket: %s",
            strerror(errno));

        abort();
    }

    /* fetch broadcast addr */
    if (0 > fetch_broadcast_addr(m->udp_socket, iface, &brcast_addr)) {
        LOG(LOG_LEVEL_FATAL,
            "Can't fetch broadcast address with ioctl(SIOCGIFBRDADDR): %s\n",
            strerror(errno));

        abort();
    }

    master_set_broadcast_addr(m, &brcast_addr);
}

void master_deinit(master_t *m) {
    assert(m);

    master_deinit(m);

    shutdown(m->udp_socket, SHUT_RDWR);
    close(m->udp_socket);
}

void master_run(master_t *m) {
    assert(m);

    io_service_post_job(m->iosvc, m->udp_socket, IO_SVC_OP_READ,
                        !IOSVC_JOB_ONESHOT,
                        (iosvc_job_function_t)data_received,
                        m);

    /* reset master initialy */
    pr_reset_master_t reset;
    reset.s.s = PR_RESET_MASTER;

    sendto(m->udp_socket, &reset, sizeof(reset), 0,
           (struct sockaddr *)&m->bcast_addr, sizeof(m->bcast_addr));

    master_start(m);

    LOG(LOG_LEVEL_DEBUG,
        "Running (pid: %d)\n", getpid());

    io_service_run(m->iosvc);
}
