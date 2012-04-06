/*
 	icinm.h:	definitions supporting the ICI instrumentation
			API.

	Copyright (c) 2011, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.

	Author: Scott Burleigh, JPL
 									*/
#ifndef _ICINM_H_
#define _ICINM_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	unsigned long	smallPoolSize;
	unsigned long	smallPoolFree;
	unsigned long	smallPoolAllocated;
	unsigned long	largePoolSize;
	unsigned long	largePoolFree;
	unsigned long	largePoolAllocated;
	unsigned long	unusedSize;
} SdrnmState;

extern void	sdrnm_state_get(SdrnmState *state);

#ifdef __cplusplus
}
#endif

#endif  /* _ICINM_H_ */
