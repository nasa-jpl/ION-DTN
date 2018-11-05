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

#define	MAX_TIME	((unsigned int) ((1U << 31) - 1))

#define	CGRVDB_NAME	"cgrvdb"

#ifdef	ION_BANDWIDTH_RESERVED
#define	MANAGE_OVERBOOKING	0
#endif

#ifndef	MANAGE_OVERBOOKING
#define	MANAGE_OVERBOOKING	1
#endif

/*		Perform a trace if a trace callback exists.		*/
#define TRACE(...) do \
{ \
	if (trace) \
	{ \
		trace->fn(trace->data, __LINE__, __VA_ARGS__); \
	} \
} while (0)

#define	PAYLOAD_CLASSES		3

/*		CGR-specific RFX data structures.			*/

typedef struct
{
	PsmAddress	rootOfSpur;	/*	Within *prior* route.	*/
	int		spursComputed;	/*	Boolean.		*/

	/*	Address of list element referencing this route, in
		either a knownRoutes list or a selectedRoutes list.	*/

	PsmAddress	referenceElt;

	/*	Contact that forms the initial hop of the route.	*/

	uvast		toNodeNbr;	/*	Initial-hop neighbor.	*/
	time_t		fromTime;	/*	As from time(2).	*/

	/*	Time at which route shuts down: earliest contact
	 *	end time among all contacts in the end-to-end path.	*/

	time_t		toTime;		/*	As from time(2).	*/

	/*	Details of the route.					*/

	float		arrivalConfidence;
	time_t		arrivalTime;	/*	As from time(2).	*/
	PsmAddress	hops;		/*	SM list: IonCXref addr.	*/

	/*	Transient values, valid only for the routing of the
	 *	current bundle.						*/

	Scalar		overbooked;	/*	Bytes needing reforward.*/
	Scalar		protected;	/*	Bytes not overbooked.	*/

	/*	NOTE: initial transmission on the "spur" portion of
	 *	this route is from the contact identified by rootOfSpur
	 *	to the contact identified by the first entry in the
	 *	hops list.  For a route that is not a branch off of
	 *	any other route, rootOfSpur is zero indicating that the
	 *	initial transmission on the "spur" portion of this
	 *	route (which is the entire route) is from the root
	 *	of the contact graph to the first contact in "hops".	*/
} CgrRoute;

typedef struct
{
	PsmAddress	nodeAddr;	/*	Back-reference.		*/
	PsmAddress	selectedRoutes;	/*	SmList of CgrRoute.	*/
	PsmAddress	knownRoutes;	/*	SmList of CgrRoute.	*/
	PsmAddress	proximateNodes;	/*	SmList of uvast node#s.	*/
} CgrRtgObject;	 	/*	IonNode routingObject is one of these.	*/

typedef struct
{
	/*	Working values, reset for each Dijkstra run.		*/

	IonCXref	*predecessor;	/*	On path to destination.	*/
	time_t		arrivalTime;	/*	As from time(2).	*/
	int		visited;	/*	Boolean.		*/
	int		suppressed;	/*	Boolean.		*/
} CgrContactNote;	/*	IonCXref routingObject is one of these.	*/

/*		Data structure for the CGR volatile database.		*/

typedef struct
{
	struct timeval	lastLoadTime;	/*	Add/del contacts/ranges	*/

	/*	There is one entry in the routingObjects list for each
	 *	remote destination node.
	 *
	 *	The content of each routingObjects list entry is a
	 *	CgrRtgObject structure containing the addresses of
	 *	two SmLists that are the lists of routes required for
	 *	Yen's algorithm: selectedRoutes is the A list and
	 *	knownRoutes is the B list.
	 *
	 *	The *list user data* of each of these SmList objects
	 *	is the address of the IonNode structure for the remote
	 *	destination node.
	 *
	 *	The *entries* of each of these SmList structures contain
	 *	the addresses of CgrRoute structures.			*/

	PsmAddress	routingObjects;	/*	SmList of CgrRtgObject.	*/
} CgrVdb;

/*	Functions for managing the CGR database.			*/

static void	removeRoute(PsmPartition ionwm, PsmAddress elt)
{
	PsmAddress	routeAddr;
	CgrRoute	*route;

	routeAddr = sm_list_data(ionwm, elt);
	route = (CgrRoute *) psp(ionwm, routeAddr);
	if (route->referenceElt)
	{
		sm_list_delete(ionwm, route->referenceElt, NULL, NULL);
	}

	if (elt != route->referenceElt)
	{
		sm_list_delete(ionwm, elt, NULL, NULL);
	}

	if (route->hops)
	{
		sm_list_destroy(ionwm, route->hops, NULL, NULL);
	}

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

	discardRouteList(ionwm, routingObject->selectedRoutes);
	discardRouteList(ionwm, routingObject->knownRoutes);

	/*	Destroy the list of proximate nodes.			*/

	if (routingObject->proximateNodes)
	{
		sm_list_destroy(ionwm, routingObject->proximateNodes, NULL,
				NULL);
	}
}

static void	destroyRoutingObjects(CgrVdb *vdb)
{
	PsmPartition	ionwm = getIonwm();
	PsmAddress	elt;
	PsmAddress	nextElt;
	PsmAddress	addr;
	CgrRtgObject	*routingObject;

	/*	Destroy all routing objects in the CGR vdb.		*/

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
}

static CgrVdb	*getCgrVdb()
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
	if ((vdb->routingObjects = sm_list_create(ionwm)) == 0
	|| psm_catlg(ionwm, name, vdbAddress) < 0)
	{
		sdr_exit_xn(sdr);
		putErrmsg("Can't initialize CGR volatile database.", name);
		return NULL;
	}
#if 0
	clearRoutingObjects(ionwm);
#endif
	sdr_exit_xn(sdr);
	return vdb;
}

/*	Functions for populating the route lists.			*/

static CgrRtgObject	*initializeRoutingObject(IonNode *terminusNode,
				time_t currentTime, CgrTrace *trace)
{
	PsmPartition	ionwm = getIonwm();
	IonVdb		*ionvdb = getIonVdb();
	CgrVdb		*cgrvdb = getCgrVdb();
	PsmAddress	routingObjectAddr;
	CgrRtgObject	*routingObject;

	CHKZERO(ionwm);
	CHKZERO(ionvdb);
	CHKZERO(cgrvdb);

	routingObjectAddr = psm_zalloc(ionwm, sizeof(CgrRtgObject));
	if (routingObjectAddr == 0)
	{
		putErrmsg("Can't create CGR routing object.", NULL);
		return 0;
	}

	terminusNode->routingObject = routingObjectAddr;
	routingObject = (CgrRtgObject *) psp(ionwm, routingObjectAddr);
	routingObject->nodeAddr = psa(ionwm, terminusNode);
	routingObject->selectedRoutes = sm_list_create(ionwm);
	routingObject->knownRoutes = sm_list_create(ionwm);
	if (routingObject->selectedRoutes == 0
	|| routingObject->knownRoutes == 0)
	{
		putErrmsg("Can't create CGR route lists.", NULL);
		return 0;
	}

	if (sm_list_insert_last(ionwm, cgrvdb->routingObjects,
			terminusNode->routingObject) == 0)
	{
		putErrmsg("Can't note CGR route list.", NULL);
		return 0;
	}

	return routingObject;
}

static int	getApplicableRange(IonCXref *contact, unsigned int *owlt)
{
	PsmPartition	ionwm = getIonwm();
	IonVdb		*ionvdb = getIonVdb();
	IonRXref	arg;
	PsmAddress	elt;
	IonRXref	*range;

	*owlt = 0;		/*	Default.			*/
	if (contact->discovered)
	{
		return 0;	/*	Physically adjacent nodes.	*/
	}

	/*	This is a scheduled contact; need to know the OWLT.	*/

	memset((char *) &arg, 0, sizeof(IonRXref));
	arg.fromNode = contact->fromNode;
	arg.toNode = contact->toNode;
	for (oK(sm_rbt_search(ionwm, ionvdb->rangeIndex, rfx_order_ranges,
			&arg, &elt)); elt; elt = sm_rbt_next(ionwm, elt))
	{
		range = (IonRXref *) psp(ionwm, sm_rbt_data(ionwm, elt));
		CHKERR(range);
		if (range->fromNode > arg.fromNode
		|| range->toNode > arg.toNode)
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
	IonCXref	*nextContact;
	time_t		earliestArrivalTime;
	time_t		earliestEndTime;
	PsmAddress	addr;

	/*	This is an implementation of Dijkstra's Algorithm.	*/

	TRACE(CgrBeginRoute);
	current = rootContact;
	currentWork = rootWork;
	memset((char *) &arg, 0, sizeof(IonCXref));

	/*	Perform this interior loop until either the best
	 *	route to the end vertex has been identified or else
	 *	it is known that there is no such route.		*/

	while (1)
	{
		/*	Consider all unvisited successors (i.e., next-
		 *	hop contacts) of the current contact, in each
		 *	case computing best-case arrival time for a
		 *	bundle transmitted during that contact.		*/

		arg.fromNode = current->toNode;
		TRACE(CgrConsiderRoot, current->fromNode, current->toNode);

		/*	First, compute and note/revise/discard the
		 *	best-case bundle arrival time for all contacts
		 *	that are topologically adjacent to the current
		 *	contact.					*/

		for (oK(sm_rbt_search(ionwm, ionvdb->contactIndex,
				rfx_order_contacts, &arg, &elt));
				elt; elt = sm_rbt_next(ionwm, elt))
		{
			contactAddr = sm_rbt_data(ionwm, elt);
			contact = (IonCXref *) psp(ionwm, contactAddr);
			if (contact->toTime <= currentTime)
			{
				/*	Contact is ended, is about to
				 *	be purged.			*/

				continue;
			}

			/*	Note: contact->fromNode can't be less
			 *	than current->toNode: we started at
			 *	that node with sm_rbt_search.		*/

			if (contact->fromNode > current->toNode)
			{
				/*	No more relevant contacts.	*/

				break;
			}

			TRACE(CgrConsiderContact, contact->fromNode,
					contact->toNode);
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
				 *	the current contact.		*/

				work->arrivalTime = arrivalTime;
				work->predecessor = current;
			}
		}

		/*	Have at this point computed the best-case
		 *	bundle arrival times for all edges of the
		 *	graph that originate at the current contact.	*/

		currentWork->visited = 1;

		/*	Now the second loop: among ALL non-suppressed
		 *	contacts in the graph, select the one with
		 *	the earliest arrival time (least distance
		 *	from the root vertex) as the new "current"
		 *	vertex to analyze.				*/

		nextContact = NULL;
		earliestArrivalTime = MAX_TIME;
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

			if (work->arrivalTime >= earliestArrivalTime)
			{
				/*	Not the lowest-cost edge.	*/

				continue;
			}

			/*	Best successor contact seen so far.	*/

			nextContact = contact;
			earliestArrivalTime = work->arrivalTime;
		}

		/*	If search is complete, stop.  Else repeat,
		 *	with new value of "current".			*/

		if (nextContact == NULL)
		{
			/*	End of search; can't proceed any
			 *	further toward the terminal contact.	*/

			break;
		}

		current = nextContact;
		currentWork = (CgrContactNote *)
				psp(ionwm, current->routingObject);
		if (current->toNode == terminusNode->nodeNbr)
		{
			earliestFinalArrivalTime = currentWork->arrivalTime;
			finalContact = current;
			break;
		}
	}

	/*	Have finished a single Dijkstra search of the contact
	 *	graph, excluding those contacts that were suppressed.	*/

	if (finalContact)	/*	Got route to terminal contact.	*/
	{
		route->arrivalTime = earliestFinalArrivalTime;
		route->arrivalConfidence = 1.0;

		/*	Load the entire route into the "hops" list,
		 *	backtracking to root, and compute the time
		 *	at which the route will become unusable.	*/

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
			TRACE(CgrHop, contact->fromNode, contact->toNode);
			if (sm_list_insert_first(ionwm, route->hops, addr) == 0)
			{
				putErrmsg("Can't insert contact into route.",
						NULL);
				return -1;
			}

			contact = work->predecessor;
			if (contact == rootContact)
			{
				break;
			}
		}

		/*	Now use the first contact in the route to
		 *	characterize the route.				*/

		addr = sm_list_data(ionwm, sm_list_first(ionwm, route->hops));
		contact = (IonCXref *) psp(ionwm, addr);
		route->toNodeNbr = contact->toNode;
		route->fromTime = contact->fromTime;
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
//puts("*** Starting at a waypoint of the last selected route. ***");
		rootContact = (IonCXref *) psp(ionwm, sm_list_data(ionwm,
				rootContactElt));
		if (rootContact->toNode == terminusNode->nodeNbr)
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
		graphRoot.fromNode = graphRoot.toNode = getOwnNodeNbr();
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
	route->hops = sm_list_create(ionwm);
	if (route->hops == 0)
	{
		psm_free(ionwm, addr);
		putErrmsg("Can't create CGR route hops list.", NULL);
		return -1;
	}

	route->rootOfSpur = rootOfSpur;

	/*	Run Dijkstra search.					*/

	if (computeDistanceToTerminus(rootContact, rootWork, terminusNode,
			currentTime, excludedEdges, route, trace) < 0)
	{
		putErrmsg("Can't finish Dijstra search.", NULL);
		return -1;
	}

	if (route->toNodeNbr == 0)
	{
		TRACE(CgrNoMoreRoutes);

		/*	No more routes in graph.			*/

		sm_list_destroy(ionwm, route->hops, NULL, NULL);
		psm_free(ionwm, addr);
		*routeAddr = 0;
	}
	else
	{
		TRACE(CgrProposeRoute, route->toNodeNbr,
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
	CgrVdb		*cgrvdb	= getCgrVdb();
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
	PsmAddress	rootOfSpurAddr;
	PsmAddress	excludedEdges;
	PsmAddress	contactElt;
	PsmAddress	nextContactElt;
	PsmAddress	contactAddr;
	IonCXref	*contact;
	CgrContactNote	*work;
	PsmAddress	routeElt;
	PsmAddress	routeAddr;
	CgrRoute	*route;
	PsmAddress	rootPathContactElt;
	PsmAddress	nextRootPathContactElt;
	PsmAddress	rootPathContactAddr;
	int		result;
	PsmAddress	newRouteAddr;
	CgrRoute	*newRoute;

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

	excludedEdges = sm_list_create(ionwm);
	CHKERR(excludedEdges);

	/*	Suppress contacts that would introduce loops, i.e.,
	 *	all contacts on the root path for this spur path.	*/

	if (rootOfSpur != 0)
	{
//puts("*** Suppressing contacts on root path. ***");
		contactElt = sm_list_prev(ionwm, rootOfSpur);
		while (contactElt)
		{
			contactAddr = sm_list_data(ionwm, contactElt);
			contact = (IonCXref *) psp(ionwm, contactAddr);
			CHKERR(work = getWorkArea(ionwm, contact));
			work->suppressed = 1;
//printf("*** Suppressing contact to node " UVAST_FIELDSPEC " on root path. ***\n", contact->toNode);
			contactElt = sm_list_prev(ionwm, contactElt);
		}
	}

	/*	Exclude edges that would introduce duplicates: for
	 *	each existing route that has this same root path,
	 *	exclude the edge from the end of the root path to
	 *	the first subsequent contact.				*/

//printf("*** rootOfSpurAddr is " UVAST_FIELDSPEC ". ***\n", rootOfSpurAddr);
	for (routeElt = sm_list_first(ionwm, routingObj->selectedRoutes);
			routeElt; routeElt = sm_list_next(ionwm, routeElt))
	{
//puts("*** Looking for edges to exclude on a selected route. ***");
		routeAddr = sm_list_data(ionwm, routeElt);
		route = (CgrRoute *) psp(ionwm, routeAddr);
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
//CHKERR(work = getWorkArea(ionwm, contact));
//printf("*** Suppressing contact to node " UVAST_FIELDSPEC " after end of root path. ***\n", contact->toNode);
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
			nextContactElt = sm_list_next(ionwm, contactElt);
			rootPathContactElt = nextRootPathContactElt;
			rootPathContactAddr = sm_list_data(ionwm,
					rootPathContactElt);
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

	if (newRouteAddr == 0)	/*	No route found.		*/
	{
//puts("*** No newly computed route reported. ***");
		return 0;
	}

	newRoute = (CgrRoute *) psp(ionwm, newRouteAddr);
	newRoute->rootOfSpur = rootOfSpur;

	/*	Prepend common trunk route to the spur route.	*/

	contactElt = rootOfSpur;
	while (contactElt)
	{
//puts("*** Prepending a contact from trunk to spur route. ***");
		contactAddr = sm_list_data(ionwm, contactElt);
		contact = (IonCXref *) psp(ionwm, contactAddr);
		TRACE(CgrHop, contact->fromNode, contact->toNode);
		if (sm_list_insert_first(ionwm, newRoute->hops, contactAddr)
				== 0)
		{
			putErrmsg("Can't prepend trunk to spur route.", NULL);
			return -1;
		}

		contactElt = sm_list_prev(ionwm, contactElt);
	}

	/*	Append new route into list of known routes.	*/

//puts("*** Appending newly computed route to list B. ***");
	newRoute->referenceElt = sm_list_insert_last(ionwm,
			routingObj->knownRoutes, newRouteAddr);
	if (newRoute->referenceElt == 0)
	{
		putErrmsg("Can't append known route.", NULL);
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
	PsmAddress	knownRouteAddr;
	CgrRoute	*knownRoute;
	int		knownRouteHopCount;
	PsmAddress	bestKnownRouteElt;
	PsmAddress	bestKnownRouteAddr;
	CgrRoute	*bestKnownRoute;
	int		bestKnownRouteHopCount;

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
			elt2 = sm_list_next(ionwm, elt2))
	{
		/*	Determine whether or not this route is the
		 *	best route in the B list. Preference is by
		 *	ascending arrival time, ascending hop count,
		 *	descending termination time, ascending entry
		 *	node number.					*/

//puts("*** Considering a B-list node for migration to A-list. ***");
		knownRouteAddr = sm_list_data(ionwm, elt2);
		knownRoute = (CgrRoute *) psp(ionwm, knownRouteAddr);
		knownRouteHopCount = sm_list_length(ionwm, knownRoute->hops);
		if (bestKnownRouteElt == 0)
		{
			bestKnownRouteElt = elt2;
			bestKnownRouteAddr = knownRouteAddr;
			bestKnownRoute = knownRoute;
			bestKnownRouteHopCount = knownRouteHopCount;
			continue;
		}

		/*	Earlier arrival time?				*/

		if (knownRoute->arrivalTime > bestKnownRoute->arrivalTime)
		{
			continue;	/*	Not better.		*/
		}

		if (knownRoute->arrivalTime < bestKnownRoute->arrivalTime)
		{
			bestKnownRouteElt = elt2;
			bestKnownRouteAddr = knownRouteAddr;
			bestKnownRoute = knownRoute;
			bestKnownRouteHopCount = knownRouteHopCount;
			continue;
		}

		/*	Fewer hops?					*/

		if (knownRouteHopCount > bestKnownRouteHopCount)
		{
			continue;	/*	Not better.		*/
		}

		if (knownRouteHopCount < bestKnownRouteHopCount)
		{
			bestKnownRouteElt = elt2;
			bestKnownRouteAddr = knownRouteAddr;
			bestKnownRoute = knownRoute;
			bestKnownRouteHopCount = knownRouteHopCount;
			continue;
		}

		/*	Termination time?				*/

		if (knownRoute->toTime < bestKnownRoute->toTime)
		{
			continue;	/*	Not better.		*/
		}

		if (knownRoute->toTime > bestKnownRoute->toTime)
		{
			bestKnownRouteElt = elt2;
			bestKnownRouteAddr = knownRouteAddr;
			bestKnownRoute = knownRoute;
			bestKnownRouteHopCount = knownRouteHopCount;
			continue;
		}

		/*	Entry node number?				*/

		if (knownRoute->toNodeNbr > bestKnownRoute->toNodeNbr)
		{
			continue;	/*	Not better.		*/
		}

		bestKnownRouteElt = elt2;
		bestKnownRouteAddr = knownRouteAddr;
		bestKnownRoute = knownRoute;
		bestKnownRouteHopCount = knownRouteHopCount;
	}

	if (bestKnownRouteElt)
	{
//puts("*** Migrating best route in list B into list A. ***");
		*elt = sm_list_insert_last(ionwm, routingObj->selectedRoutes,
				bestKnownRouteAddr);
		if (*elt == 0)
		{
			putErrmsg("Can't insert selected route.", NULL);
			return -1;
		}

		sm_list_delete(ionwm, bestKnownRouteElt, NULL, NULL);
		bestKnownRoute->referenceElt = *elt;
	}
//else puts("*** No routes in list B to migrate into list A. ***");

	return 0;
}

/*	Functions for selecting which node(s) to forward a bundle to.	*/

static int	isExcluded(uvast nodeNbr, Lyst excludedNodes)
{
	LystElt	elt;
	NodeId	*node;

	for (elt = lyst_first(excludedNodes); elt; elt = lyst_next(elt))
	{
		node = (NodeId *) lyst_data(elt);
		if (node->nbr == nodeNbr)
		{
			return 1;	/*	Node is in the list.	*/
		}
	}

	return 0;
}

static time_t	computeBundleArrivalTime(CgrRoute *route, Bundle *bundle,
			time_t currentTime, BpPlan *plan, Scalar *overbooked,
			Scalar *protected, time_t *eto)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();
	IonVdb		*vdb = getIonVdb();
	uvast		ownNodeNbr = getOwnNodeNbr();
	Scalar		priorClaims;
	Scalar		totalBacklog;
	IonCXref	arg;
	PsmAddress	elt;
	IonCXref	*contact;
	Scalar		volume;
	Scalar		allotment;
	int		eccc;	/*	Estimated volume consumption.	*/
	time_t		startTime;
	time_t		endTime;
	int		secRemaining;
	time_t		firstByteTransmitTime;
	time_t		lastByteTransmitTime;
	Scalar		radiationLatency;
	unsigned int	owlt;
	unsigned int	owltMargin;
	time_t		arrivalTime;
	Object		contactObj;
	IonContact	contactBuf;
	int		priority;
	time_t		effectiveStartTime;
	time_t		effectiveStopTime;
	PsmAddress	elt2;

	computePriorClaims(plan, bundle, &priorClaims, &totalBacklog);
	copyScalar(protected, &totalBacklog);

	/*	Reduce prior claims on the first contact in this
	 *	route by all transmission to this contact's neighbor
	 *	that will be performed during contacts that precede
	 *	this contact.  That is, the following loop examines
	 *	only contacts that occur *before* the start of this
	 *	particular route.					*/

	loadScalar(&allotment, 0);
	loadScalar(&volume, 0);
	memset((char *) &arg, 0, sizeof(IonCXref));
	arg.fromNode = ownNodeNbr;
	arg.toNode = route->toNodeNbr;
	for (oK(sm_rbt_search(ionwm, vdb->contactIndex, rfx_order_contacts,
			&arg, &elt)); elt; elt = sm_rbt_next(ionwm, elt))
	{
		contact = (IonCXref *) psp(ionwm, sm_rbt_data(ionwm, elt));
		if (contact->fromNode > ownNodeNbr
		|| contact->toNode > route->toNodeNbr
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
		subtractFromScalar(&allotment, protected);
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

			copyScalar(&allotment, protected);
		}

		/*	Determine how much of the total backlog has
		 *	been allotted to subsequent contacts.		*/

		subtractFromScalar(protected, &volume);
		if (!scalarIsValid(protected))
		{
			/*	No bundles scheduled for transmission
			 *	during any subsequent contacts.		*/

			loadScalar(protected, 0);
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

	eccc = computeECCC(guessBundleSize(bundle));
	copyScalar(overbooked, &allotment);
	increaseScalar(overbooked, eccc);
	subtractFromScalar(overbooked, &volume);
	if (!scalarIsValid(overbooked))
	{
		loadScalar(overbooked, 0);
	}

	/*	Now consider the initial contact on the route.		*/

	elt = sm_list_first(ionwm, route->hops);
	contact = (IonCXref *) psp(ionwm, sm_list_data(ionwm, elt));
	CHKERR(contact->xmitRate > 0);

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
	*eto = firstByteTransmitTime;

	/*	Add time to transmit everything preceding last byte.	*/

	copyScalar(&radiationLatency, &priorClaims);
	increaseScalar(&radiationLatency, eccc);
	divideScalar(&radiationLatency, contact->xmitRate);
	lastByteTransmitTime += ((ONE_GIG * radiationLatency.gigs)
			+ radiationLatency.units);

	/*	Now compute expected final arrival time by adding
	 *	OWLTs, inter-contact delays, and per-hop radiation
	 *	latencies along the path to the terminus node.  In
	 *	so doing, ensure that EVL is not fully depleted at
	 *	any contact in the path.				*/

	while (1)
	{
		if (firstByteTransmitTime >= contact->toTime)
		{
			/*	Due to the volume of transmission
			 *	that must precede it, this bundle
			 *	can't be transmitted during this
			 *	contact.  So the route is unusable.
			 *
			 *	NOTE that the SABR concept of
			 *	anticipatory fragmentation is NOT
			 *	implemented in ION at this time.
			 *
			 *	Note that transmit time is computed
			 *	using integer arithmetic, which will
			 *	truncate any fractional seconds of
			 *	total transmission time.  To account
			 *	for this rounding error, we require
			 *	that the computed first byte transmit
			 *	time be less than the contact end
			 *	time, rather than merely not greater.	*/

			return 0;
		}

		if (getApplicableRange(contact, &owlt) < 0)
		{
			/*	Can't determine owlt for this contact,
			 *	so arrival time can't be computed.
			 *	Route is not usable.			*/

			return 0;
		}

		owltMargin = ((MAX_SPEED_MPH / 3600) * owlt) / 186282;
		arrivalTime = lastByteTransmitTime + owlt + owltMargin;

		/*	Ensure that the contact is not depleted.	*/

		priority = COS_FLAGS(bundle->bundleProcFlags) & 0x03;
		contactObj = sdr_list_data(sdr, contact->contactElt);
		sdr_read(sdr, (char *) &contactBuf, contactObj,
				sizeof(IonContact));
		if (contactBuf.mtv[priority] < 0)
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

		if (effectiveStopTime < effectiveStartTime)
		{
			return 0;	/*	Conditional depletion.	*/
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
		if (arrivalTime > contact->fromTime)
		{
			firstByteTransmitTime = arrivalTime;
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
		loadScalar(&radiationLatency, eccc);
		divideScalar(&radiationLatency, contact->xmitRate);
		lastByteTransmitTime += ((ONE_GIG * radiationLatency.gigs)
				+ radiationLatency.units);
	}

	if (arrivalTime > (bundle->expirationTime + EPOCH_2000_SEC))
	{
		/*	Bundle will never arrive: it will expire
		 *	before arrival.					*/

		arrivalTime = 0;
	}

	return arrivalTime;
}

static float	getNewDlvConfidence(Bundle *bundle, CgrRoute *route)
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

static int	tryRoute(CgrRoute *route, time_t currentTime, Bundle *bundle,
			CgrTrace *trace, Lyst bestRoutes)
{
	Sdr		sdr = getIonsdr();
	char		eid[SDRSTRING_BUFSZ];
	VPlan		*vplan;
	PsmAddress	vplanElt;
	Object		planObj;
	BpPlan		plan;
	time_t		arrivalTime;
	Scalar		overbooked;
	Scalar		protected;
	time_t		eto;

	isprintf(eid, sizeof eid, "ipn:" UVAST_FIELDSPEC ".0",
			route->toNodeNbr);
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

	arrivalTime = computeBundleArrivalTime(route, bundle, currentTime,
			&plan, &overbooked, &protected, &eto);
	if (arrivalTime == 0)	/*	Can't be delivered in time.	*/
	{
		TRACE(CgrExcludeRoute, CgrRouteCongested);
		return 0;		/*	Connections too tight.	*/
	}

	/*	This route is a plausible opportunity for getting
	 *	the bundle forwarded to the terminus node before it
	 *	expires, so we add the route to the list of best
	 *	routes for this bundle.					*/

	copyScalar(&route->overbooked, &overbooked);
	copyScalar(&route->protected, &protected);
	if (lyst_insert_last(bestRoutes, (void *) route) == 0)
	{
		putErrmsg("Can't add route.", NULL);
		return -1;
	}

	TRACE(CgrAddRoute);
	return 0;
}

static int	checkRoute(IonNode *terminusNode, uvast viaNodeNbr,
			PsmAddress *elt, Bundle *bundle, Object bundleObj,
			Lyst excludedNodes, CgrTrace *trace, Lyst bestRoutes,
			time_t currentTime, time_t deadline)
{
	PsmPartition	ionwm = getIonwm();
	PsmAddress	nextElt;
	CgrRtgObject	*routingObj;
	PsmAddress	elt2;
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
		nextElt = sm_list_next(ionwm, *elt);
	}
	else		/*	At end of selectedRoutes list.		*/
	{
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
			addr = sm_list_data(ionwm, elt2);
			route = (CgrRoute *) psp(ionwm, addr);
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

	addr = sm_list_data(ionwm, *elt);
	route = (CgrRoute *) psp(ionwm, addr);
	TRACE(CgrCheckRoute, route->toNodeNbr, (unsigned int)(route->fromTime),
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
		if (route->toNodeNbr != viaNodeNbr)
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
				getNewDlvConfidence(bundle, route);
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

	if (route->toNodeNbr == getOwnNodeNbr())
	{
		if (!(bundle->destination.cbhe
		&& bundle->destination.c.nodeNbr == route->toNodeNbr))
		{
			/*	Never route via self -- a loop.		*/

			TRACE(CgrExcludeRoute, CgrRouteViaSelf);
			*elt = nextElt;
			return 1;
		}

		/*	Self is final destination.			*/
	}

	/*	Is the neighbor that receives bundles during this
	 *	route's initial contact excluded for any reason?	*/

	if (isExcluded(route->toNodeNbr, excludedNodes))
	{
		TRACE(CgrExcludeRoute, CgrInitialContactExcluded);
		*elt = nextElt;
		return 1;
	}

	/*	Route might work.  If this route is supported by
	 *	contacts with enough aggregate volume to convey
	 *	this bundle and all currently queued bundles of
	 *	equal or higher priority, then the neighbor is
	 *	a candidate proximate node for forwarding the
	 *	bundle to the terminus node.				*/

	if (tryRoute(route, currentTime, bundle, trace, bestRoutes) < 0)
	{
		putErrmsg("Failed trying to consider route.", NULL);
		return -1;
	}

	*elt = nextElt;
	return 1;
}

static int	loadBestRoutesList(IonNode *terminusNode, uvast viaNodeNbr,
			Bundle *bundle, Object bundleObj, Lyst excludedNodes,
		       	CgrTrace *trace, Lyst bestRoutes, time_t currentTime,
			time_t deadline, CgrRtgObject *routingObj)
{
	PsmPartition	ionwm = getIonwm();
	PsmAddress	elt;

	/*	Perform route selection outer loop until the list
	 *	of best routes is non-empty, i.e., at least one
	 *	route from the local node to the terminus node
	 *	has been identified.					*/

	elt = sm_list_first(ionwm, routingObj->selectedRoutes);
	while (lyst_length(bestRoutes) == 0)
	{
		switch (checkRoute(terminusNode, viaNodeNbr, &elt, bundle,
				bundleObj, excludedNodes, trace, bestRoutes,
				currentTime, deadline))
		{
		case -1:	/*	System failure.			*/
			putErrmsg("Failed checking route to node.",
					utoa(terminusNode->nodeNbr));
			return -1;

		case 0:		/*	No more routes to check.	*/
			return 0;

		default:	/*	Can continue checking.		*/
			break;	/*	Out of switch; try again.	*/
		}
	}

	return 0;
}

static int	loadCriticalBestRoutesList(IonNode *terminusNode,
			Bundle *bundle, Object bundleObj, Lyst excludedNodes,
		       	CgrTrace *trace, Lyst bestRoutes, time_t currentTime,
			time_t deadline, CgrRtgObject *routingObj)
{
	PsmPartition	ionwm = getIonwm();
	uvast		ownNodeNbr = getOwnNodeNbr();
	IonVdb		*ionvdb = getIonVdb();
	PsmAddress	elt;
	IonCXref	*contact;
	PsmAddress	elt2;
	uvast		nodeNbr;
	Lyst		routes;

	if (routingObj->proximateNodes == 0)
	{
		routingObj->proximateNodes = sm_list_create(ionwm);
		if (routingObj->proximateNodes == 0)
		{
			putErrmsg("Can't build list of proximate nodes.",
					utoa(terminusNode->nodeNbr));
			return -1;
		}

		/*	Identify all proximate nodes by scanning all
		 *	contacts that transmit from the local node.	*/

		for (elt = sm_rbt_first(ionwm, ionvdb->contactIndex); elt;
				elt = sm_rbt_next(ionwm, elt))
		{
			contact = (IonCXref *) psp(ionwm, sm_rbt_data(ionwm,
						elt));
			if (contact->fromNode != ownNodeNbr)
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
				nodeNbr = (uvast) sm_list_data(ionwm, elt2);
				if (nodeNbr < contact->toNode)
				{
					continue;
				}

				break;
			}

			if (elt2 == 0)	/*	No greater prox node#.	*/
			{
				if (sm_list_insert_last(ionwm,
					routingObj->proximateNodes,
					(PsmAddress) (contact->toNode)) == 0)
				{
					putErrmsg("Can't insert prox node.",
						utoa(terminusNode->nodeNbr));
					return -1;
				}

				continue;	/*	Next contact.	*/
			}

			if (nodeNbr == contact->toNode)
			{
				/*	Prox node is already in list.	*/

				continue;	/*	Next contact.	*/
			}

			/*	This node number must be inserted here.	*/

			if (sm_list_insert_before(ionwm, elt2,
					(PsmAddress) (contact->toNode)) == 0)
			{
				putErrmsg("Can't insert prox node.",
						utoa(terminusNode->nodeNbr));
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
				utoa(terminusNode->nodeNbr));
		return -1;
	}

	for (elt2 = sm_list_first(ionwm, routingObj->proximateNodes); elt2;
			elt2 = sm_list_next(ionwm, elt2))
	{
		nodeNbr = (uvast) sm_list_data(ionwm, elt2);
		if (loadBestRoutesList(terminusNode, nodeNbr, bundle, bundleObj,
				excludedNodes, trace, routes, currentTime,
				deadline, routingObj) < 0)
		{
			putErrmsg("Can't find best route via node.",
					utoa(nodeNbr));
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
						utoa(nodeNbr));
				return -1;
			}

			lyst_clear(routes);
		}
	}

	lyst_destroy(routes);
	return 0;
}

static int	identifyBestRoutes(IonNode *terminusNode, Bundle *bundle,
			Object bundleObj, Lyst excludedNodes, CgrTrace *trace,
			Lyst bestRoutes, time_t currentTime)
{
	PsmPartition	ionwm = getIonwm();
	time_t		deadline;
	CgrRtgObject	*routingObj;

	deadline = bundle->expirationTime + EPOCH_2000_SEC;
	routingObj = (CgrRtgObject *) psp(ionwm, terminusNode->routingObject);
	if (routingObj == 0)	/*	No current routes to this node.	*/
	{
		if ((routingObj = initializeRoutingObject(terminusNode,
				currentTime, trace)) == 0)
		{
			putErrmsg("Can't initialize routing object for node.",
					utoa(terminusNode->nodeNbr));
			return -1;
		}
	}

	TRACE(CgrIdentifyRoutes, deadline);
	if (bundle->ancillaryData.flags & BP_MINIMUM_LATENCY)
	{
		if (loadCriticalBestRoutesList(terminusNode, bundle, bundleObj,
				excludedNodes, trace, bestRoutes, currentTime,
				deadline, routingObj) < 0)
		{
			putErrmsg("Can't find all best routes to destination.",
					utoa(terminusNode->nodeNbr));
			return -1;
		}
	}
	else
	{
		if (loadBestRoutesList(terminusNode, 0, bundle, bundleObj,
				excludedNodes, trace, bestRoutes, currentTime,
				deadline, routingObj) < 0)
		{
			putErrmsg("Can't find best route to destination.",
					utoa(terminusNode->nodeNbr));
			return -1;
		}
	}

	return 0;
}

/*	Functions for forwarding bundle to selected neighbor.		*/

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

static int	enqueueToNeighbor(CgrRoute *route, Bundle *bundle,
			Object bundleObj, IonNode *terminusNode)
{
	Sdr		sdr = getIonsdr();
	unsigned int	serviceNbr;
	char		terminusEid[64];
	PsmPartition	ionwm;
	PsmAddress	embElt;
	Embargo		*embargo;
	BpEvent		event;
	char		neighborEid[MAX_EID_LEN + 1];
	VPlan		*vplan;
	PsmAddress	vplanElt;
	int		priority;
	int		eccc;
	PsmAddress	elt;
	PsmAddress	addr;
	IonCXref	*contact;
	Object		contactObj;
	IonContact	contactBuf;
	int		i;

	/*	Note that a copy is being sent on the route through
	 *	this neighbor.						*/

	if (bundle->xmitCopiesCount == MAX_XMIT_COPIES)
	{
		return 0;	/*	Reached forwarding limit.	*/
	}

	bundle->xmitCopies[bundle->xmitCopiesCount] = route->toNodeNbr;
	bundle->xmitCopiesCount++;
	bundle->dlvConfidence = getNewDlvConfidence(bundle, route);
	if (route->toNodeNbr == bundle->destination.c.nodeNbr)
	{
		serviceNbr = bundle->destination.c.serviceNbr;
	}
	else
	{
		serviceNbr = 0;
	}

	isprintf(terminusEid, sizeof terminusEid, "ipn:" UVAST_FIELDSPEC ".%u",
			route->toNodeNbr, serviceNbr);

	/*	If this neighbor is a currently embargoed neighbor
	 *	for this final destination (i.e., one that has been
	 *	refusing bundles destined for this final destination
	 *	node), then this bundle serves as a "probe" aimed at
	 *	that neighbor.  In that case, must now enable the
	 *	scheduling of the next probe to this neighbor.		*/

	ionwm = getIonwm();
	for (embElt = sm_list_first(ionwm, terminusNode->embargoes);
			embElt; embElt = sm_list_next(ionwm, embElt))
	{
		embargo = (Embargo *) psp(ionwm, sm_list_data(ionwm, embElt));
		if (embargo->nodeNbr < route->toNodeNbr)
		{
			continue;
		}

		if (embargo->nodeNbr > route->toNodeNbr)
		{
			break;
		}

		/*	This neighbor has been refusing bundles
		 *	destined for this final destination node,
		 *	but since it is now due for a probe bundle
		 *	(else it would have been on the excludedNodes
		 *	list and therefore would never have made it
		 *	to the list of bestRoutes), we are
		 *	sending this one to it.  So we must turn
		 *	off the flag indicating that a probe to this
		 *	node is due -- we're sending one now.		*/

		embargo->probeIsDue = 0;
		break;
	}

	/*	If the bundle is NOT critical, then we need to post
	 *	an xmitOverdue timeout event to trigger re-forwarding
	 *	in case the bundle doesn't get transmitted during the
	 *	contact in which we expect it to be transmitted.	*/

	if (!(bundle->ancillaryData.flags & BP_MINIMUM_LATENCY))
	{
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

	priority = COS_FLAGS(bundle->bundleProcFlags) & 0x03;
	eccc = computeECCC(guessBundleSize(bundle));
	CHKERR(sdr_begin_xn(sdr));
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
			contactBuf.mtv[i] -= eccc;
		}

		sdr_write(sdr, contactObj, (char *) &contactBuf,
				sizeof(IonContact));
	}

	return sdr_end_xn(sdr);
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
	priority = COS_FLAGS(newBundle->bundleProcFlags) & 0x03;
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

	protected += (ONE_GIG * route->protected.gigs)
			+ route->protected.units;
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
		ordinal = newBundle->ancillaryData.ordinal;
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
	LystElt		elt;
	LystElt		nextElt;
	CgrRoute	*route;
	CgrRtgObject	*routingObject;
	Bundle		newBundle;
	Object		newBundleObj;

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

		if (enqueueToNeighbor(route, bundle, bundleObj, terminusNode))
		{
			putErrmsg("Can't queue for neighbor.", NULL);
			lyst_destroy(bestRoutes);
			return -1;
		}
	}

	lyst_destroy(bestRoutes);
	if (bundle->dlvConfidence >= MIN_NET_DELIVERY_CONFIDENCE
	|| bundle->id.source.c.nodeNbr == bundle->destination.c.nodeNbr)
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

static int 	cgrForward(Bundle *bundle, Object bundleObj,
			uvast terminusNodeNbr, time_t atTime, CgrTrace *trace,
			int preview)
{
	PsmPartition	ionwm = getIonwm();
	IonVdb		*ionvdb = getIonVdb();
	CgrVdb		*cgrvdb = getCgrVdb();
	IonNode		*terminusNode;
	PsmAddress	nextNode;
	int		ionMemIdx;
	Lyst		bestRoutes;
	Lyst		excludedNodes;
	PsmAddress	embElt;
	Embargo		*embargo;
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

	CHKERR(bundle && bundleObj && terminusNodeNbr);

	TRACE(CgrBuildRoutes, terminusNodeNbr, bundle->payload.length,
			(unsigned int)(atTime));

	if (ionvdb->lastEditTime.tv_sec > cgrvdb->lastLoadTime.tv_sec
	|| (ionvdb->lastEditTime.tv_sec == cgrvdb->lastLoadTime.tv_sec
	    && ionvdb->lastEditTime.tv_usec > cgrvdb->lastLoadTime.tv_usec)) 
	{
		/*	Contact plan has been modified, so must discard
		 *	all route lists and reconstruct them as needed.	*/

		destroyRoutingObjects(cgrvdb);
		getCurrentTime(&(cgrvdb->lastLoadTime));
	}

	terminusNode = findNode(ionvdb, terminusNodeNbr, &nextNode);
	if (terminusNode == NULL)
	{
		TRACE(CgrInvalidTerminusNode);

		return 0;	/*	Can't apply CGR.		*/
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

	/*	Insert into the excludedNodes list all neighbors that
	 *	have been refusing custody of bundles destined for the
	 *	destination node.					*/

	for (embElt = sm_list_first(ionwm, terminusNode->embargoes);
			embElt; embElt = sm_list_next(ionwm, embElt))
	{
		embargo = (Embargo *) psp(ionwm, sm_list_data(ionwm, embElt));
		if (!(embargo->probeIsDue))
		{
			/*	(Omit the embargoed node from the list
			 *	of excluded nodes if it's now time to
			 *	probe that node for renewed acceptance
			 *	of bundles destined for this destination
			 *	node.)					*/

			if (excludeNode(excludedNodes, embargo->nodeNbr))
			{
				putErrmsg("Can't note embargo.", NULL);
				lyst_destroy(excludedNodes);
				lyst_destroy(bestRoutes);
				return -1;
			}
		}
	}

	/*	Consult the contact graph to identify the neighboring
	 *	node(s) to forward the bundle to.			*/

	if (identifyBestRoutes(terminusNode, bundle, bundleObj,
			excludedNodes, trace, bestRoutes, atTime) < 0)
	{
		putErrmsg("Can't identify best route(s) for bundle.", NULL);
		lyst_destroy(excludedNodes);
		lyst_destroy(bestRoutes);
		return -1;
	}

	/*	Enqueue the bundle on the plan for the entry node of
	 *	EACH identified best route.				*/

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
		if (!preview)
		{
			if (enqueueToNeighbor(route, bundle, bundleObj,
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
	|| bundle->id.source.c.nodeNbr == bundle->destination.c.nodeNbr)
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

int	cgr_preview_forward(Bundle *bundle, Object bundleObj,
		uvast terminusNodeNbr, time_t atTime, CgrTrace *trace)
{
	if (cgrForward(bundle, bundleObj, terminusNodeNbr, atTime, trace, 1)
			< 0)
	{
		putErrmsg("Can't compute route.", NULL);
		return -1;
	}

	return 0;
}

int	cgr_forward(Bundle *bundle, Object bundleObj, uvast terminusNodeNbr,
		CgrTrace *trace)
{
	if (cgrForward(bundle, bundleObj, terminusNodeNbr, getUTCTime(), trace,
			0) < 0)
	{
		putErrmsg("Can't compute route.", NULL);
		return -1;
	}

	return 0;
}

float	cgr_prospect(uvast terminusNodeNbr, unsigned int deadline)
{
	PsmPartition	wm = getIonwm();
	IonVdb		*ionvdb = getIonVdb();
	time_t		currentTime = getUTCTime();
	IonNode		*terminusNode;
	PsmAddress	nextNode;
	PsmAddress	routingObjectAddress;
	CgrRtgObject	*routingObject;
	PsmAddress	elt;
	PsmAddress	nextElt;
	PsmAddress	addr;
	CgrRoute	*route;
	float		prospect = 0.0;

	terminusNode = findNode(ionvdb, terminusNodeNbr, & nextNode);
	if (terminusNode == NULL)
	{
		return 0.0;		/*	Unknown node, no chance.*/
	}

	routingObjectAddress = terminusNode->routingObject;
	if (routingObjectAddress == 0)
	{
		return 0.0;		/*	No routes, no chance.	*/
	}

	routingObject = (CgrRtgObject *) psp(wm, routingObjectAddress);
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

		if (route->arrivalConfidence > prospect)
		{
			prospect = route->arrivalConfidence;
		}
	}

	return prospect;
}

void	cgr_start()
{
	oK(getCgrVdb());
}

const char	*cgr_tracepoint_text(CgrTraceType traceType)
{
	int			i = traceType;
	static const char	*tracepointText[] =
	{
	[CgrBuildRoutes] = "BUILD terminusNode:" UVAST_FIELDSPEC
		" payloadLength:%u atTime:%u",
	[CgrInvalidTerminusNode] = "    INVALID terminus node",

	[CgrBeginRoute] = "  ROUTE",
	[CgrConsiderRoot] = "    ROOT fromNode:" UVAST_FIELDSPEC
		" toNode:" UVAST_FIELDSPEC,
	[CgrConsiderContact] = "      CONTACT fromNode:" UVAST_FIELDSPEC
		" toNode:" UVAST_FIELDSPEC,
	[CgrIgnoreContact] = "        IGNORE",

	[CgrCost] = "        COST transmitTime:%u owlt:%u arrivalTime:%u",
	[CgrHop] = "    HOP fromNode:" UVAST_FIELDSPEC " toNode:"
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
	[CgrFullOverbooking] = "	Full OVERBOOKING (amount in bytes):%f",
	[CgrPartialOverbooking] = " Partial OVERBOOKING (amount in bytes):%f",
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

void	cgr_stop()
{
	PsmPartition	wm = getIonwm();
	char		*name = "cgrvdb";
	PsmAddress	vdbAddress;
	PsmAddress	elt;
	CgrVdb		*vdb;
#if 0
	/*Clear Route Caches*/
	clearRoutingObjects(wm);
#endif
	/*Free volatile database*/
	if (psm_locate(wm, name, &vdbAddress, &elt) < 0)
	{
		putErrmsg("Failed searching for vdb.", NULL);
		return;
	}

	if (elt)
	{
		vdb = (CgrVdb *) psp(wm, vdbAddress);
		destroyRoutingObjects(vdb);
		psm_free(wm, vdbAddress);
		if (psm_uncatlg(wm, name) < 0)
		{
			putErrmsg("Failed Uncataloging vdb.",NULL);
		}
	}
}
