/*

	sdrmend.c:	repairs potentially corrupt Sdr.

									*/
/*									*/
/*	Copyright (c) 2005, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include "platform.h"
#include "sdr.h"

#if defined (VXWORKS) || defined (RTEMS) || defined (bionic)
int	sdrmend(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	char		*sdrName = (char *) a1;
	int		configFlags = a2;
	long		heapWords = a3;
	int		heapKey = a4;
	char		*pathName = (char *) a5;
	char		*restartCmd = (char *) a6;

#else
int	main(int argc, char **argv)
{
	char		*sdrName;
	int		configFlags;
	long		heapWords;
	int		heapKey;
	char		*pathName;
	char		*restartCmd = NULL;

	if (argc < 6)
	{
		PUTS("Usage: sdrmend <sdr name> <config flags> <heap words> \
<heap key, e.g. -1> <pathName> [<restartCmd>]");
		return 0;
	}

	sdrName = argv[1];
	configFlags = strtol(argv[2], NULL, 0);
	heapWords = strtol(argv[3], NULL, 0);
	heapKey = strtol(argv[4], NULL, 0);
	pathName = argv[5];
	if (argc == 7)
	{
		restartCmd = argv[6];
	}
#endif

	if (sdr_initialize(0, NULL, SM_NO_KEY, NULL) < 0)
	{
		putErrmsg("Can't initialize the SDR system.", NULL);
		return 1;
	}

	if (sdr_reload_profile(sdrName, configFlags, heapWords, heapKey,
			pathName, restartCmd) < 0)
	{
		putErrmsg("Can't reload profile for SDR.", sdrName);
		return 1;
	}

	writeMemo("[i] SDR profile reloaded.");
	return 0;
}
