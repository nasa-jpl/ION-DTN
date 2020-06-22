/*
	bpclock.c:	scheduled-event management daemon for BP.

	Author: Scott Burleigh, JPL

	Copyright (c) 2004, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include "bpP.h"
#include "bibe.h"
#include "sdrhash.h"
#include "smlist.h"

#ifndef MAX_CLO_INACTIVITY
#define	MAX_CLO_INACTIVITY	(3)
#endif

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

typedef struct
{
	char	protocolName[MAX_CL_PROTOCOL_NAME_LEN + 1];
	char	ductName[MAX_CL_DUCT_NAME_LEN + 1];
} DuctRef;

static int	flushOutduct(DuctRef *dr)
{
	Sdr		sdr = getIonsdr();
			OBJ_POINTER(Outduct, outduct);
	VOutduct	*vduct;
	PsmAddress	vductElt;
	int		flushCount;
	Object		elt;
	Object		bundleObj;
	Bundle		bundle;

	/*	Any bundle previously enqueued for transmission via
	 *	this outduct that has not yet been transmitted is
	 *	assumed to be waiting on to a stuck CLO, and is
	 *	therefore placed in the limbo list for eventual
	 *	reforwarding.  We don't reforward immediately because
	 *	the presumably stuck outduct is not detached from
	 *	the plan, so upon presentation to the CLM daemon the
	 *	bundle would simply be allocated to the same stuck
	 *	outduct again.						*/

	while (1)
	{
		CHKERR(sdr_begin_xn(sdr));
		findOutduct(dr->protocolName, dr->ductName, &vduct, &vductElt);
		if (vductElt == 0)	/*	Duct removed.		*/
		{
			sdr_exit_xn(sdr);
			return 0;	/*	Done with this duct.	*/
		}

		GET_OBJ_POINTER(sdr, Outduct, outduct, vduct->outductElt);
		flushCount = 0;
		while (1)
		{
			elt = sdr_list_first(sdr, outduct->xmitBuffer);
			if (elt == 0)	/*	No bundles to flush.	*/
			{
				break;
			}

			bundleObj = sdr_list_data(sdr, elt);
			sdr_read(sdr, (char *) &bundle, bundleObj,
					sizeof(Bundle));
			if (bundle.ductXmitElt)
			{
				sdr_list_delete(sdr, bundle.ductXmitElt, NULL,
						NULL);
				bundle.ductXmitElt = 0;
			}
			else
			{
				sdr_list_delete(sdr, elt, NULL, NULL);
			}

			/*	(Normally the bundle's ductXmitElt
			 *	== the elt by which we accessed this
			 *	bundle, so need not sdr_delete(elt).)	*/

			if (bundle.overdueElt)
			{
				destroyBpTimelineEvent(bundle.overdueElt);
				bundle.overdueElt = 0;
			}

			if (enqueueToLimbo(&bundle, bundleObj) < 0)
			{
				sdr_cancel_xn(sdr);
				putErrmsg("Failed enqueuing to limbo list.",
						NULL);
				return -1;
			}

			flushCount++;
			if (flushCount > 999)
			{
				break;
			}
		}

		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("flushOutduct failed.", NULL);
			return -1;
		}
	}

	return 0;
}

static void	destroyDR(LystElt elt, void *userData)
{
	MRELEASE(lyst_data(elt));
}

static int	flushOutducts(Sdr sdr, time_t currentTime)
{
	PsmPartition	bpwm = getIonwm();
	BpVdb		*vdb = getBpVdb();
	Lyst		ductsToFlush;
	PsmAddress	elt1;
	DuctRef		*dr;
	LystElt		elt2;
	VOutduct	*vduct;
	PsmAddress	vductElt;

	/*	May be multiple transactions, so we can't safely just
	 *	step through the outducts list: an outduct might be
	 *	deleted between transactions.				*/

	ductsToFlush = lyst_create_using(getIonMemoryMgr());
	CHKERR(ductsToFlush);
	lyst_delete_set(ductsToFlush, destroyDR, NULL);
	CHKERR(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	for (elt1 = sm_list_first(bpwm, vdb->outducts); elt1;
			elt1 = sm_list_next(bpwm, elt1))
	{
		vduct = (VOutduct *) psp(bpwm, sm_list_data(bpwm, elt1));
		dr = (DuctRef *) MTAKE(sizeof(DuctRef));
		if (dr == NULL)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("No memory for duct reference.", NULL);
			lyst_destroy(ductsToFlush);
			return -1;
		}

		istrcpy(dr->protocolName, vduct->protocolName,
				sizeof(dr->protocolName));
		istrcpy(dr->protocolName, vduct->protocolName,
				sizeof(dr->protocolName));
		elt2 = lyst_insert_last(ductsToFlush, dr);
		if (elt2 == NULL)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("No memory for lyst element.", NULL);
			lyst_destroy(ductsToFlush);
			return -1;
		}
	}

	sdr_exit_xn(sdr);		/*	Unlock memory.		*/

	/*	Have now got a stable list of outducts to flush.	*/

	while (1)
	{
		elt2 = lyst_first(ductsToFlush);
		if (elt2 == NULL)
		{
			break;
		}

		dr = (DuctRef *) lyst_data(elt2);
		findOutduct(dr->protocolName, dr->ductName, &vduct, &vductElt);
		if (vductElt)
		{
			if ((currentTime - vduct->timeOfLastXmit)
					> MAX_CLO_INACTIVITY)
			{
				/*	Outduct might be stuck.		*/

				if (flushOutduct(dr) < 0)
				{
					lyst_destroy(ductsToFlush);
					return -1;
				}
			}
		}

		lyst_delete(elt2);
	}

	lyst_destroy(ductsToFlush);
	return 0;
}

static int	flushLimbo(Sdr sdr, Object limboList, time_t currentTime,
			time_t *previousFlush)
{
	int	bundlesToFlush;
	int	batchesNeeded;
	int	elapsed;
	int	batchesAvbl;
	int	bundlesFlushed;
	int	flushCount;
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
	 *	enough to justify flushing ALL bundles currently in
	 *	limbo (at 64 flushes per second of elapsed time).	*/

	bundlesToFlush = sdr_list_length(sdr, limboList);
	batchesNeeded = (bundlesToFlush >> 8) & 0x00ffffff;
	if ((batchesNeeded << 8) < bundlesToFlush)
	{
		/*	Pick up fractional batch needing release.	*/

		batchesNeeded++;
	}

	if (*previousFlush == 0)
	{
		*previousFlush = currentTime;	/*	Initialize.	*/
	}

	elapsed = currentTime - *previousFlush;
	batchesAvbl = (elapsed >> 2) & 0x3fffffff;
	if (batchesNeeded > 0 && batchesAvbl >= batchesNeeded)
	{
		/*	Flush the limbo list.  Flush one batch at
		 *	a time, to keep log file lengths reasonable.	*/

		bundlesFlushed = 0;
		while (bundlesFlushed < bundlesToFlush)
		{
			/*	Flush 256 bundles at front of list.	*/

			CHKERR(sdr_begin_xn(sdr));
			flushCount = 0;
			for (elt = sdr_list_first(sdr, limboList); elt;
					elt = nextElt)
			{
				nextElt = sdr_list_next(sdr, elt);
				bundlesFlushed++;
				switch (releaseFromLimbo(elt, 0))
				{
				case 0:		/*	Suspended.	*/
					break;	/*	No log impact.	*/
	
				case 1:		/*	Successful.	*/
					flushCount++;
					break;
	
				default:
					putErrmsg("Failed releasing bundle \
from limbo.", NULL);
					flushCount = 256;
					bundlesFlushed = bundlesToFlush;
				}

				if (flushCount > 255)
				{
					break;
				}
			}

			if (sdr_end_xn(sdr) < 0)
			{
				putErrmsg("Bundle flush transaction failed.",
						NULL);
				return -1;
			}

			/*	Limbo list might have been shortened
			 *	by bundle expiration.			*/

			if (sdr_list_length(sdr, limboList) < 1)
			{
				break;
			}
		}

		*previousFlush = currentTime;
	}

	return 0;
}

#if defined (ION_LWT)
int	bpclock(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
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
		currentTime = getCtime();
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

		/*	Flush all Outducts that appear to be stuck.	*/

		if (flushOutducts(sdr, currentTime) < 0)
		{
			putErrmsg("Can't flush outducts.", NULL);
			state = 0;
			oK(_running(&state));
		}

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
