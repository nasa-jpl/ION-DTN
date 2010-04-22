/*
	cfdpclock.c:	scheduled-event management daemon for CFDP.

	Author: Scott Burleigh, JPL

	Copyright (c) 2007, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include "cfdpP.h"

static int	_running(int *newValue)
{
	static int	state;
	
	if (newValue)
	{
		if (*newValue == 1)
		{
			state = 1;
			sm_TaskVarAdd(&state);
		}
		else
		{
			state = 0;
		}
	}

	return state;
}

static void	shutDown()	/*	Commands cfdpclock termination.	*/
{
	int	stop = 0;

	oK(_running(&stop));	/*	Terminates cfdpclock.		*/
}

static int	scanInFdus(Sdr sdr, time_t currentTime)
{
	CfdpDB		*cfdpConstants;
	Object		entityElt;
			OBJ_POINTER(Entity, entity);
	Object		elt;
	Object		nextElt;
	Object		fduObj;
			OBJ_POINTER(InFdu, fdu);
	CfdpHandler	handler;

	cfdpConstants = getCfdpConstants();
	sdr_begin_xn(sdr);
	for (entityElt = sdr_list_first(sdr, cfdpConstants->entities);
			entityElt; entityElt = sdr_list_next(sdr, entityElt))
	{
		GET_OBJ_POINTER(sdr, Entity, entity, sdr_list_data(sdr,
				entityElt));
		for (elt = sdr_list_first(sdr, entity->inboundFdus); elt;
				elt = nextElt)
		{
			nextElt = sdr_list_next(sdr, elt);
			fduObj = sdr_list_data(sdr, elt);
			GET_OBJ_POINTER(sdr, InFdu, fdu, fduObj);
			if (fdu->eofReceived && fdu->checkTime < currentTime)
			{
				sdr_stage(sdr, NULL, fduObj, 0);
				fdu->checkTimeouts++;
				fdu->checkTime
					+= cfdpConstants->checkTimerPeriod;
				sdr_write(sdr, fduObj, (char *) fdu,
						sizeof(InFdu));
			}

			if (fdu->checkTimeouts
				> cfdpConstants->checkTimeoutLimit)
			{
				if (handleFault(&(fdu->transactionId),
					CfdpCheckLimitReached, &handler) < 0)
				{
					sdr_cancel_xn(sdr);
					putErrmsg("Can't handle check limit \
reached.", NULL);
					return -1;
				}
			}
		}
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("cfdpclock failed scanning inbound FDUs.", NULL);
		return -1;
	}

	return 0;
}

static int	scanOutFdus(Sdr sdr, time_t currentTime)
{
	CfdpDB	*cfdpConstants;
	Object	elt;
	Object	nextElt;
	Object	fduObj;
		OBJ_POINTER(OutFdu, fdu);
	Object	elt2;

	cfdpConstants = getCfdpConstants();
	sdr_begin_xn(sdr);
	for (elt = sdr_list_first(sdr, cfdpConstants->outboundFdus); elt;
			elt = nextElt)
	{
		nextElt = sdr_list_next(sdr, elt);
		fduObj = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, OutFdu, fdu, fduObj);
		if (fdu->state == FduCanceled)
		{
			while (1)
			{
				elt2 = sdr_list_first(sdr, fdu->extantPdus);
				if (elt2 == 0)
				{
					break;
				}

				if (bp_cancel(sdr_list_data(sdr, elt2)) < 0)
				{
					sdr_cancel_xn(sdr);
					putErrmsg("Can't cancel bundle.", NULL);
					return -1;
				}

				/*	Note: bp_cancel destroys the
				 *	referenced bundle, in the course
				 *	of which all tracking elts are
				 *	destroyed -- including the one
				 *	that we used to navigate to the
				 *	bundle.  elt2 is now gone.	*/
			}

			/*	Simulate completion of transmission.	*/

			sdr_stage(sdr, NULL, fduObj, 0);
			fdu->progress = fdu->fileSize;
			sdr_write(sdr, fduObj, (char *) fdu, sizeof(OutFdu));
		}

		/*	If the entire file has been transmitted and
		 *	all resulting bundles have been either
		 *	delivered or destroyed (e.g., due to TTL
		 *	expiration) -- then destroy the FDU.		*/

		if (fdu->progress == fdu->fileSize
		&& sdr_list_length(sdr, fdu->extantPdus) == 0)
		{
			destroyOutFdu(fdu, fduObj, elt);
		}
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("cfdpclock failed scanning outbound FDUs.", NULL);
		return -1;
	}

	return 0;
}

#if defined (VXWORKS) || defined (RTEMS)
int	cfdpclock(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
#else
int	main(int argc, char *argv[])
{
#endif
	Sdr	sdr;
	int	state = 1;
	time_t	currentTime;

	if (cfdpInit() < 0 || bp_attach() < 0)
	{
		putErrmsg("cfdpclock can't initialize CFDP.", NULL);
		return -1;
	}

	sdr = getIonsdr();
	isignal(SIGTERM, shutDown);

	/*	Main loop: wait one second, then scan all FDUS.		*/

	oK(_running(&state));
	writeMemo("[i] cfdpclock is running.");
	while (_running(NULL))
	{
		snooze(1);
		currentTime = getUTCTime();

		/*	Update check counts for inbound FDUs.		*/

		if (scanInFdus(sdr, currentTime) < 0)
		{
			putErrmsg("Can't scan inbound FDUs.", NULL);
			state = 0;	/*	Terminate loop.		*/
			oK(_running(&state));
			continue;
		}

		/*	Clean out completed outbound FDUs.		*/

		if (scanOutFdus(sdr, currentTime) < 0)
		{
			putErrmsg("Can't scan inbound FDUs.", NULL);
			state = 0;	/*	Terminate loop.		*/
			oK(_running(&state));
		}
	}

	writeErrmsgMemos();
	writeMemo("[i] cfdpclock has ended.");
	ionDetach();
	return 0;
}
