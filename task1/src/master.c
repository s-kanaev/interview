#include "master.h"
#include "master-private.h"
#include "protocol.h"

#include <assert.h>
#include <unistd.h>
#include <string.h>

static
void timeout(master_t *m) {
    pr_request_t request;
    request.s.s = PR_REQUEST;

    /* TODO send request */
}

static
void data_received(int fd, io_svc_op_t op, master_t *m) {
    pr_signature_t *packet = NULL;

    /* TODO peek, read and parse datagram, ignore if invalid */

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

/**************** public API ****************/
void master_init(master_t *m, io_service_t *iosvc) {
    assert(m && iosvc);

    master_init_(m, iosvc);

    /* TODO allocate UDP socket */
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
                        (iosvc_job_function_t)data_received,
                        m);

    io_service_run(m->iosvc);
}
