/*

	saga.h:	definition of the application programming interface
		for managing ION's distributed history of discovered
		contacts and invoking the generation of predicted
		contacts from that history.

	Copyright (c) 2019, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/
#ifndef _SAGA_H_
#define _SAGA_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ion.h"

/*		Functions for managing "sagas".
 
 		A saga is the history of all discovered contacts
		between nodes within some region over the past N
		days, where N is 30 by default.				*/

extern void	saga_insert(time_t fromTime,
				time_t toTime,
				uvast fromNode,
				uvast toNode,
				size_t xmitRate,
				int regionIdx);
			/*	Creates a new Episode object and
				inserts that object into the saga
				(an SDR linked list) for the
				indicated region of the local node.

				Returns zero on success, -1 on any
				system error.				*/

extern int	saga_ingest(int regionIdx);
			/*	Erases all predicted contacts and
			 *	computes new predicted contacts
			 *	for all node pairs in the region
			 *	wherever possible.
			 *
			 *	Returns 0 on success, -1 on any
			 *	system error.				*/

extern int	saga_send(uvast destinationNodeNbr, int regionIdx);
			/*	Constructs a "saga" message,
			 *	containing the indicated region's
			 *	region number and the current
			 *	contents of the saga for the
			 *	indicated region of the local
			 *	node, and uses bp_send to convey
			 *	this message to the indicated
			 *	neighboring node.  In so doing,
			 *	deletes any encounters in this
			 *	saga that are obsolete (e.g.,
			 *	more than 30 days old).
			 *
			 *	Returns 0 on success, -1 on any
			 *	system error.				*/

extern int	saga_receive(BpDelivery *dlv, unsigned char *cursor,
			unsigned int unparsedBytes);

#ifdef __cplusplus
}
#endif

#endif  /* _SAGA_H_ */
