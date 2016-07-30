#ifndef _PROTOCL_H_
# define _PROTOCL_H_

# include <stdint.h>
# include <stddef.h>

# define PKD __attribute__((packed))

# define UDP_PORT                       12345
# define UDP_PORT_STR                   "12345"

# define MASTER_REQUEST_TIMEOUT_MSEC    (5000)
# define MASTER_GONE_TIMEOUT_MSEC       (MASTER_REQUEST_TIMEOUT_MSEC * 2)
# define MASTER_RESPONSE_TIMEOUT_MSEC   (MASTER_REQUEST_TIMEOUT_MSEC * 9 / 10)

enum {
    PR_REQUEST      = 0x00,
    PR_RESPONSE     = 0x01,
    PR_MSG          = 0x02,
    PR_VOTE         = 0x03,
    PR_RESET_MASTER = 0x04,
    PR_COUNT
};

typedef struct PKD pr_signature {
    uint8_t s;
} pr_signature_t;

typedef struct PKD pr_request {
    pr_signature_t s;
    /* empty */
} pr_request_t;

typedef struct PKD pr_response {
    pr_signature_t s;
    int8_t temperature;
    uint8_t illumination;
} pr_response_t;

typedef struct PKD pr_msg {
    pr_signature_t s;
    uint8_t text[5];
    int8_t avg_temperature;
    uint32_t date_time;
    uint8_t brightness;
} pr_msg_t;

typedef struct PKD pr_vote {
    pr_signature_t s;
    uint32_t vote;
} pr_vote_t;

typedef struct PKD pr_reset_master {
    pr_signature_t s;
    /* empty */
} pr_reset_master_t;

# define PR_MAX_SIZE (sizeof(pr_msg_t))
# define PR_MIN_SIZE (sizeof(pr_request_t))

const extern size_t PR_STRUCT_EXPECTED_SIZE[PR_COUNT];

#endif /* _PROTOCL_H_ */