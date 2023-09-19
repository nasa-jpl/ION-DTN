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

#if defined (ION_LWT)
int	sdrmend(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char		*sdrName = (char *) a1;
	int		configFlags = a2;
	long		heapWords = a3;
	int		heapKey = a4;
	int		logSize = a5;
	int		logKey = a6;
	char		*pathName = (char *) a7;
	char		*restartCmd = (char *) a8;

#else
int	main(int argc, char **argv)
{
	char		*sdrName;
	int		configFlags;
	long		heapWords;
	int		heapKey;
	int		logSize;
	int		logKey;
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
	logSize = strtol(argv[5], NULL, 0);
	logKey = strtol(argv[6], NULL, 0);
	pathName = argv[7];
	if (argc == 9)
	{
		restartCmd = argv[8];
	}
#endif

	if (sdr_initialize(0, NULL, SM_NO_KEY, NULL) < 0)
	{
		putErrmsg("Can't initialize the SDR system.", NULL);
		return 1;
	}

	if (sdr_reload_profile(sdrName, configFlags, heapWords, heapKey,
			logSize, logKey, pathName, restartCmd) < 0)
	{
		putErrmsg("Can't reload profile for SDR.", sdrName);
		return 1;
	}

	writeMemo("[i] SDR profile reloaded.");
	return 0;
}
