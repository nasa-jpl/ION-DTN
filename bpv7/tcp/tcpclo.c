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
int	tcpclo(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
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
