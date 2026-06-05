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
	int		parsed_int;
	uvast		parsed_uvast;
	char		errMsg[256];

	/* Command name + 7 required arguments = 8 minimum arguments */
	if (argc < 8)
	{
		PUTS("Usage: sdrmend <sdr name> <config flags> <heap words> \
<heap key, e.g. -1> <log size> <log key> <pathName> [<restartCmd>]");
		return 0;
	}

	sdrName = argv[1];

	if (platform_parse_int(argv[2], &parsed_int) < 0 || parsed_int < 0)
	{
		isprintf(errMsg, sizeof(errMsg), "[?] Invalid config flags. Must be >= 0: %s", argv[2]);
		PUTS(errMsg);
		return 1;
	}
	configFlags = parsed_int;

	if (platform_parse_uvast(argv[3], &parsed_uvast) < 0)
	{
		isprintf(errMsg, sizeof(errMsg), "[?] Invalid heap words. Must be >= 0: %s", argv[3]);
		PUTS(errMsg);
		return 1;
	}
	heapWords = (long) parsed_uvast;

	if (platform_parse_int(argv[4], &parsed_int) < 0)
	{
		isprintf(errMsg, sizeof(errMsg), "[?] Invalid heap key: %s", argv[4]);
		PUTS(errMsg);
		return 1;
	}
	heapKey = parsed_int;

	if (platform_parse_int(argv[5], &parsed_int) < 0 || parsed_int < 0)
	{
		isprintf(errMsg, sizeof(errMsg), "[?] Invalid log size. Must be >= 0: %s", argv[5]);
		PUTS(errMsg);
		return 1;
	}
	logSize = parsed_int;

	if (platform_parse_int(argv[6], &parsed_int) < 0)
	{
		isprintf(errMsg, sizeof(errMsg), "[?] Invalid log key: %s", argv[6]);
		PUTS(errMsg);
		return 1;
	}
	logKey = parsed_int;

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
