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

#ifndef LTPCLA_H
#define LTPCLA_H

#include "bpP.h"
#include "ltp.h"
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/* BP registers as LTP client ID 1 */
#define BpLtpClientId		(1)

#ifdef __cplusplus
}
#endif

#endif /* LTPCLA_H */
