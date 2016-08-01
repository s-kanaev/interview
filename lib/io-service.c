#include "io-service.h"

#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#include <sys/eventfd.h>
#include <sys/epoll.h>

typedef struct job {
    iosvc_job_function_t job;
    void *ctx;
    bool oneshot;
} job_t;

typedef struct lookup_job_element {
    int fd;
    struct epoll_event event;
    job_t job[IO_SVC_OP_COUNT];
} lookup_job_element_t;

static const int OP_FLAGS[IO_SVC_OP_COUNT] = {
    [IO_SVC_OP_READ] = EPOLLIN,
    [IO_SVC_OP_WRITE] = EPOLLOUT
};

static
void notify_svc(int fd) {
    eventfd_write(fd, 1);
}

static
eventfd_t svc_notified(int fd) {
    eventfd_t v;
    eventfd_read(fd, &v);
    return v;
}

void io_service_init(io_service_t *iosvc) {
    int r;

    assert(iosvc);

    iosvc->event_fd = eventfd(0, EFD_CLOEXEC | EFD_SEMAPHORE);

    assert(iosvc->event_fd >= 0);

    list_init(&iosvc->lookup_table, true, sizeof(lookup_job_element_t));

    iosvc->allow_new = true;
    iosvc->running = false;

    iosvc->epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    assert(iosvc->epoll_fd >= 0);

    memset(&iosvc->event_fd_event, 0, sizeof(iosvc->event_fd_event));

    iosvc->event_fd_event.events = EPOLLIN;
    iosvc->event_fd_event.data.fd = iosvc->event_fd;

    assert(0 == epoll_ctl(iosvc->epoll_fd, EPOLL_CTL_ADD,
                          iosvc->event_fd, &iosvc->event_fd_event));
}

void io_service_deinit(io_service_t *iosvc) {
    assert(iosvc);

    close(iosvc->event_fd);
    close(iosvc->epoll_fd);

    list_purge(&iosvc->lookup_table);
}

void io_service_stop(io_service_t *iosvc, bool wait_pending) {
    iosvc->allow_new = false;
    iosvc->running = wait_pending;
    notify_svc(iosvc->event_fd);
}

void io_service_post_job(io_service_t *iosvc,
                         int fd, io_svc_op_t op, bool oneshot,
                         iosvc_job_function_t j, void *ctx) {
    list_element_t *le;
    lookup_job_element_t *lje;

    assert(iosvc);

    if (iosvc->allow_new && j) {
        job_t *new_job;

        for (lje = NULL, le = list_begin(&iosvc->lookup_table); le;
             le = list_next(&iosvc->lookup_table, le)) {
            lje = (lookup_job_element_t *)le->data;
            if (lje->fd == fd)
                break;
        }

        if (!le) {
            le = list_append(&iosvc->lookup_table);
            lje = (lookup_job_element_t *)le->data;

            lje->event.data.fd = lje->fd = fd;
            lje->event.events = 0;
            memset(lje->job, 0, sizeof(lje->job));
        }

        if (lje->job[op].job == NULL) {
            lje->event.events |= OP_FLAGS[op];
            lje->job[op].job = j;
            lje->job[op].ctx = ctx;
            lje->job[op].oneshot = oneshot;

            if (iosvc->running)
                notify_svc(iosvc->event_fd);
        }
    }
}

void io_service_remove_job(io_service_t *iosvc,
                           int fd, io_svc_op_t op) {
    bool done = false;
    lookup_job_element_t *lje;
    list_element_t *le;

    assert(iosvc);

    for (le = list_begin(&iosvc->lookup_table); le;
         le = list_next(&iosvc->lookup_table, le)) {
        lje = (lookup_job_element_t *)le->data;
        if (lje->fd == fd) {
            done = !!lje->job[op].job;
            lje->event.events &= ~OP_FLAGS[op];
            lje->job[op].job = NULL;
            break;
        }
    }

    if (done && iosvc->running)
        notify_svc(iosvc->event_fd);
}

void io_service_run(io_service_t *iosvc) {
    volatile bool *running;
    struct epoll_event event;
    int r, fd;
    ssize_t idx;
    io_svc_op_t op;
    lookup_job_element_t *lje, *prev_lje;
    list_element_t *le, *prev_le;
    iosvc_job_function_t job;
    void *ctx;

    assert(iosvc);

    running = &iosvc->running;

    for (le = list_begin(&iosvc->lookup_table); le;
         le = list_next(&iosvc->lookup_table, le)) {
        lje = (lookup_job_element_t *)le->data;
        epoll_ctl(iosvc->epoll_fd, EPOLL_CTL_ADD, lje->fd, &lje->event);
    }

    *running = true;

    while (*running) {
        r = epoll_wait(iosvc->epoll_fd, &event, 1, -1);

        if (r < 0)
            continue;

        fd = event.data.fd;

        if (fd == iosvc->event_fd) {
            svc_notified(fd);

            if ((list_size(&iosvc->lookup_table) == 0) && (iosvc->allow_new == false))
                *running = false;

            for (le = list_begin(&iosvc->lookup_table); le;) {
                lje = (lookup_job_element_t *)le->data;

                if (lje->event.events == 0) {
                    epoll_ctl(iosvc->epoll_fd, EPOLL_CTL_DEL, lje->fd, NULL);
                    le = list_remove_and_advance(&iosvc->lookup_table, le);
                    continue;
                }

                if (epoll_ctl(iosvc->epoll_fd, EPOLL_CTL_MOD, lje->fd, &lje->event))
                    if (errno == ENOENT)
                        epoll_ctl(iosvc->epoll_fd, EPOLL_CTL_ADD, lje->fd, &lje->event);

                le = list_next(&iosvc->lookup_table, le);
            }

            continue;
        }

        for (op = 0; op < IO_SVC_OP_COUNT; ++op) {
            if (!(event.events & OP_FLAGS[op]))
                continue;

            for (le = list_begin(&iosvc->lookup_table); le;
                 le = list_next(&iosvc->lookup_table, le)) {
                lje = (lookup_job_element_t *)le->data;
                if (lje->fd == fd) {
                    if (lje->job[op].job != NULL) break;
                    else {
                        lje = NULL;
                        break;
                    }
                }
            }

            if (le) {
                lje = (lookup_job_element_t *)le->data;
                job = lje->job[op].job;
                ctx = lje->job[op].ctx;

                if (lje->job[op].oneshot) {
                    lje->job[op].ctx = lje->job[op].job = NULL;
                    lje->event.events &= ~OP_FLAGS[op];

                    if (lje->event.events == 0) {
                        epoll_ctl(iosvc->epoll_fd, EPOLL_CTL_DEL, lje->fd, NULL);
                        list_remove_and_advance(&iosvc->lookup_table, le);
                    }
                }

                if (job)
                    (*job)(fd, op, ctx);
                else {
                    if (le)
                        epoll_ctl(iosvc->epoll_fd,
                                  EPOLL_CTL_MOD, lje->fd, &lje->event);
                    else
                        epoll_ctl(iosvc->epoll_fd,
                                  EPOLL_CTL_DEL, fd, NULL);
                }
            } /* if (lje) */
        }   /* for (op = 0; op < IO_SVC_OP_COUNT; ++op) */
    }   /* while (*running) */
}
