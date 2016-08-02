#include "shell.h"
#include "hash-functions.h"
#include "containers.h"
#include "common.h"
#include "log.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include <sys/inotify.h>
#include <sys/ioctl.h>

static
void purge_clients_list(avl_tree_node_t *atn);

static
void base_dir_event(int fd, io_svc_op_t op, shell_t *sh);

static inline
ssize_t base_dir_single_event(shell_t *sh, void *base, size_t offset);

static
void base_dir_smth_created(shell_t *sh, const char *name, size_t name_len);

static
void base_dir_smth_deleted(shell_t *sh, const char *name, size_t name_len);

static
void base_dir_self_deleted(shell_t *sh);

static inline
bool check_unix_socket(shell_t *sh, const char *name, size_t name_len);

static inline
bool parse_unix_socket_name(shell_t *sh, const char *name, size_t name_len);

/********************** private **********************/
static
void purge_clients_list(avl_tree_node_t *atn) {
    list_t *l;
    list_element_t *le;
    usc_t *usc;

    if (!atn)
        return;

    purge_clients_list(atn->left);
    purge_clients_list(atn->right);

    l = (list_t *)atn->data;

    for (le = list_begin(l); le; list_next(l, le)) {
        usc = (usc_t *)le->data;

        unix_socket_client_deinit(usc);
    }

    list_purge(l);
}

void base_dir_self_deleted(shell_t *sh) {
    LOG_MSG(LOG_LEVEL_WARN, "Base directory deleted.\n");
    LOG_MSG(LOG_LEVEL_WARN, "  Stopping. Won't wait for pending events.\n");

    sh->running = false;
    io_service_stop(sh->iosvc, false);
}

void base_dir_smth_created(shell_t *sh, const char *name, size_t name_len) {
    /* TODO */
}

ssize_t base_dir_single_event(shell_t *sh, void *base, size_t _offset) {
    ssize_t offset = 0;
    struct inotify_event *event = base + _offset;

    offset = sizeof(*event);

    if (event->len)
        offset += event->len;

    if (event->mask & IN_CREATE)
        base_dir_smth_created(sh, event->name, event->len);

    if (event->mask & IN_DELETE)
        base_dir_smth_deleted(sh, event->name, event->len);

    if (event->mask & IN_DELETE_SELF)
        base_dir_self_deleted(sh);

    return offset;
}

void base_dir_event(int fd, io_svc_op_t op, shell_t *sh) {
    int pending;
    int rc;
    buffer_t b;
    ssize_t data_read;

    LOG_MSG(LOG_LEVEL_DEBUG, "Events for base dir\n");

    rc = ioctl(fd, FIONREAD, &pending);

    if (rc < 0) {
        LOG(LOG_LEVEL_FATAL, "Can't ioctl (FIONREAD) inotify instance: %s\n",
            strerror(errno));
        abort();
    }

    buffer_init(&b, pending, bp_non_shrinkable);

    b.offset = 0;

    while (b.offset < b.user_size) {
        data_read = read(fd, b.data + b.offset,
                         b.user_size - b.offset);

        if (data_read < 0) {
            if (errno == EINTR)
                continue;
            else
                break;
        }

        b.offset += data_read;
    }

    if (b.offset < b.user_size) {
        LOG(LOG_LEVEL_FATAL, "Couldn't read inotify events descriptions: %s\n",
            strerror(errno));
        abort();
    }

    b.offset = 0;

    while ((b.offset < b.user_size) && sh->running) {
        data_read = base_dir_single_event(sh, b.data, b.offset);

        if (data_read < 0) {
            LOG_MSG(LOG_LEVEL_FATAL,
                    "Couldn't read and react for single inotify event\n");
            abort();
        }

        b.offset += data_read;
    }

    buffer_deinit(&b);
}

/********************** API **********************/
bool shell_init(shell_t *sh,
                const char *base_path, size_t base_path_len,
                io_service_t *iosvc) {
    bool ret;

    assert(sh && iosvc && (!base_path_len || (base_path && base_path_len)));

    memset(sh, 0, sizeof(*sh));

    sh->iosvc = iosvc;

    sh->inotify_fd = inotify_init();

    if (sh->inotify_fd < 0)
        return false;

    sh->base_path_watch_descriptor = -1;

    avl_tree_init(&sh->clients, true, sizeof(list_t));

    if (0 == base_path_len) {
        base_path = DOT;
        base_path_len = DOT_LEN;
    }

    sh->base_path = (char *)malloc(base_path_len + 1);
    memcpy(sh->base_path, base_path, base_path_len);
    sh->base_path_len = base_path_len -
                            ('/' == sh->base_path[base_path_len - 1]);
    sh->base_path[sh->base_path_len] = '\0';

    sh->running = false;

    return true;
}

void shell_deinit(shell_t *sh) {
    assert(sh);

    if (!(sh->base_path_watch_descriptor < 0))
        inotify_rm_watch(sh->inotify_fd, sh->base_path_watch_descriptor);

    close(sh->inotify_fd);

    purge_clients_list(sh->clients.root);

    free(sh->base_path);
}

void shell_run(shell_t *sh) {
    int wd;

    assert(sh);

    wd = inotify_add_watch(sh->inotify_fd, sh->base_path,
                           IN_CREATE | IN_DELETE | IN_DELETE_SELF |
                            IN_EXCL_UNLINK | IN_ONLYDIR);

    if (wd < 0) {
        LOG(LOG_LEVEL_FATAL, "Couldn't allocate watch descriptor for %s: %s\n",
            sh->base_path, strerror(errno));

        return;
    }

    sh->base_path_watch_descriptor = wd;

    sh->running = true;

    io_service_post_job(sh->iosvc, sh->inotify_fd,
                        IO_SVC_OP_READ, !IOSVC_JOB_ONESHOT,
                        (iosvc_job_function_t)base_dir_event,
                        sh);

    io_service_run(sh->iosvc);
}
