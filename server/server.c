/**
 * server.c
 * --------
 *
 * Author:  Graeme Mathieson
 * Date:    29/07/1999
 *
 * This program binds to a UDP socket and sits, waiting for connections
 * to that socket.  It then performs actions based on the data it receives.
 * Eventually, it will also periodically send data in the form of UDP
 * packets, representing its current status
 */

#include <cliserv.h>
#include <protocol.h>
#include "server.h"

#include <signal.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>

/* Function prototypes */
void termination_handler (int signum); /* clean up before terminating */
int process_command ( void );
int poll_clients ( void );

/* File-level variables */
static sig_atomic_t g_keep_going = 1; /* controls when the main loop dies */

/* Global variables */
int         g_socket_fd;
const char *g_link_status_message[] =
{
	SERVER_STATUS_DOWN,
	SERVER_STATUS_UP,
	SERVER_STATUS_CONNECTING,
	SERVER_STATUS_DISCONNECTING
};

int main (int argc, char *argv[])
{
	struct sockaddr_in serv;

	/* do initial configuration */
	if (parse_command_line(argc, argv) < 0)
	{
		perror ("parse_command_line()");
		exit (EXIT_FAILURE);
	}

	if (read_config() < 0)
	{
		perror ("read_config()");
		exit (EXIT_FAILURE);
	}

	if (g_fork)
	{
		/* Code to fork ourselves off as a daemon */
		switch(fork ())
		{
		case -1: /* fork failed */
			perror ("fork()");
			exit (EXIT_FAILURE);
			/* break; unnecessary */
		case 0: /* we are the child process */
			if (!g_debug)
			{
				close (STDIN_FILENO);
				close (STDOUT_FILENO); /* need stdout still */
				close (STDERR_FILENO); /* also need stderr */
			}
			if (setsid () == -1)
				exit (EXIT_FAILURE);
			printf ("Running as pid %d\n", getpid ());
			break;
		default: /* we are the parent process, exit */
			return 0;
		}
	}

	/* Setup signal handler to cleanup on termination */
	if (signal (SIGTERM, termination_handler) == SIG_IGN)
		signal (SIGTERM, SIG_IGN);
	if (signal (SIGINT, termination_handler) == SIG_IGN)
		signal (SIGINT, SIG_IGN);
	signal (SIGHUP, SIG_IGN); /* Somebody thinks we have logs to rotate? */

	/* open a socket for the server and bind it to the server's
	   well-known port */
	if ((g_socket_fd = socket (PF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror("socket()");
		exit(EXIT_FAILURE);
	}
	memset (&serv, 0, sizeof (serv));
	serv.sin_family = AF_INET;
	serv.sin_port   = htons(g_srv_port);
	if (g_srv_inaddr)
		inet_aton(g_srv_inaddr, &serv.sin_addr);
	else
		serv.sin_addr.s_addr = INADDR_ANY;

	if (bind (g_socket_fd, (struct sockaddr *) &serv, sizeof (serv)) < 0)
	{
		perror("bind()");
		exit(EXIT_FAILURE);
	}

	if (broadcast_init_message () < 0)
	{
		perror ("broadcast_init_message()");
	}

	/* Now for the main program loop */
	while (g_keep_going)
	{
		struct timeval timeout;
		fd_set read_fds;
		int select_res;

		FD_ZERO (&read_fds);
		FD_SET (g_socket_fd, &read_fds);
		/* wait for the length specified by g_poll_time */
		timeout.tv_sec  = g_poll_time;
		timeout.tv_usec = 0;
		select_res = select (g_socket_fd + 1, &read_fds, NULL,
				     NULL, &timeout);
		if (select_res < 0)
		{
			if (errno == EINTR) /* ctrl-c at the console? */
			{
				g_keep_going = FALSE;
			}
			else
			{
				perror ("select()");
				exit (EXIT_FAILURE);
			}
		}
		else if (select_res > 0)
		{
			/* the only possible return before timeout
			   is that there is something on my socket */
			if ( process_command () < 0)
			{
				perror ("process_command ()");
				// exit (EXIT_FAILURE);
			}
		}

		/* regularly notify clients of a status change */
		if ( broadcast_status_message () < 0)
		{
			perror ("broadcast_status_message ()");
			// exit (EXIT_FAILURE);
		}

		if (timeout_old_clients () < 0)
		{
			perror ("timeout_old_clients ()");
			// exit (EXIT_FAILURE);
		}

		if (timeout_old_devices () < 0)
		{
			perror ("timeout_old_devices ()");
			// exit (EXIT_FAILURE);
		}

		if (g_debug)
		{
			printf ("---------------------------------------\n");
			dump_device_list (g_devices);
			dump_client_list (g_clients);
			printf ("---------------------------------------\n\n");
		}
	}

	/* finished */
	if (broadcast_quit_message () < 0)
	{
		perror ("broadcast_quit_message()");
	}

	if (g_debug)
		printf("link-server (pid %d) exiting...\n", getpid());

	return 0;
}

void termination_handler (int signum)
{
	/* Tell the main loop to give up next time round */
	g_keep_going = 0;

	/* register the signal handler again */
	signal (signum, termination_handler);
}

int process_command ( void )
{
	struct sockaddr_in cli;
	int recv_size, clilen = sizeof (cli);
	char recv_buffer[MAX_RECV_BUFFER];

	if ((recv_size = recvfrom (g_socket_fd, recv_buffer,
				   MAX_RECV_BUFFER, 0,
				   (struct sockaddr *) &cli,
				   &clilen)) < 0)
	{
		perror("recvfrom()");
		return (-1);
	}
	recv_buffer[recv_size] = 0; /* turn it into a real string */

	/* Find out where the message came from */
	if (strncmp (recv_buffer, CLIENT_PREFIX, strlen (CLIENT_PREFIX)) == 0)
	{
		if (g_debug)
			fprintf (stderr, "Received message from Client: %s\n",
				 recv_buffer);
		return process_client (cli, recv_buffer);
	}
	else if (strncmp (recv_buffer, NOTIFY_PREFIX,
			  strlen (NOTIFY_PREFIX)) == 0)
	{
		if (g_debug)
			fprintf (stderr, "Received message from Peer: %s\n",
				 recv_buffer);
		return process_peer (recv_buffer);
	}
	else
	{
		if (g_debug)
			fprintf (stderr, "Received invalid message: %s\n",
				 recv_buffer);
		errno = ENOTSUP; /* couldn't think of anything better */
		return (-1);
	}
}
