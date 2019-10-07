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
	time_t		currentTime = getCtime();

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

	if (cxref->type != CtRegistration && cxref->toTime > currentTime)
	{
		/*	Affects routes.					*/

		getCurrentTime(&(vdb->lastEditTime));
	}

	return cxaddr;
}

static void	insertContact(int regionIdx, time_t fromTime, time_t toTime,
		uvast fromNode, uvast toNode, size_t xmitRate, float confidence,
		ContactType contactType, PsmAddress *cxaddr)
{
	Sdr		sdr = getIonsdr();
	IonContact	contact;
	double		volume;
	Object		obj;
	Object		iondbObj;
	IonDB		iondb;
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
	iondbObj = getIonDbObject();
	sdr_read(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
	elt = sdr_list_insert_last(sdr,
			iondb.regions[regionIdx].contacts, obj);
	if (elt == 0)
	{
		return;
	}

	memset((char *) &newCx, 0, sizeof(IonCXref));
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
	if (contactType == CtPredicted)
	{
		return;
	}

	/*	Registration or Hypothetical or Scheduled, so make
	 *	sure both From and To nodes are in the list of members
	 *	of this region (which may be either the home or
	 *	outer region of the local node).  Assume they are
	 *	not passageways, i.e., the region in which they make
	 *	this contact is their home region; if not, this can
	 *	be corrected administratively via ionadmin.		*/

	ionNoteMember(regionIdx, fromNode,
			iondb.regions[regionIdx].regionNbr, -1);
	ionNoteMember(regionIdx, toNode,
			iondb.regions[regionIdx].regionNbr, -1);
}

int	rfx_insert_contact(int regionIdx, time_t fromTime, time_t toTime,
		uvast fromNode, uvast toNode, size_t xmitRate, float confidence,
		PsmAddress *cxaddr)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();
	IonVdb 		*vdb = getIonVdb();
	uvast		ownNodeNbr = getOwnNodeNbr();
	ContactType	contactType;
	IonCXref	arg;
	PsmAddress	cxelt;
	PsmAddress	nextElt;
	IonCXref	*cxref;
	char		buf1[TIMESTAMPBUFSZ];
	char		buf2[TIMESTAMPBUFSZ];
	char		contactIdString[128];

	CHKERR(cxaddr);
	*cxaddr = 0;			/*	Default.		*/
	if (regionIdx < 0 || regionIdx > 1)
	{
		writeMemo("[?] Can't insert contact, nodes not in any \
common region.");
		return 0;
	}

	if (fromNode == 0)
	{
		writeMemo("[?] Can't insert contact From node 0.");
		return 0;
	}

	if (toNode == 0)
	{
		writeMemo("[?] Can't insert contact To node 0.");
		return 0;
	}

	if (confidence < 0.0 || confidence > 1.0)
	{
		writeMemo("[?] Can't insert contact: confidence must be \
between 0.0 and 1.0.");
		return 0;
	}

	/*	Note: Discovered contacts are ONLY created by
	 *	tranformation from Hypothetical contacts, and
	 *	Suppressed contacts are ONLY created by transformation
	 *	from Scheduled contacts.  So this new contact must
	 *	be Registration, Hypothetical, Predicted, or Scheduled.	*/

	if (fromTime == (time_t) -1)	/*	Registration.		*/
	{
		CHKZERO(fromNode == toNode);
		fromTime = MAX_POSIX_TIME;
		toTime = MAX_POSIX_TIME;
		xmitRate = 0;
		confidence = 1.0;
		contactType = CtRegistration;
	}
	else if (fromTime == 0)		/*	Hypothetical.		*/
	{
		CHKZERO(fromNode != toNode);
		CHKZERO(fromNode == ownNodeNbr || toNode == ownNodeNbr);
		fromTime = 0;
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
			return 0;
		}

		if (fromTime == toTime)	/*	Predicted.		*/
		{
			CHKZERO(fromNode != ownNodeNbr && toNode != ownNodeNbr);
			fromTime = getCtime();	/*	Now.	*/
			contactType = CtPredicted;
		}
		else			/*	Scheduled.		*/
		{
			if (toTime < fromTime)
			{
				writeMemo("[?] Can't insert contact, To time \
must be later than From time.");
				return 0;
			}

			/*	No artificial parameter values,
			 *	nothing to override.			*/

			contactType = CtScheduled;
		}
	}

	*cxaddr = 0;	/*	Default.				*/
	memset((char *) &arg, 0, sizeof(IonCXref));
	arg.fromNode = fromNode;
	arg.toNode = toNode;
	CHKERR(sdr_begin_xn(sdr));

	/*	Insert registration contact, if applicable.		*/

	if (contactType == CtRegistration)
	{
		arg.fromTime = fromTime;	/*	MAX_POSIX_TIME	*/
		cxelt = sm_rbt_search(ionwm, vdb->contactIndex,
				rfx_order_contacts, &arg, &nextElt);
		if (cxelt)	/*	Registration contact exists.	*/
		{
			writeMemo("[?] Won't insert redundant registration \
contact.");
			sdr_exit_xn(sdr);
			return 0;	/*	Do not insert.		*/
		}

		/*	No registration contact, okay to add one.	*/

		insertContact(regionIdx, fromTime, toTime, fromNode, toNode,
				xmitRate, confidence, contactType, cxaddr);
		return sdr_end_xn(sdr);
	}

	/*	New contact is either Hypothetical or Scheduled or
	 *	Predicted.						*/

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
		 *	If the new contact is Hypothetical, then we must
		 *	reject it if an existing Hypothetical contact
		 *	is now Discovered, and we must be sure to remove
		 *	any existing Hypothetical contact.  As soon as
		 *	we are past all contacts with fromTime zero we
		 *	break out of the loop and insert the new contact.
		 *
		 *	If instead the new contact is Scheduled or
		 *	Predicted, then we must reject it if its time
		 *	interval overlaps with any Scheduled or
		 *	Suppressed contact, and we must be sure to
		 *	remove any existing Predicted contact with
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
				/*	If contact's fromTime is 0,
				 *	the contact must be Hypothetical,
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
					return 0;

				case CtHypothetical:
					/*	Replace this one.	*/

					if (rfx_remove_contact(&cxref->fromTime,
							cxref->fromNode,
							cxref->toNode) < 0)
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

			if (rfx_remove_contact(&cxref->fromTime,
					cxref->fromNode, cxref->toNode) < 0)
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
		return sdr_end_xn(sdr);
	}

	/*	Contact isn't already in database; okay to add.		*/

	insertContact(regionIdx, fromTime, toTime, fromNode, toNode, xmitRate,
			confidence, contactType, cxaddr);
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

int	rfx_revise_contact(time_t fromTime, uvast fromNode, uvast toNode,
		size_t xmitRate, float confidence)
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
		return 0;
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
		return 0;
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
	 *	no need to recompute routes.				*/

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't revise contact.", NULL);
		return -1;
	}

	return 0;
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

	/*	Delete all affected routes.				*/

	/*	Apply to current state of affected neighbor, if any.	*/

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

static void	removeAllContacts(uvast fromNode, uvast toNode, IonCXref *arg)
{
	PsmPartition	ionwm = getIonwm();
	IonVdb 		*vdb = getIonVdb();
	PsmAddress	cxelt;
	PsmAddress	nextElt;
	PsmAddress	cxaddr;
	IonCXref	*cxref;

	/*	Get first contact for this to/from node pair.		*/

	oK(sm_rbt_search(ionwm, vdb->contactIndex, rfx_order_contacts, arg,
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
		deleteContact(cxaddr);

		/*	Now reposition at the next contact.		*/

		if (nextElt == 0)
		{
			break;	/*	No more contacts.		*/
		}

		cxaddr = sm_rbt_data(ionwm, nextElt);
		cxref = (IonCXref *) psp(ionwm, cxaddr);
		arg->fromNode = cxref->fromNode;
		arg->toNode = cxref->toNode;
		arg->fromTime = cxref->fromTime;
		cxelt = sm_rbt_search(ionwm, vdb->contactIndex,
				rfx_order_contacts, arg, &nextElt);
	}
}

int	rfx_remove_contact(time_t *fromTime, uvast fromNode, uvast toNode)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();
	IonVdb 		*vdb = getIonVdb();
	IonCXref	arg;
	PsmAddress	cxelt;
	PsmAddress	nextElt;
	PsmAddress	cxaddr;

	/*	Note: when the fromTime passed to ionadmin is '*'
	 *	a NULL fromTime is passed to rfx_remove_contact,
	 *	where it is interpreted as "all contacts between
	 *	these two nodes".					*/

	memset((char *) &arg, 0, sizeof(IonCXref));
	arg.fromNode = fromNode;
	arg.toNode = toNode;
	CHKERR(sdr_begin_xn(sdr));
	if (fromTime)		/*	Not a wild-card deletion.	*/
	{
		arg.fromTime = *fromTime;
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
		removeAllContacts(fromNode, toNode, &arg);
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't remove contact(s).", NULL);
		return -1;
	}

	return 0;
}

void	rfx_contact_state(uvast nodeNbr, size_t *secRemaining, size_t *xmitRate)
{
	PsmPartition	ionwm = getIonwm();
	IonVdb		*ionvdb = getIonVdb();
	time_t		currentTime = getCtime();
	IonCXref	arg;
	PsmAddress	elt;
	IonCXref	*contact;
	int		candidateContacts = 0;

	memset((char *) &arg, 0, sizeof(IonCXref));
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

		candidateContacts++;
		if (contact->toNode != nodeNbr)
		{
			continue;	/*	Wrong node.		*/
		}

		if (contact->confidence < 1.0)
		{
			continue;	/*	Not current contact.	*/
		}

		if (contact->toTime <= currentTime)
		{
			continue;	/*	Contact already ended.	*/
		}

		if (contact->fromTime > currentTime)
		{
			break;		/*	Not current contact.	*/
		}

		*secRemaining = contact->toTime - currentTime;
		*xmitRate = contact->xmitRate;
		return;
	}

	/*	No current contact.					*/

	*secRemaining = 0;
	if (candidateContacts == 0)	/*	Contact plan n/a.	*/
	{
		*xmitRate = ((size_t) -1);
	}
	else
	{
		*xmitRate = 0;		/*	No transmission now.	*/
	}
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
	time_t		currentTime = getCtime();

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
		uvast toNode, unsigned int owlt, PsmAddress *rxaddr)
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

			if (rxref->owlt == owlt)
			{
				sdr_exit_xn(sdr);
				return 0;	/*	Idempotent.	*/
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
		rxref = (IonRXref *) psp(ionwm, sm_rbt_data(ionwm, nextElt));
		if (fromNode == rxref->fromNode
		&& toNode == rxref->toNode
		&& toTime > rxref->fromTime)
		{
			writeMemoNote("[?] Overlapping range for node",
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
		rxref = (IonRXref *) psp(ionwm, sm_rbt_data(ionwm, prevElt));
		if (fromNode == rxref->fromNode
		&& toNode == rxref->toNode
		&& fromTime < rxref->toTime)
		{
			writeMemoNote("[?] Overlapping range for node",
					utoa(fromNode));
			sdr_exit_xn(sdr);
			return 0;
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
			int retainIfAsserted)
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

int	rfx_remove_range(time_t *fromTime, uvast fromNode, uvast toNode)
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
			deleteRange(rxaddr, 0);
		}
	}
	else		/*	Wild-card deletion, start at time zero.	*/
	{
		removeAllRanges(fromNode, toNode, &arg, 0);
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
			deleteRange(rxaddr, 1);
		}
	}
	else		/*	Wild-card deletion, start at time zero.	*/
	{
		removeAllRanges(fromNode, toNode, &arg, 1);
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't remove range(s).", NULL);
		return -1;
	}

	return 0;
}

/*	*	RFX alarm management functions	*	*	*	*/

extern PsmAddress	rfx_insert_alarm(unsigned int term,
				unsigned int cycles)
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

	memset((char *) &cxref, 0, sizeof(IonCXref));
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
	PsmPartition	ionwm = getIonwm();
	Sdr		sdr = getIonsdr();
	IonVdb		*vdb = getIonVdb();
	Object		iondbObj;
	IonDB		iondb;
	int		i;
	Object		elt;

	iondbObj = getIonDbObject();
	CHKERR(sdr_begin_xn(sdr));	/*	To lock memory.		*/
	sdr_read(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));

	/*	Destroy and re-create volatile contact and range
	 *	databases.  This prevents contact/range duplication
	 *	as a result of adds before starting ION.		*/

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
	for (i = 0; i < 2; i++)
	{
		for (elt = sdr_list_first(sdr, iondb.regions[i].contacts); elt;
				elt = sdr_list_next(sdr, elt))
		{
			if (loadContact(elt) < 0)
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

	/*	Wipe out all red-black trees involved in routing,
	 *	for reconstruction on restart.				*/

	sm_rbt_destroy(ionwm, vdb->contactIndex, rfx_erase_data, NULL);
	sm_rbt_destroy(ionwm, vdb->rangeIndex, rfx_erase_data, NULL);
	sm_rbt_destroy(ionwm, vdb->timeline, rfx_erase_data, NULL);
	vdb->contactIndex = sm_rbt_create(ionwm);
	vdb->rangeIndex = sm_rbt_create(ionwm);
	vdb->timeline = sm_rbt_create(ionwm);
}
