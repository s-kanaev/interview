#ifndef _MASTER_H_
# define _MASTER_H_

# include "io-service.h"
# include "timer.h"
# include "avl-tree.h"

# include <stdint.h>

# define MASTER_UDP_PORT    23456

struct master;
typedef struct master master_t;

struct master {
    io_service_t *iosvc;
    tmr_t tmr;

    int8_t avg_temperature;
    uint8_t avg_brightness;

    time_t last_avg_timestamp;

    avl_tree_t slaves;

    int udp_socket;
};

void master_init(master_t *m, io_service_t *iosvc);
void master_deinit(master_t *m);
void master_run(master_t *m);

#endif /* _MASTER_H_ */