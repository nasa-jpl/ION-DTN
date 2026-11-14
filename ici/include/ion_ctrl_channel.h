/*
	ion_ctrl_channel.h:	Unix domain socket control channel
				for inter-process CLA coordination.

				Carries small TLV-framed control
				messages (slot IDs, transfer IDs,
				disposition codes) in network byte
				order.  Bundle data never traverses
				this channel.

	Author: iondev33

	Copyright (c) 2026, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
								*/

#ifndef ION_CTRL_CHANNEL_H
#define ION_CTRL_CHANNEL_H

#include <stdint.h>
#include <stddef.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/*	Control message types.						*/

#define CTRL_SLOT_READY		0x01	/* ION->Agent: slot has data	*/
#define CTRL_BUNDLE_RECV	0x02	/* Agent->ION: bundle in slot	*/
#define CTRL_XFER_SUCCESS	0x03	/* Agent->ION: transfer OK	*/
#define CTRL_XFER_FAILURE	0x04	/* Agent->ION: transfer failed	*/
#define CTRL_XFER_REFUSED	0x05	/* Agent->ION: transfer refused	*/
#define CTRL_SLOT_RELEASE	0x06	/* ION->Agent: slot released	*/
#define CTRL_SESSION_UP		0x10	/* Agent->ION: session estab.	*/
#define CTRL_SESSION_DOWN	0x11	/* Agent->ION: session ended	*/
#define CTRL_CMD_CONNECT	0x20	/* ION->Agent: connect to peer	*/
#define CTRL_CMD_LISTEN		0x21	/* ION->Agent: start listening	*/
#define CTRL_KEEPALIVE		0xFF	/* Bidirectional keepalive	*/

/*	Control message frame layout (all multi-byte fields in
 *	network byte order):
 *
 *	Type(1) | Reserved(1) | SlotIndex(2) | TransferID(8)
 *	| PayloadLen(2) | Payload(0-256)
 *
 *	Total fixed header: 14 bytes.					*/

#define CTRL_HDR_SIZE		14
#define CTRL_MAX_PAYLOAD	256
#define CTRL_MAX_MSG_SIZE	(CTRL_HDR_SIZE + CTRL_MAX_PAYLOAD)

/*	Parsed control message (host byte order).			*/

typedef struct
{
	uint8_t		type;
	uint16_t	slot_index;
	uint64_t	xfer_id;
	uint16_t	payload_len;
	char		payload[CTRL_MAX_PAYLOAD];
} CtrlMsg;

/*	Control channel handle.						*/

typedef struct
{
	int		server_fd;	/* Listening socket (-1 if N/A)	*/
	int		conn_fd;	/* Connected socket		*/
	char		path[256];	/* Unix socket path		*/
	int		keepalive_interval;	/* Seconds		*/
	int		keepalive_timeout;	/* Seconds		*/
	time_t		last_recv_time;	/* For timeout detection		*/
	time_t		last_send_time;	/* For keepalive scheduling	*/
} CtrlChannel;

/*	Server-side: create a listening Unix domain socket.
 *
 *	path:		Filesystem path for the socket
 *	keepalive_interval: Seconds between keepalive sends
 *	keepalive_timeout:  Seconds before declaring peer dead
 *
 *	Returns 0 on success, -1 on error.				*/

extern int	ctrl_server_create(CtrlChannel *ch, const char *path,
			int keepalive_interval, int keepalive_timeout);

/*	Server-side: accept one client connection.  Blocks until a client
 *	connects or an error occurs, retrying across interrupting signals.
 *
 *	Returns 0 on success, -1 on error.				*/

extern int	ctrl_server_accept(CtrlChannel *ch);

/*	Server-side: wait for one client connection while remaining
 *	interruptible.  Polls the listening socket, rechecks *running
 *	between polls, and retries across interrupting signals, so a
 *	daemon that clears *running during shutdown is not left blocked
 *	in accept().  A NULL running pointer waits indefinitely.
 *
 *	Returns 0 when a client is accepted, 1 when *running became 0
 *	before a client connected, -1 on error.				*/

extern int	ctrl_server_accept_wait(CtrlChannel *ch, volatile int *running);

/*	Client-side: connect to a listening Unix domain socket.
 *
 *	Returns 0 on success, -1 on error.				*/

extern int	ctrl_client_connect(CtrlChannel *ch, const char *path,
			int keepalive_interval, int keepalive_timeout);

/*	Send a control message.  Serializes to network byte order
 *	and writes atomically via writev().
 *
 *	Blocks if the socket buffer is full (intentional
 *	backpressure).
 *
 *	Returns 0 on success, -1 on error.				*/

extern int	ctrl_send(CtrlChannel *ch, const CtrlMsg *msg);

/*	Receive a control message (with timeout).
 *
 *	timeout_ms:  -1 for infinite, 0 for non-blocking,
 *		     >0 for poll timeout in milliseconds.
 *
 *	Returns  1 on message received,
 *		 0 on timeout (no message available),
 *		-1 on error or disconnect.				*/

extern int	ctrl_recv(CtrlChannel *ch, CtrlMsg *msg, int timeout_ms);

/*	Send a keepalive message.
 *
 *	Returns 0 on success, -1 on error.				*/

extern int	ctrl_send_keepalive(CtrlChannel *ch);

/*	Check if the keepalive timeout has elapsed since the
 *	last received message.
 *
 *	Returns 1 if timed out (peer presumed dead),
 *		0 otherwise.						*/

extern int	ctrl_check_timeout(CtrlChannel *ch);

/*	Check if it is time to send a keepalive.
 *
 *	Returns 1 if keepalive_interval has elapsed since
 *	last send, 0 otherwise.					*/

extern int	ctrl_keepalive_due(CtrlChannel *ch);

/*	Close the connection (not the server socket).			*/

extern void	ctrl_disconnect(CtrlChannel *ch);

/*	Destroy the channel: close everything, unlink socket path.	*/

extern void	ctrl_destroy(CtrlChannel *ch);

/*	Convenience: initialize a CtrlMsg from components.		*/

extern void	ctrl_msg_init(CtrlMsg *msg, uint8_t type,
			uint16_t slot_index, uint64_t xfer_id,
			const char *payload, uint16_t payload_len);

#ifdef __cplusplus
}
#endif

#endif /* ION_CTRL_CHANNEL_H */
