/*
 *	bsspclock.c:	scheduled-event management daemon for BSSP.
 *
 *	Authors: Sotirios-Angelos Lenas, SPICE
 *		 Scott Burleigh, JPL
 *
 *	Copyright (c) 2013, California Institute of Technology.
 *	Copyright (c) 2013, Space Internetworking Center,
 *	Democritus University of Thrace.
 *	
 *	All rights reserved. U.S. Government and E.U. Sponsorship acknowledged.
 */

#include "bsspP.h"

static uaddr	_running(uaddr *newValue)
{
	void	*value;
	uaddr	state;

	if (newValue)			/*	Changing state.		*/
	{
		value = (void *) (*newValue);
		state = (uaddr) sm_TaskVar(&value);
	}
	else				/*	Just check.		*/
	{
		state = (uaddr) sm_TaskVar(NULL);
	}

	return state;
}

static void	shutDown(int signum)
{
	uaddr	stop = 0;

	oK(_running(&stop));	/*	Terminates bsspclock.		*/
}

static int	dispatchEvents(Sdr sdr, Object events, time_t currentTime)
{
	Object		elt;
	Object		eventObj;
	BsspEvent	event;
	int		result;

	while (1)
	{
		CHKERR(sdr_begin_xn(sdr));
		elt = sdr_list_first(sdr, events);
		if (elt == 0)	/*	No more events to dispatch.	*/
		{
			sdr_exit_xn(sdr);
			return 0;
		}

		eventObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &event, eventObj, sizeof(BsspEvent));
		if (event.scheduledTime > currentTime)
		{
			/*	This is the first future event.		*/

			sdr_exit_xn(sdr);
			return 0;
		}

		sdr_free(sdr, eventObj);
		sdr_list_delete(sdr, elt, NULL, NULL);
		switch (event.type)
		{
		case BsspResendBlock:
			result = bsspResendBlock(event.refNbr2);
			break;		/*	Out of switch.		*/

		default:		/*	Spurious event.		*/
			result = 0;	/*	Event is ignored.	*/
		}

		if (result < 0)		/*	Dispatching failed.	*/
		{
			sdr_cancel_xn(sdr);
			putErrmsg("failed handing BSSP event", NULL);
			return result;
		}

		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("failed dispatching BSSP event", NULL);
			return -1;
		}
	}
}

static int	manageLinks(Sdr sdr, time_t currentTime)
{
	PsmPartition	ionwm = getIonwm();
	BsspVdb		*bsspvdb = getBsspVdb();
	IonVdb		*ionvdb = getIonVdb();
	PsmAddress	elt;
	BsspVspan	*vspan;
	IonNeighbor	*neighbor;
	PsmAddress	nextElt;
	unsigned int	priorXmitRate;

	CHKERR(sdr_begin_xn(sdr));
	for (elt = sm_list_first(ionwm, bsspvdb->spans); elt;
			elt = sm_list_next(ionwm, elt))
	{
		vspan = (BsspVspan *) psp(ionwm, sm_list_data(ionwm, elt));

		/*	Find Neighbor object encapsulating the current
		 *	known state of this BSSP engine.		*/

		neighbor = findNeighbor(ionvdb, vspan->engineId, &nextElt);
		if (neighbor == NULL)
		{
			neighbor = addNeighbor(ionvdb, vspan->engineId);
			if (neighbor == NULL)
			{
				putErrmsg("Can't update span.", NULL);
				return -1;
			}
		}

		if (neighbor->xmitRate == 0)
		{
			if (vspan->localXmitRate > 0)
			{
				vspan->localXmitRate = 0;
				bsspStopXmit(vspan);
			}
		}
		else
		{
			if (vspan->localXmitRate == 0)
			{
				vspan->localXmitRate = neighbor->xmitRate;
				bsspStartXmit(vspan);
			}
		}

		if (neighbor->fireRate == 0)
		{
			if (vspan->remoteXmitRate > 0)
			{
				priorXmitRate = vspan->remoteXmitRate;
				vspan->remoteXmitRate = 0;
				if (bsspSuspendTimers(vspan, elt, currentTime,
						priorXmitRate))
				{
					putErrmsg("Can't manage links.", NULL);
					return -1;
				}
			}
		}
		else
		{
			if (vspan->remoteXmitRate == 0)
			{
				vspan->remoteXmitRate = neighbor->fireRate;
				if (bsspResumeTimers(vspan, elt, currentTime,
						vspan->remoteXmitRate))
				{
					putErrmsg("Can't manage links.", NULL);
					return -1;
				}
			}
		}

		vspan->owltInbound = neighbor->owltInbound;
		vspan->owltOutbound = neighbor->owltOutbound;
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("bsspclock failed managing links.", NULL);
		return -1;
	}

	return 0;
}

#if defined (ION_LWT)
int	bsspclock(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
#else
int	main(int argc, char *argv[])
{
#endif
	Sdr	sdr;
	BsspDB	*bsspConstants;
	uaddr	state = 1;
	time_t	currentTime;

	if (bsspInit(0) < 0)
	{
		putErrmsg("bsspclock can't initialize BSSP.", NULL);
		return 1;
	}
	
	sdr = getIonsdr();
	bsspConstants = getBsspConstants();
	isignal(SIGTERM, shutDown);

	/*	Main loop: wait for event occurrence time, then
	 *	execute applicable events.				*/

	oK(_running(&state));
	writeMemo("[i] bsspclock is running.");
	while (_running(NULL))
	{
		/*	Sleep for 1 second, then dispatch all events
		 *	whose executions times have now been reached.	*/

		snooze(1);
		currentTime = getCtime();

		/*	Infer link state changes from rate changes
		 *	noted in the shared ION database.		*/

		if (manageLinks(sdr, currentTime) < 0)
		{
			putErrmsg("Can't manage links.", NULL);
			state = 0;	/*	Terminate loop.		*/
			oK(_running(&state));
			continue;
		}

		/*	Then dispatch retransmission events, as
		 *	constrained by the new link state.		*/

		if (dispatchEvents(sdr, bsspConstants->timeline, currentTime)
				< 0)
		{
			putErrmsg("Can't dispatch events.", NULL);
			state = 0;	/*	Terminate loop.		*/
			oK(_running(&state));
			continue;
		}
	}

	writeErrmsgMemos();
	writeMemo("[i] bsspclock has ended.");
	ionDetach();
	return 0;
}
