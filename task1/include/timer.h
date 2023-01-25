#ifndef _TIMER_H_
# define _TIMER_H_

# include "io-service.h"
# include <time.h>

typedef void (*tmr_job_t)(void *ctx);

struct tmr;
typedef struct tmr tmr_t;

typedef enum timer_class_enum {
    absolute,
    relative,
    periodic,
    none
} timer_class_t;

struct tmr {
    int fd;
    bool armed;
    timer_class_t tmr_class;
    struct itimerspec spec;
    io_service_t *master;
    tmr_job_t job;
    void *ctx;
};

void timer_init(tmr_t *tmr, io_service_t *iosvc);
void timer_deinit(tmr_t *tmr);
void timer_set_periodic(tmr_t *tmr, time_t sec, unsigned long nanosec,
                        tmr_job_t job, void *ctx);
void timer_set_deadline(tmr_t *tmr, time_t sec, unsigned long nanosec,
                        tmr_job_t job, void *ctx);
void timer_set_absolute(tmr_t *tmr, time_t sec, unsigned long nanosec,
                        tmr_job_t job, void *ctx);
void timer_cancel(tmr_t *tmr);

#endif /* _TIMER_H_ */