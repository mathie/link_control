/* client.h
 * --------
 *
 * Globals, structures and protoypes for the linux client
 */

#ifndef _LINK_CLIENT_H_
#define _LINK_CLIENT_H_

#include <time.h>
#include <pthread.h>

#include <cliserv.h>

#define DEFAULT_CONFIG_FILE     "./link_client.conf"
#define DEFAULT_MULTICAST_GROUP "239.255.42.42"
#define DEFAULT_MULTICAST_PORT  6789
#define DEFAULT_SERVER_TIMEOUT  60 /* seconds */

/* Structures */

typedef struct _device_t
{
	char                  *device_name;
	char                  *device_description;
	enum _device_status_t  status;
	time_t                 connect_time;
	unsigned int           no_users;
} device_t;

typedef struct _server_t
{
	struct sockaddr_in     sa;
	time_t                 last_heard_from;
	struct _device_list_t *devices_controlled;
} server_t;

typedef struct _server_list_t
{
	struct _server_list_t *next;
	struct _server_t       *data;
} server_list_t;

/* global variables */
extern int             g_socket_fd;
extern pthread_mutex_t g_socket_fd_mutex;
extern server_list_t  *g_servers;
extern pthread_mutex_t g_servers_mutex;
extern pthread_cond_t  g_servers_cond;
extern char           *g_multicast_group;
extern unsigned short  g_multicast_port;
extern int             g_server_timeout;
extern int             g_debug;
extern const char     *g_link_status_message[];

/* Function prototypes */
/* from read_config.c */
int parse_command_line (int argc, char *argv[]);
int read_config        ();

/* from list_fns.c */
int       add_server (server_list_t **pp_servers, server_t *new_server);
int       rm_server  (server_list_t **pp_servers, server_t *new_server);
server_t *get_server (server_list_t **pp_servers, struct in_addr inet_addr);
int       add_device (device_list_t **pp_devices, device_t *device);
int       rm_device  (device_list_t **pp_devices, device_t *device);
device_t *get_device (device_list_t **pp_devices, char *device_name);

#ifdef DEBUG
void dump_server_list (server_list_t **pp_servers);
void dump_device_list (device_list_t **pp_devices);
#endif // DEBUG

/* from listen_status.c */
void *listen_for_status (void *arg);

#endif // _CLIENT_H_
