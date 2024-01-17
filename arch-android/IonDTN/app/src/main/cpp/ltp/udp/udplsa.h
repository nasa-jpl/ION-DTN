/*
 	udplsa.h:	common definitions for UDP link service
			adapter modules.

	Author: Scott Burleigh, JPL

	Copyright (c) 2007, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
 									*/
#ifndef _UDPLSA_H_
#define _UDPLSA_H_

#include "ltpP.h"
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UDPLSA_BUFSZ		((256 * 256) - 1)
#define LtpUdpDefaultPortNbr	1113

#ifdef __cplusplus
}
#endif

#endif	/* _UDPLSA_H */
