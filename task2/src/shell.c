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

#define streq(a, b)         (0 == strcmp((a), (b)))
#define DELIM               " "
#define LIST_CMD            "list"
#define HELP_CMD            "help"
#define CMD_CMD             "cmd"

#define PROMPT              ">"
#define PROMPT_LEN          (sizeof(PROMPT) - 1)

#define HELP_MSG                                    \
"Commands:\n"                                       \
"list --- list all drivers\n"                       \
"help --- print this message\n"                     \
"cmd drv slot drv_cmd ... --- send command "        \
"drv_cmd to driver drv at slot with arguments\n"
#define HELP_MSG_LEN        (sizeof(HELP_MSG) - 1)

#define INVALID_MSG         "Invalid command\n"
#define INVALID_MSG_LEN     (sizeof(INVALID_MSG) - 1)

#define NEW_LINE            "\n"
#define NEW_LINE_LEN        (sizeof(NEW_LINE) - 1)

#define DRIVER_PRE          "Driver: "
#define DRIVER_PRE_LEN      (sizeof(DRIVER_PRE) - 1)

#define SLOT_PRE            "Slot: "
#define SLOT_PRE_LEN        (sizeof(SLOT_PRE) - 1)

#define DRIVER_POST         "Commands:\n"
#define DRIVER_POST_LEN     (sizeof(DRIVER_POST) - 1)

#define SPACE               " "
#define SPACE_LEN           (sizeof(SPACE) - 1)

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

static
void on_input(int fd, io_svc_op_t op, shell_t *sh);

static inline
bool detect_newline_on_input(shell_t *sh);

static
void run_command_from_input(shell_t *sh);

static inline
void shift_input(shell_t *sh);

static
void cmd_list(shell_t *sh);

static
void cmd_help(shell_t *sh);

static
void cmd_invalid(shell_t *sh);

static
void cmd_cmd(shell_t *sh,
             const char *drv,
             unsigned int slot,
             const char *cmd,
             vector_t *args);

static inline
void finish_cmd(shell_t *sh);

static
void print_drv(shell_t *sh, avl_tree_node_t *atn);

/********************** private **********************/
void print_drv(shell_t *sh, avl_tree_node_t *atn) {
    shell_driver_t *sd;
    size_t idx;

    if (!atn)
        return;

    print_drv(sh, atn->left);
    print_drv(sh, atn->right);

    sd = (shell_driver_t *)atn->data;

    fprintf(
        sh->output,
        DRIVER_PRE "%*s" NEW_LINE
        SLOT_PRE "%u" NEW_LINE
        DRIVER_POST,
        sd->name_len, sd->name,
        sd->slot
    );
    for (idx = 0; idx < vector_count(&sd->commands); ++idx) {
        const pr_driver_command_info_t *dci =
            (const pr_driver_command_info_t *)vector_get(&sd->commands, idx);

        fprintf(
            sh->output,
            "%*s <arity: %u> --- %*s" NEW_LINE,
            MAX_COMMAND_NAME_LEN, dci->name,
            dci->arity,
            MAX_COMMAND_DESCRIPTION_LEN, dci->descr
        );
    }
}

void cmd_list(shell_t *sh) {
    print_drv(sh, sh->clients.root);
    finish_cmd(sh);
}

void cmd_help(shell_t *sh) {
    fprintf(sh->output, HELP_MSG);
    finish_cmd(sh);
}

void cmd_invalid(shell_t *sh) {
    fprintf(sh->output, INVALID_MSG);
    finish_cmd(sh);
}

void cmd_cmd(shell_t *sh,
             const char *drv, unsigned int slot, const char *cmd,
             vector_t *args) {
    /* TODO driver command */
}

void finish_cmd(shell_t *sh) {
    fprintf(sh->output, PROMPT);
}

bool detect_newline_on_input(shell_t *sh) {
    if (sh->input_buffer.user_size == 0)
        return false;

    while (sh->input_buffer.offset < sh->input_buffer.user_size) {
        if ('\n' == ((char *)sh->input_buffer.data)[sh->input_buffer.offset])
            return true;

        ++sh->input_buffer.offset;
    }

    return false;
}

void run_command_from_input(shell_t *sh) {
    char *line = (char *)sh->input_buffer.data;
    char *token;
    char *cmd, *drv, *slot, *drv_cmd;
    char *slot_endptr = NULL;
    unsigned int slot_number;
    vector_t args;

    line[sh->input_buffer.offset] = '\0';

    cmd = strtok(line, DELIM);

    if (!cmd) {
        LOG_MSG(LOG_LEVEL_WARN, "Invalid input: no command\n");
        cmd_invalid(sh);
        return;
    }

    if (streq(LIST_CMD, cmd)) {
        cmd_list(sh);
        return;
    }
    else if (streq(HELP_CMD, cmd)) {
        cmd_help(sh);
        return;
    }
    else if (!streq(CMD_CMD, cmd)) {
        cmd_invalid(sh);
        return;
    }

    drv = strtok(NULL, DELIM);
    slot = strtok(NULL, DELIM);
    drv_cmd = strtok(NULL, DELIM);

    if (!drv || !slot || !drv_cmd) {
        LOG(LOG_LEVEL_WARN, "Invalid input: %s %s %s\n",
            drv ? drv : "n / a",
            slot ? slot : "n / a",
            drv_cmd ? drv_cmd : "n / a");
        cmd_invalid(sh);
        return;
    }

    slot_number = strtoul(slot, &slot_endptr, 10);
    if (*slot_endptr != '\0') {
        LOG_MSG(LOG_LEVEL_WARN, "Slot is not number\n");
        cmd_invalid(sh);
        return;
    }

    vector_init(&args, sizeof(token), 0);
    token = strtok(NULL, DELIM);

    while (token) {
        *(char **)vector_append(&args) = token;
        token = strtok(NULL, DELIM);
    }

    cmd_cmd(sh, drv, slot_number, drv_cmd, &args);

    vector_deinit(&args);

    return;
}

void shift_input(shell_t *sh) {
    uint8_t *start = (uint8_t *)sh->input_buffer.data;

    memmove(start, start + sh->input_buffer.offset,
            sh->input_buffer.user_size - sh->input_buffer.offset);

    buffer_realloc(&sh->input_buffer,
                   sh->input_buffer.user_size - sh->input_buffer.offset);
}

void on_input(int fd, io_svc_op_t op, shell_t *sh) {
    int pending;
    int rc;

    rc = ioctl(fd, FIONREAD, &pending);

    if (rc < 0) {
        LOG(LOG_LEVEL_FATAL, "Can't ioctl(FIONREAD) on input FD: %s\n",
            strerror(errno));
        abort();
    }

    buffer_realloc(&sh->input_buffer, sh->input_buffer.user_size + pending);

    while (detect_newline_on_input(sh)) {
        run_command_from_input(sh);
        shift_input(sh);
    }
}

#define READ_ERROR_HANDLER                                                      \
do {                                                                            \
    if (error) {                                                                \
        LOG(LOG_LEVEL_WARN, "Error on receive: %s\n",                           \
            strerror(errno));                                                   \
        LOG_MSG(LOG_LEVEL_WARN, "Reconnecting\n");                              \
                                                                                \
        if (!unix_socket_client_reconnect(usc))                                 \
            LOG(LOG_LEVEL_FATAL, "Can't reconnect to %*s: %s\n",                \
                usc->connected_to_name_len, usc->connected_to_name,             \
                strerror(errno));                                               \
                                                                                \
        return;                                                                 \
    }                                                                           \
                                                                                \
    if (usc->eof) {                                                             \
        LOG(LOG_LEVEL_WARN, "EOF from %*s.\n",                                  \
            usc->connected_to_name_len, usc->connected_to_name);                \
        LOG_MSG(LOG_LEVEL_WARN, "Possibly a delete will take place soon\n");    \
        return;                                                                 \
    }                                                                           \
} while(0)

void reader_info(usc_t *usc, int error, shell_t *sh) {
    size_t required_length;
    shell_driver_t *sd;
    const pr_driver_command_info_t *dci;
    const pr_driver_info_t *di = (const pr_driver_info_t *)usc->read_task.b.data;

    READ_ERROR_HANDLER;

    required_length = sizeof(*di) + sizeof(*dci) * di->commands_number;

    if (usc->read_task.b.user_size < required_length) {
        usc->read_task.b.offset = usc->read_task.b.user_size;
        unix_socket_client_recv(
            usc,
            required_length - usc->read_task.b.user_size,
            (usc_reader_t)reader_info,
            sh);

        return;
    }

    sd = (shell_driver_t *)usc->priv;
    vector_init(&sd->commands, sizeof(*dci), di->commands_number);
    memcpy(sd->commands.data.data, di + 1, sizeof(*dci) * di->commands_number);
}

void reader_response(usc_t *usc, int error, shell_t *sh) {
    size_t required_length;
    const pr_driver_response_t *dr = (const pr_driver_response_t *)usc->read_task.b.data;

    READ_ERROR_HANDLER;

    required_length = sizeof(*dr) + dr->len;

    if (usc->read_task.b.user_size < required_length) {
        usc->read_task.b.offset = usc->read_task.b.user_size;
        unix_socket_client_recv(
            usc,
            required_length - usc->read_task.b.user_size,
            (usc_reader_t)reader_response,
            sh);

        return;
    }

    fprintf(sh->output, "%*s" NEW_LINE, dr->len, (const char *)(dr + 1));
    finish_cmd(sh);
}

void reader_signature(usc_t *usc, int error, shell_t *sh) {
    const pr_signature_t *s = (const pr_signature_t *)usc->read_task.b.data;

    READ_ERROR_HANDLER;

    switch (s->s) {
        case PR_DRV_INFO:
            unix_socket_client_recv(usc,
                                    sizeof(pr_driver_info_t) - sizeof(*s),
                                    (usc_reader_t)reader_info, sh);
            break;
        case PR_DRV_RESPONSE:
            unix_socket_client_recv(usc,
                                    sizeof(pr_driver_response_t) - sizeof(*s),
                                    (usc_reader_t)reader_response, sh);
            break;
        case PR_DRV_COMMAND:
        default:
            LOG(LOG_LEVEL_WARN, "Invalid signature %#02x from %*s\n",
                (unsigned int)s->s,
                usc->connected_to_name_len,
                usc->connected_to_name);
            LOG_MSG(LOG_LEVEL_WARN, "Reconnecting\n");

            if (!unix_socket_client_reconnect(usc))
                LOG(LOG_LEVEL_FATAL, "Can't reconnect to %*s: %s\n",
                    usc->connected_to_name_len, usc->connected_to_name,
                    strerror(errno));

            break;
    }
}
#undef READ_ERROR_HANDLER

void connector(usc_t *usc, shell_t *sh) {
    buffer_realloc(&usc->read_task.b, 0);
    buffer_realloc(&usc->write_task.b, 0);

    unix_socket_client_recv(usc, sizeof(pr_signature_t),
                            (usc_reader_t)reader_signature, sh);
}

static
void purge_clients_list(avl_tree_node_t *atn) {
    list_t *l;
    list_element_t *le;
    shell_driver_t *sd;

    if (!atn)
        return;

    purge_clients_list(atn->left);
    purge_clients_list(atn->right);

    l = (list_t *)atn->data;

    for (le = list_begin(l); le; list_next(l, le)) {
        sd = (shell_driver_t *)le->data;

        unix_socket_client_deinit(&sd->usc);
        free(sd->name);
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
    shell_driver_t *sd;
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
        list_init(l, true, sizeof(*sd));

    for (le = list_begin(l); le; le = list_next(l, le)) {
        sd = (shell_driver_t *)le->data;

        if (sd->name_len != dd.driver_name_len)
            continue;

        if ((0 == strncmp(sd->name, dd.driver_name, name_len)) &&
            sd->slot == dd.slot_number)
            break;
    }

    if (le) {
        LOG(LOG_LEVEL_FATAL, "Duplicate driver: %*s\n",
            name_len, name);
        abort();
    }

    le = list_append(l);
    sd = (shell_driver_t *)le->data;

    if (!unix_socket_client_init(&sd->usc, NULL, 0, sh->iosvc)) {
        LOG(LOG_LEVEL_FATAL,
            "Can't initialize UNIX socket client for %*s: %s\n",
            name_len, name, strerror(errno));
        abort();
    }

    sd->usc.priv = sd;

    sd->name = (char *)malloc(dd.driver_name_len);
    memcpy(sd->name, dd.driver_name, dd.driver_name_len);
    sd->name_len = dd.driver_name_len;
    sd->slot = dd.slot_number;

    if (!unix_socket_client_connect(
        &sd->usc, name, name_len,
        (usc_connector_t)connector, sh))
        LOG(LOG_LEVEL_FATAL, "Can't connect to %*s: %s\n",
            name_len, name, strerror(errno));
}

void base_dir_smth_deleted(shell_t *sh, const char *name, size_t name_len) {
    driver_description_t dd;
    hash_t hash;
    avl_tree_node_t *atn;
    list_t *l;
    list_element_t *le;
    shell_driver_t *sd;

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
        sd = (shell_driver_t *)le->data;

        if (sd->name_len != dd.driver_name_len)
            continue;

        if ((0 == strncmp(sd->name, dd.driver_name, name_len)) &&
            sd->slot == dd.slot_number)
            break;
    }

    if (!le) {
        LOG(LOG_LEVEL_WARN, "UNIX socket name was not registered (2): %*s\n",
            name_len, name);
        return;
    }

    unix_socket_client_deinit(&sd->usc);
    free(sd->name);

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
                io_service_t *iosvc, int input_fd, FILE *output) {
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

    buffer_init(&sh->input_buffer, 0, bp_non_shrinkable);

    sh->input_fd = input_fd;
    sh->output = output;

    return true;
}

void shell_deinit(shell_t *sh) {
    assert(sh);

    if (!(sh->base_path_watch_descriptor < 0))
        inotify_rm_watch(sh->inotify_fd, sh->base_path_watch_descriptor);

    close(sh->inotify_fd);

    purge_clients_list(sh->clients.root);

    free(sh->base_path);

    buffer_deinit(&sh->input_buffer);
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

    io_service_post_job(sh->iosvc, sh->input_fd,
                        IO_SVC_OP_READ, !IOSVC_JOB_ONESHOT,
                        (iosvc_job_function_t)on_input,
                        sh);

    io_service_run(sh->iosvc);
}
