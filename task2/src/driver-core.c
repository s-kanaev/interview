#include "io-service.h"
#include "driver-core.h"
#include "protocol.h"
#include "containers.h"
#include "common.h"
#include "log.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>

#include <linux/un.h>

/************ declaratins ************/
static
void reader(uss_t *srv, uss_connection_t *conn,
            int error,
            driver_core_t *core);
static
void writer(uss_t *srv, uss_connection_t *conn,
            int error,
            driver_core_t *core);
static
bool acceptor(uss_t *srv, uss_connection_t *conn, driver_core_t *core);
static inline
bool driver_core_init_(io_service_t *iosvc,
                       driver_core_t *core,
                       driver_payload_t *payload);

/************ definitions ************/
void reader(uss_t *srv, uss_connection_t *conn,
            int error, driver_core_t *core) {
    const pr_signature_t *s;
    pr_driver_response_t *response_header;
    const pr_driver_command_t *dc;
    const pr_driver_command_argument_t *dca;
    const uint8_t *dca_value;
    uint8_t argc, arg_idx;
    uint8_t cmd_idx;
    size_t required_length;
    driver_command_t *comm;
    buffer_t response;
    driver_core_connection_state_t *state;

    if (error) {
        LOG(LOG_LEVEL_WARN, "Couldn't recv from client: %s\n",
            strerror(errno));
        avl_tree_remove(&core->connection_state, conn->fd);
        unix_socket_server_close_connection(srv, conn);
        return;
    }

    if (conn->eof) {
        LOG_MSG(LOG_LEVEL_DEBUG, "Another one EOF\n");
        avl_tree_remove(&core->connection_state, conn->fd);
        unix_socket_server_close_connection(srv, conn);
        return;
    }

    s = (const pr_signature_t *)(conn->read_task.b.data);

    if (PR_DRV_COMMAND != s->s) {
        LOG(LOG_LEVEL_WARN, "Invalid signature received: %#02x\n",
            s->s);
        avl_tree_remove(&core->connection_state, conn->fd);
        unix_socket_server_close_connection(srv, conn);
        return;
    }

    if (conn->read_task.b.user_size < sizeof(pr_driver_command_t)) {
        conn->read_task.b.offset = conn->read_task.b.user_size;
        unix_socket_server_recv(
            srv, conn,
            sizeof(pr_driver_command_t) - conn->read_task.b.user_size,
            (uss_reader_t)reader,
            core);

        return;
    }

    state = (driver_core_connection_state_t *)conn->priv;
    dc = (const pr_driver_command_t *)s;
    argc = dc->argc;
    cmd_idx = dc->cmd_idx;
    dca = (const pr_driver_command_argument_t *)(dc + 1);

    state->argc = argc;

    comm = (driver_command_t *)vector_get(&core->payload->commands, cmd_idx);
    if (argc > comm->max_arity) {
        LOG(LOG_LEVEL_WARN, "Maximum command arity exceeded: %#02x vs %#02x\n",
            (unsigned int)argc, (unsigned int)comm->max_arity);
        avl_tree_remove(&core->connection_state, conn->fd);
        unix_socket_server_close_connection(srv, conn);
        return;
    }

    if (!argc) {
        LOG(LOG_LEVEL_DEBUG, "Calling %*s with no arguments\n",
            comm->name_len, comm->name);
        buffer_init(&response, sizeof(*response_header), bp_non_shrinkable);
        comm->handler(cmd_idx,
                      comm->name, comm->name_len,
                      argc, NULL, &response);

        if (response.user_size < sizeof(*response_header)) {
            LOG_MSG(LOG_LEVEL_FATAL,
                    "Comand handler malforms response buffer\n");
            abort();
        }

        response_header = (pr_driver_response_t *)response.data;
        response_header->s.s = PR_DRV_RESPONSE;
        response_header->len = response.user_size - sizeof(*response_header);

        unix_socket_server_send(srv, conn, response.data, response.user_size,
                                (uss_writer_t)writer, core);
        buffer_deinit(&response);
        return;
    }   /* if (!argc) */

    if (state->argc == state->args_received) {
        LOG(LOG_LEVEL_DEBUG, "Calling %*s with %d arguments\n",
            comm->name_len, comm->name, state->argc);
        driver_command_argument_t argv[state->argc];

        for (arg_idx = 0; arg_idx < state->argc; ++arg_idx) {
            argv[arg_idx].arg = (const char *)(dca + 1);
            argv[arg_idx].arg_len = dca->len;

            dca = (const pr_driver_command_argument_t *)(
                (const uint8_t *)(dca + 1) + dca->len);
        }

        buffer_init(&response, 0, bp_non_shrinkable);
        comm->handler(cmd_idx,
                      comm->name, comm->name_len,
                      state->argc, argv, &response);
        unix_socket_server_send(srv, conn, response.data, response.user_size,
                                (uss_writer_t)writer, core);
        buffer_deinit(&response);

        return;
    }   /* if (state->argc == state->args_received) */

    /* TODO receive full arguments vector */
    if (0 == state->last_argument_offset)
        state->last_argument_offset = sizeof(*dc);

    dca = (const pr_driver_command_argument_t *)(
        (const uint8_t *)s + state->last_argument_offset
    );

    if (conn->read_task.b.user_size < state->last_argument_offset
                                            + sizeof(*dca)) {
        required_length = state->last_argument_offset + sizeof(*dca);
        conn->read_task.b.offset = conn->read_task.b.user_size;
        unix_socket_server_recv(
            srv, conn,
            required_length - conn->read_task.b.user_size,
            (uss_reader_t)reader,
            core);

        return;
    }

    if (conn->read_task.b.user_size < state->last_argument_offset
                                            + sizeof(*dca) + dca->len) {
        required_length = state->last_argument_offset + sizeof(*dca)
                            + dca->len;
        conn->read_task.b.offset = conn->read_task.b.user_size;
        unix_socket_server_recv(
            srv, conn,
            required_length - conn->read_task.b.user_size,
            (uss_reader_t)reader,
            core);

        return;
    }

    ++state->args_received;
    state->last_argument_offset += sizeof(*dca) + dca->len;
    required_length = state->last_argument_offset + sizeof(*dca);

    if (state->args_received < state->argc) {
        conn->read_task.b.offset = conn->read_task.b.user_size;
        unix_socket_server_recv(
            srv, conn,
            required_length - conn->read_task.b.user_size,
            (uss_reader_t)reader,
            core);

        return;
    }

    LOG(LOG_LEVEL_DEBUG, "Calling %*s with %d arguments\n",
        comm->name_len, comm->name, state->argc);
    driver_command_argument_t argv[state->argc];

    for (arg_idx = 0; arg_idx < state->argc; ++arg_idx) {
        argv[arg_idx].arg = (const char *)(dca + 1);
        argv[arg_idx].arg_len = dca->len;

        dca = (const pr_driver_command_argument_t *)(
            (const uint8_t *)(dca + 1) + dca->len);
    }

    buffer_init(&response, sizeof(*response_header), bp_non_shrinkable);
    comm->handler(cmd_idx,
                  comm->name, comm->name_len,
                  state->argc, argv, &response);

    if (response.user_size < sizeof(*response_header)) {
        LOG_MSG(LOG_LEVEL_FATAL,
                "Comand handler malforms response buffer\n");
        abort();
    }

    response_header = (pr_driver_response_t *)response.data;
    response_header->s.s = PR_DRV_RESPONSE;
    response_header->len = response.user_size - sizeof(*response_header);

    unix_socket_server_send(srv, conn, response.data, response.user_size,
                            (uss_writer_t)writer, core);
    buffer_deinit(&response);
}

void writer(uss_t *srv, uss_connection_t *conn,
            int error, driver_core_t *core) {
    driver_core_connection_state_t *s = conn->priv;

    if (error) {
        LOG(LOG_LEVEL_WARN, "Couldn't send buffer to client: %s\n",
            strerror(errno));
        avl_tree_remove(&core->connection_state, conn->fd);
        unix_socket_server_close_connection(srv, conn);
        return;
    }

    s->argc = s->args_received = 0;
    s->last_argument_offset = 0;

    buffer_realloc(&conn->read_task.b, 0);

    unix_socket_server_recv(srv, conn, sizeof(pr_signature_t),
                            (uss_reader_t)reader,
                            core);
}

bool acceptor(uss_t *srv, uss_connection_t *conn, driver_core_t *core) {
    avl_tree_node_t *atn = avl_tree_add(&core->connection_state, conn->fd);
    driver_core_connection_state_t *s = (driver_core_connection_state_t *)atn->data;

    s->argc = s->args_received = 0;
    s->last_argument_offset = 0;
    s->ussc = conn;

    conn->priv = s;

    unix_socket_server_send(srv, conn, core->greeting, core->greeting_length,
                            (uss_writer_t)writer, core);
    return true;
}

bool driver_core_init_(io_service_t *iosvc,
                       driver_core_t *core,
                       driver_payload_t *payload) {
    size_t path_len = BASE_DIR_LEN + SLASH_LEN
                      + payload->name_len + DOT_LEN
                      + MAX_DIGITS + DOT_LEN
                      + SUFFIX_LEN + 1;
    char path[path_len];
    size_t offset = 0;
    uint8_t idx;
    uint8_t cmd_number;
    pr_driver_info_t *drv_info;
    pr_driver_command_info_t *cmd_info;
    driver_command_t *cmd;

    memset(core, 0, sizeof(core));

    if (path_len > UNIX_PATH_MAX)
        return false;

    memcpy(path + offset, BASE_DIR, BASE_DIR_LEN);
    offset += BASE_DIR_LEN;
    memcpy(path + offset, SLASH, SLASH_LEN);
    offset += SLASH_LEN;
    memcpy(path + offset, payload->name, payload->name_len);
    offset += payload->name_len;
    memcpy(path + offset, DOT, DOT_LEN);
    offset += DOT_LEN;
    offset += snprintf(path + offset, MAX_DIGITS + 1,
                       "%u", payload->slot_number);
    memcpy(path + offset, DOT, DOT_LEN);
    offset += DOT_LEN;
    memcpy(path + offset, SUFFIX, SUFFIX_LEN);
    offset += SUFFIX_LEN;
    *(path + offset) = '\0';

    core->path = strdup(path);
    core->iosvc = iosvc;
    core->payload = payload;

    /* allocate memroy for greeting (driver info) */
    cmd_number = vector_count(&(core->payload->commands)) > 255
                  ? 255
                  : vector_count(&(core->payload->commands));

    core->greeting_length = sizeof(pr_driver_info_t)
                            + cmd_number
                              * sizeof(pr_driver_command_info_t);

    core->greeting = malloc(core->greeting_length);

    if (!core->greeting)
        return false;

    drv_info = (pr_driver_info_t *)core->greeting;
    drv_info->s.s = PR_DRV_INFO;
    drv_info->commands_number = vector_count(&(core->payload->commands));

    cmd_info = (pr_driver_command_info_t *)(drv_info + 1);

    for (idx = 0; idx < cmd_number; ++idx, ++cmd_info) {
        cmd = vector_get(&(core->payload->commands), idx);

        memset(cmd_info, 0, sizeof(*cmd_info));
        memcpy(cmd_info->name, cmd->name, cmd->name_len < MAX_COMMAND_NAME_LEN ?
                                          cmd->name_len : MAX_COMMAND_NAME_LEN);
        memcpy(cmd_info->descr, cmd->description,
               cmd->description_len < MAX_COMMAND_DESCRIPTION_LEN ?
               cmd->description_len : MAX_COMMAND_DESCRIPTION_LEN);
        cmd_info->arity = cmd->max_arity;
    }

    avl_tree_init(&core->connection_state,
                  true, sizeof(driver_core_connection_state_t));

    return unix_socket_server_init(&core->uss, path, offset, iosvc);
}

bool driver_core_init(io_service_t *iosvc,
                      driver_core_t *core,
                      driver_payload_t *payload) {
    assert(iosvc && core && payload);

    return driver_core_init_(iosvc, core, payload);
}

void driver_core_deinit(driver_core_t *core) {
    assert(core);
    unix_socket_server_deinit(&core->uss);

    vector_deinit(&core->payload->commands);

    avl_tree_purge(&core->connection_state);


    if (core->greeting)
        free(core->greeting);

    if (core->path) {
        unlink(core->path);
        free(core->path);
    }
}

void driver_core_run(driver_core_t *core) {
    assert(core);

    unix_socket_server_listen(&core->uss,
                              (uss_acceptor_t)acceptor,
                              core);

    io_service_run(core->iosvc);
}
