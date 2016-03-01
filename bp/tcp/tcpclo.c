/*
	tcpclo.c:	dummy tcpclo daemon for backward compatibility.
									*/
/*									*/
/*	Copyright (c) 2015, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include <bpP.h>

#if defined (ION_LWT)
int	tcpclo(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
#else
int	main(int argc, char **argv)
{
#endif
	if (bp_attach() < 0)
	{
		putErrmsg("tcpclo can't attach to BP.", NULL);
		return 0;
	}

	writeMemo("[i] tcpclo is deprecated.  tcpcl outducts are now drained \
by tcpcli threads.");
	bp_detach();
	return 0;
}
