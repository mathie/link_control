/**
 * list_fns.c
 * ----------
 *
 * ancillary functions to manage the lists associated with the link server.
 * we have a global list of devices and a global list of clients.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <cliserv.h>
#include "client.h"

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

#ifdef DEBUG
void dump_device_list (device_list_t **pp_devices)
{
	device_list_t *list_pos = *pp_devices;
	int i = 0;
	printf ("Displaying device status:\n");
	while (list_pos)
	{
		switch (list_pos->data->status)
		{
		case LINK_UP:
			printf ("Device %d:\t%s\t\t%s (%d)\n",
				i++,
				list_pos->data->device_name,
				g_link_status_message[list_pos->data->status],
				(int)list_pos->data->connect_time);
			break;
		default:
			printf ("Device %d:\t%s\t\t%s\n",
				i++,
				list_pos->data->device_name,
				g_link_status_message[list_pos->data->status]);
			break;
		}
		list_pos = list_pos->next;
	}
}
#endif // DEBUG

int add_server (server_list_t **pp_servers, server_t *new_server)
{
	server_list_t *new_server_list_entry;
	
	// first check to see if it's already in the list
	if (get_server (pp_servers, new_server->sa.sin_addr) != NULL)
	{
		errno = EALREADY;
		return (-1);
	}

	new_server_list_entry = (server_list_t *)malloc (
		sizeof (server_list_t));
	if (new_server_list_entry == NULL) // malloc failed
	{
		return (-1);
	}
	new_server_list_entry->next = NULL;
	new_server_list_entry->data = new_server;

	if (*pp_servers == NULL) // no servers
	{
		*pp_servers = new_server_list_entry;
	}
	else
	{
		server_list_t *list_pos = *pp_servers;
		while (list_pos->next)
		{
			// sanity check - the server is in the list
			if (list_pos->data == new_server)
			{
				free (new_server_list_entry);
				errno = EALREADY; /* already there */
				return (-1);
			}
			list_pos = list_pos->next;
		}
		list_pos->next = new_server_list_entry;
	}
	return 0;
}

int rm_server (server_list_t **pp_servers, server_t *server)
{
	server_list_t *prev_pos, *list_pos;

	// sanity check
	if (*pp_servers == NULL)
	{
		errno = ENODEV; /* no such server (?) */
		return (-1);
	}

        // is it the root node ?
	if ((*pp_servers)->data == server)
	{
		free (*pp_servers);
		*pp_servers = NULL;
		return 0;
	}

	prev_pos = *pp_servers;
	list_pos = (*pp_servers)->next;
	while (list_pos)
	{
		if (list_pos->data == server)
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

server_t *get_server (server_list_t **p_servers, struct in_addr inet_addr)
{
	server_list_t *list_pos = *p_servers;

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

int update_server (server_t *server)
{
	server->last_heard_from = time (NULL);
	return 0;
}

int timeout_old_servers ()
{
	/* any server who's last_heard_from time is more than g_server_timeout
	   seconds old is removed from existance.  This is a failsafe in case
	   the server dies */

	server_list_t *list_pos = g_servers, *next_pos;
	server_t *server;

	while (list_pos)
	{
		next_pos = list_pos->next;
		server   = list_pos->data;
		if (time (NULL) > (server->last_heard_from + g_server_timeout))
		{
			if (rm_server (&g_servers, server) < 0)
				return (-1);
			free (server);
		}
		list_pos = next_pos;
	}
	return 0;
}

#ifdef DEBUG
void dump_server_list (server_list_t **pp_servers)
{
	int i = 0;
	server_list_t *list_pos = *pp_servers;

	printf ("Dumping server list:\n");
	while (list_pos)
	{
		device_list_t *d_list_pos;

		printf ("\tServer %d:\t%s\t%d\t",
			i++,
			inet_ntoa(list_pos->data->sa.sin_addr),
			(int)list_pos->data->last_heard_from);
		d_list_pos = list_pos->data->devices_controlled;
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
