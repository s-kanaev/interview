#ifndef _SHELL_H_
# define _SHELL_H_

# include "io-service.h"
# include "avl-tree.h"
# include "unix-socket-client.h"
# include "protocol.h"

# include <stdio.h>
# include <stddef.h>
# include <stdbool.h>

struct shell;
typedef struct shell shell_t;

struct shell_driver;
typedef struct shell_driver shell_driver_t;

struct shell {
    io_service_t *iosvc;
    int inotify_fd;
    int base_path_watch_descriptor;

    bool running;

    int input_fd;
    FILE *output;

    buffer_t input_buffer;
    /*
     * Maps UNIX socket basename hash to list of clients.
     * Tree node inplaces list_t.
     * List inplaces shell_driver_t.
     */
    avl_tree_t clients;

    char *base_path;
    size_t base_path_len;
};

struct shell_driver {
    char *name;
    size_t name_len;

    unsigned int slot;

    /* vector of internal data structures */
    vector_t commands;

    usc_t usc;
};

bool shell_init(shell_t *sh, const char *base_path, size_t base_path_len,
                io_service_t *iosvc, int input_fd, FILE *output);
void shell_deinit(shell_t *sh);
void shell_run(shell_t *sh);

#endif /* _SHELL_H_ */