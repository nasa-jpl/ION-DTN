/*
	bpversion.c:	Prints the version of Bundle Protocol that
			executes on this node.
									*/
/*									*/
/*	Copyright (c) 2020, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include "platform.h"

#if defined (ION_LWT)
int	bpversion(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
#else
int	main(int argc, char **argv)
{
#endif
	puts("bpv6");
	return 6;
}
