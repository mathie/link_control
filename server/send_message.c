/* send_message.c
 * --------------
 *
 * sends a message to a client
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <cliserv.h>
#include <protocol.h>
#include "server.h"

int send_device_list   (client_t *client)
{
	/* send a list of devices back to the client, in the form:

	   <device_name>\t<device_description>\n
	   <device_name>\t<device_description>\n
	   ...

	   This means that there is a restriction on device names and
	   descriptions that they will not contain tabs or newlines.  This
	   will probably have been imposed by the config parser when I write
	   that...

	*/

	/* first construct the string to send */
	device_list_t *list_pos = g_devices;
	char *dev_str = (char *)malloc (strlen (SERVER_DEVICES) + 1);
	if (dev_str == NULL)
		return (-1);
	strcpy (dev_str, SERVER_DEVICES);

	while(list_pos)
	{
		size_t new_len = strlen (list_pos->data->device_name) +
			strlen (list_pos->data->device_description) +
			strlen (dev_str) + 4;
		dev_str = realloc (dev_str, new_len);
		if (dev_str == NULL)
			return (-1);
		strcat (dev_str, list_pos->data->device_name);
		strcat (dev_str, "\t");
		strcat (dev_str, list_pos->data->device_description);
		strcat (dev_str, "\n");
		list_pos = list_pos->next;
	}
	
	/* now send the data */
	if (sendto (g_socket_fd, dev_str, strlen (dev_str), 0,
		    (struct sockaddr *) &client->sa, sizeof (client->sa))
	    != strlen (dev_str))
	{
		/* shouldn't die horribly just 'cos client didn't
		   listen to us */
		if (errno != ECONNREFUSED)
		{
			perror ("send_device_list()");
			return (-1);
		}
	}
	return 0;
}

int send_device_status (client_t *client, device_t *device)
{
	/* create a status message for the current device and
	   send it directly to the client */

	char *dev_stat;
	int max_str_len = strlen (SERVER_STATUS_PREFIX) +
		strlen (SERVER_STATUS_DISCONNECTING) +
		strlen (device->device_name) + 4; /* this should be the longest
						     possible string length */
	char *dev_str = (char *)malloc (max_str_len);
	if (dev_str == NULL)
		return (-1);
	
	dev_stat = print_device_status (device);
	if (dev_stat == NULL)
		return (-1);
	strcpy (dev_str, SERVER_STATUS_PREFIX);
	strcat (dev_str, dev_stat);
	free (dev_stat);

	/* now send the data */
	if (sendto (g_socket_fd, dev_str, strlen (dev_str), 0,
		    (struct sockaddr *) &client->sa, sizeof (client->sa))
	    != strlen (dev_str))
	{
		/* shouldn't die horribly just 'cos client didn't
		   listen to us */
		if (errno != ECONNREFUSED)
		{
			perror ("send_device_list()");
			return (-1);
		}
	}
	return 0;
}

char *print_device_status (device_t *device)
{
	int max_str_len = strlen (device->device_name) +
		strlen (g_link_status_message[device->status]) + 2;
	char *dev_str = (char *)malloc (max_str_len);
	if (dev_str == NULL)
		return NULL;
	strcpy (dev_str, device->device_name);
	strcat (dev_str, g_link_status_message[device->status]);

	if (device->status == LINK_UP)
	{
		/* append the uptime and number of users */
		client_list_t *list_pos = device->clients_connected;
		int no_users = 0;
		char params[20]; /* combination of time and no_users cannot
				    exceed 18 digits - should be OK */
		while(list_pos)
		{
			no_users++;
			list_pos = list_pos->next;
		}
		sprintf (params, "%d %d",
			 (int)(time (NULL) - device->connect_time),
			 no_users);
		dev_str = realloc (dev_str, strlen (dev_str) +
				   strlen(params) + 1);
		if (dev_str == NULL)
			return NULL;
		strcat (dev_str, params);
	}
	
	return dev_str;
}

int send_client_status (client_t *client)
{
	/* return a list of devices that this client is currently connected
	   to, in the format:

	   <device>\t<device>\t<device>....

	*/

	/* first construct the string to send */
	device_list_t *list_pos = client->devices_connected;
	char *dev_str = (char *)malloc (strlen (SERVER_CLIENT_STATUS) + 1);
	if (dev_str == NULL)
		return (-1);
	strcpy (dev_str, SERVER_CLIENT_STATUS);
	if (list_pos)
	{
		dev_str = realloc (dev_str, strlen (dev_str) +
				   strlen (list_pos->data->device_name) + 1);
		if (dev_str == NULL)
			return (-1);
		strcat (dev_str, list_pos->data->device_name);
		list_pos = list_pos->next;
	}

	while(list_pos)
	{
		size_t new_len = strlen (list_pos->data->device_name) +
			strlen (dev_str) + 3;
		dev_str = realloc (dev_str, new_len);
		if (dev_str == NULL)
			return (-1);
		strcat (dev_str, "\t");
		strcat (dev_str, list_pos->data->device_name);

		list_pos = list_pos->next;
	}
	
	/* now send the data */
	if (sendto (g_socket_fd, dev_str, strlen (dev_str), 0,
		    (struct sockaddr *) &client->sa, sizeof (client->sa))
	    != strlen (dev_str))
	{
		/* shouldn't die horribly just 'cos client didn't
		   listen to us */
		if (errno != ECONNREFUSED)
		{
			perror ("send_device_list()");
			return (-1);
		}
	}
	return 0;
}
