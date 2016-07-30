#ifndef _PROTOCL_H_
# define _PROTOCL_H_

# include <stdint.h>

# define PKD __attribute__((packed))

# define SLAVE_UDP_PORT 12345

# define MASTER_REQUEST_TIMEOUT_MSEC    (5000)
# define MASTER_GONE_TIMEOUT_MSEC       (MASTER_REQUEST_TIMEOUT_MSEC * 2)
# define MASTER_RESPONSE_TIMEOUT_MSEC   (MASTER_REQUEST_TIMEOUT_MSEC * 9 / 10)

enum {
    PR_REQUEST      = 0x00,
    PR_RESPONSE     = 0x01,
    PR_MSG          = 0x02,
    PR_VOTE         = 0x03,
    PR_RESET_MASTER = 0x04
};

typedef struct pr_signature PKD {
    uint8_t s;
} pr_signature_t;

typedef struct pr_request PKD {
    pr_signature_t s;
    /* empty */
} pr_request_t;

typedef struct pr_response PKD {
    pr_signature_t s;
    int8_t temperature;
    uint8_t illumination;
} pr_response_t;

typedef struct pr_msg PKD {
    pr_signature_t s;
    uint8_t text[4];
    int8_t avg_temperature;
    uint32_t date_time;
    uint8_t brightness;
} pr_msg_t;

typedef struct pr_vote PKD {
    pr_signature_t s;
    uint32_t vote;
} pr_vote_t;

typedef struct pr_reset_master PKD {
    pr_signature_t s;
    /* empty */
} pr_reset_master_t;

#endif /* _PROTOCL_H_ */