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
int	bpstats(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
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
