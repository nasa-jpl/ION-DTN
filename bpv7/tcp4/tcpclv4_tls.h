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
 *	Steps:
 *	  1. Create TCP socket
 *	  2. Connect to host:port
 *	  3. Perform TLS handshake
 *	  4. Send/receive contact headers
 *	  5. Return established session
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
 *	Steps:
 *	  1. Accept connection on listen_fd (caller must call accept())
 *	  2. Perform TLS handshake
 *	  3. Send/receive contact headers
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

/*	Send a TCPCLv4 message over TLS.
 *
 *	Encrypts msg_buf and writes to SSL socket. Handles partial writes
 *	and SSL_ERROR_WANT_WRITE in non-blocking mode.
 *
 *	:param sess: Session handle
 *	:param msg_buf: Message bytes to send
 *	:param msg_len: Length of message
 *	:return: Number of bytes sent on success, -1 on error, 0 if would block
 */
int tcpclv4_tls_send_msg(tcpclv4_tls_session_t *sess,
			  const uint8_t *msg_buf, size_t msg_len);

/*	Receive a TCPCLv4 message from TLS.
 *
 *	Reads and decrypts data from SSL socket. Handles message framing
 *	(reads complete TCPCLv4 messages based on type byte and expected length).
 *
 *	:param sess: Session handle
 *	:param buf: Output buffer
 *	:param buf_cap: Buffer capacity
 *	:param out_len: Actual bytes read (output)
 *	:return: 1 on success, 0 if would block, -1 on error/closed
 */
int tcpclv4_tls_recv_msg(tcpclv4_tls_session_t *sess,
			  uint8_t *buf, size_t buf_cap, size_t *out_len);

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

/*	Close TLS session gracefully.
 *
 *	Steps:
 *	  1. Send SESS_TERM if not already sent
 *	  2. SSL_shutdown (if TLS enabled)
 *	  3. close(fd)
 *	  4. Free session memory
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
