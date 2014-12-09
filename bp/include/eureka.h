/*

	eureka.h:	definition of the application programming
			for noting discovery of the start and end
			of an opportunistic contact.  Only contacts
			utilizing TCP are discovered by ION.

	Copyright (c) 2014, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/
#ifndef _EUREKA_H_
#define _EUREKA_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "bp.h"

extern int	bp_discover_contact_acquired(
				char *socketSpec,
				uvast neighborNodeNbr,
				unsigned int xmitRate,
				unsigned int recvRate);
			/*	Notes initiation of newly discovered
				communication contact with node
				identified by neighboringNodeNbr
				and reachable by tcp socket connected
				to socketSpec.  The new neighboring
				node is known to be able to receive
				data from the local node at xmitRate
				bytes/sec and send data to the local
				node at recvRate bytes/sec.		*/

extern int	bp_discover_contact_lost(
				char *socketSpec,
				uvast neighborNodeNbr);
			/*	Notes the termination of a discovered
				communication contact previously noted
				by bp_discover_contact_acquired().	*/

#ifdef __cplusplus
}
#endif

#endif  /* _EUREKA_H_ */
