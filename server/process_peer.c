/* process_peer.c
 * ----------------
 *
 * This source contains all the functions to process commands from the
 * notification peer to the server.
 */

#include <errno.h>
#include <string.h>

#include <cliserv.h>
#include <protocol.h>
#include "server.h"

int process_peer (char *message)
{
	if (strncmp (message, NOTIFY_ISUP, strlen (NOTIFY_ISUP)) == 0)
	{
		char *dev_str = message + strlen (NOTIFY_ISUP);
		device_t *device = get_device (&g_devices, dev_str);
		if (device == NULL)
		{
			return (-1);
		}
		
		alter_device_status (device, LINK_UP);
		return 0;
	}
	else if (strncmp (message, NOTIFY_ISDOWN, strlen (NOTIFY_ISDOWN)) == 0)
	{
		char *dev_str = message + strlen (NOTIFY_ISDOWN);
		device_t *device = get_device (&g_devices, dev_str);
		if (device == NULL)
		{
			return (-1);
		}
		
		return alter_device_status (device, LINK_DOWN);
	}
	else
	{
		errno = ENOTSUP;
		return (-1);
	}
}
