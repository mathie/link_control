/* mcast.c
 * -------
 *
 * definitions of functions for version-independent IP
 * multicastng.  Currently it supports IPv4 and IPv6
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>

#include <mcast.h>

/* The following founctions return 0 if OK, -1 on error.  If there is an
   error, errno will be set appropriately. */
int mcast_join (int sock_fd, const struct sockaddr *sa, socklen_t salen,
		const char *ifname, u_int ifindex)
{
	switch (sa->sa_family)
	{
	case AF_INET:
	{
		struct ip_mreq mreq;
		struct ifreq ifreq;
		
		memcpy (&mreq.imr_multiaddr,
			&((struct sockaddr_in *) sa)->sin_addr,
			sizeof (struct in_addr));

		if (ifindex > 0)
		{
			if (if_indextoname (ifindex, ifreq.ifr_name) == NULL)
			{
				errno = ENXIO; /* if not found */
				return (-1);
			}
			goto doioctl;
		}
		else if (ifname != NULL)
		{
			strncpy (ifreq.ifr_name, ifname, IFNAMSIZ);
		doioctl:
			if (ioctl (sock_fd, SIOCGIFADDR, &ifreq) < 0)
				return (-1);
			memcpy (&mreq.imr_interface,
				&((struct sockaddr_in *) &ifreq.ifr_addr)->
				sin_addr,
				sizeof (struct in_addr));
		}
		else
		{
			mreq.imr_interface.s_addr = htonl (INADDR_ANY);
		}

		return (setsockopt (sock_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
				    &mreq, sizeof (mreq)));
	}
#ifdef IPV6
	case AF_INET6:
	{
		struct ipv6_mreq mreq6;

		memcpy (&mreq6.ipv6mr_multiaddr,
			&((struct sockaddr_in6 *) sa)->sin6_addr,
			sizeof (struct in6_addr));
		if (ifindex > 0)
		{
			req6.ipv6mr_interface = ifindex;
		}
		else if (ifname != NULL)
		{
			if ((mreq6.ipv6mr_interface = if_nametoindex (ifname))
			    == 0)
			{
				errno = ENXIO; /* if name not found */
				return (-1);
			}
		}
		else
		{
			mreq6.ipv6mr_interface = 0;
		}

		return setsockopt (sock_fd, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP,
				   &mreq6, sizeof (mreq6));
	}
#endif // IPV6
	default:
		errno = EPROTONOSUPPORT;
		return (-1);
	}
}

int mcast_leave (int sock_fd, const struct sockaddr *sa, socklen_t salen)
{
  errno = ENOTSUP;
  return -1;
}

int mcast_set_if (int sock_fd, const char *ifname, u_int ifindex)
{
  errno = ENOTSUP;
  return -1;
}

int mcast_set_loop (int sock_fd, int onoff)
{
	switch (sockfd_to_family (sock_fd))
	{
	case AF_INET:
	{
		u_char flag = onoff;
		return (setsockopt (sock_fd, IPPROTO_IP, IP_MULTICAST_LOOP,
				    &flag, sizeof (flag)));
	}
#ifdef IPV6
	case AF_INET6:
	{
		uint flag = onoff;
		return (setsockopt (sock_fd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP,
				    &flag, sizeof (flag)));
	}
#endif // IPV6
	default:
		errno = EPROTONOSUPPORT;
		return (-1);
	}
}

int mcast_set_ttl (int sock_fd, int ttl)
{
  errno = ENOTSUP;
  return -1;
}

/* returns non-negative interface index if OK, -1 on error */
int mcast_get_if (int sock_fd)
{
  errno = ENOTSUP;
  return -1;
}

/* returns current loopback flag if OK, -1 on error */
int mcast_get_loop (int sock_fd)
{
  errno = ENOTSUP;
  return -1;
}

/* returns current TTL if OK, -1 on error */
int mcast_get_ttl (int sock_fd)
{
  errno = ENOTSUP;
  return -1;
}
