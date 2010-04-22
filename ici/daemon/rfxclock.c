/*
	rfxclock.c:	scheduled-event management daemon for ION.

	Author: Scott Burleigh, JPL

	Copyright (c) 2007, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include "rfx.h"

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

static void	shutDown()	/*	Commands rfxclock termination.	*/
{
	int	stop = 0;

	oK(_running(&stop));	/*	Terminates rfxclock.		*/
}

static int	setProbeIsDue(unsigned long destNodeNbr,
			unsigned long neighborNodeNbr)
{
	IonNode		*node;
	PsmAddress	nextElt;
	PsmPartition	ionwm;
	PsmAddress	elt;
	IonSnub		*snub;

	node = findNode(getIonVdb(), destNodeNbr, &nextElt);
	if (node == NULL)
	{
		return 0;	/*	Weird, but let it go.		*/
	}

	ionwm = getIonwm();
	for (elt = sm_list_first(ionwm, node->snubs); elt;
			elt = sm_list_next(ionwm, elt))
	{
		snub = (IonSnub *) psp(ionwm, sm_list_data(ionwm, elt));
		CHKERR(snub);
		if (snub->nodeNbr == neighborNodeNbr)
		{
			snub->probeIsDue = 1;
			if (postProbeEvent(node, snub) == 0)
			{
				putErrmsg("Failed setting probeIsDue.", NULL);
				return -1;
			}

			return 0;
		}
	}

	return 0;	/*	Snub no longer exists; no problem.	*/
}

static int	purgeRanges(time_t currentTime)
{
	Sdr		sdr = getIonsdr();
	IonDB		db;
	Object		elt;
	Object		nextElt;
	Object		obj;
	IonRange	range;

	sdr_read(sdr, (char *) &db, getIonDbObject(), sizeof(IonDB));
	for (elt = sdr_list_first(sdr, db.ranges); elt; elt = nextElt)
	{
		nextElt = sdr_list_next(sdr, elt);
		obj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &range, obj, sizeof(IonRange));
		if (range.fromTime > currentTime)
		{
			break;		/*	No more to purge.	*/
		}

		if (range.toTime > currentTime)
		{
			continue;	/*	Still in effect.	*/
		}

		/*	Purge this range from the database.		*/

		sdr_free(sdr, obj);
		sdr_list_delete(sdr, elt, NULL, NULL);
	}

	return 0;
}

static int	purgeContacts(time_t currentTime)
{
	Sdr		sdr = getIonsdr();
	IonDB		db;
	IonVdb		*ionvdb = getIonVdb();
	Object		elt;
	Object		nextElt;
	Object		obj;
	IonContact	contact;
	PsmAddress	nextXmit;
	IonNode		*node;

	sdr_read(sdr, (char *) &db, getIonDbObject(), sizeof(IonDB));
	for (elt = sdr_list_first(sdr, db.contacts); elt; elt = nextElt)
	{
		nextElt = sdr_list_next(sdr, elt);
		obj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &contact, obj, sizeof(IonContact));
		if (contact.fromTime > currentTime)
		{
			break;		/*	No more to purge.	*/
		}

		if (contact.toTime > currentTime)
		{
			continue;	/*	Still in effect.	*/
		}

		/*	Scheduled end of contact has arrived.  Must
		 *	at least remove contact from routing graph.	*/

		if (contact.toNode != db.ownNodeNbr  /* To remote node.	*/
		|| contact.fromNode == db.ownNodeNbr)/* Loopback.	*/
		{
			node = findNode(ionvdb, contact.toNode, &nextXmit);
			if (node)
			{
				forgetXmit(node, &contact);
			}
		}

		/*	May still need to retain the contact itself,
		 *	so that transmission, reception, and timeout
		 *	timing are sensitive to clock error.		*/

		if (contact.xmitRate > 0)
		{
			continue;	/*	Still in effect.	*/
		}

		/*	Purge this contact from the database.		*/

		sdr_free(sdr, obj);
		sdr_list_delete(sdr, elt, NULL, NULL);
	}

	return 0;
}

static void	setOriginOwlt(unsigned long nodeNbr,
			unsigned long originNodeNbr, unsigned int owlt)
{
	IonVdb		*ionvdb = getIonVdb();
	IonNode		*node;
	IonOrigin	*origin;
	PsmAddress	nextElt;

	node = findNode(ionvdb, nodeNbr, &nextElt);
	if (node == NULL)
	{
		node = addNode(ionvdb, nodeNbr, nextElt);
		if (node == NULL)
		{
			putErrmsg("Can't set origin OWLT.", NULL);
			return;
		}
	}

	origin = findOrigin(node, originNodeNbr, &nextElt);
	if (origin == NULL)
	{
		origin = addOrigin(node, originNodeNbr, nextElt);
		if (origin == NULL)
		{
			putErrmsg("Can't set origin OWLT.", NULL);
			return;
		}
	}

	origin->owlt = owlt;
}

static int	applyRange(IonVdb *ionvdb, IonDB *iondb, IonRange *range)
{
	unsigned long	neighborNodeNbr = 0;
	IonNeighbor	*neighbor;
	PsmAddress	nextElt;

	/*	Note that range's From and To nodes are known to be
	 *	different: rfx_insert_range silently rejects OWLT
	 *	for loopback links.					*/

	if (range->fromNode == iondb->ownNodeNbr)
	{
		neighborNodeNbr = range->toNode;
		setOriginOwlt(range->toNode, range->fromNode, range->owlt);
	}
	else
	{
		setOriginOwlt(range->fromNode, range->toNode, range->owlt);
	}

	if (range->fromNode < range->toNode)	/*	Canonical.	*/
	{
		/*	Use as default OWLT for reverse path as well.	*/

		if (range->toNode == iondb->ownNodeNbr)
		{
			neighborNodeNbr = range->fromNode;
			setOriginOwlt(range->fromNode, range->toNode,
					range->owlt);
		}
		else
		{
			setOriginOwlt(range->toNode, range->fromNode,
					range->owlt);
		}
	}

	if (neighborNodeNbr == 0)
	{
		return 0;	/*	Range is between remote nodes.	*/
	}

	/*	Range is between local node and a neighbor.		*/

	neighbor = findNeighbor(ionvdb, neighborNodeNbr, &nextElt);
	if (neighbor == NULL)
	{
		neighbor = addNeighbor(ionvdb, neighborNodeNbr, nextElt);
		if (neighbor == NULL)
		{
			putErrmsg("Can't apply range.", NULL);
			return -1;
		}
	}

	if (range->fromNode == iondb->ownNodeNbr)
	{
		neighbor->owltOutbound = range->owlt;
		if (range->fromNode < range->toNode)
		{
			/*	Use as default for Inbound as well.	*/

			neighbor->owltInbound = range->owlt;
		}
	}
	else	/*	Range is from neighbor to self.			*/
	{
		neighbor->owltInbound = range->owlt;
		if (range->fromNode < range->toNode)
		{
			/*	Use as default for Outbound as well.	*/

			neighbor->owltOutbound = range->owlt;
		}
	}
	return 0;
}

static int	applyRanges(time_t currentTime)
{
	Sdr		sdr = getIonsdr();
	IonDB		db;
	IonVdb		*ionvdb = getIonVdb();
	Object		elt;
	Object		nextElt;
	Object		obj;
	IonRange	range;

	sdr_read(sdr, (char *) &db, getIonDbObject(), sizeof(IonDB));
	for (elt = sdr_list_first(sdr, db.ranges); elt; elt = nextElt)
	{
		nextElt = sdr_list_next(sdr, elt);
		obj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &range, obj, sizeof(IonRange));
		if (range.fromTime > currentTime)	/*	Done.	*/
		{
			break;
		}

		if (applyRange(ionvdb, &db, &range) < 0)
		{
			return -1;
		}
	}

	return 0;
}

static int	applyContact(IonVdb *ionvdb, IonDB *iondb, IonContact *contact,
			time_t currentTime)
{
	unsigned long	xmitRate;
	time_t		purgeTime;
	IonNeighbor	*neighbor;
	PsmAddress	nextElt;
	IonNode		*node;
	PsmPartition	ionwm;
	PsmAddress	elt;
	IonXmit		*xmit;
	IonOrigin	*origin;

	xmitRate = contact->xmitRate;
	purgeTime = contact->toTime;
	if (contact->fromNode == iondb->ownNodeNbr)
	{
		neighbor = findNeighbor(ionvdb, contact->toNode, &nextElt);
		if (neighbor == NULL)
		{
			neighbor = addNeighbor(ionvdb, contact->toNode,
					nextElt);
			if (neighbor == NULL)
			{
				putErrmsg("Can't apply contact.", NULL);
				return -1;
			}
		}

		/*	Adjust aggregate capacity as needed.		*/

		if (currentTime >= contact->fromTime
		&& currentTime < contact->toTime
		&& (node = (IonNode *) psp((ionwm = getIonwm()),
				neighbor->node)) != NULL)
		{
			/*	Must reduce aggregate capacity of own
			 *	xmits to this neighbor by 1 second's
			 *	worth of transmission capacity.		*/

			for (elt = sm_list_first(ionwm, node->xmits); elt;
					elt = sm_list_next(ionwm, elt))
			{
				xmit = (IonXmit *) psp(ionwm,
						sm_list_data(ionwm, elt));
				CHKERR(xmit);
				origin = (IonOrigin *) psp(ionwm, xmit->origin);
				CHKERR(origin);
				if (origin->nodeNbr == iondb->ownNodeNbr)
				{
					reduceScalar(&(xmit->aggrCapacity),
						xmitRate);
					if (!scalarIsValid
						(&(xmit->aggrCapacity)))
					{
						loadScalar
						(&(xmit->aggrCapacity), 0);
					}
				}
			}
		}

		/*	Be a little slow to start transmission, and
		 *	a little quick to stop, to ensure that segments
		 *	arrive only when neighbor is expecting them.	*/

		if (currentTime < (contact->toTime - iondb->maxClockError))
		{
			if (currentTime >= (contact->fromTime
					+ iondb->maxClockError))
			{
				neighbor->xmitRate = contact->xmitRate;
			}
		}
	}

	if (contact->toNode == iondb->ownNodeNbr)
	{
		/*	Retain this contact a little longer, to
		 *	enable late termination of reception.		*/

		neighbor = findNeighbor(ionvdb, contact->fromNode, &nextElt);
		if (neighbor == NULL)
		{
			neighbor = addNeighbor(ionvdb, contact->fromNode,
					nextElt);
			if (neighbor == NULL)
			{
				putErrmsg("Can't apply contact.", NULL);
				return -1;
			}
		}

		purgeTime = contact->toTime + iondb->maxClockError
				+ neighbor->owltInbound;

		/*	Be a little slow to resume timers, and a
		 *	little quick to suspend them, to minimize the
		 *	chance of timing out prematurely.		*/

		if (currentTime < (contact->toTime - iondb->maxClockError))
		{
			if (currentTime >= (contact->fromTime
					+ iondb->maxClockError))
			{
				neighbor->fireRate = contact->xmitRate;
			}
		}

		/*	Be a little quick to start accepting segments,
		 *	and a little slow to stop, to minimize the
		 *	chance of discarding legitimate input.		*/

		if (currentTime < ((contact->toTime + iondb->maxClockError)
				+ neighbor->owltInbound))
		{
			if (currentTime >= ((contact->fromTime
					- iondb->maxClockError)
					+ neighbor->owltInbound))
			{
				neighbor->recvRate = contact->xmitRate;
			}
		}
	}

	if (currentTime >= purgeTime)
	{
		contact->xmitRate = 0;
	}

	return 0;
}

static int	applyContacts(time_t currentTime)
{
	Sdr		sdr = getIonsdr();
	IonDB		db;
	IonVdb		*ionvdb = getIonVdb();
	Object		elt;
	Object		nextElt;
	Object		obj;
	IonContact	contact;

	sdr_read(sdr, (char *) &db, getIonDbObject(), sizeof(IonDB));
	for (elt = sdr_list_first(sdr, db.contacts); elt; elt = nextElt)
	{
		nextElt = sdr_list_next(sdr, elt);
		obj = sdr_list_data(sdr, elt);
		sdr_stage(sdr, (char *) &contact, obj, sizeof(IonContact));
		if (contact.fromTime > currentTime)	/*	Done.	*/
		{
			break;
		}

		/*	Apply this information to contact state.	*/

		if (applyContact(ionvdb, &db, &contact, currentTime) < 0)
		{
			return -1;
		}

		sdr_write(sdr, obj, (char *) &contact, sizeof(IonContact));
	}

	return 0;
}

#if defined (VXWORKS) || defined (RTEMS)
int	rfxclock(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
#else
int	main(int argc, char *argv[])
{
#endif
	Sdr		sdr;
	time_t		currentTime;
	PsmPartition	ionwm;
	IonVdb		*ionvdb;
	int		start = 1;
	PsmAddress	elt;
	PsmAddress	addr;
	IonProbe	*probe;
	int		destNodeNbr;
	int		neighborNodeNbr;
	IonNeighbor	*neighbor;
	PsmAddress	elt2;
	IonNode		*node;
	IonOrigin	*origin;

	if (ionAttach() < 0)
	{
		putErrmsg("rfxclock can't attach to ION.", NULL);
		return -1;
	}

	sdr = getIonsdr();
	ionwm = getIonwm();
	ionvdb = getIonVdb();
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
		currentTime = getUTCTime();
		sdr_begin_xn(sdr);

		/*	First enable probes.				*/

		for (elt = sm_list_first(ionwm, ionvdb->probes); elt;
				elt = sm_list_next(ionwm, elt))
		{
			addr = sm_list_data(ionwm, elt);
			probe = (IonProbe *) psp(ionwm, addr);
			CHKZERO(probe);
			if (probe->time > currentTime)
			{
				break;	/*	No more for now.	*/
			}

			/*	Destroy this probe and post the next.	*/

			oK(sm_list_delete(ionwm, elt, NULL, NULL));
			destNodeNbr = probe->destNodeNbr;
			neighborNodeNbr = probe->neighborNodeNbr;
			psm_free(ionwm, addr);
			if (setProbeIsDue(destNodeNbr, neighborNodeNbr) < 0)
			{
				putErrmsg("Can't enable probes.", NULL);
				sdr_cancel_xn(sdr);
				return -1;
			}
		}

		/*	The intent of this code is to (a) enable
		 *	contact and range applicability intervals to
		 *	overlap and (b) assure that on system startup
		 *	correct current rates and owlts are used to
		 *	construct the initial contact state of the
		 *	node.
		 *
		 *	First we purge all ranges and contacts that
		 *	have ended.					*/

		if (purgeRanges(currentTime) < 0)
		{
			putErrmsg("Can't purge ranges.", NULL);
			sdr_cancel_xn(sdr);
			return -1;
		}

		if (purgeContacts(currentTime) < 0)
		{
			putErrmsg("Can't purge contacts.", NULL);
			sdr_cancel_xn(sdr);
			return -1;
		}

		/*	Now we set the volatile contact and range state
		 *	per the remaining ranges and contacts in the
		 *	database.
		 *	
		 *	First we reset current state to all zeros.	*/

		for (elt = sm_list_first(ionwm, ionvdb->neighbors); elt;
				elt = sm_list_next(ionwm, elt))
		{
			neighbor = (IonNeighbor *) psp(ionwm,
					sm_list_data(ionwm, elt));
			CHKZERO(neighbor);
			neighbor->owltInbound = 0;
			neighbor->owltOutbound = 0;
			neighbor->xmitRate = 0;
			neighbor->fireRate = 0;
			neighbor->recvRate = 0;
		}

		for (elt = sm_list_first(ionwm, ionvdb->nodes); elt;
				elt = sm_list_next(ionwm, elt))
		{
			node = (IonNode *) psp(ionwm,
					sm_list_data(ionwm, elt));
			CHKZERO(node);
			for (elt2 = sm_list_first(ionwm, node->origins); elt2;
					elt2 = sm_list_next(ionwm, elt2))
			{
				origin = (IonOrigin *) psp(ionwm,
						sm_list_data(ionwm, elt2));
				CHKZERO(origin);
				origin->owlt = 0;
			}
		}

		/*	Then we apply all ranges and contacts that
		 *	have begun and have not yet ended.		*/

		if (applyRanges(currentTime) < 0)
		{
			putErrmsg("Can't apply ranges.", NULL);
			sdr_cancel_xn(sdr);
			return -1;
		}

		if (applyContacts(currentTime) < 0)
		{
			putErrmsg("Can't apply contacts.", NULL);
			sdr_cancel_xn(sdr);
			return -1;

		}

		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Can't set current topology.", NULL);
			return -1;
		}
	}

	writeErrmsgMemos();
	writeMemo("[i] rfxclock has ended.");
	ionDetach();
	return 0;
}
