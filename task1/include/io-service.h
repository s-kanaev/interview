#ifndef _IO_SERVICE_H_
# define _IO_SERVICE_H_

# include "containers.h"

# include <stdbool.h>
# include <sys/epoll.h>

struct io_service;
typedef struct io_service io_service_t;

/**
 * IO service operation type: read / write
 */
typedef enum io_svc_op {
    IO_SVC_OP_READ = 0,
    IO_SVC_OP_WRITE = 1,
    IO_SVC_OP_COUNT
} io_svc_op_t;

typedef void (*iosvc_job_function_t)(int fd, io_svc_op_t op, void *ctx);

/**
 * Simple IO service.
 * Single-thread implementation.
 */
struct io_service {
    /* are we still running flags */
    bool allow_new;
    bool running;
    /* used for notification purposes */
    int event_fd;
    /* map FD to iosvc_job_function_t */
    list_t lookup_table;

    int epoll_fd;
    struct epoll_event event_fd_event;
};

void io_service_init(io_service_t *iosvc);
void io_service_deinit(io_service_t *iosvc);
void io_service_run(io_service_t *iosvc);
void io_service_stop(io_service_t *iosvc, bool wait_pending);
void io_service_post_job(io_service_t *iosvc,
                         int fd, io_svc_op_t op, bool oneshot,
                         iosvc_job_function_t j, void *ctx);
void io_service_remove_job(io_service_t *iosvc,
                           int fd, io_svc_op_t op);

#endif /* _IO_SERVICE_H_ */