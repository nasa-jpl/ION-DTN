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

/*	*	Red-black tree ordering and deletion functions	*	*/

static int	rfx_order_nodes(PsmPartition partition, PsmAddress nodeData,
			void *dataBuffer)
{
	IonNode	*node;
	IonNode	*argNode;

	node = (IonNode *) psp(partition, nodeData);
	argNode = (IonNode *) dataBuffer;
	if (node->nodeNbr < argNode->nodeNbr)
	{
		return -1;
	}

	if (node->nodeNbr > argNode->nodeNbr)
	{
		return 1;
	}

	/*	Matching node number.					*/

	return 0;
}

static int	rfx_order_neighbors(PsmPartition partition, PsmAddress nodeData,
			void *dataBuffer)
{
	IonNeighbor	*neighbor;
	IonNeighbor	*argNeighbor;

	neighbor = (IonNeighbor *) psp(partition, nodeData);
	argNeighbor = (IonNeighbor *) dataBuffer;
	if (neighbor->nodeNbr < argNeighbor->nodeNbr)
	{
		return -1;
	}

	if (neighbor->nodeNbr > argNeighbor->nodeNbr)
	{
		return 1;
	}

	/*	Matching node number.					*/

	return 0;
}

int	rfx_order_ranges(PsmPartition partition, PsmAddress nodeData,
		void *dataBuffer)
{
	IonRXref	*range;
	IonRXref	*argRange;

	if (partition == NULL || nodeData == 0 || dataBuffer == NULL)
	{
		putErrmsg("Error calling smrbt rangeIndex compare function.",
				NULL);
		return 0;
	}

	range = (IonRXref *) psp(partition, nodeData);
	argRange = (IonRXref *) dataBuffer;
	if (range->fromNode < argRange->fromNode)
	{
		return -1;
	}

	if (range->fromNode > argRange->fromNode)
	{
		return 1;
	}

	/*	Matching "from" node number.				*/

	if (range->toNode < argRange->toNode)
	{
		return -1;
	}

	if (range->toNode > argRange->toNode)
	{
		return 1;
	}

	/*	Matching "to" node number as well.			*/

	if (range->fromTime < argRange->fromTime)
	{
		return -1;
	}

	if (range->fromTime > argRange->fromTime)
	{
		return 1;
	}

	/*	Matching start time too.				*/

	return 0;
}

int	rfx_order_contacts(PsmPartition partition, PsmAddress nodeData,
		void *dataBuffer)
{
	IonCXref	*contact;
	IonCXref	*argContact;

	if (partition == NULL || nodeData == 0 || dataBuffer == NULL)
	{
		putErrmsg("Error calling smrbt contactIndex compare function.",
				NULL);
		return 0;
	}

	contact = (IonCXref *) psp(partition, nodeData);
	argContact = (IonCXref *) dataBuffer;
	if (contact->fromNode < argContact->fromNode)
	{
		return -1;
	}

	if (contact->fromNode > argContact->fromNode)
	{
		return 1;
	}

	/*	Matching "from" node number.				*/

	if (contact->toNode < argContact->toNode)
	{
		return -1;
	}

	if (contact->toNode > argContact->toNode)
	{
		return 1;
	}

	/*	Matching "to" node number as well.			*/

	if (contact->fromTime < argContact->fromTime)
	{
		return -1;
	}

	if (contact->fromTime > argContact->fromTime)
	{
		return 1;
	}

	/*	Matching start time too.				*/

	return 0;
}

int	rfx_order_events(PsmPartition partition, PsmAddress nodeData,
		void *dataBuffer)
{
	IonEvent	*event;
	IonEvent	*argEvent;

	event = (IonEvent *) psp(partition, nodeData);
	argEvent = (IonEvent *) dataBuffer;
	if (event->time < argEvent->time)
	{
		return -1;
	}

	if (event->time > argEvent->time)
	{
		return 1;
	}

	/*	Matching schedule time.					*/

	if (event->type < argEvent->type)
	{
		return -1;
	}

	if (event->type > argEvent->type)
	{
		return 1;
	}

	/*	Matching event type as well.				*/

	if (event->ref < argEvent->ref)
	{
		return -1;
	}

	if (event->ref > argEvent->ref)
	{
		return 1;
	}

	/*	Matching cross-referenced object also.			*/
	
	return 0;
}

void	rfx_erase_data(PsmPartition partition, PsmAddress nodeData,
		void *argument)
{
	psm_free(partition, nodeData);
}

/*	*	*	RFX utility functions	*	*	*	*/

int	rfx_system_is_started()
{
	IonVdb	*vdb = getIonVdb();

	return (vdb && vdb->clockPid != ERROR);
}

IonNeighbor	*findNeighbor(IonVdb *ionvdb, uvast nodeNbr,
			PsmAddress *nextElt)
{
	PsmPartition	ionwm = getIonwm();
	IonNeighbor	arg;
	PsmAddress	elt;

	CHKNULL(ionvdb);
	CHKNULL(nextElt);
	memset((char *) &arg, 0, sizeof(IonNeighbor));
	arg.nodeNbr = nodeNbr;
	elt = sm_rbt_search(ionwm, ionvdb->neighbors, rfx_order_neighbors,
			&arg, nextElt);
	if (elt)
	{
		return (IonNeighbor *) psp(ionwm, sm_rbt_data(ionwm, elt));
	}

	return NULL;
}

IonNeighbor	*addNeighbor(IonVdb *ionvdb, uvast nodeNbr)
{
	PsmPartition	ionwm = getIonwm();
	IonNode		*node;
	PsmAddress	nextElt;
	PsmAddress	addr;
	PsmAddress	elt;
	IonNeighbor	*neighbor;

	node = findNode(ionvdb, nodeNbr, &nextElt);
	if (node == NULL)
	{
		node = addNode(ionvdb, nodeNbr);
		if (node == NULL)
		{
			putErrmsg("Can't add neighboring node.", NULL);
			return NULL;
		}
	}

	addr = psm_zalloc(ionwm, sizeof(IonNeighbor));
	if (addr == 0)
	{
		putErrmsg("Can't add neighbor.", NULL);
		return NULL;
	}

	neighbor = (IonNeighbor *) psp(ionwm, addr);
	CHKNULL(neighbor);
	memset((char *) neighbor, 0, sizeof(IonNeighbor));
	neighbor->nodeNbr = nodeNbr;
	neighbor->node = psa(ionwm, node);
	elt = sm_rbt_insert(ionwm, ionvdb->neighbors, addr, rfx_order_neighbors,
			neighbor);
	if (elt == 0)
	{
		psm_free(ionwm, addr);
		putErrmsg("Can't add neighbor.", NULL);
		return NULL;
	}

	return neighbor;
}

IonNode	*findNode(IonVdb *ionvdb, uvast nodeNbr, PsmAddress *nextElt)
{
	PsmPartition	ionwm = getIonwm();
	IonNode		arg;
	PsmAddress	elt;

	CHKNULL(ionvdb);
	CHKNULL(nextElt);
	memset((char *) &arg, 0, sizeof(IonNode));
	arg.nodeNbr = nodeNbr;
	elt = sm_rbt_search(ionwm, ionvdb->nodes, rfx_order_nodes, &arg,
			nextElt);
	if (elt)
	{
		return (IonNode *) psp(ionwm, sm_rbt_data(ionwm, elt));
	}

	return NULL;
}

IonNode	*addNode(IonVdb *ionvdb, uvast nodeNbr)
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

	node = (IonNode *) psp(ionwm, addr);
	CHKNULL(node);
	memset((char *) node, 0, sizeof(IonNode));
	node->nodeNbr = nodeNbr;
	elt = sm_rbt_insert(ionwm, ionvdb->nodes, addr, rfx_order_nodes, node);
	if (elt == 0)
	{
		psm_free(ionwm, addr);
		putErrmsg("Can't add node.", NULL);
		return NULL;
	}

	node->snubs = sm_list_create(ionwm);
	return node;
}

int	addSnub(IonNode *node, uvast neighborNodeNbr)
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

void	removeSnub(IonNode *node, uvast neighborNodeNbr)
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

/*	*	RFX contact list management functions	*	*	*/

static PsmAddress	insertCXref(IonCXref *cxref)
{
	PsmPartition	ionwm = getIonwm();
	IonVdb 		*vdb = getIonVdb();
	IonNode		*node;
	PsmAddress	nextElt;
	PsmAddress	cxaddr;
	Object		iondbObj;
	IonDB		iondb;
	PsmAddress	cxelt;
	PsmAddress	addr;
	IonEvent	*event;
	time_t		currentTime = getUTCTime();

	/*	Load the affected nodes.				*/

	node = findNode(vdb, cxref->toNode, &nextElt);
	if (node == NULL)
	{
		node = addNode(vdb, cxref->toNode);
		if (node == NULL)
		{
			return 0;
		}
	}

	node = findNode(vdb, cxref->fromNode, &nextElt);
	if (node == NULL)
	{
		node = addNode(vdb, cxref->fromNode);
		if (node == NULL)
		{
			return 0;
		}
	}

	/*	Construct the contact index entry.			*/

	cxaddr = psm_zalloc(ionwm, sizeof(IonCXref));
	if (cxaddr == 0)
	{
		return 0;
	}

	/*	Compute times of relevant events.			*/

	iondbObj = getIonDbObject();
	sdr_read(getIonsdr(), (char *) &iondb, iondbObj, sizeof(IonDB));
	if (cxref->fromNode == getOwnNodeNbr())
	{
		/*	Be a little slow to start transmission, and
		 *	a little quick to stop, to ensure that
		 *	segments arrive only when neighbor is
		 *	expecting them.					*/

		cxref->startXmit = cxref->fromTime + iondb.maxClockError;
		cxref->stopXmit = cxref->toTime - iondb.maxClockError;
	}

	if (cxref->toNode == getOwnNodeNbr())
	{
		/*	Be a little slow to resume timers, and a
		 *	little quick to suspend them, to minimize the
		 *	chance of premature timeout.			*/

		cxref->startFire = cxref->fromTime + iondb.maxClockError;
		cxref->stopFire = cxref->toTime - iondb.maxClockError;
	}
	else	/*	Not a transmission to the local node.		*/
	{
		cxref->purgeTime = cxref->toTime;
	}

	memcpy((char *) psp(ionwm, cxaddr), (char *) cxref, sizeof(IonCXref));
	cxelt = sm_rbt_insert(ionwm, vdb->contactIndex, cxaddr,
			rfx_order_contacts, cxref);
	if (cxelt == 0)
	{
		psm_free(ionwm, cxaddr);
		return 0;
	}

	/*	Insert relevant timeline events.			*/

	if (cxref->startXmit)
	{
		addr = psm_zalloc(ionwm, sizeof(IonEvent));
		if (addr == 0)
		{
			return 0;
		}

		event = (IonEvent *) psp(ionwm, addr);
		event->time = cxref->startXmit;
		event->type = IonStartXmit;
		event->ref = cxaddr;
		if (sm_rbt_insert(ionwm, vdb->timeline, addr, rfx_order_events,
				event) == 0)
		{
			psm_free(ionwm, addr);
			return 0;
		}
	}

	if (cxref->stopXmit)
	{
		addr = psm_zalloc(ionwm, sizeof(IonEvent));
		if (addr == 0)
		{
			return 0;
		}

		event = (IonEvent *) psp(ionwm, addr);
		event->time = cxref->stopXmit;
		event->type = IonStopXmit;
		event->ref = cxaddr;
		if (sm_rbt_insert(ionwm, vdb->timeline, addr, rfx_order_events,
				event) == 0)
		{
			psm_free(ionwm, addr);
			return 0;
		}
	}

	if (cxref->startFire)
	{
		addr = psm_zalloc(ionwm, sizeof(IonEvent));
		if (addr == 0)
		{
			return 0;
		}

		event = (IonEvent *) psp(ionwm, addr);
		event->time = cxref->startFire;
		event->type = IonStartFire;
		event->ref = cxaddr;
		if (sm_rbt_insert(ionwm, vdb->timeline, addr, rfx_order_events,
				event) == 0)
		{
			psm_free(ionwm, addr);
			return 0;
		}
	}

	if (cxref->stopFire)
	{
		addr = psm_zalloc(ionwm, sizeof(IonEvent));
		if (addr == 0)
		{
			return 0;
		}

		event = (IonEvent *) psp(ionwm, addr);
		event->time = cxref->stopFire;
		event->type = IonStopFire;
		event->ref = cxaddr;
		if (sm_rbt_insert(ionwm, vdb->timeline, addr, rfx_order_events,
				event) == 0)
		{
			psm_free(ionwm, addr);
			return 0;
		}
	}

	if (cxref->purgeTime)
	{
		addr = psm_zalloc(ionwm, sizeof(IonEvent));
		if (addr == 0)
		{
			return 0;
		}

		event = (IonEvent *) psp(ionwm, addr);
		event->time = cxref->purgeTime;
		event->type = IonPurgeContact;
		event->ref = cxaddr;
		if (sm_rbt_insert(ionwm, vdb->timeline, addr, rfx_order_events,
				event) == 0)
		{
			psm_free(ionwm, addr);
			return 0;
		}
	}

	if (cxref->toTime > currentTime)	/*	Affects routes.	*/
	{
		vdb->lastEditTime = currentTime;
	}

	return cxaddr;
}

PsmAddress	rfx_insert_contact(time_t fromTime, time_t toTime,
			uvast fromNode, uvast toNode, unsigned int xmitRate)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();
	IonVdb 		*vdb = getIonVdb();
	IonCXref	arg;
	PsmAddress	cxelt;
	PsmAddress	nextElt;
	PsmAddress	cxaddr;
	IonCXref	*cxref;
	PsmAddress	prevElt;
	char		contactIdString[128];
	IonContact	contact;
	Object		iondbObj;
	IonDB		iondb;
	Object		obj;
	Object		elt;

	CHKZERO(fromTime);
	CHKZERO(toTime > fromTime);
	CHKZERO(fromNode);
	CHKZERO(toNode);
	CHKZERO(sdr_begin_xn(sdr));

	/*	Make sure contact doesn't overlap with any pre-existing
	 *	contacts.						*/

	memset((char *) &arg, 0, sizeof(IonCXref));
	arg.fromNode = fromNode;
	arg.toNode = toNode;
	arg.fromTime = fromTime;
	arg.toTime = toTime;
	arg.xmitRate = xmitRate;
	arg.routingObject = 0;
	cxelt = sm_rbt_search(ionwm, vdb->contactIndex, rfx_order_contacts,
			&arg, &nextElt);
	if (cxelt)	/*	Contact is in database already.		*/
	{
		cxaddr = sm_rbt_data(ionwm, cxelt);
		cxref = (IonCXref *) psp(ionwm, cxaddr);
		if (cxref->xmitRate == xmitRate)
		{
			sdr_exit_xn(sdr);
			return cxaddr;
		}

		isprintf(contactIdString, sizeof contactIdString,
				"at %lu, %lu->%lu", fromTime, fromNode, toNode);
		writeMemoNote("[?] Contact data rate not revised",
				contactIdString);
		sdr_exit_xn(sdr);
		return 0;
	}
	else	/*	Check for overlap, which is not allowed.	*/
	{
		if (nextElt)
		{
			prevElt = sm_rbt_prev(ionwm, nextElt);
			cxref = (IonCXref *)
				psp(ionwm, sm_rbt_data(ionwm, nextElt));
			if (fromNode == cxref->fromNode
			&& toNode == cxref->toNode
			&& toTime > cxref->fromTime)
			{
				writeMemoNote("[?] Overlapping contact",
						utoa(fromNode));
				sdr_exit_xn(sdr);
				return 0;
			}
		}
		else
		{
			prevElt = sm_rbt_last(ionwm, vdb->contactIndex);
		}

		if (prevElt)
		{
			cxref = (IonCXref *)
				psp(ionwm, sm_rbt_data(ionwm, prevElt));
			if (fromNode == cxref->fromNode
			&& toNode == cxref->toNode
			&& fromTime < cxref->toTime)
			{
				writeMemoNote("[?] Overlapping contact",
						utoa(fromNode));
				sdr_exit_xn(sdr);
				return 0;
			}
		}
	}

	/*	Contact isn't already in database; okay to add.		*/

	cxaddr = 0;
	contact.fromTime = fromTime;
	contact.toTime = toTime;
	contact.fromNode = fromNode;
	contact.toNode = toNode;
	contact.xmitRate = xmitRate;
	obj = sdr_malloc(sdr, sizeof(IonContact));
	if (obj)
	{
		sdr_write(sdr, obj, (char *) &contact, sizeof(IonContact));
		iondbObj = getIonDbObject();
		sdr_read(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
		elt = sdr_list_insert_last(sdr, iondb.contacts, obj);
		if (elt)
		{
			arg.contactElt = elt;
			cxaddr = insertCXref(&arg);
			if (cxaddr == 0)
			{
				sdr_cancel_xn(sdr);
			}
		}
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't insert contact.", NULL);
		return 0;
	}

	return cxaddr;
}

char	*rfx_print_contact(PsmAddress cxaddr, char *buffer)
{
	IonCXref	*contact;
	char		fromTimeBuffer[TIMESTAMPBUFSZ];
	char		toTimeBuffer[TIMESTAMPBUFSZ];

	CHKNULL(cxaddr);
	CHKNULL(buffer);
	contact = (IonCXref *) psp(getIonwm(), cxaddr);
	writeTimestampUTC(contact->fromTime, fromTimeBuffer);
	writeTimestampUTC(contact->toTime, toTimeBuffer);
	isprintf(buffer, RFX_NOTE_LEN, "From %20s to %20s the xmit rate from \
node %10lu to node %10lu is %10lu bytes/sec.", fromTimeBuffer, toTimeBuffer,
			contact->fromNode, contact->toNode, contact->xmitRate);
	return buffer;
}

static void	deleteContact(PsmAddress cxaddr)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();
	IonVdb 		*vdb = getIonVdb();
	time_t		currentTime = getUTCTime();
	IonCXref	*cxref;
	Object		obj;
	IonEvent	event;
	IonNeighbor	*neighbor;
	PsmAddress	nextElt;

	cxref = (IonCXref *) psp(ionwm, cxaddr);

	/*	Delete contact from non-volatile database.		*/

	obj = sdr_list_data(sdr, cxref->contactElt);
	sdr_free(sdr, obj);
	sdr_list_delete(sdr, cxref->contactElt, NULL, NULL);

	/*	Delete contact events from timeline.			*/

	event.ref = cxaddr;
	if (cxref->startXmit)
	{
		event.time = cxref->startXmit;
		event.type = IonStartXmit;
		sm_rbt_delete(ionwm, vdb->timeline, rfx_order_events,
				&event, rfx_erase_data, NULL);
	}

	if (cxref->stopXmit)
	{
		event.time = cxref->stopXmit;
		event.type = IonStopXmit;
		sm_rbt_delete(ionwm, vdb->timeline, rfx_order_events,
				&event, rfx_erase_data, NULL);
	}

	if (cxref->startFire)
	{
		event.time = cxref->startFire;
		event.type = IonStartFire;
		sm_rbt_delete(ionwm, vdb->timeline, rfx_order_events,
				&event, rfx_erase_data, NULL);
	}

	if (cxref->stopFire)
	{
		event.time = cxref->stopFire;
		event.type = IonStopFire;
		sm_rbt_delete(ionwm, vdb->timeline, rfx_order_events,
				&event, rfx_erase_data, NULL);
	}

	if (cxref->startRecv)
	{
		event.time = cxref->startRecv;
		event.type = IonStartRecv;
		sm_rbt_delete(ionwm, vdb->timeline, rfx_order_events,
				&event, rfx_erase_data, NULL);
	}

	if (cxref->stopRecv)
	{
		event.time = cxref->stopRecv;
		event.type = IonStopRecv;
		sm_rbt_delete(ionwm, vdb->timeline, rfx_order_events,
				&event, rfx_erase_data, NULL);
	}

	if (cxref->purgeTime)
	{
		event.time = cxref->purgeTime;
		event.type = IonPurgeContact;
		sm_rbt_delete(ionwm, vdb->timeline, rfx_order_events,
				&event, rfx_erase_data, NULL);
	}

	/*	Apply to current state as necessary.			*/

	if (currentTime >= cxref->startXmit && currentTime <= cxref->stopXmit)
	{
		neighbor = findNeighbor(vdb, cxref->toNode, &nextElt);
		if (neighbor)
		{
			neighbor->xmitRate = 0;
		}
	}

	if (currentTime >= cxref->startFire && currentTime <= cxref->stopFire)
	{
		neighbor = findNeighbor(vdb, cxref->fromNode, &nextElt);
		if (neighbor)
		{
			neighbor->fireRate = 0;
		}
	}

	if (currentTime >= cxref->startRecv && currentTime <= cxref->stopRecv)
	{
		neighbor = findNeighbor(vdb, cxref->fromNode, &nextElt);
		if (neighbor)
		{
			neighbor->recvRate = 0;
		}
	}

	/*	Delete contact from index.				*/

	if (cxref->toTime > currentTime)	/*	Affects routes.	*/
	{
		vdb->lastEditTime = currentTime;
	}

	sm_rbt_delete(ionwm, vdb->contactIndex, rfx_order_contacts, cxref,
			rfx_erase_data, NULL);
}

int	rfx_remove_contact(time_t fromTime, uvast fromNode, uvast toNode)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();
	IonVdb 		*vdb = getIonVdb();
	IonCXref	arg;
	PsmAddress	cxelt;
	PsmAddress	nextElt;
	PsmAddress	cxaddr;
	IonCXref	*cxref;

	/*	Note: when the fromTime passed to ionadmin is '*'
	 *	a fromTime of zero is passed to rfx_remove_contact,
	 *	where it is interpreted as "all contacts between
	 *	these two nodes".					*/

	memset((char *) &arg, 0, sizeof(IonCXref));
	arg.fromNode = fromNode;
	arg.toNode = toNode;
	CHKERR(sdr_begin_xn(sdr));
	if (fromTime)		/*	Not a wild-card deletion.	*/
	{
		arg.fromTime = fromTime;
		cxelt = sm_rbt_search(ionwm, vdb->contactIndex,
				rfx_order_contacts, &arg, &nextElt);
		if (cxelt)	/*	Found it.			*/
		{
			cxaddr = sm_rbt_data(ionwm, cxelt);
			deleteContact(cxaddr);
		}
	}
	else		/*	Wild-card deletion, start at time zero.	*/
	{
		while (1)
		{
			/*	Get first remaining contact for this
			 *	to/from node pair.			*/

			oK(sm_rbt_search(ionwm, vdb->contactIndex,
					rfx_order_contacts, &arg, &cxelt));
			if (cxelt == 0)
			{
				break;	/*	No more contacts.	*/
			}

			cxaddr = sm_rbt_data(ionwm, cxelt);
			cxref = (IonCXref *) psp(ionwm, cxaddr);
			if (cxref->fromNode > fromNode
			|| cxref->toNode > toNode)
			{
				break;	/*	No more matches.	*/
			}

			deleteContact(cxaddr);
		}
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't remove contact(s).", NULL);
		return -1;
	}

	return 0;
}

/*	*	RFX range list management functions	*	*	*/

static int	insertRXref(IonRXref *rxref)
{
	PsmPartition	ionwm = getIonwm();
	IonVdb		*vdb = getIonVdb();
	IonNode		*node;
	PsmAddress	nextElt;
	IonRXref	arg;
	PsmAddress	rxaddr;
	PsmAddress	rxelt;
	PsmAddress	addr;
	IonEvent	*event;
	PsmAddress	rxaddr2;
	IonRXref	*rxref2;
	time_t		currentTime = getUTCTime();

	/*	Load the affected nodes.				*/

	node = findNode(vdb, rxref->toNode, &nextElt);
	if (node == NULL)
	{
		node = addNode(vdb, rxref->toNode);
		if (node == NULL)
		{
			return 0;
		}
	}

	node = findNode(vdb, rxref->fromNode, &nextElt);
	if (node == NULL)
	{
		node = addNode(vdb, rxref->fromNode);
		if (node == NULL)
		{
			return 0;
		}
	}

	/*	Construct the range index entry.			*/

	rxaddr = psm_zalloc(ionwm, sizeof(IonRXref));
	if (rxaddr == 0)
	{
		return 0;
	}

	memcpy((char *) psp(ionwm, rxaddr), (char *) rxref, sizeof(IonRXref));
	rxelt = sm_rbt_insert(ionwm, vdb->rangeIndex, rxaddr, rfx_order_ranges,
			rxref);
	if (rxelt == 0)
	{
		psm_free(ionwm, rxaddr);
		return 0;
	}

	/*	Insert relevant asserted timeline events.		*/

	addr = psm_zalloc(ionwm, sizeof(IonEvent));
	if (addr == 0)
	{
		return 0;
	}

	event = (IonEvent *) psp(ionwm, addr);
	event->time = rxref->fromTime;
	event->type = IonStartAssertedRange;
	event->ref = rxaddr;
	if (sm_rbt_insert(ionwm, vdb->timeline, addr, rfx_order_events, event)
			== 0)
	{
		return 0;
	}

	addr = psm_zalloc(ionwm, sizeof(IonEvent));
	if (addr == 0)
	{
		return 0;
	}

	event = (IonEvent *) psp(ionwm, addr);
	event->time = rxref->toTime;
	event->type = IonStopAssertedRange;
	event->ref = rxaddr;
	if (sm_rbt_insert(ionwm, vdb->timeline, addr, rfx_order_events, event)
			== 0)
	{
		return 0;
	}

	if (rxref->toTime > currentTime)	/*	Affects routes.	*/
	{
		vdb->lastEditTime = currentTime;
	}

	if (rxref->fromNode > rxref->toNode)
	{
		/*	This is a non-canonical range assertion,
		 *	indicating an override of the normal
		 *	symmetry in the owlt between nodes.  The
		 *	reverse range assertion does *not* hold.	*/

		return rxaddr;
	}

	/*	This is a canonical range assertion, so we may need
	 *	to insert the imputed (symmetrical) reverse range
	 *	assertion as well.  Is the reverse range already
	 *	asserted?						*/

	arg.fromNode = rxref->toNode;
	arg.toNode = rxref->fromNode;
	arg.fromTime = rxref->fromTime;
	rxelt = sm_rbt_search(ionwm, vdb->rangeIndex, rfx_order_ranges,
			&arg, &nextElt);
	if (rxelt)	/*	Asserted range exists; we're done.	*/
	{
		return rxaddr;
	}

	/*	Reverse range is not asserted, so it must be imputed.
	 *
	 *	First, load index entry for the imputed range.		*/

	rxaddr2 = psm_zalloc(ionwm, sizeof(IonRXref));
	if (rxaddr2 == 0)
	{
		return 0;
	}

	rxref2 = (IonRXref *) psp(ionwm, rxaddr2);
	rxref2->fromNode = rxref->toNode;	/*	Reversed.	*/
	rxref2->toNode = rxref->fromNode;	/*	Reversed.	*/
	rxref2->fromTime = rxref->fromTime;
	rxref2->toTime = rxref->toTime;
	rxref2->owlt = rxref->owlt;
	rxref2->rangeElt = 0;		/*	Indicates "imputed".	*/
	memcpy((char *) psp(ionwm, rxaddr2), (char *) rxref2, sizeof(IonRXref));
	rxelt = sm_rbt_insert(ionwm, vdb->rangeIndex, rxaddr2, rfx_order_ranges,
			rxref2);
	if (rxelt == 0)
	{
		psm_free(ionwm, rxaddr2);
		return 0;
	}

	/*	Then insert relevant imputed timeline events.		*/

	addr = psm_zalloc(ionwm, sizeof(IonEvent));
	if (addr == 0)
	{
		return 0;
	}

	event = (IonEvent *) psp(ionwm, addr);
	event->time = rxref->fromTime;
	event->type = IonStartImputedRange;
	event->ref = rxaddr2;
	if (sm_rbt_insert(ionwm, vdb->timeline, addr, rfx_order_events, event)
			== 0)
	{
		psm_free(ionwm, addr);
		return 0;
	}
	
	addr = psm_zalloc(ionwm, sizeof(IonEvent));
	if (addr == 0)
	{
		return 0;
	}

	event = (IonEvent *) psp(ionwm, addr);
	event->time = rxref->toTime;
	event->type = IonStopImputedRange;
	event->ref = rxaddr2;
	if (sm_rbt_insert(ionwm, vdb->timeline, addr, rfx_order_events, event)
			== 0)
	{
		psm_free(ionwm, addr);
		return 0;
	}

	return rxaddr;
}

Object	rfx_insert_range(time_t fromTime, time_t toTime, uvast fromNode,
		uvast toNode, unsigned int owlt)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();
	IonVdb		*vdb = getIonVdb();
	IonRXref	arg1;
	PsmAddress	rxelt;
	PsmAddress	nextElt;
	PsmAddress	rxaddr;
	IonRXref	*rxref;
	IonEvent	arg2;
	PsmAddress	prevElt;
	char		rangeIdString[128];
	IonRange	range;
	Object		iondbObj;
	IonDB		iondb;
	Object		obj;
	Object		elt;

	/*	Note that ranges are normally assumed to be symmetrical,
	 *	i.e., the signal propagation time from B to A is normally
	 *	assumed to be the same as the signal propagation time
	 *	from A to B.  For this reason, normally only the A->B
	 *	range (where A is a node number that is less than node
	 *	number B) need be entered; when ranges are applied to
	 *	the IonNeighbor objects in the ION database, the A->B
	 *	range is stored as the OWLT for transmissions from A to
	 *	B and also as the OWLT for transmissions from B to A.
	 *
	 *	However, it is possible to insert asymmetric ranges, as
	 *	would apply when the forward and return traffic between
	 *	some pair of nodes travels by different transmission
	 *	paths that introduce different latencies.  When this is
	 *	the case, both the A->B and B->A ranges must be entered.
	 *	The A->B range is initially processed as a symmetric
	 *	range as described above, but when the B->A range is
	 *	subsequently noted it overrides the default OWLT for
	 *	transmissions from B to A.				*/

	CHKZERO(fromTime);
	CHKZERO(toTime > fromTime);
	CHKZERO(fromNode);
	CHKZERO(toNode);
	CHKZERO(sdr_begin_xn(sdr));

	/*	Make sure range doesn't overlap with any pre-existing
	 *	ranges.							*/

	memset((char *) &arg1, 0, sizeof(IonRXref));
	arg1.fromNode = fromNode;
	arg1.toNode = toNode;
	arg1.fromTime = fromTime;
	arg1.toTime = toTime;
	arg1.owlt = owlt;
	rxelt = sm_rbt_search(ionwm, vdb->rangeIndex, rfx_order_ranges,
			&arg1, &nextElt);
	if (rxelt)	/*	Range is in database already.		*/
	{
		rxaddr = sm_rbt_data(ionwm, rxelt);
		rxref = (IonRXref *) psp(ionwm, rxaddr);
		if (rxref->rangeElt == 0)	/*	Imputed.	*/
		{
			/*	The existing range for the same nodes
			 *	and time is merely an imputed range,
			 *	which is being overridden by a non-
			 *	canonical range assertion indicating
			 *	an override of the normal symmetry in
			 *	the owlt between nodes.  Must delete
			 *	that imputed range, together with the
			 *	associated events, after which there
			 *	is no duplication.			*/

			sm_rbt_delete(ionwm, vdb->rangeIndex, rfx_order_ranges,
					&arg1, rfx_erase_data, NULL);
			arg2.ref = rxaddr;
			arg2.time = rxref->fromTime;
			arg2.type = IonStartImputedRange;
			sm_rbt_delete(ionwm, vdb->timeline, rfx_order_events,
					&arg2, rfx_erase_data, NULL);
			arg2.time = rxref->toTime;
			arg2.type = IonStopImputedRange;
			sm_rbt_delete(ionwm, vdb->timeline, rfx_order_events,
					&arg2, rfx_erase_data, NULL);
		}
		else	/*	Overriding an asserted range.		*/
		{
			/*	This is an attempt to replace an
			 *	existing asserted range with another
			 *	asserted range, which is prohibited.	*/

			if (rxref->owlt == owlt)
			{
				sdr_exit_xn(sdr);
				return rxaddr;	/*	Idempotent.	*/
			}

			isprintf(rangeIdString, sizeof rangeIdString,
					"from %lu, %lu->%lu", fromTime,
					fromNode, toNode);
			writeMemoNote("[?] Range OWLT not revised",
					rangeIdString);
			sdr_exit_xn(sdr);
			return 0;
		}
	}

	/*	Check for overlap, which is not allowed.		*/

	if (nextElt)
	{
		prevElt = sm_rbt_prev(ionwm, nextElt);
		rxref = (IonRXref *)
			psp(ionwm, sm_rbt_data(ionwm, nextElt));
		if (fromNode == rxref->fromNode
		&& toNode == rxref->toNode
		&& toTime > rxref->fromTime)
		{
			writeMemoNote("[?] Overlapping range",
					utoa(fromNode));
			sdr_exit_xn(sdr);
			return 0;
		}
	}
	else
	{
		prevElt = sm_rbt_last(ionwm, vdb->rangeIndex);
	}

	if (prevElt)
	{
		rxref = (IonRXref *)
			psp(ionwm, sm_rbt_data(ionwm, prevElt));
		if (fromNode == rxref->fromNode
		&& toNode == rxref->toNode
		&& fromTime < rxref->toTime)
		{
			writeMemoNote("[?] Overlapping range",
					utoa(fromNode));
			sdr_exit_xn(sdr);
			return 0;
		}
	}

	/*	Range isn't already in database; okay to add.		*/

	rxaddr = 0;
	range.fromTime = fromTime;
	range.toTime = toTime;
	range.fromNode = fromNode;
	range.toNode = toNode;
	range.owlt = owlt;
	obj = sdr_malloc(sdr, sizeof(IonRange));
	if (obj)
	{
		sdr_write(sdr, obj, (char *) &range, sizeof(IonRange));
		iondbObj = getIonDbObject();
		sdr_read(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
		elt = sdr_list_insert_last(sdr, iondb.ranges, obj);
		if (elt)
		{
			arg1.rangeElt = elt;
			rxaddr = insertRXref(&arg1);
			if (rxaddr == 0)
			{
				sdr_cancel_xn(sdr);
			}
		}
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't insert range.", NULL);
		return 0;
	}

	return rxaddr;
}

char	*rfx_print_range(PsmAddress rxaddr, char *buffer)
{
	IonRXref	*range;
	char		fromTimeBuffer[TIMESTAMPBUFSZ];
	char		toTimeBuffer[TIMESTAMPBUFSZ];

	CHKNULL(rxaddr);
	CHKNULL(buffer);
	range = (IonRXref *) psp(getIonwm(), rxaddr);
	writeTimestampUTC(range->fromTime, fromTimeBuffer);
	writeTimestampUTC(range->toTime, toTimeBuffer);
	isprintf(buffer, RFX_NOTE_LEN, "From %20s to %20s the OWLT from node \
%10lu to node %10lu is %10u seconds.", fromTimeBuffer, toTimeBuffer,
			range->fromNode, range->toNode, range->owlt);
	return buffer;
}

static void	deleteRange(PsmAddress rxaddr, int conditional)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();
	IonVdb 		*vdb = getIonVdb();
	time_t		currentTime = getUTCTime();
	IonRXref	*rxref;
	Object		obj;
	IonEvent	event;
	IonNeighbor	*neighbor;
	PsmAddress	nextElt;

	rxref = (IonRXref *) psp(ionwm, rxaddr);

	/*	Delete range from non-volatile database.		*/

	if (rxref->rangeElt)		/*	An asserted range.	*/
	{
		if (conditional)	/*	Delete only if imputed.	*/
		{
			return;		/*	Retain asserted range.	*/
		}

		/*	Unconditional deletion; remove range from DB.	*/

		obj = sdr_list_data(sdr, rxref->rangeElt);
		sdr_free(sdr, obj);
		sdr_list_delete(sdr, rxref->rangeElt, NULL, NULL);
	}

	/*	Delete range events from timeline.			*/

	event.ref = rxaddr;
	event.time = rxref->fromTime;
	if (rxref->rangeElt)
	{
		event.type = IonStartAssertedRange;
	}
	else
	{
		event.type = IonStartImputedRange;
	}

	sm_rbt_delete(ionwm, vdb->timeline, rfx_order_events,
			&event, rfx_erase_data, NULL);
	event.time = rxref->toTime;
	if (rxref->rangeElt)
	{
		event.type = IonStopAssertedRange;
	}
	else
	{
		event.type = IonStopImputedRange;
	}

	sm_rbt_delete(ionwm, vdb->timeline, rfx_order_events,
			&event, rfx_erase_data, NULL);

	/*	Apply to current state as necessary.			*/

	if (currentTime >= rxref->fromTime && currentTime <= rxref->toTime)
	{
		if (rxref->fromNode == getOwnNodeNbr())
		{
			neighbor = findNeighbor(vdb, rxref->toNode, &nextElt);
			if (neighbor)
			{
				neighbor->owltOutbound = 0;
			}
		}

		if (rxref->toNode == getOwnNodeNbr())
		{
			neighbor = findNeighbor(vdb, rxref->fromNode, &nextElt);
			if (neighbor)
			{
				neighbor->owltInbound = 0;
			}
		}
	}

	/*	Delete range from index.				*/

	if (rxref->toTime > currentTime)	/*	Affects routes.	*/
	{
		vdb->lastEditTime = currentTime;
	}

	sm_rbt_delete(ionwm, vdb->rangeIndex, rfx_order_ranges, rxref,
			rfx_erase_data, NULL);
}

int	rfx_remove_range(time_t fromTime, uvast fromNode, uvast toNode)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();
	IonVdb 		*vdb = getIonVdb();
	IonRXref	arg;
	PsmAddress	rxelt;
	PsmAddress	nextElt;
	PsmAddress	rxaddr;
	IonRXref	*rxref;

	/*	Note: when the fromTime passed to ionadmin is '*'
	 *	a fromTime of zero is passed to rfx_remove_range,
	 *	where it is interpreted as "all ranges between
	 *	these two nodes".					*/

	memset((char *) &arg, 0, sizeof(IonRXref));
	arg.fromNode = fromNode;
	arg.toNode = toNode;
	CHKERR(sdr_begin_xn(sdr));
	if (fromTime)		/*	Not a wild-card deletion.	*/
	{
		arg.fromTime = fromTime;
		rxelt = sm_rbt_search(ionwm, vdb->rangeIndex, rfx_order_ranges,
				&arg, &nextElt);
		if (rxelt)	/*	Found it.			*/
		{
			rxaddr = sm_rbt_data(ionwm, rxelt);
			deleteRange(rxaddr, 0);
		}
	}
	else		/*	Wild-card deletion, start at time zero.	*/
	{
		while (1)
		{
			/*	Get first remaining range for this
			 *	to/from node pair.			*/

			oK(sm_rbt_search(ionwm, vdb->rangeIndex,
					rfx_order_ranges, &arg, &rxelt));
			if (rxelt == 0)
			{
				break;	/*	No more ranges.		*/
			}

			rxaddr = sm_rbt_data(ionwm, rxelt);
			rxref = (IonRXref *) psp(ionwm, rxaddr);
			if (rxref->fromNode > arg.fromNode
			|| rxref->toNode > arg.toNode)
			{
				break;	/*	No more matches.	*/
			}

			deleteRange(rxaddr, 0);
		}
	}

	if (fromNode > toNode)
	{
		/*	This is either an imputed range or else
		 *	a non-canonical range assertion,
		 *	indicating an override of the normal
		 *	symmetry in the owlt between nodes.  The
		 *	reverse range assertion does *not* hold.	*/

		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Can't remove range(s).", NULL);
			return -1;
		}

		return 0;
	}

	/*	Canonical range assertion(s); delete corresponding
	 *	imputed range(s) as well.				*/

	arg.fromNode = toNode;	/*	Reverse direction.		*/
	arg.toNode = fromNode;	/*	Reverse direction.		*/
	if (fromTime)		/*	Not a wild-card deletion.	*/
	{
		arg.fromTime = fromTime;
		rxelt = sm_rbt_search(ionwm, vdb->rangeIndex, rfx_order_ranges,
				&arg, &nextElt);
		if (rxelt)	/*	Found it.			*/
		{
			rxaddr = sm_rbt_data(ionwm, rxelt);
			deleteRange(rxaddr, 1);
		}
	}
	else		/*	Wild-card deletion, start at time zero.	*/
	{
		while (1)
		{
			/*	Get first remaining range for this
			 *	to/from node pair.			*/

			oK(sm_rbt_search(ionwm, vdb->rangeIndex,
					rfx_order_ranges, &arg, &rxelt));
			if (rxelt == 0)
			{
				break;	/*	No more ranges.		*/
			}

			rxaddr = sm_rbt_data(ionwm, rxelt);
			rxref = (IonRXref *) psp(ionwm, rxaddr);
			if (rxref->fromNode > arg.fromNode
			|| rxref->toNode > arg.toNode)
			{
				break;	/*	No more matches.	*/
			}

			deleteRange(rxaddr, 1);
		}
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't remove range(s).", NULL);
		return -1;
	}

	return 0;
}

/*	*	*	RFX control functions	*	*	*	*/

static int	loadRange(Object elt)
{
	Sdr		sdr = getIonsdr();
	Object		obj;
	IonRange	range;
	IonRXref	rxref;

	obj = sdr_list_data(sdr, elt);
	sdr_read(sdr, (char *) &range, obj, sizeof(IonRange));

	/*	Load range list entry.					*/

	rxref.fromNode = range.fromNode;
	rxref.toNode = range.toNode;
	rxref.fromTime = range.fromTime;
	rxref.toTime = range.toTime;
	rxref.owlt = range.owlt;
	rxref.rangeElt = elt;
	if (insertRXref(&rxref) < 0)
	{
		return -1;
	}

	return 0;
}

static int	loadContact(Object elt)
{
	Sdr		sdr = getIonsdr();
	Object		obj;
	IonContact	contact;
	IonCXref	cxref;

	obj = sdr_list_data(sdr, elt);
	sdr_read(sdr, (char *) &contact, obj, sizeof(IonContact));

	/*	Load contact index entry.				*/

	cxref.fromNode = contact.fromNode;
	cxref.toNode = contact.toNode;
	cxref.fromTime = contact.fromTime;
	cxref.toTime = contact.toTime;
	cxref.xmitRate = contact.xmitRate;
	cxref.contactElt = elt;
	cxref.routingObject = 0;
	if (insertCXref(&cxref) == 0)
	{
		return -1;
	}

	return 0;
}

int	rfx_start()
{
	PsmPartition	ionwm = getIonwm();
	Sdr		sdr = getIonsdr();
	IonVdb		*vdb = getIonVdb();
	Object		iondbObj;
	IonDB		iondb;
	Object		elt;

	iondbObj = getIonDbObject();
	CHKERR(sdr_begin_xn(sdr));	/*	To lock memory.		*/
	sdr_read(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));

	/* Destroy and re-create volatile contact and range databases.
	 * This prevents contact/range duplication as a result of adds
	 * before starting ION. */
	sm_rbt_destroy(ionwm, vdb->contactIndex, rfx_erase_data, NULL);
	sm_rbt_destroy(ionwm, vdb->rangeIndex, rfx_erase_data, NULL);
	vdb->contactIndex = sm_rbt_create(ionwm);
	vdb->rangeIndex = sm_rbt_create(ionwm);

	/*	Load range index for all asserted ranges.  In so
	 *	doing, load the nodes for which ranges are known
	 *	and load events for all predicted changes in range.	*/

	for (elt = sdr_list_first(sdr, iondb.ranges); elt;
			elt = sdr_list_next(sdr, elt))
	{
		if (loadRange(elt) < 0)
		{
			putErrmsg("Can't load range.", NULL);
			sdr_exit_xn(sdr);
			return -1;
		}
	}

	/*	Load contact index for all contacts.  In so doing,
	 *	load the nodes for which contacts are planned (as
	 *	necessary) and load events for all planned changes
	 *	in data rate affecting the local node.			*/

	iondbObj = getIonDbObject();
	sdr_read(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
	for (elt = sdr_list_first(sdr, iondb.contacts); elt;
			elt = sdr_list_next(sdr, elt))
	{
		if (loadContact(elt) < 0)
		{
			putErrmsg("Can't load contact.", NULL);
			sdr_exit_xn(sdr);
			return -1;
		}
	}

	/*	Start the rfx clock if necessary.			*/

	if (vdb->clockPid == ERROR || sm_TaskExists(vdb->clockPid) == 0)
	{
		vdb->clockPid = pseudoshell("rfxclock");
	}

	sdr_exit_xn(sdr);	/*	Unlock memory.			*/
	return 0;
}

void	rfx_stop()
{
	PsmPartition	ionwm = getIonwm();
	IonVdb		*vdb = getIonVdb();

	/*	Clear ZCO availability information.			*/

	sm_SemEnd(vdb->zcoSemaphore);
	vdb->zcoClaimants = 0;
	vdb->zcoClaims = 0;

	/*	Stop the rfx clock if necessary.			*/

	if (vdb->clockPid != ERROR)
	{
		sm_TaskKill(vdb->clockPid, SIGTERM);
		while (sm_TaskExists(vdb->clockPid))
		{
			microsnooze(100000);
		}

		vdb->clockPid = ERROR;
	}

	/*	Wipe out all red-black trees involved in routing,
	 *	for reconstruction on restart.				*/

	sm_rbt_destroy(ionwm, vdb->contactIndex, rfx_erase_data, NULL);
	sm_rbt_destroy(ionwm, vdb->rangeIndex, rfx_erase_data, NULL);
	sm_rbt_destroy(ionwm, vdb->timeline, rfx_erase_data, NULL);
	vdb->contactIndex = sm_rbt_create(ionwm);
	vdb->rangeIndex = sm_rbt_create(ionwm);
	vdb->timeline = sm_rbt_create(ionwm);
}
