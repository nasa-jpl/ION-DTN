/*
 *	sda.h:	definitions supporting the implementation of SDA Service
 *		Data Aggregation (SDA) functionality.
 *
 *	Copyright (c) 2013, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 */

#include "ltp.h"

#ifndef _SDA_H_
#define _SDA_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef vast	(*SdaDelimiterFn)(unsigned int clientId,
			unsigned char *buffer,
			vast bufferLength);
		/*	An SDA delimiter function inspects the client
		 *	service data bytes in "buffer" - the first
		 *	"bufferLength" bytes of the as-yet unprocessed
		 *	remnant of an LTP service data block - to
		 *	determine the length of the client data unit
		 *	at the start of the buffer; the "clientID" of
		 *	the client data unit is provided to aid in
		 *	this determination.  It returns that length
		 *	if the determination was successful, zero if
		 *	there is no valid client service data unit
		 *	at the start of the buffer, -1 on any other
		 *	failure.					*/

typedef int	(*SdaHandlerFn)(uvast sourceEngineId,
			unsigned int clientId,
			Object clientServiceData);	/*	ZCO	*/
		/*	An SDA handler function applies application
		 *	processing to the client service data unit
		 *	for client "clientID" that is identified by
		 *	clientServiceData.  It returns -1 on any
		 *	system error, otherwise zero.			*/

/*	*	*	SDA data transmission	*	*	*	*/

extern int	sda_send(uvast destinationEngineId,
			unsigned int clientId,
			Object clientServiceData);
		/*	clientServiceData must be a "zero-copy object"
	 	 *	reference as returned by ionCreateZco().  Note
		 *	that SDA will privately make and destroy its
		 *	own reference to the client service data; the
		 *	application is free to destroy its reference
		 *	at any time.   Note that the client service
		 *	data unit will be sent reliably (i.e., "red").	*/

/*	*	*	SDA data reception	*	*	*	*/

extern int	sda_run(SdaDelimiterFn delimiter, SdaHandlerFn handler);
		/*	sda_run executes an infinite loop that receives
		 *	client service data blocks, calls "delimiter"
		 *	to determine the length of each client service
		 *	data item in each block, and passes those client
		 *	service data items to the handler function.  To
		 *	terminate the loop, call sda_interrupt().  Note
		 *	that sda_send() can only be executed while the
		 *	sda_run loop is still executing.		*/

extern void	sda_interrupt();

#ifdef __cplusplus
}
#endif

#endif
