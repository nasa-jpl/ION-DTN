/*
 	ltpcla.h:	common definitions for LTP convergence layer
			adapter modules.

	Author: Chris Krupiarz, APL
		Scott Burleigh, JPL

	Modification History:
	Date  Who What

	Copyright (c) 2007, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
 									*/
#ifndef _LTPCLA_H_
#define _LTPCLA_H_

#include "bpP.h"
#include "ltp.h"
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/* LTP Client ID for BPv7 is 4 per RFC 7116 */
#define BpLtpClientId		(4)

#ifdef __cplusplus
}
#endif

#endif	/* _LTPCLA_H */
