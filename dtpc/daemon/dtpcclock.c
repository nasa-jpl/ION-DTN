/*
	dtpcclock.c:	Scheduled-event management daemon for DTPC.

	Authors: Giorgos Papastergiou, SPICE
		 Ioannis Alexiadis, SPICE

	Copyright (c) 2011, Space Internetworking Center,
	Democritus University of Thrace. ALL RIGHTS RESERVED.
										*/

#include "dtpcP.h"

static uaddr	_running(uaddr *newValue)
{
	void	*value;
	uaddr	state;
	
	if (newValue)		/*	Changing state.			*/
	{
		value = (void *) (*newValue);
		state = (uaddr) sm_TaskVar(&value);
	}
	else			/*	Just check.			*/
	{
		state = (uaddr) sm_TaskVar(NULL);
	}

	return state;
}

static void	shutDown(int signum)	/*	Stops dtpcclock.	*/
{
	uaddr	stop = 0;

	oK(_running(&stop));	/*	Terminates dtpcclock.		*/
}

static int	updateAdus(Sdr sdr)
{
	DtpcDB		*dtpcConstants = getDtpcConstants();
	Object		aggrElt;
	Object		aduElt;
	Object		profileElt;
	Object		aduObj;
	Object		aggrObj;
	OutAdu		adu;
			OBJ_POINTER(OutAggregator, outAggr);
			OBJ_POINTER(Profile, profile);
	CHKERR(sdr_begin_xn(sdr));
	for (aggrElt = sdr_list_first(sdr, dtpcConstants->outAggregators);
			aggrElt; aggrElt = sdr_list_next(sdr, aggrElt))
	{
		aggrObj = sdr_list_data(sdr, aggrElt);
		GET_OBJ_POINTER(sdr, OutAggregator, outAggr, aggrObj);
		if (outAggr->inProgressAduElt == 0)
		{
			continue;	/*	No ADU in progress.	*/
		}

		aduElt = outAggr->inProgressAduElt;
		aduObj = sdr_list_data(sdr, aduElt);
		sdr_stage(sdr, (char *) &adu, aduObj, sizeof(OutAdu));
		if (adu.ageOfAdu < 0)
		{
			continue;	/*	No items added to ADU.	*/
		}

		adu.ageOfAdu++;
		for (profileElt = sdr_list_first(sdr, dtpcConstants->profiles);
			profileElt; profileElt = sdr_list_next(sdr, profileElt))
		{
			GET_OBJ_POINTER(sdr, Profile, profile,
				sdr_list_data(sdr, profileElt));
			if (outAggr->profileID == profile->profileID)
			{
				break;	/*	Found matching profile.	*/
			}
		}

		sdr_write(sdr, aduObj, (char *) &adu, sizeof(OutAdu));
		if (adu.ageOfAdu >= profile->aggrTimeLimit)
		{
			if (createAdu(profile, aduObj, aduElt) < 0)
			{
				putErrmsg("Can't send finished adu.", NULL);
				sdr_cancel_xn(sdr);
				return -1;
			}

			if (initOutAdu(profile, aggrObj, aggrElt, &aduObj,
					&aduElt) < 0)
			{
				putErrmsg("Can't stop aggregation for adu.",
						NULL);
				sdr_cancel_xn(sdr);
				return -1;
			}
		}
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Could not scan outbound Adus.", NULL);
		return -1;
	}
	return 0;
}

static int	handleEvents(Sdr sdr, time_t currentTime)
{
	DtpcDB		*dtpcConstants = getDtpcConstants();
	Object		elt;
	Object		eventObj;
	int		result;
			OBJ_POINTER(DtpcEvent, event);

	while (1)
	{
		CHKERR(sdr_begin_xn(sdr));
		elt = sdr_list_first(sdr, dtpcConstants->events);
		if (elt == 0)	/* 	Event list is empty.		*/
		{
			sdr_exit_xn(sdr);
			return 0;
		}	

		eventObj = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, DtpcEvent, event, eventObj);
		if (event->scheduledTime > currentTime)
		{
			/*	This is the first future event.		*/

			sdr_exit_xn(sdr);
			return 0;
		}

		switch (event->type)
		{
		case ResendAdu:
			result = resendAdu(sdr, event->aduElt, currentTime);
			break;		/*	Out of switch.		*/

		case DeleteAdu:
			deleteAdu(sdr, event->aduElt);
			if ((getDtpcVdb())->watching & WATCH_m)
			{
				putchar('m');
				fflush(stdout);
			}

			result = 0;
			break;		/*	Out of switch.		*/

		case DeleteGap:
			deletePlaceholder(sdr, event->aduElt);
			if ((getDtpcVdb())->watching & WATCH_expire)
			{
				putchar('*');
				fflush(stdout);
			}

			result = parseInAdus(sdr);
			break;		/*	Out of switch.		*/

		default:		/*	Spurious event; erase.	*/
			sdr_free(sdr, eventObj);
			sdr_list_delete(sdr, elt, NULL, NULL);
			result = 0;	/*	Event is ignored.	*/
		}

		if (result !=0)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("Failed handling Dtpc event.", NULL);
			return result;
		}

		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Failed handling Dtpc event.", NULL);
			return -1;
		}
	}
}


#if defined (ION_LWT)
int	dtpcclock(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
#else
int	main(int argc, char *argv[])
{
#endif
	Sdr	sdr;
	uaddr	state = 1;
	time_t	currentTime;

	if (dtpcInit() < 0)
	{
		putErrmsg("dtpcclock can't initialize DTPC.", NULL);
		return -1;
	}

	sdr = getIonsdr();
	isignal(SIGTERM, shutDown);

	/* Main loop: wait one second, then parse all events and ADUs.	*/

	oK(_running(&state));
	writeMemo("[i] dtpcclock is running.");

	while (_running(NULL))
	{
		snooze(1);
		currentTime = getCtime();

		/*	Parse list of events				*/

		if (handleEvents(sdr, currentTime) < 0)
		{
			state = 0;	/*	Terminate loop.		*/
			oK(_running(&state));
		}

		/*	Update age for all outbound ADUs		*/

		if (updateAdus(sdr) < 0)
		{
			state = 0;	/*	Terminate loop.		*/
			oK(_running(&state));
		}
	}

	writeErrmsgMemos();
	writeMemo("[i] dtpcclock has ended.");
	ionDetach();
	return 0;
}
