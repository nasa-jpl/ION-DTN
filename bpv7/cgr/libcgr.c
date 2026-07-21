/*
	libcgr.c:	functions implementing Contact Graph Routing.

	Author: Scott Burleigh, JPL

	Adaptation to use Dijkstra's Algorithm developed by John
	Segui, 2011.

	Adaptation for Earliest Transmission Opportunity developed
	by N. Bezirgiannidis and V. Tsaoussidis, Democritus University
	of Thrace, 2014.

	Adaptation for Overbooking management developed by C. Caini,
	D. Padalino, and M. Ruggieri, University of Bologna, 2014.

	Copyright (c) 2008, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
#include "cgr.h"

#if UNIBO_CGR
// Added by L. Persampieri
#include "../cgr/Unibo-CGR/ion_bpv7/interface/interface_cgr_ion.h"
#endif

#define CGRVDB_NAME	"cgrvdb"

typedef struct
{
	/*	Working values, reset for each Dijkstra run.		*/

	IonCXref	*predecessor;	/*	On path to destination.	*/
	time_t		arrivalTime;	/*	As from time(2).	*/
	unsigned int	hopCount;
	int		visited;	/*	Boolean.		*/
	int		suppressed;	/*	Boolean.		*/
} CgrContactNote;	/*	IonCXref routingObject is one of these.	*/

/*		Functions for managing the CGR database.		*/

/*	#1054: delete every element of 'routes' whose content is the
 *	address of the route being removed.  Used to unlink a route from
 *	a route list authoritatively, by content, rather than trusting a
 *	cached element handle.						*/

static void	purgeRouteReferences(PsmPartition ionwm, PsmAddress routes,
			PsmAddress routeAddr)
{
	PsmAddress	elt;
	PsmAddress	nextElt;

	if (routes == 0)
	{
		return;
	}

	for (elt = sm_list_first(ionwm, routes); elt; elt = nextElt)
	{
		nextElt = sm_list_next(ionwm, elt);
		if (sm_list_data(ionwm, elt) == routeAddr)
		{
			sm_list_delete(ionwm, elt, NULL, NULL);
		}
	}
}

/*	Return the first element of a route list that cites routeAddr, or 0 if
 *	none.  Used to detect a route already present in a list so it is not
 *	cited twice.							*/

static PsmAddress	firstRouteListElt(PsmPartition ionwm, PsmAddress routes,
				PsmAddress routeAddr)
{
	PsmAddress	elt;

	if (routes == 0)
	{
		return 0;
	}

	for (elt = sm_list_first(ionwm, routes); elt;
			elt = sm_list_next(ionwm, elt))
	{
		if (sm_list_data(ionwm, elt) == routeAddr)
		{
			return elt;
		}
	}

	return 0;
}

/*	#1048 follow-up: validate a handle taken from a route's hops,
 *	contact-citation, or rootOfSpur cross-references before walking the
 *	SmList it points at.  A freed/recycled block keeps a valid-looking
 *	handle but garbage contents -- and a wild offset makes sm_list_list /
 *	sm_list_next SEGV outright.  Confirm the address round-trips through
 *	the partition; callers pair this with sm_list_header_sane() when the
 *	address is supposed to be an SmList header.  Defined above
 *	severSpurReferences and removeRoute (both compiled unconditionally,
 *	above the #if !UNIBO_CGR guard helpers) so both can use it.	*/

static int	cgrHandleSane(PsmPartition ionwm, PsmAddress addr)
{
	void	*ptr;

	if (addr == 0)
	{
		return 0;
	}

	ptr = psp(ionwm, addr);
	return (ptr != NULL && psa(ionwm, ptr) == addr);
}

/*	#1133: a CgrRoute's rootOfSpur is an SmListElt that lives in some
 *	OTHER route's hops list (the route this one was spurred from).
 *	When that other route is removed its hops list is destroyed, which
 *	frees the SmListElt -- but nothing nulls the rootOfSpur fields that
 *	still point into it.  A later computeAnotherRoute then feeds the
 *	dangling waypoint to computeRoute, which dereferences a recycled
 *	SmListElt and hands computeDistanceToTerminus a bogus rootContact
 *	(toFqnn == 0), aborting in ionRegionOf.  Before a route's hops are
 *	destroyed, null every rootOfSpur in the route lists that belongs to
 *	that hops list, so no spur waypoint can outlive the hops it points
 *	into.  Resetting rootOfSpur to 0 just makes the affected route be
 *	treated as branching from the root of the contact graph.	*/

static void	severSpurReferences(PsmPartition ionwm, PsmAddress routes,
			PsmAddress hopsAddr)
{
	PsmAddress	elt;
	PsmAddress	routeAddr;
	CgrRoute	*route;

	if (routes == 0 || hopsAddr == 0)
	{
		return;
	}

	for (elt = sm_list_first(ionwm, routes); elt;
			elt = sm_list_next(ionwm, elt))
	{
		routeAddr = sm_list_data(ionwm, elt);
		route = (CgrRoute *) psp(ionwm, routeAddr);
		if (route == NULL)
		{
			continue;
		}

		/*	#1133 follow-up: rootOfSpur points into ANOTHER route's
		 *	hops list and can be left dangling -- or wild -- when that
		 *	list is freed and recycled.  Passing a wild handle to
		 *	sm_list_list SEGVs ipnfw mid-SDR-transaction, orphaning the
		 *	lock and taking the node's whole ION instance down (exactly
		 *	the crash this function exists to prevent, one level up).
		 *	Validate the handle first; if it is unsound the spur is
		 *	already broken, so just null it.			*/

		if (route->rootOfSpur != 0
		&& (!cgrHandleSane(ionwm, route->rootOfSpur)
			|| sm_list_list(ionwm, route->rootOfSpur) == hopsAddr))
		{
			route->rootOfSpur = 0;
		}
	}
}

/*	Destroy a route's "hops" list, first removing from each cited
 *	contact's "citations" list the back-reference to the hop element
 *	being freed.  EVERY path that disposes of a route's hops must funnel
 *	through here: freeing a hop element while a contact->citations entry
 *	still points at it leaves a wild citation that later aborts a
 *	removeRoute citations walk (SIGSEGV in Sm_list_delete -> orphaned SDR
 *	-> node-wide daemon cascade).  Historically only removeRoute upheld
 *	this invariant; computeRoute's Dijkstra-failure and no-route cleanups
 *	and computeSpurRoute's error returns destroyed/abandoned hops without
 *	it, leaving dangling contact citations.  [#1048 contact<->route
 *	cross-reference]						*/

static void	destroyRouteHops(PsmPartition ionwm, CgrRoute *route)
{
	PsmAddress	citation;
	PsmAddress	nextCitation;
	PsmAddress	contactAddr;
	IonCXref	*contact;
	PsmAddress	citationElt;
	int		hopsCorrupt = 0;

	if (route->hops == 0)
	{
		return;
	}

	/*	Each member of the "hops" list of this route is one among
	 *	possibly many citations of some specific contact, i.e., its
	 *	content is the address of that contact.  For each hop, go
	 *	through the cited contact's citations list and remove the
	 *	member whose content is the address of this hop element.	*/

	for (citation = sm_list_first(ionwm, route->hops); citation;
			citation = nextCitation)
	{
		/*	#1048: route->hops can be a stale/recycled list whose
		 *	elements are wild offsets; validate each before use.	*/

		if (!cgrHandleSane(ionwm, citation))
		{
			writeMemo("[?] CGR: abandoning corrupt route hops "
					"list (#1048)");
			hopsCorrupt = 1;
			break;
		}

		nextCitation = sm_list_next(ionwm, citation);
		contactAddr = sm_list_data(ionwm, citation);
		if (contactAddr == 0)
		{
			/*	Contact deleted; reference already zeroed.	*/

			sm_list_delete(ionwm, citation, NULL, NULL);
			continue;
		}

		if (!cgrHandleSane(ionwm, contactAddr))
		{
			char	dbg[128];

			isprintf(dbg, sizeof dbg, "[?] CGR: dropping stale \
contact citation (#1048): contactAddr=" UVAST_FIELDSPEC ".",
					(uvast) contactAddr);
			writeMemo(dbg);
			sm_list_delete(ionwm, citation, NULL, NULL);
			continue;
		}

		contact = (IonCXref *) psp(ionwm, contactAddr);
		if (contact->citations
		&& cgrHandleSane(ionwm, contact->citations)
		&& sm_list_header_sane(ionwm, contact->citations))
		{
			for (citationElt = sm_list_first(ionwm,
				contact->citations); citationElt;
				citationElt = sm_list_next(ionwm,
				citationElt))
			{
				if (sm_list_data(ionwm, citationElt)
						== citation)
				{
					sm_list_delete(ionwm,
						citationElt, NULL,
						NULL);
					break;
				}
			}
		}

		sm_list_delete(ionwm, citation, NULL, NULL);
	}

	if (hopsCorrupt)
	{
		route->hops = 0;	/*	leak the corrupt list; it
					 *	cannot be safely destroyed	*/
	}
	else
	{
		sm_list_destroy(ionwm, route->hops, NULL, NULL);
		route->hops = 0;
	}
}

static void	removeRoute(PsmPartition ionwm, PsmAddress routeElt)
{
	PsmAddress	routeAddr;
	CgrRoute	*route;
	PsmAddress	listAddr;
	PsmAddress	nodeAddr = 0;
	IonNode		*node;
	CgrRtgObject	*routingObj = NULL;

	routeAddr = sm_list_data(ionwm, routeElt);
	route = (CgrRoute *) psp(ionwm, routeAddr);

	/*	#1054: authoritatively unlink this route from BOTH the
	 *	selectedRoutes and knownRoutes lists, by content, rather
	 *	than trusting that referenceElt + routeElt enumerate every
	 *	element that cites it.  A bookkeeping slip (e.g. a
	 *	referenceElt rewrite in the Yen migration that orphaned the
	 *	prior element) can leave a third element pointing at this
	 *	route; freeing the route while that element survives lets a
	 *	later list walk dereference freed memory and fault in
	 *	lockSmlist (the disabledRoute use-after-free).  Recover the
	 *	routing object from the list's user data (the destination
	 *	IonNode address) and delete every element whose content is
	 *	this route's address.  If the routing object can't be
	 *	resolved -- e.g. mid-teardown, when detachRoutingObject has
	 *	already zeroed node->routingObject -- fall back to the legacy
	 *	referenceElt + routeElt deletion.				*/

	listAddr = sm_list_list(ionwm, routeElt);
	if (listAddr)
	{
		nodeAddr = sm_list_user_data(ionwm, listAddr);
	}

	if (nodeAddr)
	{
		node = (IonNode *) psp(ionwm, nodeAddr);
		if (node->routingObject)
		{
			routingObj = (CgrRtgObject *)
					psp(ionwm, node->routingObject);
		}
	}

	if (routingObj)
	{
		purgeRouteReferences(ionwm, routingObj->selectedRoutes,
				routeAddr);
		purgeRouteReferences(ionwm, routingObj->knownRoutes, routeAddr);

		/*	#1133: null any spur waypoints that point into this
		 *	route's hops list before it is destroyed below.	*/

		severSpurReferences(ionwm, routingObj->selectedRoutes,
				route->hops);
		severSpurReferences(ionwm, routingObj->knownRoutes,
				route->hops);
	}
	else
	{
		if (route->referenceElt)
		{
			sm_list_delete(ionwm, route->referenceElt, NULL, NULL);
		}

		if (routeElt != route->referenceElt)
		{
			sm_list_delete(ionwm, routeElt, NULL, NULL);
		}
	}

	/*	Detach this route from every contact cited in its hops and
	 *	destroy the hops list (centralized so every disposal path
	 *	maintains the contact<->route cross-reference).			*/

	destroyRouteHops(ionwm, route);

	psm_free(ionwm, routeAddr);
}

static void	discardRouteList(PsmPartition ionwm, PsmAddress routes)
{
	PsmAddress	elt;
	PsmAddress	nextElt;

	/*	Destroy all routes in list.				*/

	if (routes == 0)
	{
		return;
	}

	for (elt = sm_list_first(ionwm, routes); elt; elt = nextElt)
	{
		nextElt = sm_list_next(ionwm, elt);
		removeRoute(ionwm, elt);
	}

	/*	Destroy the list itself.				*/

	sm_list_destroy(ionwm, routes, NULL, NULL);
}

static void	detachRoutingObject(PsmPartition ionwm,
			CgrRtgObject *routingObject)
{
	IonNode		*node;

	/*	Detach routing object from remote node.			*/

	node = (IonNode *) psp(ionwm, routingObject->nodeAddr);
	node->routingObject = 0;

	/*	Discard the lists of routes to the remote node.		*/

	if (routingObject->selectedRoutes)
	{
		discardRouteList(ionwm, routingObject->selectedRoutes);
	}

	if (routingObject->knownRoutes)
	{
		discardRouteList(ionwm, routingObject->knownRoutes);
	}

	if (routingObject->proximateNodes)
	{
		sm_list_destroy(ionwm, routingObject->proximateNodes, NULL,
				NULL);
	}
}

void	cgr_clear_vdb(CgrVdb *vdb)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();
	PsmAddress	elt;
	PsmAddress	nextElt;
	PsmAddress	addr;
	CgrRtgObject	*routingObject;

	/*	Destroy all routing objects in the CGR vdb.		*/

	CHKVOID(sdr_begin_xn(sdr)); /* To lock memory. */
	for (elt = sm_list_first(ionwm, vdb->routingObjects); elt;
			elt = nextElt)
	{
		nextElt = sm_list_next(ionwm, elt);
		addr = sm_list_data(ionwm, elt);
		routingObject = (CgrRtgObject *) psp(ionwm, addr);
		detachRoutingObject(ionwm, routingObject);

		/*	Destroy the routing object itself.		*/

		psm_free(ionwm, addr);

		/*	And delete the reference to the routing object.	*/

		sm_list_delete(ionwm, elt, NULL, NULL);
	}

	oK(sdr_exit_xn(sdr));
}

#if !UNIBO_CGR

/*	Validate that a PsmAddress lies within the working-memory
 *	partition (mirrors psa()'s bounds check) without dereferencing
 *	it.  CGR uses this to detect a stale/recycled route or hops
 *	handle (#1048) before passing it to sm_list_first(), which would
 *	otherwise fault in lockSmlist() reading a wild list pointer.	*/

static int	cgrHandleValid(PsmPartition ionwm, PsmAddress addr)
{
	void	*ptr;

	if (addr == 0)
	{
		return 0;
	}

	ptr = psp(ionwm, addr);		/*	NULL if below the heap.	*/
	if (ptr == NULL)
	{
		return 0;
	}

	/*	psa() returns 0 if ptr is outside the partition's upper
	 *	bound; a valid handle round-trips back to itself.	*/

	return (psa(ionwm, ptr) == addr);
}

/*	Stronger validator for a PsmAddress that should refer to an
 *	SmList.  cgrHandleValid only checks address bounds + round-trip
 *	but doesn't look at the bytes -- a Psm_freed SmList block that's
 *	been recycled keeps a valid-looking address while its header
 *	contents are garbage.  cgrSmListValid additionally checks that
 *	the SmList header still looks like one (lock field is either the
 *	unlocked sentinel or a valid sem index).  Without this, a stale
 *	hops handle slips past the disabledRoute guard and trips an
 *	assertion in sm_SemTake -> aborts ipnfw -> drops the SDR
 *	transaction and degrades the node (#1048).			*/

static int	cgrSmListValid(PsmPartition ionwm, PsmAddress listAddr)
{
	if (!cgrHandleValid(ionwm, listAddr))
	{
		return 0;
	}

	return sm_list_header_sane(ionwm, listAddr);
}

static int	disabledRoute(PsmPartition ionwm, PsmAddress routeElt,
			PsmAddress *routeAddr, CgrRoute **route)
{
	PsmAddress	elt;

	*routeAddr = sm_list_data(ionwm, routeElt);
	*route = (CgrRoute *) psp(ionwm, *routeAddr);

	/*	Defensive + self-diagnosing (#1048).  A use-after-free in
	 *	the route-list bookkeeping can leave an element pointing at
	 *	a freed/recycled CgrRoute, so *routeAddr or (*route)->hops
	 *	can be a stale handle.  If we passed such a handle to
	 *	sm_list_first() below it would assert on a NULL list or --
	 *	worse -- SIGSEGV in lockSmlist() on a wild list pointer,
	 *	orphaning the SDR lock and cascading the node.  Validate the
	 *	handles first; if bad, log enough to pin the missed unlink
	 *	(the core/ASan evidence we may not get again), drop just
	 *	this list element (not removeRoute -- the route memory is
	 *	suspect and may be shared/already-freed), and treat the
	 *	route as disabled.  The short-circuit order guarantees we
	 *	never dereference *route until *routeAddr is known valid.	*/

	if (!cgrHandleValid(ionwm, *routeAddr)
	|| (*route)->hops == 0
	|| !cgrSmListValid(ionwm, (*route)->hops))
	{
		char		dbg[256];
		PsmAddress	refElt = 0;
		PsmAddress	hops = 0;
		PsmAddress	listAddr = sm_list_list(ionwm, routeElt);

		if (cgrHandleValid(ionwm, *routeAddr))
		{
			/*	Route struct is readable; capture its
			 *	(possibly garbage) cross-references.	*/

			refElt = (*route)->referenceElt;
			hops = (*route)->hops;
		}

		isprintf(dbg, sizeof dbg, "[?] CGR: dropping malformed route \
(#1048): routeElt=" UVAST_FIELDSPEC " routeAddr=" UVAST_FIELDSPEC " hops=" \
UVAST_FIELDSPEC " referenceElt=" UVAST_FIELDSPEC " list=" UVAST_FIELDSPEC ".",
				(uvast) routeElt, (uvast) *routeAddr,
				(uvast) hops, (uvast) refElt, (uvast) listAddr);
		writeMemo(dbg);

		sm_list_delete(ionwm, routeElt, NULL, NULL);
		*route = NULL;
		return 1;
	}

	for (elt = sm_list_first(ionwm, (*route)->hops); elt;
				elt = sm_list_next(ionwm, elt))
	{
		if (sm_list_data(ionwm, elt) == 0)
		{
			/*	At least one of the contacts in this
			 *	route has been deleted, so the route
			 *	is no longer usable.			*/

			removeRoute(ionwm, routeElt);
			return 1;
		}
	}

	return 0;
}

#endif

CgrVdb	*cgr_get_vdb(void)
{
	static char	*name = CGRVDB_NAME;
	PsmPartition	ionwm = getIonwm();
	PsmAddress	vdbAddress;
	PsmAddress	elt;
	CgrVdb		*vdb;
	Sdr		sdr;

	/*	Attaching to volatile database.				*/

	if (psm_locate(ionwm, name, &vdbAddress, &elt) < 0)
	{
		putErrmsg("Failed searching for vdb.", name);
		return NULL;
	}

	if (elt)
	{
		vdb = (CgrVdb *) psp(ionwm, vdbAddress);
		return vdb;
	}

	/*	CGR volatile database doesn't exist yet.		*/

	sdr = getIonsdr();
	CHKNULL(sdr_begin_xn(sdr));	/*	To lock memory.		*/
	vdbAddress = psm_zalloc(ionwm, sizeof(CgrVdb));
	if (vdbAddress == 0)
	{
		sdr_exit_xn(sdr);
		putErrmsg("No space for CGR volatile database.", name);
		return NULL;
	}

	vdb = (CgrVdb *) psp(ionwm, vdbAddress);
	memset((char *) vdb, 0, sizeof(CgrVdb));
	if ((vdb->routingObjects = sm_list_create_unlocked(ionwm)) == 0
	|| psm_catlg(ionwm, name, vdbAddress) < 0)
	{
		sdr_exit_xn(sdr);
		putErrmsg("Can't initialize CGR volatile database.", name);
		return NULL;
	}

	sdr_exit_xn(sdr);
	return vdb;
}

/*	Functions for populating the route lists.			*/

#if !UNIBO_CGR

static int	getApplicableRange(IonCXref *contact, unsigned int *owlt)
{
	PsmPartition	ionwm = getIonwm();
	IonVdb		*ionvdb = getIonVdb();
	IonRXref	arg;
	PsmAddress	elt;
	IonRXref	*range;

	*owlt = 0;		/*	Default.			*/
	if (contact->type == CtHypothetical || contact->type == CtDiscovered)
	{
		return 0;	/*	Physically adjacent nodes.	*/
	}

	/*	This is a scheduled contact; need to know the OWLT.	*/

	memset((char *) &arg, 0, sizeof(IonRXref));
	arg.fromFqnn = contact->fromFqnn;
	arg.toFqnn = contact->toFqnn;
	for (oK(sm_rbt_search(ionwm, ionvdb->rangeIndex, rfx_order_ranges,
			&arg, &elt)); elt; elt = sm_rbt_next(ionwm, elt))
	{
		range = (IonRXref *) psp(ionwm, sm_rbt_data(ionwm, elt));
		CHKERR(range);
		if (range->fromFqnn > arg.fromFqnn
		|| range->toFqnn > arg.toFqnn)
		{
			break;
		}

		if (range->toTime < contact->fromTime)
		{
			continue;	/*	Range is in the past.	*/
		}

		if (range->fromTime > contact->fromTime)
		{
			/*	Range unknown at contact start time.	*/
			break;
		}

		/*	Found applicable range.				*/

		*owlt = range->owlt;
		return 0;
	}

	/*	No applicable range.					*/

	return -1;
}

static CgrContactNote	*getWorkArea(PsmPartition ionwm, IonCXref *contact)
{
	CgrContactNote	*work;

	if (contact->routingObject == 0)
	{
		contact->routingObject = psm_zalloc(ionwm,
				sizeof(CgrContactNote));
		if (contact->routingObject == 0)
		{
			return NULL;
		}

		work = (CgrContactNote *) psp(ionwm, contact->routingObject);
		memset((char *) work, 0, sizeof(CgrContactNote));
	}
	else
	{
		work = (CgrContactNote *) psp(ionwm, contact->routingObject);
	}

	return work;
}

static int	clearWorkAreas(IonCXref *rootContact)
{
	PsmPartition	ionwm = getIonwm();
	IonVdb		*ionvdb = getIonVdb();
	PsmAddress	elt;
	IonCXref	*contact;
	CgrContactNote	*work;

	for (elt = sm_rbt_first(ionwm, ionvdb->contactIndex); elt;
			elt = sm_rbt_next(ionwm, elt))
	{
		contact = (IonCXref *) psp(ionwm, sm_rbt_data(ionwm, elt));
		CHKERR(work = getWorkArea(ionwm, contact));
		work->predecessor = NULL;
		work->visited = 0;
		work->suppressed = 0;

		/*	Reset arrival time, except preserve arrival
		 *	time of root contact.				*/

		if (contact != rootContact)
		{
			work->arrivalTime = MAX_TIME;
			work->hopCount = 0;
		}
	}

	return 0;
}

static int	edgeIsExcluded(PsmPartition ionwm, PsmAddress excludedEdges,
			PsmAddress contactAddr)
{
	PsmAddress	elt;

	if (excludedEdges)
	{
		for (elt = sm_list_first(ionwm, excludedEdges); elt;
				elt = sm_list_next(ionwm, elt))
		{
			if (sm_list_data(ionwm, elt) == contactAddr)
			{
				return 1;
			}
		}
	}

	return 0;
}

static int	computeDistanceToTerminus(IonCXref *rootContact,
			CgrContactNote *rootWork, IonNode *terminusNode,
			time_t currentTime, PsmAddress excludedEdges,
			CgrRoute *route, CgrTrace *trace)
{
	PsmPartition	ionwm = getIonwm();
	IonVdb		*ionvdb = getIonVdb();
	IonCXref	*current;
	CgrContactNote	*currentWork;
	IonCXref	arg;
	uint32_t	regionNbr;
	PsmAddress	elt;
	PsmAddress	contactAddr;
	IonCXref	*contact;
	CgrContactNote	*work;
	unsigned int	owlt;
	unsigned int	owltMargin;
	time_t		transmitTime;
	time_t		arrivalTime;
	IonCXref	*finalContact = NULL;
	time_t		earliestFinalArrivalTime = MAX_TIME;
	IonCXref	*nextCurrentContact;
	time_t		earliestArrivalTime;
	unsigned int	fewestHops;
	time_t		earliestEndTime;
	PsmAddress	addr;
	PsmAddress	citation;
	IonCXref	*firstContact;

	/*	This is an implementation of Dijkstra's Algorithm.	*/

	TRACE(CgrBeginRoute);
	current = rootContact;
	currentWork = rootWork;
	oK(ionRegionOf(current->toFqnn, terminusNode->fqnn, &regionNbr));

	/*	Perform this outer loop until either the best
	 *	route to the end vertex has been identified or else
	 *	it is known that there is no such route.		*/

	while (1)
	{
		/*	On each cycle through the outer loop, the
		 *	"current" contact is the contact that has been
		 *	most recently identified as one of the contacts
		 *	on the best path from the local node to the
		 *	destination node, given all of the exclusions
		 *	(suppression, prior visitation) that apply to
		 *	this invocation of Dijkstra's Algorithm.
		 *
		 *	Each time we cycle through the outer loop, we
		 *	perform two inner loops.
		 *
		 *	In the first innner loop, we consider all
		 *	unvisited successors (i.e., topologically
		 *	adjacent "next-hop" contacts; the "frontier")
		 *	of the current contact, in each case computing
		 *	best-case arrival time for a bundle that might
		 *	be transmitted during that contact.		*/

		TRACE(CgrConsiderRoot, current->fromFqnn, current->toFqnn);
		memset((char *) &arg, 0, sizeof(IonCXref));
		arg.regionNbr = regionNbr;
		arg.fromFqnn = current->toFqnn;
		for (oK(sm_rbt_search(ionwm, ionvdb->contactIndex,
				rfx_order_contacts, &arg, &elt));
				elt; elt = sm_rbt_next(ionwm, elt))
		{
			contactAddr = sm_rbt_data(ionwm, elt);
			contact = (IonCXref *) psp(ionwm, contactAddr);
			if (contact->type == CtRegistration)
			{
				continue;	/*	No bearing.	*/
			}

			if (contact->toTime <= currentTime)
			{
				/*	Contact is ended, is about to
				 *	be purged.			*/

				continue;
			}

			/*	Note: contact->fromFqnn can't be less
			 *	than current->toFqnn: we started at
			 *	that node with sm_rbt_search.		*/

			if (contact->fromFqnn > current->toFqnn)
			{
				/*	No more relevant contacts.	*/

				break;
			}

			TRACE(CgrConsiderContact, contact->fromFqnn,
					contact->toFqnn);
			CHKERR(work = getWorkArea(ionwm, contact));
			if (work->suppressed)
			{
				TRACE(CgrIgnoreContact, CgrSuppressed);
				continue;
			}

			if (work->visited)
			{
				TRACE(CgrIgnoreContact, CgrVisited);
				continue;
			}

			if (current == rootContact)
			{
				if (edgeIsExcluded(ionwm, excludedEdges,
						contactAddr))
				{
					work->suppressed = 1;
					TRACE(CgrIgnoreContact, CgrSuppressed);
					continue;
				}
			}

			if (contact->toTime <= currentWork->arrivalTime)
			{
				TRACE(CgrIgnoreContact, CgrContactEndsEarly);

				/*	Can't be a next-hop contact:
				 *	transmission has stopped by
				 *	the time of arrival of data
				 *	during the current contact.	*/

				continue;
			}

			/*	Get OWLT between the nodes in contact,
			 *	from applicable range in range index.	*/

			if (getApplicableRange(contact, &owlt) < 0)
			{
				TRACE(CgrIgnoreContact, CgrNoRange);

				/*	Don't know the OWLT between
				 *	these BP nodes at this time,
				 *	so can't consider in CGR.	*/

				work->suppressed = 1;
				continue;
			}

			/*	Allow for possible additional latency
			 *	due to the movement of the receiving
			 *	node during the propagation of signal
			 *	from the sending node.			*/

			owltMargin = ((MAX_SPEED_MPH / 3600) * owlt) / 186282;
			owlt += owltMargin;

			/*	Compute cost of choosing this edge:
			 *	earliest bundle arrival time, given
			 *	that the bundle arrives at the sending
			 *	node in the course of the current
			 *	contact.				*/

			if (contact->fromTime < currentWork->arrivalTime)
			{
				transmitTime = currentWork->arrivalTime;
			}
			else
			{
				transmitTime = contact->fromTime;
			}

			arrivalTime = transmitTime + owlt;

			/*	Note that this arrival time is best
			 *	case.  It is based on the earliest
			 *	possible transmit time, which would
			 *	be applicable to a bundle transmitted
			 *	on this route immediately; any delay
			 *	in transmission due to queueing behind
			 *	other bundles would result in a later
			 *	transmit time and therefore a later
			 *	arrival time.				*/

			TRACE(CgrCost, (unsigned int)(transmitTime), owlt,
					(unsigned int)(arrivalTime));

			if (arrivalTime < work->arrivalTime)
			{
				/*	Bundle arrival time will
				 *	vary with bundle transmit
				 *	time, which may be delayed
				 *	awaiting bundle arrival via
				 *	the current contact.
				 *
				 *	The new improved arrival time
				 *	for this contact is obtained
				 *	only if the preceding contact
				 *	is the "current" contact.	*/

				work->arrivalTime = arrivalTime;
				work->predecessor = current;
				work->hopCount = currentWork->hopCount;
				work->hopCount++;
			}
		}

		/*	We have at this point computed the best-case
		 *	bundle arrival times for all edges of the
		 *	graph that originate at the current contact.	*/

		currentWork->visited = 1;

		/*	Now the second inner loop: among ALL non-
		 *	suppressed contacts in the graph (not just the
		 *	successors to the current contact), select the
		 *	one with the earliest arrival time (the least
		 *	distance from the root vertex) -- and, in the
		 *	event of a tie, the one comprising the smallest
		 *	number of successive contacts ("hops") -- to be
		 *	the new "current" vertex to analyze.		*/

		nextCurrentContact = NULL;
		earliestArrivalTime = MAX_TIME;
		fewestHops = (unsigned int) -1;
		for (elt = sm_rbt_first(ionwm, ionvdb->contactIndex); elt;
				elt = sm_rbt_next(ionwm, elt))
		{
			contact = (IonCXref *) psp(ionwm, sm_rbt_data(ionwm,
					elt));
			CHKERR(contact);
			if (contact->toTime <= currentTime)
			{
				/*	Contact is ended, is about to
				 *	be purged.			*/

				continue;
			}

			CHKERR(work = getWorkArea(ionwm, contact));
			if (work->suppressed || work->visited)
			{
				continue;	/*	Ineligible.	*/
			}

			if (work->arrivalTime == MAX_TIME)
			{
				/*	Not reachable from root.	*/

				continue;
			}

			/*	Dijkstra search edge cost function.	*/

			if (work->arrivalTime > earliestArrivalTime)
			{
				/*	Not the lowest-cost edge.	*/

				continue;
			}

			if (work->arrivalTime == earliestArrivalTime
					&& work->hopCount >= fewestHops)
			{
				/*	Not the lowest-cost edge.	*/

				continue;
			}

			/*	Best candidate "next current" contact
			 *	seen so far.				*/

			nextCurrentContact = contact;
			earliestArrivalTime = work->arrivalTime;
			fewestHops = work->hopCount;
		}

		/*	If search is complete, stop.  Else repeat,
		 *	with new value of "current".			*/

		if (nextCurrentContact == NULL)
		{
			/*	End of search; can't proceed any
			 *	further toward the terminal contact.	*/

			break;	/*	Out of outer loop; no path.	*/
		}

		current = nextCurrentContact;
		currentWork = (CgrContactNote *)
				psp(ionwm, current->routingObject);
		if (current->toFqnn == terminusNode->fqnn)
		{
			earliestFinalArrivalTime = currentWork->arrivalTime;
			finalContact = current;
			break;	/*	Out of outer loop; found path.	*/
		}
	}

	/*	Have finished a single Dijkstra search of the contact
	 *	graph, excluding those contacts that were suppressed.	*/

	if (finalContact)	/*	Got route to terminal contact.	*/
	{
		/*	We have found the best path from the local node
		 *	to the destination node, given the applicable
		 *	exclusions.					*/

		route->arrivalTime = earliestFinalArrivalTime;
		route->arrivalConfidence = 1.0;

		/*	Load the entire route into the "hops" list,
		 *	backtracking to root, and compute the time
		 *	at which the route will become unusable.	*/

		route->hops = sm_list_create_unlocked(ionwm);
		if (route->hops == 0)
		{
			writeMemo("[i] CGR: skipping route - can't"
					" create hops list.");
			return 0;
		}

		earliestEndTime = MAX_TIME;
		contact = finalContact;
		while (contact)
		{
			work = (CgrContactNote *) psp(ionwm,
					contact->routingObject);
			if (contact->toTime < earliestEndTime)
			{
				earliestEndTime = contact->toTime;
			}

			route->arrivalConfidence *= contact->confidence;
			addr = psa(ionwm, contact);
			TRACE(CgrHop, contact->fromFqnn, contact->toFqnn);
			citation = sm_list_insert_first(ionwm, route->hops,
					addr);

			/*	Content of citation (which is a list
			 *	element) is the address of the contact
			 *	that is this hop of this route.
			 *
			 *	Content of new member of contact's
			 *	citations list is the address of this
			 *	list element.				*/

			if (citation == 0)
			{
				putErrmsg("Can't insert contact into route.",
						NULL);
				return -1;
			}

			if (contact->citations == 0)
			{
				contact->citations = sm_list_create_unlocked(ionwm);
				if (contact->citations == 0)
				{
					writeMemo("[i] CGR: skipping route -"
						" can't create citation"
						" list.");
					return 0;
				}
			}

			if (sm_list_insert_last(ionwm, contact->citations,
					citation) == 0)
			{
				putErrmsg("Can't insert contact into route.",
						NULL);
				return -1;
			}

			firstContact = contact;
			contact = work->predecessor;
			if (contact == rootContact)
			{
				break;
			}
		}

		/*	Now use the first contact in the route to
		 *	characterize the route.				*/

		route->toFqnn = firstContact->toFqnn;
		route->fromTime = firstContact->fromTime;
		route->toTime = earliestEndTime;
	}

	return 0;
}

static int	computeRoute(PsmPartition ionwm, PsmAddress rootContactElt,
			IonNode *terminusNode, time_t currentTime,
			PsmAddress excludedEdges, PsmAddress *routeAddr,
			CgrTrace *trace)
{
	IonCXref	*rootContact;
	CgrContactNote	*rootWork;
	PsmAddress	rootOfSpur;
	PsmAddress	addr;
	CgrRoute	*route;
	IonCXref	graphRoot;
	CgrContactNote	graphRootWork;

	*routeAddr = 0;		/*	Default.			*/
	if (rootContactElt)	/*	Computing route from waypoint.	*/
	{
		PsmAddress	rootContactAddr;

//puts("*** Starting at a waypoint of the last selected route. ***");
		rootContactAddr = sm_list_data(ionwm, rootContactElt);
		rootContact = (IonCXref *) psp(ionwm, rootContactAddr);

		/*	Defensive + self-diagnosing (#1133).  rootContactElt
		 *	is a spur waypoint -- an SmListElt in some other route's
		 *	hops list.  If the cited contact was purged (its citation
		 *	data zeroed) or the hops list was freed and recycled
		 *	(severSpurReferences should now prevent the latter), the
		 *	waypoint resolves to a NULL or recycled rootContact whose
		 *	toFqnn is 0.  Passing that to ionRegionOf via
		 *	computeDistanceToTerminus aborts ipnfw on
		 *	CHKERR(fqnnA > 0).  Detect it here -- before any deref of
		 *	rootContact -- log enough to pin the lifecycle bug, and
		 *	treat the waypoint as unroutable.			*/

		if (rootContactAddr == 0 || rootContact == NULL
		|| rootContact->fromFqnn == 0 || rootContact->toFqnn == 0)
		{
			char		dbg[256];
			PsmAddress	listAddr = sm_list_list(ionwm,
						rootContactElt);

			isprintf(dbg, sizeof dbg, "[?] CGR: skipping stale spur \
waypoint (#1133): rootContactElt=" UVAST_FIELDSPEC " rootContactAddr=" \
UVAST_FIELDSPEC " fromFqnn=" UVAST_FIELDSPEC " toFqnn=" UVAST_FIELDSPEC " list=" \
UVAST_FIELDSPEC ".", (uvast) rootContactElt, (uvast) rootContactAddr,
					(uvast) (rootContact ?
						rootContact->fromFqnn : 0),
					(uvast) (rootContact ?
						rootContact->toFqnn : 0),
					(uvast) listAddr);
			writeMemo(dbg);
			return 0;	/*	No route from this waypoint.	*/
		}

		if (rootContact->toFqnn == terminusNode->fqnn)
		{
			/*	No forwarding from destination.		*/

			return 0;
		}

		rootWork = (CgrContactNote *) psp(ionwm,
				rootContact->routingObject);
		rootOfSpur = rootContactElt;
	}
	else	 /*	Computing route from root of contact graph.	*/
	{
//puts("*** Starting at root of contact graph. ***");

		memset((char *) &graphRoot, 0, sizeof graphRoot);
		graphRoot.fromFqnn = graphRoot.toFqnn = getOwnFqnn();
		rootContact = &graphRoot;
		memset((char *) &graphRootWork, 0, sizeof graphRootWork);
		graphRootWork.arrivalTime = currentTime;
		rootWork = &graphRootWork;
		rootOfSpur = 0;
	}

	addr = psm_zalloc(ionwm, sizeof(CgrRoute));
	if (addr == 0)
	{
		putErrmsg("Can't create CGR route.", NULL);
		return -1;
	}

	route = (CgrRoute *) psp(ionwm, addr);
	memset((char *) route, 0, sizeof(CgrRoute));
	route->rootOfSpur = rootOfSpur;

	/*	Run Dijkstra search.					*/

	if (computeDistanceToTerminus(rootContact, rootWork, terminusNode,
			currentTime, excludedEdges, route, trace) < 0)
	{
		destroyRouteHops(ionwm, route);
		psm_free(ionwm, addr);
		putErrmsg("Can't finish Dijstra search.", NULL);
		return -1;
	}

	if (route->toFqnn == 0)
	{
		TRACE(CgrNoMoreRoutes);

		/*	No more routes in graph.			*/

		destroyRouteHops(ionwm, route);
		psm_free(ionwm, addr);
		*routeAddr = 0;
	}
	else
	{
		TRACE(CgrProposeRoute, route->toFqnn,
				(unsigned int)(route->fromTime),
				(unsigned int)(route->arrivalTime));

		/*	Found best route, given current exclusions.	*/

		*routeAddr = addr;
	}

	return 0;
}

static int	insertFirstRoute(IonNode *terminusNode, time_t currentTime,
			CgrTrace *trace)
{
	PsmPartition	ionwm = getIonwm();
	IonVdb		*ionvdb = getIonVdb();
	CgrVdb		*cgrvdb	= cgr_get_vdb();
	PsmAddress	routeAddr;
	CgrRtgObject	*routingObj;
	CgrRoute	*route;

//puts("***Inserting first route.***");
	CHKERR(ionwm);
	CHKERR(ionvdb);
	CHKERR(cgrvdb);

	/*	Find first route.					*/

	clearWorkAreas(NULL);
	if (computeRoute(ionwm, 0, terminusNode, currentTime, 0, &routeAddr,
			trace) < 0)
	{
		putErrmsg("Can't insert first route.", NULL);
		return -1;
	}

	if (routeAddr == 0)
	{
		return 0;	/*	No first route found.		*/
	}

	/*	Found best possible route.				*/

	routingObj = (CgrRtgObject *) psp(ionwm, terminusNode->routingObject);
	route = (CgrRoute *) psp(ionwm, routeAddr);
	route->referenceElt = sm_list_insert_last(ionwm,
			routingObj->selectedRoutes, routeAddr);
	if (route->referenceElt == 0)
	{
		putErrmsg("Can't add route to list.", NULL);
		return -1;
	}

	return 0;
}

static int	computeSpurRoute(PsmPartition ionwm, IonNode *terminusNode,
			CgrRoute *lastSelectedRoute, time_t currentTime,
			PsmAddress rootOfSpur, CgrRtgObject *routingObj,
			CgrTrace *trace)
{
//	char		nbrBuf[FQN_MAX_LENGTH];
	PsmAddress	rootOfSpurAddr;
	PsmAddress	excludedEdges;
	PsmAddress	contactElt;
	PsmAddress	nextContactElt;
	PsmAddress	contactAddr;
	IonCXref	*contact;
	CgrContactNote	*work;
	unsigned int	owlt;
	PsmAddress	routeElt;
	PsmAddress	nextRouteElt;
	PsmAddress	routeAddr;
	CgrRoute	*route = NULL;
	PsmAddress	rootPathContactElt;
	PsmAddress	nextRootPathContactElt;
	PsmAddress	rootPathContactAddr;
	int		result;
	PsmAddress	newRouteAddr;
	PsmAddress	citation;
	CgrRoute	*newRoute;

	/*	Guard (UAF): lastSelectedRoute is the route we are spurring
	 *	from; its hops list is walked below -- and again when locating
	 *	rootOfNextSpur -- without further validation.  Under heavy
	 *	contact-plan churn that route can have been freed/recycled,
	 *	leaving a corrupt hops handle that aborts ipnfw in sm_list_first
	 *	(the smlist.c "list" assertion observed in computeSpurRoute).
	 *	If the hops list is unsound there are no spurs to compute from
	 *	it; bail out cleanly.					*/

	if (lastSelectedRoute == NULL
	|| !cgrHandleSane(ionwm, lastSelectedRoute->hops)
	|| !sm_list_header_sane(ionwm, lastSelectedRoute->hops))
	{
		writeMemo("[?] CGR: skipping spur - lastSelectedRoute hops list \
is corrupt (abandoning spur computation).");
		return 0;
	}

//puts("*** Computing a spur route. ***");
	if (rootOfSpur == 0)
	{
		rootOfSpurAddr = 0;
		clearWorkAreas(NULL);
	}
	else
	{
		rootOfSpurAddr = sm_list_data(ionwm, rootOfSpur);
		clearWorkAreas((IonCXref *) psp(ionwm, rootOfSpurAddr));
	}

	excludedEdges = sm_list_create_unlocked(ionwm);
	if (excludedEdges == 0)
	{
		writeMemo("[i] CGR: skipping spur - can't create"
				" excludedEdges list.");
		return 0;
	}

	/*	Suppress contacts that would introduce loops, i.e.,
	 *	all contacts on the root path for this spur path.
	 *	But restore arrival times for those contacts so that
	 *	the computed arrival times of non-suppressed contacts
	 *	will be correct.					*/

	if (rootOfSpur != 0)
	{
//puts("*** Suppressing contacts on root path. ***");
		contactElt = sm_list_prev(ionwm, rootOfSpur);
		while (contactElt)
		{
			contactAddr = sm_list_data(ionwm, contactElt);
			contact = (IonCXref *) psp(ionwm, contactAddr);
			CHKERR(work = getWorkArea(ionwm, contact));
			if (getApplicableRange(contact, &owlt) >= 0)
			{
				/*	 Got OWLT; add some margin.	*/

				owlt += ((MAX_SPEED_MPH/3600) * owlt) / 186282;
			}

			work->arrivalTime = contact->fromTime + owlt;
			work->suppressed = 1;
//putFqn(nbrBuf, contact->toFqnn); debugPrint("*** Suppressing contact to node %s on root path. ***\n", nbrBuf);
			contactElt = sm_list_prev(ionwm, contactElt);
		}
	}

	/*	Exclude edges that would introduce duplicates: for
	 *	each existing route that has this same root path,
	 *	exclude the edge from the end of the root path to
	 *	the first subsequent contact.				*/

//printf("*** rootOfSpurAddr is " UVAST_FIELDSPEC ". ***\n", rootOfSpurAddr);
	for (routeElt = sm_list_first(ionwm, routingObj->selectedRoutes);
			routeElt; routeElt = nextRouteElt)
	{
		nextRouteElt = sm_list_next(ionwm, routeElt);
		if (disabledRoute(ionwm, routeElt, &routeAddr, &route))
		{
			continue;
		}

//puts("*** Looking for edges to exclude on a selected route. ***");
		nextContactElt = sm_list_first(ionwm, route->hops);
		nextRootPathContactElt = sm_list_first(ionwm,
				lastSelectedRoute->hops);
		contactAddr = 0;
		rootPathContactAddr = 0;
		while (1)
		{
//printf("*** rootPathContactAddr is " UVAST_FIELDSPEC ". ***\n", rootPathContactAddr);
			if (contactAddr != rootPathContactAddr)
			{
//puts("*** Root paths diverge at this point. ***");
				/*	No shared root path, so
				 *	end review of this route.	*/

				break;
			}

//puts("*** Root path is the same up to this point. ***");
			if (rootPathContactAddr == rootOfSpurAddr)
			{
				/*	Entire root path is shared,
				 *	so suppress the next contact
				 *	in this route and end review
				 *	of this route.			*/

				if (nextContactElt)
				{
					contactAddr = sm_list_data(ionwm,
							nextContactElt);
					if (contactAddr)
					{
						if (sm_list_insert_last(ionwm,
							excludedEdges,
							contactAddr) == 0)
						{
							putErrmsg("Can't add \
excluded edge.", NULL);
							sm_list_destroy(ionwm,
								excludedEdges,
								NULL, NULL);
							return -1;
						}

//contact = (IonCXref *) psp(ionwm, contactAddr);
//putFqn(nbrBuf, contact->toFqnn); printf("*** Suppressing contact to node %s after end of root path. ***\n", nbrBuf);
					}
				}

				break;
			}

			/*	Up to this point, this route's root
			 *	path is the same as that of the new
			 *	spur route we are computing.  Review
			 *	of this route must continue.		*/

			if (nextContactElt == 0 || nextRootPathContactElt == 0)
			{
//puts("*** Reached end of route before reaching root of spur! ***");
				/*	Root paths diverge, end review.	*/

				break;
			}

//puts("*** Preparing to check next root path contact. ***");
			contactElt = nextContactElt;
			contactAddr = sm_list_data(ionwm, contactElt);
			if (contactAddr == 0)
			{
				break;
			}

			nextContactElt = sm_list_next(ionwm, contactElt);
			rootPathContactElt = nextRootPathContactElt;
			rootPathContactAddr = sm_list_data(ionwm,
					rootPathContactElt);
			if (rootPathContactAddr == 0)
			{
				break;
			}

			nextRootPathContactElt = sm_list_next(ionwm,
					rootPathContactElt);
		}
//puts("*** Done looking for contacts to exclude on that selected route. ***");
	}

//puts("*** Done looking for contacts to exclude on selected routes. ***");

	/*	Compute best route, within these constraints.		*/

	result = computeRoute(ionwm, rootOfSpur, terminusNode, currentTime,
			excludedEdges, &newRouteAddr, trace);
	sm_list_destroy(ionwm, excludedEdges, NULL, NULL);
	if (result < 0)
	{
		putErrmsg("Can't compute route.", NULL);
		return -1;
	}

	if (newRouteAddr == 0)		/*	No route found.		*/
	{
//puts("*** No newly computed route reported. ***");
		return 0;
	}

	newRoute = (CgrRoute *) psp(ionwm, newRouteAddr);
	newRoute->rootOfSpur = rootOfSpur;

	/*	Prepend common trunk route to the spur route.		*/

	contact = NULL;
	contactElt = rootOfSpur;
	while (contactElt)
	{
//puts("*** Prepending a contact from trunk to spur route. ***");
		contactAddr = sm_list_data(ionwm, contactElt);
		contact = (IonCXref *) psp(ionwm, contactAddr);
		TRACE(CgrHop, contact->fromFqnn, contact->toFqnn);
		citation = sm_list_insert_first(ionwm, newRoute->hops,
				contactAddr);
		if (citation == 0)
		{
			putErrmsg("Can't prepend trunk to spur route.", NULL);
			destroyRouteHops(ionwm, newRoute);
			psm_free(ionwm, newRouteAddr);
			return -1;
		}

		/*	Cross-reference this hop with the contact it cites,
		 *	exactly as the Dijkstra backtrack does (#1048).
		 *	Register the new hop element in the contact's
		 *	citations list so that deleteContact() zeroes it
		 *	when the contact is purged; otherwise a purged trunk
		 *	contact leaves a stale pointer in this spur route's
		 *	hops list.					*/

		if (contact->citations == 0)
		{
			contact->citations = sm_list_create_unlocked(ionwm);
			if (contact->citations == 0)
			{
				putErrmsg("Can't create citation list for spur \
hop.", NULL);
				destroyRouteHops(ionwm, newRoute);
				psm_free(ionwm, newRouteAddr);
				return -1;
			}
		}

		if (sm_list_insert_last(ionwm, contact->citations, citation)
				== 0)
		{
			putErrmsg("Can't cross-reference spur hop.", NULL);
			destroyRouteHops(ionwm, newRoute);
			psm_free(ionwm, newRouteAddr);
			return -1;
		}

		contactElt = sm_list_prev(ionwm, contactElt);
	}

	/*	Have now navigated back through all contacts in the
	 *	route.  Make sure the first of the contacts preceding
	 *	the root of the spur within this route - if any - is
	 *	used to characterize the route.				*/

	if (contact)
	{
		newRoute->toFqnn = contact->toFqnn;
		newRoute->fromTime = contact->fromTime;
	}

	/*	Append new route to list of known routes.		*/

//puts("*** Appending newly computed route to list B. ***");
	newRoute->referenceElt = sm_list_insert_last(ionwm,
			routingObj->knownRoutes, newRouteAddr);
	if (newRoute->referenceElt == 0)
	{
		putErrmsg("Can't append known route.", NULL);
		destroyRouteHops(ionwm, newRoute);
		psm_free(ionwm, newRouteAddr);
		return -1;
	}

	return 0;
}

static int	computeAnotherRoute(IonNode *terminusNode,
			CgrRoute *lastSelectedRoute, time_t currentTime,
			PsmAddress *elt, CgrTrace *trace)
{
	PsmPartition	ionwm = getIonwm();
	PsmAddress	rootOfSpur;	/*	An SmListElt in hops.	*/
	PsmAddress	rootOfNextSpur;	/*	An SmListElt in hops.	*/
	CgrRtgObject	*routingObj;
	PsmAddress	elt2;
	PsmAddress	nextRouteElt;
	PsmAddress	knownRouteAddr;
	CgrRoute	*knownRoute;
	PsmAddress	bestKnownRouteElt;
	PsmAddress	bestKnownRouteAddr = 0;
	CgrRoute	*bestKnownRoute = 0;

//puts("*** Computing another route. ***");
	*elt = 0;	/*	Default: no new route found.		*/

	/*	This code implements the Lawler modification of Yen's
	 *	algorithm.						*/

	rootOfSpur = lastSelectedRoute->rootOfSpur;
	if (rootOfSpur == 0)
	{
		/*	Last selected route branched from the root
		 *	of the graph, so must compute spur routes
		 *	from ALL hops of the last selected route.	*/

		rootOfNextSpur = sm_list_first(ionwm, lastSelectedRoute->hops);
	}
	else
	{
		/*	Last selected route branched off from some
		 *	hop of the *previous* selected route, so must
		 *	compute spur routes only from hops starting
		 *	with that branch point - and then graft all
		 *	earlier hops of the last selected route onto
		 *	the front of each computed spur route.		*/

		rootOfNextSpur = sm_list_next(ionwm, rootOfSpur);
	}

	routingObj = (CgrRtgObject *) psp(ionwm, terminusNode->routingObject);

	/*	Compute spur routes that branch off the current last
	 *	selected route, inserting them into Yen's "list B".	*/

	while (1)
	{
		if (computeSpurRoute(ionwm, terminusNode, lastSelectedRoute,
				currentTime, rootOfSpur, routingObj, trace) < 0)
		{
			putErrmsg("Failed computing spur route.", NULL);
			return -1;
		}

		rootOfSpur = rootOfNextSpur;
		if (rootOfSpur == 0)
		{
			/*	Receiver of prior root contact was
			 *	the destination.			*/

			break;	/*	No more spur routes.		*/
		}

		rootOfNextSpur = sm_list_next(ionwm, rootOfSpur);
	}

//puts("*** Finished computing all spur routes from last selected route. ***");

	/*	Move the best (lowest-cost) route in the list of
	 *	known routes (Yen's "list B") into the list of
	 *	selected routes (Yen's "list A").			*/

	bestKnownRouteElt = 0;
	for (elt2 = sm_list_first(ionwm, routingObj->knownRoutes); elt2;
			elt2 = nextRouteElt)
	{
//puts("*** Considering a B-list node for migration to A-list. ***");
		nextRouteElt = sm_list_next(ionwm, elt2);
		if (disabledRoute(ionwm, elt2, &knownRouteAddr, &knownRoute))
		{
			continue;
		}

		/*	Determine whether or not this route is the
		 *	best route in the B list. Preference is by
		 *	ascending arrival time.				*/

		if (bestKnownRouteElt == 0)
		{
			bestKnownRouteElt = elt2;
			bestKnownRouteAddr = knownRouteAddr;
			bestKnownRoute = knownRoute;
			continue;
		}

		if (knownRoute->arrivalTime < bestKnownRoute->arrivalTime)
		{
			bestKnownRouteElt = elt2;
			bestKnownRouteAddr = knownRouteAddr;
			bestKnownRoute = knownRoute;
		}
	}

	if (bestKnownRouteElt)
	{
		PsmAddress	existingElt;

//puts("*** Migrating best route in list B into list A. ***");

		/*	#1048: if this route is already cited in selectedRoutes
		 *	-- e.g. a recomputed spur that duplicates an already-
		 *	selected route (possibly via a recycled address) -- do
		 *	NOT add a second citation.  A duplicate selectedRoutes
		 *	element orphans one citation, which later dereferences a
		 *	freed/recycled route and is dropped as a malformed route.
		 *	Reuse the existing element and just drop the redundant
		 *	knownRoutes element.					*/

		existingElt = firstRouteListElt(ionwm,
				routingObj->selectedRoutes, bestKnownRouteAddr);
		if (existingElt)
		{
			sm_list_delete(ionwm, bestKnownRouteElt, NULL, NULL);
			bestKnownRoute->referenceElt = existingElt;
			*elt = existingElt;
		}
		else
		{
			*elt = sm_list_insert_last(ionwm,
					routingObj->selectedRoutes,
					bestKnownRouteAddr);
			if (*elt == 0)
			{
				putErrmsg("Can't insert selected route.", NULL);
				return -1;
			}

			/*	#1054 tripwire: a known route's only reference
			 *	should be the knownRoutes element we are migrating
			 *	from.  If referenceElt points at some other
			 *	element, overwriting it below would orphan that
			 *	element (still citing this route) -- the suspected
			 *	source of the disabledRoute use-after-free.  Log it
			 *	and delete the stale reference so the route cannot
			 *	be left dangling.				*/

			if (bestKnownRoute->referenceElt
			&& bestKnownRoute->referenceElt != bestKnownRouteElt)
			{
				char	memo[160];

				isprintf(memo, sizeof memo, "[?] CGR: migrating \
route with referenceElt " UVAST_FIELDSPEC " != listElt " UVAST_FIELDSPEC \
" (#1054).", (uvast) bestKnownRoute->referenceElt,
						(uvast) bestKnownRouteElt);
				writeMemo(memo);
				sm_list_delete(ionwm, bestKnownRoute->referenceElt,
						NULL, NULL);
			}

			sm_list_delete(ionwm, bestKnownRouteElt, NULL, NULL);
			bestKnownRoute->referenceElt = *elt;
		}
	}
//else puts("*** No routes in list B to migrate into list A. ***");

	return 0;
}

/*	Functions for selecting which node(s) to forward a bundle to.	*/

static int	isExcluded(uvast fqnn, Lyst excludedNodes)
{
	LystElt	elt;

	for (elt = lyst_first(excludedNodes); elt; elt = lyst_next(elt))
	{
		if (((uaddr) lyst_data(elt)) == fqnn)
		{
			return 1;	/*	Node is in the list.	*/
		}
	}

	return 0;
}

static time_t	computePBAT(CgrRoute *route, Bundle *bundle,
			time_t currentTime, BpPlan *plan)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();
	IonVdb		*vdb = getIonVdb();
	uvast		ownNodeNbr = getOwnFqnn();
	Scalar		priorClaims;
	Scalar		totalBacklog;
	IonCXref	arg;
	PsmAddress	elt;
	PsmAddress	contactAddr;
	IonCXref	*contact;
	Scalar		volume;
	Scalar		allotment;
	time_t		startTime;
	time_t		endTime;
	int		secRemaining;
	time_t		firstByteTransmitTime;
	time_t		lastByteTransmitTime;
	int		doNotFragment;
	Scalar		radiationLatency;
	unsigned int	owlt;
	unsigned int	owltMargin;
	time_t		acqTime;
	SdrObject	contactObj;
	IonContact	contactBuf;
	int		priority;
	time_t		effectiveStartTime;
	time_t		effectiveStopTime;
	time_t		effectiveDuration;
	double		effectiveVolumeLimit;
	PsmAddress	elt2;

	computePriorClaims(plan, bundle, &priorClaims, &totalBacklog);
	copyScalar(&(route->committed), &totalBacklog);

	/*	Reduce prior claims on the first contact in this
	 *	route by all transmission to this contact's neighbor
	 *	that will be performed during contacts that precede
	 *	this contact.  That is, the following loop examines
	 *	only contacts that occur *before* the start of this
	 *	particular route.					*/

	loadScalar(&allotment, 0);
	loadScalar(&volume, 0);
	memset((char *) &arg, 0, sizeof(IonCXref));
	oK(ionRegionOf(ownNodeNbr, route->toFqnn, &arg.regionNbr));
	arg.fromFqnn = ownNodeNbr;
	arg.toFqnn = route->toFqnn;
	for (oK(sm_rbt_search(ionwm, vdb->contactIndex, rfx_order_contacts,
			&arg, &elt)); elt; elt = sm_rbt_next(ionwm, elt))
	{
		contact = (IonCXref *) psp(ionwm, sm_rbt_data(ionwm, elt));
		if (contact->fromFqnn > ownNodeNbr
		|| contact->toFqnn > route->toFqnn
		|| contact->fromTime > route->fromTime)
		{
			/*	The first contact on this route no
			 *	longer exists.  I.e., the initial
			 *	contact on the route has expired
			 *	and has been removed (but the route
			 *	itself has not yet been removed).
			 *
			 *	Note that nominal exit from this loop
			 *	appears at the end, upon checking
			 *	whether or not this contact is the
			 *	first contact on the route.		*/

			return 0;
		}

		if (contact->toTime <= currentTime)
		{
			/*	This contact has already terminated.	*/

			continue;
		}

		/*	Compute available volume of contact.		*/

		if (currentTime > contact->fromTime)
		{
			startTime = currentTime;
		}
		else
		{
			startTime = contact->fromTime;
		}

		endTime = contact->toTime;
		secRemaining = endTime - startTime;
		loadScalar(&volume, secRemaining);
		multiplyScalar(&volume, contact->xmitRate);

		/*	Determine how much spare volume the
		 *	contact has.					*/

		copyScalar(&allotment, &volume);
		subtractFromScalar(&allotment, &(route->committed));
		if (!scalarIsValid(&allotment))
		{
			/*	Volume is less than remaining
			 *	backlog, so the contact is fully
			 *	subscribed.				*/

			copyScalar(&allotment, &volume);
		}
		else
		{
			/*	Volume is greater than or equal to
			 *	the remaining backlog, so the last of
			 *	the backlog will be served by this
			 *	contact, possibly with some volume
			 *	left over.				*/

			copyScalar(&allotment, &(route->committed));
		}

		/*	Determine how much of the total backlog has
		 *	been allotted to subsequent contacts.		*/

		subtractFromScalar(&(route->committed), &volume);
		if (!scalarIsValid(&(route->committed)))
		{
			/*	No bundles scheduled for transmission
			 *	during any subsequent contacts.		*/

			loadScalar(&(route->committed), 0);
		}

		/*	Loop limit check.				*/

		if (contact->fromTime >= route->fromTime)
		{
			/*	This is the initial contact on the
			 *	route we are considering.  All prior
			 *	contacts have been allocated to prior
			 *	transmission claims.  So we have
			 *	finished dispositioning priorClaims.	*/

			break;
		}

		/*	This is a contact that precedes the initial
		 *	contact on the route we are considering.
		 *	Determine how much of the prior claims on
		 *	the route's first contact will be served by
		 *	this contact.					*/

		subtractFromScalar(&priorClaims, &volume);
		if (!scalarIsValid(&priorClaims))
		{
			/*	Last of the prior claims will be
			 *	served by this contact.			*/

			loadScalar(&priorClaims, 0);
		}
	}

	/*	At this point, priorClaims contains the applicable
	 *	"residual backlog", so we can check for potential
	 *	overbooking.						*/

	route->bundleECCC = computeECCC(guessBundleSize(bundle));
	copyScalar(&(route->overbooked), &allotment);
	increaseScalar(&(route->overbooked), route->bundleECCC);
	subtractFromScalar(&(route->overbooked), &volume);
	if (!scalarIsValid(&(route->overbooked)))
	{
		loadScalar(&(route->overbooked), 0);
	}

	/*	Now consider the initial contact on the route.		*/

	elt = sm_list_first(ionwm, route->hops);
	contactAddr = sm_list_data(ionwm, elt);
	if (contactAddr == 0)
	{
		/*	First contact deleted, route is not usable.	*/

		return 0;
	}

	contact = (IonCXref *) psp(ionwm, contactAddr);
	if (contact->xmitRate == 0)
	{
		/* First contact is inactive, route not usable. */
		return 0;
	}

	/*	Compute the expected initial transmit time
	 *	(Earliest Transmission Opportunity): start of
	 *	initial contact plus delay imposed by transmitting
	 *	all remaining prior claims, at the transmission
	 *	rate of the initial contact.				*/

	if (currentTime > contact->fromTime)
	{
		firstByteTransmitTime = currentTime;
	}
	else
	{
		firstByteTransmitTime = contact->fromTime;
	}

	lastByteTransmitTime = firstByteTransmitTime;

	/*	Add time to transmit everything preceding 1st byte.	*/

	copyScalar(&radiationLatency, &priorClaims);
	divideScalar(&radiationLatency, contact->xmitRate);
	firstByteTransmitTime += ((ONE_GIG * radiationLatency.gigs)
			+ radiationLatency.units);
	route->eto = firstByteTransmitTime;

	/*	Add time to transmit everything preceding last byte.	*/

	copyScalar(&radiationLatency, &priorClaims);
	increaseScalar(&radiationLatency, route->bundleECCC);
	divideScalar(&radiationLatency, contact->xmitRate);
	lastByteTransmitTime += ((ONE_GIG * radiationLatency.gigs)
			+ radiationLatency.units);

	/*	Determine whether or not fragmentation of this bundle
	 *	is prohibited.						*/

	doNotFragment = bundle->bundleProcFlags & BDL_DOES_NOT_FRAGMENT;
	route->maxVolumeAvbl = route->bundleECCC;

	/*	Now compute expected bundle delivery time by adding
	 *	OWLTs, inter-contact delays, and per-hop radiation
	 *	latencies along the path to the terminus node.  In
	 *	so doing, ensure that EVL is not fully depleted at
	 *	any contact in the path.				*/

	while (1)
	{
		if (contact->toTime <= firstByteTransmitTime)
		{
			/*	Due to the volume of transmission
			 *	that must precede it, this bundle
			 *	can't be transmitted during this
			 *	contact.  So the route is unusable.
			 *
			 *	Note that transmit time is computed
			 *	using integer arithmetic, which will
			 *	truncate any fractional seconds of
			 *	total transmission time.  To account
			 *	for this rounding error, we require
			 *	that the computed first byte transmit
			 *	time be LESS than the contact end
			 *	time, rather than merely not greater.	*/

			return 0;
		}

		if (getApplicableRange(contact, &owlt) < 0)
		{
			/*	Can't determine owlt for this contact,
			 *	so delivery time can't be computed.
			 *	Route is not usable.			*/

			return 0;
		}

		owltMargin = ((MAX_SPEED_MPH / 3600) * owlt) / 186282;
		acqTime = lastByteTransmitTime + owlt + owltMargin;

		/*	Ensure that the contact is not depleted.	*/

		priority = bundle->priority;
		contactObj = sdr_list_data(sdr, contact->contactElt);
		sdr_read(sdr, (char *) &contactBuf, contactObj,
				sizeof(IonContact));
		if (contactBuf.mtv[priority] <= 0.0)
		{
			return 0;	/*	Unconditional depletion.*/
		}

		effectiveStartTime = firstByteTransmitTime;

		/*	(Effective stop time is the earliest stop
		 *	time among this contact and all successors.)	*/

		effectiveStopTime = contact->toTime;
		elt2 = sm_list_next(ionwm, elt);
		while (elt2)
		{
			contact = (IonCXref *) psp(ionwm, sm_list_data(ionwm,
					elt2));
			if (contact->toTime < effectiveStopTime)
			{
				effectiveStopTime = contact->toTime;
			}

			elt2 = sm_list_next(ionwm, elt2);
		}

		effectiveDuration = effectiveStopTime - effectiveStartTime;
		if (effectiveDuration <= 0)
		{
			return 0;	/*	Conditional depletion.	*/
		}

		effectiveVolumeLimit = effectiveDuration * contactBuf.xmitRate;
		if (contactBuf.mtv[priority] < effectiveVolumeLimit)
		{
			effectiveVolumeLimit = contactBuf.mtv[priority];
		}

		if (effectiveVolumeLimit < route->maxVolumeAvbl)
		{
			/*	Contact is too brief for transmission
			 *	of entire bundle.			*/

			if (doNotFragment)
			{
				/*	Fragmentation not permitted,
				 *	so the route is unusable.	*/

				return 0;
			}

			route->maxVolumeAvbl = effectiveVolumeLimit;
		}

		/*	Now check next contact in the end-to-end path.	*/

		elt = sm_list_next(ionwm, elt);
		if (elt == 0)
		{
			break;	/*	End of route.			*/
		}

		/*	Not end of route, so the "to" node for this
		 *	contact is not the terminus node, i.e., the
		 *	bundle must be forwarded from this node.	*/

		contact = (IonCXref *) psp(ionwm, sm_list_data(ionwm, elt));
		if (acqTime > contact->fromTime)
		{
			firstByteTransmitTime = acqTime;
		}
		else
		{
			firstByteTransmitTime = contact->fromTime;
		}

		/*	Consider additional latency imposed by the
		 *	time required to transmit all bytes of the
		 *	bundle.  At each hop of the path, additional
		 *	radiation latency is computed as bundle size
		 *	divided by data rate.				*/

		lastByteTransmitTime = firstByteTransmitTime;
		loadScalar(&radiationLatency, route->bundleECCC);
		divideScalar(&radiationLatency, contact->xmitRate);
		lastByteTransmitTime += ((ONE_GIG * radiationLatency.gigs)
				+ radiationLatency.units);
	}

	if (acqTime > bundle->expirationTime)
	{
		/*	Bundle will never arrive: it will expire
		 *	before arrival.					*/

		acqTime = 0;
	}

	route->pbat = acqTime;
	return acqTime;
}

#endif

float	cgr_get_dlv_confidence(Bundle *bundle, CgrRoute *route)
{
	float		dlvFailureConfidence;

	/*	Delivery of bundle fails if and only if all forwarded
	 *	copies fail to arrive.  Our confidence that this will
	 *	happen is the product of our confidence in the delivery
	 *	failures of all forwarded copies, each of which is
	 *	1.0 minus our confidence that this copy will arrive.	*/

	dlvFailureConfidence = (1.0 - bundle->dlvConfidence)
			* (1.0 - route->arrivalConfidence);
	return (1.0 - dlvFailureConfidence);
}

#if !UNIBO_CGR

static int	tryRoute(CgrRoute *route, time_t currentTime, Bundle *bundle,
			CgrTrace *trace, Lyst bestRoutes)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	wm = getIonwm();
	char		nbrBuf[FQN_MAX_LENGTH];
	char		eid[SDRSTRING_BUFSZ];
	VPlan		*vplan;
	PsmAddress	vplanElt;
	SdrObject	planObj;
	BpPlan		plan;
	time_t		pbat;
	LystElt		candidateElt;
	CgrRoute	*candidateRoute;

	putFqn(nbrBuf, route->toFqnn);
	isprintf(eid, sizeof eid, "ipn:%s.0", nbrBuf);
	findPlan(eid, &vplan, &vplanElt);
	if (vplanElt == 0)
	{
		TRACE(CgrExcludeRoute, CgrNoPlan);
		return 0;		/*	No egress plan to node.	*/
	}

	planObj = sdr_list_data(sdr, vplan->planElt);
	sdr_read(sdr, (char *) &plan, planObj, sizeof(BpPlan));

	/*	Now determine whether or not the bundle could be sent
	 *	to this neighbor via the applicable egress plan in
	 *	time to follow the route that is being considered.
	 *	There are two criteria.  First, is the egress plan
	 *	blocked (i.e., temporarily shut off by operations)?	*/

	if (plan.blocked)
	{
		TRACE(CgrExcludeRoute, CgrBlockedPlan);
		return 0;		/*	Node is unreachable.	*/
	}

	/*	Second: if this bundle were sent on this route, given
	 *	all other bundles enqueued ahead of it, could it make
	 *	all of its contact connections in time to arrive
	 *	before its expiration time?  For this purpose we need
	 *	to scan the scheduled intervals of contact with the
	 *	candidate neighbor.					*/

	pbat = computePBAT(route, bundle, currentTime, &plan);
	if (pbat == 0)			/*	Can't arrive in time.	*/
	{
		TRACE(CgrExcludeRoute, CgrRouteCongested);
		return 0;		/*	Connections too tight.	*/
	}

	/*	This route is a plausible opportunity for getting
	 *	the bundle forwarded to the terminus node before it
	 *	expires.  But we want only a single route in the
	 *	bestRoutes list.  If this route's arrival time is
	 *	earlier than that of our current bestRoutes candidate
	 *	(if any), or its arrival time is the same but other
	 *	qualities make it better, then we add this route to
	 *	the list of best routes for this bundle; the current
	 *	bestRoutes candidate route (if any) is removed from
	 *	the list.						*/

	candidateElt = lyst_first(bestRoutes);
	if (candidateElt)	/*	May need to replace this one.	*/
	{
		candidateRoute = (CgrRoute *) lyst_data(candidateElt);
		CHKZERO(candidateRoute);
		if (candidateRoute->pbat < pbat)
		{
			/*	Current candidate is better.		*/

			return 0;
		}

		if (candidateRoute->pbat > pbat)
		{
			/*	This route is better.			*/

//printf("Earlier delivery time: old %lu, new %lu.\n", candidateRoute->pbat, pbat);
			lyst_delete(candidateElt);
		}
		else	/*	Same delivery time.			*/
		{
			if (sm_list_length(wm, candidateRoute->hops) <
					sm_list_length(wm, route->hops))
			{
				/*	Current candidate is better.	*/

				return 0;
			}

			if (sm_list_length(wm, candidateRoute->hops) >
					sm_list_length(wm, route->hops))
			{
				/*	This route is better.		*/

//puts("Fewer hops.");
				lyst_delete(candidateElt);
			}
			else	/*	Same number of hops.		*/
			{
				if (candidateRoute->toTime > route->toTime)
				{
					/*	Current one is better.	*/

					return 0;
				}

				if (candidateRoute->toTime < route->toTime)
				{
					/*	This one is better.	*/

//puts("Later termination time.");
					lyst_delete(candidateElt);
				}
				else	/*	Same termination time.	*/
				{
					if (candidateRoute->toFqnn <
							route->toFqnn)
					{
						/*	Current better.	*/

						return 0;
					}

					/*	This one is better.	*/

//puts("Smaller entry node number.");
					lyst_delete(candidateElt);
				}
			}
		}
	}

	if (lyst_insert_last(bestRoutes, (void *) route) == 0)
	{
		putErrmsg("Can't add route.", NULL);
		return -1;
	}

	TRACE(CgrAddRoute);
	return 0;
}

static int	checkRoute(IonNode *terminusNode, uvast viaNodeNbr,
			PsmAddress *elt, Bundle *bundle, Lyst excludedNodes,
			CgrTrace *trace, Lyst bestRoutes, time_t currentTime,
			time_t deadline)
{
	PsmPartition	ionwm = getIonwm();
	PsmAddress	nextElt;
	CgrRtgObject	*routingObj;
	PsmAddress	elt2 = 0;
	PsmAddress	prevElt2;
	PsmAddress	addr;
	CgrRoute	*route;
	IonCXref	*contact;
	float		newDlvConfidence;
	float		confidenceImprovement;

	/*	Examine next opportunity for transmission to any
	 *	neighboring node that would result in arrival at
	 *	the terminus node.  (If none identified, identify
	 *	the next one.)						*/

	if (*elt)	/*	Have got a route to examine.		*/
	{
		/*	We want to check the next route in the
		 *	selectedRoutes list (if any) after we
		 *	have checked this one.				*/

		nextElt = sm_list_next(ionwm, *elt);
	}
	else		/*	At end of selectedRoutes list.		*/
	{
		/*	If we identify a new candidate route, then
		 *	that route will by definition be at the
		 *	end of the selectedRoutes list; in that case,
		 *	there is no next route to check.		*/

		nextElt = 0;

		/*	Must compute a new selected route.  This
		 *	may entail computing spur routes that branch
		 *	off of the last selected route from which
		 *	spurs have not yet been computed.		*/

		routingObj = (CgrRtgObject *) psp(ionwm,
			       	terminusNode->routingObject);
		for (elt2 = sm_list_last(ionwm, routingObj->selectedRoutes);
				elt2; elt2 = prevElt2)
		{
			prevElt2 = sm_list_prev(ionwm, elt2);
			if (disabledRoute(ionwm, elt2, &addr, &route))
			{
				continue;
			}

			if (route->toTime <= currentTime)
			{
				/*	This route includes a contact
				 *	that has ended; can't use any
				 *	branch from it.			*/

				TRACE(CgrExpiredRoute);
				removeRoute(ionwm, elt2);
				continue;
			}

			if (route->spursComputed)
			{
				/*	Nothing new can come from
				 *	branching off of this route.
				 *	Try the one before it.		*/

				continue;
			}

			route->spursComputed = 1;
			if (computeAnotherRoute(terminusNode, route,
					currentTime, elt, trace))
			{
				putErrmsg("Failed computing another route.",
						NULL);
				return -1;
			}

			if (*elt == 0)	/*	No more routes.		*/
			{
				TRACE(CgrNoMoreRoutes);
				return 0;
			}

			break;
		}

		if (*elt == 0)	/*	No spur routes were computed.	*/
		{
			if (sm_list_length(ionwm, routingObj->selectedRoutes)
					== 0)	/*	Starting fresh.	*/
			{
				TRACE(CgrFirstRoute);
				if (insertFirstRoute(terminusNode, currentTime,
						trace))
				{
					putErrmsg("Failed computing 1st route.",
							NULL);
					return -1;
				}

				/*	Get new last (i.e., first)
				 *	route.				*/

				*elt = sm_list_last(ionwm,
						routingObj->selectedRoutes);
				if (*elt == 0)	/*	No routes.	*/
				{
					TRACE(CgrNoMoreRoutes);
					return 0;
				}
			}
			else	/*	All routing options exhausted.	*/
			{
				TRACE(CgrNoMoreRoutes);
				return 0;
			}
		}
	}

	/*	Check this Selected route.				*/

	if (disabledRoute(ionwm, *elt, &addr, &route))
	{
		TRACE(CgrExpiredRoute);
		*elt = nextElt;
		return 1;
	}

	TRACE(CgrCheckRoute, route->toFqnn, (unsigned int)(route->fromTime),
			(unsigned int)(route->arrivalTime));
	if (route->toTime <= currentTime)
	{
		/*	This route includes a contact that has already
		 *	ended; it is not usable.			*/

		TRACE(CgrExpiredRoute);
		removeRoute(ionwm, *elt);
		*elt = nextElt;
		return 1;
	}

	if (route->arrivalTime > deadline)
	{
		/*	Not a plausible route.				*/

		TRACE(CgrExcludeRoute, CgrRouteTooSlow);
		*elt = nextElt;
		return 1;
	}

	addr = sm_list_data(ionwm, sm_list_first(ionwm, route->hops));
	if (addr == 0)
	{
		/*	First contact deleted, route is not usable.	*/

		TRACE(CgrExpiredRoute);
		removeRoute(ionwm, *elt);
		*elt = nextElt;
		return 1;
	}

	contact = (IonCXref *) psp(ionwm, addr);
	if (contact->confidence != 1.0)
	{
		/*	Initial contact of route must be certain.	*/

		TRACE(CgrUncertainEntry);
		*elt = nextElt;
		return 1;
	}

	/*	If routing a critical bundle, this Selected route
	 *	cannot be chosen as a Best Route if it doesn't
	 *	begin with a contact from the local node to the
	 *	node identified by viaNodeNbr.				*/

	if (viaNodeNbr)	/*	Looking for route via this node.	*/
	{
		if (route->toFqnn != viaNodeNbr)
		{
			TRACE(CgrWrongViaNode);
			*elt = nextElt;
			return 1;
		}
	}
	else		/*	Selecting single best route.		*/
	{
		/*	Skip this candidate if not cost-effective.	*/

		if (bundle->dlvConfidence > 0.0
		&& bundle->dlvConfidence < 1.0)
		{
			newDlvConfidence =
				cgr_get_dlv_confidence(bundle, route);
			confidenceImprovement =
				(newDlvConfidence / bundle->dlvConfidence)
				- 1.0;
			if (confidenceImprovement < MIN_CONFIDENCE_IMPROVEMENT)
			{
				TRACE(CgrExcludeRoute, CgrNoHelp);
				*elt = nextElt;
				return 1;
			}
		}
	}

	/*	Never route to self unless self is the final
	 *	destination.						*/

	if (route->toFqnn == getOwnFqnn())
	{
		/*	Okay if and only if self is the destination.	*/

		if (route->toFqnn != terminusNode->fqnn)
		{
			/*	Can't route to a remote node via self
			 *	-- would be a routing loop.		*/

			TRACE(CgrExcludeRoute, CgrRouteViaSelf);
			*elt = nextElt;
			return 1;
		}
	}

	/*	Is the neighbor that receives bundles during this
	 *	route's initial contact excluded for any reason?	*/

	if (isExcluded(route->toFqnn, excludedNodes))
	{
		TRACE(CgrExcludeRoute, CgrInitialContactExcluded);
		*elt = nextElt;
		return 1;
	}

	/*	Route might work.  If this route is supported by
	 *	contacts with enough aggregate volume to convey
	 *	this bundle and all currently queued bundles of
	 *	equal or higher priority, then this is a candidate
	 *	route for forwarding the bundle to the terminus node.	*/

	if (tryRoute(route, currentTime, bundle, trace, bestRoutes) < 0)
	{
		putErrmsg("Failed trying to consider route.", NULL);
		return -1;
	}

	*elt = nextElt;
	return 1;
}

/*	Bounds on a single forwarding decision's CGR route computation
 *	(#1048).  Without these, when no usable route exists for a bundle
 *	the selection loop below keeps invoking Yen's spur expansion until
 *	the entire route space over the contact plan is enumerated -- a
 *	single call was observed holding the SDR write lock for 44.88 s,
 *	starving every other daemon waiting on sdr_begin_xn.  The time
 *	budget bounds the operational harm (lock-hold duration); the
 *	computed-route cap is a deterministic backstop in case the wall
 *	clock is stepped backward (which would otherwise disable the time
 *	check).  On either bound we stop and use the best route found so
 *	far (if any); otherwise the bundle falls through to limbo and is
 *	retried later -- it is never dropped.  Both are overridable.	*/

#ifndef	CGR_MAX_COMPUTE_USEC
#define	CGR_MAX_COMPUTE_USEC	(250000)	/*	250 ms.		*/
#endif

#ifndef	CGR_MAX_ROUTES_PER_BUNDLE
#define	CGR_MAX_ROUTES_PER_BUNDLE	(100)	/*	Routes computed.*/
#endif

/*	Negative-result cache TTL (#1048): when route enumeration for a
 *	destination comes up completely empty (no geometric route exists,
 *	so the routing object's selectedRoutes list is bare), defer
 *	re-enumerating that destination's route space for this many
 *	seconds.  Repeated bundles to a currently-unroutable destination
 *	then go straight to limbo instead of each paying a full (bounded)
 *	CGR computation, which is what produced the multi-second SDR-
 *	lock-hold storms.  The cache is deliberately NOT armed when
 *	routes exist but none suited the current bundle (e.g., deadline
 *	too tight): that outcome is bundle-specific, evaluating the next
 *	bundle against the cached route list is cheap, and arming on it
 *	was observed to deny routing to viable bundles sent shortly after
 *	a short-lifetime bundle to the same destination.  A contact-plan
 *	change clears the cache (cgr_clear_vdb wipes the routing object);
 *	the short TTL bounds how long a newly-usable route (e.g. from a
 *	contact that just became active) waits to be discovered.	*/

#ifndef	CGR_NEG_CACHE_SECS
#define	CGR_NEG_CACHE_SECS	(1)
#endif

static int	loadBestRoutesList(IonNode *terminusNode, uvast viaNodeNbr,
			Bundle *bundle, Lyst excludedNodes, CgrTrace *trace,
			Lyst bestRoutes, time_t currentTime, time_t deadline,
			CgrRtgObject *routingObj)
{
	PsmPartition	ionwm = getIonwm();
	PsmAddress	elt;
	struct timeval	startTime;
	struct timeval	now;
	long		elapsedUsec;
	unsigned int	routesComputed = 0;

	/*	Perform route selection outer loop until the list
	 *	of best routes contains the single best route for
	 *	transmission of this bundle.				*/

	getCurrentTime(&startTime);
	elt = sm_list_first(ionwm, routingObj->selectedRoutes);
	while (1)
	{
		/*	Bound the lock-hold time of this decision (#1048).
		 *	The check covers both walking cached routes and
		 *	(re)computing new ones.				*/

		getCurrentTime(&now);
		elapsedUsec = ((long) (now.tv_sec - startTime.tv_sec))
				* 1000000L + (now.tv_usec - startTime.tv_usec);
		if (elapsedUsec >= CGR_MAX_COMPUTE_USEC)
		{
			char	cbuf[160];

			isprintf(cbuf, sizeof cbuf, "[?] CGR: route compute \
time-bounded for dest node " UVAST_FIELDSPEC " after %ld us (%u routes \
computed); %d candidate(s) so far.", (uvast) terminusNode->fqnn, elapsedUsec,
					routesComputed, (int) lyst_length(bestRoutes));
			writeMemo(cbuf);
			return 0;
		}

		switch (checkRoute(terminusNode, viaNodeNbr, &elt, bundle,
				excludedNodes, trace, bestRoutes, currentTime,
				deadline))
		{
		case 1:			/*	A route was checked.	*/
			if (elt)	/*	There's another.	*/
			{
				continue;
			}

			/*	At end of selectedRoutes.		*/

			if (lyst_length(bestRoutes) == 0)
			{
				/*	No candidate; force computation
				 *	of another selected route -- the
				 *	unbounded path, so cap it (#1048).*/

				routesComputed++;
				if (routesComputed >= CGR_MAX_ROUTES_PER_BUNDLE)
				{
					char	cbuf[160];

					isprintf(cbuf, sizeof cbuf, "[?] CGR: \
route compute count-bounded for dest node " UVAST_FIELDSPEC " at %u routes; no \
usable route found -> limbo.", (uvast) terminusNode->fqnn, routesComputed);
					writeMemo(cbuf);
					return 0;
				}

				continue;
			}

			/*	Have checked all selectedRoutes and
			 *	picked the best one as candidate.	*/

			return 0;

		case 0:			/*	No more possible routes.*/
			return 0;

		default:
			putErrmsg("Failed checking route to node.",
					utoa(terminusNode->fqnn));
			return -1;
		}
	}
}

static int	loadCriticalBestRoutesList(IonNode *terminusNode,
			Bundle *bundle, Lyst excludedNodes, CgrTrace *trace,
			Lyst bestRoutes, time_t currentTime, time_t deadline,
			CgrRtgObject *routingObj)
{
	PsmPartition	ionwm = getIonwm();
	uvast		ownNodeNbr = getOwnFqnn();
	IonVdb		*ionvdb = getIonVdb();
	PsmAddress	elt;
	IonCXref	*contact;
	PsmAddress	elt2;
	uvast		fqnn;
	Lyst		routes;

	if (routingObj->proximateNodes == 0)
	{
		routingObj->proximateNodes = sm_list_create_unlocked(ionwm);
		if (routingObj->proximateNodes == 0)
		{
			putErrmsg("Can't build list of proximate nodes.",
					utoa(terminusNode->fqnn));
			return -1;
		}

		/*	Identify all proximate nodes by scanning all
		 *	contacts that transmit from the local node.
		 *
		 *	Because the contact index in the IonVdb
		 *	contains all contacts involving nodes in
		 *	all regions in which the local node resides,
		 *	this list of proximate nodes may include nodes
		 *	residing in either the home region or outer
		 *	region (if any) of the local node.		*/

		for (elt = sm_rbt_first(ionwm, ionvdb->contactIndex); elt;
				elt = sm_rbt_next(ionwm, elt))
		{
			contact = (IonCXref *) psp(ionwm, sm_rbt_data(ionwm,
						elt));
			if (contact->fromFqnn != ownNodeNbr)
			{
				continue;
			}

			if (contact->toTime <= currentTime)
			{
				/*	Contact is ended, is about to
				 *	be purged.			*/

				continue;
			}

			for (elt2 = sm_list_first(ionwm,
					routingObj->proximateNodes); elt2;
					elt2 = sm_list_next(ionwm, elt2))
			{
				fqnn = (uvast) sm_list_data(ionwm, elt2);
				if (fqnn < contact->toFqnn)
				{
					continue;
				}

				break;
			}

			if (elt2 == 0)	/*	No greater prox node#.	*/
			{
				if (sm_list_insert_last(ionwm,
					routingObj->proximateNodes,
					(PsmAddress) (contact->toFqnn)) == 0)
				{
					putErrmsg("Can't insert prox node.",
						utoa(terminusNode->fqnn));
					return -1;
				}

				continue;	/*	Next contact.	*/
			}

			if (fqnn == contact->toFqnn)
			{
				/*	Prox node is already in list.	*/

				continue;	/*	Next contact.	*/
			}

			/*	This node number must be inserted here.	*/

			if (sm_list_insert_before(ionwm, elt2,
					(PsmAddress) (contact->toFqnn)) == 0)
			{
				putErrmsg("Can't insert prox node.",
						utoa(terminusNode->fqnn));
				return -1;
			}
		}

		/*	All contacts have been checked against the
		 *	proximate nodes list, so the list is ready
		 *	to use.						*/
	}

	routes = lyst_create_using(getIonMemoryMgr());
	if (routes == NULL)
	{
		putErrmsg("Can't create routes list.",
				utoa(terminusNode->fqnn));
		return -1;
	}

	for (elt2 = sm_list_first(ionwm, routingObj->proximateNodes); elt2;
			elt2 = sm_list_next(ionwm, elt2))
	{
		fqnn = (uvast) sm_list_data(ionwm, elt2);
		if (loadBestRoutesList(terminusNode, fqnn, bundle,
				excludedNodes, trace, routes, currentTime,
				deadline, routingObj) < 0)
		{
			putErrmsg("Can't find best route via node.",
					utoa(fqnn));
			return -1;
		}

		if (lyst_length(routes) > 0)
		{
			/*	Found best route through this proximate
			 *	node; add that route to the bestRoutes
			 *	list.					*/

			if (lyst_insert_last(bestRoutes,
					lyst_data(lyst_last(routes))) == NULL)
			{
				putErrmsg("Can't insert best route via node.",
						utoa(fqnn));
				return -1;
			}

			lyst_clear(routes);
		}
	}

	lyst_destroy(routes);
	return 0;
}

#endif

int	cgr_create_routing_object(IonNode *node)
{
	PsmPartition	ionwm = getIonwm();
	PsmAddress	routingObjectAddr;
	CgrRtgObject	*routingObject;

	CHKZERO(ionwm);
	routingObjectAddr = psm_zalloc(ionwm, sizeof(CgrRtgObject));
	if (routingObjectAddr == 0)
	{
		putErrmsg("Can't create CGR routing object.", NULL);
		return -1;
	}

	node->routingObject = routingObjectAddr;
	routingObject = (CgrRtgObject *) psp(ionwm, routingObjectAddr);
	memset((char *) routingObject, 0, sizeof(CgrRtgObject));
	routingObject->nodeAddr = psa(ionwm, node);
	return 0;
}

#if !UNIBO_CGR

int	cgr_start_SAP(uvast ownNodeNbr, time_t referenceTime, CgrSAP *sap)
{
	/* Parameters intentionally unused. */
	(void)ownNodeNbr;
	(void)referenceTime;

	*sap = NULL;
	return 0;
}

int	cgr_identify_best_routes(IonNode *terminusNode, Bundle *bundle,
			Lyst excludedNodes, time_t currentTime, CgrSAP sap,
			CgrTrace *trace, Lyst bestRoutes)
{
	PsmPartition	ionwm = getIonwm();
	CgrVdb		*cgrvdb = cgr_get_vdb();
	time_t		deadline;
	CgrRtgObject	*routingObj;
	int		potential;

	/* Parameter intentionally unused. */
	(void)sap;

	deadline = bundle->expirationTime;
	routingObj = (CgrRtgObject *) psp(ionwm, terminusNode->routingObject);
	CHKERR(routingObj);
	if (routingObj->selectedRoutes == 0)
	{
		/*	Must initialize routing object for CGR.		*/

		routingObj->selectedRoutes = sm_list_create_unlocked(ionwm);
		if (routingObj->selectedRoutes == 0)
		{
			putErrmsg("Can't initialize CGR routing.", NULL);
			return -1;
		}

		routingObj->knownRoutes = sm_list_create_unlocked(ionwm);
		if (routingObj->knownRoutes == 0)
		{
			putErrmsg("Can't initialize CGR routing.", NULL);
			return -1;
		}

		/*	#1054: tag both route lists with the destination
		 *	IonNode address so removeRoute can recover the routing
		 *	object and authoritatively unlink a route from both
		 *	lists by content.				*/

		oK(sm_list_user_data_set(ionwm, routingObj->selectedRoutes,
				routingObj->nodeAddr));
		oK(sm_list_user_data_set(ionwm, routingObj->knownRoutes,
				routingObj->nodeAddr));

		if (sm_list_insert_last(ionwm, cgrvdb->routingObjects,
				terminusNode->routingObject) == 0)
		{
			putErrmsg("Can't add routing object to cgrvdb.", NULL);
			return -1;
		}
	}

	TRACE(CgrIdentifyRoutes, deadline);
	if (bundle->ancillaryData.flags & BP_MINIMUM_LATENCY)
	{
		if (loadCriticalBestRoutesList(terminusNode, bundle,
				excludedNodes, trace, bestRoutes,
				currentTime, deadline, routingObj) < 0)
		{
			putErrmsg("Can't find all best routes to destination.",
					utoa(terminusNode->fqnn));
			return -1;
		}
	}
	else
	{
		/*	Negative-result cache (#1048): if a recent decision
		 *	found no usable route to this destination and the
		 *	contact plan hasn't changed since (a change wipes
		 *	this routing object), skip the costly re-enumeration
		 *	and let the bundle wait in limbo.  Return a non-zero
		 *	potential so the caller keeps it in limbo and retries
		 *	rather than abandoning it.			*/

		if (routingObj->computeDeferredUntil != 0
		&& currentTime < routingObj->computeDeferredUntil)
		{
			TRACE(CgrNoRoute);
			return 1;
		}

		if (loadBestRoutesList(terminusNode, 0, bundle,
				excludedNodes, trace, bestRoutes,
				currentTime, deadline, routingObj) < 0)
		{
			putErrmsg("Can't find best route to destination.",
					utoa(terminusNode->fqnn));
			return -1;
		}

		/*	Update the negative-result cache for next time.
		 *	Arm it only when this destination has no computed
		 *	routes at all, i.e., the route enumeration itself
		 *	came up empty; only then would the next bundle pay
		 *	a full re-enumeration.  When routes exist but none
		 *	suited this particular bundle (e.g., its deadline
		 *	precedes every route's arrival time), the next
		 *	bundle may well qualify, and evaluating it against
		 *	the cached route list is cheap -- arming the cache
		 *	in that case would deny routing to viable bundles
		 *	on the basis of an unrelated bundle's failure.	*/

		if (lyst_length(bestRoutes) == 0
		&& sm_list_length(ionwm, routingObj->selectedRoutes) == 0)
		{
			routingObj->computeDeferredUntil = currentTime
					+ CGR_NEG_CACHE_SECS;
		}
		else
		{
			routingObj->computeDeferredUntil = 0;
		}
	}

	potential = sm_list_length(ionwm, routingObj->selectedRoutes)
			+ sm_list_length(ionwm, routingObj->knownRoutes);
	return potential;
}

void	cgr_stop_SAP(CgrSAP sap)
{
	/* Parameter intentionally unused. */
	(void)sap;

	return;
}

static void	deleteObject(LystElt elt, void *userdata)
{
	void	*object = lyst_data(elt);

	/* Parameter intentionally unused. */
	(void)userdata;

	if (object)
	{
		MRELEASE(lyst_data(elt));
	}
}

#endif

#if UNIBO_CGR

int	cgr_preview_forward(uvast terminusNodeNbr, Bundle *bundle,
			time_t atTime, CgrSAP sap, CgrTrace *trace)
{
	return 0;
}

#else

int	cgr_preview_forward(uvast terminusNodeNbr, Bundle *bundle,
			time_t atTime, CgrSAP sap, CgrTrace *trace)
{
	IonVdb		*ionvdb = getIonVdb();
	IonNode		*terminusNode;
	PsmAddress	nextNode;
	int		ionMemIdx;
	Lyst		bestRoutes;
	Lyst		excludedNodes;
	int		result;

	terminusNode = findNode(ionvdb, terminusNodeNbr, &nextNode);
	if (terminusNode == NULL)
	{
		writeMemoNote("Terminus node unknown", itoa(terminusNodeNbr));
		return 0;
	}

	ionMemIdx = getIonMemoryMgr();
	bestRoutes = lyst_create_using(ionMemIdx);
	CHKERR(bestRoutes);
	excludedNodes = lyst_create_using(ionMemIdx);
	CHKERR(excludedNodes);
	lyst_delete_set(bestRoutes, deleteObject, NULL);
	if (terminusNode->routingObject == 0)
	{
		if (cgr_create_routing_object(terminusNode) < 0)
		{
			putErrmsg("Can't initialize routing object.", NULL);
			return -1;
		}
	}

	result = cgr_identify_best_routes(terminusNode, bundle,
			excludedNodes, atTime, sap, trace, bestRoutes);
	lyst_destroy(bestRoutes);
	lyst_destroy(excludedNodes);
	if (result < 0)
	{
		putErrmsg("Can't compute route.", NULL);
		return -1;
	}

	return 0;
}

#endif

int	cgr_prospect(uvast terminusNodeNbr, time_t deadline)
{
	PsmPartition	wm = getIonwm();
	IonVdb		*ionvdb = getIonVdb();
	time_t		currentTime = getCtime();
	IonNode		*terminusNode;
	PsmAddress	nextNode;
	PsmAddress	routingObjectAddress;
	CgrRtgObject	*routingObject;
	PsmAddress	elt;
	PsmAddress	nextElt;
	PsmAddress	addr;
	CgrRoute	*route;

	terminusNode = findNode(ionvdb, terminusNodeNbr, &nextNode);
	if (terminusNode == NULL)
	{
		return 0;		/*	Unknown node, no chance.*/
	}

	routingObjectAddress = terminusNode->routingObject;
	if (routingObjectAddress == 0)
	{
		return 0;		/*	No routes, no chance.	*/
	}

	routingObject = (CgrRtgObject *) psp(wm, routingObjectAddress);
	if (routingObject->selectedRoutes == 0)
	{
		return 0;		/*	No routes, no chance.	*/
	}

	for (elt = sm_list_first(wm, routingObject->selectedRoutes); elt;
			elt = nextElt)
	{
		nextElt = sm_list_next(wm, elt);
		addr = sm_list_data(wm, elt);
		route = (CgrRoute *) psp(wm, addr);
		if (route->toTime <= currentTime)
		{
			removeRoute(wm, elt);	/*	Expired.	*/
			continue;
		}

		if (route->arrivalTime > deadline)
		{
			continue;	/*	Not a plausible route.	*/
		}

		/*	Route may work if a hypothetical contact
		 *	materializes.					*/

		return 1;
	}

	return 0;
}

void	cgr_start(void)
{
	oK(cgr_get_vdb());
}

const char	*cgr_tracepoint_text(CgrTraceType traceType)
{
	int			i = traceType;
	static const char	*tracepointText[] =
	{
	[CgrBuildRoutes] = "\n\nBUILD terminusNode:" UVAST_FIELDSPEC
		" payloadLength:%u atTime:%u",
	[CgrInvalidTerminusNode] = "    INVALID terminus node",

	[CgrBeginRoute] = "  ROUTE",
	[CgrConsiderRoot] = "    ROOT fromFqnn:" UVAST_FIELDSPEC
		" toFqnn:" UVAST_FIELDSPEC,
	[CgrConsiderContact] = "      CONTACT fromFqnn:" UVAST_FIELDSPEC
		" toFqnn:" UVAST_FIELDSPEC,
	[CgrIgnoreContact] = "        IGNORE",

	[CgrCost] = "        COST transmitTime:%u owlt:%u arrivalTime:%u",
	[CgrHop] = "    HOP fromFqnn:" UVAST_FIELDSPEC " toFqnn:"
		UVAST_FIELDSPEC,

	[CgrProposeRoute] = "    PROPOSE firstHop to:" UVAST_FIELDSPEC
		" fromTime:%u arrivalTime:%u",

	[CgrIdentifyRoutes] = "IDENTIFY deadline:%u",
	[CgrFirstRoute] = "    FIRST",
	[CgrNoMoreRoutes] = "    NO MORE",
	[CgrCheckRoute] = "  CHECK firstHop to:" UVAST_FIELDSPEC
		" fromTime:%u arrivalTime:%u",
	[CgrExpiredRoute] = "    EXPIRED",
	[CgrExcludeRoute] = "    EXCLUDE",
	[CgrUncertainEntry] = "    UNCERTAIN",
	[CgrWrongViaNode] = "    IRRELEVANT",

	[CgrAddRoute] = "    ADD",
	[CgrUpdateRoute] = "    UPDATE",

	[CgrSelectRoutes] = "SELECTING",
	[CgrUseAllRoutes] = "  USE all best routes",
	[CgrConsiderRoute] = "  CONSIDER " UVAST_FIELDSPEC,
	[CgrSelectRoute] = "    SELECT",
	[CgrSkipRoute] = "    SKIP",
	[CgrUseRoute] = "  USE " UVAST_FIELDSPEC,
	[CgrNoRoute] = "  NO route",
	[CgrFullOverbooking] = "	Full OVERBOOKING (amount in bytes): %f",
	[CgrPartialOverbooking] = " Partial OVERBOOKING (amount in bytes): %f",
	};

	if (i < 0 || i >= CgrTraceTypeMax)
	{
		return "";
	}

	return tracepointText[i];
}

const char	*cgr_reason_text(CgrReason reason)
{
	int			i = reason;
	static const char	*reasonText[] =
	{
	[CgrContactEndsEarly] = "contact ends before data arrives",
	[CgrSuppressed] = "contact is suppressed",
	[CgrVisited] = "contact has been visited",
	[CgrNoRange] = "no range for contact",

	[CgrRouteViaSelf] = "route is via self",
	[CgrRouteVolumeTooSmall] = "route includes a contact that's too \
small for this bundle",
	[CgrInitialContactExcluded] = "first node on route is an excluded \
neighbor",
	[CgrRouteTooSlow] = "route is too slow; radiation latency delays \
arrival time too much",
	[CgrRouteCongested] = "route is congested, timely arrival impossible",
	[CgrNoPlan] = "no egress plan",
	[CgrBlockedPlan] = "egress plan is blocked",
	[CgrMaxPayloadTooSmall] = "max payload too small",
	[CgrNoResidualVolume] = "contact with this neighbor is already \
fully subscribed",
	[CgrResidualVolumeTooSmall] = "too little residual aggregate \
volume for this bundle",

	[CgrMoreHops] = "more hops",
	[CgrEarlierTermination] = "earlier route termination time",
	[CgrNoHelp] = "insufficient delivery confidence improvement",
	[CgrLowerVolume] = "lower path volume",
	[CgrLaterArrivalTime] = "later arrival time",
	[CgrLargerNodeNbr] = "initial hop has larger node number",
	};

	if (i < 0 || i >= CgrReasonMax)
	{
		return "";
	}

	return reasonText[i];
}

void	cgr_stop(void)
{
	PsmPartition	wm = getIonwm();
	char		*name = "cgrvdb";
	PsmAddress	vdbAddress;
	PsmAddress	elt;
	CgrVdb		*vdb;

	/*	Free volatile database					*/

	if (psm_locate(wm, name, &vdbAddress, &elt) < 0)
	{
		putErrmsg("Failed searching for vdb.", NULL);
		return;
	}

	if (elt)
	{
		vdb = (CgrVdb *) psp(wm, vdbAddress);
		cgr_clear_vdb(vdb);
		psm_free(wm, vdbAddress);
		if (psm_uncatlg(wm, name) < 0)
		{
			putErrmsg("Failed Uncataloging vdb.",NULL);
		}
	}
}
