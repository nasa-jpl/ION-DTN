/*
 	icinm.h:	definitions supporting the ICI instrumentation
			API.

	Copyright (c) 2011, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.

	Author: Scott Burleigh, JPL
	Adapted by Vinny Ramachandran, JHU/APL
 									*/
#ifndef _ICINM_H_
#define _ICINM_H_

#include "ion.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	unsigned int	smallPoolSize;
	unsigned int	smallPoolFree;
	unsigned int	smallPoolAllocated;
	unsigned int	largePoolSize;
	unsigned int	largePoolFree;
	unsigned int	largePoolAllocated;
	unsigned int	unusedSize;
} SdrnmState;

extern void	sdrnm_state_get(SdrnmState *state);

#ifdef __cplusplus
}
#endif

#endif  /* _ICINM_H_ */
