/*
	ionunlock.c:	Safely unlocks an ION system that is currently
			locked up due to a thread exiting while it is
			the owner of the current SDR transaction.
									*/
/*									*/
/*	Copyright (c) 2017, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include "platform.h"
#include "sdrP.h"
#include "ion.h"

#if defined (ION_LWT)
int	ionunlock(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char	*sdrName = a1 ? (char *) a1 : "ion";
#else
int	main(int argc, char **argv)
{
	char	*sdrName = argc > 1 ? argv[1] : "ion";
#endif
	Sdr	sdrv;

	if (sdr_initialize(0, NULL, SM_NO_KEY, NULL) < 0)
	{
		putErrmsg("Can't initialize the SDR system.", NULL);
		return -1;
	}

	sdrv = sdr_start_using(sdrName);
	if (sdrv == NULL)
	{
		putErrmsg("Can't start using SDR.", sdrName);
		return -1;
	}

	/*	Hijack and cancel the current transaction, i.e.,
	 *	impersonate the current owner of the ION mutex.		*/

	if (sdrv->sdr == NULL
	|| sdrv->sdr->sdrOwnerTask == -1)
	{
		writeMemoNote("[!] ionunlock unnecessary; exiting.", sdrName);
		return 0;
	}

	sdrv->sdr->sdrOwnerTask = sm_TaskIdSelf();
	sdrv->sdr->sdrOwnerThread = pthread_self();
	sdrv->sdr->xnDepth = 1;
	sdr_cancel_xn(sdrv);
	sdr_stop_using(sdrv);
	writeMemo("[i] ionunlock: finished unlocking ION.");
	return 0;
}
