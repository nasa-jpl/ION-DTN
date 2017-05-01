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

static void	shutDown()	/*	Commands bpclock termination.	*/
{
	uaddr	stop = 0;

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
			if ((getBpVdb())->watching & WATCH_retransmit)
			{
				iwatch('$');
			}

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

static void	detectCurrentTopologyChanges(Sdr sdr)
{
	PsmPartition	ionwm = getIonwm();
	IonVdb		*ionvdb = getIonVdb();
	PsmAddress	elt;
	IonNeighbor	*neighbor;
	int		mustPrintStats = 0;

	CHKVOID(sdr_begin_xn(sdr));
	for (elt = sm_rbt_first(ionwm, ionvdb->neighbors); elt;
			elt = sm_rbt_next(ionwm, elt))
	{
		neighbor = (IonNeighbor *) psp(ionwm, sm_rbt_data(ionwm, elt));
		CHKVOID(neighbor);
		if (neighbor->xmitRate != neighbor->prevXmitRate)
		{
#ifndef ION_NOSTATS
			if (neighbor->nodeNbr != getOwnNodeNbr())
			{
				/*	We report transmission
				 *	statistics as necessary.	*/

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
			neighbor->prevXmitRate = neighbor->xmitRate;
			neighbor->xmitThrottle.nominalRate = neighbor->xmitRate;
			neighbor->xmitThrottle.capacity = neighbor->xmitRate;
		}

		if (neighbor->recvRate != neighbor->prevRecvRate)
		{
#ifndef ION_NOSTATS
			if (neighbor->nodeNbr != getOwnNodeNbr())
			{
				/*	We report transmission
				 *	statistics as necessary.	*/

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
			neighbor->prevRecvRate = neighbor->recvRate;
		}
	}

	if (mustPrintStats)
	{
		reportAllStateStats();
	}

	oK(sdr_end_xn(sdr));
}

static void	applyRateControl(Sdr sdr)
{
	PsmPartition	ionwm = getIonwm();
	BpVdb		*bpvdb = getBpVdb();
	PsmAddress	elt;
	VPlan		*vplan;
	Throttle	*throttle;

	/*	Rate control is regulated and effected at node --
	 *	that is, neighbor egress plan -- granularity.
	 *	We want to blip the throttles of all neighboring
	 *	nodes once per second.
	 *
	 *	However, not all egress plans can be matched to
	 *	Neighbors: a given plan might be for a neighbor
	 *	that is only identified by a dtn-scheme endpoint
	 *	ID, or it might be cited in the egress plan for a
	 *	node for which there are no contacts in the contact
	 *	plan (hence no Neighbor has been created).  When
	 *	this is the case, we must drop back to simply using
	 *	the nominal data rate arbitrarily asserted for the
	 *	plan.							*/

	CHKVOID(sdr_begin_xn(sdr));

	/*	Enable some bundle transmission on each egress plan.	*/

	for (elt = sm_list_first(ionwm, bpvdb->plans); elt;
			elt = sm_list_next(ionwm, elt))
	{
		vplan = (VPlan *) psp(ionwm, sm_list_data(ionwm, elt));
		throttle = applicableThrottle(vplan);
		CHKVOID(throttle);

		/*	If throttle is rate controlled, added capacity
		 *	is 1 second's worth of transmission.  If not,
		 *	no change.					*/

		if (throttle->nominalRate > 0)
		{
			throttle->capacity += throttle->nominalRate;
			if (throttle->capacity > throttle->nominalRate)
			{
				throttle->capacity = throttle->nominalRate;
			}
		}
	}

	oK(sdr_end_xn(sdr));
}

static int	flushLimbo(Sdr sdr, Object limboList, time_t currentTime,
			time_t *previousFlush)
{
	int	length;
	int	batchesNeeded;
	int	elapsed;
	int	batchesAvbl;
	Object	elt;
	Object	nextElt;

	/*	We don't want this feature to slow down the operation
	 *	of the node, so we limit it.  We flush at most once
	 *	every 4 seconds, and each 4 seconds we are okay with
	 *	releasing a "batch" of up to 256 bundles in limbo.
	 *	But we can't just release one batch every 4 seconds
	 *	because we may miss some or release some multiple
	 *	times, because reforwarding may put one or more
	 *	bundles back in limbo.  So we defer flushing until
	 *	the total elapsed time since the previous flush is
	 *	enough to justify flushing all bundles currently in
	 *	limbo.							*/

	CHKERR(sdr_begin_xn(sdr));
	length = sdr_list_length(sdr, limboList);
	batchesNeeded = (length >> 8) & 0x00ffffff;
	if (*previousFlush == 0)
	{
		*previousFlush = currentTime;	/*	Initialize.	*/
	}

	elapsed = currentTime - *previousFlush;
	batchesAvbl = (elapsed >> 2) & 0x3fffffff;
	if (batchesAvbl > 0 && batchesAvbl >= batchesNeeded)
	{
		for (elt = sdr_list_first(sdr, limboList); elt; elt = nextElt)
		{
			nextElt = sdr_list_next(sdr, elt);
			if (releaseFromLimbo(elt, 0) < 0)
			{
				putErrmsg("Failed releasing bundle from limbo.",
						NULL);
				break;
			}
		}

		*previousFlush = currentTime;
	}

	return sdr_end_xn(sdr);
}

#if defined (ION_LWT)
int	bpclock(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
#else
int	main(int argc, char *argv[])
{
#endif
	Sdr	sdr;
	BpDB	*bpConstants;
	uaddr	state = 1;
	time_t	currentTime;
	time_t	previousFlush = 0;

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

		/*	Also detect current topology changes resulting
		 *	from rate changes imposed per the contact plan.	*/

		detectCurrentTopologyChanges(sdr);

		/*	Then apply rate control.			*/

		applyRateControl(sdr);

		/*	Finally, possibly give bundles in limbo an
		 *	opportunity to be forwarded, in case an
		 *	Outduct was temporarily stuck.			*/

		if (flushLimbo(sdr, bpConstants->limboQueue, currentTime,
				&previousFlush) < 0)
		{
			putErrmsg("Can't flush limbo queue.", NULL);
			state = 0;
			oK(_running(&state));
		}
	}

	writeErrmsgMemos();
	writeMemo("[i] bpclock has ended.");
	ionDetach();
	return 0;
}
