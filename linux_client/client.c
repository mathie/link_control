/*
 * client.c
 * ------------
 *
 * Author:  Graeme Mathieson
 * Date:    29/07/1999
 *
 * This is a test client to complement the server.  It is not intended
 * to be fully featured, but rather to be a basis for the Windows client,
 * which is of rather higher priority.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>

#include <errno.h>

#include <cliserv.h>
#include <protocol.h>
#include "client.h"

/* Global variables */
int             g_socket_fd;
pthread_mutex_t g_socket_fd_mutex = PTHREAD_MUTEX_INITIALIZER;
server_list_t  *g_servers;
pthread_mutex_t g_servers_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  g_servers_cond = PTHREAD_COND_INITIALIZER;
const char     *g_link_status_message[] =
{
	SERVER_STATUS_DOWN,
	SERVER_STATUS_UP,
	SERVER_STATUS_CONNECTING,
	SERVER_STATUS_DISCONNECTING
};

/* file scoped variables */
static pthread_t listen_thread;

/* local prototypes */
int wait_for_servers (void);
int init_socket (void);

int main (int argc, char *argv[])
{
	if (parse_command_line (argc, argv) < 0)
	{
		perror ("parse_command_line()");
		exit (EXIT_FAILURE);
	}

	if (read_config () < 0)
	{
		perror ("read_config()");
		exit (EXIT_FAILURE);
	}

	/* setup the socket */
	if (init_socket () < 0)
	{
		perror ("init_socket()");
		exit (EXIT_FAILURE);
	}

	/* create the thread that listens for servers */

	if (pthread_create (&listen_thread, NULL,
			    listen_for_status, NULL) != 0)
	{
		perror ("create_thread()");
		exit (EXIT_FAILURE);
	}

	/* sit in a loop, displaying the current status information
	   available to us */
	while (TRUE)
	{
		server_list_t *s_list_pos;
		if (g_servers == NULL)
		{
			/* Now wait until we have some servers to talk to */
			if (wait_for_servers () < 0)
			{
				perror ("wait_for_servers()");
				exit (EXIT_FAILURE);
			}
		}
		fprintf (stdout, "Current status:\n");
		pthread_mutex_lock (&g_servers_mutex);
		s_list_pos = g_servers;
		while (s_list_pos)
		{
			device_list_t *d_list_pos;
			fprintf (stdout, "Server %s (last heard %d):\n",
				 inet_ntoa(s_list_pos->data->sa.sin_addr),
				 (int)s_list_pos->data->last_heard_from);
			d_list_pos = s_list_pos->data->devices_controlled;
			while(d_list_pos)
			{
				fprintf (stdout, "\tDevice: %s\tStatus:%s\t"
					 "Time connected: %d\tNo Users: %d\n",
					 d_list_pos->data->device_name,
					 g_link_status_message[
						 d_list_pos->data->status],
					 (int)d_list_pos->data->connect_time,
					 d_list_pos->data->no_users);
				d_list_pos = d_list_pos->next;
			}
			s_list_pos = s_list_pos->next;
		}
		fprintf(stdout, "\n");
		pthread_mutex_unlock (&g_servers_mutex);
		sleep (5);
	}

	/* once we're finished, cancel the listener and wait for it to
	   exit properly */
/*	if (pthread_cancel (listen_thread) != 0)
	{
		perror ("pthread_cancel()");
		exit (EXIT_FAILURE);
		}*/
	if (pthread_join (listen_thread, NULL) != 0)
	{
		perror ("pthread_join()");
		exit (EXIT_FAILURE);
	}
	return 0;
}

int init_socket () 
{
	struct sockaddr_in group;
	struct ip_mreq mreq;

	pthread_mutex_lock (&g_socket_fd_mutex); /* no need to do this now but
						    there's no harm in doing so
						    for consistency */
	if ((g_socket_fd = socket (PF_INET, SOCK_DGRAM, 0)) < 0)
		return (-1);
	memset (&group, 0, sizeof (group));
	group.sin_family = AF_INET;
	inet_aton (g_multicast_group, &group.sin_addr);
	group.sin_port = htons (g_multicast_port);
	if (bind (g_socket_fd, (struct sockaddr *) &group, sizeof (group)) < 0)
		return (-1);

	/* Joing the multicast group */
	memcpy (&mreq.imr_multiaddr, &group.sin_addr, sizeof (struct in_addr));
	mreq.imr_interface.s_addr = htonl (INADDR_ANY);
	if (setsockopt (g_socket_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
			&mreq, sizeof (mreq)) < 0)
		return (-1);
	pthread_mutex_unlock (&g_socket_fd_mutex);
	return 0;
}

int wait_for_servers ()
{
	if (g_debug)
		fprintf (stderr, "Waiting for servers.");
	pthread_mutex_lock (&g_servers_mutex);
	while (g_servers == NULL)
	{
		int wait_result;
		struct timespec timeout;
		struct timeval now;
		gettimeofday (&now, NULL);
		timeout.tv_sec = now.tv_sec + 1;
		timeout.tv_nsec = now.tv_usec * 1000;

		if (g_debug)
			fputc ('.', stderr);
		wait_result = pthread_cond_timedwait (&g_servers_cond,
						      &g_servers_mutex,
						      &timeout);
		if (wait_result != 0)
		{
			switch (errno)
			{
			case ETIMEDOUT:
			case EINTR:
				break;
			default:
				return (-1);
			}
		}
	}
     	pthread_mutex_unlock (&g_servers_mutex);
	if (g_debug)
		fprintf (stderr, "\nFound a server!\n");
	return 0;
}
