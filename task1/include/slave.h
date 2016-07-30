#ifndef _SLAVE_H_
# define _SLAVE_H_

# include "io-service.h"
# include "timer.h"
# include "master.h"

# include <netinet/in.h>

struct slave;
typedef struct slave slave_t;

typedef enum {
    SLAVE_IDLE,             /**< normal slave state */
    SLAVE_POLLING,          /**< slave initialized or responded to vote */
    SLAVE_MASTER,           /**< same as \c SLAVE_IDLE but
                                 mastering other slaves */
    SLAVE_WAITING_MASTER    /**< slave is idle but has quit the poll
                                 so won't answer votes*/
} slave_state_t;

struct slave {
    io_service_t *iosvc;
    tmr_t master_gone_tmr;
    tmr_t poll_tmr;
    tmr_t mastering_tmr;

    slave_state_t state;

    uint32_t max_vote_per_poll;

    uint8_t illumination;
    int8_t temperature;

    struct sockaddr_storage local_addr;
    struct sockaddr_in bcast_addr;

    int udp_socket;

    master_t master;
};

void slave_init(slave_t *sl, io_service_t *iosvc,
                const char *local_addr,
                const char *iface);
void slave_deinit(slave_t *sl);
void slave_run(slave_t *sl);

#endif /* _SLAVE_H_ */