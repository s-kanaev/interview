#ifndef _MASTER_H_
# define _MASTER_H_

# include "io-service.h"
# include "timer.h"
# include "avl-tree.h"

# include <stdint.h>
# include <netinet/in.h>

struct master;
typedef struct master master_t;

struct master {
    io_service_t *iosvc;
    tmr_t tmr;

    /** slaves' parameters sum to use to calculate average */
    struct {
        int32_t temperature;
        uint32_t illumination;
    } sum;

    /** calculated averages and it's timestamp */
    struct {
        int8_t temperature;
        uint8_t illumination;
        time_t timestamp;
    } avg;

    /** slaves' lookup tree */
    avl_tree_t slaves;

    struct sockaddr_storage local_addr;
    struct sockaddr_in bcast_addr;
    int udp_socket;
};

void master_init(master_t *m, io_service_t *iosvc,
                 const char *iface);
void master_deinit(master_t *m);
void master_run(master_t *m);

#endif /* _MASTER_H_ */