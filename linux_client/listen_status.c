/**
 * listen_status.c
 * --------------------
 *
 * Author:  Graeme Mathieson
 * Date:    30/07/1999
 *
 * This contains the code inside the thread which listens out for servers
 * on the multicast channel.
 */


#include <signal.h>
#include <sys/time.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>

#include <cliserv.h>
#include <protocol.h>
#include "client.h"

/* Global variables */

/* Function prototypes */
int process_command();
int process_status_message (server_t *server, char *message);

/* This is the entry point for the listener thread */
void *listen_for_status (void *nothing)
{
	fd_set read_fds;
	int select_res = 0;

	/* don't cancel the thread until an appropriate point */
	pthread_setcanceltype (PTHREAD_CANCEL_DEFERRED, NULL);

	while (TRUE) /* the main program will kill us */
	{
		pthread_mutex_lock (&g_socket_fd_mutex);
		FD_ZERO(&read_fds);
		FD_SET(g_socket_fd, &read_fds);
		select_res = select (g_socket_fd + 1, &read_fds, NULL,
				     NULL, NULL);
		pthread_mutex_unlock (&g_socket_fd_mutex);

		if (select_res < 0)
		{
			perror ("select()");
			return NULL;
		}
		else if (select_res > 0)
		{
			/* the only possible return before timeout
			   is that there is something on my socket */
			if (process_command () < 0)
			{
				perror ("process_command()");
				continue;
			}
		}
		/* If you want to cancel, do it now */
		pthread_testcancel ();
	}
	return NULL;
}

int process_command()
{
	server_t *new_server;
	struct sockaddr_in server_sa;
	int recv_size, server_sa_len = sizeof (server_sa);
	char recv_buffer[MAX_RECV_BUFFER];

	pthread_mutex_lock (&g_socket_fd_mutex);
	if ((recv_size = recvfrom (g_socket_fd, recv_buffer,
				   MAX_RECV_BUFFER, 0,
				   (struct sockaddr *) &server_sa,
				   &server_sa_len)) < 0)
	{
		perror("recvfrom()");
		exit(EXIT_FAILURE);
	}
	pthread_mutex_unlock (&g_socket_fd_mutex);

	/* Turn it into a proper string so I can use string handling stuff */
	recv_buffer[recv_size] = 0;
	
	/* Before parsing the string, check we know about the server */
	if ((new_server = get_server (&g_servers, server_sa.sin_addr)) == NULL)
	{
		if (errno != ENODEV)
			return (-1);
		/* server does not exist - add it */
		new_server = (server_t *)malloc (sizeof (server_t));
		if (new_server == NULL)
			return (-1);
		new_server->sa = server_sa;
		new_server->devices_controlled = NULL;

		pthread_mutex_lock (&g_servers_mutex);
		add_server (&g_servers, new_server);
	
		/* signal the condition to the main thread */
		if (g_servers)
			pthread_cond_signal (&g_servers_cond);
		pthread_mutex_unlock (&g_servers_mutex);
	}

	/* update the last heard time - since we just heard from it :) */
	pthread_mutex_lock (&g_servers_mutex);
	new_server->last_heard_from = time (NULL);
	pthread_mutex_unlock (&g_servers_mutex);

	/* Parse the string for broadcast information */
	if (strncmp (recv_buffer, BROADCAST_INIT,
		     strlen (BROADCAST_INIT)) == 0)
	{
		/* Initialisation message from the server */
		if (g_debug)
			fprintf (stderr, "Dropping stored information about"
				 " server %s\n",
				 inet_ntoa(new_server->sa.sin_addr));
		/* remove knowledge of devices controlled */
		pthread_mutex_lock (&g_servers_mutex);
		while (new_server->devices_controlled)
		{
			if (rm_device(&new_server->devices_controlled,
				      new_server->devices_controlled->data)< 0)
			{
				int real_errno = errno;
				pthread_mutex_unlock (&g_servers_mutex);
				errno = real_errno;
				return (-1);
			}
		}
		pthread_mutex_unlock (&g_servers_mutex);
		return 0;
	}
	else if (strncmp (recv_buffer, BROADCAST_QUIT,
			  strlen (BROADCAST_QUIT)) == 0)
	{
		if (g_debug)
			fprintf (stderr, "Server %s quitting removing entry\n",
				 inet_ntoa(new_server->sa.sin_addr));
		/* the server has quit.  Remove it from our list */
		pthread_mutex_lock (&g_servers_mutex);
		if (rm_server (&g_servers, new_server) < 0)
		{
			int real_errno = errno;
			pthread_mutex_unlock (&g_servers_mutex);
			errno = real_errno;
			return (-1);
		}
		pthread_mutex_unlock (&g_servers_mutex);
		return 0;
	}
	else if (strncmp (recv_buffer, BROADCAST_STATUS,
			  strlen (BROADCAST_STATUS)) == 0)
	{
		/* regular status message */
		char *message = recv_buffer + strlen (BROADCAST_STATUS);
		pthread_mutex_unlock (&g_servers_mutex);
		if (process_status_message (new_server, message) < 0)
		{
			int real_errno = errno;
			pthread_mutex_unlock (&g_servers_mutex);
			errno = real_errno;
			return (-1);
		}
		pthread_mutex_unlock (&g_servers_mutex);
		return 0;
	}
	else 
	{
		/* unrecognised message */
		errno = ENOTSUP;
		return (-1);
	}
	return 0;
}

int process_status_message (server_t *server, char *message)
{
	/* We get a message of the format <device_name>\t<device_status>\n
	   repeated ad nauseum.   Parse it nicely so that it can be
	   interpreted and inserted into the appropriate device entry. */

	int strtok_setup = FALSE;
	char delims[] = "\n\t";
	char *device_name, *device_status;

	while ((device_name = strtok ((strtok_setup ? NULL : message),
				      delims)) != NULL)
	{
		device_t *device;
		strtok_setup = TRUE;
		device_status = strtok(NULL, delims);

		if (device_status == NULL)
		{
			fprintf (stderr, "Unbalanced device name/status.\n");
			break;
		}
		
		if ((device = get_device (&server->devices_controlled, 
					  device_name)) == NULL)
		{
			/* the device does not exist */
			device = malloc (sizeof (device_t));
			if (device == NULL)
				return (-1);
			memset (device, 0, sizeof (device_t));
			device->device_name = strdup (device_name);
			if (add_device (&server->devices_controlled,
					device) < 0)
			{
				return (-1);
			}
		}

		/* We have a device, update its status */
		if (strncmp (device_status, "UP", strlen ("UP")) == 0)
		{
			/* we have "UP x y" where x is connect_time and 
			   y is number of users */
			unsigned long x, y;
			char *x_str = device_status + 3;
			char *y_str = strchr (x_str, ' ') + 1;
			x = strtoul (x_str, NULL, 0);
			y = strtoul (y_str, NULL, 0);
			device->status = LINK_UP;
			device->connect_time = x;
			device->no_users = y;
		}
		else if (strcmp (device_status, "DOWN") == 0)
		{
			device->status = LINK_DOWN;
			device->connect_time = 0;
			device->no_users = 0;
		}
		else if (strcmp (device_status, "CONNECTING") == 0)
		{
			device->status = LINK_CONNECTING;
			device->connect_time = 0;
			device->no_users = 0;
		}
		else if (strcmp (device_status, "DISCONNECTING") == 0)
		{
			device->status = LINK_DISCONNECTING;
			device->connect_time = 0;
			device->no_users = 0;
		}
		else
		{
			fprintf (stderr, "Unknown status for device %s: %s\n",
				 device_name, device_status);
		}
	}
	return 0;
}

