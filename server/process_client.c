/* process_client.c
 * ----------------
 *
 * This source contains all the functions to process commands from the client
 * to the server.
 */

#include <errno.h>

#include <cliserv.h>
#include <protocol.h>
#include "server.h"
#include <string.h>

int process_client (struct sockaddr_in cli, char *message)
{
	/* see whether the client is in our list of known clients */
	client_t *client = get_client (&g_clients,cli.sin_addr);

	if (client == NULL)
	{
		if (errno == ENODEV) /* must be a new client */
		{
			client = (client_t *)malloc (sizeof (client_t));
			if (client == NULL)
				return (-1);
			memcpy (&client->sa, &cli,
				sizeof (struct sockaddr_in));
			client->last_heard_from = 0;
			client->devices_connected = NULL;
			if (add_client (&g_clients, client) < 0)
				return (-1);
		}
	}
	/* update the last heard time  and sockaddr_in struct */
	update_client (client, &cli);

	/* first figure out what the message is */
	if (strncmp (message, CLIENT_PING, strlen (CLIENT_PING)) == 0)
	{
		/* Simply update the last heard from time */
		return 0;
	}
	else if(strncmp (message, CLIENT_DEVICES, strlen (CLIENT_DEVICES)) == 0)
	{
		/* return a list of devices to the client */
		return send_device_list (client);
	}
	else if (strncmp (message, CLIENT_UP, strlen (CLIENT_UP)) == 0)
	{
		char *dev_str = message + strlen (CLIENT_UP);
		device_t *device = get_device (&g_devices, dev_str);
		if (device == NULL)
		{
			return (-1);
		}
		if (connect_client_to_device (client, device) < 0)
		{
			return (-1);
		}
		return 0;
	}
	else if (strncmp (message, CLIENT_DOWN, strlen (CLIENT_DOWN)) == 0)
	{
		char *dev_str = message + strlen (CLIENT_DOWN);
		device_t *device = get_device (&client->devices_connected,
					       dev_str);
		if (device == NULL)
		{
			return (-1);
		}
		if (disconnect_client_from_device (client, device) < 0)
		{
			return (-1);
		}
		return 0;
	}
	else if (strncmp (message, CLIENT_FORCE_DOWN,
			  strlen (CLIENT_FORCE_DOWN)) == 0)
	{
		/* the current device is to be forced down regardless of who
		   is connected */
		char *dev_str = message + strlen (CLIENT_FORCE_DOWN);
		device_t *device = get_device (&g_devices, dev_str);
		if (device == NULL)
		{
			errno = ENODEV;
			return (-1);
		}
		
		if (alter_device_status (device, LINK_DOWN) < 0)
			return (-1);
		if (remove_all_clients_from_device (device) < 0)
			return (-1);

		return 0;
	}
	else if (strncmp (message, CLIENT_STATUS, strlen (CLIENT_STATUS)) == 0)
	{
		char *dev_str = message + strlen (CLIENT_STATUS);
		device_t *device = get_device (&g_devices, dev_str);
		if (device == NULL)
		{	
			return (-1);
		}

		return send_device_status (client, device);
	}
	else if (strncmp (message, CLIENT_CLIENT_STATUS,
			  strlen (CLIENT_CLIENT_STATUS)) == 0)
	{
		return send_client_status (client);
	}
	else
	{
		/* unknown message */
		errno = ENOTSUP;
		return (-1);
	}
	
}

