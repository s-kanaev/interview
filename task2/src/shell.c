#include "shell.h"
#include "hash-functions.h"
#include "containers.h"
#include "common.h"
#include "protocol.h"
#include "log.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include <sys/inotify.h>
#include <sys/ioctl.h>

typedef struct {
    const char *driver_name;
    size_t driver_name_len;

    unsigned int slot_number;
} driver_description_t;

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
bool parse_unix_socket_name(shell_t *sh, const char *name, size_t name_len,
                            driver_description_t *dd);

static
void connector(usc_t *usc, shell_t *sh);

static
void reader_signature(usc_t *usc, int error, shell_t *sh);

static
void reader_info(usc_t *usc, int error, shell_t *sh);
static
void reader_response(usc_t *usc, int error, shell_t *sh);

/********************** private **********************/
void reader_signature(usc_t *usc, int error, shell_t *sh) {
    const pr_signature_t *s = (const pr_signature_t *)usc->read_task.b.data;

    if (error) {
        /* TODO */
        return;
    }

    if (usc->eof) {
        /* TODO */
        return;
    }

    switch (s->s) {
        case PR_DRV_INFO:
            break;
        case PR_DRV_RESPONSE:
            break;
        case PR_DRV_COMMAND:
        default:
            LOG(LOG_LEVEL_WARN, "Invalid signature %#02x from %*s\n",
                (unsigned int)s->s,
                usc->connected_to_name_len,
                usc->connected_to_name);
            LOG_MSG(LOG_LEVEL_WARN, "Reconnecting\n");

            if (!unix_socket_client_reconnect(usc)) {
                /* TODO */
                return;
            }
            /* TODO */

            break;
    }
}


void connector(usc_t *usc, shell_t *sh) {
    unix_socket_client_recv(usc, sizeof(pr_signature_t),
                            (usc_reader_t)reader_signature, sh);
}

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
    driver_description_t dd;
    hash_t hash;
    avl_tree_node_t *atn;
    list_t *l;
    list_element_t *le;
    usc_t *usc;
    bool inserted = false;

    LOG(LOG_LEVEL_DEBUG, "Created: %*s\n",
        name_len, name);

    if (!check_unix_socket(sh, name, name_len)) {
        LOG(LOG_LEVEL_DEBUG, "It is not a readable UNIX socket: %*s\n",
            name_len, name);
        return;
    }

    if (!parse_unix_socket_name(sh, name, name_len, &dd)) {
        LOG(LOG_LEVEL_DEBUG, "It is not valid readable UNIX socket name: %*s\n",
            name_len, name);
        return;
    }

    hash = hash_pearson(name, name_len);

    atn = avl_tree_add_or_get(&sh->clients, hash, &inserted);

    l = (list_t *)atn->data;

    if (!inserted)
        list_init(l, true, sizeof(*usc));

    for (le = list_begin(l); le; le = list_next(l, le)) {
        usc = (usc_t *)le->data;

        if (usc->connected_to_name_len != name_len)
            continue;

        if (0 == strncmp(usc->connected_to_name, name, name_len))
            break;
    }

    if (le) {
        LOG(LOG_LEVEL_FATAL, "Duplicate driver: %*s\n",
            name_len, name);
        abort();
    }

    le = list_append(l);
    usc = (usc_t *)le->data;

    if (!unix_socket_client_init(usc, NULL, 0, sh->iosvc)) {
        LOG(LOG_LEVEL_FATAL,
            "Can't initialize UNIX socket client for %*s: %s\n",
            name_len, name, strerror(errno));
        abort();
    }

    if (!unix_socket_client_connect(
        usc, name, name_len,
        (usc_connector_t)connector, sh)) {
        LOG(LOG_LEVEL_FATAL, "Can't connect to %*s: %s\n",
            name_len, name, strerror(errno));
        abort();
    }
}

void base_dir_smth_deleted(shell_t *sh, const char *name, size_t name_len) {
    driver_description_t dd;
    hash_t hash;
    avl_tree_node_t *atn;
    list_t *l;
    list_element_t *le;
    usc_t *usc;

    LOG(LOG_LEVEL_DEBUG, "Deleted: %*s\n",
        name_len, name);

    if (!parse_unix_socket_name(sh, name, name_len, &dd)) {
        LOG(LOG_LEVEL_DEBUG, "It is not valid readable UNIX socket name: %*s\n",
            name_len, name);
        return;
    }

    hash = hash_pearson(name, name_len);

    atn = avl_tree_get(&sh->clients, hash);

    if (!atn) {
        LOG(LOG_LEVEL_WARN, "UNIX socket name was not registered (1): %*s\n",
            name_len, name);
        return;
    }

    l = (list_t *)atn->data;

    for (le = list_begin(l); le; le = list_next(l, le)) {
        usc = (usc_t *)le->data;

        if (usc->connected_to_name_len != name_len)
            continue;

        if (name_len && (0 == strncmp(usc->connected_to_name, name, name_len)))
            break;
    }

    if (!le) {
        LOG(LOG_LEVEL_WARN, "UNIX socket name was not registered (2): %*s\n",
            name_len, name);
        return;
    }

    unix_socket_client_deinit(usc);

    list_remove_and_advance(l, le);
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
