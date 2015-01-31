/*
 *	bssp.h:	definitions supporting the implementation of BSSP
 *		(Bundle Streaming Service Protocol) application software.
 *
 *	Authors: Sotirios-Angelos Lenas, SPICE
 *
 *	Copyright (c) 2013, California Institute of Technology.
 *	Copyright (c) 2013, Space Internetworking Center,
 *	Democritus University of Thrace.
 *	
 *	All rights reserved. U.S. Government and E.U. Sponsorship acknowledged.
 *
 */

#include "platform.h"
#include "ion.h"
#include "sdr.h"

#ifndef _BSSP_H_
#define _BSSP_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	uvast		sourceEngineId;
	unsigned int	sessionNbr;	/*	Assigned by source.	*/
} BsspSessionId;

/*	*	*	BSSP initialization	*	*	*	*/

extern int	bssp_attach();

extern void	bssp_detach();

extern int	bssp_engine_is_started();
		/*	Returns 1 if the local BSSP engine has been
		 *	started and not yet stopped, 0 otherwise.	*/

/*	*	*	BSSP data transmission	*	*	*	*/

extern int	bssp_send(uvast destinationEngineId,
			unsigned int clientId,
			Object clientServiceData,
			int inOrder,
			BsspSessionId *sessionId);

		/*	clientServiceData must be a "zero-copy object"
	 	 *	reference as returned by ionCreateZco().  Note
		 *	that BSSP will privately make and destroy its
		 *	own reference to the client service data; the
		 *	application is free to destroy its reference
		 *	at any time.  The inOrder parameter is a
		 *	Boolean variable indicating whether or not
		 *	the service data item that is being sent is
		 *	"in order", i.e., was originally transmitted
		 *	after all items that have previously been
		 *	sent to this destination by this local BSSP
		 *	engine: 0 if no (meaning that the item must
		 *	be transmitted using the "reliable" channel),
		 *	1 if yes (meaning that the item must be
		 *	transmitted using the "best-efforts" channel).
	         */

/*	*	*	BSSP data reception	*	*	*	*/

typedef enum
{
	BsspNoNotice = 0,
	BsspXmitSuccess,
	BsspXmitFailure,
	BsspRecvSuccess
} BsspNoticeType;

extern int	bssp_open(unsigned int clientId);

extern int	bssp_get_notice(unsigned int clientId,
			BsspNoticeType *type,
			BsspSessionId *sessionId,
			unsigned char *reasonCode,
			unsigned int *dataLength,
			Object *data);
		/*	The value returned in *data is always a zero-
		 *	copy object; use the zco_* functions defined
		 *	in "zco.h" to retrieve the content of that
		 *	object.
		 *
		 *	When the notice is a BsspRecvSuccess, the ZCO 
		 *	returned in *data contains the content of a 
		 *	single BSSP block.
		 *
		 *	The cancellation of an export session results
		 *	in delivery of a BsspXmitFailure notice.  In
		 *	this case, the ZCO returned in *data is a
		 *	service data unit that had previously beenx
		 *	passed to the bssp_send function.		*/

extern void	bssp_interrupt(unsigned int clientId);

extern void	bssp_release_data(Object data);

extern void	bssp_close(unsigned int clientId);

#ifdef __cplusplus
}
#endif

#endif
