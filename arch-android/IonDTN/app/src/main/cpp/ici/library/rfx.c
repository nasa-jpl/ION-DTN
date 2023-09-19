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

#ifndef MAX_CONTACT_LOG_LENGTH
#define MAX_CONTACT_LOG_LENGTH	(8)
#endif
#define CONFIDENCE_BASIS	(MAX_CONTACT_LOG_LENGTH * 2.0)

static int	removePredictedContacts(uvast fromNode, uvast toNode);

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
	probe->time = getUTCTime();
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
		getCurrentTime(&(vdb->lastEditTime));
	}

	return cxaddr;
}

int	rfx_insert_contact(time_t fromTime, time_t toTime,
			uvast fromNode, uvast toNode, size_t xmitRate,
			float confidence, PsmAddress *cxaddr)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();
	IonVdb 		*vdb = getIonVdb();
	int		discovered = 0;
	IonCXref	newCx;
	IonCXref	arg;
	PsmAddress	cxelt;
	PsmAddress	nextElt;
	IonCXref	*cxref;
	char		buf1[TIMESTAMPBUFSZ];
	char		buf2[TIMESTAMPBUFSZ];
	char		contactIdString[128];
	IonContact	contact;
	Object		iondbObj;
	IonDB		iondb;
	Object		obj;
	Object		elt;

	CHKERR(fromTime);
	if (toTime == 0)
	{
		discovered = 1;
		toTime = MAX_POSIX_TIME;
	}

	CHKERR(toTime > fromTime);
	CHKERR(fromNode);
	CHKERR(toNode);
	CHKERR(confidence > 0.0 && confidence <= 1.0);
	CHKERR(cxaddr);
	CHKERR(sdr_begin_xn(sdr));
	*cxaddr = 0;	/*	Default.				*/

	/*	Navigate to the first contact for this node pair.	*/

	memset((char *) &arg, 0, sizeof(IonCXref));
	arg.fromNode = fromNode;
	arg.toNode = toNode;
	cxelt = sm_rbt_search(ionwm, vdb->contactIndex, rfx_order_contacts,
			&arg, &nextElt);
	if (cxelt)	/*	A contact with start time zero exists.	*/
	{
		putErrmsg("Contact with start time 0 exists from node",
				itoa(fromNode));
		writeMemoNote("...to node", itoa(toNode));
		sdr_exit_xn(sdr);
		return 0;	/*	Do not insert.			*/
	}

	cxelt = nextElt;	/*	First contact for node pair.	*/

	/*	Now look for overlapping contacts.			*/

	while (cxelt)
	{
		*cxaddr = sm_rbt_data(ionwm, cxelt);
		cxref = (IonCXref *) psp(ionwm, *cxaddr);
		if (cxref->fromNode != fromNode
		|| cxref->toNode != toNode
		|| cxref->fromTime > toTime)	/*	Future contact.	*/
		{
			break;	/*	No more possible overlaps.	*/
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

		if (confidence < 1.0)
		{
			/*	The new contact is a predicted contact.
			 *	It can't override any existing contact.	*/

			sdr_exit_xn(sdr);
			return 0;	/*	Do not insert.		*/
		}

		if (cxref->confidence < 1.0)
		{
			/*	The new contact overlaps with a
			 *	predicted contact.  The predicted
			 *	contact is overridden.			*/

			if (rfx_remove_contact(cxref->fromTime, cxref->fromNode,
					cxref->toNode) < 0)
			{
				sdr_cancel_xn(sdr);
				return -1;
			}

			/*	Removing a contact reshuffles the
			 *	contact RBT, so must reposition
			 *	within the table.  Start again with
			 *	the first remaining contact for this
			 *	node pair.				*/

			cxelt = sm_rbt_search(ionwm, vdb->contactIndex,
					rfx_order_contacts, &arg, &nextElt);
			cxelt = nextElt;
			continue;
		}

		/*	Overlapping non-predicted contacts.		*/

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

	contact.fromTime = fromTime;
	contact.toTime = toTime;
	contact.fromNode = fromNode;
	contact.toNode = toNode;
	contact.xmitRate = xmitRate;
	contact.confidence = confidence;
	contact.discovered = discovered;
	obj = sdr_malloc(sdr, sizeof(IonContact));
	if (obj)
	{
		sdr_write(sdr, obj, (char *) &contact, sizeof(IonContact));
		iondbObj = getIonDbObject();
		sdr_read(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
		elt = sdr_list_insert_last(sdr, iondb.contacts, obj);
		if (elt)
		{
			memset((char *) &newCx, 0, sizeof(IonCXref));
			newCx.fromTime = fromTime;
			newCx.toTime = toTime;
			newCx.fromNode = fromNode;
			newCx.toNode = toNode;
			newCx.xmitRate = xmitRate;
			newCx.confidence = confidence;
			newCx.discovered = discovered;
			newCx.routingObject = 0;
			newCx.contactElt = elt;
			*cxaddr = insertCXref(&newCx);
			if (*cxaddr == 0)
			{
				sdr_cancel_xn(sdr);
			}
		}
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't insert contact.", NULL);
		return -1;
	}

	return 0;
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

static void	insertLogEntry(Sdr sdr, Object log, Object entryObj,
			PastContact *newEntry)
{
	Object		elt;
	PastContact	entry;

	for (elt = sdr_list_first(sdr, log); elt; elt = sdr_list_next(sdr, elt))
	{
		sdr_read(sdr, (char *) &entry, sdr_list_data(sdr, elt),
				sizeof(PastContact));
		if (entry.fromNode < newEntry->fromNode)
		{
			continue;
		}

		if (entry.fromNode > newEntry->fromNode)
		{
			break;
		}

		/*	Same sending node.				*/

		if (entry.toNode < newEntry->toNode)
		{
			continue;
		}

		if (entry.toNode > newEntry->toNode)
		{
			break;
		}

		/*	Same receiving node.				*/

		if (entry.fromTime < newEntry->fromTime)
		{
			continue;
		}

		if (entry.fromTime > newEntry->fromTime)
		{
			break;
		}

		/*	Identical; duplicate, so don't add to log.	*/

puts("Duplicate log entry is rejected.");
		sdr_free(sdr, entryObj);
		return;
	}

	/*	Okay to insert the new contact history entry.		*/

	if (elt)
	{
		oK(sdr_list_insert_before(sdr, elt, entryObj));
	}
	else
	{
		oK(sdr_list_insert_last(sdr, log, entryObj));
	}

	/*	If length of contact history now exceeds limit,
	 *	delete the earliest entry.				*/

	if (sdr_list_length(sdr, log) > MAX_CONTACT_LOG_LENGTH)
	{
		elt = sdr_list_first(sdr, log);
		entryObj = sdr_list_data(sdr, elt);
		sdr_free(sdr, entryObj);
		sdr_list_delete(sdr, elt, NULL, NULL);
	}
}

void	rfx_log_discovered_contact(time_t fromTime, time_t toTime,
		uvast fromNode, uvast toNode, size_t xmitRate, int idx)
{
	Sdr		sdr = getIonsdr();
	Object		dbobj = getIonDbObject();
	IonDB 		db;
	Object		log;
	PastContact	entry;
	Object		entryObj;
char	buf1[64];
char	buf2[64];

writeTimestampLocal(fromTime, buf1);
writeTimestampLocal(toTime, buf2);
printf("Inserting new entry into discovered contact log, from "
UVAST_FIELDSPEC " to " UVAST_FIELDSPEC ", start %s, stop %s.\n",
fromNode, toNode, buf1, buf2);
	sdr_read(sdr, (char *) &db, dbobj, sizeof(IonDB));
	log = db.contactLog[idx];
	entry.fromTime = fromTime;
	entry.toTime = toTime;
	entry.fromNode = fromNode;
	entry.toNode = toNode;
	entry.xmitRate = xmitRate;
	entryObj = sdr_malloc(sdr, sizeof(PastContact));
	if (entryObj)
	{
		sdr_write(sdr, entryObj, (char *) &entry, sizeof(PastContact));
		insertLogEntry(sdr, log, entryObj, &entry);
	}
}

static void	deleteContact(PsmAddress cxaddr)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();
	IonVdb 		*vdb = getIonVdb();
	time_t		currentTime = getUTCTime();
	uvast		ownNodeNbr = getOwnNodeNbr();
	IonCXref	*cxref;
	int		predictionsNeeded = 0;
	uvast		fromNode = 0;
	uvast		toNode = 0;
	Object		obj;
	IonEvent	event;
	IonNeighbor	*neighbor;
	PsmAddress	nextElt;

	cxref = (IonCXref *) psp(ionwm, cxaddr);

	/*	Possibly write to contact log.				*/

	if (cxref->discovered)
	{
		if (cxref->fromNode == ownNodeNbr)
		{
			rfx_log_discovered_contact(cxref->fromTime, currentTime,
					cxref->fromNode, cxref->toNode,
					cxref->xmitRate, SENDER_NODE);
		}
		else if (cxref->toNode == ownNodeNbr)
		{
			rfx_log_discovered_contact(cxref->fromTime, currentTime,
					cxref->fromNode, cxref->toNode,
					cxref->xmitRate, RECEIVER_NODE);
		}

		/*	In any case, rebuild list of predicted
		 *	contacts now that the discovered contact
		 *	has ended.					*/

		predictionsNeeded = 1;
		fromNode = cxref->fromNode;
		toNode = cxref->toNode;
	}

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
		getCurrentTime(&(vdb->lastEditTime));
	}

	sm_rbt_delete(ionwm, vdb->contactIndex, rfx_order_contacts, cxref,
			rfx_erase_data, NULL);

	/*	Finally, compute new predicted contacts if necessary.	*/

	if (predictionsNeeded)
	{
		rfx_predict_contacts(fromNode, toNode);
	}
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

int	rfx_remove_discovered_contacts(uvast peerNode)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();
	IonVdb 		*vdb = getIonVdb();
	uvast		self = getOwnNodeNbr();
	IonDB		iondb;
	Object		obj;
	Object		elt;
	Object		nextElt;
	IonContact	contact;
	IonCXref	arg;
	PsmAddress	cxelt;
	PsmAddress	nextCxelt;
	PsmAddress	cxaddr;

puts("In rfx_remove_discovered_contacts....");
	CHKERR(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &iondb, getIonDbObject(), sizeof(IonDB));
	for (elt = sdr_list_first(sdr, iondb.contacts); elt; elt = nextElt)
	{
		nextElt = sdr_list_next(sdr, elt);
		obj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &contact, obj, sizeof(IonContact));
		if (contact.discovered == 0)
		{
puts("(contact not discovered)");
			continue;	/*	Not discovered.		*/
		}

		/*	This is a discovered (non-predicted) contact.	*/

		if (!((contact.fromNode == peerNode && contact.toNode == self)
		|| (contact.toNode == peerNode && contact.fromNode == self)))
		{
puts("(contact not affected)");
			continue;	/*	Contact not affected.	*/
		}

		/*	This is our local discovered contact to or from
		 *	the indicated node.				*/

		memset((char *) &arg, 0, sizeof(IonCXref));
		arg.fromNode = contact.fromNode;
		arg.toNode = contact.toNode;
		arg.fromTime = contact.fromTime;
		cxelt = sm_rbt_search(ionwm, vdb->contactIndex,
				rfx_order_contacts, &arg, &nextCxelt);
		if (cxelt)	/*	Found it.			*/
		{
			cxaddr = sm_rbt_data(ionwm, cxelt);
			deleteContact(cxaddr);
		}
else puts("(contact not found in index)");
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't remove discovered contacts.", NULL);
		return -1;
	}

	return 0;
}

void	rfx_contact_state(uvast nodeNbr, size_t *secRemaining, size_t *xmitRate)
{
	PsmPartition	ionwm = getIonwm();
	IonVdb		*ionvdb = getIonVdb();
	time_t		currentTime = getUTCTime();
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
			break;		/*	No current contact.	*/
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

/*	*	RFX contact prediction functions	*	*	*/

#define	LOW_BASE_CONFIDENCE	(.05)
#define	HIGH_BASE_CONFIDENCE	(.20)

typedef struct
{
	uvast		duration;
	uvast		capacity;
	uvast		fromNode;
	uvast		toNode;
	time_t		fromTime;
	time_t		toTime;
	size_t		xmitRate;
} PbContact;

static int	removePredictedContacts(uvast fromNode, uvast toNode)
{
	Sdr		sdr = getIonsdr();
	IonDB		iondb;
	Object		obj;
	Object		elt;
	Object		nextElt;
	IonContact	contact;

	CHKERR(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &iondb, getIonDbObject(), sizeof(IonDB));
	for (elt = sdr_list_first(sdr, iondb.contacts); elt; elt = nextElt)
	{
		nextElt = sdr_list_next(sdr, elt);
		obj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &contact, obj, sizeof(IonContact));
		if (contact.confidence == 1.0)
		{
			continue;	/*	Managed or discovered.	*/
		}

		/*	This is a predicted contact.			*/

		if (fromNode)		/*	Selective removal.	*/
		{
			if (contact.fromNode != fromNode
			|| contact.toNode != toNode)
			{
				continue;	/*	N/A		*/
			}
		}

		if (rfx_remove_contact(contact.fromTime, contact.fromNode,
				contact.toNode) < 0)
		{
			putErrmsg("Failure in rfx_remove_contact.", NULL);
			break;
		}
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't remove predicted contacts.", NULL);
		return -1;
	}

	return 0;
}

static void	freePbContents(LystElt elt, void *userdata)
{
	MRELEASE(lyst_data(elt));
}

static int	insertIntoPredictionBase(Lyst pb, PastContact *logEntry)
{
	vast		duration;
	LystElt		elt;
	PbContact	*contact;
char	buf1[64];
char	buf2[64];

writeTimestampLocal(logEntry->fromTime, buf1);
writeTimestampLocal(logEntry->toTime, buf2);
printf("Inserting log entry into prediction base, contact from "
UVAST_FIELDSPEC " to " UVAST_FIELDSPEC ", start %s, stop %s.\n",
logEntry->fromNode, logEntry->toNode, buf1, buf2);
	duration = logEntry->toTime - logEntry->fromTime;
	if (duration <= 0 || logEntry->xmitRate == 0)
	{
		return 0;	/*	Useless contact.		*/
	}

	for (elt = lyst_first(pb); elt; elt = lyst_next(elt))
	{
		contact = (PbContact *) lyst_data(elt);
		if (contact->fromNode < logEntry->fromNode)
		{
			continue;
		}

		if (contact->fromNode > logEntry->fromNode)
		{
			break;
		}

		if (contact->toNode < logEntry->toNode)
		{
			continue;
		}

		if (contact->toNode > logEntry->toNode)
		{
			break;
		}

		if (contact->toTime < logEntry->fromTime)
		{
			/*	Ends before start of log entry.		*/

			continue;
		}

		if (contact->fromTime > logEntry->toTime)
		{
			/*	Starts after end of log entry.		*/

			break;
		}

		/*	This previously inserted contact starts
		 *	before the log entry ends and ends after
		 *	the log entry starts, so the log entry
		 *	overlaps with it and can't be inserted.		*/

		return 0;
	}

	contact = MTAKE(sizeof(PbContact));
	if (contact == NULL)
	{
		putErrmsg("No memory for prediction base contact.", NULL);
		return -1;
	}

	contact->duration = duration;
	contact->capacity = duration * logEntry->xmitRate;
	contact->fromNode = logEntry->fromNode;
	contact->toNode = logEntry->toNode;
	contact->fromTime = logEntry->fromTime;
	contact->toTime = logEntry->toTime;
	contact->xmitRate = logEntry->xmitRate;
	if (elt)
	{
		elt = lyst_insert_before(elt, contact);
	}
	else
	{
		elt = lyst_insert_last(pb, contact);
	}

	if (elt == NULL)
	{
		putErrmsg("No memory for prediction base list element.", NULL);
		return -1;
	}

	return 0;
}

static Lyst	constructPredictionBase(uvast fromNode, uvast toNode)
{
	Sdr		sdr = getIonsdr();
	Lyst		pb;
	IonDB		iondb;
	int		i;
	Object		elt;
	PastContact	logEntry;

printf("Building prediction base for contacts from " UVAST_FIELDSPEC " to "
UVAST_FIELDSPEC ".\n", fromNode, toNode);
	pb = lyst_create_using(getIonMemoryMgr());
	if (pb == NULL)
	{
		putErrmsg("No memory for prediction base.", NULL);
		return NULL;
	}

	lyst_delete_set(pb, freePbContents, NULL);
	CHKNULL(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &iondb, getIonDbObject(), sizeof(IonDB));
	for (i = 0; i < 2; i++)
	{
		for (elt = sdr_list_first(sdr, iondb.contactLog[i]); elt;
				elt = sdr_list_next(sdr, elt))
		{
			sdr_read(sdr, (char *) &logEntry, sdr_list_data(sdr,
					elt), sizeof(PastContact));
			if (fromNode)	/*	Selective.		*/
			{
				if (logEntry.fromNode != fromNode
				|| logEntry.toNode != toNode)
				{
					continue;	/*	N/A	*/
				}
			}

			if (insertIntoPredictionBase(pb, &logEntry) < 0)
			{
				putErrmsg("Can't insert into prediction base.",
						NULL);
				lyst_destroy(pb);
				sdr_exit_xn(sdr);
				return NULL;
			}
		}
	}

	sdr_exit_xn(sdr);
	return pb;
}

static int	processSequence(LystElt start, LystElt end, time_t currentTime)
{
	uvast		fromNode;
	uvast		toNode;
	time_t		horizon;
	LystElt		elt;
	PbContact	*contact;
	uvast		totalCapacity = 0;
	uvast		totalContactDuration = 0;
	unsigned int	contactsCount = 0;
	uvast		totalGapDuration = 0;
	unsigned int	gapsCount = 0;
	LystElt		prevElt;
	PbContact	*prevContact;
	uvast		gapDuration;
	uvast		meanCapacity;
	uvast		meanContactDuration;
	uvast		meanGapDuration;
	vast		deviation;
	uvast		contactDeviationsTotal = 0;
	uvast		gapDeviationsTotal = 0;
	uvast		contactStdDev;
	uvast		gapStdDev;
	float		contactDoubt;
	float		gapDoubt;
	float		contactConfidence;
	float		gapConfidence;
	float		netConfidence;
	size_t		xmitRate;
	PsmAddress	cxaddr;
char	buf[255];

	if (start == NULL)	/*	No sequence found yet.		*/
	{
		return 0;
	}

	contact = (PbContact *) lyst_data(start);
	fromNode = contact->fromNode;
	toNode = contact->toNode;
	horizon = currentTime + (currentTime - contact->fromTime);
writeTimestampLocal(currentTime, buf);
printf("Current time: %s\n", buf);
writeTimestampLocal(horizon, buf);
printf("Horizon: %s\n", buf);

	/*	Compute totals and means.				*/

	elt = start;
	prevElt = NULL;
	while (1)
	{
printf("Contact capacity " UVAST_FIELDSPEC ", duration " UVAST_FIELDSPEC ".\n",
contact->capacity, contact->duration);
		totalCapacity += contact->capacity;
		totalContactDuration += contact->duration;
		contactsCount++;
		if (prevElt)
		{
			prevContact = (PbContact *) lyst_data(prevElt);
			gapDuration = contact->fromTime - prevContact->toTime;
printf("Gap duration " UVAST_FIELDSPEC ".\n", gapDuration);
			totalGapDuration += gapDuration;
			gapsCount++;
		}

		prevElt = elt;
//		if (lyst_data(elt) == lyst_data(end))
		if (elt == end)
		{
			break;
		}

		elt = lyst_next(elt);
		contact = (PbContact *) lyst_data(elt);
	}

	if (gapsCount == 0)
	{
puts("No gaps in contact log, can't predict contacts.");
		return 0;
	}

	meanCapacity = totalCapacity / contactsCount;
	meanContactDuration = totalContactDuration / contactsCount;
	meanGapDuration = totalGapDuration / gapsCount;
printf("Mean contact capacity " UVAST_FIELDSPEC ", mean contact duration "
UVAST_FIELDSPEC ", mean gap duration " UVAST_FIELDSPEC ".\n",
meanCapacity, meanContactDuration, meanGapDuration);

	/*	Compute standard deviations.				*/

	contact = (PbContact *) lyst_data(start);
	elt = start;
	prevElt = NULL;
	while (1)
	{
		deviation = contact->duration - meanContactDuration;
		contactDeviationsTotal += (deviation * deviation);
		if (prevElt)
		{
			prevContact = (PbContact *) lyst_data(prevElt);
			gapDuration = contact->fromTime - prevContact->toTime;
			deviation = gapDuration - meanGapDuration;
			gapDeviationsTotal += (deviation * deviation);
		}

		prevElt = elt;
//		if (lyst_data(elt) == lyst_data(end))
		if (elt == end)
		{
			break;
		}

		elt = lyst_next(elt);
		contact = (PbContact *) lyst_data(elt);
	}

	contactStdDev = sqrt(contactDeviationsTotal / contactsCount);
	gapStdDev = sqrt(gapDeviationsTotal / gapsCount);
printf("Contact duration sigma " UVAST_FIELDSPEC ", gap duration sigma "
UVAST_FIELDSPEC ".\n", contactStdDev, gapStdDev);

	/*	Compute net confidence for predicted contact between
	 *	these two nodes.					*/

	if (contactStdDev > meanContactDuration)
	{
		contactDoubt = 1.0;
	}
	else
	{
		contactDoubt = MAX(0.1,
				((double) contactStdDev) / meanContactDuration);
	}

printf("Contact doubt %f.\n", contactDoubt);
	if (gapStdDev > meanGapDuration)
	{
		gapDoubt = 1.0;
	}
	else
	{
		gapDoubt = MAX(0.1, ((double) gapStdDev) / meanGapDuration);
	}

printf("Gap doubt %f.\n", gapDoubt);
	contactConfidence = 1.0 - powf(contactDoubt,
			contactsCount / CONFIDENCE_BASIS);
printf("Contact confidence %f.\n", contactConfidence);
	gapConfidence = 1.0 - powf(gapDoubt, gapsCount / CONFIDENCE_BASIS);
printf("Gap confidence %f.\n", gapConfidence);
	netConfidence = MAX(((contactConfidence + gapConfidence) / 2.0), .01);
printf("Net confidence %f.\n", netConfidence);

	/*	Insert predicted contact (aggregate).			*/

	xmitRate = totalCapacity / (horizon - currentTime);
	if (xmitRate > 1)
	{
		if (rfx_insert_contact(currentTime, horizon, fromNode, toNode,
				xmitRate, netConfidence, &cxaddr) < 0)
		{
			putErrmsg("Can't insert contact.", NULL);
			return -1;
		}
	}

	return 0;
}

int	rfx_predict_contacts(uvast fromNode, uvast toNode)
{
	time_t		currentTime = getUTCTime();
	Lyst		predictionBase;
	PastContact	logEntry;
	int		result = 0;

	/*	First, remove predicted contacts from contact plan.	*/

	if (removePredictedContacts(fromNode, toNode) < 0)
	{
		putErrmsg("Can't predict contacts.", NULL);
		return -1;
	}

	/*	Next, construct a prediction base from the current
	 *	current contact logs (containing all discovered
	 *	contacts that occurred in the past).			*/

	predictionBase = constructPredictionBase(fromNode, toNode);
	if (predictionBase == NULL)
	{
		putErrmsg("Can't predict contacts.", NULL);
		return -1;
	}

	/*	If the prediction base is empty, insert a single
	 *	"hypothetical" past discovered contact (from one day
	 *	ago to 1 hour later) to enable bold route anticipation.	*/

	if (lyst_length(predictionBase) == 0)
	{
		logEntry.fromTime = currentTime - 86400;
		logEntry.toTime = logEntry.fromTime + 3600;
		logEntry.fromNode = fromNode;
		logEntry.toNode = toNode;
		logEntry.xmitRate = 125000000;	/*	1 Gbps		*/
		if (insertIntoPredictionBase(predictionBase, &logEntry) < 0)
		{
			putErrmsg("Can't insert hypothetical contact.", NULL);
			lyst_destroy(predictionBase);
			return -1;
		}
	}

	/*	If the prediction base contains only a single contact,
	 *	insert an additional "stub" past discovered contact
	 *	(creating one gap) to enable bold route anticipation.	*/

	if (lyst_length(predictionBase) == 1)
	{
		logEntry.fromTime = currentTime - 2;
		logEntry.toTime = logEntry.fromTime + 1;
		logEntry.fromNode = fromNode;
		logEntry.toNode = toNode;
		logEntry.xmitRate = 125000000;	/*	1 Gbps		*/
		if (insertIntoPredictionBase(predictionBase, &logEntry) < 0)
		{
			putErrmsg("Can't insert hypothetical contact.", NULL);
			lyst_destroy(predictionBase);
			return -1;
		}
	}

	/*	Now generate predicted contacts from the prediction
	 *	base.							*/

	if (processSequence(lyst_first(predictionBase),
			lyst_last(predictionBase), currentTime) < 0)
	{
		putErrmsg("Can't predict contacts.", NULL);
		result = -1;
	}

	lyst_destroy(predictionBase);
	return result;
}

int	rfx_predict_all_contacts()
{
	time_t		currentTime = getUTCTime();
	Lyst		predictionBase;
	LystElt		elt;
	PbContact	*contact;
	LystElt		startOfSequence = NULL;
	LystElt	 	endOfSequence = NULL;
	uvast		sequenceFromNode = 0;
	uvast		sequenceToNode = 0;
	int		result = 0;

	/*	First, remove all predicted contacts from contact plan.	*/

	if (removePredictedContacts(0, 0) < 0)
	{
		putErrmsg("Can't predict contacts.", NULL);
		return -1;
	}

	/*	Next, construct a prediction base from the current
	 *	current contact logs (containing all discovered
	 *	contacts that occurred in the past).			*/

	predictionBase = constructPredictionBase(0, 0);
	if (predictionBase == NULL)
	{
		putErrmsg("Can't predict contacts.", NULL);
		return -1;
	}

	/*	Now generate predicted contacts from the prediction
	 *	base.							*/

	for (elt = lyst_first(predictionBase); elt; elt = lyst_next(elt))
	{
		contact = (PbContact *) lyst_data(elt);
		if (contact->fromNode != sequenceFromNode
		|| contact->toNode != sequenceToNode)
		{
			if (processSequence(startOfSequence, endOfSequence,
					currentTime) < 0)
			{
				putErrmsg("Can't predict contacts.", NULL);
				lyst_destroy(predictionBase);
				return -1;
			}

			/*	Note start of new sequence.		*/

			sequenceFromNode = contact->fromNode;
			sequenceToNode = contact->toNode;
			startOfSequence = elt;
		}

		/*	Continuation of current prediction sequence.	*/

		endOfSequence = elt;
	}

	/*	Process the last sequence in the prediction base.	*/

	if (processSequence(startOfSequence, endOfSequence, currentTime) < 0)
	{
		putErrmsg("Can't predict contacts.", NULL);
		result = -1;
	}

	lyst_destroy(predictionBase);
	return result;
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
		getCurrentTime(&(vdb->lastEditTime));
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

/*	*	RFX alarm management functions	*	*	*	*/

extern PsmAddress	rfx_insert_alarm(unsigned int term,
				unsigned int cycles)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();
	IonVdb 		*vdb = getIonVdb();
	time_t		currentTime = getUTCTime();
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

	CHKERR(sdr_begin_xn(sdr));	/*	To lock memory.		*/
	sm_SemEnd(alarm->semaphore);
	microsnooze(50000);
	sm_SemDelete(alarm->semaphore);
	alarm->semaphore = SM_SEM_NONE;
	event.time = alarm->nextTimeout;
	event.type = IonAlarmTimeout;
	event.ref = alarmAddr;
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
	cxref.discovered = contact.discovered;
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
			sm_SemEnd(req->semaphore);
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
