/*
 	sppcla.h:	common definitions for Space Packet Protocol convergence layer
			adapter modules.

	Author: Gregory Miles JPL

	Modification History:
	Date  Who What

	Copyright (c) 2025, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
 									*/
#ifndef _SPPCLA_H_
#define _SPPCLA_H_

#include "bpP.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

// include space packet protocol libraries

#ifdef __cplusplus
extern "C" {
#endif

#define SPPCLA_BUFSZ		(65536)

extern int	sendBytesBySPP(int fd, char *from, int length);
extern int	sendBundleBySPP(int fd, unsigned int bundleLength,
				Object bundleZco, unsigned char *buffer);
extern int	receiveBytesBySPP(int fd,char *into, int length);

#ifdef __cplusplus
}
#endif

#endif	/* _SPPCLA_H */
