#ifndef _COMMON_H_
# define _COMMON_H_

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


#endif /* _COMMON_H_ */