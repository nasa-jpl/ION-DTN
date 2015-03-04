/*
 	dgr.h:	definitions supporting the implementation of DGR
       		applications.

	Author: Scott Burleigh, JPL

	Modification History:
	Date  		Who 	What
	4 July 2010	Scott	Revise to align with LTP spec.

	Copyright (c) 2003, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
 									*/
#ifndef _DGR_H_
#define _DGR_H_

#include "platform.h"
#include "lyst.h"

#ifdef __cplusplus
extern "C" {
#endif

/*	dgr_send notification flags (to be OR'd together in flags parm)	*/
#define	DGR_NOTE_NONE	(0)		/*	No notification.	*/
#define	DGR_NOTE_FAILED	(1)		/*	Note delivery failure.	*/
#define	DGR_NOTE_ACKED	(2)		/*	Note delivery success.	*/
#define	DGR_NOTE_ALL	(DGR_NOTE_FAILED | DGR_NOTE_ACKED)

/*	dgr_receive timeout values					*/
#define	DGR_POLL	(0)		/*	Return immediately.	*/
#define DGR_BLOCKING	(-1)		/*	Wait forever.		*/

typedef enum
{
	DgrFailed = 1,
	DgrOpened,
	DgrDatagramSent,
	DgrDatagramReceived,
	DgrTimedOut,
	DgrInterrupted,
	DgrDatagramAcknowledged,
	DgrDatagramNotAcknowledged
} DgrRC;

typedef struct dgrsapst	*Dgr;

extern int		dgr_open(	uvast ownEngineId,
					unsigned int clientSvcId,
					unsigned short ownPortNbr,
					unsigned int ownIpAddress,
					char *memmgrName,
					Dgr *dgr,
					DgrRC *rc);
			/*	Arguments are:
			 *		port number to use for DGR
			 *			service (if 0, defaults
			 *			to system-assigned UDP
			 *			port number)
			 *		Internet address of IP interface
			 *			to use for DGR service;
			 *			if 0, defaults to the
			 *			address of the interface
			 *			identified by the local
			 *			machine's host name
			 *		name of memory manager to use
			 *			for dynamic memory
			 *			management; if NULL,
			 *			defaults to standard
			 *			system malloc/free
			 *		location in which to store
			 *			service access pointer
			 *		location in which to store
			 *			return code
			 *
			 *	Returns 0 on success, -1 on failure.	*/

extern void		dgr_getsockname(Dgr dgr,
					unsigned short *portNbr,
					unsigned int *ipAddress);
			/*	States the port number and IP address
			 *	of the UDP socket used for this DGR
			 *	service access point.			*/

extern void		dgr_close(Dgr dgr);
			/*	Reverses dgr_open, releasing resources
			 *	where possible.				*/

extern int		dgr_send(	Dgr dgr,
					unsigned short toPortNbr,
					unsigned int toIpAddress,
					int notificationFlags,
					char *content,
					int length,
					DgrRC *rc);
			/*	Sends the indicated content, of length
			 *	as indicated, to the indicated remote
			 *	DGR service access point.  The message
			 *	will be retransmitted as necessary
			 *	until either it is acknowledged or
			 *	DGR determines that it cannot be
			 *	delivered.  Length of content must be
			 *	greater than zero and may be as great
			 *	as 65535, but lengths greater than
			 *	8192 may not be supported by the local
			 *	underlying UDP implementation; to
			 *	minimize the chance of data loss when
			 *	transmitting over the internet, length
			 *	should not exceed 512.  Returns 0 on
			 *	success (setting *rc as appropriate),
			 *	-1 on failure.				*/

extern int		dgr_receive(	Dgr dgr,
					unsigned short *fromPortNbr,
					unsigned int *fromIpAddress,
					char *content,
					int *length,
					int *errnbr,
					int timeoutSeconds,
					DgrRC *rc);
			/*	Delivers the oldest undelivered DGR
			 *	event queued for delivery.
			 *
			 *	DGR events are of two type: (a) a
			 *	message received from a remote DGR
			 *	service access point and (b) a notice
			 *	of a previously sent message that
			 *	DGR has determined either has been
			 *	or cannot be delivered, as requested
			 *	in the notificationFlags parm of the
			 *	dgr_send call that sent the message.
			 *
			 *	In the former case, dgr_receive will
			 *	place the content of the inbound
			 *	message in "content", its length in
			 *	"length", and the IP address and port
			 *	number of the sender in "fromIpAddress"
			 *	and "fromPortNbr", and will set *rc to
			 *	DgrDatagramReceived.
			 *
			 *	In the latter case, dgr_receive will
			 *	place the content of the outbound
			 *	message in "content" and its length
			 *	in "length", will place the relevant
			 *	errno (if any) in errnbr, and will set
			 *	*rc to either DgrDatagramAcknowledged
			 *	or DgrDatagramNotAcknowledged.
			 *
			 *	The "content" buffer should be at least
			 *	65535 bytes in length to enable delivery
			 *	of the content of the received or
			 *	delivered/undeliverable message.
			 *
			 *	The "timeoutSeconds" argument controls
			 *	blocking behavior.  If timeoutSeconds
			 *	is DGR_BLOCKING, dgr_receive will not
			 *	return until there is either an inbound
			 *	message to deliver or an outbound
			 *	message delivery result to report
			 *	(or an I/O error).  If timeoutSeconds
			 *	is DGR_POLL, dgr_receive returns
			 *	immediately; if there is currently no
			 *	inbound message to deliver and no
			 *	outbound message delivery result to
			 *	report, the function sets *rc to
			 *	DgrTimedOut.  For any other positive
			 *	value of timeoutSeconds, dgr_receive
			 *	returns after the indicated number of
			 *	seconds have lapsed, or there is a
			 *	message to deliver or a delivery
			 *	result to report, whichever occurs
			 *	first; in the former case, it sets
			 *	*rc to DgrTimedOut.
			 *
			 *	Returns 0 on success, -1 on failure.	*/

extern void		dgr_interrupt(Dgr dgr);
			/*	Interrupts a dgr_receive invocation
			 *	that is currently blocked.  Designed 
			 *	to be called from a signal handler.	*/

#ifdef __cplusplus
}
#endif

#endif	/* _DGR_H */
