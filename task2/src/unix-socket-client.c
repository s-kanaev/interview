#include "unix-socket-client.h"
#include "common.h"
#include "log.h"

#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/socket.h>

#include <linux/un.h>

static
void connector(int fd, io_svc_op_t op, usc_t *usc);

static
void data_may_be_sent(int fd, io_svc_op_t op, usc_t *usc);

static
void data_may_be_read(int fd, io_svc_op_t op, usc_t *usc);

static inline
bool unix_socket_client_connect_(usc_t *usc,
                                 const char *name, size_t name_len,
                                 usc_connector_t _connector, void *ctx,
                                 bool internal);

static inline
void unix_socket_client_disconnect_(usc_t *usc, bool internal);

/**************** private ****************/
void connector(int fd, io_svc_op_t op, usc_t *usc) {
    usc->connected = true;
    if (usc->connector)
        usc->connector(usc, usc->connector_ctx);
}

void data_may_be_sent(int fd, io_svc_op_t op, usc_t *usc) {
    size_t bytes_wrote = usc->write_task.currently_wrote;
    size_t bytes_to_write = usc->write_task.b.user_size - bytes_wrote;
    ssize_t current_write;
    int err;

    errno = 0;
    while (bytes_to_write) {
        current_write = send(usc->fd,
                             usc->write_task.b.data + bytes_wrote,
                             bytes_to_write,
                             MSG_DONTWAIT | MSG_NOSIGNAL);

        if (current_write < 0) {
            if (errno == EINTR) {
                errno = 0;
                continue;
            }
            else
                break;
        }

        bytes_wrote += current_write;
        bytes_to_write -= current_write;
    }

    usc->write_task.currently_wrote = bytes_wrote;

    err = errno;

    if (bytes_to_write && (errno == EAGAIN || errno == EWOULDBLOCK))
        return;

    io_service_remove_job(usc->iosvc, fd, IO_SVC_OP_WRITE);

    if (usc->write_task.writer)
        usc->write_task.writer(usc, err, usc->write_task.ctx);
}

void data_may_be_read(int fd, io_svc_op_t op, usc_t *usc) {
    size_t bytes_read = usc->read_task.currently_read;
    size_t bytes_to_read = usc->read_task.b.user_size - bytes_read;
    ssize_t current_read;
    bool eof = false;
    int err;

    errno = 0;
    while (bytes_to_read && !eof) {
        current_read = recv(usc->fd,
                            usc->read_task.b.data
                                + usc->read_task.b.offset + bytes_read,
                            bytes_to_read,
                            MSG_DONTWAIT | MSG_NOSIGNAL);

        if (current_read < 0) {
            if (errno == EINTR) {
                errno = 0;
                continue;
            }
            else
                break;
        }

        if (current_read == 0) /* EOF */
            eof = true;

        bytes_read += current_read;
        bytes_to_read -= current_read;
    }

    usc->read_task.currently_read = bytes_read;

    err = errno;

    usc->eof = eof;
    if (eof) {
        io_service_remove_job(usc->iosvc, fd, IO_SVC_OP_READ);

        if (usc->read_task.reader)
            usc->read_task.reader(usc, err, usc->read_task.ctx);
    }

    if (bytes_to_read && (errno == EAGAIN || errno == EWOULDBLOCK))
        return;

    io_service_remove_job(usc->iosvc, fd, IO_SVC_OP_READ);

    if (usc->read_task.reader)
        usc->read_task.reader(usc, err, usc->read_task.ctx);
}

bool unix_socket_client_connect_(usc_t *usc,
                                 const char *name, size_t name_len,
                                 usc_connector_t _connector, void *ctx,
                                 bool internal) {
    int flags;
    int rc;
    struct sockaddr_un addr;

    usc->connector = _connector;
    usc->connector_ctx = ctx;

    flags = fcntl(usc->fd, F_GETFL);

    if (flags < 0) {
        LOG(LOG_LEVEL_WARN, "Can't fetch socket status flags: %s\n",
            strerror(errno));
        LOG_MSG(LOG_LEVEL_WARN, "Will connect synchronously\n");
    }
    else {
        flags |= O_NONBLOCK;
        if (0 > fcntl(usc->fd, F_SETFL, flags)) {
            LOG(LOG_LEVEL_WARN, "Can't socket socket status flags: %s\n",
                strerror(errno));
            LOG_MSG(LOG_LEVEL_WARN, "Will connect synchronously\n");
        }
    }

    memset(&addr.sun_path, 0, sizeof(addr.sun_path));

    addr.sun_family = AF_UNIX;
    memcpy(addr.sun_path, name, name_len < UNIX_PATH_MAX ?
                                name_len : UNIX_PATH_MAX);
    addr.sun_path[UNIX_PATH_MAX - 1] = '\0';

    rc = connect(usc->fd, (struct sockaddr *)&addr, sizeof(addr));

    if (0 == rc) {
        if (!internal) {
            usc->connected_to_name = (char *)malloc(name_len);
            memcpy(usc->connected_to_name, name, name_len);
            usc->connected_to_name_len = name_len;
        }

        connector(usc->fd, IO_SVC_OP_WRITE, usc);
        return true;
    }

    if (errno == EINPROGRESS) {
        if (!internal) {
            usc->connected_to_name = (char *)malloc(name_len);
            memcpy(usc->connected_to_name, name, name_len);
            usc->connected_to_name_len = name_len;
        }

        io_service_post_job(usc->iosvc, usc->fd,
                            IO_SVC_OP_WRITE, IOSVC_JOB_ONESHOT,
                            (iosvc_job_function_t)connector,
                            usc);
        return true;
    }

    return false;
}

void unix_socket_client_disconnect_(usc_t *usc, bool internal) {
    shutdown(usc->fd, SHUT_RDWR);

    if (!internal) {
        close(usc->fd);

        free(usc->connected_to_name);
        usc->connected_to_name = NULL;
        usc->connected_to_name_len = 0;
    }

    usc->connected = false;
}

/**************** API ****************/
bool unix_socket_client_init(usc_t *usc,
                             const char *name, size_t name_len,
                             io_service_t *iosvc) {
    assert(usc);

    memset(usc, 0, sizeof(*usc));

    usc->iosvc = iosvc;
    usc->connector = NULL;

    usc->fd = allocate_unix_socket(name, name_len);

    if (usc->fd < 0)
        return false;

    usc->eof = false;
    usc->connected = false;

    buffer_init(&usc->read_task.b, 0, bp_non_shrinkable);
    buffer_init(&usc->write_task.b, 0, bp_non_shrinkable);

    usc->name = (char *)malloc(name_len);
    memcpy(usc->name, name, name_len);
    usc->name_len = name_len;

    return true;
}

void unix_socket_client_deinit(usc_t *usc) {
    assert(usc);

    unix_socket_client_disconnect(usc);

    unlink(usc->name);
    free(usc->name);
}

bool unix_socket_client_connect(usc_t *usc,
                                const char *name, size_t name_len,
                                usc_connector_t _connector, void *ctx) {
    assert(usc && name && name_len);

    return unix_socket_client_connect_(usc, name, name_len,
                                       _connector, ctx, false);
}

void unix_socket_client_disconnect(usc_t *usc) {
    assert(usc && usc->connected);

    unix_socket_client_disconnect_(usc, false);
}

void unix_socket_client_send(usc_t *usc,
                             const void *d, size_t sz,
                             usc_writer_t writer, void *ctx) {
    assert(usc);

    memset(&(usc->write_task), 0, sizeof(usc->write_task));
    buffer_realloc(&usc->write_task.b, sz);
    memcpy(usc->write_task.b.data, d, sz);
    usc->write_task.writer = writer;
    usc->write_task.ctx = ctx;

    io_service_post_job(
        usc->iosvc, usc->fd, IO_SVC_OP_WRITE, !IOSVC_JOB_ONESHOT,
        (iosvc_job_function_t)data_may_be_sent,
        usc
    );
}

void unix_socket_client_recv(usc_t *usc,
                             size_t sz,
                             usc_reader_t reader, void *ctx) {
    assert(usc);

    memset(&(usc->read_task), 0, sizeof(usc->read_task));
    usc->read_task.b.offset = usc->read_task.b.user_size;
    buffer_realloc(&usc->read_task.b, usc->read_task.b.offset + sz);
    usc->read_task.reader = reader;
    usc->read_task.ctx = ctx;

    io_service_post_job(
        usc->iosvc, usc->fd, IO_SVC_OP_READ, !IOSVC_JOB_ONESHOT,
        (iosvc_job_function_t)data_may_be_read,
        usc
    );
}

bool unix_socket_client_reconnect(usc_t *usc) {
    assert(usc && usc->connected);

    unix_socket_client_disconnect_(usc, true);
    return unix_socket_client_connect_(usc,
                                       usc->connected_to_name,
                                       usc->connected_to_name_len,
                                       usc->connector, usc->connector_ctx,
                                       true);
}
