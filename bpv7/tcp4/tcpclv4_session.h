/*
	tcpclv4_session.h:	TCPCLv4 session state machine (RFC 9174).

			Drives one TCPCL session over a tcpclv4_tls
			transport: SESS_INIT negotiation, keepalives,
			bundle transfer segmentation and reassembly,
			acknowledgment tracking, and termination.

			Single-threaded and event-driven: the owner
			polls the session's file descriptor, calls
			tcpclv4_session_handle_io() when it is readable
			and tcpclv4_session_tick() periodically, and
			receives protocol events through callbacks.

	Author: iondev33

	Copyright (c) 2026, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
								*/

#ifndef _TCPCLV4_SESSION_H_
#define _TCPCLV4_SESSION_H_

#include "tcpclv4_tls.h"
#include "tcpclv4_msg.h"

#ifdef __cplusplus
extern "C" {
#endif

/*	Session states (RFC 9174 §4.1 message sequencing)		*/

typedef enum
{
	TCPCLV4_SESS_NEGOTIATING = 1,	/* SESS_INIT sent, awaiting peer's */
	TCPCLV4_SESS_ESTABLISHED,	/* Both SESS_INITs exchanged	*/
	TCPCLV4_SESS_ENDING,		/* SESS_TERM sent, draining	*/
	TCPCLV4_SESS_TERMINATED		/* Session over; close it	*/
} tcpclv4_session_state_t;

/*	Maximum concurrent outbound (unacknowledged) transfers.	*/

#define TCPCLV4_SESSION_MAX_INFLIGHT	64

/*	Session configuration.  Zero/NULL fields get defaults.		*/

typedef struct
{
	const char	*node_id;	/* Our node ID, e.g. "ipn:2.0" */

	uint16_t	keepalive_interval;	/* Proposed, seconds.
						 * 0 = disable keepalive */
	uint64_t	segment_mru;	/* Max rx segment we accept */
	uint64_t	transfer_mru;	/* Max rx transfer we accept */
	uint64_t	segment_mtu;	/* Max tx segment we send (also
					 * capped by peer's segment MRU) */
	uint64_t	max_tx_transfer;	/* Max outbound bundle size;
					 * bounds the preallocated output
					 * queue.  0 => use transfer_mru. */

	const tcpclv4_tls_config_t *tls;	/* NULL = plain TCP */
} tcpclv4_session_config_t;

/*	Event callbacks.  Any pointer may be NULL to ignore that
 *	event.  Callbacks run synchronously from handle_io/tick.	*/

typedef struct tcpclv4_session_s tcpclv4_session_t;

typedef struct
{
	/*	Session negotiation complete; transfers may begin.	*/

	void	(*on_established)(void *user, tcpclv4_session_t *sess);

	/*	Session is over (peer SESS_TERM, timeout, or error).
	 *	The owner should call tcpclv4_session_close().		*/

	void	(*on_terminated)(void *user, tcpclv4_session_t *sess,
			int reason_code);

	/*	Outbound transfer completed (ok=1: fully acked;
	 *	ok=0: refused or failed).				*/

	void	(*on_xfer_result)(void *user, tcpclv4_session_t *sess,
			uint64_t xfer_id, int ok);

	/*	Complete inbound bundle reassembled.  `data` is valid
	 *	only for the duration of the callback.			*/

	void	(*on_bundle)(void *user, tcpclv4_session_t *sess,
			const uint8_t *data, uint64_t len);
} tcpclv4_session_callbacks_t;

/*	==============================================================
 *	ESTABLISHMENT
 *	==============================================================
 *
 *	Both constructors block through TCP connect / contact header
 *	exchange / TLS handshake, send our SESS_INIT, and return with
 *	the session in NEGOTIATING state.  The peer's SESS_INIT is
 *	consumed by handle_io(), which fires on_established.
 */

tcpclv4_session_t *tcpclv4_session_connect(const char *host, int port,
		const tcpclv4_session_config_t *cfg,
		const tcpclv4_session_callbacks_t *cbs, void *user);

tcpclv4_session_t *tcpclv4_session_accept(int connected_fd,
		const tcpclv4_session_config_t *cfg,
		const tcpclv4_session_callbacks_t *cbs, void *user);

/*	==============================================================
 *	EVENT LOOP INTEGRATION
 *	==============================================================
 */

/*	File descriptor to poll for readability.			*/

int	tcpclv4_session_fd(tcpclv4_session_t *sess);

/*	Process all complete messages currently available on the
 *	transport.  Call when the fd polls readable; also call once
 *	after establishment (TLS may have buffered data).
 *
 *	Returns 0 on success (including state changes), -1 if the
 *	session died (on_terminated has fired; close the session).	*/

int	tcpclv4_session_handle_io(tcpclv4_session_t *sess);

/*	Run timers: send KEEPALIVE if due, detect idle timeout.
 *	Call at least once per second.
 *
 *	Returns 0 on success, -1 if the session died.			*/

int	tcpclv4_session_tick(tcpclv4_session_t *sess);

/*	Sends never block: bytes the socket will not immediately take
 *	are queued inside the session.  The owner's poll loop must:
 *
 *	  - include POLLOUT for the session fd whenever
 *	    tcpclv4_session_wants_write() returns 1, and
 *	  - call tcpclv4_session_handle_output() when the fd polls
 *	    writable, and
 *	  - consult tcpclv4_session_writable() before starting new
 *	    transfers, deferring them while it returns 0
 *	    (backpressure: the queue is over its watermark).
 *
 *	This is what makes simultaneous bidirectional transfer safe:
 *	neither side ever blocks in write(), so both keep reading.	*/

int	tcpclv4_session_wants_write(tcpclv4_session_t *sess);

int	tcpclv4_session_handle_output(tcpclv4_session_t *sess);

int	tcpclv4_session_writable(tcpclv4_session_t *sess);

/*	==============================================================
 *	TRANSFERS
 *	==============================================================
 */

/*	Send one bundle as a TCPCL transfer.  Segments the data per
 *	the negotiated MTU and writes all segments (blocking).  The
 *	transfer completes asynchronously: on_xfer_result fires when
 *	the peer's final XFER_ACK (or XFER_REFUSE) arrives.
 *
 *	xfer_id must be unique among this session's outstanding
 *	transfers (RFC 9174 requires IDs increase monotonically
 *	within a session; the caller owns ID assignment).
 *
 *	Returns 0 if all segments were written, -1 on error (session
 *	not established, too many transfers in flight, transfer
 *	exceeds peer's transfer MRU, or transport failure).		*/

int	tcpclv4_session_send_bundle(tcpclv4_session_t *sess,
		uint64_t xfer_id, const uint8_t *data, uint64_t len);

/*	Number of outbound transfers awaiting final acknowledgment.	*/

int	tcpclv4_session_inflight_count(tcpclv4_session_t *sess);

/*	==============================================================
 *	TERMINATION
 *	==============================================================
 */

/*	Initiate graceful termination: send SESS_TERM and enter
 *	ENDING state.  Outstanding transfers may still be acked by
 *	the peer; the session terminates when the peer replies with
 *	its own SESS_TERM (or the transport closes).

 *	Returns 0 on success, -1 on transport error.			*/

int	tcpclv4_session_terminate(tcpclv4_session_t *sess,
		uint8_t reason_code);

/*	Release all session resources.  Does not send anything.	*/

void	tcpclv4_session_close(tcpclv4_session_t *sess);

/*	==============================================================
 *	INTROSPECTION
 *	==============================================================
 */

tcpclv4_session_state_t	tcpclv4_session_state(tcpclv4_session_t *sess);

/*	Peer node ID from SESS_INIT (NULL before negotiation done).	*/

const char	*tcpclv4_session_peer_node_id(tcpclv4_session_t *sess);

int	tcpclv4_session_is_secure(tcpclv4_session_t *sess);

/*	Last error message, or NULL.					*/

const char	*tcpclv4_session_error(tcpclv4_session_t *sess);

#ifdef __cplusplus
}
#endif

#endif  /* _TCPCLV4_SESSION_H_ */
