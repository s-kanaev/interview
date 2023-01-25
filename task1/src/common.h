#ifndef _COMMON_H_
# define _COMMON_H_

# include "io-service.h"
# include <stdint.h>

struct addrinfo;
struct sockaddr;

/**
 * Allocate UDP socket with broadcasting enabled.
 * \param [in] iface local endpoint interface to bind to
 * \param [in] local_port local endpoint port to bind to
 * \return socket FD or \c -1 on failure
 */
int allocate_udp_broadcasting_socket(const char *iface,
                                     uint16_t local_port,
                                     struct sockaddr *local_addr);

/**
 * Fetch broadcast address
 * \param [in] sfd socket FD
 * \param [in] iface interface to query for broadcast address
 * \param [out] braddr address to fill
 * \return \c 0 on success, \c -1 on failure
 */
int fetch_broadcast_addr(int sfd,
                         const char *iface,
                         struct sockaddr *braddr);

/**
 * Post a job to wait for sigterm, sigint through \c io_serivce_t instance.
 * \param [in] iosvc io service instance
 * \return \c true on success, \c false on failure
 *
 * The job will stop io service instance without waiting
 * for pending jobs.
 */
bool wait_for_sigterm_sigint(io_service_t *iosvc);

#endif /* _COMMON_H_ */