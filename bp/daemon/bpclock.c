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
	IonVdb		*ionvdb = getIonVdb();
	BpVdb		*bpvdb = getBpVdb();
	PsmAddress	elt;
	IonNeighbor	*neighbor;
	Throttle	*throttle;
	VOutduct	*outduct;

	/*	Rate control is effected at Outduct granularity
	 *	but is normally regulated at (neighboring) Node
	 *	granularity, because it is nominally controlled
	 *	by the contact plan and the contact information
	 *	in the contact plan is at Node granularity.  So we
	 *	blip the throttles of all Neighbors once per second.
	 *
	 *	However, not all Outducts can be matched to Neighbors:
	 *	an Outduct might be cited only in dtn-scheme egress
	 *	plans, or it might be for a "promiscuous" protocol,
	 *	or it might be cited in the egress plan for a node
	 *	for which there are no contacts in the contact plan
	 *	(hence no Neighbor has been created).  When this is
	 *	the case, we must drop back to simply using the
	 *	nominal data rate of the Protocol for the Outduct
	 *	to regulate transmission.  For this purpose we also
	 *	blip the throttles of all Outducts once per second,
	 *	in case they are needed.
	 *
	 *	Whenever rate control is enacted at a given Outduct,
	 *	we first try to identify the corresponding Neighbor
	 *	and use its throttle to control the duct.  When no
	 *	Neighbor can be identified, we use the duct's own
	 *	throttle instead.					*/

	CHKVOID(sdr_begin_xn(sdr));

	/*	Enable some bundle transmission to each Neighbor.	*/

	for (elt = sm_rbt_first(ionwm, ionvdb->neighbors); elt;
			elt = sm_rbt_next(ionwm, elt))
	{
		neighbor = (IonNeighbor *) psp(ionwm, sm_rbt_data(ionwm, elt));
		throttle = &(neighbor->xmitThrottle);

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

	/*	Enable some bundle transmission on each Outduct.	*/

	for (elt = sm_list_first(ionwm, bpvdb->outducts); elt;
			elt = sm_list_next(ionwm, elt))
	{
		outduct = (VOutduct *) psp(ionwm, sm_list_data(ionwm, elt));
		throttle = &(outduct->xmitThrottle);

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

		/*	Also detect current topology changes resulting
		 *	from rate changes imposed per the contact plan.	*/

		detectCurrentTopologyChanges(sdr);

		/*	Then apply rate control.			*/

		applyRateControl(sdr);
	}

	writeErrmsgMemos();
	writeMemo("[i] bpclock has ended.");
	ionDetach();
	return 0;
}
