#ifndef _PROTOCOL_H_
# define _PROTOCOL_H_

# include <stdint.h>

# define PKD __attribute__((packed))

# define MAX_COMMAND_NAME_LEN               6
# define MAX_COMMAND_DESCRIPTION_LEN        255

struct pr_signature;
typedef struct pr_signature pr_signature_t;

struct pr_driver_info;
typedef struct pr_driver_info pr_driver_info_t;

struct pr_driver_command_info;
typedef struct pr_driver_command_info pr_driver_command_info_t;

struct pr_driver_command;
typedef struct pr_driver_command pr_driver_command_t;

struct pr_driver_command_argument;
typedef struct pr_driver_command_argument pr_driver_command_argument_t;

struct pr_driver_response;
typedef pr_driver_response pr_driver_response_t;

enum {
    PR_DRV_INFO             = 0x00,
    PR_DRV_COMMAND          = 0x01,
    PR_DRV_RESPONSE         = 0x02
};

struct PKD pr_signature {
    uint8_t s;
};

struct PKD pr_driver_info {
    pr_signature_t s;
    uint8_t commands_number;
};

struct PKD pr_driver_command_info {
    uint8_t name[MAX_COMMAND_NAME_LEN];
    uint8_t arity;
    uint8_t descr[MAX_COMMAND_DESCRIPTION_LEN];
};

struct PKD pr_driver_command {
    pr_signature_t s;
    uint8_t cmd_idx;
    uint8_t argc;
};

struct PKD pr_driver_command_argument {
    uint8_t len;
};

struct PKD pr_driver_response {
    pr_signature_t s;
    uint32_t len;
};

#endif /* _PROTOCOL_H_ */