/* link-protocol.h
 * ---------------
 *
 * Specifies the various parts of the protocol used in communicating with the
 * various parts of the system.  For documentation on the protocol, see
 * README.protocol in the source distribution.
 */

/* Notification peer */
#define NOTIFY_PREFIX               "NOTIFY "
#define NOTIFY_ISUP                 NOTIFY_PREFIX "ISUP " /* <device> */
#define NOTIFY_ISDOWN               NOTIFY_PREFIX "ISDOWN " /* <device> */

/* Client */
#define CLIENT_PREFIX               "CLIENT "
#define CLIENT_PING		    CLIENT_PREFIX "PING"
#define CLIENT_DEVICES              CLIENT_PREFIX "DEVICES"
#define CLIENT_UP                   CLIENT_PREFIX "UP "  /* <device> */
#define CLIENT_DOWN                 CLIENT_PREFIX "DOWN " /* <device> */
#define CLIENT_FORCE_DOWN           CLIENT_PREFIX "FORCE_DOWN " /* <device> */
#define CLIENT_STATUS               CLIENT_PREFIX "STATUS " /* <device> */
#define CLIENT_CLIENT_STATUS        CLIENT_PREFIX "CLIENT_STATUS"

/* Server */
#define SERVER_PREFIX               "SERVER "
#define SERVER_DEVICES              SERVER_PREFIX "DEVICES " /* ... */
#define SERVER_STATUS_PREFIX        SERVER_PREFIX "STATUS " /* <device> */
#define SERVER_STATUS_UP                          "\tUP " /* <time> <n_user> */
#define SERVER_STATUS_DOWN                        "\tDOWN"
#define SERVER_STATUS_CONNECTING                  "\tCONNECTING"
#define SERVER_STATUS_DISCONNECTING               "\tDISCONNECTING"
#define SERVER_CLIENT_STATUS        SERVER_PREFIX "CLIENT_STATUS " /* ... */

/* Server broadcast messages */
#define BROADCAST_PREFIX            "BROADCAST "
#define BROADCAST_INIT              BROADCAST_PREFIX "INIT"
#define BROADCAST_STATUS            BROADCAST_PREFIX "STATUS " /* ... */
#define BROADCAST_QUIT              BROADCAST_PREFIX "QUIT"
