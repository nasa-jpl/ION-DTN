/*
	tcpclv4_tls.h:	TCPCLv4 TLS session management library.

			Handles TCP socket, TLS handshake, contact header
			exchange, and message framing over TLS. Removes
			Python GIL from data path for performance.

	Author: iondev33

	Copyright (c) 2026, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
								*/

#ifndef _TCPCLV4_TLS_H_
#define _TCPCLV4_TLS_H_

#include "platform.h"

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*	Opaque session handle						*/

typedef struct tcpclv4_tls_session_s tcpclv4_tls_session_t;

/*	TLS Configuration						*/

typedef struct
{
	const char	*tls_cert_file;		/* Path to cert PEM */
	const char	*tls_key_file;		/* Path to private key PEM */
	const char	*tls_ca_file;		/* Path to CA bundle */
	int		tls_verify_peer;	/* 1 = verify, 0 = no verify */
} tcpclv4_tls_config_t;

/*	==============================================================
 *	SESSION ESTABLISHMENT
 *	==============================================================
 */

/*	Create TLS session in client mode (active open).
 *
 *	Steps (RFC 9174 §4.1 ordering):
 *	  1. Create TCP socket, connect to host:port
 *	  2. Exchange contact headers in plaintext, offering
 *	     CAN_TLS iff tls_config is non-NULL
 *	  3. Perform TLS handshake, only if both sides set CAN_TLS
 *	     (if tls_config is given but the peer cannot do TLS,
 *	     establishment fails: configured TLS is required TLS)
 *	  4. Return established session (caller then exchanges
 *	     SESS_INIT over it)
 *
 *	:param host: Hostname or IP address
 *	:param port: TCP port number
 *	:param local_node_id: Our node ID (e.g., "ipn:2.0")
 *	:param tls_config: TLS certificate configuration (NULL = no TLS)
 *	:return: Session handle on success, NULL on error
 */
tcpclv4_tls_session_t* tcpclv4_tls_connect(
	const char *host, int port,
	const char *local_node_id,
	const tcpclv4_tls_config_t *tls_config);

/*	Create TLS session in server mode (passive open).
 *
 *	Steps (RFC 9174 §4.1 ordering):
 *	  1. Take connected socket from caller's accept()
 *	  2. Exchange contact headers in plaintext
 *	  3. Perform TLS handshake if both sides set CAN_TLS
 *	  4. Return established session
 *
 *	:param client_fd: Connected socket from accept()
 *	:param local_node_id: Our node ID
 *	:param tls_config: TLS certificate configuration (NULL = no TLS)
 *	:return: Session handle on success, NULL on error
 */
tcpclv4_tls_session_t* tcpclv4_tls_accept(
	int client_fd,
	const char *local_node_id,
	const tcpclv4_tls_config_t *tls_config);

/*	==============================================================
 *	MESSAGE I/O
 *	==============================================================
 */

/*	Send a complete TCPCLv4 message on a blocking session.
 *
 *	Writes all of msg_buf, retrying partial writes.  On a
 *	non-blocking session use tcpclv4_tls_write instead.
 *
 *	:param sess: Session handle
 *	:param msg_buf: Message bytes to send
 *	:param msg_len: Length of message
 *	:return: Number of bytes sent on success, -1 on error
 */
int tcpclv4_tls_send_msg(tcpclv4_tls_session_t *sess,
			  const uint8_t *msg_buf, size_t msg_len);

/*	Receive one complete TCPCLv4 message.
 *
 *	Accumulates stream bytes internally and returns exactly one
 *	complete message per successful call, framed per RFC 9174
 *	message layouts.  A single socket read may buffer several
 *	messages, so callers must invoke this repeatedly until it
 *	returns 0 before polling the socket again (see also
 *	tcpclv4_tls_pending()).
 *
 *	:param sess: Session handle
 *	:param buf: Output buffer
 *	:param buf_cap: Buffer capacity
 *	:param out_len: Actual bytes read (output)
 *	:return: 1 on success, 0 if would block, -1 on error/closed
 */
int tcpclv4_tls_recv_msg(tcpclv4_tls_session_t *sess,
			  uint8_t *buf, size_t buf_cap, size_t *out_len);

/*	Non-blocking raw write.  Writes as many bytes as the transport
 *	will take without blocking (the socket should be in
 *	non-blocking mode; see tcpclv4_tls_set_nonblocking).
 *
 *	:param sess: Session handle
 *	:param buf: Bytes to write
 *	:param len: Number of bytes offered
 *	:return: Bytes written (>= 0; 0 = would block), -1 on error
 */
int tcpclv4_tls_write(tcpclv4_tls_session_t *sess,
		const uint8_t *buf, size_t len);

/*	Check whether a complete message is already buffered (or TLS
 *	has decrypted bytes pending) so that recv_msg would succeed
 *	without waiting for socket readability.
 *
 *	:param sess: Session handle
 *	:return: 1 if a recv_msg call should be made now, 0 otherwise
 */
int tcpclv4_tls_pending(tcpclv4_tls_session_t *sess);

/*	Set the maximum acceptable single-message size, used as a
 *	framing sanity limit (e.g., after SESS_INIT negotiation, set
 *	to segment MRU plus header overhead).  Default is 64MB.
 *
 *	:param sess: Session handle
 *	:param max_msg_len: Limit in bytes (0 = unlimited)
 *	:return: 0 on success, -1 on error
 */
int tcpclv4_tls_set_max_msg_len(tcpclv4_tls_session_t *sess,
		uint64_t max_msg_len);

/*	==============================================================
 *	SESSION CONTROL
 *	==============================================================
 */

/*	Get socket file descriptor for select/poll/epoll integration.
 *
 *	:param sess: Session handle
 *	:return: Socket FD, or -1 if session is invalid
 */
int tcpclv4_tls_get_fd(tcpclv4_tls_session_t *sess);

/*	Set socket to non-blocking mode.
 *
 *	In non-blocking mode, send_msg/recv_msg return 0 if they would block
 *	instead of blocking the caller.
 *
 *	:param sess: Session handle
 *	:param nonblock: 1 = non-blocking, 0 = blocking
 *	:return: 0 on success, -1 on error
 */
int tcpclv4_tls_set_nonblocking(tcpclv4_tls_session_t *sess, int nonblock);

/*	Get peer node ID from contact header exchange.
 *
 *	:param sess: Session handle
 *	:return: Peer node ID string (owned by session, do not free),
 *	         or NULL if not available
 */
const char* tcpclv4_tls_get_peer_nodeid(tcpclv4_tls_session_t *sess);

/*	Check if TLS is enabled for this session.
 *
 *	:param sess: Session handle
 *	:return: 1 if TLS enabled, 0 if plain TCP
 */
int tcpclv4_tls_is_secure(tcpclv4_tls_session_t *sess);

/*	==============================================================
 *	SESSION TEARDOWN
 *	==============================================================
 */

/*	Close the TLS transport and free the session.
 *
 *	This is the transport-level teardown; it does NOT send a
 *	SESS_TERM.  SESS_TERM is a session-layer message: the caller
 *	sends it with tcpclv4_session_terminate() before closing the
 *	transport.  Steps here:
 *	  1. SSL_shutdown (if TLS enabled)
 *	  2. close(fd)
 *	  3. Free session memory
 *
 *	:param sess: Session handle (may be NULL)
 */
void tcpclv4_tls_close(tcpclv4_tls_session_t *sess);

/*	==============================================================
 *	ERROR HANDLING
 *	==============================================================
 */

/*	Get last error message for this session.
 *
 *	:param sess: Session handle
 *	:return: Error string (owned by session, do not free),
 *	         or NULL if no error
 */
const char* tcpclv4_tls_get_error(tcpclv4_tls_session_t *sess);

/*	Clear error state.
 *
 *	:param sess: Session handle
 */
void tcpclv4_tls_clear_error(tcpclv4_tls_session_t *sess);

/*	==============================================================
 *	UTILITY
 *	==============================================================
 */

/*	Initialize OpenSSL library.
 *
 *	Must be called once before any TLS operations. Safe to call
 *	multiple times (only first call has effect).
 */
void tcpclv4_tls_init(void);

/*	Cleanup OpenSSL library.
 *
 *	Call at program exit. Safe to call multiple times.
 */
void tcpclv4_tls_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif  /* _TCPCLV4_TLS_H_ */
