/**
 * cliserv.h
 * ---------
 *
 * Stuff that's common to both the client and the server
 */

#ifndef _CLISERV_H_
#define _CLISERV_H_

/* bunch of includes that mostly everything needs */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_SEND_BUFFER 400
#define MAX_RECV_BUFFER 400

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif 

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* structures */
typedef enum _device_status_t
{
	LINK_DOWN,
	LINK_UP,
	LINK_CONNECTING,
	LINK_DISCONNECTING
} device_status_t;

typedef struct _device_list_t
{
	struct _device_list_t *next;
	struct _device_t      *data;
} device_list_t;

#endif // _CLISERV_H_
