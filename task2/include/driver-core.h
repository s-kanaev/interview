#ifndef _DRIVER_CORE_H_
# define _DRIVER_CORE_H_

# include "containers.h"
# include "unix-socket-server.h"

# include <stdbool.h>
# include <stddef.h>

struct driver_core;
typedef struct driver_core driver_core_t;

struct driver_payload;
typedef struct driver_payload driver_payload_t;

struct driver_command;
typedef struct driver_command driver_command_t;

struct driver_command_argument;
typedef struct driver_command_argument driver_command_argument_t;

typedef void (*driver_cmd_handler_t)(int idx,
                                     const char *name, size_t name_len,
                                     int argc,
                                     driver_command_argument_t *argv,
                                     buffer_t *response);

struct driver_core {
    uss_t uss;

    driver_payload_t *payload;
};

struct driver_payload {
    const char *name;
    size_t name_len;

    /* vector of driver_command_t */
    vector_t commands;
};

struct driver_command {
    const char *name;
    size_t name_len;

    const char *description;
    size_t description_len;

    int max_arity;

    driver_cmd_handler_t handler;
};

struct driver_command_argument {
    const char *arg;
    size_t arg_len;
};

bool driver_core_init(driver_core_t *core, driver_payload_t *payload);
void driver_core_deinit(driver_core_t *core);
void driver_core_run(driver_core_t *core);

#endif /* _DRIVER_CORE_H_ */
