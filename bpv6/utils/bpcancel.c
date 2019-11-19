/*
	bpcancel.c:	bundle cancellation utility.
									*/
/*									*/
/*	Copyright (c) 2009, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include <bpP.h>

#if defined (ION_LWT)
int	bpcancel(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char		*sourceEid = (char *) a1;
	unsigned int	creationSec = a2;
	unsigned int	creationCount = a3;
	unsigned int	fragmentOffset = a4;
	unsigned int	fragmentLength = a5;
#else
int	main(int argc, char **argv)
{
	char		*sourceEid = argc > 1 ? argv[1] : NULL;
	unsigned int	creationSec = argc > 2 ? atoi(argv[2]) : 0;
	unsigned int	creationCount = argc > 3 ? atoi(argv[3]) : 0;
	unsigned int	fragmentOffset = argc > 4 ? atoi(argv[4]) : 0;
	unsigned int	fragmentLength = argc > 5 ? atoi(argv[5]) : 0;
#endif
	Sdr		sdr;
	BpTimestamp	creationTime;
	Object		bundleObj;

	if (sourceEid == NULL || creationSec == 0)
	{
		PUTS("Usage: bpcancel <source EID> <creation seconds> \
<creation count> <fragment offset> <fragment length>");
		return 0;
	}

	creationTime.seconds = creationSec;
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
		if (bpDestroyBundle(bundleObj, 1) < 0)
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
