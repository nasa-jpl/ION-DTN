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

/**
 * Notes initiation of newly discovered
	communication contact with node
	identified by neighboringNodeEid
	and reachable by socket connected
	to socketSpec. The new neighboring
	node is known to be able to receive
	data from the local node at xmitRate
	bytes/sec and send data to the local
	node at recvRate bytes/sec.  socketSpec
	will have the appropriate format for 
	the applicable convergence-layer protocol.
	For example if the convergence-layer
	protocol is tcp, the socketSpec will have
	the form ipaddress:port or hostname:port.
 * @param  socketSpec      socketSpec for the required CLA.
 * @param  neighborEid     Node identification. 
 * @param  claProtocol     Target CLA.
 * @param  xmitRate        Neighbor's rate of receiving data.
 * @param  recvRate        Neighbor's rate of sending data.
 * @return                 0 on success. -1 on error.
 */
extern int	bp_discover_contact_acquired(
				char *socketSpec,
				char *neighborEid,
				char *claProtocol,
				unsigned int xmitRate,
				unsigned int recvRate);

/**
 * Notes the termination of a discovered
	communication contact previously noted
	by bp_discover_contact_acquired().
 * @param  socketSpec      socketSpec of the terminated contact.
 * @param  neighborEid     Node identification. 
 * @param  claProtocol	   Target CLA.
 * @return                 0 on success, -1 on error.
 */
extern int	bp_discover_contact_lost(
				char *socketSpec,
				char *neighborEid,
				char *claProtocol);

/**
 * Locates a neighbor previously noted
	by bp_discover_contact_acquired().
 * @param  neighborEid     Node identification. 
 * @return                 SmList elt on success, 0 if neighbor not found.
 */
extern PsmAddress	bp_discover_find_neighbor(
				char *neighborEid);

#ifdef __cplusplus
}
#endif

#endif  /* _EUREKA_H_ */
