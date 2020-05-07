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

static void	removeRoute(PsmPartition ionwm, PsmAddress routeElt)
{
	PsmAddress	routeAddr;
	CgrRoute	*route;
	PsmAddress	citation;
	PsmAddress	nextCitation;
	PsmAddress	contactAddr;
	IonCXref	*contact;
	PsmAddress	citationElt;

	routeAddr = sm_list_data(ionwm, routeElt);
	route = (CgrRoute *) psp(ionwm, routeAddr);
	if (route->referenceElt)
	{
		sm_list_delete(ionwm, route->referenceElt, NULL, NULL);
	}

	if (routeElt != route->referenceElt)
	{
		sm_list_delete(ionwm, routeElt, NULL, NULL);
	}

	/*	Each member of the "hops" list of this route is one
	 *	among possibly many citations of some specific contact,
	 *	i.e., its content is the address of that contact.
	 *	For many-to-many cross-referencing, we append the
	 *	address of each citation - that is, the address of
	 *	the list element that cites some contact - to the
	 *	cited contact's list of citations; the content of
	 *	each member of a contact's citations list is the
	 *	address of one among possibly many citations of that
	 *	contact, each of which is a member of some route's
	 *	list of hops.
	 *
	 *	When a route is removed, we must detach the route
	 *	from every contact that is cited in one of that
	 *	route's hops.  That is, for each hop of the route,
	 *	we must go through the citations list of the contact
	 *	cited by that hop and remove from that list the list
	 *	member whose content is the address of this citation
	 *	- the address of this "hops" list element.		*/

	if (route->hops)
	{
		for (citation = sm_list_first(ionwm, route->hops); citation;
				citation = nextCitation)
		{
			nextCitation = sm_list_next(ionwm, citation);
			contactAddr = sm_list_data(ionwm, citation);
			if (contactAddr == 0)
			{
				/*	This contact has been deleted;
				 *	all references to it have been
				 *	zeroed out.			*/

				sm_list_delete(ionwm, citation, NULL, NULL);
				continue;
			}

			contact = (IonCXref *) psp(ionwm, contactAddr);
			if (contact->citations)
			{
				for (citationElt = sm_list_first(ionwm,
					contact->citations); citationElt;
					citationElt = sm_list_next(ionwm,
					citationElt))
				{
					/*	Does this list element
					 *	point at this route's
					 *	citation of the contact?
					 *	If so, delete it.	*/

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

	if (routingObject->viaPassageways)
	{
		sm_list_destroy(ionwm, routingObject->viaPassageways, NULL,
				NULL);
	}
}

void	cgr_clear_vdb(CgrVdb *vdb)
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

static int	disabledRoute(PsmPartition ionwm, PsmAddress routeElt,
			PsmAddress *routeAddr, CgrRoute **route)
{
	PsmAddress	elt;

	*routeAddr = sm_list_data(ionwm, routeElt);
	*route = (CgrRoute *) psp(ionwm, *routeAddr);
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

CgrVdb	*cgr_get_vdb()
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

	sdr_exit_xn(sdr);
	return vdb;
}

/*	Functions for populating the route lists.			*/

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
	memset((char *) &arg, 0, sizeof(IonCXref));

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

		TRACE(CgrConsiderRoot, current->fromNode, current->toNode);
		arg.fromNode = current->toNode;
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
				work->hopCount =
					(getWorkArea(ionwm, current))->hopCount;
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
		if (current->toNode == terminusNode->nodeNbr)
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

		route->hops = sm_list_create(ionwm);
		if (route->hops == 0)
		{
			putErrmsg("Can't create CGR route hops list.", NULL);
			return -1;
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
			TRACE(CgrHop, contact->fromNode, contact->toNode);
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
				contact->citations = sm_list_create(ionwm);
				if (contact->citations == 0)
				{
					putErrmsg("Can't create citation list.",
							NULL);
					return -1;
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

		route->toNodeNbr = firstContact->toNode;
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
	route->rootOfSpur = rootOfSpur;

	/*	Run Dijkstra search.					*/

	if (computeDistanceToTerminus(rootContact, rootWork, terminusNode,
			currentTime, excludedEdges, route, trace) < 0)
	{
		if (route->hops)
		{
			sm_list_destroy(ionwm, route->hops, NULL, NULL);
		}

		psm_free(ionwm, addr);
		putErrmsg("Can't finish Dijstra search.", NULL);
		return -1;
	}

	if (route->toNodeNbr == 0)
	{
		TRACE(CgrNoMoreRoutes);

		/*	No more routes in graph.			*/

		if (route->hops)
		{
			sm_list_destroy(ionwm, route->hops, NULL, NULL);
		}

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
//debugPrint("*** Suppressing contact to node " UVAST_FIELDSPEC " on root path. ***\n", contact->toNode);
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
//printf("*** Suppressing contact to node " UVAST_FIELDSPEC " after end of root path. ***\n", contact->toNode);
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
		TRACE(CgrHop, contact->fromNode, contact->toNode);
		if (sm_list_insert_first(ionwm, newRoute->hops, contactAddr)
				== 0)
		{
			putErrmsg("Can't prepend trunk to spur route.", NULL);
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
		route->toNodeNbr = contact->toNode;
		route->fromTime = contact->fromTime;
	}

	/*	Append new route to list of known routes.		*/

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
	PsmAddress	nextRouteElt;
	PsmAddress	knownRouteAddr;
	CgrRoute	*knownRoute;
	PsmAddress	bestKnownRouteElt;
	PsmAddress	bestKnownRouteAddr;
	CgrRoute	*bestKnownRoute;

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

static time_t	computePBAT(CgrRoute *route, Bundle *bundle,
			time_t currentTime, BpPlan *plan)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();
	IonVdb		*vdb = getIonVdb();
	uvast		ownNodeNbr = getOwnNodeNbr();
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
	Object		contactObj;
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

	if (acqTime > (bundle->expirationTime + EPOCH_2000_SEC))
	{
		/*	Bundle will never arrive: it will expire
		 *	before arrival.					*/

		acqTime = 0;
	}

	route->pbat = acqTime;
	return acqTime;
}

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

static int	tryRoute(CgrRoute *route, time_t currentTime, Bundle *bundle,
			CgrTrace *trace, Lyst bestRoutes)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	wm = getIonwm();
	char		eid[SDRSTRING_BUFSZ];
	VPlan		*vplan;
	PsmAddress	vplanElt;
	Object		planObj;
	BpPlan		plan;
	time_t		pbat;
	LystElt		candidateElt;
	CgrRoute	*candidateRoute;

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
					if (candidateRoute->toNodeNbr <
							route->toNodeNbr)
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
			PsmAddress *elt, Bundle *bundle, Object bundleObj,
			Lyst excludedNodes, CgrTrace *trace, Lyst bestRoutes,
			time_t currentTime, time_t deadline)
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

	if (route->toNodeNbr == getOwnNodeNbr())
	{
		if (!(bundle->destination.schemeCodeNbr == 2
		&& bundle->destination.ssp.ipn.nodeNbr == route->toNodeNbr))
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

static int	loadBestRoutesList(IonNode *terminusNode, uvast viaNodeNbr,
			Bundle *bundle, Object bundleObj, Lyst excludedNodes,
		       	CgrTrace *trace, Lyst bestRoutes, time_t currentTime,
			time_t deadline, CgrRtgObject *routingObj)
{
	PsmPartition	ionwm = getIonwm();
	PsmAddress	elt;

	/*	Perform route selection outer loop until the list
	 *	of best routes contains the single best route for
	 *	transmission of this bundle.				*/

	elt = sm_list_first(ionwm, routingObj->selectedRoutes);
	while (1)
	{
		switch (checkRoute(terminusNode, viaNodeNbr, &elt, bundle,
				bundleObj, excludedNodes, trace, bestRoutes,
				currentTime, deadline))
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
				 *	of another selected route.	*/

				continue;
			}

			/*	Have checked all selectedRoutes and
			 *	picked the best one as candidate.	*/

			return 0;

		case 0:			/*	No more possible routes.*/
			return 0;

		default:
			putErrmsg("Failed checking route to node.",
					utoa(terminusNode->nodeNbr));
			return -1;
		}
	}
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

int	cgr_identify_best_routes(IonNode *terminusNode, Bundle *bundle,
			Object bundleObj, Lyst excludedNodes, CgrTrace *trace,
			Lyst bestRoutes, time_t currentTime)
{
	PsmPartition	ionwm = getIonwm();
	CgrVdb		*cgrvdb = cgr_get_vdb();
	time_t		deadline;
	CgrRtgObject	*routingObj;

	deadline = bundle->expirationTime + EPOCH_2000_SEC;
	routingObj = (CgrRtgObject *) psp(ionwm, terminusNode->routingObject);
	CHKERR(routingObj);
	if (routingObj->selectedRoutes == 0)
	{
		/*	Must initialize routing object for CGR.		*/

		routingObj->selectedRoutes = sm_list_create(ionwm);
		if (routingObj->selectedRoutes == 0)
		{
			putErrmsg("Can't initialize CGR routing.", NULL);
			return -1;
		}

		routingObj->knownRoutes = sm_list_create(ionwm);
		if (routingObj->knownRoutes == 0)
		{
			putErrmsg("Can't initialize CGR routing.", NULL);
			return -1;
		}

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

static void	deleteObject(LystElt elt, void *userdata)
{
	void	*object = lyst_data(elt);

	if (object)
	{
		MRELEASE(lyst_data(elt));
	}
}

int	cgr_preview_forward(Bundle *bundle, Object bundleObj,
		uvast terminusNodeNbr, time_t atTime, CgrTrace *trace)
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
	lyst_delete_set(excludedNodes, deleteObject, NULL);
	if (terminusNode->routingObject == 0)
	{
		if (cgr_create_routing_object(terminusNode) < 0)
		{
			putErrmsg("Can't initialize routing object.", NULL);
			return -1;
		}
	}

	result = cgr_identify_best_routes(terminusNode, bundle, bundleObj,
			excludedNodes, trace, bestRoutes, atTime);
	lyst_destroy(bestRoutes);
	lyst_destroy(excludedNodes);
       	if (result < 0)
	{
		putErrmsg("Can't compute route.", NULL);
		return -1;
	}

	return 0;
}

int	cgr_prospect(uvast terminusNodeNbr, unsigned int deadline)
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

void	cgr_start()
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

void	cgr_stop()
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
