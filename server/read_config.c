/**
 * read_config.c
 * -------------
 *
 * reads in the configuration for the link-server and stores it into the
 * appropriate server variables.
 */

/* Format of the configuration file
 * --------------------------------
 *
 * The format is (kind of) stolen from windows ini files and from samba's
 * configuration file.  First of all, we have a server section, which is 
 * laid out in the following manner:
 *
 * [Server]
 * name = value
 * name = "String value"
 *
 * The following options are currently supported in the server section:
 *
 * Name               | Type      | Default value
 * -------------------+-----------+--------------
 * fork               | number    | 1 (true)
 * debug              | number    | 0 (false)
 * poll_time          | number    | 5 (seconds)
 * srv_addr           | string    | "0.0.0.0" (listen on all interfaces)
 * srv_port           | number    | 9876
 * multicast_group    | string    | "239.255.42.42" (site-local admin group)
 * multicast_port     | number    | 6789
 * client_timeout     | number    | 7200 (seconds - 2 hours)
 * retries            | number    | 3
 * connect_timeout    | number    | 60
 * disconnect_timeout | number    | 60
 *
 * The remainder of the configuration file specifies devices.  It takes the
 * form:
 *
 * [Device]
 * name = value
 * name = "string value"
 *
 * The following options are currently supported in the devices section:
 *
 * Name            | Type      | Default value
 * ----------------+-----------+--------------
 * name            | string    | "" (if not specified, result in an error)
 * description     | string    | ""
 * link_up         | string    | "" (command to activate the link)
 * link_down       | string    | "" (command to deactivate the link)
 * link_force_down | string    | "" (command to force the link to die)
 *
 * Currently, escaped characters are not supported, but support may be
 * added later...  Tabs and newlines are not accepted in strings.  IP
 * addresses may (currently) only be done numerically.
 *
 * WARNING: parse_server_section() and parse_device_section are broken in that
 *          they will not accept the last line if there is not a carriage
 *          return at the end of the file.  I don't see this as a worry, since
 *          certain programs which parse fstab have exactly the same issue...
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "server.h"

/* Server variables */
int            g_fork               = TRUE;
int            g_debug              = FALSE;
int            g_poll_time          = DEFAULT_POLL_TIME;
char          *g_config_file        = DEFAULT_CONFIG_FILE;
char          *g_srv_inaddr         = NULL;
unsigned short g_srv_port           = DEFAULT_SRV_PORT;
char          *g_multicast_group    = DEFAULT_MULTICAST_GROUP;
unsigned short g_multicast_port     = DEFAULT_MULTICAST_PORT;
int            g_client_timeout     = DEFAULT_CLIENT_TIMEOUT;
int            g_retries            = DEFAULT_RETRIES;
int            g_connect_timeout    = DEFAULT_CONNECT_TIMEOUT;
int            g_disconnect_timeout = DEFAULT_DISCONNECT_TIMEOUT;

/* File-level variables */
static int   s_config_fd       = -1;
static char *s_config_data     = NULL;
static int   s_config_data_len = 0;

/* Local prototypes */
char *open_config_file        (void);
int   close_config_file       (void);
char *get_server_section      (char *config_data);
char *get_next_device_section (char *config_data);
int   parse_server_section    (char *server_data);
int   parse_device_section    (char *device_data);
int   parse_line              (char *line, char **name, char **value);
int   modify_server_conf      (char *name, char *value);

int parse_command_line (int argc, char *argv[])
{
	int i;

	if (argc == 1) /* no arguments */
		return 0;

	for (i = 1; i < argc; i++)
	{
		/* search for switches */
		char *cur_arg = argv[i];
		if (*cur_arg == '-')
		{
			switch (*(++cur_arg))
			{
			case 'f': // don't fork
				g_fork = FALSE;
				break;
			case 'd': // debug
				g_debug = TRUE;
				break;
			case 'c': // config file
				if (++i > argc)
				{
					fprintf(stderr, "You didn't specify the name of the configuration file.\n");
				}
				else
				{
					g_config_file = strdup (argv[i]);
				}
				break;
			case 'p': // poll time
				if (++i > argc)
				{
					fprintf(stderr, "You didn't specify the poll time.\n");
				}
				else
				{
					g_poll_time = atoi(argv[i]);
				}
			default:
				fprintf (stderr, "Unrecognised switch: %s\n",
					 --cur_arg);
				break;
			}
		}
		else
		{
			fprintf (stderr, "Unrecognised option: %s\n", cur_arg);
		}
	}
	return 0;
}

int read_config ()
{
	char *config, *server_section, *device_section;
	if ((config = open_config_file()) == NULL)
	{
		if (errno == ENOENT) // config file doesn't exist, use defaults
			return 0;
		else
			return (-1);
	}

	if ((server_section = get_server_section (config)) == NULL)
	{
		if (errno != ENOMSG) // no server section - use defaults
		{
			int real_errno = errno;
			close_config_file ();
			errno = real_errno;
			return (-1);
		}
	}
	else
	{
		if (parse_server_section (server_section) < 0)
		{
			int real_errno = errno;
			close_config_file ();
			errno = real_errno;
			return (-1);
		}
	}
	
	while ((device_section = get_next_device_section (config))
	       != NULL)
	{
		if (parse_device_section (device_section) < 0)
		{
			int real_errno = errno;
			close_config_file ();
			errno = real_errno;
			return (-1);
		}
	}

	if (close_config_file () < 0)
		return (-1);
	return 0;
}

char *open_config_file ()
{
	struct stat config_stat;
       
	// sanity check
	if (g_config_file == NULL)
	{
		errno = ENOENT;
		return NULL;
	}

	if (s_config_fd != -1) // already open
	{
		errno = EBADFD;
		return NULL;
	}

	if ((s_config_fd = open (g_config_file, O_RDONLY)) < 0)
		return NULL;
	if (fstat (s_config_fd, &config_stat) < 0)
		return NULL;
	s_config_data_len = config_stat.st_size;

	if ((s_config_data = (char *)mmap (0, s_config_data_len, PROT_READ,
					 MAP_SHARED, s_config_fd, 0))
	    == MAP_FAILED)
		return NULL;
	return s_config_data;
}

int close_config_file ()
{
	if (s_config_fd == -1)
	{
		errno = EBADFD;
		return (-1);
	}

	if (munmap (s_config_data, s_config_data_len) < 0)
		return (-1);

	if (close (s_config_fd) < 0)
		return (-1);

	return 0;
}

char *get_server_section (char *config_data)
{
	/* search the config_data for a section beginning "[Server]"
	   (case sensitive).  Return a string with the block of data
	   inside the server section. */

	/* first, find the start position. */
	char *start, *end, *ret_val = NULL;

	start = strstr (config_data, "[Server]");
	if (start == NULL)
	{
		errno = ENOMSG;
		return NULL;
	}

	end = strchr (start + 1, '[');
	if (end == NULL) // The server section is at the end of the file
	{
		ret_val = (char *)malloc (strlen (start) + 1);
		strcpy (ret_val, start);
	}
	else
	{
		ret_val = (char *)malloc (strlen (start) - strlen (end) + 1);
		strncpy (ret_val, start, strlen (start) - strlen (end));
	}
	return ret_val;
}
char *get_next_device_section (char *config_data)
{
	/* search the config_data for a section beginning "[Device]"
	   (case sensitive).  Return a string with the block of data
	   inside the device section. */

	/* keep track of our current position */
	static int cur_pos = 0;

	/* first, find the start position. */
	char *start, *end, *ret_val = NULL;

	if (cur_pos == -1) // finished
	{
		cur_pos = 0; // ready to start again
		errno = ENOMSG;
		return NULL;
	}

	start = strstr (config_data + cur_pos, "[Device]");
	if (start == NULL)
	{
		cur_pos = 0; // ready to start again
		errno = ENOMSG;
		return NULL;
	}

	end = strchr (start + 1, '[');
	if (end == NULL) // This device section is at the end of the file
	{
		ret_val = (char *)malloc (strlen (start) + 1);
		strcpy (ret_val, start);
		cur_pos = -1; // we've reached the end
	}
	else
	{
		cur_pos = (end - start) + 1;
		ret_val = (char *)malloc (strlen (start) - strlen (end) + 1);
		strncpy (ret_val, start, strlen (start) - strlen (end));
	}
	return ret_val;
}

int parse_server_section (char *server_data)
{
	char *name, *value, *section_name, *pos, *prev_pos, *line;
	/* find the section name */
	pos = strpbrk (server_data, "\n\0");
	prev_pos = pos + 1;
	section_name = (char *)malloc (pos - server_data + 1);
	strncpy (section_name, server_data, pos - server_data);
	section_name[pos - server_data] = '\0';

	while ((pos = strpbrk (prev_pos + 1, "\n\0")) != NULL)
	{
		line = (char *)malloc (pos - prev_pos + 1);
		strncpy (line, prev_pos, pos - prev_pos);
		line[pos-prev_pos] = '\0';
		if (strlen (line) == 0)
		{
			/* empty line */
		}
		else if (parse_line (line, &name, &value) < 0)
		{
			fprintf(stderr,
				"Warning: invalid line in config file:  %s\n",
				line);
		}
		else
		{
			char * end_ptr;
			int numeric_value = strtoul(value, &end_ptr, 0);
			int number_valid = ((*value != '\0') 
					      && (*end_ptr =='\0')) ? 1 : 0;
			
			if ((strcasecmp (name, "fork") == 0) && number_valid)
				g_fork = numeric_value;
			else if ((strcasecmp (name, "debug") == 0) &&
				 number_valid)
				g_debug = numeric_value;
			else if ((strcasecmp (name, "poll_time") == 0) &&
				  number_valid)
				 g_poll_time = numeric_value;
			else if ((strcasecmp (name, "srv_port") == 0) &&
				 number_valid)
				g_srv_port = numeric_value;
			else if ((strcasecmp (name, "srv_inaddr") == 0) &&
				 number_valid)
				g_srv_inaddr = strdup(value);
			else if ((strcasecmp (name, "client_timeout") == 0) &&
				 number_valid)
				g_client_timeout = numeric_value;
			else if (strcasecmp (name, "multicast_group") == 0)
				g_multicast_group = strdup(value);
			else if ((strcasecmp (name, "retries") == 0) &&
				 number_valid)
				g_retries = numeric_value;
			else if ((strcasecmp (name, "connect_timeout") == 0) &&
				 number_valid)
				g_connect_timeout = numeric_value;
			else if ((strcasecmp (name, "disconnect_timeout") == 0)
				 && number_valid)
				g_disconnect_timeout = numeric_value;
			else
				fprintf(stderr,
					"Invalid server option %s\n", name);
		}

		prev_pos = pos + 1;
		free (line);
	}

	return 0;
}

int parse_device_section (char *device_data)
{
	char *name, *value, *section_name, *pos, *prev_pos, *line;
	device_t *new_device = (device_t *)malloc (sizeof (device_t));
	memset (new_device, 0, sizeof (device_t)); // make sure its empty

	if (new_device == NULL)
		return (-1);

	/* find the section name */
	pos = strpbrk (device_data, "\n\0");
	prev_pos = pos + 1;
	section_name = (char *)malloc (pos - device_data + 1);
	strncpy (section_name, device_data, pos - device_data);
	section_name[pos - device_data] = '\0';

	while ((pos = strpbrk (prev_pos + 1, "\n\0")) != NULL)
	{
		line = (char *)malloc (pos - prev_pos + 1);
		strncpy (line, prev_pos, pos - prev_pos);
		line[pos-prev_pos] = '\0';
		if (strlen (line) == 0)
		{
			/* empty line */
		}
		else if (parse_line (line, &name, &value) < 0)
		{
			fprintf(stderr,
				"Warning: invalid line in config file:  %s\n",
				line);
		}
		else
		{
			if (strcasecmp (name, "name") == 0)
				new_device->device_name = strdup (value);
			else if (strcasecmp (name, "description") == 0)
				new_device->device_description = strdup(value);
			else if (strcasecmp (name, "link_up") == 0)
				new_device->link_up_command = strdup (value);
			else if (strcasecmp (name, "link_down") == 0)
				new_device->link_down_command = strdup (value);
			else if (strcasecmp (name, "link_force_down") == 0)
				new_device->link_force_down_command = 
					strdup (value);
			else
				fprintf(stderr, "Unrecognised option %s in "
					"[Device] section.\n", name);
		}

		prev_pos = pos + 1;
		free (line);
		free (name);
		free (value);
	}

	/* thanks to the memset at the start, anything that wasn't filled in
	   has a valid default.  So, unless we have an empty name, add it
	   to the device list */
	if (new_device->device_name != NULL)
		add_device (&g_devices, new_device);
	else
		fprintf(stderr, "Device section has no name.\n");

	return 0;
}

int parse_line (char *line, char **name, char **value)
{
	char *pos, *end_pos;

	/* retrieve the name */
	pos = strpbrk (line, " \t=");
	if (pos == NULL) // can't figure out this line - assume its empty?
		return 0;

	*name = (char *)malloc (pos - line + 1);
	if (*name == NULL)
		return (-1);

	strncpy (*name, line, pos - line);
	(*name)[pos - line] = '\0';

	/* Now search for the value */
	pos = strpbrk (pos, "\"0123456789");
	if (pos == NULL)
	{
		// Invalid value
		return (-1);
	}
	else if (*pos == '\"') // a string
	{
		end_pos = strpbrk (++pos, "\"");
		*value = (char *)malloc (end_pos - pos + 1);
		if (*value == NULL)
			return (-1);

		strncpy (*value, pos, end_pos - pos);
		(*value)[end_pos - pos] = '\0';
	}
	else if (strchr ("0123456789", *pos) != NULL) // a number
	{
		*value = strdup (pos);
		if (*value == NULL)
			return (-1);
	}
	return 0;
}

int modify_server_conf (char *name, char *value)
{
	printf ("%s:\t%s\n", name, value);
	return 0;
}
