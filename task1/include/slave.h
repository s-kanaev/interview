#ifndef _SLAVE_H_
# define _SLAVE_H_

# include "io-service.h"
# include "timer.h"
# include "master.h"

# include <netinet/in.h>

struct slave;
typedef struct slave slave_t;

struct slave {
    io_service_t *iosvc;
    tmr_t tmr;

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