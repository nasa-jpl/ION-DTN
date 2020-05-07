/*
	bpstats.c:	print snapshot of node's current statistics.
									*/
/*									*/
/*	Copyright (c) 2008, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include <bpP.h>

#if defined (ION_LWT)
int	bpstats(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
#else
int	main(int argc, char **argv)
{
#endif
	if (bp_attach() < 0)
	{
		putErrmsg("bpstats can't attach to BP.", NULL);
		return 0;
	}

	writeMemo("[i] Start of statistics snapshot...");
	reportAllStateStats();
	writeMemo("[i] ...end of statistics snapshot.");
	bp_detach();
	return 0;
}
