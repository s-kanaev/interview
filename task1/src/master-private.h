#ifndef _MASTER_PRIVATE_H_
# define _MASTER_PRIVATE_H_

# include "master.h"
# include "protocol.h"

typedef struct slave_description {
    int8_t temperature;
    uint8_t illumination;
} slave_description_t;

/**
 * Update or set slave in master's lookup tree.
 * \param [in] m master instance
 * \param [in] ip slave address
 * \param [in] sd slave's description
 * \return tree node pointing to the slave
 *
 * The function will either add another node or update existing one.
 * Also, \c m->temperature_sum and \c m->illumination_sum will be updated
 * appropriately.
 *
 */
avl_tree_node_t *
master_update_slave(master_t *m,
                    uint32_t ip,
                    const slave_description_t *sd);

/**
 * Calculate averages of parameters.
 * \param [in] m master instance
 *
 * The function will also update \c m->last_avg_timestamp
 */
void
master_calculate_averages(master_t *m);

/**
 * Calculate brightness based on illumination average.
 * \param [in] m master instance
 * \return average brightness setting
 *
 * Ascending brightness values: 1, 2, 3, 4, ..., 254, 255, 0
 */
uint8_t
master_calculate_brightenss(const master_t *m);

/**
 * Arm master's timer
 * \param [in] m master instance
 */
void
master_arm_timer(master_t *m);

/**
 * Disarm master's timer
 * \param [in] m master instance
 */
void
master_disarm_timer(master_t *m);

/**
 * Initialize master's internals.
 * \param [in] m master instance
 * \param [in] iosvc IO service instance
 *
 * The function will initialize timer, slaves lookup tree and
 * zero paramters sums and averages and timestamps.
 */
void
master_init_(master_t *m,
             io_service_t *iosvc);

/**
 * Deinitialize master's internals.
 * \param [in] m master instance
 *
 * The function will deinitialize only internals initialized
 * with \c master_init_
 */
void
master_deinit_(master_t *m);

/**
 * Act appropriately to data received
 * \param [in] m master instance
 * \param [in] packet data received header
 * \param [in] fd UDP socket fd to send response if any
 */
void
master_act(master_t *m, const pr_signature_t *packet, int fd);

/**
 * Set master's broadcast address.
 * \param m master instance
 * \param bcast_addr broadcast address pointer
 */
void
master_set_broadcast_addr(master_t *m, struct sockaddr *bcast_addr);

#endif /* _MASTER_PRIVATE_H_ */
