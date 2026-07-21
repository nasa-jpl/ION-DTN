/*
	bpcancel.c:	bundle cancellation utility.
									*/
/*									*/
/*	Copyright (c) 2009, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include <bpP.h>
#include "platform.h"

#if defined (ION_LWT)
int     bpcancel(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char            *sourceEid = (char *) a1;
	uvast           creationMsec = a2;
	unsigned int    creationCount = a3;
	unsigned int    fragmentOffset = a4;
	unsigned int    fragmentLength = a5;
#else
int     main(int argc, char **argv)
{
	char            *sourceEid = NULL;
	uvast           creationMsec = 0;
	unsigned int    creationCount = 0;
	unsigned int    fragmentOffset = 0;
	unsigned int    fragmentLength = 0;
#endif
	Sdr             sdr;
	BpTimestamp     creationTime;
	SdrObject       bundleObj;
	int             parsed_int;
	uvast           parsed_uvast;
	char            errMsg[256];

#ifndef ION_LWT
	sourceEid = argc > 1 ? argv[1] : NULL;

	if (argc > 2)
	{
		if (platform_parse_uvast(argv[2], &parsed_uvast) < 0)
		{
			isprintf(errMsg, sizeof(errMsg), "[?] Invalid creation seconds: %s", argv[2]);
			PUTS(errMsg);
			return 0;
		}
		creationMsec = parsed_uvast;
	}

	if (argc > 3)
	{
		if (platform_parse_int(argv[3], &parsed_int) < 0 || parsed_int < 0)
		{
			isprintf(errMsg, sizeof(errMsg), "[?] Invalid creation count. Must be >= 0: %s", argv[3]);
			PUTS(errMsg);
			return 0;
		}
		creationCount = (unsigned int) parsed_int;
	}

	if (argc > 4)
	{
		if (platform_parse_int(argv[4], &parsed_int) < 0 || parsed_int < 0)
		{
			isprintf(errMsg, sizeof(errMsg), "[?] Invalid fragment offset. Must be >= 0: %s", argv[4]);
			PUTS(errMsg);
			return 0;
		}
		fragmentOffset = (unsigned int) parsed_int;
	}

	if (argc > 5)
	{
		if (platform_parse_int(argv[5], &parsed_int) < 0 || parsed_int < 0)
		{
			isprintf(errMsg, sizeof(errMsg), "[?] Invalid fragment length. Must be >= 0: %s", argv[5]);
			PUTS(errMsg);
			return 0;
		}
		fragmentLength = (unsigned int) parsed_int;
	}
#endif

	if (sourceEid == NULL || creationMsec == 0)
	{
		PUTS("Usage: bpcancel <source EID> <creation seconds> \
<creation count> <fragment offset> <fragment length>");
		return 0;
	}

	creationTime.msec = creationMsec;
	creationTime.count = creationCount;
	if (bp_attach() < 0)
	{
		putErrmsg("Can't attach to BP.", NULL);
		return 0;
	}

	sdr = bp_get_sdr();
	CHKZERO(sdr_begin_xn(sdr));
	if (findBundle(sourceEid, &creationTime, fragmentOffset, fragmentLength,
			&bundleObj) < 0)
	{
		sdr_cancel_xn(sdr);
		bp_detach();
		putErrmsg("bpcancel failed finding bundle.", NULL);
		return 0;
	}

	if (bundleObj)
	{
		if (bpDestroyBundle(bundleObj, 3) < 0)
		{
			sdr_cancel_xn(sdr);
			bp_detach();
			putErrmsg("bpcancel failed destroying bundle.", NULL);
			return 0;
		}

		PUTS("Bundle transmission has been canceled.");
	}
	else
	{
		PUTS("Unable to cancel transmission of this bundle.");
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("bpcancel failed.", NULL);
	}

	writeErrmsgMemos();
	PUTS("Stopping bpcancel.");
	bp_detach();
	return 0;
}
