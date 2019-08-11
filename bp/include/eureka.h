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
	identified by discoveryEid
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
 * @param  discoveryEid    Node identification. 
 * @param  claProtocol     Target CLA.
 * @param  xmitRate        Discovered node's rate of receiving data.
 * @param  recvRate        Discovered node's rate of sending data.
 * @return                 0 on success. -1 on error.
 */
extern int	bp_discovery_acquired(
				char *socketSpec,
				char *discoveryEid,
				char *claProtocol,
				unsigned int xmitRate,
				unsigned int recvRate);

/**
 * Notes the termination of a discovered
	communication contact previously noted
	by bp_discover_contact_acquired().
 * @param  socketSpec      socketSpec of the terminated contact.
 * @param  discoveryEid    Node identification. 
 * @param  claProtocol	   Target CLA.
 * @return                 0 on success, -1 on error.
 */
extern int	bp_discovery_lost(
				char *socketSpec,
				char *discoveryEid,
				char *claProtocol);

/**
 * Locates a discovery previously noted
	by bp_discover_contact_acquired().
 * @param  discoveryEid    Node identification. 
 * @return                 SmList elt on success, 0 if discovery not found.
 */
extern PsmAddress	bp_find_discovery(
				char *discoveryEid);

#ifdef __cplusplus
}
#endif

#endif  /* _EUREKA_H_ */
