/*
	amsstop.c:	an AMS utility program that simply shuts down
			a single message space.
									*/
/*									*/
/*	Copyright (c) 2011, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include "ams.h"

#if defined (VXWORKS) || defined (RTEMS)
int	amsstop(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	char		*applicationName = (char *) a1;
	char		*authorityName = (char *) a2;
#else
int	main(int argc, char **argv)
{
	char		*applicationName = (argc > 1 ? argv[1] : NULL);
	char		*authorityName = (argc > 2 ? argv[2] : NULL);
#endif
	AmsModule	me;

	if (applicationName == NULL || authorityName == NULL)
	{
		PUTS("Usage: amsstop <application name> <authority name>");
		return 0;
	}

	if (ams_register("", NULL, applicationName, authorityName, "", "stop",
			&me) < 0)
	{
		putErrmsg("amsstop can't register.", NULL);
		return 1;
	}

	writeErrmsgMemos();
	return 0;
}
