/*
 *	ltp.h:	definitions supporting the implementation of LTP
 *		(Licklider Transmission Protocol) application software.
 *
 *
 *	Copyright (c) 2007, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 */

#include "sdr.h"

#ifndef _LTP_H_
#define _LTP_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	uvast		sourceEngineId;
	unsigned int	sessionNbr;	/*	Assigned by source.	*/
} LtpSessionId;

/*	*	*	LTP initialization	*	*	*	*/

extern int	ltp_attach();

extern void	ltp_detach();

extern int	ltp_engine_is_started();
		/*	Returns 1 if the local LTP engine has been
		 *	started and not yet stopped, 0 otherwise.	*/

/*	*	*	LTP data transmission	*	*	*	*/

#define	LTP_ALL_RED	((unsigned int) -1)

extern int	ltp_send(uvast destinationEngineId,
			unsigned int clientId,
			Object clientServiceData,
			unsigned int redLength,
			LtpSessionId *sessionId);
		/*	clientServiceData must be a "zero-copy object"
	 	 *	reference as returned by ionCreateZco().  Note
		 *	that LTP will privately make and destroy its
		 *	own reference to the client service data; the
		 *	application is free to destroy its reference
		 *	at any time.   If the entire client service
		 *	data unit is to be sent reliably, redLength
		 *	may be simply LTP_ALL_RED.	 		*/

/*	*	*	LTP data reception	*	*	*	*/

typedef enum
{
	LtpNoNotice = 0,
	LtpExportSessionStart,
	LtpXmitComplete,
	LtpExportSessionCanceled,
	LtpExportSessionComplete,
	LtpRecvGreenSegment,
	LtpRecvRedPart,
	LtpImportSessionCanceled
} LtpNoticeType;

extern int	ltp_open(unsigned int clientId);

extern int	ltp_get_notice(unsigned int clientId,
			LtpNoticeType *type,
			LtpSessionId *sessionId,
			unsigned char *reasonCode,
			unsigned char *endOfBlock,
			unsigned int *dataOffset,
			unsigned int *dataLength,
			Object *data);
		/*	The value returned in *data is always a zero-
		 *	copy object; use the zco_* functions defined
		 *	in "zco.h" to retrieve the content of that
		 *	object.
		 *
		 *	When the notice is an LtpRecvGreenSegment,
		 *	the ZCO returned in *data contains the content
		 *	of a single LTP green segment.  Reassembly of
		 *	the green part of some block from these segments
		 *	is the responsibility of the application.
		 *
		 *	When the notice is an LtpRecvRedPart, the ZCO
		 *	returned in *data contains the red part of a
		 *	possibly aggregated block.  The ZCO's content
		 *	may therefore comprise multiple service data
		 *	objects.  Extraction of individual service
		 *	data objects from the aggregated block is the
		 *	responsibility of the application.  A simple
		 *	way to do this is to prepend the length of
		 *	the service data object to the object itself
		 *	(using zco_prepend_header) before calling
		 *	ltp_send, so that the receiving application
		 *	can alternate extraction of object lengths and
		 *	objects from the delivered block's red part.
		 *
		 *	The cancellation of an export session may result
		 *	in delivery of multiple LtpExportSessionCanceled
		 *	notices, one for each service data unit in the
		 *	export session's (potentially) aggregated block.
		 *	The ZCO returned in *data for each such notice
		 *	is a service data unit ZCO that had previously
		 *	been passed to the ltp_send function.		*/

extern void	ltp_interrupt(unsigned int clientId);

extern void	ltp_release_data(Object data);

extern void	ltp_close(unsigned int clientId);

#ifdef __cplusplus
}
#endif

#endif
