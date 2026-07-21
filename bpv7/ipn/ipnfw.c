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
#include "bei.h"	/* For findExtensionBlock */
#include "cbr.h"	/* For CBR_BLOCK_TYPE_CTEB */
#include "cbdedup.h"	/* Critical-bundle forward duplication guard. */

#ifdef	ION_BANDWIDTH_RESERVED
#define	MANAGE_OVERBOOKING	0
#endif

#ifndef	MANAGE_OVERBOOKING
#define	MANAGE_OVERBOOKING	1
#endif

#ifndef	MIN_PROSPECT
#define	MIN_PROSPECT		(0.0)
#endif

#ifndef CGR_DEBUG
#define CGR_DEBUG		0
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
	switch (traceType)
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
	fflush(stdout);
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

static CgrSAP	cgrSap(CgrSAP *newSap)
{
	static CgrSAP	sap;

	if (newSap)
	{
		sap = *newSap;
	}

	return sap;
}

static void	shutDown(int signum)
{
	/* Tell the compiler that we are not using 'signum' */
	(void)signum;

	isignal(SIGTERM, shutDown);
	sm_SemEnd(_ipnfwSemaphore(NULL));
}

/*	#1047 instrumentation.  ipnfw was observed to exit silently while
 *	holding the SDR lock mid-transaction (no stack, no core).  These
 *	breadcrumbs make the next occurrence legible: the atexit reporter
 *	prints a stack and the last bundle being forwarded if ipnfw leaves
 *	by any path other than the normal forwarder shutdown, and the main
 *	loop logs any transaction that holds the lock abnormally long.	*/

#ifndef	IPNFW_SLOW_HOLD_USEC
#define	IPNFW_SLOW_HOLD_USEC	(50000)		/*	50 ms.		*/
#endif

static int		ipnfwShutdownClean = 0;
static uvast		ipnfwCurrentDest = 0;
static unsigned int	ipnfwCurrentSize = 0;

static void	ipnfwReportExit(void)
{
	if (ipnfwShutdownClean)
	{
		return;		/*	Normal forwarder shutdown.	*/
	}

	writeMemoNote("[?] ipnfw is exiting via an UNEXPECTED path (not the \
normal forwarder shutdown); last bundle forwarded was to destination node",
			uvasttoa(ipnfwCurrentDest));
	writeMemoNote("[?] ipnfw last forwarded bundle payload size (bytes)",
			itoa((int) ipnfwCurrentSize));
	printStackTrace();
	writeErrmsgMemos();
}

/*		CGR override functions.					*/

static int applyRoutingOverride(Bundle *bundle, SdrObject bundleObj, uvast fqnn)
{
	Sdr		sdr = getIonsdr();
	char		nbrBuf[FQN_MAX_LENGTH];
	char		eid[MAX_EID_LEN + 1];
	VPlan		*vplan;
	PsmAddress	vplanElt;
	BpPlan		plan;

	/* Parameter intentionally unused. */
	(void)fqnn;

	if (bundle->ovrdNeighbor == 0)
	{
		return 0;
	}

	/*	Must forward to override neighbor.			*/

	putFqn(nbrBuf, bundle->ovrdNeighbor);
	isprintf(eid, sizeof eid, "ipn:%s.0", nbrBuf);
	findPlan(eid, &vplan, &vplanElt);
	if (vplanElt == 0)	/*	Not a usable override.		*/
	{
		return 0;
	}

	sdr_read(sdr, (char *) &plan, sdr_list_data(sdr, vplan->planElt),
			sizeof(BpPlan));
	if (plan.viaEid)	/*	Potential loop.			*/
	{
		writeMemoNote("[?] Routing override to this neighbor selects \
an egress plan that redirects to another EID; potential forwarding loop", eid);
		return 0;
	}

	if (plan.blocked)	/*	Maybe later.			*/
	{
		if (enqueueToLimbo(bundle, bundleObj) < 0)
		{
			putErrmsg("Can't put bundle in limbo.", NULL);
			return -1;
		}

		return 0;
	}

	/*	Forward per the neighbor override.			*/

	if (bpEnqueue(vplan, bundle, bundleObj) < 0)
	{
		putErrmsg("Can't enqueue bundle.", NULL);
		return -1;
	}

	return 0;
}

/*		CGR invocation functions.				*/

static void	deleteObject(LystElt elt, void *userdata)
{
	void	*object = lyst_data(elt);

	/* Parameter intentionally unused. */
	(void)userdata;

	if (object)
	{
		MRELEASE(object);
	}
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

static int proactivelyFragment(Bundle *bundle, SdrObject *bundleObj,
		CgrRoute *route)
{
	Sdr		sdr = getIonsdr();
	SdrObject	stationEidElt;
	SdrObject	stationEid;
	char		eid[SDRSTRING_BUFSZ];
	MetaEid		stationMetaEid;
	VScheme		*vscheme = NULL;
	PsmAddress	vschemeElt;
	size_t		fragmentLength;
	Bundle		firstBundle;
	SdrObject	firstBundleObj;
	Bundle		secondBundle;
	SdrObject	secondBundleObj;
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
		putErrmsg("Bad station EID", eid);
		return -1;
	}

	fragmentLength = carryingCapacity(route->maxVolumeAvbl);
	if (fragmentLength == 0)
	{
		fragmentLength = 1;	/*	Assume rounding error.	*/
	}

	if (bundle->payload.length < 0 || fragmentLength >= (size_t)bundle->payload.length)
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

	clearMetaEid(&stationMetaEid);
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

static int enqueueToEntryNode(CgrRoute *route, Bundle *bundle,
		SdrObject bundleObj, IonNode *terminusNode)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();
	BpEvent		event;
	char		nbrBuf[FQN_MAX_LENGTH];
	char		neighborEid[MAX_EID_LEN + 1];
	VPlan		*vplan;
	PsmAddress	vplanElt;
	int		priority;
	PsmAddress	elt;
	PsmAddress	addr;
	IonCXref	*contact;
	SdrObject	contactObj;
	IonContact	contactBuf;
	int		i;

	/* Parameter intentionally unused. */
	(void)terminusNode;

	/*	Note that a copy is being sent on the route through
	 *	this neighbor.						*/

	if (bundle->xmitCopiesCount == MAX_XMIT_COPIES)
	{
		return 0;	/*	Reached forwarding limit.	*/
	}

	bundle->xmitCopies[bundle->xmitCopiesCount] = route->toFqnn;
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

	putFqn(nbrBuf, route->toFqnn);
	isprintf(neighborEid, sizeof neighborEid, "ipn:%s.0", nbrBuf);
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
	SdrObject currentElt; /* SDR list element. */
	SdrObject limitElt;   /* SDR list element. */
} QueueControl;

static SdrObject getUrgentLimitElt(BpPlan *plan, int ordinal)
{
	Sdr	sdr = getIonsdr();
	int	i;
	SdrObject limitElt;

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

static SdrObject nextBundle(QueueControl *queueControls, int *queueIdx)
{
	Sdr		sdr = getIonsdr();
	QueueControl	*queue;
	SdrObject	currentElt;

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
	char		nbrBuf[FQN_MAX_LENGTH];
	char		neighborEid[MAX_EID_LEN + 1];
	VPlan		*vplan;
	PsmAddress	vplanElt;
	SdrObject	planObj;
	BpPlan		plan;
	QueueControl	queueControls[] = { {0, 0}, {0, 0}, {0, 0} };
	int		queueIdx = 0;
	int		priority;
	int		ordinal;
	double		protected = 0.0;
	double		overbooked = 0.0;
	SdrObject	elt;
	SdrObject	bundleObj;
	Bundle		bundle;
	int		eccc;

	putFqn(nbrBuf, route->toFqnn);
	isprintf(neighborEid, sizeof neighborEid, "ipn:%s.0", nbrBuf);
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

static int	proxNodeRedundant(Bundle *bundle, vast fqnn)
{
	int	i;

	for (i = 0; i < bundle->xmitCopiesCount; i++)
	{
		if (fqnn >= 0 && bundle->xmitCopies[i] == (uvast)fqnn)
		{
			return 1;
		}
	}

	return 0;
}

static int sendCriticalBundle(Bundle *bundle, SdrObject bundleObj,
		IonNode *terminusNode, Lyst bestRoutes, int potential,
		int preview)
{
	LystElt		elt;
	LystElt		nextElt;
	CgrRoute	*route;
	Bundle		newBundle;
	SdrObject	newBundleObj;
	int		enqueued = 0;

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

		if (proxNodeRedundant(bundle, route->toFqnn))
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

		enqueued = 1;
	}

	lyst_destroy(bestRoutes);
	if (enqueued)
	{
		oK(cbdedup_record(bundle));
	}

	if (bundle->dlvConfidence >= MIN_NET_DELIVERY_CONFIDENCE
	|| bundle->id.source.ssp.ipn.fqnn
			== bundle->destination.ssp.ipn.fqnn)
	{
		return 0;	/*	Potential future fwd unneeded.	*/
	}

	if (potential == 0)
	{
		return 0; 	/*	No potential future forwarding.	*/
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

static unsigned char	initializeSnw(unsigned int ttl, uvast toFqnn)
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

	/* Parameters intentionally unused. */
	(void)ttl;
	(void)toFqnn;

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
				contact->toFqnn);
	}

	if (bundle->permits < 2)	/*	(Should never be 0.)	*/
	{
		/*	When SNW permits count is 1 (or 0), the bundle
		 *	can only be forwarded to the final destination
		 *	node.						*/

		if (contact->toFqnn != bundle->destination.ssp.ipn.fqnn)
		{
			return 0;
		}
	}

	return 1;
}

static int tryCGR(Bundle *bundle, SdrObject bundleObj, IonNode *terminusNode,
		time_t atTime, CgrTrace *trace, int preview)
{
	IonVdb		*ionvdb = getIonVdb();
	CgrVdb		*cgrvdb = cgr_get_vdb();
	int		ionMemIdx;
	Lyst		bestRoutes;
	Lyst		excludedNodes;
	int		potential;
	LystElt		elt;
	CgrRoute	*route;
	Bundle		newBundle;
	SdrObject	newBundleObj;

	/*	Determine whether or not the contact graph for the
	 *	terminus node identifies one or more routes over
	 *	which the bundle may be sent in order to get it
	 *	delivered to the terminus node.  If so, use the
	 *	Plan asserted for the entry node of the best route
	 *	or - for a critical bundle - the Plans asserted
	 *	for each neighboring node for which there is at
	 *	least one route to the terminus node.
	 *
	 *	Note that CGR can be used to compute a route to an
	 *	intermediate "station" node selected by another
	 *	routing mechanism (such as static routing or IRF),
	 *	not only to the bundle's final destination node.
	 *	In the simplest case, the bundle's destination is
	 *	the only "station" selected for the bundle.  To
	 *	avoid confusion, we here use the term "terminus"
	 *	to refer to the node to which a route is being
	 *	computed, regardless of whether that node is the
	 *	bundle's final destination or an intermediate
	 *	forwarding station.		 			*/

	CHKERR(bundle && bundleObj && terminusNode);
	if (ionvdb == NULL || cgrvdb == NULL)
	{
		putErrmsg("Can't get VDB for CGR.", NULL);
		return -1;
	}

	TRACE(CgrBuildRoutes, terminusNode->fqnn, bundle->payload.length,
			(unsigned int) atTime);

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
	if (!bundle->returnToSender)
	{
		/*	Must exclude sender of bundle from consideration
		 *	as a station on the route, to minimize routing
		 *	loops.  If returnToSender is 1 then we are
		 *	re-routing, possibly back through the sender,
		 *	because we have hit a dead end in routing and
		 *	must backtrack.					*/

		if (lyst_insert_last(excludedNodes, (void *)
			((uaddr) bundle->clDossier.senderFqnn)) == NULL)
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

	potential = cgr_identify_best_routes(terminusNode, bundle,
			excludedNodes, atTime, cgrSap(NULL), trace, bestRoutes);
       	if (potential < 0)
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
				bestRoutes, potential, preview);
	}

	/*	Non-critical bundle; send to the most preferred
	 *	neighbor.						*/

	elt = lyst_first(bestRoutes);
	if (elt)
	{
		route = (CgrRoute *) lyst_data_set(elt, NULL);
		TRACE(CgrUseRoute, route->toFqnn);
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
	|| bundle->id.source.ssp.ipn.fqnn
			== bundle->destination.ssp.ipn.fqnn)
	{
		return 0;	/*	Potential future fwd unneeded.	*/
	}

	if (potential == 0)
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

/*		Contingency functions for when CGR and IRF don't work.	*/

static int enqueueToNeighbor(Bundle *bundle, SdrObject bundleObj, uvast fqnn)
{
	Sdr		sdr = getIonsdr();
	char		nbrBuf[FQN_MAX_LENGTH];
	char		eid[MAX_EID_LEN + 1];
	VPlan		*vplan;
	PsmAddress	vplanElt;
	BpPlan		plan;

	putFqn(nbrBuf, fqnn);
	isprintf(eid, sizeof eid, "ipn:%s.0", nbrBuf);
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

static int	openCgr(void)
{
	CgrSAP	sap;

	sap = cgrSap(NULL);
	if (sap)	/*	CGR is already open.			*/
	{
		writeMemo("[i] CGR service access point is already open.");
		return 0;
	}

	if (cgr_start_SAP(getOwnFqnn(), ionReferenceTime(NULL), &sap) < 0)
	{
		putErrmsg("Failed starting CGR SAP", NULL);
		return -1;
	}

	oK(cgrSap(&sap));
	return 0;
}

static void	closeCgr(void)
{
	CgrSAP	noSap = NULL;

	cgr_stop_SAP(cgrSap(NULL));
	oK(cgrSap(&noSap));
}

static int enqueueBundle(Bundle *bundle, SdrObject bundleObj, CgrSAP sap)
{
	Sdr		sdr = getIonsdr();
	IonVdb		*ionvdb = getIonVdb();
	SdrObject	elt;
	char		eid[SDRSTRING_BUFSZ];
	MetaEid		metaEid;
	VScheme		*vscheme = NULL;
	PsmAddress	vschemeElt;
	uvast		fqnn;
	IonNode		*node;
	PsmAddress	nextNode;
	uint32_t	regionNbr;

	/* Parameter intentionally unused. */
	(void)sap;

#if CGR_DEBUG == 1
	CgrTrace	*trace = &(CgrTrace) { .fn = printCgrTraceLine };
#else
	CgrTrace	*trace = NULL;
#endif

	/*	Critical-bundle de-duplication (Layer 1): if this node has
	 *	already forwarded a copy of this critical bundle, drop the
	 *	duplicate here, before any routing/cloning/migration, so it
	 *	never coexists with the sibling copy in the forward/transmit
	 *	path.  cbdedup_seen() is a no-op for non-critical bundles.
	 *	There is no "duplicate" status-report reason code, so use the
	 *	generic "no additional information" reason (BP_REASON_NONE).	*/

	if (cbdedup_seen(bundle))
	{
		return bpAbandon(bundleObj, bundle, BP_REASON_NONE);
	}

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
		return bpAbandon(bundleObj, bundle, BP_REASON_EID_MALFORMED);
	}

	if (metaEid.nullEndpoint)
	{
		clearMetaEid(&metaEid);
		putErrmsg("Can't forward to null endpoint.", eid);
		return bpAbandon(bundleObj, bundle, BP_REASON_NO_ROUTE);
	}

	if (strcmp(vscheme->name, "ipn") != 0)
	{
		putErrmsg("Forwarding error; EID scheme is not 'ipn'.",
				vscheme->name);
		return -1;
	}

	fqnn = metaEid.elementNbr;
	clearMetaEid(&metaEid);

	/*	Apply routing override, if any.				*/

	if (applyRoutingOverride(bundle, bundleObj, fqnn) < 0)
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

	node = findNode(ionvdb, fqnn, &nextNode);
	if (node == NULL)
	{
		node = addNode(ionvdb, fqnn);
		if (node == NULL)
		{
			putErrmsg("Can't add node.", NULL);
			return -1;
		}
	}

	/*	If the terminus node resides in a region in which the
	 *	local node also resides, consult the contact plan (CGR)
	 *	to compute a route.  Inter-regional forwarding (routing
	 *	across region boundaries via passageways) is not
	 *	implemented, so a terminus in an unknown region simply
	 *	falls through to direct neighbor delivery below.	*/

	if (ionRegionOf(fqnn, 0, &regionNbr) >= 0)
	{
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
		/*	Enqueued.				*/

		return bpAccept(bundleObj, bundle);
	}

	/*	No luck using the contact graph or region tree to
	 *	compute a route to the destination node.  So see if
	 *	destination node is a neighbor (not identified in the
	 *	contact plan); if so, enqueue for direct transmission.	*/

	if (enqueueToNeighbor(bundle, bundleObj, fqnn) < 0)
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

	if (ipn_lookupExit(fqnn, eid) == 1)
	{
		/*	Found applicable exit; forward via the
		 *	indicated endpoint.				*/

		sdr_write(sdr, bundleObj, (char *) bundle, sizeof(Bundle));
		return forwardBundle(bundleObj, bundle, eid);
	}

	/*	No applicable exit.  If there's at least a route
	 *	that might work if some hypothetical contact should
	 *	materialize, we place the bundle in limbo and hope
	 *	for the best.
	 *
	 *	For custody bundles (Orange Book CT), always keep
	 *	in limbo - the source has custody responsibility
	 *	and must retain the bundle until custody is transferred.	*/

	/*	Check if bundle has CTEB (custody transfer).
	 *	Use findExtensionBlock which works for both locally
	 *	sourced and received bundles.				*/
	{
		SdrObject ctebElt = findExtensionBlock(bundle,
				CBR_BLOCK_TYPE_CTEB, 0);
		int	hasCustody = (ctebElt != 0);

		if (cgr_prospect(fqnn, bundle->expirationTime) > 0 || hasCustody)
		{
			if (hasCustody)
			{
#ifdef DEBUG_CUSTODY_SRC
				writeMemo("[DEBUG-CUSTODY-SRC] ipnfw: custody bundle going to limbo");
#endif
			}

			if (enqueueToLimbo(bundle, bundleObj) < 0)
			{
				putErrmsg("Can't put bundle in limbo.", NULL);
				return -1;
			}
		}
	}

	if (bundle->planXmitElt)
	{
		/*	Bundle was enqueued to limbo.			*/

		return bpAccept(bundleObj, bundle);
	}

#ifdef DEBUG_CUSTODY_SRC
	writeMemo("[DEBUG-CUSTODY-SRC] ipnfw: abandoning bundle (no route, no custody)");
#endif
	return bpAbandon(bundleObj, bundle, BP_REASON_NO_ROUTE);
}

#if defined (ION_LWT)
int	ipnfw(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
#else
int	main(void)
{
#endif
	int		running = 1;
	Sdr		sdr;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	Scheme		scheme;
	SdrObject	elt;
	SdrObject	bundleAddr;
	Bundle		bundle;
	SdrObject	ovrdAddr;
	IpnOverride	ovrd;
	struct timeval	xnStart;
	struct timeval	xnEnd;

	if (bpAttach() < 0)
	{
		putErrmsg("ipnfw can't attach to BP.", NULL);
		return 1;
	}

	if (atexit(ipnfwReportExit) != 0)
	{
		writeMemo("[?] ipnfw couldn't register exit reporter.");
	}

	if (ipnInit() < 0)
	{
		putErrmsg("ipnfw can't load routing database.", NULL);
		return 1;
	}

	cgr_start();
	if (cbdedup_init() < 0)
	{
		putErrmsg("ipnfw can't init critical-bundle dedup table.",
				NULL);
		return 1;
	}

	findScheme("ipn", &vscheme, &vschemeElt);
	if (vschemeElt == 0)
	{
		putErrmsg("'ipn' scheme is unknown.", NULL);
		return 1;
	}

	sdr = getIonsdr();
	CHKZERO(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &scheme, sdr_list_data(sdr,
			vscheme->schemeElt), sizeof(Scheme));
	sdr_exit_xn(sdr);
	oK(_ipnfwSemaphore(&vscheme->semaphore));
	isignal(SIGTERM, shutDown);
	if (openCgr() < 0)
	{
		putErrmsg("Can't open CGR service access point.", NULL);
		return -1;
	}

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

		bundleAddr = (SdrObject) sdr_list_data(sdr, elt);
		sdr_stage(sdr, (char *) &bundle, bundleAddr, sizeof(Bundle));

		/*	Breadcrumb (#1047): record the bundle now being
		 *	forwarded and when its transaction began, so an
		 *	unexpected exit or an abnormally long lock hold can
		 *	be attributed to a specific bundle/operation.	*/

		ipnfwCurrentDest = bundle.destination.ssp.ipn.fqnn;
		ipnfwCurrentSize = (unsigned int) bundle.payload.length;
		getCurrentTime(&xnStart);

		/*	Note any applicable overrides for routing
		 *	and/or class of service.			*/

		bundle.priority = bundle.classOfService;
		bundle.ordinal = bundle.ancillaryData.ordinal;
		bundle.qosFlags = bundle.ancillaryData.flags;
		if (ipn_lookupOvrd(bundle.ancillaryData.dataLabel,
				bundle.destination.ssp.ipn.fqnn,
				bundle.id.source.ssp.ipn.fqnn, &ovrdAddr))
		{
			sdr_read(sdr, (char *) &ovrd, ovrdAddr,
					sizeof(IpnOverride));
			if (ovrd.priority != (unsigned char) -1)
			{
				/*	Override requested CoS.		*/

				bundle.priority = ovrd.priority;
				bundle.ordinal = ovrd.ordinal;
				bundle.qosFlags = ovrd.qosFlags;
			}

			if (ovrd.neighborFqnn)
			{
				bundle.ovrdNeighbor = ovrd.neighborFqnn;
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
		if (enqueueBundle(&bundle, bundleAddr, cgrSap(NULL)) < 0)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("Can't enqueue bundle.", NULL);
			continue;
		}

		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Can't enqueue bundle.", NULL);
			running = 0;	/*	Terminate loop.		*/
		}

		/*	Breadcrumb (#1047): flag any transaction that held
		 *	the SDR lock abnormally long (the incident showed a
		 *	179 ms hold just before the silent death).		*/

		getCurrentTime(&xnEnd);
		{
			long	heldUsec;

			heldUsec = ((long) (xnEnd.tv_sec - xnStart.tv_sec))
					* 1000000L
					+ (xnEnd.tv_usec - xnStart.tv_usec);
			if (heldUsec >= IPNFW_SLOW_HOLD_USEC)
			{
				char	hbuf[160];

				isprintf(hbuf, sizeof hbuf, "[?] ipnfw slow \
forwarding hold: dest node " UVAST_FIELDSPEC ", payload %u bytes, held SDR \
lock %ld us.", ipnfwCurrentDest, ipnfwCurrentSize, heldUsec);
				writeMemo(hbuf);
			}
		}

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();
	}

	closeCgr();
	cbdedup_shutdown();
	writeErrmsgMemos();
	ipnfwShutdownClean = 1;	/*	Normal exit; suppress exit report.	*/
	writeMemo("[i] ipnfw forwarder has ended.");
	ionDetach();
	return 0;
}
