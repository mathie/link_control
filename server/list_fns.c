/**
 * list_fns.c
 * ----------
 *
 * ancillary functions to manage the lists associated with the link server.
 * we have a global list of devices and a global list of clients.
 */

#include <errno.h>
#include <string.h>

#include "server.h"

/* managing g_devices  and device_lists that clients are connected to */
device_list_t *g_devices = NULL;

int add_device (device_list_t **pp_devices, device_t *new_device)
{
	device_list_t *new_dev_list_entry, *list_pos;

	if (get_device (pp_devices, new_device->device_name) != NULL)
	{
		errno = EALREADY;
		return (-1);
	}

	new_dev_list_entry = (device_list_t *)malloc (sizeof (device_list_t));
	if (new_dev_list_entry == NULL) /* malloc failed */
	{
		return (-1);
	}
	new_dev_list_entry->next = NULL;
	new_dev_list_entry->data = new_device;

	// check for an empty list
	if (*pp_devices == NULL)
	{
		*pp_devices = new_dev_list_entry;
	}
	else
	{
		list_pos = *pp_devices;
		while (list_pos->next != NULL) /* find the last entry */
		{
			// sanity check - is the device already there?
			if (list_pos->data == new_device)
			{
				free (new_dev_list_entry);
				errno = EALREADY; /* already there */
				return (-1);
			}
			list_pos = list_pos->next;
		}
		list_pos->next = new_dev_list_entry;
	}
	return 0;
}

int rm_device (device_list_t **pp_devices, device_t *dev)
{
	device_list_t *list_pos, *prev_pos;

	// sanity check
	if (*pp_devices == NULL)
	{
		errno = ENODEV; /* device not found */
		return (-1);
	}

	// check if it's the root node
	if ((*pp_devices)->data == dev)
	{
		list_pos = (*pp_devices)->next;
		free (*pp_devices);
		*pp_devices = list_pos;
		return 0;
	}
	
	prev_pos = *pp_devices;
	list_pos = (*pp_devices)->next;
	while (list_pos)
	{
		if (list_pos->data == dev)
		{
			// delete it
			prev_pos->next = list_pos->next;
			free (list_pos);
			return 0;
		}

		prev_pos = list_pos;
		list_pos = list_pos->next;
	}
	// didn't find it
	errno = ENODEV; /* device not found */
	return (-1);
}

device_t *get_device (device_list_t **pp_devices, char *dev_name)
{
	device_list_t *list_pos = *pp_devices;

	while (list_pos)
	{
		if (strcmp (list_pos->data->device_name, dev_name) == 0)
		{
			// we have a match
			return list_pos->data;
		}
		list_pos = list_pos->next;
	}
	// not found
	errno = ENODEV;
	return NULL;
}

int timeout_old_devices ()
{
	/* Here we are searching for the following occurences:
	 *
	 * (status == LINK_CONNECTING) && ((connect_time + timeout) < time()):
	 * The link has failed to connect properly.  if retries > 0, 
	 * decrement it and retry.  Else, force the link down.
	 *
	 * (status == LINK_DISCONNECTING) && ((connect_time+timeout) < time()):
	 * The link has failed to disconnect within its alloted time. Force
	 * the link down.
	 */

	device_list_t *list_pos = g_devices;
	
	while (list_pos)
	{
		switch (list_pos->data->status)
		{
		case LINK_CONNECTING:
			if ((list_pos->data->connect_time + g_connect_timeout)
			    < time (NULL))
			{
				if (alter_device_status (list_pos->data,
							 LINK_CONNECTING) < 0)
					return (-1);
			}
			break;
		case LINK_DISCONNECTING:
			if ((list_pos->data->connect_time
			     + g_disconnect_timeout) < time (NULL))
			{
				if (alter_device_status (list_pos->data,
							 LINK_DISCONNECTING)
				    < 0)
					return (-1);
			}
			break;
		default:
			/* not interested in any other case */
			break;
		}
		list_pos = list_pos->next;
	}

	return 0;
}

#ifdef DEBUG
void dump_device_list (device_list_t *devices)
{
	int i = 0;
	printf ("Displaying device status:\n");
	while (devices)
	{
		client_list_t *c_list_pos;
		switch (devices->data->status)
		{
		case LINK_UP:
			printf ("Device %d:\t%s\t\t%s (%d)\t",
				i++,
				devices->data->device_name,
				g_link_status_message[devices->data->status],
				(int)devices->data->connect_time);
			break;
		default:
			printf ("Device %d:\t%s\t\t%s\t",
				i++,
				devices->data->device_name,
				g_link_status_message[devices->data->status]);
			break;
		}

		c_list_pos = devices->data->clients_connected;
		if (c_list_pos != NULL)
		{
			printf ("%s",
				inet_ntoa(c_list_pos->data->sa.sin_addr));
			while (c_list_pos->next)
			{
				c_list_pos = c_list_pos->next;
				printf(", %s",
				     inet_ntoa(c_list_pos->data->sa.sin_addr));
			}
		}
		putchar('\n');
		
		devices = devices->next;
	}
}
#endif // DEBUG

/* managing g_clients list */
client_list_t *g_clients = NULL;

int add_client (client_list_t **pp_clients, client_t *new_client)
{
	client_list_t *new_client_list_entry;
	
	// first check to see if it's already in the list
	if (get_client (pp_clients, new_client->sa.sin_addr) != NULL)
	{
		errno = EALREADY;
		return (-1);
	}

	new_client_list_entry = (client_list_t *)malloc (
		sizeof (client_list_t));
	if (new_client_list_entry == NULL) // malloc failed
	{
		return (-1);
	}
	new_client_list_entry->next = NULL;
	new_client_list_entry->data = new_client;

	if (*pp_clients == NULL) // no clients
	{
		*pp_clients = new_client_list_entry;
	}
	else
	{
		client_list_t *list_pos = *pp_clients;
		while (list_pos->next)
		{
			// sanity check - the client is in the list
			if (list_pos->data == new_client)
			{
				free (new_client_list_entry);
				errno = EALREADY; /* already there */
				return (-1);
			}
			list_pos = list_pos->next;
		}
		list_pos->next = new_client_list_entry;
	}
	return 0;
}

int rm_client (client_list_t **pp_clients, client_t *client)
{
	client_list_t *prev_pos, *list_pos;

	// sanity check
	if (*pp_clients == NULL)
	{
		errno = ENODEV; /* no such client (?) */
		return (-1);
	}

        // is it the root node ?
	if ((*pp_clients)->data == client)
	{
		list_pos = (*pp_clients)->next;
		free (*pp_clients);
		*pp_clients = list_pos;
		return 0;
	}

	prev_pos = *pp_clients;
	list_pos = (*pp_clients)->next;
	while (list_pos)
	{
		if (list_pos->data == client)
		{
			prev_pos->next = list_pos->next;
			free (list_pos);
			return 0;
		}
		prev_pos = list_pos;
		list_pos = list_pos->next;
	}
	// not found
	errno = ENODEV;
	return (-1);
}

client_t *get_client (client_list_t **p_clients, struct in_addr inet_addr)
{
	client_list_t *list_pos = *p_clients;

	while (list_pos)
	{
		if (list_pos->data->sa.sin_addr.s_addr == inet_addr.s_addr)
		{
			return list_pos->data;
		}
		list_pos = list_pos->next;
	}
	errno = ENODEV;
	return NULL; // didn't find it
}

int update_client (client_t *client, struct sockaddr_in *cli)
{
	client->last_heard_from = time (NULL);
	memcpy (&client->sa, cli, sizeof(struct sockaddr_in));

	return 0;
}

int timeout_old_clients ()
{
	/* any client who's last_heard_from time is more than g_client_timeout
	   seconds old is removed from existance.  This is a failsafe in case
	   somebody closes down their computer without closing the link
	   properly.  The only issue is that a live client must poll the server
	   more often than this timeout. */

	client_list_t *list_pos = g_clients, *next_pos;
	client_t *client;

	while (list_pos)
	{
		next_pos = list_pos->next;
		client   = list_pos->data;
		if (time (NULL) > (client->last_heard_from + g_client_timeout))
		{
			if (remove_client_from_all_devices (client) < 0)
				return (-1);
			if (rm_client (&g_clients, client) < 0)
				return (-1);
			free (client);
		}
		list_pos = next_pos;
	}
	return 0;
}

#ifdef DEBUG
void dump_client_list (client_list_t *clients)
{
	int i = 0;
	client_list_t *list_pos = clients;

	printf ("Dumping client list:\n");
	while (list_pos)
	{
		device_list_t *d_list_pos;

		printf ("\tClient %d:\t%s\t%d\t",
			i++,
			inet_ntoa(list_pos->data->sa.sin_addr),
			(int)list_pos->data->last_heard_from);
		d_list_pos = list_pos->data->devices_connected;
		if (d_list_pos != NULL)
		{
			printf ("%s", d_list_pos->data->device_name);
			while (d_list_pos->next)
			{
				d_list_pos = d_list_pos->next;
				printf(", %s", d_list_pos->data->device_name);
			}
		}
		putchar('\n');
		
		list_pos = list_pos->next;
	}
}
#endif // DEBUG

int remove_device_from_all_clients (device_t *device)
{
	client_list_t *list_pos;

	/* search through the global client list and remove all instances
	   of this device from the client */

	for (list_pos = g_clients; list_pos; list_pos = list_pos->next)
	{
		int ret = rm_device (&list_pos->data->devices_connected,
				     device);
		if (ret < 0) // rm_device failed
		{
			if (errno != ENODEV) // ignore ENODEV
			{
				return (-1);
			}
		}
	}
	return 0;
}

int remove_client_from_all_devices (client_t *client)
{
	device_list_t *list_pos;

	/* search through the global device list and remove all instances
	   of this client from each device */

	for (list_pos = g_devices; list_pos; list_pos = list_pos->next)
	{
		int ret = disconnect_client_from_device (client,
							 list_pos->data);
		if (ret < 0) // disconnect failed
		{
			if (errno != ENODEV) // ignore ENODEV
			{
				return (-1);
			}
		}
	}
	return 0;
}

int remove_all_clients_from_device (device_t *device)
{
	client_list_t *list_pos = device->clients_connected;
	device->clients_connected = NULL;
	while(list_pos)
	{
		client_list_t *next_pos = list_pos->next;

		if (rm_client (&list_pos, list_pos->data) < 0)
			return (-1);
		list_pos = next_pos;
	}
	return 0;
}
		
int remove_all_devices_from_client (client_t *client)
{
	device_list_t *list_pos = client->devices_connected;
	client->devices_connected = NULL;
	while(list_pos)
	{
		device_list_t *next_pos = list_pos->next;

		if (rm_device (&list_pos, list_pos->data) < 0)
			return (-1);
		list_pos = next_pos;
	}
	return 0;
}
		
int connect_client_to_device (client_t *client, device_t *device)
{
	/* connect the client to the specified device.  If the client
	   is already connected, ignore the request.  If clients_connected
	   was empty before, call link_up. */

	if (device->clients_connected == NULL)
	{
		device->retries = g_retries;
		if (alter_device_status (device, LINK_CONNECTING) < 0)
			return (-1);
	}

	if (add_client (&device->clients_connected, client) < 0)
	{
		if (errno != EALREADY) /* ignore this one */
			return (-1);
	}

	if (add_device (&client->devices_connected, device) < 0)
	{
		if (errno != EALREADY) /* ignore this one */
			return (-1);
	}

	return 0;
}

int disconnect_client_from_device (client_t *client, device_t *device)
{
	/* disconnect a client from the device.  If the client is not
	   connected, ignore the request.  If doing this causes
	   clients_connected to become empty, bring down the link. */

	if (rm_client (&device->clients_connected, client) < 0)
	{
		if (errno != ENODEV)
			return (-1);
	}
	else
	{
		if (device->clients_connected == NULL)
		{
			if (alter_device_status (device,
						 LINK_DISCONNECTING) < 0)
				return (-1);
		}
	}

	if (rm_device (&client->devices_connected, device) < 0)
	{
		if (errno != ENODEV)
			return (-1);
	}

	return 0;
}

int link_up (device_t *device)
{
	int retval;

	if (device == NULL)
		return -1; // sanity check
	
	retval = system (device->link_up_command);
	if (retval == 127)
	{
		fprintf (stderr, "link_up(): failed to execve %s\n",
			 device->link_up_command);
		retval = -1;
	}

	if (retval == 0)
	{
		device->connect_time = time (NULL);
	}
	return retval;
}

int link_down (device_t *device)
{
	int retval;

	if (device == NULL)
		return -1; // sanity check
	
	retval = system (device->link_down_command);
	if (retval == 127)
	{
		fprintf (stderr, "link_down(): failed to execve %s\n",
			 device->link_down_command);
		retval = -1;
	}

	if (retval == 0)
	{
		device->connect_time = time (NULL);
	}
	return retval;
}

int link_force_down (device_t *device)
{
	int retval;

	if (device == NULL)
		return -1; // sanity check
	
	retval = system (device->link_force_down_command);
	if (retval == 127)
	{
		fprintf (stderr, "link_down(): failed to execve %s\n",
			 device->link_force_down_command);
		retval = -1;
	}

	if (retval == 0)
	{
		if (remove_all_clients_from_device (device) < 0)
			return (-1);
		if (remove_device_from_all_clients (device) < 0)
			return (-1);
	}
	return retval;
}

int alter_device_status (device_t *device, device_status_t new_status)
{
	/* 16 possible states.  We should examine all of them and choose
	   the appropriate action.  Summarised below:

	*/
	if (g_debug)
		fprintf (stderr, "Transition from %s to %s:  ",
			 g_link_status_message[device->status],
			 g_link_status_message[new_status]);

	switch (new_status)
	{
	case LINK_CONNECTING:
		switch (device->status)
		{
		case LINK_DISCONNECTING:
			if (link_force_down (device) < 0)
				return (-1);
			sleep (2); // give the kill command a chance to work
		case LINK_DOWN:
			device->retries = g_retries;
		case LINK_CONNECTING:
			if (device->retries-- < 0)
				return alter_device_status (device, LINK_DOWN);
			if (link_up (device) < 0)
				return (-1);
			device->connect_time = time (NULL);
			break;
		case LINK_UP:
			fprintf (stderr, "Invalid\n");
			errno = EINVAL;
			return (-1);
		default:
			fprintf (stderr, "Unknown existing state (%d)\n",
				 device->status);
			errno = EINVAL;
			return (-1);
		}
		break;
	case LINK_DISCONNECTING:
		switch (device->status)
		{
		case LINK_CONNECTING:
			if (link_force_down (device) < 0)
				return (-1);
			break;
		case LINK_DISCONNECTING:
			/* This is for timing out disconnection operations */
			if (link_force_down (device) < 0)
				return (-1);
			new_status = LINK_DOWN;
			break;
		case LINK_UP:
			if (link_down (device) < 0)
				return (-1);
			break;
		case LINK_DOWN:
			if (g_debug)
				fprintf (stderr, "Invalid\n");
			errno = EINVAL;
			return (-1);
		default:
			if (g_debug)
				fprintf (stderr,
					 "Unknown existing state (%d)\n",
					 device->status);
			errno = EINVAL;
			return (-1);
		}
		break;
	case LINK_UP:
		switch (device->status)
		{
		case LINK_UP:
		case LINK_DISCONNECTING:
		case LINK_DOWN:
			if (g_debug)
				fprintf (stderr, "Invalid\n");
			errno = EINVAL;
			return (-1);
		case LINK_CONNECTING:
			device->connect_time = time (NULL);
			break;
		default:
			if (g_debug)
				fprintf (stderr,
					 "Unknown existing state (%d)\n",
					 device->status);
			errno = EINVAL;
			return (-1);
		}
		break;
	case LINK_DOWN:
		switch (device->status)
		{
		case LINK_DISCONNECTING:
			break;
		case LINK_CONNECTING:
		case LINK_UP:
		case LINK_DOWN:
			if (link_force_down (device) < 0)
				return (-1);
			break;
		default:
			if (g_debug)
				fprintf (stderr,
					 "Unknown existing state (%d)\n",
					 device->status);
			errno = EINVAL;
			return (-1);
		}
		break;
	default:
		if (g_debug)
			fprintf (stderr,
				 "Unknown existing state (%d)\n",
				 device->status);
		errno = EINVAL;
		return (-1);
	}
	
	/* If we get this far, the transition must have succeeded */
	if (g_debug)
		fprintf (stderr, "OK\n");

	device->status = new_status;
	return 0;
}
