/* mcast.h
 * -------
 *
 * definitions and function prototypes for version-independent IP
 * multicastng.  Currently it supports IPv4 and IPv6
 */

#ifndef _MCAST_H_
#define _MCAST_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* The following founctions return 0 if OK, -1 on error.  If there is an
   error, errno will be set appropriately. */
int mcast_join (int sock_fd, const struct sockaddr *sa, socklen_t salen,
		const char *ifname, u_int ifindex);

int mcast_leave (int sock_fd, const struct sockaddr *sa, socklen_t salen);

int mcast_set_if (int sock_fd, const char *ifname, u_int ifindex);

int mcast_set_loop (int sock_fd, int flag);

int mcast_set_ttl (int sock_fd, int ttl);

/* returns non-negative interface index if OK, -1 on error */
int mcast_get_if (int sock_fd);

/* returns current loopback flag if OK, -1 on error */
int mcast_get_loop (int sock_fd);

/* returns current TTL if OK, -1 on error */
int mcast_get_ttl (int sock_fd);

#endif // _MCAST_H_
