/*
	ionxnowner.c:	Prints the process ID and thread ID of the
			thread that initiated the current transaction
			(if any) for the local node's SDR data store.
									*/
/*									*/
/*	Copyright (c) 2019, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include "platform.h"
#include "sdrP.h"
#include "ion.h"

#if defined (ION_LWT)
int	ionxnowner(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	int	interval = a1 ? strtol((char *) a1, NULL, 0) : 0;
	int	count = a2 ? strtol((char *) a2, NULL, 0) : 1;
	int	echo = a3 ? strtol((char *) a3, NULL, 0) : 0;
#else
int	main(int argc, char **argv)
{
	int	interval = argc > 1 ? strtol((char *) argv[1], NULL, 0) : 0;
	int	count = argc > 2 ? strtol((char *) argv[2], NULL, 0) : 1;
	int	echo = argc > 3 ? strtol((char *) argv[3], NULL, 0) : 0;
#endif
	Sdr	sdr;
	char	rpt[80];

	if (interval < 1)
	{
		interval = 1;
	}

	if (count < 1)
	{
		count = 1;
	}

	if (ionAttach() < 0)
	{
		putErrmsg("Can't attach to ION.", NULL);
		return -1;
	}

	sdr = getIonsdr();
	if (sdr == NULL)
	{
		putErrmsg("Can't access SDR.", NULL);
		return -1;
	}

	while (1)
	{
		isprintf(rpt, sizeof rpt, "SDR owned by PID %d thread "
				UVAST_FIELDSPEC ".", sdr->sdr->sdrOwnerTask,
				(uvast) (sdr->sdr->sdrOwnerThread));
		PUTS(rpt);
		if (echo)
		{
			writeMemo(rpt);
		}

		count--;
		if (count < 1)
		{
			break;
		}

		snooze(interval);
	}

	return 0;
}
