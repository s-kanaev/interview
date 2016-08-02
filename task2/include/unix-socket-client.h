#ifndef _UNIX_SOCKET_CLIENT_H_
# define _UNIX_SOCKET_CLIENT_H_

# include "io-service.h"

# include <stdbool.h>
# include <stddef.h>

struct unix_socket_client;
typedef struct unix_socket_client usc_t;

typedef void (*usc_connector_t)(usc_t *usc, void *ctx);
typedef void (*usc_reader_t)(usc_t *usc,
                             int error,
                             void *ctx);
typedef void (*usc_writer_t)(usc_t *usc,
                             int error,
                             void *ctx);

struct unix_socket_client {
    io_service_t *iosvc;

    int fd;
    bool connected;

    bool eof;

    usc_connector_t connector;
    void *connector_ctx;

    char *name;
    size_t name_len;

    struct {
        usc_reader_t reader;
        void *ctx;
        buffer_t b;
        size_t currently_read;
    } read_task;

    struct {
        usc_writer_t writer;
        void *ctx;
        buffer_t b;
        size_t currently_wrote;
    } write_task;
};

bool unix_socket_client_init(usc_t *usc,
                             const char *name /* NULL for unnamed */,
                             size_t name_len,
                             io_service_t *iosvc);
void unix_socket_client_deinit(usc_t *usc);
bool unix_socket_client_connect(usc_t *usc,
                                const char *name,
                                size_t name_len,
                                usc_connector_t connector,
                                void *ctx);
void unix_socket_client_disconnect(usc_t *usc);
void unix_socket_client_send(usc_t *usc,
                             const void *d, size_t sz,
                             usc_writer_t writer,
                             void *ctx);
void unix_socket_client_recv(usc_t *usc,
                             size_t sz,
                             usc_reader_t reader,
                             void *ctx);

#endif /* _UNIX_SOCKET_CLIENT_H_ */