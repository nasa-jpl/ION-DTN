/*

	rfx.c:	API for managing ION's time-ordered lists of
		contacts and ranges.

	Author:	Scott Burleigh, JPL

	rfx_remove_contact and rfx_remove_range adapted by Greg Menke
	to apply to all objects for specified from/to nodes when
	fromTime is zero.

	Copyright (c) 2007, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.

									*/
#include "rfx.h"
#include "lyst.h"

typedef struct
{
	time_t	time;
	long	prevXmitRate;
	long	xmitRate;
	int	fromNeighbor;		/*	Boolean.		*/
} RateChange;

static char	*_cannotForecast()
{
	return "Can't complete congestion forecast.";
}

int	rfx_system_is_started()
{
	IonVdb	*vdb = getIonVdb();

	return (vdb && vdb->clockPid != ERROR);
}

IonNeighbor	*findNeighbor(IonVdb *ionvdb, unsigned long nodeNbr,
			PsmAddress *nextElt)
{
	PsmPartition	ionwm = getIonwm();
	PsmAddress	elt;
	IonNeighbor	*neighbor;

	CHKNULL(ionvdb);
	CHKNULL(nextElt);
	for (elt = sm_list_first(ionwm, ionvdb->neighbors); elt;
			elt = sm_list_next(ionwm, elt))
	{
		neighbor = (IonNeighbor *) psp(ionwm, sm_list_data(ionwm, elt));
		CHKNULL(neighbor);
		if (neighbor->nodeNbr < nodeNbr)
		{
			continue;
		}

		if (neighbor->nodeNbr > nodeNbr)
		{
			break;
		}

		return neighbor;
	}

	*nextElt = elt;
	return NULL;
}

IonNeighbor	*addNeighbor(IonVdb *ionvdb, unsigned long nodeNbr,
			PsmAddress nextElt)
{
	PsmPartition	ionwm = getIonwm();
	PsmAddress	addr;
	PsmAddress	elt;
	IonNeighbor	*neighbor;
	IonNode		*node;
	PsmAddress	nextNode;

	addr = psm_zalloc(ionwm, sizeof(IonNeighbor));
	if (addr == 0)
	{
		putErrmsg("Can't add neighbor.", NULL);
		return NULL;
	}

	if (nextElt)
	{
		elt = sm_list_insert_before(ionwm, nextElt, addr);
	}
	else
	{
		elt = sm_list_insert_last(ionwm, ionvdb->neighbors, addr);
	}

	if (elt == 0)
	{
		psm_free(ionwm, addr);
		putErrmsg("Can't add neighbor.", NULL);
		return NULL;
	}

	neighbor = (IonNeighbor *) psp(ionwm, addr);
	memset((char *) neighbor, 0, sizeof(IonNeighbor));
	neighbor->nodeNbr = nodeNbr;
	node = findNode(ionvdb, nodeNbr, &nextNode);
	if (node == NULL)
	{
		node = addNode(ionvdb, nodeNbr, nextNode);
		if (node == NULL)
		{
			putErrmsg("Can't add neighbor.", NULL);
			return NULL;
		}
	}

	neighbor->node = psa(ionwm, node);
	return neighbor;
}

IonNode	*findNode(IonVdb *ionvdb, unsigned long nodeNbr, PsmAddress *nextElt)
{
	PsmPartition	ionwm = getIonwm();
	PsmAddress	elt;
	IonNode		*node;

	CHKNULL(ionvdb);
	CHKNULL(nextElt);
	for (elt = sm_list_first(ionwm, ionvdb->nodes); elt;
			elt = sm_list_next(ionwm, elt))
	{
		node = (IonNode *) psp(ionwm, sm_list_data(ionwm, elt));
		CHKNULL(node);
		if (node->nodeNbr < nodeNbr)
		{
			continue;
		}

		if (node->nodeNbr > nodeNbr)
		{
			break;
		}

		return node;
	}

	*nextElt = elt;
	return NULL;
}

IonNode	*addNode(IonVdb *ionvdb, unsigned long nodeNbr, PsmAddress nextElt)
{
	PsmPartition	ionwm = getIonwm();
	PsmAddress	addr;
	PsmAddress	elt;
	IonNode		*node;

	addr = psm_zalloc(ionwm, sizeof(IonNode));
	if (addr == 0)
	{
		putErrmsg("Can't add node.", NULL);
		return NULL;
	}

	if (nextElt)
	{
		elt = sm_list_insert_before(ionwm, nextElt, addr);
	}
	else
	{
		elt = sm_list_insert_last(ionwm, ionvdb->nodes, addr);
	}

	if (elt == 0)
	{
		psm_free(ionwm, addr);
		putErrmsg("Can't add node.", NULL);
		return NULL;
	}

	node = (IonNode *) psp(ionwm, addr);
	CHKNULL(node);
	node->nodeNbr = nodeNbr;
	node->xmits = sm_list_create(ionwm);
	node->origins = sm_list_create(ionwm);
	node->snubs = sm_list_create(ionwm);
	return node;
}

IonOrigin	*findOrigin(IonNode *node, unsigned long neighborNodeNbr,
			PsmAddress *nextElt)
{
	PsmPartition	ionwm = getIonwm();
	PsmAddress	elt;
	IonOrigin	*origin;

	CHKNULL(node);
	CHKNULL(nextElt);
	for (elt = sm_list_first(ionwm, node->origins); elt;
			elt = sm_list_next(ionwm, elt))
	{
		origin = (IonOrigin *) psp(ionwm, sm_list_data(ionwm, elt));
		CHKNULL(origin);
		if (origin->nodeNbr < neighborNodeNbr)
		{
			continue;
		}

		if (origin->nodeNbr > neighborNodeNbr)
		{
			break;
		}

		return origin;
	}

	*nextElt = elt;
	return NULL;
}

IonOrigin	*addOrigin(IonNode *node, unsigned long neighborNodeNbr,
			PsmAddress nextElt)
{
	PsmPartition	ionwm = getIonwm();
	PsmAddress	addr;
	PsmAddress	elt;
	IonOrigin	*origin;

	CHKNULL(node);
	addr = psm_zalloc(ionwm, sizeof(IonOrigin));
	if (addr == 0)
	{
		putErrmsg("Can't add origin.", NULL);
		return NULL;
	}

	if (nextElt)
	{
		elt = sm_list_insert_before(ionwm, nextElt, addr);
	}
	else
	{
		elt = sm_list_insert_last(ionwm, node->origins, addr);
	}

	if (elt == 0)
	{
		psm_free(ionwm, addr);
		putErrmsg("Can't add origin.", NULL);
		return NULL;
	}

	origin = (IonOrigin *) psp(ionwm, addr);
	CHKNULL(origin);
	origin->nodeNbr = neighborNodeNbr;
	origin->owlt = 0;
	return origin;
}

static int	noteXmit(IonNode *node, IonContact *contact)
{
	IonOrigin	*origin;
	PsmAddress	originAddr;
	PsmPartition	ionwm = getIonwm();
	PsmAddress	nextElt;
	PsmAddress	elt;
	IonXmit		*xmit;
	IonXmit		*prevXmit;
	Object		iondbObj;
	IonDB		iondb;
	PsmAddress	addr;
	time_t		currentTime;
	int		secRemaining;
	Scalar		capacity;

	origin = findOrigin(node, contact->fromNode, &nextElt);
	if (origin == NULL)
	{
		origin = addOrigin(node, contact->fromNode, nextElt);
		if (origin == NULL)
		{
			putErrmsg("Can't create origin for xmit.", NULL);
			return -1;
		}
	}

	originAddr = psa(ionwm, origin);

	/*	Find insertion point in xmits list.			*/

	nextElt = 0;
	prevXmit = NULL;
	for (elt = sm_list_first(ionwm, node->xmits); elt;
			elt = sm_list_next(ionwm, elt))
	{
		xmit = (IonXmit *) psp(ionwm, sm_list_data(ionwm, elt));
		CHKERR(xmit);
		if (xmit->toTime > contact->toTime)
		{
			nextElt = elt;
			break;	/*	Have found insertion point.	*/
		}

		/*	Have not located list insertion point yet.	*/

		if (xmit->origin == originAddr)
		{
			if (xmit->toTime == contact->toTime)
			{
				/*	This xmit is already loaded;
				 *	no need to load again.		*/

				return 0;
			}

			/*	This is a prior xmit for the same
			 *	origin.  Remember it, in case it's
			 *	the last prior xmit for the same
			 *	origin.					*/

			prevXmit = xmit;
		}
	}

	addr = psm_zalloc(ionwm, sizeof(IonXmit));
	if (addr == 0)
	{
		putErrmsg("Can't add xmit.", NULL);
		return -1;
	}

	if (nextElt)
	{
		elt = sm_list_insert_before(ionwm, nextElt, addr);
	}
	else
	{
		elt = sm_list_insert_last(ionwm, node->xmits, addr);
	}

	if (elt == 0)
	{
		psm_free(ionwm, addr);
		putErrmsg("Can't add xmit.", NULL);
		return -1;
	}

	xmit = (IonXmit *) psp(ionwm, addr);
	CHKERR(xmit);
	xmit->origin = originAddr;
	xmit->fromTime = contact->fromTime;
	xmit->toTime = contact->toTime;
	xmit->xmitRate = contact->xmitRate;

	/*	If node is a neighbor, must note this addition in
	 *	the aggregated capacities of all subsequent xmits
	 *	from the local node to this neighbor.			*/

	iondbObj = getIonDbObject();
	sdr_read(getIonsdr(), (char *) &iondb, iondbObj, sizeof(IonDB));
	if (origin->nodeNbr != iondb.ownNodeNbr)
	{
		loadScalar(&(xmit->aggrCapacity), 0);
		return 0;	/*	Not a neighbor, capacity N/A.	*/
	}

	/*	Compute amount of transmission capacity gained.		*/

	currentTime = getUTCTime();
	if (currentTime > xmit->toTime)
	{
		secRemaining = 0;
	}
	else
	{
		if (currentTime > xmit->fromTime)
		{
			secRemaining = xmit->toTime - currentTime;
		}
		else
		{
			secRemaining = xmit->toTime - xmit->fromTime;
		}
	}

	loadScalar(&capacity, secRemaining);
	multiplyScalar(&capacity, xmit->xmitRate);

	/*	Compute aggregate capacity for this new xmit itself:
	 *	its own capacity plus the aggregate capacities of all
	 *	earlier xmits from the local node to this neighbor.	*/

	copyScalar(&(xmit->aggrCapacity), &capacity);
	if (prevXmit)
	{
		addToScalar(&(xmit->aggrCapacity), &(prevXmit->aggrCapacity));
	}

	/*	Fwd-propagate the change in aggregate capacity.		*/

	for (elt = nextElt; elt; elt = sm_list_next(ionwm, elt))
	{
		xmit = (IonXmit *) psp(ionwm, sm_list_data(ionwm, elt));
		CHKERR(xmit);
		if (xmit->origin == originAddr)
		{
			addToScalar(&(xmit->aggrCapacity), &capacity);
		}
	}

	return 0;
}

void	forgetXmit(IonNode *node, IonContact *contact)
{
	PsmPartition	ionwm = getIonwm();
	PsmAddress	elt;
	PsmAddress	addr;
	IonXmit		*xmit;
	PsmAddress	originAddr;
	IonOrigin	*origin;
	PsmAddress	nextElt;
	Object		iondbObj;
	IonDB		iondb;
	time_t		currentTime;
	int		secRemaining;
	Scalar		capacity;

	CHKVOID(node);
	CHKVOID(contact);
	for (elt = sm_list_first(ionwm, node->xmits); elt;
			elt = sm_list_next(ionwm, elt))
	{
		addr = sm_list_data(ionwm, elt);
		xmit = (IonXmit *) psp(ionwm, addr);
		CHKVOID(xmit);
		if (xmit->fromTime != contact->fromTime)
		{
			continue;
		}

		originAddr = xmit->origin;
		origin = (IonOrigin *) psp(ionwm, originAddr);
		CHKVOID(origin);
		if (origin->nodeNbr == contact->fromNode)
		{
			break;		/*	Found the xmit.		*/
		}
	}

	if (elt == 0)
	{
		return;			/*	Xmit not found.		*/
	}

	nextElt = sm_list_next(ionwm, elt);
	oK(sm_list_delete(ionwm, elt, NULL, NULL));

	/*	If node is a neighbor, must note this removal in
	 *	the aggregated capacities of all subsequent xmits
	 *	from the local node to this neighbor.			*/

	iondbObj = getIonDbObject();
	sdr_read(getIonsdr(), (char *) &iondb, iondbObj, sizeof(IonDB));
	if (origin->nodeNbr != iondb.ownNodeNbr)
	{
		/*	Node isn't a neighbor, capacity doesn't matter.	*/

		psm_free(ionwm, addr);
		return;
	}

	/*	Compute amount of transmission capacity lost.		*/

	currentTime = getUTCTime();
	if (currentTime > xmit->toTime)
	{
		secRemaining = 0;
	}
	else
	{
		if (currentTime > xmit->fromTime)
		{
			secRemaining = xmit->toTime - currentTime;
		}
		else
		{
			secRemaining = xmit->toTime - xmit->fromTime;
		}
	}

	loadScalar(&capacity, secRemaining);
	multiplyScalar(&capacity, xmit->xmitRate);
	psm_free(ionwm, addr);

	/*	Fwd-propagate the change in aggregate capacity.		*/

	for (elt = nextElt; elt; elt = sm_list_next(ionwm, elt))
	{
		xmit = (IonXmit *) psp(ionwm, sm_list_data(ionwm, elt));
		CHKVOID(xmit);
		if (xmit->origin == originAddr)
		{
			subtractFromScalar(&(xmit->aggrCapacity), &capacity);
		}
	}
}

int	addSnub(IonNode *node, unsigned long neighborNodeNbr)
{
	PsmPartition	ionwm = getIonwm();
	PsmAddress	nextElt;
	PsmAddress	elt;
	IonSnub		*snub;
	PsmAddress	addr;

	/*	Find insertion point in snubs list.			*/

	CHKERR(node);
	nextElt = 0;
	for (elt = sm_list_first(ionwm, node->snubs); elt;
			elt = sm_list_next(ionwm, elt))
	{
		snub = (IonSnub *) psp(ionwm, sm_list_data(ionwm, elt));
		CHKERR(snub);
		if (snub->nodeNbr < neighborNodeNbr)
		{
			continue;
		}

		if (snub->nodeNbr > neighborNodeNbr)
		{
			nextElt = elt;
			break;	/*	Have found insertion point.	*/
		}

		return 0;	/*	Snub has already been added.	*/
	}

	addr = psm_zalloc(ionwm, sizeof(IonSnub));
	if (addr == 0)
	{
		putErrmsg("Can't add snub.", NULL);
		return -1;
	}

	if (nextElt)
	{
		elt = sm_list_insert_before(ionwm, nextElt, addr);
	}
	else
	{
		elt = sm_list_insert_last(ionwm, node->snubs, addr);
	}

	if (elt == 0)
	{
		psm_free(ionwm, addr);
		putErrmsg("Can't add snub.", NULL);
		return -1;
	}

	snub = (IonSnub *) psp(ionwm, addr);
	CHKERR(snub);
	snub->nodeNbr = neighborNodeNbr;
	snub->probeIsDue = 0;
	postProbeEvent(node, snub);	/*	Initial probe event.	*/
	return 0;
}

void	removeSnub(IonNode *node, unsigned long neighborNodeNbr)
{
	PsmPartition	ionwm = getIonwm();
	PsmAddress	elt;
	PsmAddress	addr;
	IonSnub		*snub;

	CHKVOID(node);
	for (elt = sm_list_first(ionwm, node->snubs); elt;
			elt = sm_list_next(ionwm, elt))
	{
		addr = sm_list_data(ionwm, elt);
		snub = (IonSnub *) psp(ionwm, addr);
		CHKVOID(snub);
		if (snub->nodeNbr < neighborNodeNbr)
		{
			continue;
		}

		if (snub->nodeNbr > neighborNodeNbr)
		{
			return;	/*	Snub not found.			*/
		}

		break;		/*	Found the snub to remove.	*/
	}

	if (elt == 0)
	{
		return;			/*	Snub not found.		*/
	}

	oK(sm_list_delete(ionwm, elt, NULL, NULL));
	psm_free(ionwm, addr);
}

PsmAddress	postProbeEvent(IonNode *node, IonSnub *snub)
{
	PsmPartition	ionwm = getIonwm();
	PsmAddress	addr;
	IonProbe	*probe;
	IonVdb		*ionvdb;
	IonNeighbor	*neighbor;
	PsmAddress	nextElt;
	unsigned int	rtlt;		/*	Round-trip light time.	*/
	int		interval = 6;	/*	Minimum 6-sec interval.	*/
	PsmAddress	elt;
	IonProbe	*pr;

	CHKZERO(node);
	CHKZERO(snub);
	addr = psm_zalloc(ionwm, sizeof(IonProbe));
	if (addr == 0)
	{
		putErrmsg("Can't create probe event.", NULL);
		return 0;
	}

	probe = (IonProbe *) psp(ionwm, addr);
	CHKZERO(probe);
	probe->time = getUTCTime();
	probe->destNodeNbr = node->nodeNbr;
	probe->neighborNodeNbr = snub->nodeNbr;

	/*	Schedule next probe of this snubbing neighbor for the
	 *	time that is the current time plus 2x the round-trip
	 *	light time from the local node to the neighbor (but
	 *	at least 6 seconds).					*/
	 
	ionvdb = getIonVdb();
	neighbor = findNeighbor(ionvdb, snub->nodeNbr, &nextElt);
	if (neighbor)
	{
		rtlt = (neighbor->owltOutbound + neighbor->owltInbound) << 1;
		if (rtlt > interval)
		{
			interval = rtlt;
		}
	}

	probe->time += interval;
	for (elt = sm_list_last(ionwm, ionvdb->probes); elt;
			elt = sm_list_prev(ionwm, elt))
	{
		pr = (IonProbe *) psp(ionwm, sm_list_data(ionwm, elt));
		CHKZERO(pr);
		if (pr->time <= probe->time)
		{
			return sm_list_insert_after(ionwm, elt, addr);
		}
	}

	return sm_list_insert_first(ionwm, ionvdb->probes, addr);
}

int	checkForCongestion()
{
	time_t		forecastTime;
	PsmPartition	ionwm;
	int		ionMemIdx;
	Lyst		neighbors;
	Lyst		changes;
	Sdr		sdr;
	Object		iondbObj;
	IonDB		iondb;
	long		maxForecastOccupancy;
	long		maxForecastInTransit;
	long		forecastOccupancy;
	long		forecastInTransit;
	long		netGrowthPerSec;
	IonVdb		*ionvdb;
	PsmAddress	elt1;
	IonNeighbor	*neighbor;
	Object		elt2;
	IonContact	contact;
	unsigned long	neighborNodeNbr;
	LystElt		elt3;
	IonNeighbor	*np = NULL;
	RateChange	*newChange;
	LystElt		elt4;
	RateChange	*change;
	long		secInEpoch;
	long		secAdvanced = 0;
	long		increment;
	long		spaceRemaining;
	long		secUntilOutOfSpace;
	long		netInTransitGrowthPerSec;
	time_t		alarmTime = 0;
	long		delta;
	char		timestampBuffer[TIMESTAMPBUFSZ];
	char		alarmBuffer[40 + TIMESTAMPBUFSZ];
	int		result;

	forecastTime = getUTCTime();
	ionwm = getIonwm();
	ionMemIdx = getIonMemoryMgr();
	neighbors = lyst_create_using(ionMemIdx);
	changes = lyst_create_using(ionMemIdx);
	if (neighbors == NULL || changes == NULL)
	{
		putErrmsg(_cannotForecast() , NULL);
		return -1;		/*	Out of memory.		*/
	}

	sdr = getIonsdr();
	iondbObj = getIonDbObject();
	sdr_begin_xn(sdr);
	sdr_stage(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
 	forecastOccupancy = maxForecastOccupancy = iondb.currentOccupancy;
 	forecastInTransit = maxForecastInTransit = iondb.currentOccupancy;
	netGrowthPerSec = iondb.productionRate - iondb.consumptionRate;
	ionvdb = getIonVdb();
	for (elt1 = sm_list_first(ionwm, ionvdb->neighbors); elt1;
			elt1 = sm_list_next(ionwm, elt1))
	{
		neighbor = (IonNeighbor*) psp(ionwm, sm_list_data(ionwm, elt1));
		CHKERR(neighbor);
		netGrowthPerSec += neighbor->recvRate;
		netGrowthPerSec -= neighbor->xmitRate;
		np = (IonNeighbor *) MTAKE(sizeof(IonNeighbor));
		if (np == NULL)
		{
			sdr_cancel_xn(sdr);
			putErrmsg(_cannotForecast() , NULL);
			return -1;	/*	Out of memory.		*/
		}

		memcpy((char *) np, (char *) neighbor, sizeof(IonNeighbor));
		if (lyst_insert_last(neighbors, np) == NULL)
		{
			sdr_cancel_xn(sdr);
			putErrmsg(_cannotForecast() , NULL);
			return -1;	/*	Out of memory.		*/
		}
	}

	/*	Have now got *current* occupancy and growth rate.
	 *	Next, extract list of all relevant growth rate changes.	*/

	for (elt2 = sdr_list_first(sdr, iondb.contacts); elt2;
			elt2 = sdr_list_next(sdr, elt2))
	{
		sdr_read(sdr, (char *) &contact, sdr_list_data(sdr, elt2),
				sizeof(IonContact));
		if (contact.toTime < forecastTime)
		{
			continue;	/*	Happened in the past.	*/
		}

		if (contact.fromNode == contact.toNode)
		{
			/*	This is a loopback contact, which
			 *	has no net effect on congestion.
			 *	Ignore it.				*/

			continue;
		}

		if (contact.fromNode == iondb.ownNodeNbr)
		{
			neighborNodeNbr = contact.toNode;
		}
		else if (contact.toNode == iondb.ownNodeNbr)
		{
			neighborNodeNbr = contact.fromNode;
		}
		else
		{
			continue;	/*	Don't care about this.	*/
		}

		/*	Find affected neighbor; add if necessary.	*/

		for (elt3 = lyst_first(neighbors); elt3; elt3 = lyst_next(elt3))
		{
			np = (IonNeighbor *) lyst_data(elt3);
			if (np->nodeNbr == neighborNodeNbr)
			{
				break;
			}
		}

		if (elt3 == NULL)	/*	This is a new neighbor.	*/
		{
			np = (IonNeighbor *) MTAKE(sizeof(IonNeighbor));
			if (np == NULL)
			{
				sdr_cancel_xn(sdr);
				putErrmsg(_cannotForecast() , NULL);
				return -1;	/*	Out of memory.	*/
			}

			memset((char *) np, 0, sizeof(IonNeighbor));
			np->nodeNbr = neighborNodeNbr;
			if (lyst_insert_last(neighbors, np) == NULL)
			{
				sdr_cancel_xn(sdr);
				putErrmsg(_cannotForecast() , NULL);
				return -1;	/*	Out of memory.	*/
			}
		}

		/*	Now insert rate change(s).			*/

		if (contact.fromTime < forecastTime)
		{
			/*	The start of this contact is already
			 *	reflected in current netGrowthPerSec,
			 *	so ignore it; just prepare for adding
			 *	RateChange for end of contact.		*/

			elt4 = lyst_first(changes);
		}
		else
		{
			/*	Insert RateChange for start of contact.	*/

			newChange = (RateChange *) MTAKE(sizeof(RateChange));
			if (newChange == NULL)
			{
				sdr_cancel_xn(sdr);
				putErrmsg(_cannotForecast() , NULL);
				return -1;	/*	Out of memory.	*/
			}

			newChange->time = contact.fromTime;
			newChange->xmitRate = contact.xmitRate;
			if (contact.fromNode == iondb.ownNodeNbr)
			{
				newChange->fromNeighbor = 0;
				newChange->prevXmitRate = np->xmitRate;
			}
			else
			{
				newChange->fromNeighbor = 1;
				newChange->prevXmitRate = np->recvRate;
			}

			for (elt4 = lyst_first(changes); elt4;
				elt4 = lyst_next(elt4))
			{
				change = (RateChange *) lyst_data(elt4);
				if (change->time > newChange->time)
				{
					break;
				}
			}

			if (elt4)
			{
				elt4 = lyst_insert_before(elt4, newChange);
			}
			else
			{
				elt4 = lyst_insert_last(changes, newChange);
			}

			if (elt4 == NULL)
			{
				sdr_cancel_xn(sdr);
				putErrmsg(_cannotForecast() , NULL);
				return -1;	/*	Out of memory.	*/
			}
		}

		/*	Insert RateChange for end of contact.	*/

		newChange = (RateChange *) MTAKE(sizeof(RateChange));
		if (newChange == NULL)
		{
			sdr_cancel_xn(sdr);
			putErrmsg(_cannotForecast() , NULL);
			return -1;	/*	Out of memory.		*/
		}

		newChange->time = contact.toTime;
		newChange->xmitRate = 0;
		if (contact.fromNode == iondb.ownNodeNbr)
		{
			newChange->fromNeighbor = 0;
			newChange->prevXmitRate = np->xmitRate;
		}
		else
		{
			newChange->fromNeighbor = 1;
			newChange->prevXmitRate = np->recvRate;
		}

		while (elt4)
		{
			change = (RateChange *) lyst_data(elt4);
			if (change->time > newChange->time)
			{
				break;
			}

			elt4 = lyst_next(elt4);
		}

		if (elt4)
		{
			elt4 = lyst_insert_before(elt4, newChange);
		}
		else
		{
			elt4 = lyst_insert_last(changes, newChange);
		}

		if (elt4 == NULL)
		{
			sdr_cancel_xn(sdr);
			putErrmsg(_cannotForecast() , NULL);
			return -1;	/*	Out of memory.		*/
		}
	}

	/*	Now revise occupancy level over time as growth occurs
	 *	and growth rates change.  Occupancy computation
	 *	optimized by Greg Menke 22 September 2011.		*/

	for (elt4 = lyst_first(changes); elt4; elt4 = lyst_next(elt4))
	{
		change = (RateChange *) lyst_data(elt4);

		/*	Let occupancy level change per current rate
		 *	up to time of next rate change.
		 *
		 *	NOTE: change->time can never be less than
		 *	forecastTime because contacts that started
		 *	in the past are reflected in the initial
		 *	netGrowthPerSec; the start of such a contact
		 *	is never posted to the list of RateChanges. 	*/

		if (change->time < forecastTime)
		{
			putErrmsg("Investigate congestion check error.",
					utoa(change->time));
			continue;
		}

		if (iondb.horizon > 0 && iondb.horizon < change->time) 
		{
			secInEpoch = iondb.horizon - forecastTime;
			if (secInEpoch < 0)
			{
				secInEpoch = 0;
			}
		}
		else
		{
			secInEpoch = change->time - forecastTime;
		}

		if (netGrowthPerSec > 0)
		{
			/*	net growth > 0 means we are receiving
			 *	more than transmitting.  Project the
			 *	rate out till the forecast reaches
			 *	the occupancy ceiling (round up to
			 *	next second) or we reach the end of
			 *	the epoch, whichever occurs first.	*/

			spaceRemaining = iondb.occupancyCeiling
					- forecastOccupancy;
			secUntilOutOfSpace = 1 + (spaceRemaining
					/ netGrowthPerSec);
			if (secInEpoch < secUntilOutOfSpace)
			{
				secAdvanced = secInEpoch;
			}
			else
			{
				secAdvanced = secUntilOutOfSpace;
			}
		}
		else	/*	netGrowthPerSec is <= 0			*/
		{
			/*	Note: net growth < 0 means we are
			 *	transmitting more than receiving.
			 *	Although this rate of growth is the
			 *	one that is theoretically correct
			 *	throughout the epoch, the reduction
			 *	in occupancy will obviously stop
			 *	when occupancy reaches zero; from
			 *	that time until the end of the epoch
			 *	the theoretical negative net growth
			 *	in fact represents unused transmission
			 *	capacity.  We correct for this by
			 *	changing forecastOccupancy to zero
			 *	whenever it is driven negative.		*/

			secAdvanced = secInEpoch;
		}

		increment = netGrowthPerSec * secAdvanced;
		if (netGrowthPerSec > 0 && increment < 0)
		{
			/*	Multiplication overflow.		*/

			forecastOccupancy = iondb.occupancyCeiling;
		}
		else
		{
			forecastOccupancy += increment;
		}

		if (forecastOccupancy < 0)
		{
			forecastOccupancy = 0;
		}

		if (forecastOccupancy > maxForecastOccupancy)
		{
			maxForecastOccupancy = forecastOccupancy;
		}

		/*	The in-transit high-water mark is the total
		 *	occupancy high-water mark less the estimated
		 *	occupancy due to local bundle origination,
		 *	i.e., all bundles originating at other nodes
		 *	that were received at this node and have not
		 *	yet been either forwarded or delivered.  It
		 *	constitutes the available margin for local
		 *	bundle origination and, as such, is the basis
		 *	for local bundle admission control.		*/

		netInTransitGrowthPerSec = netGrowthPerSec 
				- iondb.productionRate;



		increment = netInTransitGrowthPerSec * secAdvanced;
		if (netInTransitGrowthPerSec > 0 && increment < 0)
		{
			/*	Multiplication overflow. Positive Direction*/

			forecastInTransit = iondb.occupancyCeiling;
		}
		else
		{
			if(netInTransitGrowthPerSec<0 && increment > 0)
			{
				/*Multiplication overflow. Negative Direction*/
				netInTransitGrowthPerSec=0;
			}
			else
			{
				forecastInTransit += increment;
			}
		}

		if (forecastInTransit < 0)
		{
			forecastInTransit = 0;
		}

		if (forecastInTransit > maxForecastInTransit)
		{
			maxForecastInTransit = forecastInTransit;
		}
#if 0
		{
			char msg[1024];
			sprintf(msg, "c4c1a: horizon=%d, change->time=%d, forecasttime=%d, netgrowthpersec=%d, productionRate=%d",
					  (int)iondb.horizon,
					  (int)change->time,
					  (int)forecastTime,
					  (int)netGrowthPerSec,
					  (int)iondb.productionRate );

			writeMemo(msg);
			sprintf(msg, "c4c1b: forecastOccupancy=%d, forecastInTransit=%d, secAdvanced=%d",
					  (int)forecastOccupancy,
					  (int)forecastInTransit,
					  (int)secAdvanced );

			writeMemo(msg);
		}
#endif
		/*	Advance the forecast time, by epoch or to end,
		 *	and check for congestion alarm.			*/

		forecastTime += secAdvanced;
		if (maxForecastOccupancy >= iondb.occupancyCeiling)
		{
			alarmTime = forecastTime;
		}

		if (alarmTime != 0
		|| (iondb.horizon > 0 && forecastTime > iondb.horizon))
		{
			break;		/*	Stop forecast.		*/
		}

		/*	Apply the adjustment that occurs at the time
		 *	of this change (end of prior epoch).		*/

		delta = change->xmitRate - change->prevXmitRate;
		if (change->fromNeighbor)
		{
			netGrowthPerSec += delta;
		}
		else
		{
			netGrowthPerSec -= delta;
		}
	}

	/*	Have determined final net growth rate as of end of
	 *	last scheduled contact.					*/

	if (netGrowthPerSec > 0 && alarmTime == 0)
	{
		/*	Unconstrained growth; will max out eventually,
		 *	just need to determine when.  Final epoch.	*/

		if (iondb.horizon > 0) 
		{
			secInEpoch = (iondb.horizon - forecastTime);
		}
		else
		{
			secInEpoch = LONG_MAX;
		}

		/*	net growth > 0 means we are receiving more than
		 *	transmitting.  Project the rate out till the
		 *	forecast reaches the occupancy ceiling (round
		 *	up to the next second) or we reach the end of
		 *	the epoch, whichever occurs first.		*/

		spaceRemaining = iondb.occupancyCeiling - forecastOccupancy;
		secUntilOutOfSpace = 1 + (spaceRemaining / netGrowthPerSec);
		if (secInEpoch < secUntilOutOfSpace)
		{
			secAdvanced = secInEpoch;
		}
		else
		{
			secAdvanced = secUntilOutOfSpace;
		}

		increment = netGrowthPerSec * secAdvanced;
		if (increment < 0)
		{
			/*	Multiplication overflow.		*/

			forecastOccupancy = iondb.occupancyCeiling;
		}
		else
		{
			forecastOccupancy += increment;
		}

		if (forecastOccupancy > maxForecastOccupancy)
		{
			maxForecastOccupancy = forecastOccupancy;
		}

		netInTransitGrowthPerSec = netGrowthPerSec 
				- iondb.productionRate;
		increment = netInTransitGrowthPerSec * secAdvanced;
		if (netInTransitGrowthPerSec > 0 && increment < 0)
		{
			/*	Multiplication overflow.		*/

			forecastInTransit = iondb.occupancyCeiling;
		}
		else
		{
			forecastInTransit += increment;
		}

		if (forecastInTransit < 0)
		{
			forecastInTransit = 0;
		}

		if (forecastInTransit > maxForecastInTransit)
		{
			maxForecastInTransit = forecastInTransit;
		}

		forecastTime += secAdvanced;
		if (maxForecastOccupancy >= iondb.occupancyCeiling)
		{
			alarmTime = forecastTime;
		}
	}

	if (alarmTime == 0)
	{
		writeMemo("[i] No congestion collapse predicted.");
	}
	else
	{
		/*	Have determined time at which occupancy limit
		 *	will be reached.				*/

		writeTimestampUTC(alarmTime, timestampBuffer);
		isprintf(alarmBuffer, sizeof alarmBuffer,
				"[i] Congestion collapse forecast: %s.",
				timestampBuffer);
		writeMemo(alarmBuffer);
		if (iondb.alarmScript)
		{
			sdr_string_read(sdr, alarmBuffer, iondb.alarmScript);
			pseudoshell(alarmBuffer);
		}
	}

	/*	In any case, update maxForecastOccupancy and InTransit.	*/

	iondb.maxForecastOccupancy = maxForecastOccupancy;
	iondb.maxForecastInTransit = maxForecastInTransit;
	sdr_write(sdr, iondbObj, (char *) &iondb, sizeof(IonDB));
	result = sdr_end_xn(sdr);

	/*	Release memory used for neighbors and changes lists.	*/

	elt3 = lyst_first(neighbors);
	while (elt3)
	{
		np = (IonNeighbor *) lyst_data(elt3);
		MRELEASE(np);
		lyst_delete(elt3);
		elt3 = lyst_first(neighbors);
	}

	lyst_destroy(neighbors);
	elt4 = lyst_first(changes);
	while (elt4)
	{
		change = (RateChange *) lyst_data(elt4);
		MRELEASE(change);
		lyst_delete(elt4);
		elt4 = lyst_first(changes);
	}

	lyst_destroy(changes);
	if (result < 0)
	{
		putErrmsg("Failed on check for congestion.", NULL);
		return -1;
	}

	return 0;
}

static int	isExcluded(unsigned long nodeNbr, Lyst excludedNodes)
{
	LystElt elt;

	for (elt = lyst_first(excludedNodes); elt; elt = lyst_next(elt))
	{
		if ((unsigned long) lyst_data(elt) == nodeNbr)
		{
			return 1;	/*	Node is in the list.	*/
		}
	}

	return 0;
}

static int	assessContacts(IonNode *node, time_t deadline,
			time_t *mootAfter, Lyst excludedNodes)
{
	PsmPartition	ionwm = getIonwm();
	LystElt		exclusion;
	PsmAddress	elt;
	IonXmit		*xmit;
	IonOrigin	*origin;
	unsigned long	owltMargin;
	unsigned long	lastChanceFromOrigin;
	IonNode		*originNode;
	IonVdb		*ionvdb = getIonVdb();
	PsmAddress	nextNode;

	/*	Make sure we don't get into a routing loop while
	 *	trying to compute all paths to this node.		*/

	exclusion = lyst_insert_last(excludedNodes, (void *) (node->nodeNbr));
	if (exclusion == NULL)
	{
		putErrmsg("Can't identify proximate nodes.", NULL);
		return -1;
	}

	/*	Examine all opportunities for transmission to node.	*/

	for (elt = sm_list_last(ionwm, node->xmits); elt;
			elt = sm_list_prev(ionwm, elt))
	{
		xmit = (IonXmit *) psp(ionwm, sm_list_data(ionwm, elt));
		origin = (IonOrigin *) psp(ionwm, xmit->origin);
		if (isExcluded(origin->nodeNbr, excludedNodes))
		{
			/*	Can't continue -- it would be a routing
			 *	loop.  We've already computed all
			 *	routes on this path that go through
			 *	this origin node.			*/

			continue;
		}

		/*	Not a loop.  Compute this contact's mootAfter
		 *	time if not already known.			*/

		if (xmit->mootAfter == MAX_POSIX_TIME)	/*	unknown	*/
		{
			/*	By default, unconditionally moot.	*/

			xmit->mootAfter = 0;

			/*	Does contact start after the last
			 *	possible moment for transmission to
			 *	this node before the node's own
			 *	transmission opportunity ends?  If so,
			 *	it's unusable.				*/

			owltMargin = ((MAX_SPEED_MPH / 3600) * origin->owlt)
					/ 186282;
			lastChanceFromOrigin = deadline
					- (origin->owlt + owltMargin);
			if (xmit->fromTime > lastChanceFromOrigin)
			{
				continue;	/*	Non-viable.	*/
			}

			/*	This is a viable opportunity to
			 *	transmit this bundle to the station
			 *	node from the indicated origin node.	*/

			if (origin->nodeNbr == getOwnNodeNbr())
			{
				xmit->mootAfter = xmit->toTime;
			}
			else
			{
				originNode = findNode(ionvdb, origin->nodeNbr,
						&nextNode);
				if (originNode == NULL)
				{
					/*	Not in contact plan.
					 *	No way to compute
					 *	mootAfter time for
					 *	this contact.		*/

					continue;
				}

				/*	Compute mootAfter time as the
				 *	end of the latest-ending contact
				 *	from the local node that is on
				 *	any viable path to this xmit's
				 *	origin node.			*/

				if (assessContacts(originNode, xmit->toTime,
					&(xmit->mootAfter), excludedNodes))
				{
					putErrmsg("Can't set mootAfter time.",
							NULL);
					break;
				}
			}
		}

		/*	Have computed mootAfter time for this contact.
		 *	Now, if that time is later than the current
		 *	guess at the best-case mootAfter to pass back
		 *	for this assessment, it becomes the new best-
		 *	case mootAfter.					*/

		if (xmit->mootAfter > *mootAfter)
		{
			*mootAfter = xmit->mootAfter;
		}
	}

	/*	No more opportunities for transmission to this node.	*/

	lyst_delete(exclusion);
	return 0;
}

static int	setMootAfterTimes()
{
	time_t		maxTime = MAX_POSIX_TIME;
	PsmPartition	ionwm = getIonwm();
	IonVdb		*ionvdb = getIonVdb();
	unsigned long	ownNodeNbr = getOwnNodeNbr();
	Lyst		excludedNodes;
	PsmAddress	elt;
	IonNode		*node;
	PsmAddress	elt2;
	IonXmit		*xmit;
	time_t		mootAfter;

	excludedNodes = lyst_create_using(getIonMemoryMgr());
	if (excludedNodes == NULL)
	{
		putErrmsg("Can't create excludedNodes list.", NULL);
		return -1;
	}

	/*	First initialize all mootAfter times to max time.	*/

	for (elt = sm_list_first(ionwm, ionvdb->nodes); elt;
			elt = sm_list_next(ionwm, elt))
	{
		node = (IonNode *) psp(ionwm, sm_list_data(ionwm, elt));
		for (elt2 = sm_list_first(ionwm, node->xmits); elt2;
				elt2 = sm_list_next(ionwm, elt2))
		{
			xmit = (IonXmit *) psp(ionwm,
					sm_list_data(ionwm, elt2));
			xmit->mootAfter = maxTime;	/*	unknown	*/
		}
	}

	/*	Now compute new mootAfter times for all contacts.	*/

	for (elt = sm_list_first(ionwm, ionvdb->nodes); elt;
			elt = sm_list_next(ionwm, elt))
	{
		node = (IonNode *) psp(ionwm, sm_list_data(ionwm, elt));
		if (node->nodeNbr == ownNodeNbr)
		{
			continue;
		}

		mootAfter = 0;
		if (assessContacts(node, maxTime, &mootAfter, excludedNodes))
		{
			putErrmsg("Can't set mootAfter times.", NULL);
			break;
		}
	}

	lyst_destroy(excludedNodes);
	return 0;
}

Object	rfx_insert_contact(time_t fromTime, time_t toTime,
		unsigned long fromNode, unsigned long toNode,
		unsigned long xmitRate)
{
	Sdr		sdr;
	Object		iondbObj;
	IonDB		iondb;
	Object		elt;
	Object		obj;
	IonContact	contact;
	Object		newElt = 0;
	IonVdb		*ionvdb;
	IonNode		*node;
	PsmAddress	nextElt;

	CHKZERO(toTime > fromTime);
	sdr = getIonsdr();
	iondbObj = getIonDbObject();
	sdr_read(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
	sdr_begin_xn(sdr);
	for (elt = sdr_list_first(sdr, iondb.contacts); elt;
			elt = sdr_list_next(sdr, elt))
	{
		obj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &contact, obj, sizeof(IonContact));
		if (contact.fromTime < fromTime)
		{
			continue;
		}

		if (contact.fromTime > fromTime)
		{
			break;
		}

		if (contact.fromNode < fromNode)
		{
			continue;
		}

		if (contact.fromNode > fromNode)
		{
			break;
		}

		if (contact.toNode < toNode)
		{
			continue;
		}

		if (contact.toNode > toNode)
		{
			break;
		}

		/*	Contact has been located in database.		*/

		sdr_cancel_xn(sdr);
		if (contact.xmitRate == xmitRate)
		{
			return elt;
		}

		putErrmsg("Current data rate for this interval not revised.",
				utoa(contact.xmitRate));
		return 0;
	}

	/*	Contact isn't already in database; okay to add.		*/

	contact.fromTime = fromTime;
	contact.toTime = toTime;
	contact.fromNode = fromNode;
	contact.toNode = toNode;
	contact.xmitRate = xmitRate;
	obj = sdr_malloc(sdr, sizeof(IonContact));
	if (obj)
	{
		sdr_write(sdr, obj, (char *) &contact, sizeof(IonContact));
		if (elt)
		{
			newElt = sdr_list_insert_before(sdr, elt, obj);
		}
		else
		{
			newElt = sdr_list_insert_last(sdr, iondb.contacts, obj);
		}
	}

	/*	If contact bears on routing, note xmit.			*/

	if (toNode != iondb.ownNodeNbr		/*	To remote node.	*/
	|| fromNode == iondb.ownNodeNbr)	/*	Loopback.	*/
	{
		ionvdb = getIonVdb();
		node = findNode(ionvdb, contact.toNode, &nextElt);
		if (node == NULL)
		{
			node = addNode(ionvdb, contact.toNode, nextElt);
			if (node == NULL)
			{
				sdr_cancel_xn(sdr);
				putErrmsg("Can't add xmit.", NULL);
				return 0;
			}
		}

		noteXmit(node, &contact);
		if (setMootAfterTimes() < 0)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("Can't update mootAfter times.", NULL);
			return 0;
		}
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't insert contact.", NULL);
		return 0;
	}

	return newElt;
}

char	*rfx_print_contact(Object obj, char *buffer)
{
	Sdr		sdr;
	IonContact	contact;
	char		fromTimeBuffer[TIMESTAMPBUFSZ];
	char		toTimeBuffer[TIMESTAMPBUFSZ];

	CHKNULL(obj);
	CHKNULL(buffer);
	sdr = getIonsdr();
	sdr_read(sdr, (char *) &contact, obj, sizeof(IonContact));
	writeTimestampUTC(contact.fromTime, fromTimeBuffer);
	writeTimestampUTC(contact.toTime, toTimeBuffer);
	isprintf(buffer, RFX_NOTE_LEN, "From %20s to %20s the xmit rate from \
node %10lu to node %10lu is %10lu bytes/sec.", fromTimeBuffer, toTimeBuffer,
			contact.fromNode, contact.toNode, contact.xmitRate);
	return buffer;
}

int	rfx_remove_contact(time_t fromTime, unsigned long fromNode,
		unsigned long toNode)
{
	Sdr		sdr;
	Object		iondbObj;
	IonDB		iondb;
	Object		elt;
	Object		nextElt;
	Object		obj;
	IonContact	contact;
	IonVdb		*ionvdb;
	IonNode		*node;
	PsmAddress	nextNodeElt;
	int		removalCount = 0;

	/*	Note: when the fromTIme passed to ionadmin is '*'
	 *	a fromTime of zero is passed to rfx_remove_contact,
	 *	where it is interpreted as "all contacts between
	 *	these two nodes".					*/

	sdr = getIonsdr();
	iondbObj = getIonDbObject();
	sdr_read(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
	sdr_begin_xn(sdr);
	for (elt = sdr_list_first(sdr, iondb.contacts); elt; elt = nextElt)
	{
		nextElt = sdr_list_next(sdr, elt);
		obj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &contact, obj, sizeof(IonContact));
		if (fromTime != 0)	/*	Not wildcard removal.	*/
		{
			/*	fromTimes must exactly match in order
			 *	for contact to be removed.  Contacts
			 *	are in fromTime/fromNode/toNode order.	*/

			if (contact.fromTime < fromTime)
			{
				continue;	/*	Keep looking.	*/
			}

			if (contact.fromTime > fromTime)
			{
				break;	/*	Contact not found.	*/
			}
		}

		if (contact.fromNode != fromNode)
		{
			continue;		/*	Keep looking.	*/
		}

		if (contact.toNode != toNode)
		{
			continue;
		}

		/*	Found a contact to remove.			*/

		sdr_free(sdr, obj);
		sdr_list_delete(sdr, elt, NULL, NULL);

		/*	If contact bears on routing, remove xmit.	*/

		if (toNode != iondb.ownNodeNbr	/*	To remote node.	*/
		|| fromNode == iondb.ownNodeNbr)/*	Loopback.	*/
		{
			ionvdb = getIonVdb();
			node = findNode(ionvdb, toNode, &nextNodeElt);
			if (node)
			{
				forgetXmit(node, &contact);
				if (setMootAfterTimes() < 0)
				{
					sdr_cancel_xn(sdr);
					putErrmsg("Can't update mootAfter \
times.", NULL);
					return -1;
				}
			}
		}

		removalCount += 1;
		if (fromTime == 0)	/*	Wild-card removal.	*/
		{
			continue;	/*	Keep looking.		*/
		}

		break;	/*	Have removed the only possible match.	*/
	}

	if (removalCount > 0)
	{
		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Can't remove contact(s).", NULL);
			return -1;
		}

		return 0;
	}

	sdr_cancel_xn(sdr);
	writeMemo("[?] Contact not found in database.");
	return 0;
}

Object	rfx_insert_range(time_t fromTime, time_t toTime, unsigned long fromNode,
		unsigned long toNode, unsigned int owlt)
{
	Sdr		sdr;
	Object		iondbObj;
	IonDB		iondb;
	Object		elt;
	Object		obj;
	IonRange	range;
	char		rangeIdString[128];
	Object		newElt = 0;

	CHKZERO(toTime > fromTime);

	/*	Note that ranges are normally assumed to be symmetrical,
	 *	i.e., the signal propagation time from B to A is normally
	 *	assumed to be the same as the signal propagation time
	 *	from A to B.  For this reason, normally only the A->B
	 *	range (where A is a node number that is less than node
	 *	number B) need be entered; when ranges are applied to
	 *	the Origin objects of Nodes in the ION database, the
	 *	A->B range is stored as the OWLT of the "A" origin object
	 *	for node "B" and also as the OWLT of the "B" origin
	 *	object for node "A".
	 *
	 *	However, it is possible to insert asymmetric ranges, as
	 *	would apply when the forward and return traffic between
	 *	some pair of nodes travels by different transmission
	 *	paths that introduce different latencies.  When this is
	 *	the case, both the A->B and B->A ranges must be entered.
	 *	The A->B range is initially processed as a symmetric
	 *	range as described above, but when the B->A range is
	 *	subsequently applied (ranges are applied in ascending
	 *	"from" node order) it overrides the default OWLT of the
	 *	"B" origin object for node A.				*/

	sdr = getIonsdr();
	iondbObj = getIonDbObject();
	sdr_read(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
	sdr_begin_xn(sdr);
	for (elt = sdr_list_first(sdr, iondb.ranges); elt;
			elt = sdr_list_next(sdr, elt))
	{
		obj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &range, obj, sizeof(IonRange));
		if (range.fromTime < fromTime)
		{
			continue;
		}

		if (range.fromTime > fromTime)
		{
			break;
		}

		if (range.fromNode < fromNode)
		{
			continue;
		}

		if (range.fromNode > fromNode)
		{
			break;
		}

		if (range.toNode < toNode)
		{
			continue;
		}

		if (range.toNode > toNode)
		{
			break;
		}

		/*	Range has been located in database.		*/

		sdr_cancel_xn(sdr);
		if (range.owlt == owlt)
		{
			return elt;
		}

		isprintf(rangeIdString, sizeof rangeIdString,
			"from %lu, %lu->%lu", fromTime, fromNode, toNode);
		writeMemoNote("[?] Range OWLT not revised", rangeIdString);
		return 0;
	}

	/*	Range isn't already in database; okay to add.		*/

	range.fromTime = fromTime;
	range.toTime = toTime;
	range.fromNode = fromNode;
	range.toNode = toNode;
	range.owlt = owlt;
	obj = sdr_malloc(sdr, sizeof(IonRange));
	if (obj)
	{
		sdr_write(sdr, obj, (char *) &range, sizeof(IonRange));
		if (elt)
		{
			newElt = sdr_list_insert_before(sdr, elt, obj);
		}
		else
		{
			newElt = sdr_list_insert_last(sdr, iondb.ranges, obj);
		}
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't insert range.", NULL);
		return 0;
	}

	return newElt;
}

char	*rfx_print_range(Object obj, char *buffer)
{
	Sdr		sdr;
	IonRange	range;
	char		fromTimeBuffer[TIMESTAMPBUFSZ];
	char		toTimeBuffer[TIMESTAMPBUFSZ];

	if (obj == 0)
	{
		return NULL;
	}

	CHKNULL(buffer);
	sdr = getIonsdr();
	sdr_read(sdr, (char *) &range, obj, sizeof(IonRange));
	writeTimestampUTC(range.fromTime, fromTimeBuffer);
	writeTimestampUTC(range.toTime, toTimeBuffer);
	isprintf(buffer, RFX_NOTE_LEN, "From %20s to %20s the OWLT from node \
%10lu to node %10lu is %10u seconds.", fromTimeBuffer, toTimeBuffer,
			range.fromNode, range.toNode, range.owlt);
	return buffer;
}

int	rfx_remove_range(time_t fromTime, unsigned long fromNode,
		unsigned long toNode)
{
	Sdr		sdr;
	Object		iondbObj;
	IonDB		iondb;
	Object		elt;
	Object		nextElt;
	Object		obj;
	IonRange	range;
	char		rangeIdString[128];
	int		removalCount = 0;

	sdr = getIonsdr();
	iondbObj = getIonDbObject();
	sdr_read(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
	sdr_begin_xn(sdr);
	for (elt = sdr_list_first(sdr, iondb.ranges); elt; elt = nextElt)
	{
		nextElt = sdr_list_next(sdr, elt);
		obj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &range, obj, sizeof(IonRange));
		if (fromTime != 0)	/*	Not wildcard removal.	*/
		{
			/*	fromTimes must exactly match in order
			 *	for range to be deleted.  Ranges are
			 *	in fromTime/fromNode/toNode order.	*/

			if (range.fromTime < fromTime)
			{
				continue;	/*	Keep looking.	*/
			}

			if (range.fromTime > fromTime)
			{
				break;	/*	Range not found.	*/
			}
		}

		if (range.fromNode != fromNode)
		{
			continue;
		}

		if (range.toNode != toNode)
		{
			continue;
		}

		/*	Found a range to remove.			*/

		sdr_free(sdr, obj);
		sdr_list_delete(sdr, elt, NULL, NULL);
		removalCount += 1;
		if (fromTime == 0)	/*	Wild-card removal.	*/
		{
			continue;		/*	Keep looking.	*/
		}

		break;
	}

	if (removalCount > 0)
	{
		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Can't remove range(s).", NULL);
			return -1;
		}

		return 0;
	}

	sdr_cancel_xn(sdr);
	isprintf(rangeIdString, sizeof rangeIdString, "from %lu, %lu->%lu",
				fromTime, fromNode, toNode);
	writeMemoNote("[?] Range not found in database", rangeIdString);
	return 0;
}

int	rfx_start()
{
	Sdr		sdr;
	IonVdb		*vdb;
	Object		iondbObj;
	IonDB		iondb;
	Object		elt;
	Object		obj;
	IonContact	contact;
	IonNode		*node;
	PsmAddress	nextElt;

	sdr = getIonsdr();
	vdb = getIonVdb();

	/*	Reload all xmit objects, as necessary.			*/

	iondbObj = getIonDbObject();
	sdr_begin_xn(sdr);	/*	Just to lock memory.		*/
	sdr_read(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
	for (elt = sdr_list_first(sdr, iondb.contacts); elt;
			elt = sdr_list_next(sdr, elt))
	{
		obj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &contact, obj, sizeof(IonContact));

		/*	If contact bears on routing, note xmit.		*/

		if (contact.toNode != iondb.ownNodeNbr
		|| contact.fromNode == iondb.ownNodeNbr)
		{
			node = findNode(vdb, contact.toNode, &nextElt);
			if (node == NULL)
			{
				node = addNode(vdb, contact.toNode, nextElt);
				if (node == NULL)
				{
					sdr_cancel_xn(sdr);
					putErrmsg("Can't reload xmit.", NULL);
					return -1;
				}
			}

			noteXmit(node, &contact);
		}
	}

	/*	Start the rfx clock if necessary.			*/

	if (vdb->clockPid == ERROR || sm_TaskExists(vdb->clockPid) == 0)
	{
		vdb->clockPid = pseudoshell("rfxclock");
	}

	sdr_cancel_xn(sdr);	/*	Unlock memory.			*/
	return 0;
}

void	rfx_stop()
{
	IonVdb	*vdb;

	vdb = getIonVdb();
	if (vdb->clockPid != ERROR)
	{
		sm_TaskKill(vdb->clockPid, SIGTERM);
		while (sm_TaskExists(vdb->clockPid))
		{
			microsnooze(100000);
		}

		vdb->clockPid = ERROR;
	}
}
