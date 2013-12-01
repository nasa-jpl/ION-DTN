/*
	Private header file for ICI network management instrumentation 
	routines
									*/
/*									*/
/*	Copyright (c) 1997, California Institute of Technology.		*/
/*	ALL RIGHTS RESERVED.  U.S. Government Sponsorship		*/
/*	acknowledged.							*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*	Adapted by Vinny Ramachandran, JHU//APL		*/
/*									*/
#ifndef _ICINM_H_
#define _ICINM_H_

#include "ion.h"
 
typedef struct
{
	   unsigned int    smallPoolSize;
	   unsigned int    smallPoolFree;
	   unsigned int    smallPoolAllocated;
	   unsigned int    largePoolSize;
	   unsigned int    largePoolFree;
	   unsigned int    largePoolAllocated;
	   unsigned int    unusedSize;
} SdrnmState;
               
extern 	void sdrnm_state_get(SdrnmState *state);

#endif  /* _ICINM_H_ */
