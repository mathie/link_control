/**
 * notify.c
 * --------
 *
 * Author:  Graeme Mathieson
 * Date:    29/07/1999
 *
 * This program is a one-shot UDP messenger intended to send a packet to
 * the link server to indicate that the link has been activated or
 * closed.
 */

#include <stdio.h>
#include <string.h>

#include <cliserv.h>
#include <protocol.h>

#include "notify.h"

void print_usage (void);

int main(int argc, char *argv[])
{
	struct sockaddr_in serv;
	char send_buffer[MAX_SEND_BUFFER];
	int socket_fd;

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

	/* Do main initialisation here */
	if ((socket_fd = socket (PF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror("socket()");
		exit(EXIT_FAILURE);
	}

	memset (&serv, 0, sizeof (serv));
	serv.sin_family      = AF_INET;
	inet_aton(g_serv_addr, &serv.sin_addr);
	serv.sin_port        = htons(g_serv_port);

	if (g_command == UP)
	{
		strcpy (send_buffer, NOTIFY_ISUP);
	}
	else if (g_command == DOWN)
	{
		strcpy (send_buffer, NOTIFY_ISDOWN);
	}
	strcat (send_buffer, g_device);

	if (sendto(socket_fd, send_buffer, MAX_SEND_BUFFER, 0,
		   (struct sockaddr *)&serv, sizeof(serv)) != MAX_SEND_BUFFER)
	{
		perror("sento()");
		exit(EXIT_FAILURE);
	}
	return EXIT_SUCCESS;
}
