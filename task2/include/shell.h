#ifndef _SHELL_H_
# define _SHELL_H_

# include "io-service.h"
# include "avl-tree.h"
# include "unix-socket-client.h"

# include <stddef.h>
# include <stdbool.h>

struct shell;
typedef struct shell shell_t;

struct shell {
    io_service_t *iosvc;
    int inotify_fd;
    int base_path_watch_descriptor;

    bool running;

    /*
     * Maps UNIX socket basename hash to list of clients.
     * Tree node inplaces list_t.
     * List inplaces usc_t.
     */
    avl_tree_t clients;

    char *base_path;
    size_t base_path_len;
};

bool shell_init(shell_t *sh, const char *base_path, size_t base_path_len,
                io_service_t *iosvc);
void shell_deinit(shell_t *sh);
void shell_run(shell_t *sh);

#endif /* _SHELL_H_ */