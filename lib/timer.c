#include "timer.h"
#include "io-service.h"

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <sys/timerfd.h>

static void tmr_job_tpl(int fd, io_svc_op_t op, void *_ctx) {
    tmr_t *timer = _ctx;
    uint64_t stub;

    read(fd, &stub, sizeof(stub));

    timer->job(timer->ctx);

    if (timer->tmr_class != periodic)
        timer->armed = false;
}

void timer_init(tmr_t* timer, io_service_t* iosvc) {
    int fd;

    assert(timer && iosvc);

    fd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC);

    assert(fd >= 0);

    timer->fd = fd;
    timer->armed = false;
    timer->master = iosvc;
    timer->tmr_class = none;
}

void timer_deinit(tmr_t* tmr) {
    timer_cancel(tmr);
    close(tmr->fd);
}

void timer_set_deadline(tmr_t *tmr,
                        time_t sec, long unsigned int nanosec,
                        tmr_job_t job, void *ctx) {
    tmr->job = job;
    tmr->ctx = ctx;

    tmr->tmr_class = relative;
    tmr->spec.it_value.tv_sec = sec;
    tmr->spec.it_value.tv_nsec = nanosec;
    tmr->spec.it_interval.tv_sec = tmr->spec.it_interval.tv_nsec = 0;
    timerfd_settime(
        tmr->fd, 0,
        &tmr->spec,
        NULL
    );

    tmr->armed = true;

    io_service_post_job(tmr->master,
                        tmr->fd, IO_SVC_OP_READ, true,
                        tmr_job_tpl, tmr);
}

void timer_set_periodic(tmr_t *tmr,
                        time_t sec, long unsigned int nanosec,
                        tmr_job_t job, void *ctx) {
    tmr->job = job;
    tmr->ctx = ctx;

    tmr->tmr_class = periodic;
    tmr->spec.it_value.tv_sec = sec;
    tmr->spec.it_value.tv_nsec = nanosec;
    tmr->spec.it_interval.tv_sec = sec;
    tmr->spec.it_interval.tv_nsec = nanosec;
    timerfd_settime(
        tmr->fd, 0,
        &tmr->spec,
        NULL
    );

    tmr->armed = true;

    io_service_post_job(tmr->master,
                        tmr->fd, IO_SVC_OP_READ, false,
                        tmr_job_tpl, tmr);
}

void timer_set_absolute(tmr_t *tmr,
                        time_t sec, long unsigned int nanosec,
                        tmr_job_t job, void *ctx) {
    tmr->job = job;
    tmr->ctx = ctx;

    tmr->tmr_class = absolute;
    tmr->spec.it_value.tv_sec = sec;
    tmr->spec.it_value.tv_nsec = nanosec;
    tmr->spec.it_interval.tv_sec = tmr->spec.it_interval.tv_nsec = 0;
    timerfd_settime(
        tmr->fd, TFD_TIMER_ABSTIME,
        &tmr->spec,
        NULL
    );

    tmr->armed = true;

    io_service_post_job(tmr->master,
                        tmr->fd, IO_SVC_OP_READ, true,
                        tmr_job_tpl, tmr);
}

void timer_cancel(tmr_t *tmr) {
    io_service_remove_job(tmr->master,
                          tmr->fd, IO_SVC_OP_READ);
}
