/* link-server.h
 * -------------
 *
 * Author:  Graeme Mathieson
 * Date:    10/08/1999
 *
 * I'm separating out all the definitions for the link server into a 
 * separate include file, since I've a feeling its going to spill into
 * multiple files.
 */

#ifndef _LINK_SERVER_H_
#define _LINK_SERVER_H_

#include <time.h>
#include <cliserv.h>

#define DEFAULT_CONFIG_FILE        "/etc/link_server.conf"
#define DEFAULT_SRV_PORT           9876
#define DEFAULT_POLL_TIME          5 /* seconds */
#define DEFAULT_MULTICAST_GROUP    "239.255.42.42" /* site-local admin group */
#define DEFAULT_MULTICAST_PORT     6789
#define DEFAULT_CLIENT_TIMEOUT     (2 * 60 * 20) /* seconds */
#define DEFAULT_RETRIES            2
#define DEFAULT_CONNECT_TIMEOUT    60 /* seconds */
#define DEFAULT_DISCONNECT_TIMEOUT 60 /* seconds */

/* type definitions */
typedef struct 
{
	struct sockaddr_in     sa;
	time_t                 last_heard_from; /*used to age the connection */
	struct _device_list_t *devices_connected;
} client_t;

typedef struct _client_list_t
{
	struct _client_list_t *next;
	client_t              *data;
} client_list_t;

typedef struct _device_t
{
	char            *device_name;
	char            *device_description;
	char            *link_up_command;
	char            *link_down_command;
	char            *link_force_down_command;
	device_status_t  status;
	time_t           connect_time;
	client_list_t   *clients_connected;
	int              retries;
} device_t;

/* global variables */
extern device_list_t *g_devices;
extern client_list_t *g_clients;
extern const char    *g_link_status_message[];
extern char          *g_config_file;
extern int            g_fork;
extern int            g_debug;
extern int            g_poll_time;
extern unsigned short g_srv_port;
extern char          *g_srv_inaddr;
extern char          *g_multicast_group;
extern unsigned short g_multicast_port;
extern int            g_client_timeout;
extern int            g_socket_fd;
extern int            g_retries;
extern int            g_connect_timeout;
extern int            g_disconnect_timeout;

/* exportable function prototypes */
/* from read_config.c */
int       parse_command_line (int argc, char *argv[]);
int       read_config (void);

/* from list_fns.c */
int       add_device (device_list_t **pp_devices, device_t *new_device);
int       add_client (client_list_t **pp_clients, client_t *new_client);
int       rm_device  (device_list_t **pp_devices, device_t *dev);
int       rm_client  (client_list_t **pp_clients, client_t *client);
device_t *get_device (device_list_t **pp_devices, char *dev_name);
client_t *get_client (client_list_t **pp_clients, struct in_addr inet_addr);

int       timeout_old_clients ();
int       timeout_old_devices ();

int       remove_client_from_all_devices (client_t *client);
int       remove_device_from_all_clients (device_t *device);
int       remove_all_devices_from_client (client_t *client);
int       remove_all_clients_from_device (device_t *device);

int       connect_client_to_device      (client_t *client, device_t *device);
int       disconnect_client_from_device (client_t *client, device_t *device);

int       update_client (client_t *client, struct sockaddr_in *cli);

int       link_up             (device_t *device);
int       link_down           (device_t *device);
int       link_force_down     (device_t *device);
int       alter_device_status (device_t *device, device_status_t new_status);
/* from process_client.c */
int process_client (struct sockaddr_in cli, char *recv_buffer);

/* from process_peer.c */
int process_peer   (char *recv_buffer);

/* from send_message.c */
int   send_device_list    (client_t *client);
int   send_device_status  (client_t *client, device_t *device);
int   send_client_status  (client_t *client);
char *print_device_status (device_t *device);

/* from poll_clients.c */
int broadcast_status_message (void);
int broadcast_init_message (void);
int broadcast_quit_message (void);

// functions to aid debugging
#ifdef DEBUG
void      dump_client_list (client_list_t *clients);
void      dump_device_list (device_list_t *devices);
#endif

#endif // _LINK_SERVER_H_
