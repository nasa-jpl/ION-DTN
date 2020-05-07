/*
	ipnfw.c:	scheme-specific forwarder for the "ipn"
			scheme, used for Interplanetary Internet.

	Author: Scott Burleigh, JPL

	Copyright (c) 2006, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
#include <stdarg.h>

#include "ipnfw.h"

#ifdef	ION_BANDWIDTH_RESERVED
#define	MANAGE_OVERBOOKING	0
#endif

#ifndef	MANAGE_OVERBOOKING
#define	MANAGE_OVERBOOKING	1
#endif

#ifndef	MIN_PROSPECT
#define	MIN_PROSPECT	(0.0)
#endif

#ifndef CGR_DEBUG
#define CGR_DEBUG	0
#endif

#if CGR_DEBUG == 1
static void	printCgrTraceLine(void *data, unsigned int lineNbr,
			CgrTraceType traceType, ...)
{
	va_list args;
	const char *text;

	va_start(args, traceType);
	text = cgr_tracepoint_text(traceType);
	vprintf(text, args);
	switch(traceType)
	{
	case CgrIgnoreContact:
	case CgrExcludeRoute:
	case CgrSkipRoute:
		fputc(' ', stdout);
		fputs(cgr_reason_text(va_arg(args, CgrReason)), stdout);
	default:
		break;
	}

	putchar('\n');
	va_end(args);
}
#endif

static sm_SemId		_ipnfwSemaphore(sm_SemId *newValue)
{
	uaddr		temp;
	void		*value;
	sm_SemId	sem;

	if (newValue)			/*	Add task variable.	*/
	{
		temp = *newValue;
		value = (void *) temp;
		value = sm_TaskVar(&value);
	}
	else				/*	Retrieve task variable.	*/
	{
		value = sm_TaskVar(NULL);
	}

	temp = (uaddr) value;
	sem = temp;
	return sem;
}

static void	shutDown(int signum)
{
	isignal(SIGTERM, shutDown);
	sm_SemEnd(_ipnfwSemaphore(NULL));
}

/*		CGR override functions.					*/

static int	applyRoutingOverride(Bundle *bundle, Object bundleObj,
			uvast nodeNbr)
{
	Sdr		sdr = getIonsdr();
	Object		addr;
			OBJ_POINTER(IpnOverride, ovrd);
	char		eid[MAX_EID_LEN + 1];
	VPlan		*vplan;
	PsmAddress	vplanElt;
	BpPlan		plan;

	if (ipn_lookupOvrd(bundle->ancillaryData.dataLabel,
			bundle->id.source.ssp.ipn.nodeNbr,
			bundle->destination.ssp.ipn.nodeNbr, &addr) == 0)
	{
		/*	No applicable routing override.			*/

		return 0;
	}

	/*	Routing override found.					*/

	GET_OBJ_POINTER(sdr, IpnOverride, ovrd, addr);
	if (ovrd->neighbor == 0)
	{
		/*	Override neighbor not yet determined.		*/

		bundle->ovrdPending = 1;
		sdr_write(sdr, bundleObj, (char *) bundle, sizeof(Bundle));
		return 0;
	}

	/*	Must forward to override neighbor.			*/

	isprintf(eid, sizeof eid, "ipn:" UVAST_FIELDSPEC ".0", ovrd->neighbor);
	findPlan(eid, &vplan, &vplanElt);
	if (vplanElt == 0)	/*	Not a usable override.		*/
	{
		return 0;
	}

	sdr_read(sdr, (char *) &plan, sdr_list_data(sdr, vplan->planElt),
			sizeof(BpPlan));
	if (plan.blocked)	/*	Maybe later.			*/
	{
		if (enqueueToLimbo(bundle, bundleObj) < 0)
		{
			putErrmsg("Can't put bundle in limbo.", NULL);
			return -1;
		}

		return 0;
	}

	/*	Invoke the routing override.				*/

	if (bpEnqueue(vplan, bundle, bundleObj) < 0)
	{
		putErrmsg("Can't enqueue bundle.", NULL);
		return -1;
	}

	return 0;
}

static void	bindOverride(Bundle *bundle, Object bundleObj, uvast nodeNbr)
{
	Sdr		sdr = getIonsdr();
	Object		ovrdAddr;
	IpnOverride	ovrd;

	bundle->ovrdPending = 0;
	sdr_write(sdr, bundleObj, (char *) bundle, sizeof(Bundle));
	if (ipn_lookupOvrd(bundle->ancillaryData.dataLabel,
			bundle->id.source.ssp.ipn.nodeNbr,
			bundle->destination.ssp.ipn.nodeNbr, &ovrdAddr) == 0)
	{
		return;		/*	No pending override to bind.	*/
	}

	sdr_stage(sdr, (char *) &ovrd, ovrdAddr, sizeof(IpnOverride));
	if (ovrd.neighbor == 0)
	{
		ovrd.neighbor = nodeNbr;
		sdr_write(sdr, ovrdAddr, (char *) &ovrd, sizeof(IpnOverride));
	}
}

/*		HIRR invocation functions.				*/

static int	initializeHIRR(CgrRtgObject *routingObj)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();
	IonDB		iondb;
	int		i;
	Object		elt;
	Object		addr;
			OBJ_POINTER(RegionMember, member);

	routingObj->viaPassageways = sm_list_create(ionwm);
	if (routingObj->viaPassageways == 0)
	{
		putErrmsg("Can't initialize HIRR routing.", NULL);
		return -1;
	}

	sdr_read(sdr, (char *) &iondb, getIonDbObject(), sizeof(IonDB));

	/*	For each region in which the local node resides, add
	 *	to the viaPassageways list for this remote node one
	 *	entry for every passageway residing in that region.	*/

	for (i = 0; i < 2; i++)
	{
		for (elt = sdr_list_first(sdr, iondb.regions[i].members); elt;
					elt = sdr_list_next(sdr, elt))
		{
			addr = sdr_list_data(sdr, elt);
			GET_OBJ_POINTER(sdr, RegionMember, member, addr);
			if (member->nodeNbr == getOwnNodeNbr())
			{
				continue;	/*	Safety check.	*/
			}

			if (member->outerRegionNbr != -1)
			{
				/*	Node is a passageway.		*/

				if (sdr_list_insert_last(sdr,
						routingObj->viaPassageways,
						member->nodeNbr) == 0)
				{
					putErrmsg("Can't note passageway.",
							NULL);
					return -1;
				}
			}
		}
	}

	return 0;
}

static int 	tryHIRR(Bundle *bundle, Object bundleObj, IonNode *terminusNode,
			time_t atTime)
{
	PsmPartition	ionwm = getIonwm();
	CgrRtgObject	*routingObj;

	if (terminusNode->routingObject == 0)
	{
		if (cgr_create_routing_object(terminusNode) < 0)
		{
			putErrmsg("Can't initialize routing object.", NULL);
			return -1;
		}
	}

	routingObj = (CgrRtgObject *) psp(ionwm, terminusNode->routingObject);
	if (routingObj->viaPassageways == 0)
	{
		if (initializeHIRR(routingObj) < 0)
		{
			return -1;
		}
	}

	return 0;
}

/*		CGR invocation functions.				*/

static void	deleteObject(LystElt elt, void *userdata)
{
	void	*object = lyst_data(elt);

	if (object)
	{
		MRELEASE(lyst_data(elt));
	}
}

static int	excludeNode(Lyst excludedNodes, uvast nodeNbr)
{
	NodeId	*node = (NodeId *) MTAKE(sizeof(NodeId));

	if (node == NULL)
	{
		return -1;
	}

	node->nbr = nodeNbr;
	if (lyst_insert_last(excludedNodes, node) == NULL)
	{
		return -1;
	}

	return 0;
}

static size_t	carryingCapacity(size_t avblVolume)
{
	size_t	computedCapacity = avblVolume / 1.0625;
	size_t	typicalCapacity;

	if (avblVolume > TYPICAL_STACK_OVERHEAD)
	{
		typicalCapacity = avblVolume - TYPICAL_STACK_OVERHEAD;
	}
	else
	{
		typicalCapacity = 0;
	}

	if (computedCapacity < typicalCapacity)
	{
		return computedCapacity;
	}
	else
	{
		return typicalCapacity;
	}
}

static int	proactivelyFragment(Bundle *bundle, Object *bundleObj,
			CgrRoute *route)
{
	Sdr		sdr = getIonsdr();
	Object		stationEidElt;
	Object		stationEid;
	char		eid[SDRSTRING_BUFSZ];
	MetaEid		stationMetaEid;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	size_t		fragmentLength;
	Bundle		firstBundle;
	Object		firstBundleObj;
	Bundle		secondBundle;
	Object		secondBundleObj;
	Scheme		schemeBuf;

	CHKERR(bundle->payload.length > 1);
	stationEidElt = sdr_list_first(sdr, bundle->stations);
	CHKERR(stationEidElt);
	stationEid = sdr_list_data(sdr, stationEidElt);
	CHKERR(stationEid);
	if (sdr_string_read(sdr, eid, stationEid) < 0)
	{
		return -1;
	}

	if (parseEidString(eid, &stationMetaEid, &vscheme, &vschemeElt) == 0)
	{
		restoreEidString(&stationMetaEid);
		putErrmsg("Bad station EID", eid);
		return -1;
	}

	fragmentLength = carryingCapacity(route->maxVolumeAvbl);
	if (fragmentLength == 0)
	{
		fragmentLength = 1;	/*	Assume rounding error.	*/
	}

	if (fragmentLength >= bundle->payload.length)
	{
		fragmentLength = bundle->payload.length - 1;
	}

	if (bpFragment(bundle, *bundleObj, NULL, fragmentLength,
			&firstBundle, &firstBundleObj, &secondBundle,
			&secondBundleObj) < 0)
	{
		return -1;
	}

	/*	Send the second fragment back through the routing
	 *	procedure; adapted from forwardBundle().		*/

	restoreEidString(&stationMetaEid);
	stationEid = sdr_string_create(sdr, eid);
	if (stationEid == 0
	|| sdr_list_insert_first(sdr, secondBundle.stations, stationEid) == 0)
	{
		putErrmsg("Can't note station for second fragment", eid);
		return -1;
	}

	sdr_read(sdr, (char *) &schemeBuf, sdr_list_data(sdr,
			vscheme->schemeElt), sizeof(Scheme));
	secondBundle.fwdQueueElt = sdr_list_insert_first(sdr,
			schemeBuf.forwardQueue, secondBundleObj);
	sdr_write(sdr, secondBundleObj, (char *) &secondBundle, sizeof(Bundle));
	if (vscheme->semaphore != SM_SEM_NONE)
	{
		sm_SemGive(vscheme->semaphore);
	}

	/*	Return the first fragment to be enqueued per plan.	*/

	*bundleObj = firstBundleObj;
	memcpy((char *) bundle, (char *) &firstBundle, sizeof(Bundle));
	return 0;
}

static int	enqueueToEntryNode(CgrRoute *route, Bundle *bundle,
			Object bundleObj, IonNode *terminusNode)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();
	BpEvent		event;
	char		neighborEid[MAX_EID_LEN + 1];
	VPlan		*vplan;
	PsmAddress	vplanElt;
	int		priority;
	PsmAddress	elt;
	PsmAddress	addr;
	IonCXref	*contact;
	Object		contactObj;
	IonContact	contactBuf;
	int		i;

	if (bundle->ovrdPending)
	{
		bindOverride(bundle, bundleObj, route->toNodeNbr);
	}

	/*	Note that a copy is being sent on the route through
	 *	this neighbor.						*/

	if (bundle->xmitCopiesCount == MAX_XMIT_COPIES)
	{
		return 0;	/*	Reached forwarding limit.	*/
	}

	bundle->xmitCopies[bundle->xmitCopiesCount] = route->toNodeNbr;
	bundle->xmitCopiesCount++;
	bundle->dlvConfidence = cgr_get_dlv_confidence(bundle, route);

	/*	If the bundle is NOT critical, then:			*/

	if (!(bundle->ancillaryData.flags & BP_MINIMUM_LATENCY))
	{
		/*	We may need to do anticipatory fragmentation
		 *	of the bundle before enqueuing it for
		 *	transmission.					*/

		if (route->maxVolumeAvbl < route->bundleECCC
		&& bundle->payload.length > 1
		&& !(bundle->bundleProcFlags & BDL_DOES_NOT_FRAGMENT))
		{
#if 0
printf("*** fragmenting; to node %lu, volume avbl %lu, bundle ECCC %lu.\n", route->toNodeNbr, route->maxVolumeAvbl, route->bundleECCC);
#endif
			if (proactivelyFragment(bundle, &bundleObj, route) < 0)
			{
				putErrmsg("Anticipatory fragmentation failed.",
						NULL);
				return -1;
			}
		}

		/*	In any case, we need to post an xmitOverdue
		 *	timeout event to trigger re-forwarding in case
		 *	the bundle doesn't get transmitted during the
		 *	contact in which we expect that to happen.	*/

		event.type = xmitOverdue;
		addr = sm_list_data(ionwm, sm_list_first(ionwm, route->hops));
		contact = (IonCXref *) psp(ionwm, addr);
		event.time = contact->toTime;
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
	 *	Since we've already determined that the plan to this
	 *	neighbor is not blocked (else the route would not
	 *	be in the list of best routes), the bundle can't go
	 *	into limbo at this point.				*/

	isprintf(neighborEid, sizeof neighborEid, "ipn:" UVAST_FIELDSPEC ".0",
			route->toNodeNbr);
	findPlan(neighborEid, &vplan, &vplanElt);
	CHKERR(vplanElt);
	if (bpEnqueue(vplan, bundle, bundleObj) < 0)
	{
		putErrmsg("Can't enqueue bundle.", NULL);
		return -1;
	}

	/*	And we reserve transmission volume for this bundle
	 *	on every contact along the end-to-end path for the
	 *	bundle.							*/

	priority = bundle->priority;
	for (elt = sm_list_first(ionwm, route->hops); elt;
			elt = sm_list_next(ionwm, elt))
	{
		addr = sm_list_data(ionwm, elt);
		contact = (IonCXref *) psp(ionwm, addr);
		contactObj = sdr_list_data(sdr, contact->contactElt);
		sdr_stage(sdr, (char *) &contactBuf, contactObj,
				sizeof(IonContact));
		for (i = priority; i >= 0; i--)
		{
			contactBuf.mtv[i] -= route->bundleECCC;
		}

		sdr_write(sdr, contactObj, (char *) &contactBuf,
				sizeof(IonContact));
	}

	return 0;
}

#if (MANAGE_OVERBOOKING == 1)
typedef struct
{
	Object	currentElt;	/*	SDR list element.		*/
	Object	limitElt;	/*	SDR list element.		*/
} QueueControl;

static Object	getUrgentLimitElt(BpPlan *plan, int ordinal)
{
	Sdr	sdr = getIonsdr();
	int	i;
	Object	limitElt;

	/*	Find last bundle enqueued for the lowest ordinal
	 *	value that is higher than the bundle's ordinal;
	 *	limit elt is the next bundle in the urgent queue
	 *	following that one (i.e., the first enqueued for
	 *	the bundle's ordinal).  If none, then the first
	 *	bundle in the urgent queue is the limit elt.		*/

	for (i = ordinal + 1; i < 256; i++)
	{
		limitElt = plan->ordinals[i].lastForOrdinal;
		if (limitElt)
		{
			return sdr_list_next(sdr, limitElt);
		}
	}

	return sdr_list_first(sdr, plan->urgentQueue);
}

static Object	nextBundle(QueueControl *queueControls, int *queueIdx)
{
	Sdr		sdr = getIonsdr();
	QueueControl	*queue;
	Object		currentElt;

	queue = queueControls + *queueIdx;
	while (queue->currentElt == 0)
	{
		(*queueIdx)++;
		if ((*queueIdx) > BP_EXPEDITED_PRIORITY)
		{
			return 0;
		}

		queue++;
	}

	currentElt = queue->currentElt;
	if (currentElt == queue->limitElt)
	{
		queue->currentElt = 0;
	}
	else
	{
		queue->currentElt = sdr_list_prev(sdr, queue->currentElt);
	}

	return currentElt;
}

static int	manageOverbooking(CgrRoute *route, Bundle *newBundle,
			CgrTrace *trace)
{
	Sdr		sdr = getIonsdr();
	char		neighborEid[MAX_EID_LEN + 1];
	VPlan		*vplan;
	PsmAddress	vplanElt;
	Object		planObj;
	BpPlan		plan;
	QueueControl	queueControls[] = { {0, 0}, {0, 0}, {0, 0} };
	int		queueIdx = 0;
	int		priority;
	int		ordinal;
	double		protected = 0.0;
	double		overbooked = 0.0;
	Object		elt;
	Object		bundleObj;
	Bundle		bundle;
	int		eccc;

	isprintf(neighborEid, sizeof neighborEid, "ipn:" UVAST_FIELDSPEC ".0",
			route->toNodeNbr);
	priority = newBundle->priority;
	if (priority == 0)
	{
		/*	New bundle's priority is Bulk, can't possibly
		 *	bump any other bundles.				*/

		return 0;
	}

	overbooked += (ONE_GIG * route->overbooked.gigs)
			+ route->overbooked.units;
	if (overbooked == 0.0)
	{
		return 0;	/*	No overbooking to manage.	*/
	}

	protected += (ONE_GIG * route->committed.gigs)
			+ route->committed.units;
	if (protected == 0.0)
	{
		TRACE(CgrPartialOverbooking, overbooked);
	}
	else
	{
		TRACE(CgrFullOverbooking, overbooked);
	}

	findPlan(neighborEid, &vplan, &vplanElt);
	if (vplanElt == 0)
	{
		TRACE(CgrSkipRoute, CgrNoPlan);

		return 0;		/*	No egress plan to node.	*/
	}

	planObj = sdr_list_data(sdr, vplan->planElt);
	sdr_read(sdr, (char *) &plan, planObj, sizeof(BpPlan));
	queueControls[0].currentElt = sdr_list_last(sdr, plan.bulkQueue);
	queueControls[0].limitElt = sdr_list_first(sdr, plan.bulkQueue);
	if (priority > 1)
	{
		queueControls[1].currentElt = sdr_list_last(sdr,
				plan.stdQueue);
		queueControls[1].limitElt = sdr_list_first(sdr,
				plan.stdQueue);
		ordinal = newBundle->ordinal;
		if (ordinal > 0)
		{
			queueControls[2].currentElt = sdr_list_last(sdr,
					plan.urgentQueue);
			queueControls[2].limitElt = getUrgentLimitElt(&plan,
					ordinal);
		}
	}

	while (overbooked > 0.0)
	{
		elt = nextBundle(queueControls, &queueIdx);
		if (elt == 0)
		{
			break;
		}

		bundleObj = sdr_list_data(sdr, elt);
		sdr_stage(sdr, (char *) &bundle, bundleObj, sizeof(Bundle));
		eccc = computeECCC(guessBundleSize(&bundle));

		/*	Skip over all bundles that are protected
		 *	from overbooking because they are in contacts
		 *	following the contact in which the new bundle
		 *	is scheduled for transmission.			*/

		if (protected > 0.0)
		{
			protected -= eccc;
			continue;
		}

		/*	The new bundle has bumped this bundle out of
		 *	its originally scheduled contact.  Rebook it.	*/

		sdr_stage(sdr, (char *) &plan, planObj, sizeof(BpPlan));
		removeBundleFromQueue(&bundle, &plan);
		sdr_write(sdr, planObj, (char *) &plan, sizeof(BpPlan));
		sdr_write(sdr, bundleObj, (char *) &bundle, sizeof(Bundle));
		if (bpReforwardBundle(bundleObj) < 0)
		{
			putErrmsg("Overbooking management failed.", NULL);
			return -1;
		}

		overbooked -= eccc;
	}

	return 0;
}
#endif

static int	proxNodeRedundant(Bundle *bundle, vast nodeNbr)
{
	int	i;

	for (i = 0; i < bundle->xmitCopiesCount; i++)
	{
		if (bundle->xmitCopies[i] == nodeNbr)
		{
			return 1;
		}
	}

	return 0;
}

static int	sendCriticalBundle(Bundle *bundle, Object bundleObj,
			IonNode *terminusNode, Lyst bestRoutes, int preview)
{
	PsmPartition	ionwm = getIonwm();
	CgrRtgObject	*routingObject;
	LystElt		elt;
	LystElt		nextElt;
	CgrRoute	*route;
	Bundle		newBundle;
	Object		newBundleObj;

	/*	Enqueue the bundle on the plan for the entry node of
	 *	EACH identified best route.				*/

	for (elt = lyst_first(bestRoutes); elt; elt = nextElt)
	{
		nextElt = lyst_next(elt);
		route = (CgrRoute *) lyst_data_set(elt, NULL);
		lyst_delete(elt);
		if (preview)
		{
			continue;
		}

		if (proxNodeRedundant(bundle, route->toNodeNbr))
		{
			continue;
		}

		if (bundle->planXmitElt)
		{
			/*	This copy of bundle has already
			 *	been enqueued.				*/

			if (bpClone(bundle, &newBundle, &newBundleObj, 0, 0)
					< 0)
			{
				putErrmsg("Can't clone bundle.", NULL);
				lyst_destroy(bestRoutes);
				return -1;
			}

			bundle = &newBundle;
			bundleObj = newBundleObj;
		}

		if (enqueueToEntryNode(route, bundle, bundleObj, terminusNode))
		{
			putErrmsg("Can't queue for neighbor.", NULL);
			lyst_destroy(bestRoutes);
			return -1;
		}
	}

	lyst_destroy(bestRoutes);
	if (bundle->dlvConfidence >= MIN_NET_DELIVERY_CONFIDENCE
	|| bundle->id.source.ssp.ipn.nodeNbr
			== bundle->destination.ssp.ipn.nodeNbr)
	{
		return 0;	/*	Potential future fwd unneeded.	*/
	}

	routingObject = (CgrRtgObject *) psp(ionwm,
			terminusNode->routingObject);
	if (routingObject == 0
	|| (sm_list_length(ionwm, routingObject->selectedRoutes) == 0
	&& sm_list_length(ionwm, routingObject->knownRoutes) == 0))
	{
		return 0;	/*	No potential future forwarding.	*/
	}

	/*	Must put bundle in limbo, keep on trying to send it.	*/

	if (bundle->planXmitElt)
	{
		/*	This copy of bundle has already been enqueued.	*/

		if (bpClone(bundle, &newBundle, &newBundleObj, 0, 0) < 0)
		{
			putErrmsg("Can't clone bundle.", NULL);
			return -1;
		}

		bundle = &newBundle;
		bundleObj = newBundleObj;
	}

	if (enqueueToLimbo(bundle, bundleObj) < 0)
	{
		putErrmsg("Can't put bundle in limbo.", NULL);
		return -1;
	}

	return 0;
}

static unsigned char	initializeSnw(unsigned int ttl, uvast toNode)
{
	/*	Compute spray-and-wait "L" value.  The only required
	 *	parameters are the required expected delay "aEDopt"
	 *	and the number of nodes "M".  Expected delay is
	 *	computed as the product of the delay constraint "a"
	 *	(we choose 8 for this value), the expected delay for
	 *	direct transmission (1 second), and the TTL less
	 *	some margin for safety (we discount by 1/8) -- so
	 *	7 * TTL.  The number of nodes is the length of the
	 *	list of members for the region in which the local
	 *	node and the initial contact's "to" node both reside.
	 *
	 *	The computation is very complex, left for later.	*/

	return 16;	/*	Dummy result, for now.			*/
}

static int	forwardOkay(CgrRoute *route, Bundle *bundle)
{
	PsmPartition	ionwm = getIonwm();
	PsmAddress	hopsElt;
	PsmAddress	contactAddr;
	IonCXref	*contact;

	hopsElt = sm_list_first(ionwm, route->hops);
	contactAddr = sm_list_data(ionwm, hopsElt);
	contact = (IonCXref *) psp(ionwm, contactAddr);
	if (contact->type != CtDiscovered)
	{
		return 1;	/*	No Spray and Wait rule applies.	*/
	}

	/*	Discovered contact, must check Spray and Wait.		*/

	if (bundle->permits == 0)	/*	Not sprayed yet.	*/
	{
		bundle->permits = initializeSnw(bundle->timeToLive,
				contact->toNode);
	}

	if (bundle->permits < 2)	/*	(Should never be 0.)	*/
	{
		/*	When SNW permits count is 1 (or 0), the bundle
		 *	can only be forwarded to the final destination
		 *	node.						*/

		if (contact->toNode != bundle->destination.ssp.ipn.nodeNbr)
		{
			return 0;
		}
	}

	return 1;
}

static int 	tryCGR(Bundle *bundle, Object bundleObj, IonNode *terminusNode,
			time_t atTime, CgrTrace *trace, int preview)
{
	PsmPartition	ionwm = getIonwm();
	IonVdb		*ionvdb = getIonVdb();
	CgrVdb		*cgrvdb = cgr_get_vdb();
	int		ionMemIdx;
	Lyst		bestRoutes;
	Lyst		excludedNodes;
	LystElt		elt;
	CgrRoute	*route;
	PsmAddress	routingObjectAddr;
	CgrRtgObject	*routingObject;
	Bundle		newBundle;
	Object		newBundleObj;

	/*	Determine whether or not the contact graph for the
	 *	terminus node identifies one or more routes over
	 *	which the bundle may be sent in order to get it
	 *	delivered to the terminus node.  If so, use the
	 *	Plan asserted for the entry node of each of the
	 *	best route ("dynamic route").
	 *
	 *	Note that CGR can be used to compute a route to an
	 *	intermediate "station" node selected by another
	 *	routing mechanism (such as static routing), not
	 *	only to the bundle's final destination node.  In
	 *	the simplest case, the bundle's destination is the
	 *	only "station" selected for the bundle.  To avoid
	 *	confusion, we here use the term "terminus" to refer
	 *	to the node to which a route is being computed,
	 *	regardless of whether that node is the bundle's
	 *	final destination or an intermediate forwarding
	 *	station.			 			*/

	CHKERR(bundle && bundleObj && terminusNode);
	TRACE(CgrBuildRoutes, terminusNode->nodeNbr, bundle->payload.length,
			(unsigned int)(atTime));

	if (ionvdb->lastEditTime.tv_sec > cgrvdb->lastLoadTime.tv_sec
	|| (ionvdb->lastEditTime.tv_sec == cgrvdb->lastLoadTime.tv_sec
	    && ionvdb->lastEditTime.tv_usec > cgrvdb->lastLoadTime.tv_usec)) 
	{
		/*	Contact plan has been modified, so must discard
		 *	all route lists and reconstruct them as needed.	*/

		cgr_clear_vdb(cgrvdb);
		getCurrentTime(&(cgrvdb->lastLoadTime));
	}

	ionMemIdx = getIonMemoryMgr();
	bestRoutes = lyst_create_using(ionMemIdx);
	excludedNodes = lyst_create_using(ionMemIdx);
	if (bestRoutes == NULL || excludedNodes == NULL)
	{
		putErrmsg("Can't create lists for route computation.", NULL);
		return -1;
	}

	lyst_delete_set(bestRoutes, deleteObject, NULL);
	lyst_delete_set(excludedNodes, deleteObject, NULL);
	if (!bundle->returnToSender)
	{
		/*	Must exclude sender of bundle from consideration
		 *	as a station on the route, to minimize routing
		 *	loops.  If returnToSender is 1 then we are
		 *	re-routing, possibly back through the sender,
		 *	because we have hit a dead end in routing and
		 *	must backtrack.					*/

		if (excludeNode(excludedNodes, bundle->clDossier.senderNodeNbr))
		{
			putErrmsg("Can't exclude sender from routes.", NULL);
			lyst_destroy(excludedNodes);
			lyst_destroy(bestRoutes);
			return -1;
		}
	}

	/*	Consult the contact graph to identify the neighboring
	 *	node(s) to forward the bundle to.			*/

	if (terminusNode->routingObject == 0)
	{
		if (cgr_create_routing_object(terminusNode) < 0)
		{
			putErrmsg("Can't initialize routing object.", NULL);
			return -1;
		}
	}

	if (cgr_identify_best_routes(terminusNode, bundle, bundleObj,
			excludedNodes, trace, bestRoutes, atTime) < 0)
	{
		putErrmsg("Can't identify best route(s) for bundle.", NULL);
		lyst_destroy(excludedNodes);
		lyst_destroy(bestRoutes);
		return -1;
	}

	lyst_destroy(excludedNodes);
	TRACE(CgrSelectRoutes);
	if (bundle->ancillaryData.flags & BP_MINIMUM_LATENCY)
	{
		/*	Critical bundle; send to all capable neighbors.	*/

		TRACE(CgrUseAllRoutes);
		return sendCriticalBundle(bundle, bundleObj, terminusNode,
				bestRoutes, preview);
	}

	/*	Non-critical bundle; send to the most preferred
	 *	neighbor.						*/

	elt = lyst_first(bestRoutes);
	if (elt)
	{
		route = (CgrRoute *) lyst_data_set(elt, NULL);
		TRACE(CgrUseRoute, route->toNodeNbr);
		if (!preview && forwardOkay(route, bundle))
		{
			if (enqueueToEntryNode(route, bundle, bundleObj,
					terminusNode))
			{
				putErrmsg("Can't queue for neighbor.", NULL);
				return -1;
			}

#if (MANAGE_OVERBOOKING == 1)
			/*	Handle any contact overbooking caused
			 *	by enqueuing this bundle.		*/

			if (manageOverbooking(route, bundle, trace))
			{
				putErrmsg("Can't manage overbooking", NULL);
				return -1;
			}
#endif
		}
	}
	else
	{
		TRACE(CgrNoRoute);
	}

	lyst_destroy(bestRoutes);
	if (bundle->dlvConfidence >= MIN_NET_DELIVERY_CONFIDENCE
	|| bundle->id.source.ssp.ipn.nodeNbr
			== bundle->destination.ssp.ipn.nodeNbr)
	{
		return 0;	/*	Potential future fwd unneeded.	*/
	}

	routingObjectAddr = terminusNode->routingObject;
	if (routingObjectAddr == 0)
	{
		return 0;	/*	No potential future forwarding.	*/
	}

	routingObject = (CgrRtgObject *) psp(ionwm, routingObjectAddr);
	if (sm_list_length(ionwm, routingObject->selectedRoutes) == 0)
	{
		return 0;	/*	No potential future forwarding.	*/
	}

	/*	Must put bundle in limbo, keep on trying to send it.	*/

	if (bundle->planXmitElt)
	{
		/*	This copy of bundle has already been enqueued.	*/

		if (bpClone(bundle, &newBundle, &newBundleObj, 0, 0) < 0)
		{
			putErrmsg("Can't clone bundle.", NULL);
			return -1;
		}

		bundle = &newBundle;
		bundleObj = newBundleObj;
	}

	if (enqueueToLimbo(bundle, bundleObj) < 0)
	{
		putErrmsg("Can't put bundle in limbo.", NULL);
		return -1;
	}

	return 0;
}

/*		Contingency functions for when CGR and HIRR don't work.	*/

static int	enqueueToNeighbor(Bundle *bundle, Object bundleObj,
			uvast nodeNbr)
{
	Sdr		sdr = getIonsdr();
	char		eid[MAX_EID_LEN + 1];
	VPlan		*vplan;
	PsmAddress	vplanElt;
	BpPlan		plan;

	isprintf(eid, sizeof eid, "ipn:" UVAST_FIELDSPEC ".0", nodeNbr);
	findPlan(eid, &vplan, &vplanElt);
	if (vplanElt == 0)
	{
		return 0;
	}

	sdr_read(sdr, (char *) &plan, sdr_list_data(sdr, vplan->planElt),
			sizeof(BpPlan));
	if (plan.blocked)
	{
		if (enqueueToLimbo(bundle, bundleObj) < 0)
		{
			putErrmsg("Can't put bundle in limbo.", NULL);
			return -1;
		}
	}
	else
	{
		if (bpEnqueue(vplan, bundle, bundleObj) < 0)
		{
			putErrmsg("Can't enqueue bundle.", NULL);
			return -1;
		}
	}

	return 0;
}

/*		Top-level ipnfw functions				*/

static int	enqueueBundle(Bundle *bundle, Object bundleObj)
{
	Sdr		sdr = getIonsdr();
	IonVdb		*ionvdb = getIonVdb();
	Object		elt;
	char		eid[SDRSTRING_BUFSZ];
	MetaEid		metaEid;
	uvast		nodeNbr;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	IonNode		*node;
	PsmAddress	nextNode;
#if CGR_DEBUG == 1
	CgrTrace	*trace = &(CgrTrace) { .fn = printCgrTraceLine };
#else
	CgrTrace	*trace = NULL;
#endif

	elt = sdr_list_first(sdr, bundle->stations);
	if (elt == 0)
	{
		putErrmsg("Forwarding error; stations stack is empty.", NULL);
		return -1;
	}

	sdr_string_read(sdr, eid, sdr_list_data(sdr, elt));
	if (parseEidString(eid, &metaEid, &vscheme, &vschemeElt) == 0)
	{
		putErrmsg("Can't parse node EID string.", eid);
		return bpAbandon(bundleObj, bundle, BP_REASON_NO_ROUTE);
	}

	if (strcmp(vscheme->name, "ipn") != 0)
	{
		putErrmsg("Forwarding error; EID scheme is not 'ipn'.",
				vscheme->name);
		return -1;
	}

	nodeNbr = metaEid.elementNbr;
	restoreEidString(&metaEid);

	/*	Apply routing override, if any.				*/

	if (applyRoutingOverride(bundle, bundleObj, nodeNbr) < 0)
	{
		putErrmsg("Can't send bundle to override neighbor.", NULL);
		return -1;
	}

	/*	If override routing succeeded in enqueuing the bundle
	 *	to a neighbor, accept the bundle and return.		*/

	if (bundle->planXmitElt)
	{
		/*	Enqueued.					*/

		return bpAccept(bundleObj, bundle);
	}

	/*	No applicable override.  Try dynamic routing.		*/

	node = findNode(ionvdb, nodeNbr, &nextNode);
	if (node == NULL)
	{
		node = addNode(ionvdb, nodeNbr);
		if (node == NULL)
		{
			putErrmsg("Can't add node.", NULL);
			return -1;
		}
	}

	if (ionRegionOf(nodeNbr, 0) < 0)
	{
		/*	Destination node is not in any region that
		 *	the local node is in.  Send via passageway(s).	*/

		if (tryHIRR(bundle, bundleObj, node, getCtime()))
		{
			putErrmsg("HIRR failed.", NULL);
			return -1;
		}
	}
	else
	{
		/*	Destination node resides in a region in which
		 *	the local node resides.  Consult contact plan.	*/

		if (tryCGR(bundle, bundleObj, node, getCtime(), trace, 0))
		{
			putErrmsg("CGR failed.", NULL);
			return -1;
		}
	}

	/*	If dynamic routing succeeded in enqueuing the bundle
	 *	to a neighbor, accept the bundle and return.		*/

	if (bundle->planXmitElt)
	{
		/*	Enqueued.					*/

		return bpAccept(bundleObj, bundle);
	}

	/*	No luck using the contact graph or region tree to
	 *	compute a route to the destination node.  So see if
	 *	destination node is a neighbor (not identified in the
	 *	contact plan); if so, enqueue for direct transmission.	*/

	if (enqueueToNeighbor(bundle, bundleObj, nodeNbr) < 0)
	{
		putErrmsg("Can't send bundle to neighbor.", NULL);
		return -1;
	}

	if (bundle->planXmitElt)
	{
		/*	Enqueued.					*/

		return bpAccept(bundleObj, bundle);
	}

	/*	No egress plan for direct transmission to destination
	 *	node.  So look for the narrowest applicable static
	 *	route (node range, i.e., "exit") and forward to the
	 *	prescribed "via" endpoint for that exit.		*/

	if (ipn_lookupExit(nodeNbr, eid) == 1)
	{
		/*	Found applicable exit; forward via the
		 *	indicated endpoint.				*/

		sdr_write(sdr, bundleObj, (char *) &bundle, sizeof(Bundle));
		return forwardBundle(bundleObj, bundle, eid);
	}

	/*	No applicable exit.  If there's at least a route
	 *	that might work if some hypothetical contact should
	 *	materialize, we place the bundle in limbo and hope
	 *	for the best.						*/

	if (cgr_prospect(nodeNbr, bundle->expirationTime + EPOCH_2000_SEC) > 0)
	{
		if (enqueueToLimbo(bundle, bundleObj) < 0)
		{
			putErrmsg("Can't put bundle in limbo.", NULL);
			return -1;
		}
	}

	if (bundle->planXmitElt)
	{
		/*	Bundle was enqueued to limbo.			*/

		return bpAccept(bundleObj, bundle);
	}

	return bpAbandon(bundleObj, bundle, BP_REASON_NO_ROUTE);
}

#if defined (ION_LWT)
int	ipnfw(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
#else
int	main(int argc, char *argv[])
{
#endif
	int		running = 1;
	Sdr		sdr;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	Scheme		scheme;
	Object		elt;
	Object		bundleAddr;
	Bundle		bundle;
	Object		ovrdAddr;
	IpnOverride	ovrd;

	if (bpAttach() < 0)
	{
		putErrmsg("ipnfw can't attach to BP.", NULL);
		return 1;
	}

	if (ipnInit() < 0)
	{
		putErrmsg("ipnfw can't load routing database.", NULL);
		return 1;
	}

	cgr_start();
	sdr = getIonsdr();
	findScheme("ipn", &vscheme, &vschemeElt);
	if (vschemeElt == 0)
	{
		putErrmsg("'ipn' scheme is unknown.", NULL);
		return 1;
	}

	CHKZERO(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &scheme, sdr_list_data(sdr,
			vscheme->schemeElt), sizeof(Scheme));
	sdr_exit_xn(sdr);
	oK(_ipnfwSemaphore(&vscheme->semaphore));
	isignal(SIGTERM, shutDown);

	/*	Main loop: wait until forwarding queue is non-empty,
	 *	then drain it.						*/

	writeMemo("[i] ipnfw is running.");
	while (running && !(sm_SemEnded(vscheme->semaphore)))
	{
		/*	Wrapping forwarding in an SDR transaction
		 *	prevents race condition with bpclock (which
		 *	is destroying bundles as their TTLs expire).	*/

		CHKZERO(sdr_begin_xn(sdr));
		elt = sdr_list_first(sdr, scheme.forwardQueue);
		if (elt == 0)	/*	Wait for forwarding notice.	*/
		{
			sdr_exit_xn(sdr);
			if (sm_SemTake(vscheme->semaphore) < 0)
			{
				putErrmsg("Can't take forwarder semaphore.",
						NULL);
				running = 0;
			}

			continue;
		}

		bundleAddr = (Object) sdr_list_data(sdr, elt);
		sdr_stage(sdr, (char *) &bundle, bundleAddr, sizeof(Bundle));

		/*	Note any applicable class of service override.	*/

		bundle.priority = bundle.classOfService;
		bundle.ordinal = bundle.ancillaryData.ordinal;
		if (ipn_lookupOvrd(bundle.ancillaryData.dataLabel,
				bundle.id.source.ssp.ipn.nodeNbr,
				bundle.destination.ssp.ipn.nodeNbr, &ovrdAddr))
		{
			sdr_read(sdr, (char *) &ovrd, ovrdAddr,
					sizeof(IpnOverride));
			if (ovrd.priority != (unsigned char) -1)
			{
				/*	Override requested CoS.		*/

				bundle.priority = ovrd.priority;
				bundle.ordinal = ovrd.ordinal;
			}
		}

		/*	Remove bundle from queue.			*/

		sdr_list_delete(sdr, elt, NULL, NULL);
		bundle.fwdQueueElt = 0;

		/*	Must rewrite bundle to note removal of
		 *	fwdQueueElt, in case the bundle is abandoned
		 *	and bpDestroyBundle re-reads it from the
		 *	database.					*/

		sdr_write(sdr, bundleAddr, (char *) &bundle, sizeof(Bundle));
		if (enqueueBundle(&bundle, bundleAddr) < 0)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("Can't enqueue bundle.", NULL);
			running = 0;	/*	Terminate loop.		*/
			continue;
		}

		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Can't enqueue bundle.", NULL);
			running = 0;	/*	Terminate loop.		*/
		}

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();
	}

	writeErrmsgMemos();
	writeMemo("[i] ipnfw forwarder has ended.");
	ionDetach();
	return 0;
}
