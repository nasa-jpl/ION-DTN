/*
 *	libicinm.c:	functions that implement the ICI instrumentation
 *			API.
 *
 *	Copyright (c) 2011, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 */

#include "ion.h"
#include "icinm.h"

void    sdrnm_state_get(SdrnmState *state)
{
	Sdr		sdr = getIonsdr();
	SdrUsageSummary	usage;

	CHKVOID(state);
	CHKVOID(sdr_begin_xn(sdr));
	sdr_usage(sdr, &usage);
	state->smallPoolSize = usage.smallPoolSize;
	state->smallPoolFree = usage.smallPoolFree;
	state->smallPoolAllocated = usage.smallPoolAllocated;
	state->largePoolSize = usage.largePoolSize;
	state->largePoolFree = usage.largePoolFree;
	state->largePoolAllocated = usage.largePoolAllocated;
	state->unusedSize = usage.unusedSize;
	sdr_exit_xn(sdr);
}
