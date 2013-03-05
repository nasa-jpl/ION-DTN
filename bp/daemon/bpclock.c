/*
	bpclock.c:	scheduled-event management daemon for BP.

	Author: Scott Burleigh, JPL

	Copyright (c) 2004, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include "bpP.h"
#include "sdrhash.h"
#include "smlist.h"

#ifdef ENABLE_BPACS
#include "acs.h"		/* provides sendAcs */
#endif /* ENABLE_ACS */

static long	_running(long *newValue)
{
	void	*value;
	long	state;

	if (newValue)			/*	Changing state.		*/
	{
		value = (void *) (*newValue);
		state = (long) sm_TaskVar(&value);
	}
	else				/*	Just check.		*/
	{
		state = (long) sm_TaskVar(NULL);
	}

	return state;
}

static void	shutDown()	/*	Commands bpclock termination.	*/
{
	long	stop = 0;

	oK(_running(&stop));	/*	Terminates bpclock.		*/
}

static int	dispatchEvents(Sdr sdr, Object events, time_t currentTime)
{
	Object	elt;
	Object	eventObj;
		OBJ_POINTER(BpEvent, event);
	int	result;

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
		GET_OBJ_POINTER(sdr, BpEvent, event, eventObj);
		if (event->time > currentTime)
		{
			/*	This is the first future event.		*/

			sdr_exit_xn(sdr);
			return 0;
		}

		switch (event->type)
		{
		case expiredTTL:
			result = bpDestroyBundle(event->ref, 1);

			/*	Note that bpDestroyBundle() always
			 *	erases the bundle's timeline event,
			 *	so we must NOT do so here.		*/

			break;		/*	Out of switch.		*/

		case xmitOverdue:
			result = bpReforwardBundle(event->ref);

			/*	Note that bpReforwardBundle() always
			 *	erases the bundle's xmitOverdue event,
			 *	so we must NOT do so here.		*/

			break;		/*	Out of switch.		*/

		case ctDue:
			result = bpReforwardBundle(event->ref);

			/*	Note that bpReforwardBundle() always
			 *	erases the bundle's ctDue event, so
			 *	we must NOT do so here.			*/

			break;		/*	Out of switch.		*/

#ifdef ENABLE_BPACS
        case csDue:
            result = sendAcs(event->ref);

			/* Note that sendAcs() always erases the 
			 * csDue event, so we must NOT do so
			 * here. */

			break;		/*	Out of switch.		*/
#endif

		default:		/*	Spurious event; erase.	*/
			destroyBpTimelineEvent(elt);
			result = 0;	/*	Event is ignored.	*/
		}

		if (result != 0)	/*	Dispatching failed.	*/
		{
			sdr_cancel_xn(sdr);
			putErrmsg("Failed handing BP event.", NULL);
			return result;
		}

		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Failed dispatching BP event.", NULL);
			return -1;
		}
	}
}

static int	adjustThrottles()
{
	PsmPartition	ionwm = getIonwm();
	IonVdb		*ionvdb = getIonVdb();
	BpVdb		*bpvdb = getBpVdb();
	PsmAddress	elt;
	VOutduct	*outduct;
	uvast		nodeNbr;
	IonNeighbor	*neighbor;
	PsmAddress	nextElt;
	int		mustPrintStats = 0;
	int		delta;
	VInduct		*induct;

	/*	Only the LTP induct and outduct throttles can be
	 *	dynamically adjusted in response to changes in data
	 *	rate between the local node and its neighbors, because
	 *	(currently) there is no mechanism for mapping neighbor
	 *	node number to duct name for any other CL protocol.
	 *	For LTP, duct name is LTP engine number which, by
	 *	convention, is identical to BP node number.  For all
	 *	other CL protocols, duct nominal data rate is initially
	 *	set to the protocol's configured nominal data rate and
	 *	is never subsequently modified.
	 *	
	 *	So, first we find the LTP induct if any.		*/

	for (elt = sm_list_first(ionwm, bpvdb->inducts); elt;
			elt = sm_list_next(ionwm, elt))
	{
		induct = (VInduct *) psp(ionwm, sm_list_data(ionwm, elt));
		if (strcmp(induct->protocolName, "ltp") == 0)
		{
			break;	/*	Found the LTP induct.		*/
		}
	}

	if (elt == 0)		/*	No LTP induct; nothing to do.	*/
	{
		return 0;
	}

	/*	Now update all LTP outducts, and the induct as well,
	 *	inferring the existence of Neighbors in the process.	*/

	for (elt = sm_list_first(ionwm, bpvdb->outducts); elt;
			elt = sm_list_next(ionwm, elt))
	{
		outduct = (VOutduct *) psp(ionwm, sm_list_data(ionwm, elt));
		if (strcmp(outduct->protocolName, "ltp") != 0)
		{
			continue;
		}

		nodeNbr = strtouvast(outduct->ductName);
		neighbor = findNeighbor(ionvdb, nodeNbr, &nextElt);
		if (neighbor == NULL)
		{
			neighbor = addNeighbor(ionvdb, nodeNbr);
			if (neighbor == NULL)
			{
				putErrmsg("Can't adjust outduct throttle.",
						NULL);
				return -1;
			}
		}

		if (neighbor->xmitRate != neighbor->prevXmitRate)
		{
#ifndef ION_NOSTATS
			if (neighbor->nodeNbr != getOwnNodeNbr())
			{
			/*	We report transmission statistics
			 *	as necessary.  NOTE that this
			 *	procedure is based on the assumption
			 *	that the local node is in LTP
			 *	transmission contact with AT MOST
			 *	ONE neighbor at any time.  For more
			 *	complex topologies it will need to
			 *	be redesigned.				*/

				if (neighbor->xmitRate == 0)
				{
					/*	End of xmit contact.	*/
					mustPrintStats = 1;
				}
				else if (neighbor->prevXmitRate == 0)
				{
					/*	Start of xmit contact.	*/
					mustPrintStats = 1;
				}
			}
#endif
			outduct->xmitThrottle.nominalRate = neighbor->xmitRate;
			neighbor->prevXmitRate = neighbor->xmitRate;
		}

		/*	Note that the LTP induct is aggregate; the
		 *	duct's nominal rate is the sum of the rates
		 *	at which all neighbors are expected to be
		 *	transmitting to the local node at any given
		 *	moment.  So we must add the change in rate
		 *	for each known neighbor to the aggregate
		 *	nominal reception rate for the induct.		*/

		if (neighbor->recvRate != neighbor->prevRecvRate)
		{
#ifndef ION_NOSTATS
			if (neighbor->nodeNbr != getOwnNodeNbr())
			{
			/*	We report and clear reception
			 *	statistics as necessary.  NOTE that
			 *	this procedure is based on the
			 *	assumption that the local node is
			 *	in LTP reception contact with
			 *	AT MOST ONE neighbor at any time.
			 *	For more complex topologies it will
			 *	need to be redesigned.			*/

				if (neighbor->recvRate == 0)
				{
					/*	End of recv contact.	*/
					mustPrintStats = 1;
				}
				else if (neighbor->prevRecvRate == 0)
				{
					/*	Start of recv contact.	*/
					mustPrintStats = 1;
				}
			}
#endif
			delta = neighbor->recvRate - neighbor->prevRecvRate;
			induct->acqThrottle.nominalRate += delta;
			neighbor->prevRecvRate = neighbor->recvRate;
		}
	}

	if (mustPrintStats)
	{
		reportAllStateStats();
	}

	return 0;
}

static void	applyRateControl(Sdr sdr)
{
	PsmPartition	ionwm = getIonwm();
	BpVdb		*vdb = getBpVdb();
	Throttle	*throttle;
	PsmAddress	elt;
	VInduct		*induct;
	VOutduct	*outduct;

	CHKVOID(sdr_begin_xn(sdr));

	/*	Enable some bundle acquisition.				*/

	for (elt = sm_list_first(ionwm, vdb->inducts); elt;
			elt = sm_list_next(ionwm, elt))
	{
		induct = (VInduct *) psp(ionwm, sm_list_data(ionwm, elt));
		throttle = &(induct->acqThrottle);
		if (throttle->nominalRate < 0)
		{
			continue;	/*	Not rate-controlled.	*/
		}

		if (throttle->capacity <= throttle->nominalRate)
		{
			throttle->capacity += throttle->nominalRate;
		}

		if (throttle->capacity > 0)
		{
			sm_SemGive(throttle->semaphore);
		}
	}

	/*	Enable some bundle transmission.			*/

	for (elt = sm_list_first(ionwm, vdb->outducts); elt;
			elt = sm_list_next(ionwm, elt))
	{
		outduct = (VOutduct *) psp(ionwm, sm_list_data(ionwm, elt));
		throttle = &(outduct->xmitThrottle);
		if (throttle->nominalRate < 0)
		{
			continue;	/*	Not rate-controlled.	*/
		}

		if (throttle->capacity <= throttle->nominalRate)
		{
			throttle->capacity += throttle->nominalRate;
		}

		if (throttle->capacity > 0)
		{
			sm_SemGive(throttle->semaphore);
		}
	}

	oK(sdr_end_xn(sdr));
}

#if defined (VXWORKS) || defined (RTEMS) || defined (bionic)
int	bpclock(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
#else
int	main(int argc, char *argv[])
{
#endif
	Sdr	sdr;
	BpDB	*bpConstants;
	long	state = 1;
	time_t	currentTime;

	if (bpAttach() < 0)
	{
		putErrmsg("bpclock can't attach to BP.", NULL);
		return 1;
	}

	sdr = getIonsdr();
	bpConstants = getBpConstants();
	isignal(SIGTERM, shutDown);

	/*	Main loop: wait for event occurrence time, then
	 *	execute applicable events.				*/

	oK(_running(&state));
	writeMemo("[i] bpclock is running.");
	while (_running(NULL))
	{
		/*	Sleep for 1 second, then dispatch all events
		 *	whose executions times have now been reached.	*/

		snooze(1);
		currentTime = getUTCTime();
		if (dispatchEvents(sdr, bpConstants->timeline, currentTime) < 0)
		{
			putErrmsg("Can't dispatch events.", NULL);
			state = 0;	/*	Terminate loop.		*/
			oK(_running(&state));
			continue;
		}

		/*	Also adjust throttles in response to rate
		 *	changes noted in the shared ION database.	*/

		if (adjustThrottles() < 0)
		{
			putErrmsg("Can't adjust throttles.", NULL);
			state = 0;	/*	Terminate loop.		*/
			oK(_running(&state));
			continue;
		}

		/*	Then apply rate control.			*/

		applyRateControl(sdr);
	}

	writeErrmsgMemos();
	writeMemo("[i] bpclock has ended.");
	ionDetach();
	return 0;
}
