/* notify.h
 * --------
 */

#define DEFAULT_SERV_ADDR   "192.168.55.103"
#define DEFAULT_SERV_PORT   9876
#define DEFAULT_CONFIG_FILE "/etc/link_peer.conf"

/* enums */
typedef enum {UP, DOWN} command_t;

/* Prototypes */
int parse_command_line (int argc, char *argv[]);
int read_config        ();

/* Global variables */
extern char           *g_serv_addr;
extern unsigned short  g_serv_port;
extern int             g_debug;
extern command_t       g_command;
extern char *	       g_device;
