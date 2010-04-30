/*
	bpclock.c:	scheduled-event management daemon for BP.

	Author: Scott Burleigh, JPL

	Copyright (c) 2004, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include "bpP.h"
#include "smlist.h"

extern void	manageProductionThrottle(BpVdb *vdb);

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

static void	shutDown()	/*	Commands bpclock termination.	*/
{
	int	stop = 0;

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
		sdr_begin_xn(sdr);
		CHKERR(ionLocked());	/*	In case of killm.	*/
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

		default:		/*	Spurious event; erase.	*/
			sdr_free(sdr, eventObj);
			sdr_list_delete(sdr, elt, NULL, NULL);
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
	unsigned long	nodeNbr;
	IonNeighbor	*neighbor;
	PsmAddress	nextElt;
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

		nodeNbr = atol(outduct->ductName);
		neighbor = findNeighbor(ionvdb, nodeNbr, &nextElt);
		if (neighbor == NULL)
		{
			neighbor = addNeighbor(ionvdb, nodeNbr, nextElt);
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
			/*	We report and clear transmission
			 *	statistics as necessary.  NOTE that
			 *	this procedure is based on the
			 *	assumption that the local node is
			 *	in LTP transmission contact with
			 *	AT MOST ONE neighbor at any time.
			 *	For more complex topologies it will
			 *	need to be redesigned.			*/

				if (neighbor->xmitRate == 0)
				{
					/*	End of xmit contact.	*/
					reportAllStateStats();
					clearAllStateStats();
				}
				else if (neighbor->prevXmitRate == 0)
				{
					/*	Start of xmit contact.	*/
					reportAllStateStats();
					clearAllStateStats();
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
					reportAllStateStats();
					clearAllStateStats();
				}
				else if (neighbor->prevRecvRate == 0)
				{
					/*	Start of recv contact.	*/
					reportAllStateStats();
					clearAllStateStats();
				}
			}
#endif
			delta = neighbor->recvRate - neighbor->prevRecvRate;
			induct->acqThrottle.nominalRate += delta;
			neighbor->prevRecvRate = neighbor->recvRate;
		}
	}

	return 0;
}

static void	applyRateControl(Sdr sdr)
{
	BpVdb		*vdb = getBpVdb();
	PsmPartition	ionwm = getIonwm();
	Throttle	*throttle;
	PsmAddress	elt;
	VInduct		*induct;
	VOutduct	*outduct;
	long		capacityLimit;

	sdr_begin_xn(sdr);	/*	Just to lock memory.		*/

	/*	Recalculate limit on local bundle generation.		*/

	manageProductionThrottle(vdb);

	/*	Enable some bundle acquisition.				*/

	for (elt = sm_list_first(ionwm, vdb->inducts); elt;
			elt = sm_list_next(ionwm, elt))
	{
		induct = (VInduct *) psp(ionwm, sm_list_data(ionwm, elt));
		throttle = &(induct->acqThrottle);
		capacityLimit = throttle->nominalRate << 1;
		throttle->capacity += throttle->nominalRate;
		if (throttle->capacity > capacityLimit)
		{
			throttle->capacity = capacityLimit;
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
		capacityLimit = throttle->nominalRate << 1;
		throttle->capacity += throttle->nominalRate;
		if (throttle->capacity > capacityLimit)
		{
			throttle->capacity = capacityLimit;
		}

		if (throttle->capacity > 0)
		{
			sm_SemGive(throttle->semaphore);
		}
	}

	sdr_exit_xn(sdr);	/*	Unlock memory.			*/
}

#if defined (VXWORKS) || defined (RTEMS)
int	bpclock(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
#else
int	main(int argc, char *argv[])
{
#endif
	Sdr	sdr;
	BpDB	*bpConstants;
	int	state = 1;
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
