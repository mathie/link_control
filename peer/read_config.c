/**
 * Read_config.c
 * -------------
 *
 * reads in the configuration for the peer and stores it into the
 * appropriate peer variables.
 */

/* Format of the configuration file
 * --------------------------------
 *
 * The format is (kind of) stolen from windows ini files and from samba's
 * configuration file.  First of all, we have a peer section, which is 
 * laid out in the following manner:
 *
 * [Peer]
 * name = value
 * name = "String value"
 *
 * The following options are currently supported in the peer section:
 *
 * Name          | Type      | Default value
 * --------------+-----------+--------------
 * debug         | number    | 0 (false)
 * serv_addr     | string    | "192.168.55.103" (my machine!)
 * serv_port     | number    | 9876
 *
 * Currently, escaped characters are not supported, but support may be
 * added later...  Tabs and newlines are not accepted in strings.  IP
 * addresses may (currently) only be done numerically.
 *
 * WARNING: parse_peer_section() and parse_device_section are broken in that
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
#include <stdio.h>
#include <stdlib.h>

#include <cliserv.h>
#include "notify.h"

/* Peer variables */
int             g_debug          = FALSE;
char           *g_config_file    = DEFAULT_CONFIG_FILE;
char           *g_serv_addr      = DEFAULT_SERV_ADDR;
unsigned short  g_serv_port      = DEFAULT_SERV_PORT;
command_t       g_command;
char *		g_device	 = NULL;

/* File-level variables */
static int   s_config_fd       = -1;
static char *s_config_data     = NULL;
static int   s_config_data_len = 0;

/* Local prototypes */
char *open_config_file        (void);
int   close_config_file       (void);
char *get_peer_section      (char *config_data);
int   parse_peer_section    (char *peer_data);
int   parse_line              (char *line, char **name, char **value);
int   modify_peer_conf      (char *name, char *value);


int parse_command_line (int argc, char *argv[])
{
	int i, command_set = FALSE;

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
			case 'U': // link is up
				if (!command_set)
				{
					command_set = TRUE;
					g_command = UP;
				}
				else
				{
					fprintf (stderr, "You can only specify one of -U or -D\n");
					errno = EINVAL;
					return (-1);
				}
				break;
			case 'D': // link is down
				if (!command_set)
				{
					command_set = TRUE;
					g_command = DOWN;
				}
				else
				{
					fprintf (stderr, "You can only specify one of -U or -D\n");
					errno = EINVAL;
					return (-1);
				}
				break;
			default:
				fprintf (stderr, "Unrecognised switch: %s\n",
					 --cur_arg);
				break;
			}
		}
		else
		{
			/* The only option that doesn't have a switch that
			   will reach here is the device name. */
			g_device = strdup (cur_arg);
		}
	}
	if (!command_set)
	{
		fprintf(stderr, "You must specify one of -U or -D to specify"
			"that the link is up or down respectively.\n");
		errno = EINVAL;
		return (-1);
	}
	return 0;
}

int read_config ()
{
	char *config, *peer_section;
	if ((config = open_config_file()) == NULL)
	{
		if (errno == ENOENT) // config file doesn't exist, use defaults
			return 0;
		else
			return (-1);
	}

	if ((peer_section = get_peer_section (config)) == NULL)
	{
		if (errno != ENOMSG) // no peer section - use defaults
		{
			int real_errno = errno;
			close_config_file ();
			errno = real_errno;
			return (-1);
		}
	}
	else
	{
		if (parse_peer_section (peer_section) < 0)
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

char *get_peer_section (char *config_data)
{
	/* search the config_data for a section beginning "[Peer]"
	   (case sensitive).  Return a string with the block of data
	   inside the peer section. */

	/* first, find the start position. */
	char *start, *end, *ret_val = NULL;

	start = strstr (config_data, "[Peer]");
	if (start == NULL)
	{
		errno = ENOMSG;
		return NULL;
	}

	end = strchr (start + 1, '[');
	if (end == NULL) // The peer section is at the end of the file
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

int parse_peer_section (char *peer_data)
{
	char *name, *value, *section_name, *pos, *prev_pos, *line;
	/* find the section name */
	pos = strpbrk (peer_data, "\n\0");
	prev_pos = pos + 1;
	section_name = (char *)malloc (pos - peer_data + 1);
	strncpy (section_name, peer_data, pos - peer_data);
	section_name[pos - peer_data] = '\0';

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
			
			if ((strcasecmp (name, "debug") == 0) &&
				 number_valid)
				g_debug = numeric_value;
			else if (strcasecmp (name, "serv_addr") == 0)
				g_serv_addr = strdup(value);
			else if ((strcasecmp (name, "serv_port") == 0) &&
				 number_valid)
				g_serv_port = numeric_value;
			else
				fprintf(stderr,
					"Invalid peer option %s\n", name);
		}

		prev_pos = pos + 1;
		free (line);
	}

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
