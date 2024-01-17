/*
 	udpcla.h:	common definitions for UDP convergence layer
			adapter modules.

	Author: Ted Piotrowski, APL
		Scott Burleigh, JPL

	Modification History:
	Date  Who What

	Copyright (c) 2003, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
 									*/
#ifndef _UDPCLA_H_
#define _UDPCLA_H_

#include "bpP.h"
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UDPCLA_BUFSZ		((256 * 256) - 1)

extern int	sendBytesByUDP(int *bundleSocket, char *from, int length,
			struct sockaddr *socketName);
extern int	sendBundleByUDP(struct sockaddr *socketName, int *bundleSocket,
			unsigned int bundleLength, Object bundleZco,
			unsigned char *buffer);
extern int	receiveBytesByUDP(int bundleSocket,
			struct sockaddr_in *fromAddr,char *into, int length);

#ifdef __cplusplus
}
#endif

#endif	/* _UDPCLA_H */
