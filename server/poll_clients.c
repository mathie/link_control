/* poll_clients.c
 * --------------
 *
 * regularly poll all the clients with current status information.
 * There are two methods here for polling clients - broadcasting to the
 * local subnet, or multicasting to all clients who are members of our
 * multicast group.
 */

#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <string.h>

#include <sys/utsname.h>

#include <protocol.h>
#include "server.h"

/* Multicasting versions of the polling code.  We have the necesary information
 * for polling in the form of g_multicast_group and g_multicast_port. 
 */

/* local prototypes */
int broadcast_message (char *message);

int broadcast_status_message (void)
{
	int retval;
	char *message;
	device_list_t *list_pos = g_devices;

	/* create the status message of the format:
	 *
	 * BROADCAST STATUS <device>\t<status>\n<device>\t<status>\n...
	 *
	 * where <status> can be one of:
	 *
	 * UP <time> <no_users>
	 * DOWN
	 * CONNECTING
	 * DISCONNECTING
	 */

	message = malloc (strlen (BROADCAST_STATUS) + 1);
	if (message == NULL)
		return (-1);

	strcpy (message, BROADCAST_STATUS);
	while (list_pos)
	{
		char *dev_stat = print_device_status (list_pos->data);
		if (dev_stat == NULL)
			return (-1);
		message = realloc (message,
				   strlen (message) + strlen (dev_stat) + 2);
		if (message == NULL)
			return (-1);
		strcat (message, dev_stat);
		strcat (message, "\n");
		free (dev_stat);
		list_pos = list_pos->next;
	}
		
	retval = broadcast_message(message);
	free (message);
	return retval;
}

int broadcast_init_message (void)
{
	char *message = strdup (BROADCAST_INIT);
	int retval = broadcast_message(message);
	free (message);
	return retval;
}

int broadcast_quit_message (void)
{
	char *message = strdup (BROADCAST_QUIT);
	int retval = broadcast_message(message);
	free (message);
	return retval;
}

int broadcast_message (char *send_buffer)
{
	struct sockaddr_in group;

	/* setup the socket address */
	memset (&group, 0, sizeof (group));
	group.sin_family = AF_INET;
	if (inet_aton (g_multicast_group, &group.sin_addr) == 0)
		return (-1);
	group.sin_port = htons (g_multicast_port);

	/* send it */
	if (sendto (g_socket_fd, send_buffer, strlen (send_buffer), 0,
		    (struct sockaddr *) &group,
		    sizeof (group)) != strlen (send_buffer))
	{
		return (-1);
	}

	return 0;
}

