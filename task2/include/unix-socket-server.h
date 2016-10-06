#ifndef _UNIX_SOCKET_SERVER_H_
# define _UNIX_SOCKET_SERVER_H_

# include "io-service.h"
# include "avl-tree.h"

# include <stdbool.h>
# include <stddef.h>

struct unix_socket_server;
typedef struct unix_socket_server uss_t;

struct unix_socket_connection;
typedef struct unix_socket_server_connection uss_connection_t;

typedef bool (*uss_acceptor_t)(uss_t *srv, uss_connection_t *conn, void *ctx);
typedef void (*uss_reader_t)(uss_t *srv, uss_connection_t *conn,
                             int error, void *ctx);
typedef void (*uss_writer_t)(uss_t *srv, uss_connection_t *conn,
                             int error,
                             void *ctx);

struct unix_socket_server {
    io_service_t *iosvc;
    int fd;

    avl_tree_t connections;

    uss_acceptor_t acceptor;
    void *acceptor_ctx;
};

struct unix_socket_server_connection {
    uss_t *host;

    int fd;

    bool eof;

    struct {
        uss_reader_t reader;
        void *ctx;
        buffer_t b;
        size_t currently_read;
    } read_task;

    struct {
        uss_writer_t writer;
        void *ctx;
        buffer_t b;
        size_t currently_wrote;
    } write_task;

    void *priv;
};

bool unix_socket_server_init(uss_t *srv,
                             const char *name /* non-NULL only */,
                             size_t name_len,
                             io_service_t *iosvc);
void unix_socket_server_deinit(uss_t *srv);
bool unix_socket_server_listen(uss_t *srv,
                               uss_acceptor_t acceptor,
                               void *ctx);
void unix_socket_server_close_connection(uss_t *srv,
                                         uss_connection_t *conn);
void unix_socket_server_send(uss_t *srv, uss_connection_t *conn,
                             const void *d, size_t sz,
                             uss_writer_t writer,
                             void *ctx);
void unix_socket_server_recv(uss_t *srv, uss_connection_t *conn,
                             size_t sz,
                             uss_reader_t reader,
                             void *ctx);

#endif /* _UNIX_SOCKET_SERVER_H_ */