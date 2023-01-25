#include "master.h"
#include "protocol.h"

#include <assert.h>
#include <unistd.h>

typedef struct slave_description {
    int8_t temperature;
    uint8_t illumination;
} slave_description_t;

static
void timeout(master_t *m) {
    /* TODO send request */
}

void master_init(master_t *m, io_service_t *iosvc) {
    assert(m && iosvc);

    m->iosvc = iosvc;
    timer_init(&m->tmr, m->iosvc);

    m->last_avg_timestamp = 0;

    avl_tree_init(&m->slaves, true, sizeof(slave_description_t));
    /* TODO allocate UDP socket */
}

void master_deinit(master_t *m) {
    assert(m);

    close(m->udp_socket);
    timer_deinit(&m->tmr);
}

void master_run(master_t *m) {
    assert(m);

    timer_set_periodic(
        &m->tmr,
        MASTER_REQUEST_TIMEOUT_MSEC / 1000,
        (MASTER_REQUEST_TIMEOUT_MSEC % 1000) * 1000000,
        (tmr_job_t)timeout, m
    );

    io_service_post_job(m->iosvc, m->udp_socket, IO_SVC_OP_READ, ...);
    io_service_run(m->iosvc);
}
