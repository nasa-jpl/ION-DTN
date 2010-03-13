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

static Sdr		cgrsdr;

typedef struct
{
	unsigned long	neighborNodeNbr;
	FwdDirective	directive;
	time_t		forfeitTime;
	int		distance;	/*	# hops from dest. node.	*/
} ProximateNode;

static int	getPlan(int nodeNbr, Object plans, IpnPlan *plan)
{
	Object	elt;
	Object	planAddr;

	for (elt = sdr_list_first(cgrsdr, plans); elt;
			elt = sdr_list_next(cgrsdr, elt))
	{
		planAddr = sdr_list_data(cgrsdr, elt);
		sdr_read(cgrsdr, (char *) plan, planAddr, sizeof(IpnPlan));
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
			int distance)
{
	PsmPartition	ionwm = getIonwm();
	PsmAddress	snubElt;
	IonSnub		*snub;
	IpnPlan		plan;
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

	if (getPlan(neighbor->nodeNbr, plans, &plan) == 0)
	{
		return 0;		/*	No plan on file.	*/
	}

	/*	Now determine whether or not the bundle could be
	 *	sent to this neighbor via the outduct for this plan's
	 *	default directive during the contact that is being
	 *	considered.  That is: is the capacity of this contact,
	 *	plus the sum of the capacities of all preceding
	 *	contacts with the same neighbor, greater than the
	 *	sum of the current applicable backlog of pending
	 *	transmissions on that outduct plus the estimated
	 *	transmission capacity consumption of this bundle?	*/

	copyScalar(&capacity, &(xmit->aggrCapacity));
	sdr_read(cgrsdr, (char *) &outduct, sdr_list_data(cgrsdr,
			plan.defaultDirective.outductElt), sizeof(Outduct));
	computeApplicableBacklog(&outduct, bundle, &backlog);
	subtractFromScalar(&capacity, &backlog);
	if (!scalarIsValid(&capacity))
	{
		/*	Contact is already fully subscribed; no
		 *	available residual capacity.			*/

		return 0;
	}

	sdr_read(cgrsdr, (char *) &protocol, outduct.protocol,
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

	/*	The forfeitTime noted here is the earliest among
	 *	the termination times of all the contacts on ANY
	 *	path to the destination starting at this neighbor.
	 *	The distance noted here is the shortest among the
	 *	distances of ALL paths to the destination, starting
	 *	at this neighbor, that have minimum forfeitTime.	*/

	for (elt = lyst_first(proximateNodes); elt; elt = lyst_next(elt))
	{
		proxNode = (ProximateNode *) lyst_data(elt);
		if (proxNode->neighborNodeNbr == neighbor->nodeNbr)
		{
			/*	This xmit is another contact with a
			 *	neighbor that's already in the list.	*/

			if (forfeitTime < proxNode->forfeitTime)
			{
				proxNode->forfeitTime = forfeitTime;
				proxNode->distance = distance;
			}
			else
			{
				if (forfeitTime == proxNode->forfeitTime)
				{
					if (distance < proxNode->distance)
					{
						proxNode->distance = distance;
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
	proxNode->forfeitTime = forfeitTime;
	proxNode->distance = distance;
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
			Bundle *bundle, Object bundleObj,
			Lyst excludedNodes, Lyst proximateNodes,
			time_t forfeitTime, int distance)
{
	LystElt		exclusion;
	PsmPartition	ionwm = getIonwm();
	PsmAddress	elt;
	IonXmit		*xmit;
	time_t		closingTime;
	IonOrigin	*origin;
	unsigned long	owltMargin;
	unsigned long	lastChanceFromOrigin;
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

	/*	Examine all opportunities for transmission to node.	*/

#if CGRDEBUG
printf("In identifyProximateNodes for node %lu, deadline %lu.\n",
node->nodeNbr, deadline);
#endif
	for (elt = sm_list_first(ionwm, node->xmits); elt;
			elt = sm_list_next(ionwm, elt))
	{
		xmit = (IonXmit *) psp(ionwm, sm_list_data(ionwm, elt));
		if (xmit->fromTime > deadline)
		{
#if CGRDEBUG
printf("xmit->fromTime %lu is %lu sec after deadline.  No further \
opportunities.\n", xmit->fromTime, xmit->fromTime - deadline);
#endif
			lyst_delete(exclusion);
			return 0;	/*	No more opportunities.	*/
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
					proximateNodes, closingTime, distance))
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
					proximateNodes, closingTime, distance))
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
				maxFromTime, stationNode, bundle,
			       	bundleObj, excludedNodes, proximateNodes,
				closingTime, distance + 1) < 0)
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

		sdr_write(cgrsdr, bundleObj, (char *) bundle, sizeof(Bundle));
	}

	/*	In any event, we enqueue the bundle for transmission.	*/

	if (enqueueToDuct(&(proxNode->directive), bundle, bundleObj,
			stationEid) < 0)
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
	LystElt		elt;
	LystElt		nextElt;
	ProximateNode	*proxNode;
	ProximateNode	*selectedNeighbor;
	time_t		earliestStopTime = 0;
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

	stationNode = findNode(ionvdb, stationNodeNbr, &nextNode);
	if (stationNode == NULL)
	{
#if CGRDEBUG
printf("No routing information for node %lu.\n", stationNodeNbr);
#endif
		return 0;	/*	Can't apply CGR.		*/
	}

	cgrsdr = getIonsdr();
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
	if (identifyProximateNodes(stationNode, plans,
			bundle->expirationTime + EPOCH_2000_SEC,
			stationNode, bundle, bundleObj, excludedNodes,
			proximateNodes, 0, 0) < 0)
	{
		putErrmsg("Can't identify proximate nodes for bundle.", NULL);
		return -1;
	}

	/*	Examine the list of proximate nodes.  If the bundle
	 *	is critical, enqueue it on the outduct to EACH
	 *	identified proximate destination node.
	 *
	 *	Otherwise, enqueue the bundle on the outduct to the
	 *	identified proximate destination for the contact with
	 *	the earliest termination time.  The intent of this
	 *	selection is not to minimize delivery latency for
	 *	the bundle but rather to maximize link utilization:
	 *	bundles that must be forwarded in the future may
	 *	have later expiration times that enable them to be
	 *	forwarded in contacts that end later, so let's not
	 *	risk wasting any transmission opportunity in this
	 *	earlier/shorter contact.				*/

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

	/*	Non-critical bundle; send on path that closes earliest.
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
			earliestStopTime = proxNode->forfeitTime;
			shortestDistance = proxNode->distance;
			selectedNeighbor = proxNode;
		}
		else if (proxNode->forfeitTime < earliestStopTime)
		{
			earliestStopTime = proxNode->forfeitTime;
			shortestDistance = proxNode->distance;
			if (selectedNeighbor)
			{
				MRELEASE(selectedNeighbor);
			}

			selectedNeighbor = proxNode;
		}
		else if (proxNode->forfeitTime == earliestStopTime)
		{
			if (proxNode->distance < shortestDistance)
			{
				shortestDistance = proxNode->distance;
				if (selectedNeighbor)
				{
					MRELEASE(selectedNeighbor);
				}

				selectedNeighbor = proxNode;
			}
			else	/*	Greater distance; ignore.	*/
			{
				MRELEASE(proxNode);
			}
		}
		else	/*	Later forfeit time; ignore.		*/
		{
			MRELEASE(proxNode);
		}
	}

	if (selectedNeighbor)
	{
#if CGRDEBUG
printf("CGR selected node %d for next hop.\n",
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
