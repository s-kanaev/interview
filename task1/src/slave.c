#include "slave.h"
#include "master-private.h"
#include "master.h"
#include "protocol.h"
#include "common.h"
#include "log.h"

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
#include <net/if.h>
#include <arpa/inet.h>

#include <sys/socket.h>
#include <sys/ioctl.h>

#define MASTERING_TIMEOUT_MSEC      (MASTER_REQUEST_TIMEOUT_MSEC / 2)

typedef void (slave_actor_t)(slave_t *m, const pr_signature_t *packet, int fd,
                             const struct sockaddr_in *remote_addr);

static
void slave_master_timed_out(slave_t *sl);
static
slave_actor_t slave_act,
              slave_act_idle,
              slave_act_poll,
              slave_act_master,
              slave_act_waiting_master;
static
void data_received(int fd, io_svc_op_t op, slave_t *sl);

static
void slave_arm_master_gone_timer(slave_t *sl);
static
void slave_disarm_master_gone_timer(slave_t *sl);
static
void slave_arm_poll_timer(slave_t *sl);
static
void slave_disarm_poll_timer(slave_t *sl);
static
void slave_arm_mastering_timer(slave_t *sl);
static
void slave_disarm_mastering_timer(slave_t *sl);

static
void slave_initialize_master_polling(slave_t *sl);
static
void slave_finish_master_polling(slave_t *sl, slave_state_t new_state);
static
void slave_poll_timeout(slave_t *sl);
static
void slave_fetch_parameters(slave_t *sl);
static
void slave_memorize(slave_t *sl, const pr_msg_t *msg);
static
void slave_poll(slave_t *sl);
static
void slave_mastering_timeout(slave_t *sl);


/* static data */
static const
slave_actor_t *ACTORS[] = {
    [SLAVE_IDLE]            = slave_act_idle,
    [SLAVE_POLLING]         = slave_act_poll,
    [SLAVE_MASTER]          = slave_act_master,
    [SLAVE_WAITING_MASTER]  = slave_act_waiting_master
};

/**************** private ****************/
void slave_memorize(slave_t *sl, const pr_msg_t *msg) {
    LOG(LOG_LEVEL_INFO,
        "Memorize:\nText: %s\nTemperature: %d\nBrightness: %u\nDate/time: %ld\n",
        (char *)msg->text,
        (int)msg->avg_temperature,
        (int)msg->brightness,
        (long int)msg->date_time
    );
}

void slave_fetch_parameters(slave_t *sl) {
    sl->temperature = random();
    sl->illumination = random();
}

void slave_mastering_timeout(slave_t *sl) {
    pr_response_t response;
    struct sockaddr_in special_addr;

    switch (sl->state) {
        case SLAVE_MASTER:
            slave_fetch_parameters(sl);

            response.s.s = PR_RESPONSE;
            response.illumination = sl->illumination;
            response.temperature = sl->temperature;

            special_addr.sin_addr.s_addr = htonl(0x00000000);
            special_addr.sin_port = htons(UDP_PORT);
            special_addr.sin_family = AF_INET;

            master_act(&sl->master, (const pr_signature_t *)&response,
                       sl->udp_socket, &special_addr);
            break;

        default:
            timer_cancel(&sl->mastering_tmr);
            break;
    }
}

void slave_poll(slave_t *sl) {
    pr_vote_t vote;
    long int rnd;
    uint32_t v;

    rnd = random();
    v = rnd & 0xffffffff;

    if (!(v > sl->max_vote_per_poll)) {
        slave_finish_master_polling(sl, SLAVE_WAITING_MASTER);
        return;
    }

    sl->max_vote_per_poll = v;

    vote.s.s = PR_VOTE;
    vote.vote = v;

    sendto(sl->udp_socket, &vote, sizeof(vote), 0,
           (struct sockaddr *)&sl->bcast_addr, sizeof(sl->bcast_addr));

    slave_arm_poll_timer(sl);
}

void slave_act_idle(slave_t *sl, const pr_signature_t *packet, int fd,
                    const struct sockaddr_in *remote_addr) {
    pr_response_t response;

    switch (packet->s) {
        case PR_REQUEST:
            slave_arm_master_gone_timer(sl);
            slave_fetch_parameters(sl);
            response.s.s = PR_RESPONSE;
            response.illumination = sl->illumination;
            response.temperature = sl->temperature;

            sendto(sl->udp_socket, &response, sizeof(response), 0,
                   (struct sockaddr *)&sl->bcast_addr, sizeof(sl->bcast_addr));
            break;

        case PR_MSG:
            slave_arm_master_gone_timer(sl);
            slave_memorize(sl, (const pr_msg_t *)packet);
            break;

        default:
            break;
    }
}

void slave_act_poll(slave_t *sl, const pr_signature_t *packet, int fd,
                    const struct sockaddr_in *remote_addr) {
    const pr_vote_t *vote;
    const pr_reset_master_t *reset;

    switch (packet->s) {
        case PR_VOTE:
            vote = (const pr_vote_t *)packet;
            if (vote->vote > sl->max_vote_per_poll) {
                slave_finish_master_polling(sl, SLAVE_WAITING_MASTER);
                break;
            }

            slave_poll(sl);
            break;

        case PR_RESET_MASTER:
            reset = (const pr_reset_master_t *)reset;
            slave_finish_master_polling(sl, SLAVE_IDLE);
            break;
    }
}

void slave_act_master(slave_t *sl, const pr_signature_t *packet, int fd,
                      const struct sockaddr_in *remote_addr) {
    const pr_vote_t *vote;
    pr_response_t response;

    switch (packet->s) {
        case PR_VOTE:
            slave_finish_master_polling(sl, SLAVE_IDLE);

            vote = (const pr_vote_t *)packet;

            sl->max_vote_per_poll = vote->vote;

            slave_initialize_master_polling(sl);
            break;

        case PR_REQUEST:
            slave_finish_master_polling(sl, SLAVE_IDLE);

            slave_fetch_parameters(sl);
            response.s.s = PR_RESPONSE;
            response.illumination = sl->illumination;
            response.temperature = sl->temperature;

            sendto(sl->udp_socket, &response, sizeof(response), 0,
                   (struct sockaddr *)&sl->bcast_addr, sizeof(sl->bcast_addr));
            break;

        case PR_MSG:
            slave_finish_master_polling(sl, SLAVE_IDLE);

            slave_memorize(sl, (const pr_msg_t *)packet);
            break;

        case PR_RESET_MASTER:
            slave_finish_master_polling(sl, SLAVE_IDLE);
            break;
    }
}

void slave_act_waiting_master(slave_t *sl, const pr_signature_t *packet, int fd,
                              const struct sockaddr_in *remote_addr) {
    const pr_vote_t *vote;
    const pr_reset_master_t *reset;

    switch (packet->s) {
        case PR_VOTE:
            slave_finish_master_polling(sl, SLAVE_IDLE);

            vote = (const pr_vote_t *)packet;

            sl->max_vote_per_poll = vote->vote;

            slave_initialize_master_polling(sl);
            break;

        case PR_REQUEST:
            master_act(&sl->master, packet, fd, remote_addr);
            break;

        case PR_RESET_MASTER:
            slave_arm_master_gone_timer(sl);
            slave_finish_master_polling(sl, SLAVE_IDLE);
            break;
    }
}

void slave_master_timed_out(slave_t *sl) {
    slave_initialize_master_polling(sl);
}

void slave_poll_timeout(slave_t *sl) {
    if (sl->state == SLAVE_POLLING) {
        slave_finish_master_polling(sl, SLAVE_MASTER);
        master_init_(&sl->master, sl->iosvc);
        master_set_broadcast_addr(
            &sl->master,
            (const struct sockaddr *)&sl->bcast_addr);
        sl->master.udp_socket = sl->udp_socket;
        master_start(&sl->master);
    }
}

void slave_initialize_master_polling(slave_t *sl) {
    LOG(LOG_LEVEL_DEBUG,
        "Initialize master poll from state: %d\n", (int)sl->state);

    sl->state = SLAVE_POLLING;

    slave_poll(sl);
}

void slave_finish_master_polling(slave_t *sl, slave_state_t new_state) {
    LOG(LOG_LEVEL_DEBUG,
        "Finish master poll: %d -> %d\n", (int)sl->state, (int)new_state);

    if (SLAVE_POLLING == sl->state)
        slave_disarm_poll_timer(sl);

    if (SLAVE_MASTER == sl->state) {
        master_deinit_(&sl->master);
        timer_cancel(&sl->mastering_tmr);
    }

    sl->state = new_state;
    sl->max_vote_per_poll = 0;

    if (SLAVE_WAITING_MASTER == sl->state || SLAVE_IDLE == sl->state)
        slave_arm_master_gone_timer(sl);
}

void slave_act(slave_t *sl, const pr_signature_t *packet, int fd,
               const struct sockaddr_in *remote_addr) {
    LOG(LOG_LEVEL_DEBUG,
        "Acting within state %#02x for signature: %#02x, from: %s\n",
        (int)sl->state,
        (int)packet->s,
        inet_ntoa(remote_addr->sin_addr));
    ACTORS[sl->state](sl, packet, fd, remote_addr);
}

void data_received(int fd, io_svc_op_t op, slave_t *sl) {
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
        ((struct sockaddr_in *)&sl->local_addr)->sin_addr.s_addr) /* discard */
        return;

    slave_act(sl, packet, fd, (struct sockaddr_in *)&remote_addr);
}

void slave_arm_master_gone_timer(slave_t *sl) {
    timer_set_deadline(
        &sl->master_gone_tmr,
        MASTER_GONE_TIMEOUT_MSEC / 1000,
        (MASTER_GONE_TIMEOUT_MSEC % 1000) * 1000000,
        (tmr_job_t)slave_master_timed_out, sl
    );
}

void slave_disarm_master_gone_timer(slave_t *sl) {
    timer_cancel(&sl->master_gone_tmr);
}

void slave_arm_poll_timer(slave_t *sl) {
    timer_set_deadline(
        &sl->poll_tmr,
        SLAVE_POLLING_TIMEOUT_MSEC / 1000,
        (SLAVE_POLLING_TIMEOUT_MSEC % 1000) * 1000000,
        (tmr_job_t)slave_poll_timeout, sl
    );
}

void slave_disarm_poll_timer(slave_t *sl) {
    timer_cancel(&sl->poll_tmr);
}

void slave_arm_mastering_timer(slave_t *sl) {
    timer_set_periodic(
        &sl->poll_tmr,
        MASTERING_TIMEOUT_MSEC / 1000,
        (MASTERING_TIMEOUT_MSEC % 1000) * 1000000,
        (tmr_job_t)slave_mastering_timeout, sl
    );
}

void slave_disarm_mastering_timer(slave_t *sl) {
    timer_cancel(&sl->poll_tmr);
}

/**************** API ****************/
void slave_init(slave_t *sl, io_service_t *iosvc,
                const char *iface) {
    struct addrinfo addr;
    struct sockaddr brcast_addr;

    assert(sl && iosvc && iface);

    sl->iosvc = iosvc;

    timer_init(&sl->master_gone_tmr, sl->iosvc);
    timer_init(&sl->poll_tmr, sl->iosvc);
    timer_init(&sl->mastering_tmr, sl->iosvc);

    /* find suitable local address */
    sl->udp_socket = allocate_udp_broadcasting_socket(iface, UDP_PORT);

    if (sl->udp_socket < 0) {
        LOG(LOG_LEVEL_FATAL,
            "Can't locate suitable socket: %s",
            strerror(errno));

        abort();
    }

    memcpy(&sl->local_addr, &addr.ai_addr, sizeof(addr.ai_addrlen));

    /* fetch broadcast addr */
    if (0 > fetch_broadcast_addr(sl->udp_socket, iface, &brcast_addr)) {
        LOG(LOG_LEVEL_FATAL,
            "Can't fetch broadcast address with ioctl(SIOCGIFBRDADDR): %s\n",
            strerror(errno));

        abort();
    }

    memcpy(&sl->bcast_addr, &brcast_addr, sizeof(brcast_addr));
    sl->bcast_addr.sin_port = htons(UDP_PORT);

    sl->illumination = 0;
    sl->temperature = 0;
}

void slave_deinit(slave_t *sl) {
    timer_deinit(&sl->master_gone_tmr);
    timer_deinit(&sl->poll_tmr);
    timer_deinit(&sl->mastering_tmr);

    shutdown(sl->udp_socket, SHUT_RDWR);
    close(sl->udp_socket);
}

void slave_run(slave_t *sl) {
    assert(sl);

    sl->state = SLAVE_IDLE;

    slave_arm_master_gone_timer(sl);

    io_service_post_job(sl->iosvc, sl->udp_socket, IO_SVC_OP_READ,
                        !IOSVC_JOB_ONESHOT,
                        (iosvc_job_function_t)data_received,
                        sl);

    LOG(LOG_LEVEL_INFO, "Starting (pid: %d)\n", getpid());

    io_service_run(sl->iosvc);
}
