/*
	rfxclock.c:	scheduled-event management daemon for ION.

	Author: Scott Burleigh, JPL

	Copyright (c) 2007, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.

									*/
#include "rfx.h"

#ifndef	MAX_SECONDS_UNCLAIMED
#define	MAX_SECONDS_UNCLAIMED	(3)
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

static void	shutDown(int signum)	/*	Stops rfxclock.	*/
{
	/* Tell the compiler that we are not using 'signum' */
	(void)signum;

	uaddr	stop = 0;
	oK(_running(&stop));	/*	Terminates rfxclock.		*/
}

static int	setProbeIsDue(unsigned long destFqnn,
			unsigned long neighborFqnn)
{
	IonNode		*node;
	PsmAddress	nextElt;
	PsmPartition	ionwm;
	PsmAddress	elt;
	Embargo		*embargo;

	node = findNode(getIonVdb(), destFqnn, &nextElt);
	if (node == NULL)
	{
		return 0;	/*	Weird, but let it go.		*/
	}

	ionwm = getIonwm();
	for (elt = sm_list_first(ionwm, node->embargoes); elt;
			elt = sm_list_next(ionwm, elt))
	{
		embargo = (Embargo *) psp(ionwm, sm_list_data(ionwm, elt));
		CHKERR(embargo);
		if (embargo->fqnn == neighborFqnn)
		{
			embargo->probeIsDue = 1;
			if (postProbeEvent(node, embargo) == 0)
			{
				putErrmsg("Failed setting probeIsDue.", NULL);
				return -1;
			}

			return 0;
		}
	}

	return 0;	/*	Embargo no longer exists; no problem.	*/
}

/*	Helper function to look ahead in the timeline for an adjacent Start
 *	event at the same time for the same node pair.  Returns 1 if found,
 *	0 otherwise.							*/

static int	findAdjacentStartEvent(IonVdb *vdb, time_t eventTime,
			IonEventType startType, uvast fromFqnn, uvast toFqnn)
{
	PsmPartition	ionwm = getIonwm();
	PsmAddress	elt;
	PsmAddress	addr;
	IonEvent	*candidateEvent;
	IonCXref	*candidateCxref;

	/*	Search timeline for events at the same time.		*/

	for (elt = sm_rbt_first(ionwm, vdb->timeline); elt;
			elt = sm_rbt_next(ionwm, elt))
	{
		addr = sm_rbt_data(ionwm, elt);
		candidateEvent = (IonEvent *) psp(ionwm, addr);

		if (candidateEvent->time > eventTime)
		{
			break;	/*	Past the time we're looking for.*/
		}

		if (candidateEvent->time < eventTime)
		{
			continue;	/*	Not there yet.		*/
		}

		/*	Same time - check if it's the Start event
		 *	we're looking for.				*/

		if (candidateEvent->type == startType)
		{
			candidateCxref = (IonCXref *) psp(ionwm,
					candidateEvent->ref);
			if (candidateCxref->fromFqnn == fromFqnn
			&& candidateCxref->toFqnn == toFqnn)
			{
				return 1;  /*	Found adjacent!	*/
			}
		}
	}

	return 0;	/*	No adjacent Start event found.		*/
}

static int	dispatchEvent(IonVdb *vdb, IonEvent *event, int *forecastNeeded)
{
	PsmPartition	ionwm = getIonwm();
	PsmAddress	addr;
	IonRXref	*rxref;
	IonNeighbor	*neighbor;
	IonCXref	*cxref;
	PsmAddress	ref;
	SdrObject	iondbObj;
	IonDB		iondb;
	IonEvent	*newEvent;
	PsmAddress	alarmAddr;
	IonAlarm	*alarm;
	time_t		currentTime;

	switch (event->type)
	{
	case IonStopImputedRange:
	case IonStopAssertedRange:
		rxref = (IonRXref *) psp(ionwm, event->ref);

		/*	rfx_remove_range deletes all range events
		 *	from timeline including the one that invoked
		 *	this function.					*/

		return rfx_remove_range(&(rxref->fromTime),
				rxref->fromFqnn, rxref->toFqnn, 0);

	case IonStopXmit:
		cxref = (IonCXref *) psp(ionwm, event->ref);
#if DEBUG_RFX
		{
			char	timebuf[TIMESTAMPBUFSZ];
			writeTimestampUTC(event->time, timebuf);
			printf("[RFX] Node " UVAST_FIELDSPEC " %s IonStopXmit: " UVAST_FIELDSPEC "->" UVAST_FIELDSPEC "\n",
				getOwnFqnn(), timebuf, cxref->fromFqnn, cxref->toFqnn);
			fflush(stdout);
		}
#endif

		/*	Look ahead for adjacent IonStartXmit at same time.
		 *	If found, skip setting xmitRate to 0 to maintain
		 *	seamless transmission across adjacent contacts.	*/

		if (findAdjacentStartEvent(vdb, event->time, IonStartXmit,
				cxref->fromFqnn, cxref->toFqnn))
		{
#if DEBUG_RFX
			{
				char	timebuf[TIMESTAMPBUFSZ];
				writeTimestampUTC(event->time, timebuf);
				printf("[RFX] Node " UVAST_FIELDSPEC " %s Skipping IonStopXmit: " UVAST_FIELDSPEC "->" UVAST_FIELDSPEC " (adjacent IonStartXmit found)\n",
					getOwnFqnn(), timebuf, cxref->fromFqnn, cxref->toFqnn);
				fflush(stdout);
			}
#endif
			sm_rbt_delete(ionwm, vdb->timeline, rfx_order_events,
					event, rfx_erase_data, NULL);
			return 0;
		}

		if (cxref->fromFqnn == getOwnFqnn()
		&& cxref->type != CtSuppressed)
		{
			neighbor = getNeighbor(vdb, cxref->toFqnn);
			CHKERR(neighbor);
			neighbor->xmitRate = 0;
			*forecastNeeded = 1;
		}

		sm_rbt_delete(ionwm, vdb->timeline, rfx_order_events,
				event, rfx_erase_data, NULL);
		return 0;

	case IonStopFire:
		cxref = (IonCXref *) psp(ionwm, event->ref);
#if DEBUG_RFX
		{
			char	timebuf[TIMESTAMPBUFSZ];
			writeTimestampUTC(event->time, timebuf);
			printf("[RFX] Node " UVAST_FIELDSPEC " %s IonStopFire: " UVAST_FIELDSPEC "->" UVAST_FIELDSPEC "\n",
				getOwnFqnn(), timebuf, cxref->fromFqnn, cxref->toFqnn);
			fflush(stdout);
		}
#endif

		/*	Look ahead for adjacent IonStartFire at same time.
		 *	If found, skip setting fireRate to 0 to maintain
		 *	seamless timer management across adjacent contacts.	*/

		if (findAdjacentStartEvent(vdb, event->time, IonStartFire,
				cxref->fromFqnn, cxref->toFqnn))
		{
#if DEBUG_RFX
			{
				char	timebuf[TIMESTAMPBUFSZ];
				writeTimestampUTC(event->time, timebuf);
				printf("[RFX] Node " UVAST_FIELDSPEC " %s Skipping IonStopFire: " UVAST_FIELDSPEC "->" UVAST_FIELDSPEC " (adjacent IonStartFire found)\n",
					getOwnFqnn(), timebuf, cxref->fromFqnn, cxref->toFqnn);
				fflush(stdout);
			}
#endif
			sm_rbt_delete(ionwm, vdb->timeline, rfx_order_events,
					event, rfx_erase_data, NULL);
			return 0;
		}

		if (cxref->toFqnn == getOwnFqnn()
		&& cxref->type != CtSuppressed)
		{
			neighbor = getNeighbor(vdb, cxref->fromFqnn);
			CHKERR(neighbor);
			neighbor->fireRate = 0;
		}

		sm_rbt_delete(ionwm, vdb->timeline, rfx_order_events,
				event, rfx_erase_data, NULL);
		return 0;

	case IonStopRecv:
		cxref = (IonCXref *) psp(ionwm, event->ref);
#if DEBUG_RFX
		{
			char	timebuf[TIMESTAMPBUFSZ];
			writeTimestampUTC(event->time, timebuf);
			printf("[RFX] Node " UVAST_FIELDSPEC " %s IonStopRecv: " UVAST_FIELDSPEC "->" UVAST_FIELDSPEC "\n",
				getOwnFqnn(), timebuf, cxref->fromFqnn, cxref->toFqnn);
			fflush(stdout);
		}
#endif

		/*	Look ahead for adjacent IonStartRecv at same time,
		 *	or for IonStartFire that will create IonStartRecv.
		 *	If found, skip setting recvRate to 0 to maintain
		 *	seamless reception across adjacent contacts.	*/

		neighbor = getNeighbor(vdb, cxref->fromFqnn);
		if (neighbor == NULL)
		{
			neighbor = addNeighbor(vdb, cxref->fromFqnn);
			if (neighbor == NULL)
			{
				putErrmsg("Can't get neighbor.", NULL);
				return -1;
			}
		}

		if (findAdjacentStartEvent(vdb, event->time, IonStartRecv,
				cxref->fromFqnn, cxref->toFqnn)
		|| findAdjacentStartEvent(vdb, event->time - neighbor->owltInbound,
				IonStartFire, cxref->fromFqnn, cxref->toFqnn))
		{
#if DEBUG_RFX
			{
				char	timebuf[TIMESTAMPBUFSZ];
				writeTimestampUTC(event->time, timebuf);
				printf("[RFX] Node " UVAST_FIELDSPEC " %s Skipping IonStopRecv: " UVAST_FIELDSPEC "->" UVAST_FIELDSPEC " (adjacent reception continues)\n",
					getOwnFqnn(), timebuf, cxref->fromFqnn, cxref->toFqnn);
				fflush(stdout);
			}
#endif
			sm_rbt_delete(ionwm, vdb->timeline, rfx_order_events,
					event, rfx_erase_data, NULL);
			return 0;
		}

		if (cxref->toFqnn == getOwnFqnn()
		&& cxref->type != CtSuppressed)
		{
			neighbor->recvRate = 0;
			*forecastNeeded = 1;
		}

		sm_rbt_delete(ionwm, vdb->timeline, rfx_order_events,
				event, rfx_erase_data, NULL);
		return 0;

	case IonStartImputedRange:
	case IonStartAssertedRange:
		rxref = (IonRXref *) psp(ionwm, event->ref);
		if (rxref->fromFqnn == getOwnFqnn())
		{
			neighbor = getNeighbor(vdb, rxref->toFqnn);
			if (neighbor)
			{
				neighbor->owltOutbound = rxref->owlt;
			}
		}

		if (rxref->toFqnn == getOwnFqnn())
		{
			neighbor = getNeighbor(vdb, rxref->fromFqnn);
			if (neighbor)
			{
				neighbor->owltInbound = rxref->owlt;
			}
		}

		sm_rbt_delete(ionwm, vdb->timeline, rfx_order_events,
				event, rfx_erase_data, NULL);
		return 0;

	case IonStartXmit:
		cxref = (IonCXref *) psp(ionwm, event->ref);
#if DEBUG_RFX
		{
			char	timebuf[TIMESTAMPBUFSZ];
			writeTimestampUTC(event->time, timebuf);
			printf("[RFX] Node " UVAST_FIELDSPEC " %s IonStartXmit: " UVAST_FIELDSPEC "->" UVAST_FIELDSPEC " rate=%zu\n",
				getOwnFqnn(), timebuf, cxref->fromFqnn, cxref->toFqnn, cxref->xmitRate);
			fflush(stdout);
		}
#endif
		if (cxref->fromFqnn == getOwnFqnn()
		&& cxref->type != CtSuppressed)
		{
			neighbor = getNeighbor(vdb, cxref->toFqnn);
			CHKERR(neighbor);
			neighbor->xmitRate = cxref->xmitRate;
			*forecastNeeded = 1;
		}

		sm_rbt_delete(ionwm, vdb->timeline, rfx_order_events,
				event, rfx_erase_data, NULL);
		return 0;

	case IonStartFire:
		cxref = (IonCXref *) psp(ionwm, event->ref);
#if DEBUG_RFX
		{
			char	timebuf[TIMESTAMPBUFSZ];
			writeTimestampUTC(event->time, timebuf);
			printf("[RFX] Node " UVAST_FIELDSPEC " %s IonStartFire: " UVAST_FIELDSPEC "->" UVAST_FIELDSPEC " rate=%zu\n",
				getOwnFqnn(), timebuf, cxref->fromFqnn, cxref->toFqnn, cxref->xmitRate);
			fflush(stdout);
		}
#endif
		if (cxref->toFqnn == getOwnFqnn())
		{
			neighbor = getNeighbor(vdb, cxref->fromFqnn);
			CHKERR(neighbor);
			if (cxref->type != CtSuppressed)
			{
				neighbor->fireRate = cxref->xmitRate;
			}

			/*	Only now can we post the
			 *	events for start/stop of
			 *	reception and purging of
			 *	contact: we need to know the
			 *	range (one-way light time)
			 *	from this neighbor.		*/

			ref = event->ref;
			iondbObj = getIonDbObject();
			sdr_read(getIonsdr(), (char *) &iondb, iondbObj,
					sizeof(IonDB));

			/*	Be a little quick to start
			 *	accepting segments, and a
			 *	little slow to stop, to
			 *	minimize the chance of
			 *	discarding legitimate input.
			 *
			 *	Note: For adjacent contacts, startFire/stopFire
			 *	equal fromTime/toTime (no maxClockError applied
			 *	in rfx.c). Detect this and skip maxClockError
			 *	for reception events to prevent temporal reversal.	*/

			/*	Detect if this contact is adjacent to another
			 *	by checking if maxClockError was applied to
			 *	startFire/stopFire.				*/

			int adjacentAtStart = 0;
			int adjacentAtEnd = 0;

			if (cxref->startFire == cxref->fromTime)
			{
				/*	No maxClockError at start = adjacent
				 *	contact before this one.		*/

				adjacentAtStart = 1;
			}

			if (cxref->stopFire == cxref->toTime)
			{
				/*	No maxClockError at end = adjacent
				 *	contact after this one.			*/

				adjacentAtEnd = 1;
			}

			addr = psm_zalloc(ionwm, sizeof(IonEvent));
			if (addr == 0)
			{
				return -1;
			}

			newEvent = (IonEvent *) psp(ionwm, addr);
			if (adjacentAtStart)
			{
				/*	Adjacent contact: no maxClockError
				 *	adjustment at start.			*/

				cxref->startRecv = newEvent->time =
					cxref->startFire + neighbor->owltInbound;
			}
			else
			{
				/*	Normal contact: apply maxClockError.	*/

				cxref->startRecv = newEvent->time =
					cxref->startFire - iondb.maxClockError
						+ neighbor->owltInbound;
			}

			newEvent->type = IonStartRecv;
			newEvent->ref = ref;
			if (sm_rbt_insert(ionwm, vdb->timeline, addr,
					rfx_order_events, newEvent) == 0)
			{
				psm_free(ionwm, addr);
				return -1;
			}

			addr = psm_zalloc(ionwm, sizeof(IonEvent));
			if (addr == 0)
			{
				return -1;
			}

			newEvent = (IonEvent *) psp(ionwm, addr);
			if (adjacentAtEnd)
			{
				/*	Adjacent contact: no maxClockError
				 *	adjustment at end.			*/

				cxref->stopRecv = newEvent->time =
					cxref->stopFire + neighbor->owltInbound;
			}
			else
			{
				/*	Normal contact: apply maxClockError.	*/

				cxref->stopRecv = newEvent->time =
					cxref->stopFire + iondb.maxClockError
						+ neighbor->owltInbound;
			}
			newEvent->type = IonStopRecv;
			newEvent->ref = ref;
			if (sm_rbt_insert(ionwm, vdb->timeline, addr,
					rfx_order_events, newEvent) == 0)
			{
				psm_free(ionwm, addr);
				return -1;
			}

			/*	Purge contact when reception
			 *	is expected to stop.
			 *	Use stopRecv time (already computed above).	*/

			addr = psm_zalloc(ionwm, sizeof(IonEvent));
			if (addr == 0)
			{
				return -1;
			}

			newEvent = (IonEvent *) psp(ionwm, addr);
			cxref->purgeTime = newEvent->time = cxref->stopRecv;
			newEvent->type = IonPurgeContact;
			newEvent->ref = ref;
			if (sm_rbt_insert(ionwm, vdb->timeline, addr,
					rfx_order_events, newEvent) == 0)
			{
				psm_free(ionwm, addr);
				return -1;
			}
		}

		sm_rbt_delete(ionwm, vdb->timeline, rfx_order_events,
				event, rfx_erase_data, NULL);
		return 0;

	case IonStartRecv:
		cxref = (IonCXref *) psp(ionwm, event->ref);
#if DEBUG_RFX
		{
			char	timebuf[TIMESTAMPBUFSZ];
			writeTimestampUTC(event->time, timebuf);
			printf("[RFX] Node " UVAST_FIELDSPEC " %s IonStartRecv: " UVAST_FIELDSPEC "->" UVAST_FIELDSPEC " rate=%zu\n",
				getOwnFqnn(), timebuf, cxref->fromFqnn, cxref->toFqnn, cxref->xmitRate);
			fflush(stdout);
		}
#endif
		if (cxref->toFqnn == getOwnFqnn()
		&& cxref->type != CtSuppressed)
		{
			neighbor = getNeighbor(vdb, cxref->fromFqnn);
			CHKERR(neighbor);
			neighbor->recvRate = cxref->xmitRate;
			*forecastNeeded = 1;
		}

		sm_rbt_delete(ionwm, vdb->timeline, rfx_order_events,
				event, rfx_erase_data, NULL);
		return 0;

	case IonPurgeContact:
		cxref = (IonCXref *) psp(ionwm, event->ref);
#if DEBUG_RFX
		{
			char	timebuf[TIMESTAMPBUFSZ];
			char	startTimebuf[TIMESTAMPBUFSZ];
			writeTimestampUTC(event->time, timebuf);
			writeTimestampUTC(cxref->fromTime, startTimebuf);
			printf("[RFX] Node " UVAST_FIELDSPEC " %s IonPurgeContact: " UVAST_FIELDSPEC "->" UVAST_FIELDSPEC " (started at %s)\n",
				getOwnFqnn(), timebuf, cxref->fromFqnn, cxref->toFqnn, startTimebuf);
			fflush(stdout);
		}
#endif

		/*	rfx_remove_contact deletes all contact events
		 *	from timeline including the one that invoked
		 *	this function.					*/

		return rfx_remove_contact(cxref->regionNbr, &(cxref->fromTime),
				cxref->fromFqnn, cxref->toFqnn, 0);

	case IonAlarmTimeout:
		alarmAddr = event->ref;
		alarm = (IonAlarm *) psp(ionwm, alarmAddr);
		if (alarm->term == 0)		/*	Cleaning up.	*/
		{
			if (sm_SemEnded(alarm->semaphore))
			{
				/*	Time to remove alarm.		*/

				sm_SemEnd(alarm->semaphore);
				microsnooze(50000);
				sm_SemDelete(alarm->semaphore);
				psm_free(ionwm, alarmAddr);
				sm_rbt_delete(ionwm, vdb->timeline,
						rfx_order_events,
						event, rfx_erase_data, NULL);
				return 0;
			}

			/*	Must continue cleanup.			*/

			sm_SemEnd(alarm->semaphore);
		}
		else	/*	Must raise alarm.			*/
		{
			sm_SemGive(alarm->semaphore);
			if (alarm->cycles > 0)
			{
				alarm->cycles -= 1;
				if (alarm->cycles == 0)
				{
					/*	Must start cleanup.	*/

					alarm->term = 0;

					/*	Setting term to zero
					 *	forces one more timeout
					 *	event, 1 second in the
					 *	future, that kills the
					 *	alarm without giving
					 *	the alarm's semaphore.	*/
				}
			}
		}

		/*	Must repeat alarm.  Delete the expired event,
		 *	without deleting the alarm structure.		*/

		event->ref = 0;
		sm_rbt_delete(ionwm, vdb->timeline, rfx_order_events, event,
				rfx_erase_data, NULL);

		/*	Now post next event for this alarm.		*/

		currentTime = getCtime();
		if (alarm->term == 0)	/*	Cleaning up.		*/
		{
			alarm->nextTimeout = currentTime + 1;
		}
		else
		{
			alarm->nextTimeout = currentTime + alarm->term;
		}

		addr = psm_zalloc(ionwm, sizeof(IonEvent));
		if (addr == 0)
		{
			sm_SemEnd(alarm->semaphore);
			microsnooze(50000);
			sm_SemDelete(alarm->semaphore);
			psm_free(ionwm, alarmAddr);
			return -1;
		}

		newEvent = (IonEvent *) psp(ionwm, addr);
		newEvent->time = alarm->nextTimeout;
		newEvent->type = IonAlarmTimeout;
		newEvent->ref = alarmAddr;
		if (sm_rbt_insert(ionwm, vdb->timeline, addr, rfx_order_events,
				newEvent) == 0)
		{
			psm_free(ionwm, addr);
			sm_SemEnd(alarm->semaphore);
			microsnooze(50000);
			sm_SemDelete(alarm->semaphore);
			psm_free(ionwm, alarmAddr);
			return -1;
		}

		return 0;

	default:
		putErrmsg("Invalid RFX timeline event.", itoa(event->type));
		sm_rbt_delete(ionwm, vdb->timeline, rfx_order_events,
				event, rfx_erase_data, NULL);
		return -1;

	}
}

#ifdef TRACKRFXEVENTS

/*	Note: this user-provided source code MUST include a function
 *	named handleEvent, with the same function prototype as the
 *	default handleEvent() function defined below.  That function
 *	MUST return the return code obtained by calling dispatchEvent().*/

#include "rfxtracker.c"

#else
static int	handleEvent(IonVdb *vdb, IonEvent *event, int *forecastNeeded)
{
	return dispatchEvent(vdb, event, forecastNeeded);
}
#endif

#if defined (ION_LWT)
int	rfxclock(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
#else
int	main(void)
{
#endif
	Sdr		sdr;
	PsmPartition	ionwm;
	IonVdb		*vdb;
	uaddr		start = 1;
	time_t		currentTime;
	PsmAddress	elt;
	PsmAddress	addr;
	IonProbe	*probe;
	int		destFqnn;
	int		neighborFqnn;
	int		forecastNeeded;
	IonEvent	*event;
	int		i;
	PsmAddress	nextElt;
	PsmAddress	reqAddr;
	Requisition	*req;

	if (ionAttach() < 0)
	{
		putErrmsg("rfxclock can't attach to ION.", NULL);
		return -1;
	}

	sdr = getIonsdr();
	ionwm = getIonwm();
	vdb = getIonVdb();
	isignal(SIGTERM, shutDown);

	/*	Main loop: wait for event occurrence time, then
	 *	execute applicable events.				*/

	oK(_running(&start));
	writeMemo("[i] rfxclock is running.");
	while (_running(NULL))
	{
		/*	Sleep for 1 second, then dispatch all events
		 *	whose execution times have now been reached.	*/

		snooze(1);
		currentTime = getCtime();
		if (!sdr_begin_xn(sdr))
		{
			putErrmsg("rfxclock failed to begin new transaction.",
					NULL);
			break;
		}

		/*	First enable probes.				*/

		for (elt = sm_list_first(ionwm, vdb->probes); elt;
				elt = sm_list_next(ionwm, elt))
		{
			addr = sm_list_data(ionwm, elt);
			probe = (IonProbe *) psp(ionwm, addr);
			if (probe == NULL)
			{
				putErrmsg("List of probes is corrupt.", NULL);
				sdr_exit_xn(sdr);
				break;
			}

			if (probe->time > currentTime)
			{
				sdr_exit_xn(sdr);
				break;	/*	No more for now.	*/
			}

			/*	Destroy this probe and post the next.	*/

			oK(sm_list_delete(ionwm, elt, NULL, NULL));
			destFqnn = probe->destFqnn;
			neighborFqnn = probe->neighborFqnn;
			psm_free(ionwm, addr);
			if (setProbeIsDue(destFqnn, neighborFqnn) < 0)
			{
				putErrmsg("Can't enable probes.", NULL);
				sdr_cancel_xn(sdr);
				return -1;
			}
		}

		/*	Now dispatch all events that are scheduled
		 *	for any time that is not in the future.  In
		 *	so doing, adjust the volatile contact and
		 *	range state of the local node.			*/

		forecastNeeded = 0;
		while (1)
		{
			elt = sm_rbt_first(ionwm, vdb->timeline);
			if (elt == 0)
			{
				break;
			}

			addr = sm_rbt_data(ionwm, elt);
			event = (IonEvent *) psp(ionwm, addr);
			if (event->time > currentTime)
			{
				break;
			}

			if (handleEvent(vdb, event, &forecastNeeded) < 0)
			{
				putErrmsg("Failed handling event.", NULL);
				break;
			}

			/*	The handleEvent() function calls
			 *	dispatchEvent(), which is responsible
			 *	for deletion of the timeline list
			 *	element and event structure.		*/
		}

		/*	Finally, clean up any ZCO space requisitions
		 *	that have been serviced but that applicants
		 *	have not yet claimed; let other applicants
		 *	get access to ZCO space.			*/

		for (i = 0; i < 2; i++)
		{
			for (elt = sm_list_first(ionwm, vdb->requisitions[i]);
					elt; elt = nextElt)
			{
				nextElt = sm_list_next(ionwm, elt);
				reqAddr = sm_list_data(ionwm, elt);
				if (reqAddr == 0)
				{
					continue;
				}

				req = (Requisition *) psp(ionwm, reqAddr);
				switch (req->secondsUnclaimed)
				{
				case -1:	/*	Not serviced.	*/
					break;	/*	Out of switch.	*/

				case MAX_SECONDS_UNCLAIMED:

					/*	Requisition expired.  Free
					 *	the requisition but leave
					 *	the list element (ticket)
					 *	intact so the consumer can
					 *	safely call ionShred().	*/

					writeMemo("[?] ZCO space not claimed \
in time, released to other applications.");
					sm_SemEnd(req->semaphore);
					sm_list_data_set(ionwm, elt, 0);
					psm_free(ionwm, reqAddr);
					continue;

				default:	/*	Still waiting.	*/
					req->secondsUnclaimed++;
					continue;
				}

				/*	No more serviced requisitions.	*/

				break;		/*	Out of loop.	*/
			}
		}

		/*	Finished all ION 1Hz processing.		*/

		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Can't do ION 1Hz updates.", NULL);
			return -1;
		}

		if (forecastNeeded)
		{
			oK(pseudoshell("ionwarn"));
		}
	}

	writeErrmsgMemos();
	writeMemo("[i] rfxclock has ended.");
	ionDetach();
	return 0;
}
