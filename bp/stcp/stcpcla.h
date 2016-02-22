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
#ifndef _STCPCLA_H_
#define _STCPCLA_H_

#include "bpP.h"
#include <pthread.h>
#include "ipnfw.h"
#include "dtn2fw.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STCPCLA_BUFSZ		(64 * 1024)
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
			Object bundleZco, char *buffer);
extern int	receiveBundleByStcp(int *bundleSocket, AcqWorkArea *work,
			char *buffer, ReqAttendant *attendant);
extern void	closeStcpOutductSocket(int *bundleSocket);

#ifdef __cplusplus
}
#endif

#endif	/* _STCPCLA_H */
