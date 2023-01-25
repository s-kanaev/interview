#ifndef _SLAVE_H_
# define _SLAVE_H_

# include "io-service.h"
# include "timer.h"

struct master;

struct slave;
typedef struct slave slave_t;

struct slave {
    io_service_t *iosvc;
    tmr_t *tmr;
    int udp_socket;
    struct master *master;
};

void slave_init(slave_t *sl, io_service_t *iosvc);
void slave_deinit(slave_t *sl);
void slave_run(slave_t *sl);

#endif /* _SLAVE_H_ */