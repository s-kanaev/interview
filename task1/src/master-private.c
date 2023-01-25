#include "master-private.h"
#include "protocol.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <string.h>

static
void timeout(master_t *m) {
    pr_request_t request;
    request.s.s = PR_REQUEST;

    sendto(m->udp_socket, &request, sizeof(request), 0,
           (struct sockaddr *)&m->bcast_addr, sizeof(m->bcast_addr));
}

static
void reset_master(master_t *m) {
    pr_reset_master_t reset;
    reset.s.s = PR_RESET_MASTER;

    sendto(m->udp_socket, &reset, sizeof(reset), 0,
           (struct sockaddr *)&m->bcast_addr, sizeof(m->bcast_addr));
}

static
void send_info_message(master_t *m, uint8_t brightness, int fd) {
    pr_msg_t msg;
    msg.s.s = PR_MSG;
    msg.avg_temperature = m->avg.temperature;
    msg.brightness = brightness;
    msg.date_time = time(NULL);
    snprintf((char *)msg.text, sizeof(msg.text), "%d", (int)m->avg.temperature);
    /*msg.text[sizeof(msg.text) - 1] = '\0';*/

    sendto(m->udp_socket, &msg, sizeof(msg), 0,
           (struct sockaddr *)&m->bcast_addr, sizeof(m->bcast_addr));
}

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

bool
master_calculate_averages(master_t *m) {
    int8_t prev_temperature = m->avg.temperature;
    uint8_t prev_illumination = m->avg.illumination;

    m->avg.temperature = m->slaves.count
                          ? m->sum.temperature / m->slaves.count
                          : 0;
    m->avg.illumination = m->slaves.count
                          ? m->sum.illumination / m->slaves.count
                          : 0;

    return !((prev_temperature == m->avg.temperature) &&
             (prev_illumination == m->avg.illumination));
}

uint8_t
master_calculate_brightenss(const master_t *m) {
    return m->avg.illumination + 1;
}

void
master_act(master_t *m, const pr_signature_t *packet, int fd,
           const struct sockaddr_in *remote_addr) {
    const pr_response_t *response;
    const pr_vote_t *vote;
    avl_tree_node_t *updated_slave;
    uint32_t slave_addr;
    slave_description_t sd;
    uint8_t brightness;
    bool avg_changed = false;

    switch (packet->s) {
        case PR_RESPONSE:
            /* parse */
            response = (const pr_response_t *)(packet + 1);
            slave_addr = ntohl(remote_addr->sin_addr.s_addr);

            sd.illumination = response->illumination;
            sd.temperature = response->temperature;

            /* update slave, calculate averages, send new info msg if need to */
            updated_slave = master_update_slave(m, slave_addr, &sd);
            avg_changed = master_calculate_averages(m);

            if (!avg_changed)
                break;

            brightness = master_calculate_brightenss(m);
            send_info_message(m, brightness, fd);
            break;

        case PR_VOTE:
            /* send master reset packet */
            vote = (const pr_vote_t *)(vote + 1);
            reset_master(m);
            break;

        default:
            /* do nothing if the packet is neither response nor vote */
            break;
    }
}

void
master_set_broadcast_addr(master_t *m, const struct sockaddr *bcast_addr) {
    memcpy(&m->bcast_addr, bcast_addr, sizeof(*bcast_addr));
    m->bcast_addr.sin_port = htons(UDP_PORT);
}
