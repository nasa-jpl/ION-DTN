/*

	rfx.c:	API for managing ION's time-ordered lists of
		contacts and ranges.  Also manages timeline
		of ION events, including alarm timeouts.

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
	if (contact->regionNbr < argContact->regionNbr)
	{
		return -1;
	}

	if (contact->regionNbr > argContact->regionNbr)
	{
		return 1;
	}

	/*	Matching region.					*/

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
	if (nodeData)
	{
		psm_free(partition, nodeData);
	}
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

IonNeighbor	*getNeighbor(IonVdb *vdb, uvast nodeNbr)
{
	IonNeighbor	*neighbor;
	PsmAddress	next;

	neighbor = findNeighbor(vdb, nodeNbr, &next);
	if (neighbor == NULL)
	{
		neighbor = addNeighbor(vdb, nodeNbr);
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

	node->embargoes = sm_list_create(ionwm);
	return node;
}

int	addEmbargo(IonNode *node, uvast neighborNodeNbr)
{
	PsmPartition	ionwm = getIonwm();
	PsmAddress	nextElt;
	PsmAddress	elt;
	Embargo		*embargo;
	PsmAddress	addr;

	/*	Find insertion point in embargoes list.			*/

	CHKERR(node);
	nextElt = 0;
	for (elt = sm_list_first(ionwm, node->embargoes); elt;
			elt = sm_list_next(ionwm, elt))
	{
		embargo = (Embargo *) psp(ionwm, sm_list_data(ionwm, elt));
		CHKERR(embargo);
		if (embargo->nodeNbr < neighborNodeNbr)
		{
			continue;
		}

		if (embargo->nodeNbr > neighborNodeNbr)
		{
			nextElt = elt;
			break;	/*	Have found insertion point.	*/
		}

		return 0;	/*	Embargo has already been added.	*/
	}

	addr = psm_zalloc(ionwm, sizeof(Embargo));
	if (addr == 0)
	{
		putErrmsg("Can't add embargo.", NULL);
		return -1;
	}

	if (nextElt)
	{
		elt = sm_list_insert_before(ionwm, nextElt, addr);
	}
	else
	{
		elt = sm_list_insert_last(ionwm, node->embargoes, addr);
	}

	if (elt == 0)
	{
		psm_free(ionwm, addr);
		putErrmsg("Can't add embargo.", NULL);
		return -1;
	}

	embargo = (Embargo *) psp(ionwm, addr);
	CHKERR(embargo);
	embargo->nodeNbr = neighborNodeNbr;
	embargo->probeIsDue = 0;
	postProbeEvent(node, embargo);	/*	Initial probe event.	*/
	return 0;
}

void	removeEmbargo(IonNode *node, uvast neighborNodeNbr)
{
	PsmPartition	ionwm = getIonwm();
	PsmAddress	elt;
	PsmAddress	addr;
	Embargo		*embargo;

	CHKVOID(node);
	for (elt = sm_list_first(ionwm, node->embargoes); elt;
			elt = sm_list_next(ionwm, elt))
	{
		addr = sm_list_data(ionwm, elt);
		embargo = (Embargo *) psp(ionwm, addr);
		CHKVOID(embargo);
		if (embargo->nodeNbr < neighborNodeNbr)
		{
			continue;
		}

		if (embargo->nodeNbr > neighborNodeNbr)
		{
			return;	/*	Embargo not found.		*/
		}

		break;		/*	Found the embargo to remove.	*/
	}

	if (elt == 0)
	{
		return;		/*	Embargo not found.		*/
	}

	oK(sm_list_delete(ionwm, elt, NULL, NULL));
	psm_free(ionwm, addr);
}

PsmAddress	postProbeEvent(IonNode *node, Embargo *embargo)
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
	CHKZERO(embargo);
	addr = psm_zalloc(ionwm, sizeof(IonProbe));
	if (addr == 0)
	{
		putErrmsg("Can't create probe event.", NULL);
		return 0;
	}

	probe = (IonProbe *) psp(ionwm, addr);
	CHKZERO(probe);
	probe->time = getCtime();
	probe->destNodeNbr = node->nodeNbr;
	probe->neighborNodeNbr = embargo->nodeNbr;

	/*	Schedule next probe of this embargoed neighbor for the
	 *	time that is the current time plus 2x the round-trip
	 *	light time from the local node to the neighbor (but
	 *	at least 6 seconds).					*/
	 
	ionvdb = getIonVdb();
	neighbor = findNeighbor(ionvdb, embargo->nodeNbr, &nextElt);
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

static void	postCpsNotice(uint32_t regionNbr, time_t fromTime,
			time_t toTime, uvast fromNode, uvast toNode,
			size_t xmitRate, float confidence)
{
	Sdr		sdr = getIonsdr();
	Object		iondbObj;
	IonDB		iondb;
	CpsNotice	notice;
	Object		noticeObj;

	/*	If regionNbr is 0
	 *		This is a range notice
	 *	Else
	 *		This is a contact notice
	 *
	 * 	*** For a contact notice:
	 *
	 *	If fromTime is -1 (a registration contact)
	 *		If toTime is 0
	 *			Unregister fromNode in region (regionNbr)
	 *		Else (toTime is -1)
	 *			Register fromNode in region (regionNbr)
	 *	Else (scheduled contact)
	 *		If toTime is 0
	 *			Delete contact (per fromTime and node nbrs)
	 *		Else (not a deletion)
	 *			If confidence is >= 2.0
	 *				Revise contact
	 *			Else
	 *				Add this contact
	 *
	 *	*** For a range notice:
	 *
	 *	If toTime is 0
	 *		Delete range (per fromTime and node nbrs)
	 *	Else (not a deletion)
	 *		Add this range					*/

	iondbObj = getIonDbObject();
	CHKVOID(iondbObj);
	sdr_read(getIonsdr(), (char *) &iondb, iondbObj, sizeof(IonDB));
	notice.regionNbr = regionNbr;
	notice.fromTime = fromTime;
	notice.toTime = toTime;
	notice.fromNode = fromNode;
	notice.toNode = toNode;
	notice.magnitude = xmitRate;
	notice.confidence = confidence;
	noticeObj = sdr_malloc(sdr, sizeof(CpsNotice));
	if (noticeObj)
	{
		sdr_write(sdr, noticeObj, (char *) &notice, sizeof(CpsNotice));
		oK(sdr_list_insert_last(sdr, iondb.cpsNotices, noticeObj));
	}
}

static PsmAddress	insertCXref(IonCXref *cxref)
{
	PsmPartition	ionwm = getIonwm();
	IonVdb 		*vdb = getIonVdb();
	IonCXref	arg;
	IonNode		*node;
	PsmAddress	nextElt;
	PsmAddress	cxaddr;
	Object		iondbObj;
	IonDB		iondb;
	PsmAddress	cxelt;
	PsmAddress	addr;
	IonEvent	*event;
	time_t		currentTime = getCtime();

	/*	If the CXref already exists, just return its address.	*/

	memset((char *) &arg, 0, sizeof(IonCXref));
	arg.regionNbr = cxref->regionNbr;
	arg.fromNode = cxref->fromNode;
	arg.toNode = cxref->toNode;
	arg.fromTime = cxref->fromTime;
	cxelt = sm_rbt_search(ionwm, vdb->contactIndex, rfx_order_contacts,
			&arg, &nextElt);
	if (cxelt)
	{
		cxaddr = sm_rbt_data(ionwm, cxelt);
		return cxaddr;
	}

	/*	New CXref, so load the affected nodes.			*/

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
	if (cxref->type == CtScheduled)
	{
		if (cxref->fromNode == getOwnNodeNbr())
		{
			/*	Be a little slow to start transmission,
			 *	and a little quick to stop, to ensure
			 *	that segments arrive only when neighbor
			 *	is expecting them.			*/

			cxref->startXmit = cxref->fromTime
					+ iondb.maxClockError;
			cxref->stopXmit = cxref->toTime - iondb.maxClockError;
		}

		if (cxref->toNode == getOwnNodeNbr())
		{
			/*	Be a little slow to resume timers,
			 *	and a little quick to suspend them,
			 *	to minimize the chance of premature
			 *	timeout.				*/

			cxref->startFire = cxref->fromTime
			      	 	+ iondb.maxClockError;
			cxref->stopFire = cxref->toTime - iondb.maxClockError;
		}
		else	/*	Not a transmission to the local node.	*/
		{
			cxref->purgeTime = cxref->toTime;
		}
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

	if (cxref->type != CtRegistration && cxref->toTime > currentTime)
	{
		/*	Affects routes.					*/

		getCurrentTime(&(vdb->lastEditTime));
	}

	return cxaddr;
}

static void	insertContact(int regionIdx, IonDB *iondb, Object iondbObj,
			time_t fromTime, time_t toTime, uvast fromNode,
			uvast toNode, size_t xmitRate, float confidence,
			ContactType contactType, PsmAddress *cxaddr)
{
	Sdr		sdr = getIonsdr();
	uint32_t	regionNbr = iondb->regions[regionIdx].regionNbr;
	IonContact	contact;
	double		volume;
	Object		obj;
	Object		elt;
	IonCXref	newCx;

	contact.fromTime = fromTime;
	contact.toTime = toTime;
	contact.fromNode = fromNode;
	contact.toNode = toNode;
	contact.xmitRate = xmitRate;
	contact.confidence = confidence;
	contact.type = contactType;
	volume = xmitRate * (toTime - fromTime);
	contact.mtv[0] = volume;		/*	Bulk.		*/
	contact.mtv[1] = volume;		/*	Standard.	*/
	contact.mtv[2] = volume;		/*	Expedited.	*/
	obj = sdr_malloc(sdr, sizeof(IonContact));
	if (obj == 0)
	{
		return;
	}

	sdr_write(sdr, obj, (char *) &contact, sizeof(IonContact));
	elt = sdr_list_insert_last(sdr,
			iondb->regions[regionIdx].contacts, obj);
	if (elt == 0)
	{
		return;
	}

	memset((char *) &newCx, 0, sizeof(IonCXref));
	newCx.regionNbr = regionNbr;
	newCx.fromTime = fromTime;
	newCx.toTime = toTime;
	newCx.fromNode = fromNode;
	newCx.toNode = toNode;
	newCx.xmitRate = xmitRate;
	newCx.confidence = confidence;
	newCx.type = contactType;
	newCx.contactElt = elt;
	newCx.routingObject = 0;
	*cxaddr = insertCXref(&newCx);
}

static void	deleteContact(PsmAddress cxaddr)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();
	IonVdb 		*vdb = getIonVdb();
	time_t		currentTime = getCtime();
	IonCXref	*cxref;
	Object		obj;
	IonEvent	event;
	IonNeighbor	*neighbor;
	PsmAddress	nextElt;
	PsmAddress	elt;
	PsmAddress	citation;

	cxref = (IonCXref *) psp(ionwm, cxaddr);

	/*	Delete contact from non-volatile database.		*/

	obj = sdr_list_data(sdr, cxref->contactElt);
	sdr_free(sdr, obj);
	sdr_list_delete(sdr, cxref->contactElt, NULL, NULL);

	/*	Delete routing object, if any.				*/

	if (cxref->routingObject)
	{
		psm_free(ionwm, cxref->routingObject);
		cxref->routingObject = 0;
	}

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

	/*	Apply to current state of affected neighbors, if any.	*/

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

	/*	Detach all references.					*/

	if (cxref->citations)
	{
		for (elt = sm_list_first(ionwm, cxref->citations); elt;
				elt = sm_list_next(ionwm, elt))
		{
			/*	Data content of contact is a routing-
			 *	dependent SmList element.  Erase the
			 *	content of that SmList element, so
			 *	that it is no longer pointing at
			 *	this (now nonexistent) IonCXref
			 *	object - to prevent seg faults.		*/

			citation = sm_list_data(ionwm, elt);
			oK(sm_list_data_set(ionwm, citation, 0));
		}

		sm_list_destroy(ionwm, cxref->citations, NULL, NULL);
	}

	/*	Delete contact from index.				*/

	if (cxref->type != CtRegistration && cxref->toTime > currentTime)
	{
		/*	Affects routes.					*/

		getCurrentTime(&(vdb->lastEditTime));
	}

	sm_rbt_delete(ionwm, vdb->contactIndex, rfx_order_contacts, cxref,
			rfx_erase_data, NULL);
}

static void	vacateRegion(IonDB *iondb, Object iondbObj, int regionIdx,
			int announce)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();
	IonVdb		*vdb = getIonVdb();
	uint32_t	regionNbr = iondb->regions[regionIdx].regionNbr;
	Object		elt;
	Object		nextElt;
	Object		obj;
	IonContact	contact;
	IonCXref	arg;
	PsmAddress	cxelt;
	PsmAddress	cxaddr;
	RegionMember	member;

	iondb->regions[regionIdx].regionNbr = 0;

	/*	Forget contact plan for this region.			*/

	for (elt = sdr_list_first(sdr, iondb->regions[regionIdx].contacts);
			elt; elt = nextElt)
	{
		nextElt = sdr_list_next(sdr, elt);
		obj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &contact, obj, sizeof(IonContact));
		memset((char *) &arg, 0, sizeof(IonCXref));
		arg.regionNbr = regionNbr;
		arg.fromNode = contact.fromNode;
		arg.toNode = contact.toNode;
		arg.fromTime = contact.fromTime;
		oK(sm_rbt_search(ionwm, vdb->contactIndex, rfx_order_contacts,
				&arg, &cxelt));
		if (cxelt)
		{
			cxaddr = sm_rbt_data(ionwm, cxelt);
			deleteContact(cxaddr);
		}
	}

	sdr_list_destroy(sdr, iondb->regions[regionIdx].contacts, NULL, NULL);
	iondb->regions[regionIdx].contacts = 0;

	/*	Forget membership of this region.			*/

	if (regionIdx == 1)	/*	Vacating outer region.		*/
	{
		/*	At this point, the local node's outer region
		 *	number has been set to zero.			*/

		for (elt = sdr_list_first(sdr, iondb->rolodex); elt;
				elt = nextElt)
		{
			nextElt = sdr_list_next(sdr, elt);
			obj = sdr_list_data(sdr, elt);
			sdr_read(sdr, (char *) &member, obj,
					sizeof(RegionMember));
			if (member.outerRegionNbr == regionNbr)
			{
				/*	Member's outer region is the
				 *	same as the local node's
				 *	previous outer region, so
				 *	the member must be in the
				 *	local node's home region;
				 *	retain this member as a
				 *	passageway up to that outer
				 *	region.				*/

				continue;
			}

			if (member.homeRegionNbr == regionNbr)
			{
				/*	Member is a passageway up to
				 *	the super-region of the local
				 *	node's previous outer region.
				 *	That passageway will no longer
				 *	be usable, so discard member.	*/

				sdr_free(sdr, obj);
				sdr_list_delete(sdr, elt, NULL, NULL);
			}
		}
	}
	else			/*	Vacating home region.		*/
	{
		/*	The local node's outer region (if any) is
		 *	about to become the local node's new home
		 *	region.  The local node's home region number
		 *	has been set to zero.				*/

		for (elt = sdr_list_first(sdr, iondb->rolodex); elt;
				elt = nextElt)
		{
			nextElt = sdr_list_next(sdr, elt);
			obj = sdr_list_data(sdr, elt);
			sdr_read(sdr, (char *) &member, obj,
					sizeof(RegionMember));
			if (member.outerRegionNbr == regionNbr)
			{
				/*	Member is a passageway down
				 *	to a sub-region of the local
				 *	node's previous home region.
				 *	That passageway will no longer
				 *	be usable, so discard member.	*/

				sdr_free(sdr, obj);
				sdr_list_delete(sdr, elt, NULL, NULL);
				continue;
			}

			if (member.homeRegionNbr == regionNbr)
			{
				if (member.outerRegionNbr
						== iondb->regions[1].regionNbr)
				{
					/*	Member will still be
					 *	in the same region as
					 *	the local node (the
					 *	local node's current
					 *	outer region, which
					 *	will soon become its
					 *	home region).  So
					 *	retain this member as
					 *	a passageway down to
					 *	the local node's
					 *	previous home region.	*/

					continue;
				}

				/*	Since the local node's home
				 *	region can only be a sub-region
				 *	of one other region, and this
				 *	member's outer region is not
				 *	that region, the member can't
				 *	be a passageway.  It must be
				 *	a terminal node residing in
				 *	the local node's current home
				 *	region, soon to be unreachable
				 *	by CGR.  So discard it.		*/

				sdr_free(sdr, obj);
				sdr_list_delete(sdr, elt, NULL, NULL);
			}
		}
	}

	/*	Post node unregistration notice if necessary.		*/

	if (announce)
	{
		postCpsNotice(regionNbr, MAX_POSIX_TIME, 0, contact.fromNode,
				0, 0, 0.0);
	}
}

static void	purgePassageways(uint32_t regionNbr, IonDB *iondb,
			Object iondbObj, int announce)
{
	Sdr		sdr = getIonsdr();
	uint32_t	homeRegionNbr;
	Object		elt;
	Object		memberObj;
	RegionMember	member;

	homeRegionNbr = iondb->regions[0].regionNbr;
	for (elt = sdr_list_first(sdr, iondb->rolodex); elt;
			elt = sdr_list_next(sdr, elt))
	{
		memberObj = sdr_list_data(sdr, elt);
		sdr_stage(sdr, (char *) &member, memberObj,
				sizeof(RegionMember));
		if (member.homeRegionNbr != homeRegionNbr
		|| member.outerRegionNbr == 0
		|| member.outerRegionNbr == regionNbr)
		{
			continue;
		}

		/*	This node is a passageway to the region which
		 *	was formerly - but is no longer - the local
		 *	node's outer region.  Must note that it is no
		 *	longer registered in that former outer region.	*/

		if (member.nodeNbr == getOwnNodeNbr())
		{
			vacateRegion(iondb, iondbObj, 1, announce);
		}

		member.outerRegionNbr = 0;
		sdr_write(sdr, memberObj, (char *) &member,
				sizeof(RegionMember));
	}
}

static void	registerInRegion(uint32_t regionNbr, uvast nodeNbr,
			IonDB *iondb, Object iondbObj, int announce)
{
	Sdr		sdr = getIonsdr();
	Object		elt;
	Object		memberObj;
	RegionMember	member;

	for (elt = sdr_list_first(sdr, iondb->rolodex); elt;
			elt = sdr_list_next(sdr, elt))
	{
		memberObj = sdr_list_data(sdr, elt);
		sdr_stage(sdr, (char *) &member, memberObj,
				sizeof(RegionMember));
		if (member.nodeNbr < nodeNbr)
		{
			continue;
		}

		if (member.nodeNbr > nodeNbr)
		{
			/*	Node is not in the membership list.	*/

			break;
		}

		/*	Node is a known member of at least one of
		 *	the local node's (up to 2) regions.  Adjust
		 *	membership if necessary.			*/

		if (regionNbr < member.homeRegionNbr)
		{
			/*	Registering in new outer region.	*/

			if (regionNbr == member.outerRegionNbr)
			{
				/*	Redundant registration; ignore.	*/

				return;
			}

			/*	Node is now a passageway to this region.*/

			member.outerRegionNbr = regionNbr;
			sdr_write(sdr, memberObj, (char *) &member,
					sizeof(RegionMember));

			/*	Any existing passageways to the former
			 *	outer region must now be erased.	*/

			purgePassageways(regionNbr, iondb, iondbObj, announce);
			return;
		}

		if (regionNbr == member.homeRegionNbr)
		{
			/*	Redundant registration; ignore.		*/

			return;
		}

		/*	Registering in new home region.  Current
		 *	home region becomes new outer region.		*/

		member.outerRegionNbr = member.homeRegionNbr;
		member.homeRegionNbr = regionNbr;
		sdr_write(sdr, memberObj, (char *) &member,
				sizeof(RegionMember));
		return;
	}

	/*	Not in membership list.	 Assume regionNbr identifies
	 *	the node's home region; if it's not, a supplementary
	 *	registration in another region will force the
	 *	adjustment.						*/

	member.nodeNbr = nodeNbr;
	member.homeRegionNbr = regionNbr;
	member.outerRegionNbr = 0;
	memberObj = sdr_malloc(sdr, sizeof(RegionMember));
	if (memberObj)
	{
		sdr_write(sdr, memberObj, (char *) &member,
				sizeof(RegionMember));
		if (elt)
		{
			sdr_list_insert_before(sdr, elt, memberObj);
		}
		else
		{
			sdr_list_insert_last(sdr, iondb->rolodex, memberObj);
		}
	}
}

static void	registerSelf(uint32_t regionNbr, IonDB *iondb, Object iondbObj,
			int announce)
{
	Sdr	sdr = getIonsdr();

	/*	Must unregister from current outer region, if any.	*/

	if (iondb->regions[1].regionNbr != 0)
	{
		vacateRegion(iondb, iondbObj, 1, announce);
	}

	/*	Select registration mode.  The region number of a
	 *	sub-region is always greater than the region number
	 *	of its super-region.					*/

	if (regionNbr > iondb->regions[0].regionNbr)
	{
		/*	Node is migrating to a (possibly new) sub-
		 *	region.  Node's current home region becomes
		 *	its outer region...				*/

		iondb->regions[1].regionNbr = iondb->regions[0].regionNbr;
		iondb->regions[1].contacts = iondb->regions[0].contacts;

		/*	...and the cited region becomes the node's
		 *	new home region.				*/

		iondb->regions[0].regionNbr = regionNbr;
		iondb->regions[0].contacts = sdr_list_create(sdr);
		CHKVOID(iondb->regions[0].contacts);
		sdr_list_user_data_set(sdr, iondb->regions[0].contacts,
				(Address) regionNbr);
	}
	else
	{
		/*	Node is being recruited as passageway into a
		 *	super-region.  The cited region becomes the
		 *	node's outer region.				*/

		iondb->regions[1].regionNbr = regionNbr;
		iondb->regions[1].contacts = sdr_list_create(sdr);
		CHKVOID(iondb->regions[1].contacts);
		sdr_list_user_data_set(sdr, iondb->regions[1].contacts,
				(Address) regionNbr);
	}

	registerInRegion(regionNbr, getOwnNodeNbr(), iondb, iondbObj, announce);
}

static void	handleRegistrationContact(uint32_t regionNbr, uvast nodeNbr,
			IonDB *iondb, Object iondbObj, PsmAddress *cxaddr,
			int announce)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();
	IonVdb		*vdb = getIonVdb();
	IonCXref	arg;
	PsmAddress	cxelt;
	PsmAddress	nextElt;
	int		regionIdx;

	arg.regionNbr = regionNbr;
	arg.fromNode = nodeNbr;
	arg.toNode = nodeNbr;
	arg.fromTime = MAX_POSIX_TIME;
	cxelt = sm_rbt_search(ionwm, vdb->contactIndex, rfx_order_contacts,
			&arg, &nextElt);
	if (cxelt)	/*	Node already registered in region.	*/
	{
		return;
	}

       	if (nodeNbr == getOwnNodeNbr())
	{
		/*	Registering self in a region.			*/

		registerSelf(regionNbr, iondb, iondbObj, announce);
	}
	else	/*	Registering some other node.			*/
	{
		registerInRegion(regionNbr, nodeNbr, iondb, iondbObj, announce);
	}

	/*	Post node registration announcement.			*/

	if (announce)
	{
		postCpsNotice(regionNbr, MAX_POSIX_TIME, MAX_POSIX_TIME,
				nodeNbr, nodeNbr, 0, 1.0);
	}

	/*	Add the registration contact.				*/

	sdr_write(sdr, iondbObj, (char *) iondb, sizeof(IonDB));
	regionIdx = ionPickRegion(regionNbr);
	CHKVOID(regionIdx == 0 || regionIdx == 1);
	insertContact(regionIdx, iondb, iondbObj, MAX_POSIX_TIME,
			MAX_POSIX_TIME, nodeNbr, nodeNbr, 0, 1.0,
			CtRegistration, cxaddr);
}

int	rfx_insert_contact(uint32_t regionNbr, time_t fromTime, time_t toTime,
		uvast fromNode, uvast toNode, size_t xmitRate, float confidence,
		PsmAddress *cxaddr, int announce)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();
	IonVdb 		*vdb = getIonVdb();
	uvast		ownNodeNbr = getOwnNodeNbr();
	ContactType	contactType;
	Object		iondbObj;
	IonDB		iondb;
	int		regionIdx;
	IonCXref	arg;
	PsmAddress	cxelt;
	PsmAddress	nextElt;
	IonCXref	*cxref;
	char		buf1[TIMESTAMPBUFSZ];
	char		buf2[TIMESTAMPBUFSZ];
	char		contactIdString[128];

	CHKERR(cxaddr);
	*cxaddr = 0;			/*	Default.		*/
	if (regionNbr == 0)
	{
		writeMemo("[?] Can't insert contact for region number 0.");
		return 1;
	}

	if (fromNode == 0)
	{
		writeMemo("[?] Can't insert contact From node 0.");
		return 2;
	}

	if (toNode == 0)
	{
		writeMemo("[?] Can't insert contact To node 0.");
		return 3;
	}

	if (confidence < 0.0 || confidence > 1.0)
	{
		writeMemo("[?] Can't insert contact: confidence must be \
between 0.0 and 1.0.");
		return 4;
	}

	/*	Override artificial parameter values as necessary.
	 *
	 *	Note: Discovered contacts are ONLY created by
	 *	tranformation from Hypothetical contacts, and
	 *	Suppressed contacts are ONLY created by transformation
	 *	from Scheduled contacts.  So this new contact must
	 *	be Registration, Hypothetical, Predicted, or Scheduled.	*/

	if (fromTime == MAX_POSIX_TIME)	/*	Registration.		*/
	{
		CHKZERO(fromNode == toNode);
		toTime = MAX_POSIX_TIME;
		xmitRate = 0;
		confidence = 1.0;
		contactType = CtRegistration;
	}
	else if (fromTime == 0)		/*	Hypothetical.		*/
	{
		CHKZERO(fromNode != toNode);
		CHKZERO(fromNode == ownNodeNbr || toNode == ownNodeNbr);
		toTime = MAX_POSIX_TIME;
		xmitRate = 0;
		confidence = 0.0;
		contactType = CtHypothetical;
	}
	else			/*	Scheduled or Predicted.		*/
	{
		if (xmitRate == 0)
		{
			writeMemo("[?] Can't insert contact with xmit rate 0.");
			return 5;
		}

		if (fromTime == toTime)	/*	Predicted.		*/
		{
			CHKZERO(fromNode != ownNodeNbr && toNode != ownNodeNbr);
			fromTime = getCtime();	/*	Now.		*/
			contactType = CtPredicted;
		}
		else			/*	Scheduled.		*/
		{
			if (toTime < fromTime)
			{
				writeMemo("[?] Can't insert contact, To time \
must be later than From time.");
				return 6;
			}

			/*	No artificial parameter values,
			 *	nothing to override.			*/

			contactType = CtScheduled;
		}
	}

	iondbObj = getIonDbObject();
	CHKERR(iondbObj);
	CHKERR(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));

	/*	Insert registration contact, if applicable.		*/

	if (contactType == CtRegistration)
	{
		handleRegistrationContact(regionNbr, fromNode, &iondb, iondbObj,
				cxaddr, announce);
		return sdr_end_xn(sdr);
	}

	/*	New contact is either Hypothetical or Scheduled or
	 *	Predicted.  Must occur within one of this node's
	 *	regions in order to be utilized.			*/

	if (contactType != CtScheduled)
	{
		/*	Effects of adding Hypothetical and Predicted
		 *	contacts are private, not to be synchronized
		 *	with the rest of the region.			*/

		announce = 0;
	}

	if (regionNbr != iondb.regions[0].regionNbr
	&& regionNbr != iondb.regions[1].regionNbr)
	{
		writeMemoNote("[?] Contact is for a foreign region",
				itoa(regionNbr));
		sdr_exit_xn(sdr);
		return 7;	/*	Do not insert.		*/
	}

	*cxaddr = 0;	/*	Default.				*/
	memset((char *) &arg, 0, sizeof(IonCXref));
	arg.regionNbr = regionNbr;
	arg.fromNode = fromNode;
	arg.toNode = toNode;
	cxelt = sm_rbt_search(ionwm, vdb->contactIndex, rfx_order_contacts,
			&arg, &nextElt);

	/*	Set cxelt to point to first contact for this node pair,
	 *	if any.							*/

	if (cxelt == 0)
	{
		cxelt = nextElt;
	}

	while (cxelt)
	{
		/*	In this loop we examine the existing contacts
		 *	for this node pair.
		 *
		 *	If the new contact is Hypothetical, then (a)
		 *	we must reject it if an existing Hypothetical
		 *	contact is now Discovered and (b) if no current
		 *	contact is Discovered then we must remove any
		 *	existing Hypothetical contact.  As soon as we
		 *	are past all contacts with fromTime zero we
		 *	know these conditions have been satisfied and
		 *	we can break out of the loop and insert the
		 *	new contact.
		 *
		 *	If instead the new contact is Scheduled or
		 *	Predicted, then (a) we must reject it if its
		 *	time interval overlaps with any Scheduled or
		 *	Suppressed contact and (b) we must be sure
		 *	to emove any existing Predicted contact with
		 *	which this one overlaps.  As soon as we are
		 *	past all potentially overlapping contacts we
		 *	break out of the loop and insert the new
		 *	contact.					*/
		
		*cxaddr = sm_rbt_data(ionwm, cxelt);
		cxref = (IonCXref *) psp(ionwm, *cxaddr);
		if (cxref->fromNode != fromNode || cxref->toNode != toNode)
		{
			break;	/*	No more contacts for node pair.	*/
		}

		if (contactType == CtHypothetical)
		{
			if (cxref->fromTime == 0)
			{
				/*	If contact's fromTime is 0, the
				 *	contact must be Hypothetical,
				 *	Discovered, Scheduled, or
				 *	Suppressed (scheduled).
				 *	Predicted contacts always
				 *	have non-zero fromTime values.	*/

				switch (cxref->type)
				{
				case CtDiscovered:
					writeMemo("[?] Can't replace \
hypothetical contact, as that contact is now discovered.");
					sdr_exit_xn(sdr);
					return 8;

				case CtHypothetical:
					/*	Replace this one.	*/

					if (rfx_remove_contact(cxref->regionNbr,
							&cxref->fromTime,
							cxref->fromNode,
							cxref->toNode, 0) < 0)
					{
						sdr_cancel_xn(sdr);
						return -1;
					}

					/*	No need to check any
					 *	further.		*/

					break;	/*	Out of switch.	*/

				default:
					/*	Continue examining
					 *	contacts.		*/

					cxelt = sm_rbt_next(ionwm, cxelt);
					continue;
				}
			}

			/*	No need to check any further.		*/

			break;			/*	Out of loop.	*/
		}

		/*	New contact is scheduled or predicted.		*/

		if (cxref->fromTime > toTime)	/*	Future contact.	*/
		{
			/*	No more potential overlaps.		*/

			break;			/*	Out of loop.	*/
		}

		if (cxref->toTime < fromTime)
		{
			/*	Prior contact, no overlap.		*/

			cxelt = sm_rbt_next(ionwm, cxelt);
			continue;
		}

		/*	This contact begins at or before the stop
		 *	time of the new contact but ends on or after
		 *	the start time of the new contact, so the new
		 *	contact overlaps with it.			*/

		if (cxref->type == CtPredicted)
		{
			/*	The new contact overlaps with a
			 *	predicted contact.  The predicted
			 *	contact is overridden.			*/

			if (rfx_remove_contact(cxref->regionNbr,
					&cxref->fromTime, cxref->fromNode,
					cxref->toNode, 0) < 0)
			{
				sdr_cancel_xn(sdr);
				return -1;
			}

			/*	Removing a contact reshuffles the
			 *	contact RBT, so must reposition within
			 *	the table.  Start again with the first
			 *	remaining contact for this node pair.	*/

			cxelt = sm_rbt_search(ionwm, vdb->contactIndex,
					rfx_order_contacts, &arg, &nextElt);
			if (cxelt == 0)
			{
				cxelt = nextElt;
			}

			continue;
		}

		/*	New contact overlaps with a non-predicted
		 *	contact.					*/

		writeTimestampUTC(fromTime, buf1);
		writeTimestampUTC(toTime, buf2);
		isprintf(contactIdString, sizeof contactIdString,
				"from %s until %s, %lu->%lu", buf1, buf2,
				fromNode, toNode);
		writeMemoNote("[?] Overlapping contact ignored",
				contactIdString);
		if (sdr_end_xn(sdr) < 0)
		{
			return -1;
		}

		return 9;
	}

	/*	Contact doesn't conflict with any other contact in
	 *	the database; okay to add.
	 *
	 *	By virtue of this contact, the nodes involved are
	 *	automatically members of the indicated region even
	 *	if no registration contacts were previously posted.	*/

	registerInRegion(regionNbr, fromNode, &iondb, iondbObj, announce);
	registerInRegion(regionNbr, toNode, &iondb, iondbObj, announce);
	regionIdx = ionPickRegion(regionNbr);
	insertContact(regionIdx, &iondb, iondbObj, fromTime, toTime, fromNode,
			toNode, xmitRate, confidence, contactType, cxaddr);

	/*	Notify other nodes in the region if necessary.		*/

	if (announce)
	{
		postCpsNotice(regionNbr, fromTime, toTime, fromNode, toNode,
				xmitRate, confidence);
	}

	return sdr_end_xn(sdr);
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
node " UVAST_FIELDSPEC " to node " UVAST_FIELDSPEC " is %10lu bytes/sec, \
confidence %f.", fromTimeBuffer, toTimeBuffer, contact->fromNode,
		contact->toNode, contact->xmitRate, contact->confidence);
	return buffer;
}

int	rfx_revise_contact(uint32_t regionNbr, time_t fromTime, uvast fromNode,
		uvast toNode, size_t xmitRate, float confidence, int announce)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();
	IonVdb 		*vdb = getIonVdb();
	time_t		currentTime = getCtime();
	IonCXref	arg;
	PsmAddress	cxelt;
	PsmAddress	nextElt;
	PsmAddress	cxaddr;
	IonCXref	*cxref;
	Object		obj;
	IonContact	contact;
	IonNeighbor	*neighbor;

	CHKERR(confidence <= 1.0);
	memset((char *) &arg, 0, sizeof(IonCXref));
	arg.regionNbr = regionNbr;
	arg.fromNode = fromNode;
	arg.toNode = toNode;
	arg.fromTime = fromTime;
	CHKERR(sdr_begin_xn(sdr));
	cxelt = sm_rbt_search(ionwm, vdb->contactIndex, rfx_order_contacts,
			&arg, &nextElt);
	if (cxelt == 0)		/*	No such contact.		*/
	{
		writeMemo("[!] Attempt to revise a nonexistent contact.");
		sdr_exit_xn(sdr);
		return 1;
	}

	/*	Update the contact and its xref.			*/

	cxaddr = sm_rbt_data(ionwm, cxelt);
	cxref = (IonCXref *) psp(ionwm, cxaddr);
	if (cxref->type == CtRegistration
	|| cxref->type == CtHypothetical
	|| cxref->type == CtDiscovered)
	{
		writeMemo("[!] Attempt to revise a non-managed contact.");
		sdr_exit_xn(sdr);
		return 2;
	}

	obj = sdr_list_data(sdr, cxref->contactElt);
	sdr_stage(sdr, (char *) &contact, obj, sizeof(IonContact));
	contact.xmitRate = xmitRate;
	cxref->xmitRate = xmitRate;
	if (confidence >= 0.0)
	{
		contact.confidence = confidence;
		cxref->confidence = confidence;
	}

	sdr_write(sdr, obj, (char *) &contact, sizeof(IonContact));

	/*	Apply to current state of affected neighbor, if any.	*/

	if (currentTime >= cxref->startXmit && currentTime <= cxref->stopXmit)
	{
		neighbor = findNeighbor(vdb, cxref->toNode, &nextElt);
		if (neighbor)
		{
			neighbor->xmitRate = xmitRate;
		}
	}

	if (currentTime >= cxref->startFire && currentTime <= cxref->stopFire)
	{
		neighbor = findNeighbor(vdb, cxref->fromNode, &nextElt);
		if (neighbor)
		{
			neighbor->fireRate = xmitRate;
		}
	}

	if (currentTime >= cxref->startRecv && currentTime <= cxref->stopRecv)
	{
		neighbor = findNeighbor(vdb, cxref->fromNode, &nextElt);
		if (neighbor)
		{
			neighbor->recvRate = xmitRate;
		}
	}

	/*	Contact has been updated.  No change to contact graph,
	 *	no need to recompute routes.  Announce if necessary.	*/

	if (announce)
	{
		if (confidence < 0.0)	/*	No change.		*/
		{
			postCpsNotice(regionNbr, cxref->fromTime, cxref->toTime,
				cxref->fromNode, cxref->toNode,
				cxref->xmitRate, 4.0);
		}
		else	/*	Encode confidence to indicate revision.	*/
		{
			postCpsNotice(regionNbr, cxref->fromTime, cxref->toTime,
				cxref->fromNode, cxref->toNode,
				cxref->xmitRate, cxref->confidence + 2.0);
		}
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't revise contact.", NULL);
		return -1;
	}

	return 0;
}

static void	removeAllContacts(uint32_t regionNbr, uvast fromNode,
			uvast toNode, int announce)
{
	PsmPartition	ionwm = getIonwm();
	IonVdb 		*vdb = getIonVdb();
	IonCXref	arg;
	PsmAddress	cxelt;
	PsmAddress	nextElt;
	PsmAddress	cxaddr;
	IonCXref	*cxref;

	memset((char *) &arg, 0, sizeof(IonCXref));
	arg.regionNbr = regionNbr;
	arg.fromNode = fromNode;
	arg.toNode = toNode;

	/*	Get first contact for this to/from node pair.		*/

	oK(sm_rbt_search(ionwm, vdb->contactIndex, rfx_order_contacts, &arg,
			&cxelt));

	/*	Now delete contacts until a node number changes.	*/

	while (cxelt)
	{
		cxaddr = sm_rbt_data(ionwm, cxelt);
		cxref = (IonCXref *) psp(ionwm, cxaddr);
		if (cxref->fromNode > fromNode
		|| cxref->toNode > toNode)
		{
			break;	/*	No more matches.		*/
		}

		nextElt = sm_rbt_next(ionwm, cxelt); 
		if (announce)
		{
			postCpsNotice(regionNbr, cxref->fromTime, 0,
				cxref->fromNode, cxref->toNode, 0, 0.0);
		}

		deleteContact(cxaddr);

		/*	Now reposition at the next contact.		*/

		if (nextElt == 0)
		{
			break;	/*	No more contacts.		*/
		}

		cxaddr = sm_rbt_data(ionwm, nextElt);
		cxref = (IonCXref *) psp(ionwm, cxaddr);
		arg.regionNbr = regionNbr;
		arg.fromNode = cxref->fromNode;
		arg.toNode = cxref->toNode;
		arg.fromTime = cxref->fromTime;
		cxelt = sm_rbt_search(ionwm, vdb->contactIndex,
				rfx_order_contacts, &arg, &nextElt);
	}
}

static void	unregisterFromRegion(uvast fromNode, IonCXref *cxref,
			uint32_t regionNbr, int announce)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();
	IonVdb 		*vdb = getIonVdb();
	int		regionIdx;
	Object		iondbObj;
	IonDB		iondb;
	Object		elt;
	Object		nextElt;
	Object		obj;
	IonContact	contact;
	IonCXref	arg;
	PsmAddress	cxelt;
	PsmAddress	cxaddr;
	RegionMember	member;

	iondbObj = getIonDbObject();
	CHKVOID(iondbObj);
	sdr_stage(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
	if (fromNode == getOwnNodeNbr())
	{
		/*	Local node is being removed from region.	*/

		if (regionNbr == iondb.regions[0].regionNbr)
		{
			/*	Node is leaving its home region.
			 *	Its outer region, if any, becomes
			 *	its new home region.			*/

			vacateRegion(&iondb, iondbObj, 0, announce);
			iondb.regions[0].regionNbr = iondb.regions[1].regionNbr;
			iondb.regions[0].contacts = iondb.regions[1].contacts;
			iondb.regions[1].regionNbr = 0;
			iondb.regions[1].contacts = 0;
		}
		else
		{
			/*	Node is leaving its outer region.	*/

			vacateRegion(&iondb, iondbObj, 1, announce);
		}

		sdr_write(sdr, iondbObj, (char *) &iondb, sizeof(IonDB));
		return;
	}

	/*	The unregistering node is not the local node.  Just
	 *	delete all other contacts for this region that are
	 *	From or To this node and update the rolodex entry
	 *	for this node.						*/

	regionIdx = ionPickRegion(regionNbr);
	if (regionIdx < 0 || regionIdx > 1)
	{
		return;
	}

	for (elt = sdr_list_first(sdr, iondb.regions[regionIdx].contacts);
			elt; elt = nextElt)
	{
		nextElt = sdr_list_next(sdr, elt);
		obj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &contact, obj, sizeof(IonContact));
		if (contact.fromNode == fromNode
		|| contact.toNode == fromNode)
		{
			memset((char *) &arg, 0, sizeof(IonCXref));
			arg.regionNbr = regionNbr;
			arg.fromNode = contact.fromNode;
			arg.toNode = contact.toNode;
			arg.fromTime = contact.fromTime;
			oK(sm_rbt_search(ionwm, vdb->contactIndex,
					rfx_order_contacts, &arg, &cxelt));
			if (cxelt)
			{
				cxaddr = sm_rbt_data(ionwm, cxelt);
				deleteContact(cxaddr);
			}
		}
	}

	for (elt = sdr_list_first(sdr, iondb.rolodex); elt; elt = nextElt)
	{
		nextElt = sdr_list_next(sdr, elt);
		obj = sdr_list_data(sdr, elt);
		sdr_stage(sdr, (char *) &member, obj, sizeof(RegionMember));
		if (member.nodeNbr < fromNode)
		{
			continue;
		}

		if (member.nodeNbr > fromNode)
		{
			break;		/*	Node not found.		*/
		}

		/*	This is the unregistering node.			*/

		if (member.outerRegionNbr == regionNbr)
		{
			/*	Node is being unregistered from its
			 *	outer region.				*/

			member.outerRegionNbr = 0;
			sdr_write(sdr, obj, (char *) &member,
					sizeof(RegionMember));
			break;
		}

		if (member.homeRegionNbr == regionNbr)
		{
			/*	Node is being unregistered from its
			 *	home region; its outer region, if
			 *	any, becomes its home region.		*/

			if (member.outerRegionNbr == 0)
			{
				/*	Complete removal of node.	*/

				sdr_free(sdr, obj);
				sdr_list_delete(sdr, elt, NULL, NULL);
			}
			else
			{
				member.homeRegionNbr = member.outerRegionNbr;
				member.outerRegionNbr = 0;
				sdr_write(sdr, obj, (char *) &member,
						sizeof(RegionMember));
			}

			break;
		}

		/*	A glitch: the node is not registered in this
		 *	region.  Nothing to do.				*/

		break;
	}
}

static void	removeOneContact(uint32_t regionNbr, time_t fromTime,
			uvast fromNode, uvast toNode, int announce)
{
	PsmPartition	ionwm = getIonwm();
	IonVdb 		*vdb = getIonVdb();
	IonCXref	arg;
	PsmAddress	cxelt;
	PsmAddress	nextElt;
	PsmAddress	cxaddr;
	IonCXref	*cxref;

	memset((char *) &arg, 0, sizeof(IonCXref));
	arg.regionNbr = regionNbr;
	arg.fromNode = fromNode;
	arg.toNode = toNode;
	arg.fromTime = fromTime;

	/*	Get the contact.					*/

	cxelt = sm_rbt_search(ionwm, vdb->contactIndex, rfx_order_contacts,
			&arg, &nextElt);
	if (cxelt == 0)		/*	No such contact.		*/
	{
		return;
	}

	cxaddr = sm_rbt_data(ionwm, cxelt);
	cxref = (IonCXref *) psp(ionwm, cxaddr);
	if (cxref->type == CtRegistration)
	{
		/*	Deleting a registration contact removes
		 *	the node from the region.  The registration
		 *	contact itself is deleted in the process.	*/

		unregisterFromRegion(fromNode, cxref, regionNbr, announce);
	}
	else	/*	Ordinary contact removal.			*/
	{
		if (announce)
		{
			postCpsNotice(regionNbr, cxref->fromTime, 0,
				cxref->fromNode, cxref->toNode, 0, 0.0);
		}

		deleteContact(cxaddr);
	}
}

int	rfx_remove_contact(uint32_t regionNbr, time_t *fromTime, uvast fromNode,
		uvast toNode, int announce)
{
	Sdr	sdr = getIonsdr();

	/*	Note: when the fromTime passed to ionadmin is '*'
	 *	a NULL fromTime is passed to rfx_remove_contact,
	 *	where it is interpreted as "all contacts between
	 *	these two nodes".					*/

	CHKERR(regionNbr > 0);
	CHKERR(sdr_begin_xn(sdr));
	if (fromTime)		/*	Not a wild-card deletion.	*/
	{
		removeOneContact(regionNbr, *fromTime, fromNode, toNode,
				announce);
	}
	else		/*	Wild-card deletion, start at time zero.	*/
	{
		removeAllContacts(regionNbr, fromNode, toNode, announce);
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't remove contact(s).", NULL);
		return -1;
	}

	return 0;
}

void	rfx_brief_contacts(uint32_t regionNbr)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();
	IonVdb		*vdb = getIonVdb();
	char		fileName[64];
	int		briefingFile;
	PsmAddress	elt;
	PsmAddress	addr;
	IonCXref	*contact;
	char		fromTimeBuffer[TIMESTAMPBUFSZ];
	char		toTimeBuffer[TIMESTAMPBUFSZ];
	char		buffer[256];
	int		textLen;

	isprintf(fileName, sizeof fileName, "contacts.%lu.ionrc", regionNbr);
	briefingFile = iopen(fileName, O_WRONLY | O_CREAT, 0666);
	if (briefingFile == -1)
	{
		putSysErrmsg("Can't create briefing file", fileName);
		return;
	}

	isprintf(buffer, sizeof buffer, "^ %lu\n", regionNbr);
       	textLen = strlen(buffer);
	if (write(briefingFile, buffer, textLen) < 0)
	{
		close(briefingFile);
		putSysErrmsg("Can't write '^' command to briefing file",
				fileName);
		return;
	}

	if (sdr_begin_xn(sdr) < 0)
	{
		close(briefingFile);
		putErrmsg("Can't start transaction", NULL);
		return;
	}

	/*	First list all registration contacts.			*/

	for (elt = sm_rbt_first(ionwm, vdb->contactIndex); elt;
			elt = sm_rbt_next(ionwm, elt))
	{
		addr = sm_rbt_data(ionwm, elt);
		contact = (IonCXref *) psp(ionwm, addr);
		if (contact->type != CtRegistration)
		{
			continue;
		}

		if (contact->regionNbr < regionNbr)
		{
			continue;
		}

		if (contact->regionNbr > regionNbr)
		{
			break;
		}

		writeTimestampUTC(contact->fromTime, fromTimeBuffer);
		writeTimestampUTC(contact->toTime, toTimeBuffer);
		isprintf(buffer, sizeof buffer, "a contact %20s %20s "
UVAST_FIELDSPEC " " UVAST_FIELDSPEC " %lu %f\n", fromTimeBuffer, toTimeBuffer,
				contact->fromNode, contact->toNode,
				contact->xmitRate, contact->confidence);
       		textLen = strlen(buffer);
		if (write(briefingFile, buffer, textLen) < 0)
		{
			putSysErrmsg("Can't write briefing command", fileName);
			break;
		}
	}

	/*	Then all non-registration contacts.			*/

	for (elt = sm_rbt_first(ionwm, vdb->contactIndex); elt;
			elt = sm_rbt_next(ionwm, elt))
	{
		addr = sm_rbt_data(ionwm, elt);
		contact = (IonCXref *) psp(ionwm, addr);
		if (contact->type == CtRegistration)
		{
			continue;
		}

		if (contact->regionNbr < regionNbr)
		{
			continue;
		}

		if (contact->regionNbr > regionNbr)
		{
			break;
		}

		writeTimestampUTC(contact->fromTime, fromTimeBuffer);
		writeTimestampUTC(contact->toTime, toTimeBuffer);
		isprintf(buffer, sizeof buffer, "a contact %20s %20s "
UVAST_FIELDSPEC " " UVAST_FIELDSPEC " %lu %f\n", fromTimeBuffer, toTimeBuffer,
				contact->fromNode, contact->toNode,
				contact->xmitRate, contact->confidence);
       		textLen = strlen(buffer);
		if (write(briefingFile, buffer, textLen) < 0)
		{
			putSysErrmsg("Can't write briefing command", fileName);
			break;
		}
	}

	/*	Done.							*/

	sdr_exit_xn(sdr);
	close(briefingFile);
	return;
}

void	rfx_contact_state(uvast nodeNbr, size_t *secRemaining, size_t *xmitRate)
{
	int		regionIdx;
	uint32_t	regionNbr;
	PsmPartition	ionwm = getIonwm();
	IonVdb		*ionvdb = getIonVdb();
	time_t		currentTime = getCtime();
	IonCXref	arg;
	PsmAddress	elt;
	IonCXref	*contact;
	int		candidateContacts = 0;

	*secRemaining = 0;		/*	Default.		*/
	*xmitRate = ((size_t) -1);	/*	Default.		*/
	regionIdx = ionRegionOf(getOwnNodeNbr(), nodeNbr, &regionNbr);
	if (regionIdx < 0)	/*	Node not in known contact plan.	*/
	{
		return;
	}

	memset((char *) &arg, 0, sizeof(IonCXref));
	arg.regionNbr = regionNbr;
	arg.fromNode = getOwnNodeNbr();
	for (oK(sm_rbt_search(ionwm, ionvdb->contactIndex, rfx_order_contacts,
			&arg, &elt)); elt; elt = sm_rbt_next(ionwm, elt))
	{
		contact = (IonCXref *) psp(ionwm, sm_rbt_data(ionwm, elt));
		if (contact->fromNode > arg.fromNode)
		{
			/*	No more candidate contacts.		*/

			break;
		}

		if (contact->toNode != nodeNbr)
		{
			continue;	/*	Wrong node.		*/
		}

		if (contact->toTime <= currentTime)
		{
			continue;	/*	Contact already ended.	*/
		}

		if (contact->confidence < 1.0)
		{
			continue;	/*	Not useful contact.	*/
		}

		candidateContacts++;
		if (contact->fromTime > currentTime)
		{
			break;		/*	Not current contact.	*/
		}

		*secRemaining = contact->toTime - currentTime;
		*xmitRate = contact->xmitRate;
		return;
	}

	/*	No current contact.					*/

	if (candidateContacts)
	{
		*xmitRate = 0;		/*	No transmission now.	*/
	}
}

/*	*	RFX range list management functions	*	*	*/

static PsmAddress	insertRXref(IonRXref *rxref)
{
	PsmPartition	ionwm = getIonwm();
	IonVdb		*vdb = getIonVdb();
	IonRXref	arg;
	IonNode		*node;
	PsmAddress	nextElt;
	PsmAddress	rxaddr;
	PsmAddress	rxelt;
	PsmAddress	addr;
	IonEvent	*event;
	PsmAddress	rxaddr2;
	IonRXref	*rxref2;
	time_t		currentTime = getCtime();

	/*	If the RXref already exists, just return its address.	*/

	memset((char *) &arg, 0, sizeof(IonRXref));
	arg.fromNode = rxref->fromNode;
	arg.toNode = rxref->toNode;
	arg.fromTime = rxref->fromTime;
	rxelt = sm_rbt_search(ionwm, vdb->rangeIndex, rfx_order_ranges, &arg,
			&nextElt);
	if (rxelt)
	{
		rxaddr = sm_rbt_data(ionwm, rxelt);
		return rxaddr;
	}

	/*	New RXref, so load the affected nodes.			*/

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
		getCurrentTime(&(vdb->lastEditTime));
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

int	rfx_insert_range(time_t fromTime, time_t toTime, uvast fromNode,
		uvast toNode, unsigned int owlt, PsmAddress *rxaddr,
		int announce)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();
	IonVdb		*vdb = getIonVdb();
	IonRXref	arg1;
	PsmAddress	rxelt;
	PsmAddress	nextElt;
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

	CHKERR(fromTime);
	if (toTime == 0)			/*	Discovered.	*/
	{
		toTime = MAX_POSIX_TIME;
	}

	CHKERR(toTime > fromTime);
	CHKERR(fromNode);
	CHKERR(toNode);
	CHKERR(rxaddr);
	CHKERR(sdr_begin_xn(sdr));

	/*	Make sure range doesn't overlap with any pre-existing
	 *	ranges.							*/

	*rxaddr = 0;
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
		*rxaddr = sm_rbt_data(ionwm, rxelt);
		rxref = (IonRXref *) psp(ionwm, *rxaddr);
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
			arg2.ref = *rxaddr;
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

			isprintf(rangeIdString, sizeof rangeIdString,
					"from %lu, %lu->%lu", fromTime,
					fromNode, toNode);
			if (rxref->owlt == owlt)
			{
				writeMemoNote("[?] This range is already \
asserted", rangeIdString);
				sdr_exit_xn(sdr);
				return 1;	/*	Idempotent.	*/
			}

			writeMemoNote("[?] Range OWLT not revised",
					rangeIdString);
			sdr_exit_xn(sdr);
			return 2;
		}
	}

	/*	Check for overlap, which is not allowed.		*/

	if (nextElt)
	{
		prevElt = sm_rbt_prev(ionwm, nextElt);
		rxref = (IonRXref *) psp(ionwm, sm_rbt_data(ionwm, nextElt));
		if (fromNode == rxref->fromNode
		&& toNode == rxref->toNode
		&& toTime > rxref->fromTime)
		{
			writeMemoNote("[?] Overlapping end of range for node",
					utoa(fromNode));
			sdr_exit_xn(sdr);
			return 3;
		}
	}
	else
	{
		prevElt = sm_rbt_last(ionwm, vdb->rangeIndex);
	}

	if (prevElt)
	{
		rxref = (IonRXref *) psp(ionwm, sm_rbt_data(ionwm, prevElt));
		if (fromNode == rxref->fromNode
		&& toNode == rxref->toNode
		&& fromTime < rxref->toTime)
		{
			writeMemoNote("[?] Overlapping start of range for node",
					utoa(fromNode));
			sdr_exit_xn(sdr);
			return 4;
		}
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
		iondbObj = getIonDbObject();
		sdr_read(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
		elt = sdr_list_insert_last(sdr, iondb.ranges, obj);
		if (elt)
		{
			arg1.rangeElt = elt;
			*rxaddr = insertRXref(&arg1);
			if (*rxaddr == 0)
			{
				sdr_cancel_xn(sdr);
			}
			else
			{
				if (announce)
				{
					postCpsNotice(0, fromTime, toTime,
						fromNode, toNode, owlt, 1.0);
				}
			}
		}
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't insert range.", NULL);
		return -1;
	}

	return 0;
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
	isprintf(buffer, RFX_NOTE_LEN, "From %20s to %20s the OWLT from \
node " UVAST_FIELDSPEC " to node " UVAST_FIELDSPEC " is %10u seconds.",
			fromTimeBuffer, toTimeBuffer, range->fromNode,
			range->toNode, range->owlt);
	return buffer;
}

static void	deleteRange(PsmAddress rxaddr, int retainIfAsserted)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();
	IonVdb 		*vdb = getIonVdb();
	time_t		currentTime = getCtime();
	IonRXref	*rxref;
	Object		obj;
	IonEvent	event;
	IonNeighbor	*neighbor;
	PsmAddress	nextElt;

	/*	Remove assertion object from database if possible.	*/

	rxref = (IonRXref *) psp(ionwm, rxaddr);
	if (rxref->rangeElt)	/*	Range is asserted, not imputed.	*/
	{
		if (retainIfAsserted)
		{
			return;	/*	Must not delete this range.	*/
		}

		/*	Remove range assertion object from DB.		*/

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
		getCurrentTime(&(vdb->lastEditTime));
	}

	sm_rbt_delete(ionwm, vdb->rangeIndex, rfx_order_ranges, rxref,
			rfx_erase_data, NULL);
}

static void	removeAllRanges(uvast fromNode, uvast toNode, IonRXref *arg,
			int retainIfAsserted, int announce)
{
	PsmPartition	ionwm = getIonwm();
	IonVdb 		*vdb = getIonVdb();
	PsmAddress	rxelt;
	PsmAddress	nextElt;
	PsmAddress	rxaddr;
	IonRXref	*rxref;

	/*	Get first range for this to/from node pair.		*/

	oK(sm_rbt_search(ionwm, vdb->rangeIndex, rfx_order_ranges, arg,
			&rxelt));

	/*	Now delete ranges until a node number changes.		*/

	while (rxelt)
	{
		rxaddr = sm_rbt_data(ionwm, rxelt);
		rxref = (IonRXref *) psp(ionwm, rxaddr);
		if (rxref->fromNode > fromNode
		|| rxref->toNode > toNode)
		{
			break;	/*	No more matches.		*/
		}

		nextElt = sm_rbt_next(ionwm, rxelt); 
		if (announce)
		{
			postCpsNotice(0, rxref->fromTime, 0, fromNode, toNode,
					0, 0.0);
		}

		deleteRange(rxaddr, retainIfAsserted);

		/*	Now reposition at the next range.		*/

		if (nextElt == 0)
		{
			break;	/*	No more ranges.			*/
		}

		rxaddr = sm_rbt_data(ionwm, nextElt);
		rxref = (IonRXref *) psp(ionwm, rxaddr);
		arg->fromNode = rxref->fromNode;
		arg->toNode = rxref->toNode;
		arg->fromTime = rxref->fromTime;
		rxelt = sm_rbt_search(ionwm, vdb->rangeIndex, rfx_order_ranges,
				arg, &nextElt);
	}
}

int	rfx_remove_range(time_t *fromTime, uvast fromNode, uvast toNode,
		int announce)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();
	IonVdb 		*vdb = getIonVdb();
	IonRXref	arg;
	PsmAddress	rxelt;
	PsmAddress	nextElt;
	PsmAddress	rxaddr;

	/*	Note: when the fromTime passed to ionadmin is '*'
	 *	a NULL fromTime is passed to rfx_remove_range,
	 *	where it is interpreted as "all ranges between
	 *	these two nodes".					*/

	memset((char *) &arg, 0, sizeof(IonRXref));
	arg.fromNode = fromNode;
	arg.toNode = toNode;
	CHKERR(sdr_begin_xn(sdr));
	if (fromTime)		/*	Not a wild-card deletion.	*/
	{
		arg.fromTime = *fromTime;
		rxelt = sm_rbt_search(ionwm, vdb->rangeIndex, rfx_order_ranges,
				&arg, &nextElt);
		if (rxelt)	/*	Found it.			*/
		{
			rxaddr = sm_rbt_data(ionwm, rxelt);
			if (announce)
			{
				postCpsNotice(0, *fromTime, 0, fromNode, toNode,
						0, 0.0);
			}

			deleteRange(rxaddr, 0);
		}
	}
	else		/*	Wild-card deletion, start at time zero.	*/
	{
		removeAllRanges(fromNode, toNode, &arg, 0, announce);
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
		arg.fromTime = *fromTime;
		rxelt = sm_rbt_search(ionwm, vdb->rangeIndex, rfx_order_ranges,
				&arg, &nextElt);
		if (rxelt)	/*	Found it.			*/
		{
			rxaddr = sm_rbt_data(ionwm, rxelt);
			if (announce)
			{
				postCpsNotice(0, *fromTime, 0, fromNode,
						toNode, 0, 0.0);
			}

			deleteRange(rxaddr, 1);
		}
	}
	else		/*	Wild-card deletion, start at time zero.	*/
	{
		removeAllRanges(fromNode, toNode, &arg, 1, announce);
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't remove range(s).", NULL);
		return -1;
	}

	return 0;
}

void	rfx_brief_ranges()
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();
	IonVdb		*vdb = getIonVdb();
	int		briefingFile;
	PsmAddress	elt;
	PsmAddress	addr;
	IonRXref	*range;
	char		fromTimeBuffer[TIMESTAMPBUFSZ];
	char		toTimeBuffer[TIMESTAMPBUFSZ];
	char		buffer[256];
	int		textLen;

	briefingFile = iopen("ranges.ionrc", O_WRONLY | O_CREAT, 0666);
	if (briefingFile == -1)
	{
		putSysErrmsg("Can't create ranges.ionrc file", NULL);
		return;
	}

	if (sdr_begin_xn(sdr) < 0)
	{
		close(briefingFile);
		putErrmsg("Can't start transaction", NULL);
		return;
	}

	for (elt = sm_rbt_first(ionwm, vdb->rangeIndex); elt;
			elt = sm_rbt_next(ionwm, elt))
	{
		addr = sm_rbt_data(ionwm, elt);
		range = (IonRXref *) psp(ionwm, addr);
		writeTimestampUTC(range->fromTime, fromTimeBuffer);
		writeTimestampUTC(range->toTime, toTimeBuffer);
		isprintf(buffer, sizeof buffer, "a range %20s %20s "
UVAST_FIELDSPEC " " UVAST_FIELDSPEC " %u\n", fromTimeBuffer, toTimeBuffer,
				range->fromNode, range->toNode, range->owlt);
       		textLen = strlen(buffer);
		if (write(briefingFile, buffer, textLen) < 0)
		{
			putSysErrmsg("Can't write to ranges.ionrc file",
				       NULL);
			break;
		}
	}

	sdr_exit_xn(sdr);
	close(briefingFile);
	return;
}

/*	*	RFX alarm management functions	*	*	*	*/

extern PsmAddress	rfx_insert_alarm(unsigned int term, unsigned int cycles)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();
	IonVdb 		*vdb = getIonVdb();
	time_t		currentTime = getCtime();
	PsmAddress	alarmAddr;
	IonAlarm	*alarm;
	PsmAddress	eventAddr;
	IonEvent	*event;

	CHKZERO(term);

	/*	Construct the alarm control structure.			*/

	CHKERR(sdr_begin_xn(sdr));	/*	To lock memory.		*/
	alarmAddr = psm_zalloc(ionwm, sizeof(IonAlarm));
	if (alarmAddr == 0)
	{
		sdr_exit_xn(sdr);
		return 0;
	}

	alarm = (IonAlarm *) psp(ionwm, alarmAddr);
	alarm->term = term;
	alarm->cycles = cycles;
	alarm->semaphore = sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO);
	alarm->nextTimeout = currentTime + alarm->term;

	/*	Construct the timeout event referencing this structure.	*/

	eventAddr = psm_zalloc(ionwm, sizeof(IonEvent));
	if (eventAddr == 0)
	{
		sm_SemEnd(alarm->semaphore);
		microsnooze(50000);
		sm_SemDelete(alarm->semaphore);
		psm_free(ionwm, alarmAddr);
		sdr_exit_xn(sdr);
		return 0;
	}

	event = (IonEvent *) psp(ionwm, eventAddr);
	event->time = alarm->nextTimeout;
	event->type = IonAlarmTimeout;
	event->ref = alarmAddr;
	if (sm_rbt_insert(ionwm, vdb->timeline, eventAddr, rfx_order_events,
			event) == 0)
	{
		psm_free(ionwm, eventAddr);
		sm_SemEnd(alarm->semaphore);
		microsnooze(50000);
		sm_SemDelete(alarm->semaphore);
		psm_free(ionwm, alarmAddr);
		sdr_exit_xn(sdr);
		return 0;
	}

	sdr_exit_xn(sdr);
	return alarmAddr;
}

extern int	rfx_alarm_raised(PsmAddress alarmAddr)
{
	IonAlarm	*alarm;
	sm_SemId	semaphore;

	if (alarmAddr == 0)
	{
		return 0;	/*	Can't wait for this alarm.	*/
	}

	alarm = (IonAlarm *) psp(getIonwm(), alarmAddr);
	semaphore = alarm->semaphore;

	/*	We save the value of semaphore in case the alarm
	 *	itself is destroyed while we're waiting.		*/

	if (semaphore == SM_SEM_NONE
	|| sm_SemTake(semaphore) < 0
	|| sm_SemEnded(semaphore))
	{
		return 0;	/*	Alarm is dead.			*/
	}

	return 1;
}

extern int	rfx_remove_alarm(PsmAddress alarmAddr)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();
	IonVdb 		*vdb = getIonVdb();
	IonAlarm	*alarm;
	IonEvent	event;

	if (alarmAddr == 0)
	{
		return 0;	/*	Nothing to do.			*/
	}

	alarm = (IonAlarm *) psp(ionwm, alarmAddr);

	/*	First disable the alarm.				*/

	sm_SemEnd(alarm->semaphore);	/*	Tell thread to stop.	*/
	sm_TaskYield();			/*	Give thread time.	*/

	/*	Now remove the alarm.					*/

	event.time = alarm->nextTimeout;
	event.type = IonAlarmTimeout;
	event.ref = alarmAddr;		/*	Needed for search.	*/
	CHKERR(sdr_begin_xn(sdr));	/*	To lock memory.		*/
	sm_SemEnd(alarm->semaphore);
	microsnooze(50000);
	sm_SemDelete(alarm->semaphore);
	psm_free(ionwm, alarmAddr);	/*	Alarm no longer exists.	*/
	sm_rbt_delete(ionwm, vdb->timeline, rfx_order_events, &event,
			rfx_erase_data, NULL);
	sdr_exit_xn(sdr);
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
	if (insertRXref(&rxref) == 0)
	{
		return -1;
	}

	return 0;
}

static int	loadContact(Object elt, uint32_t regionNbr)
{
	Sdr		sdr = getIonsdr();
	Object		obj;
	IonContact	contact;
	IonCXref	cxref;

	obj = sdr_list_data(sdr, elt);
	sdr_read(sdr, (char *) &contact, obj, sizeof(IonContact));

	/*	Load contact index entry.				*/

	memset((char *) &cxref, 0, sizeof(IonCXref));
	cxref.regionNbr = regionNbr;
	cxref.fromNode = contact.fromNode;
	cxref.toNode = contact.toNode;
	cxref.fromTime = contact.fromTime;
	cxref.toTime = contact.toTime;
	cxref.xmitRate = contact.xmitRate;
	cxref.confidence = contact.confidence;
	cxref.type = contact.type;
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
	Sdr		sdr = getIonsdr();
	IonVdb		*vdb = getIonVdb();
	Object		iondbObj;
	IonDB		iondb;
	int		i;
	uint32_t	regionNbr;
	Object		elt;

	iondbObj = getIonDbObject();
	CHKERR(sdr_begin_xn(sdr));	/*	To lock memory.		*/
	sdr_read(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));

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
	for (i = 0; i < 2; i++)
	{
		if (iondb.regions[i].contacts == 0)
		{
			continue;
		}

		regionNbr = iondb.regions[i].regionNbr;
		for (elt = sdr_list_first(sdr, iondb.regions[i].contacts); elt;
				elt = sdr_list_next(sdr, elt))
		{
			if (loadContact(elt, regionNbr) < 0)
			{
				putErrmsg("Can't load contact.", NULL);
				sdr_exit_xn(sdr);
				return -1;
			}
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
	int		i;
	PsmAddress	elt;
	PsmAddress	nextElt;
	PsmAddress	addr;
	Requisition	*req;

	/*	Safely shut down the ZCO flow control system.		*/

	for (i = 0; i < 1; i++)
	{
		for (elt = sm_list_first(ionwm, vdb->requisitions[i]); elt;
				elt = nextElt)
		{
			nextElt = sm_list_next(ionwm, elt);
			addr = sm_list_data(ionwm, elt);
			req = (Requisition *) psp(ionwm, addr);
			if (req->semaphore != SM_SEM_NONE)
			{
				sm_SemEnd(req->semaphore);
			}

			psm_free(ionwm, addr);
			sm_list_delete(ionwm, elt, NULL, NULL);
		}
	}

	zco_unregister_callback();

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
}
