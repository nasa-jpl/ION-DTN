/*
	xlsa.h:		Common definitions for the "X" link service
			adapter -- a generic, transport-agnostic LTP
			link service template.

	This header is the SINGLE seam a system integrator must
	reimplement to carry LTP segments over a custom lower layer
	(serial/UART, SpaceWire, a shared bus, an FPGA FIFO, etc.).

	The LTP engine itself is never modified.  An LTP link service
	is just two daemons bound to a span:

	  - the output daemon (xlso) pulls one ready segment at a time
	    from the engine via ltpDequeueOutboundSegment() and hands it
	    to xport_send();

	  - the input daemon (xlsi) receives one segment at a time via
	    xport_recv() and hands it to ltpHandleInboundSegment().

	CRITICAL CONTRACT -- MESSAGE BOUNDARIES:
	LTP is datagram-oriented.  ltpHandleInboundSegment() expects
	EXACTLY ONE complete segment per call; it does NOT reassemble a
	byte stream.  Therefore xport_recv() MUST return exactly one
	segment per call, with the same boundaries that xport_send()
	was given.  A datagram transport (UDP, shared-memory ring)
	preserves boundaries for free.  A byte-stream transport
	(TCP, serial, UART) does NOT -- such a backend MUST add framing
	on send and a deframer on receive.  See doc/DESIGN.md.

	Patterned on ltp/udp/udplsa.h from nasa-jpl/ion-dtn (integration).

	Copyright (c) 2026.  Provided as a template; license follows the
	host ION distribution (MIT) unless your organization states
	otherwise.
									*/

#ifndef XLSA_H
#define XLSA_H

#include "ltpP.h"
#include "ion_atomic.h"
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/*	Maximum bytes the transport will ever be asked to move in one
 *	call.  This MUST be >= the span's maxSegmentSize (configured in
 *	ltpadmin).  The engine allocates vspan->segmentBuffer at exactly
 *	maxSegmentSize, so a segment can never exceed it; we keep a small
 *	margin for an optional framing header in byte-stream backends.	*/

#define XLSA_BUFSZ		(1024 * 1024)

/*	Opaque per-link transport state.  The reference shared-memory
 *	backend stores its ring handles here; a serial backend would
 *	store a file descriptor and deframer state.  The template code
 *	never inspects the inside of this struct -- only the backend
 *	does -- so you may add whatever fields your transport needs.	*/

typedef struct xport_link_st	XportLink;

/*	Parameters handed to the receiver thread.  Mirrors
 *	udp_ReceiverThreadParms but with no socket/address coupling:
 *	the transport is reached only through the XportLink handle.	*/

typedef struct
{
	XportLink	*link;		/*	Backend handle.		*/
	pthread_mutex_t	lock;		/*	Guards backend if needed.*/
	ion_atomic_t	running;	/*	Set 0 to stop the thread.*/
} xlsa_ReceiverThreadParms;

/*	*	*	Transport seam -- IMPLEMENT THESE	*	*/

/*	xport_open() acquires/creates the link named by endpointSpec.
 *	  - 'isSender' is 1 for the xlso side, 0 for the xlsi side, so a
 *	    backend can pick the correct direction of a unidirectional
 *	    ring or open the bus for the right purpose.
 *	  - 'remoteEngineId' is the LTP engine number of the peer (sender
 *	    side only; 0 on the receiver side).
 *	Returns a non-NULL handle on success, NULL on failure.		*/

extern XportLink	*xport_open(const char *endpointSpec, int isSender,
				uvast remoteEngineId);

/*	xport_send() transmits exactly one segment of 'length' bytes.
 *	Returns the number of payload bytes accepted (== length) on
 *	success, or -1 on a fatal/unrecoverable error.  Transient
 *	conditions (a momentarily full ring, a busy bus) should be
 *	retried internally or treated as link loss (return 'length'),
 *	NOT reported as fatal -- mirrors udplso's treatment of UDP
 *	network errors as packet loss so the daemon survives link
 *	flaps.								*/

extern int		xport_send(XportLink *link, char *segment,
				int length);

/*	xport_recv() blocks until exactly one segment is available, then
 *	copies it into 'buf' (capacity 'maxlen') and returns its length.
 *	Special return values mirror the LTP receiver-thread contract:
 *	  >  1 : a real segment of that many bytes;
 *	  ==1 : the one-byte shutdown sentinel (stop the loop cleanly);
 *	  == 0 : interrupted, retry;
 *	  == -1 : fatal error.						*/

extern int		xport_recv(XportLink *link, char *buf, int maxlen);

/*	xport_close() releases the link.  Safe to call on NULL.		*/

extern void		xport_close(XportLink *link);

/*	xport_wake() unblocks a receiver currently parked in
 *	xport_recv() so the daemon can shut down.  The reference backend
 *	implements this by enqueuing the one-byte sentinel, mirroring
 *	the UDP driver's "send a 1-byte datagram to myself" shutdown
 *	trick.								*/

extern void		xport_wake(XportLink *link);

/*	Receiver thread entry point (provided by libxlsa.c; you should
 *	not need to touch it).						*/

extern void		*xlsa_handle_datagrams(void *parm);

#ifdef __cplusplus
}
#endif

#endif	/* XLSA_H */
