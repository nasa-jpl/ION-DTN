/*
	stcpcla.h:	common definitions for simple TCP
			convergence layer adapter modules.

	Author: Scott Burleigh, JPL

	Modification History:
	Date  Who What

	Copyright (c) 2003, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/

#ifndef STCPCLA_H
#define STCPCLA_H

#include "bpP.h"
#include <pthread.h>
#include "ipnfw.h"
#include "dtn2fw.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ENABLE_HIGH_SPEED
#define STCPCLA_BUFSZ		(64 * 1024)
#else
#define STCPCLA_BUFSZ		(512 * 1024)
#endif

/*	Maximum bundle length that receiveBundleByStcp() will accept from
 *	a four-octet STCP length preamble.  The preamble is read into an
 *	unsigned 32-bit field but the acquisition machinery below it
 *	carries the length in a signed int, so any value at or above 2^31
 *	would become negative; and a four-byte preamble must not be able
 *	to make the node commit an arbitrary amount of acquisition space.
 *	The cap must stay below 2^31; raise it at compile time if a
 *	deployment legitimately carries larger bundles over STCP.	*/
#ifndef STCPCLA_MAX_BUNDLE_LEN
#define STCPCLA_MAX_BUNDLE_LEN	(512 * 1024 * 1024)
#endif
#define BpStcpDefaultPortNbr	4456

#ifndef STCP_KEEPALIVE_PERIOD
#define STCP_KEEPALIVE_PERIOD	(15)
#endif

/*	Note that libstcpcla functionality is invoked not only by
 *	the stcp CLA but also by the brss and brsc (Bundle Relay
 *	Service) CLAs.  Because outduct management is different for
 *	these different CL protocols, the invoking CLA's protocol
 *	name must be provided as an argument to some of the functions.	*/

extern int	openStcpOutductSocket(char *protocolName, char *ductName,
			int *bundleSocket);
extern int	sendBundleByStcp(char *protocolName, char *ductName,
			int *bundleSocket, unsigned int bundleLength,
			SdrObject bundleZco, char *buffer);
extern int	receiveBundleByStcp(int *bundleSocket, AcqWorkArea *work,
			char *buffer, ReqAttendant *attendant);
extern void	closeStcpOutductSocket(int *bundleSocket);

#ifdef __cplusplus
}
#endif

#endif /* STCPCLA_H */
