/*
	libcgr.c:	functions implementing Contact Graph Routing.

	Author: Scott Burleigh, JPL

	Copyright (c) 2008, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
#include "ipnfw.h"

#ifndef CGRDEBUG
#define CGRDEBUG	0
#endif

typedef struct
{
	unsigned long	neighborNodeNbr;
	FwdDirective	directive;
	time_t		forfeitTime;
	time_t		deliveryTime;
	int		distance;	/*	# hops from dest. node.	*/
} ProximateNode;

static void	resetLastVisitor()
{
	PsmPartition	ionwm = getIonwm();
	IonVdb		*ionvdb = getIonVdb();
	PsmAddress	elt;
	IonNode		*node;
	PsmAddress	elt2;
	IonXmit		*xmit;

	for (elt = sm_list_first(ionwm, ionvdb->nodes); elt;
			elt = sm_list_next(ionwm, elt))
	{
		node = (IonNode *) psp(ionwm, sm_list_data(ionwm, elt));
		for (elt2 = sm_list_first(ionwm, node->xmits); elt2;
				elt2 = sm_list_next(ionwm, elt2))
		{
			xmit = (IonXmit *) psp(ionwm, sm_list_data(ionwm, elt));
			xmit->lastVisitor = 0;
			xmit->visitHorizon = 0;
		}
	}
}

static int	_visitorCount(int increment)
{
	static unsigned int	count = 0;

	count += increment;
	if (count == 0)
	{
		/*	On counter roll-over, reinitialize every
		 *	xmit's lastVisitor to zero.			*/

		count = 1;
		resetLastVisitor();
	}

	return count;
}

static int	getPlan(Sdr sdr, int nodeNbr, Object plans, IpnPlan *plan)
{
	Object	elt;
	Object	planAddr;

	for (elt = sdr_list_first(sdr, plans); elt;
			elt = sdr_list_next(sdr, elt))
	{
		planAddr = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) plan, planAddr, sizeof(IpnPlan));
		if (plan->nodeNbr < nodeNbr)
		{
			continue;
		}
		
		if (plan->nodeNbr > nodeNbr)
		{
			return 0;	/*	Same as end of list.	*/
		}

		return 1;
	}

	return 0;
}

static int	tryContact(IonNode *neighbor, IonXmit *xmit, Bundle *bundle,
			Object plans, Lyst proximateNodes, time_t forfeitTime,
			time_t deliveryTime, int distance)
{
	PsmPartition	ionwm = getIonwm();
	PsmAddress	snubElt;
	IonSnub		*snub;
	IpnPlan		plan;
	Sdr		sdr;
	Scalar		capacity;
	Outduct		outduct;
	Scalar		backlog;
	ClProtocol	protocol;
	int		eccc;	/*	Estimated capacity consumption.	*/
	LystElt		elt;
	ProximateNode	*proxNode;

	/*	Never route to self unless self is final destination.	*/

	if (neighbor->nodeNbr == getOwnNodeNbr())
	{
		if (!(bundle->destination.cbhe
		&& bundle->destination.c.nodeNbr == neighbor->nodeNbr))
		{
			return 0;	/*	Don't send to self.	*/
		}

		/*	Self is final destination.			*/

		for (snubElt = sm_list_first(ionwm, neighbor->snubs); snubElt;
				snubElt = sm_list_next(ionwm, snubElt))
		{
			snub = (IonSnub *) psp(ionwm, sm_list_data(ionwm,
					snubElt));
			if (snub->nodeNbr < neighbor->nodeNbr)
			{
				continue;
			}

			if (snub->nodeNbr > neighbor->nodeNbr)
			{
				break;
			}

			/*	Node is refusing custody of bundles
			 *	that it sends to itself.		*/

			return 0;
		}
	}

	/*	ION currently supports only a single contact plan: all
	 *	contacts are assumed to be implemented per the default
	 *	directive in the IpnPlan defined for each neighboring
	 *	node.  Overriding plan rules whose directives would
	 *	implement supplementary contact plans are not supported.
	 *	That support would require a means of characterizing a
	 *	Contact by the specific directive that would implement
	 *	it.  This level of complexity is not required by any
	 *	currently anticipated application of ION.		*/

	sdr = getIonsdr();
	if (getPlan(sdr, neighbor->nodeNbr, plans, &plan) == 0)
	{
		return 0;		/*	No plan on file.	*/
	}

	/*	Now determine whether or not the bundle could be
	 *	sent to this neighbor via the outduct for this plan's
	 *	default directive during the contact that is being
	 *	considered.  There are two criteria.  First, is the
	 *	duct blocked (e.g., no TCP connection)?			*/

	copyScalar(&capacity, &(xmit->aggrCapacity));
	sdr_read(sdr, (char *) &outduct, sdr_list_data(sdr,
			plan.defaultDirective.outductElt), sizeof(Outduct));
	if (outduct.blocked)
	{
		return 0;		/*	Outduct is unusable.	*/
	}

	/*	Second: is the total capacity of this contact,
	 *	plus the sum of the capacities of all preceding
	 *	contacts with the same neighbor, greater than the
	 *	sum of the current applicable backlog of pending
	 *	transmissions on that outduct plus the estimated
	 *	transmission capacity consumption of this bundle?	*/

	computeApplicableBacklog(&outduct, bundle, &backlog);
	subtractFromScalar(&capacity, &backlog);
	if (!scalarIsValid(&capacity))
	{
		/*	Contact is already fully subscribed; no
		 *	available residual capacity.			*/

		return 0;
	}

	sdr_read(sdr, (char *) &protocol, outduct.protocol,
			sizeof(ClProtocol));
	eccc = computeECCC(guessBundleSize(bundle), &protocol);
	reduceScalar(&capacity, eccc);
	if (!scalarIsValid(&capacity))
	{
		/*	Available residual capacity is not enough
		 *	to get all of this bundle transmitted.		*/

		return 0;
	}

	/*	This contact is plausible, so add this neighbor to the
	 *	list of proximateNodes if it's not already in the list.	*/

	/*	The deliveryTime noted here is the earliest among
	 *	the final-contact end times of ALL paths to the
	 *	destination starting at this neighbor.
	 *
	 *	The distance noted here is the shortest among the
	 *	distances of ALL paths to the destination, starting
	 *	at this neighbor, that have minimum deliveryTime.
	 *
	 *	We set forfeit time to the forfeit time associated with
	 *	the "best" (lowest-latency, shortest) path.  Note that
	 *	the best path might not have the lowest associated
	 *	forfeit time.						*/

	for (elt = lyst_first(proximateNodes); elt; elt = lyst_next(elt))
	{
		proxNode = (ProximateNode *) lyst_data(elt);
		if (proxNode->neighborNodeNbr == neighbor->nodeNbr)
		{
			/*	This xmit is another contact with a
			 *	neighbor that's already in the list.	*/

			if (deliveryTime < proxNode->deliveryTime)
			{
				proxNode->deliveryTime = deliveryTime;
				proxNode->distance = distance;
				proxNode->forfeitTime = forfeitTime;
			}
			else
			{
				if (deliveryTime == proxNode->deliveryTime)
				{
					if (distance < proxNode->distance)
					{
						proxNode->distance = distance;
						proxNode->forfeitTime =
								forfeitTime;
					}
				}
			}

			return 0;
		}
	}

	/*	This neighbor is not yet in the list, so add it.	*/

	proxNode = (ProximateNode *) MTAKE(sizeof(ProximateNode));
	if (proxNode == NULL
	|| lyst_insert_last(proximateNodes, (void *) proxNode) == 0)
	{
		putErrmsg("Can't add proximateNode.", NULL);
		return -1;
	}

	proxNode->neighborNodeNbr = neighbor->nodeNbr;
	memcpy((char *) &(proxNode->directive), (char *) &plan.defaultDirective,
			sizeof(FwdDirective));
	proxNode->deliveryTime = deliveryTime;
	proxNode->distance = distance;
	proxNode->forfeitTime = forfeitTime;
	return 0;
}

static int	isExcluded(unsigned long nodeNbr, Lyst excludedNodes)
{
	LystElt	elt;

	for (elt = lyst_first(excludedNodes); elt; elt = lyst_next(elt))
	{
		if ((unsigned long) lyst_data(elt) == nodeNbr)
		{
			return 1;	/*	Node is in the list.	*/
		}
	}

	return 0;
}

static int	identifyProximateNodes(IonNode *node, Object plans,
			unsigned long deadline, IonNode *stationNode,
			Bundle *bundle, Object bundleObj, Lyst excludedNodes,
			Lyst proximateNodes, time_t forfeitTime,
			time_t deliveryTime, int distance,
			unsigned int visitorNbr)
{
	PsmPartition	ionwm = getIonwm();
	LystElt		exclusion;
	PsmAddress	elt;
	IonXmit		*xmit;
	time_t		closingTime;
	IonOrigin	*origin;
	unsigned long	owltMargin;
	unsigned long	lastChanceFromOrigin;
	unsigned long	currentTime;
	IonNode		*originNode;
	IonVdb		*ionvdb = getIonVdb();
	PsmAddress	nextNode;
	unsigned long	forwardingLatency;
	unsigned long	maxFromTime;

	/*	Make sure we don't get into a routing loop while
	 *	trying to compute routes to this node.			*/

	exclusion = lyst_insert_last(excludedNodes, (void *) (node->nodeNbr));
	if (exclusion == NULL)
	{
		putErrmsg("Can't identify proximate nodes.", NULL);
		return -1;
	}

	/*	Examine all opportunities for transmission to node.
	 *	Walk the list in reverse order, i.e., in descending
	 *	toTime order, so that the maximum deadline time is
	 *	the first one passed to subordinate invocations of
	 *	identifyProximateNodes.  This enables subsequent visits
	 *	to this vertext of the contact graph to be skipped.	*/

#if CGRDEBUG
printf("In identifyProximateNodes for node %lu, deadline %lu.\n",
node->nodeNbr, deadline);
#endif
	for (elt = sm_list_last(ionwm, node->xmits); elt;
			elt = sm_list_prev(ionwm, elt))
	{
		xmit = (IonXmit *) psp(ionwm, sm_list_data(ionwm, elt));
		if (xmit->fromTime > deadline)
		{
			continue;	/*	Too late; ignore it.	*/
		}

		/*	The "usability" of a contact is a best-case
		 *	determination as to whether or not it is
		 *	mathematically plausible for a bundle sent
		 *	from the local node to be transmitted during
		 *	this contact prior to some deadline.  For each
		 *	bundle, once we have determined the usability
		 *	of a contact in the context of a given
		 *	deadline we need not consider that contact
		 *	again the context of any earlier deadline.
		 *	The reasoning is that if the contact was usable
		 *	for that original deadline then it has already
		 *	had whatever effect on the proximateNodes list
		 *	it could potentially have, so there's no need
		 *	to re-consider it; it not, then it certainly
		 *	won't be usable for this tighter deadline, so
		 *	there's no reason to consider it at all.	*/

		if (xmit->lastVisitor == visitorNbr)
		{
			/*	Have already considered this contact.	*/

			if (deadline <= xmit->visitHorizon)
			{
				continue;
			}
		}

		/*	Either we've never considered this contact
		 *	before (for this bundle) or else we only
		 *	determined its usability for an earlier
		 *	deadline.  If the former, we have to visit
		 *	it.  Now suppose the latter.  If the contact
		 *	was found to be usable for that deadline
		 *	then we've already used it to update the
		 *	proximateNodes list.  It's hard to know if
		 *	this is the case; even if it is, revisiting
		 *	the contact is harmless, just a waste of time.
		 *	So we may as well revisit.  On the other hand,
		 *	if the contact was found to be unusable for
		 *	that earlier deadline it might still be
		 *	usable for this later, less restrictive
		 *	deadline, so we surely MUST revisit.		*/

		xmit->lastVisitor = visitorNbr;
		xmit->visitHorizon = deadline;
		if (distance == 0)	/*	Final contact on path.	*/
		{
			deliveryTime = xmit->toTime;
		}

		/*	First handle the special case of loopback
		 *	contact, if necessary.				*/

		if (node->nodeNbr == getOwnNodeNbr())
		{
			/*	xmits are recorded only if they are
			 *	to a node other than the local node,
			 *	or from the local node.  We now know
			 *	that the former is NOT true, so the
			 *	latter must be true, so this must be
			 *	a loopback contact.  (And in fact all
			 *	xmits in this list must be loopback
			 *	contacts.)  So just look for outduct
			 *	and continue.				*/

			closingTime = xmit->toTime;
			if (forfeitTime && forfeitTime < closingTime)
			{
				closingTime = forfeitTime;
			}

			if (tryContact(node, xmit, bundle, plans, 
					proximateNodes, closingTime,
					deliveryTime, distance))
			{
				putErrmsg("Can't check contact.", NULL);
				return -1;
			}

			continue;
		}

		/*	Not a loopback contact.  Routing loop?		*/

		origin = (IonOrigin *) psp(ionwm, xmit->origin);
#if CGRDEBUG
printf("Considering xmit from node %lu to node %lu.\n", origin->nodeNbr,
node->nodeNbr);
#endif
		if (isExcluded(origin->nodeNbr, excludedNodes))
		{
#if CGRDEBUG
printf("xmit's origin node (%lu) is excluded; next xmit.\n", origin->nodeNbr);
#endif
			/*	Can't continue -- it would be a routing
			 *	loop.  We've already computed all
			 *	routes on this path that go through
			 *	this origin node.			*/

			continue;
		}

		/*	Not a routing loop.  Can this happen in time?	*/

		owltMargin = ((MAX_SPEED_MPH / 3600) * origin->owlt) / 186282;
		lastChanceFromOrigin = deadline - (origin->owlt + owltMargin);
		currentTime = (unsigned long) getUTCTime();
		if (currentTime > lastChanceFromOrigin)
		{
#if CGRDEBUG
printf("lastChanceFromOrigin is %lu.\n", lastChanceFromOrigin);
printf("currentTime %lu is %lu sec after lastChanceFromOrigin.\n",
currentTime, currentTime - lastChanceFromOrigin);
#endif
			continue;	/*	Non-viable opportunity.	*/
		}

		if (xmit->fromTime > lastChanceFromOrigin)
		{
#if CGRDEBUG
printf("lastChanceFromOrigin is %lu.\n", lastChanceFromOrigin);
printf("xmit->fromTime %lu is %lu sec after lastChanceFromOrigin.\n",
xmit->fromTime, xmit->fromTime - lastChanceFromOrigin);
#endif
			continue;	/*	Non-viable opportunity.	*/
		}

		/*	This is a viable opportunity to transmit this
		 *	bundle to the station node from the indicated
		 *	origin node.					*/

		if (origin->nodeNbr == getOwnNodeNbr())
		{
#if CGRDEBUG
printf("Queueing directly from local node to node %lu.\n", node->nodeNbr);
#endif
			/*	The node we are thinking of transmitting
			 *	to is a neighbor, i.e., we could transmit
			 *	directly to it.				*/

			closingTime = xmit->toTime;
			if (forfeitTime && forfeitTime < closingTime)
			{
				closingTime = forfeitTime;
			}

			if (tryContact(node, xmit, bundle, plans, 
					proximateNodes, closingTime,
					deliveryTime, distance))
			{
				putErrmsg("Can't check contact.", NULL);
				return -1;
			}

			continue;
		}
		
		/*	The origin node for this opportunity is not
		 *	the local node, so how do we route to that
		 *	origin node in time to seize this opportunity?
		 *
		*	Must check out the origin node's contacts.  For
		 *	this purpose, the deadline is whichever is
		 *	earlier: (a) the latest time at which the bundle
		 *	could be sent from the origin in order to arrive
		 *	before deadline or (b) the latest time by which
		 *	the bundle must arrive at the origin node in
		 *	order to be transmitted before the end of the
		 *	transmit opportunity.  To compute the latter we
		 *	subtract from the opportunity end time an
		 *	additional latency, leaving time for reception
		 *	and re-radiation of the bundle; we estimate
		 *	this latency by doubling the bundle's payload
		 *	size and dividing the result by the transmission
		 *	rate on this xmit opportunity.			*/

		originNode = findNode(ionvdb, origin->nodeNbr, &nextNode);
		if (originNode)	/*	Node must be in contact graph.	*/
		{
#if CGRDEBUG
printf("Now computing route to node %lu.\n", origin->nodeNbr);
#endif
			closingTime = xmit->toTime;
			if (forfeitTime && forfeitTime < closingTime)
			{
				closingTime = forfeitTime;
			}

			forwardingLatency = (bundle->payload.length << 1)
					/ xmit->xmitRate;
			maxFromTime = xmit->toTime - forwardingLatency;
#if CGRDEBUG
printf("New deadline is %lu.\n", maxFromTime);
#endif
			if (lastChanceFromOrigin < maxFromTime)
			{
				maxFromTime = lastChanceFromOrigin;
#if CGRDEBUG
printf("New deadline changed to %lu.\n", maxFromTime);
#endif
			}

			if (identifyProximateNodes(originNode, plans,
				maxFromTime, stationNode, bundle, bundleObj,
				excludedNodes, proximateNodes, closingTime,
				deliveryTime, distance + 1, visitorNbr) < 0)
			{
				putErrmsg("Can't identify origin prox. nodes.",
						NULL);
				return -1;
			}
#if CGRDEBUG
printf("Finished computing route to node %lu, back to node %lu.\n",
origin->nodeNbr, node->nodeNbr);
#endif
		}
	}

	/*	No more opportunities for transmission to this node.	*/

	lyst_delete(exclusion);
	return 0;
}

static int	enqueueToNeighbor(ProximateNode *proxNode, Bundle *bundle,
			Object bundleObj, IonNode *stationNode)
{
	char		*decoration;
	unsigned long	serviceNbr;
	char		stationEid[64];
	PsmPartition	ionwm;
	PsmAddress	snubElt;
	IonSnub		*snub;
	BpEvent		event;

#if BP_URI_RFC
	decoration = "dtn::";
#else
	decoration = "";
#endif
	if (proxNode->neighborNodeNbr == bundle->destination.c.nodeNbr)
	{
		serviceNbr = bundle->destination.c.serviceNbr;
	}
	else
	{
		serviceNbr = 0;
	}

	isprintf(stationEid, sizeof stationEid, "%.5s%.8s:%lu.%lu", decoration,
		CBHE_SCHEME_NAME, proxNode->neighborNodeNbr, serviceNbr);

	/*	If this neighbor is a currently snubbing neighbor
	 *	for this final destination (i.e., one that has been
	 *	refusing bundles destined for this final destination
	 *	node), then this bundle serves as a "probe" aimed at
	 *	that neighbor.  In that case, must now schedule the
	 *	next probe to this neighbor.				*/

	ionwm = getIonwm();
	for (snubElt = sm_list_first(ionwm, stationNode->snubs); snubElt;
			snubElt = sm_list_next(ionwm, snubElt))
	{
		snub = (IonSnub *) psp(ionwm, sm_list_data(ionwm, snubElt));
		if (snub->nodeNbr < proxNode->neighborNodeNbr)
		{
			continue;
		}

		if (snub->nodeNbr > proxNode->neighborNodeNbr)
		{
			break;
		}

		/*	This neighbor has been refusing bundles
		 *	destined for this final destination node,
		 *	but since it is now due for a probe bundle
		 *	(else it would have been on the excludedNodes
		 *	list and therefore would never have made it
		 *	to the list of proximateNodes), we are
		 *	sending this one to it.  So we must turn
		 *	off the flag indicating that a probe to this
		 *	node is due -- we're sending one now.		*/

		snub->probeIsDue = 0;
		break;
	}

	/*	If the bundle is NOT critical, then we need to post
	 *	an xmitOverdue timeout event to trigger re-forwarding
	 *	in case the bundle doesn't get transmitted during the
	 *	contact in which we expect it to be transmitted.	*/

	if (!(bundle->extendedCOS.flags & BP_MINIMUM_LATENCY))
	{
		event.type = xmitOverdue;
		event.time = proxNode->forfeitTime;
		event.ref = bundleObj;
		bundle->overdueElt = insertBpTimelineEvent(&event);
		if (bundle->overdueElt == 0)
		{
			putErrmsg("Can't schedule xmitOverdue.", NULL);
			return -1;
		}

		sdr_write(getIonsdr(), bundleObj, (char *) bundle,
				sizeof(Bundle));
	}

	/*	In any event, we enqueue the bundle for transmission.
	 *	Since we've already determined that the outduct to
	 *	this neighbor is not blocked (else the neighbor would
	 *	not be in the list of proximate nodes), the bundle
	 *	can't go into limbo at this point.			*/

	if (bpEnqueue(&proxNode->directive, bundle, bundleObj, stationEid) < 0)
	{
		putErrmsg("Can't enqueue bundle.", NULL);
		return -1;
	}

	return 0;
}

int	cgr_forward(Bundle *bundle, Object bundleObj,
		unsigned long stationNodeNbr, Object plans)
{
	int		ionMemIdx;
	Lyst		proximateNodes;
	Lyst		excludedNodes;
	IonNode		*stationNode;
	IonVdb		*ionvdb = getIonVdb();
	PsmAddress	nextNode;
	PsmPartition	ionwm = getIonwm();
	PsmAddress	snubElt;
	IonSnub		*snub;
	unsigned int	visitorNbr;
	LystElt		elt;
	LystElt		nextElt;
	ProximateNode	*proxNode;
	ProximateNode	*selectedNeighbor;
	time_t		earliestDeliveryTime = 0;
	int		shortestDistance = 0;

	/*	Determine whether or not the contact graph for this
	 *	node identifies one or more proximate nodes to
	 *	which the bundle may be sent in order to get it
	 *	delivered to the specified node.  If so, use
	 *	the Plan asserted for the best proximate node(s)
	 *	("dynamic route").
	 *
	 *	Note that CGR can be used to compute a route to an
	 *	intermediate "station" node selected by another
	 *	routing mechanism (such as static routing), not
	 *	only to the bundle's final destination node.  In
	 *	the simplest case, the bundle's destination is the
	 *	only "station" selected for the bundle.			*/

	CHKERR(bundle && bundleObj && stationNodeNbr && plans);
	stationNode = findNode(ionvdb, stationNodeNbr, &nextNode);
	if (stationNode == NULL)
	{
#if CGRDEBUG
printf("No routing information for node %lu.\n", stationNodeNbr);
#endif
		return 0;	/*	Can't apply CGR.		*/
	}

	ionMemIdx = getIonMemoryMgr();
	proximateNodes = lyst_create_using(ionMemIdx);
	excludedNodes = lyst_create_using(ionMemIdx);
	if (proximateNodes == NULL || excludedNodes == NULL)
	{
		putErrmsg("Can't create lists for route computation.", NULL);
		return -1;
	}

	if (!bundle->returnToSender)
	{
		/*	Must exclude sender of bundle from consideration
		 *	as a station on the route, to minimize routing
		 *	loops.  If returnToSender is 1 then we are
		 *	re-routing, possibly back through the sender,
		 *	because we have hit a dead end in routing and
		 *	must backtrack.					*/

		if (lyst_insert_last(excludedNodes, (void *)
				(bundle->clDossier.senderNodeNbr)) == NULL)
		{
			putErrmsg("Can't exclude sender from routes.", NULL);
			return -1;
		}
	}

	/*	Insert into the excludedNodes list all neighbors that
	 *	have been refusing custody of bundles destined for the
	 *	destination node.					*/

	for (snubElt = sm_list_first(ionwm, stationNode->snubs); snubElt;
			snubElt = sm_list_next(ionwm, snubElt))
	{
		snub = (IonSnub *) psp(ionwm, sm_list_data(ionwm, snubElt));
		if (!(snub->probeIsDue))
		{
			/*	(Omit the snubbing node from the list
			 *	of excluded nodes if it's now time to
			 *	probe that node for renewed acceptance
			 *	of bundles destined for this destination
			 *	node.)					*/

			if (lyst_insert_last(excludedNodes,
				(void *) (snub->nodeNbr)) == NULL)
			{
				putErrmsg("Can't note snub.", NULL);
				return -1;
			}
		}
	}

	/*	Consult the contact graph to identify the neighboring
	 *	node(s) to forward the bundle to.			*/

#if CGRDEBUG
printf("--------------- Start of contact graph traversal -------------\n");
#endif
	visitorNbr = _visitorCount(1);
	if (identifyProximateNodes(stationNode, plans,
			bundle->expirationTime + EPOCH_2000_SEC,
			stationNode, bundle, bundleObj, excludedNodes,
			proximateNodes, 0, 0, 0, visitorNbr) < 0)
	{
		putErrmsg("Can't identify proximate nodes for bundle.", NULL);
		return -1;
	}

	/*	Examine the list of proximate nodes.  If the bundle
	 *	is critical, enqueue it on the outduct to EACH
	 *	identified proximate destination node.
	 *
	 *	Otherwise, enqueue the bundle on the outduct to the
	 *	identified proximate destination for the path with
	 *	the earliest worst-case delivery time.			*/

	if (bundle->extendedCOS.flags & BP_MINIMUM_LATENCY)
	{
		/*	Critical bundle; send on all paths.		*/

		for (elt = lyst_first(proximateNodes); elt; elt = nextElt)
		{
			nextElt = lyst_next(elt);
			proxNode = (ProximateNode *) lyst_data(elt);
			lyst_delete(elt);
			if (enqueueToNeighbor(proxNode, bundle, bundleObj,
					stationNode))
			{
				putErrmsg("Can't queue for neighbor.", NULL);
				return -1;
			}

			MRELEASE(proxNode);
		}

		lyst_destroy(excludedNodes);
		lyst_destroy(proximateNodes);
		return 0;
	}

	/*	Non-critical bundle; send on the minimum-latency path.
	 *	In case of a tie, select the path of minimum distance
	 *	from the destination node.				*/

	selectedNeighbor = NULL;
	for (elt = lyst_first(proximateNodes); elt; elt = nextElt)
	{
		nextElt = lyst_next(elt);
		proxNode = (ProximateNode *) lyst_data(elt);
		lyst_delete(elt);
		if (selectedNeighbor == NULL)
		{
			earliestDeliveryTime = proxNode->deliveryTime;
			shortestDistance = proxNode->distance;
			selectedNeighbor = proxNode;
		}
		else if (proxNode->deliveryTime < earliestDeliveryTime)
		{
			earliestDeliveryTime = proxNode->deliveryTime;
			shortestDistance = proxNode->distance;
			MRELEASE(selectedNeighbor);
			selectedNeighbor = proxNode;
		}
		else if (proxNode->deliveryTime == earliestDeliveryTime)
		{
			if (proxNode->distance < shortestDistance)
			{
				shortestDistance = proxNode->distance;
				MRELEASE(selectedNeighbor);
				selectedNeighbor = proxNode;
			}
			else if (proxNode->distance == shortestDistance)
			{
				if (proxNode->neighborNodeNbr <
					selectedNeighbor-> neighborNodeNbr)
				{
					MRELEASE(selectedNeighbor);
					selectedNeighbor = proxNode;
				}
				else	/*	Larger node#; ignore.	*/
				{
					MRELEASE(proxNode);
				}
			}
			else	/*	More hops; ignore.		*/
			{
				MRELEASE(proxNode);
			}
		}
		else	/*	Later delivery time; ignore.		*/
		{
			MRELEASE(proxNode);
		}
	}

	if (selectedNeighbor)
	{
#if CGRDEBUG
printf("CGR selected node %lu for next hop.\n",
selectedNeighbor->neighborNodeNbr);
#endif
		if (enqueueToNeighbor(selectedNeighbor, bundle, bundleObj,
				stationNode))
		{
			putErrmsg("Can't queue for neighbor.", NULL);
			return -1;
		}

		MRELEASE(selectedNeighbor);
	}
#if CGRDEBUG
else
printf("CGR found no route.\n");
#endif

	lyst_destroy(excludedNodes);
	lyst_destroy(proximateNodes);
	return 0;
}
