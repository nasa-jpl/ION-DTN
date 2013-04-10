/*
	libcgr.c:	functions implementing Contact Graph Routing.

	Author: Scott Burleigh, JPL

	Adaptation to use Dijkstra's Algorithm developed by John
	Segui, 2011.

	Copyright (c) 2008, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
#include "cgr.h"

#ifndef CGRDEBUG
#define CGRDEBUG	0
#endif

#define	MAX_TIME	((unsigned int) ((1U << 31) - 1))

/*		CGR-specific RFX data structures.			*/

typedef struct
{
	/*	Contact that forms the initial hop of the route.	*/

	uvast		toNodeNbr;	/*	Initial-hop neighbor.	*/
	time_t		fromTime;	/*	As from time(2).	*/
	time_t		toTime;		/*	As from time(2).	*/

	/*	Details of the route.					*/

	time_t		deliveryTime;	/*	As from time(2).	*/
	PsmAddress	hops;		/*	SM list: IonCXref addr	*/
	uvast		maxCapacity;
	int		payloadClass;
} CgrRoute;		/*	IonNode routingObject is list of these.	*/

typedef struct
{
	/*	Working values, reset for each Dijkstra run.		*/

	IonCXref	*predecessor;	/*	On path to destination.	*/
	uvast		capacity;
	time_t		arrivalTime;	/*	As from time(2).	*/
	int		visited;	/*	Boolean.		*/
	int		suppressed;	/*	Boolean.		*/
} CgrContactNote;	/*	IonCXref routingObject is one of these.	*/

/*		Data structure for the CGR volatile database.		*/

typedef struct
{
	time_t		lastLoadTime;	/*	Add/del contacts/ranges	*/
	PsmAddress	routeLists;	/*	SM list: CgrRoute list	*/
} CgrVdb;

/*		Data structure for temporary linked list.		*/

typedef struct
{
	uvast		neighborNodeNbr;
	FwdDirective	directive;
	time_t		forfeitTime;
	time_t		deliveryTime;
	int		hopCount;	/*	# hops from dest. node.	*/
} ProximateNode;

#define	PAYLOAD_CLASSES	3

static uvast	_minCapacity(int payloadClass)
{
	static uvast	capacityFloor[PAYLOAD_CLASSES] =
				{ 1024, 1024*1024, 1024*1024*1024 };

	if (payloadClass < 0 || payloadClass >= PAYLOAD_CLASSES)
	{
		return (uvast) -1;
	}

	return capacityFloor[payloadClass];
}

/*		Functions for managing the CGR database.		*/

static void	discardRouteList(PsmPartition ionwm, PsmAddress routes)
{
	PsmAddress	elt2;
	PsmAddress	next2;
	PsmAddress	addr;
	CgrRoute	*route;

	if (routes == 0)
	{
		return;
	}

	/*	Erase all routes in the list.				*/

	for (elt2 = sm_list_first(ionwm, routes); elt2; elt2 = next2)
	{
		next2 = sm_list_next(ionwm, elt2);
		addr = sm_list_data(ionwm, elt2);
		route = (CgrRoute *) psp(ionwm, addr);
		if (route->hops)
		{
			sm_list_destroy(ionwm, route->hops, NULL, NULL);
		}

		psm_free(ionwm, addr);
		sm_list_delete(ionwm, elt2, NULL, NULL);
	}

	/*	Destroy the list of routes to this remote node.	*/

	sm_list_destroy(ionwm, routes, NULL, NULL);
}

static void	discardRouteLists(CgrVdb *vdb)
{
	PsmPartition	ionwm = getIonwm();
	PsmAddress	elt;
	PsmAddress	nextElt;
	PsmAddress	routes;		/*	SM list: CgrRoute	*/
	PsmAddress	addr;
	IonNode		*node;

	for (elt = sm_list_first(ionwm, vdb->routeLists); elt; elt = nextElt)
	{
		nextElt = sm_list_next(ionwm, elt);
		routes = sm_list_data(ionwm, elt);	/*	SmList	*/

		/*	Detach route list from remote node.		*/

		addr = sm_list_user_data(ionwm, routes);
		node = (IonNode *) psp(ionwm, addr);
		node->routingObject = 0;

		/*	Discard the list of routes to remote node.	*/

		discardRouteList(ionwm, routes);

		/*	And delete the reference to the destroyed list.	*/

		sm_list_delete(ionwm, elt, NULL, NULL);
	}
}

static void	clearRoutingObjects(PsmPartition ionwm)
{
	IonVdb		*ionvdb = getIonVdb();
	PsmAddress	elt;
	IonNode		*node;
	PsmAddress	routes;

	for (elt = sm_rbt_first(ionwm, ionvdb->nodes); elt;
			elt = sm_rbt_next(ionwm, elt))
	{
		node = (IonNode *) psp(ionwm, sm_rbt_data(ionwm, elt));
		if (node->routingObject)
		{
			routes = node->routingObject;
			node->routingObject = 0;
			discardRouteList(ionwm, routes);
		}
	}
}

static CgrVdb	*_cgrvdb(char **name)
{
	static CgrVdb	*vdb = NULL;
	PsmPartition	ionwm;
	PsmAddress	vdbAddress;
	PsmAddress	elt;
	Sdr		sdr;

	if (name)
	{
		if (*name == NULL)	/*	Terminating.		*/
		{
			vdb = NULL;
			return vdb;
		}

		/*	Attaching to volatile database.			*/

		ionwm = getIonwm();
		if (psm_locate(ionwm, *name, &vdbAddress, &elt) < 0)
		{
			putErrmsg("Failed searching for vdb.", *name);
			return NULL;
		}

		if (elt)
		{
			vdb = (CgrVdb *) psp(ionwm, vdbAddress);
			return vdb;
		}

		/*	CGR volatile database doesn't exist yet.	*/

		sdr = getIonsdr();
		CHKNULL(sdr_begin_xn(sdr));	/*	To lock memory.	*/
		vdbAddress = psm_zalloc(ionwm, sizeof(CgrVdb));
		if (vdbAddress == 0)
		{
			sdr_exit_xn(sdr);
			putErrmsg("No space for volatile database.", *name);
			return NULL;
		}

		vdb = (CgrVdb *) psp(ionwm, vdbAddress);
		memset((char *) vdb, 0, sizeof(CgrVdb));
		if ((vdb->routeLists = sm_list_create(ionwm)) == 0
		|| psm_catlg(ionwm, *name, vdbAddress) < 0)
		{
			sdr_exit_xn(sdr);
			putErrmsg("Can't initialize volatile database.", *name);
			return NULL;
		}

		clearRoutingObjects(ionwm);
		sdr_exit_xn(sdr);
	}

	return vdb;
}

/*		Functions for loading the routing table.		*/

static int	computeDistanceToStation(IonCXref *rootContact,
			CgrContactNote *rootWork, IonNode *stationNode,
			int payloadClass, CgrRoute *route)
{
	PsmPartition	ionwm = getIonwm();
	IonVdb		*ionvdb = getIonVdb();
	uvast		capacityFloor = _minCapacity(payloadClass);
	IonCXref	*current;
	CgrContactNote	*currentWork;
	IonCXref	arg;
	PsmAddress	elt;
	IonCXref	*contact;
	CgrContactNote	*work;
	IonRXref	arg2;
	PsmAddress	elt2;
	IonRXref	*range;
	unsigned int	owltMargin;
	unsigned int	owlt;
	time_t		transmitTime;
	time_t		arrivalTime;
	IonCXref	*finalContact = NULL;
	time_t		earliestDeliveryTime = MAX_TIME;
	IonCXref	*nextContact;
	time_t		earliestArrivalTime;
	time_t		earliestEndTime;
	uvast		maxCapacity;
	PsmAddress	addr;

	/*	This is an implementation of Dijkstra's Algorithm.	*/

#if CGRDEBUG
printf("\nSeeking a class %d route via contact to node " UVAST_FIELDSPEC
" arrival time %u.\n", payloadClass, rootContact->toNode, (unsigned int)
(rootWork->arrivalTime));
#endif
	current = rootContact;
	currentWork = rootWork;
	memset((char *) &arg, 0, sizeof(IonCXref));
	memset((char *) &arg2, 0, sizeof(IonRXref));
	while (1)
	{
		/*	Consider all unvisited neighbors (i.e., next-
		 *	hop contacts) of the current contact.		*/

		arg.fromNode = current->toNode;
#if CGRDEBUG
printf("\nConsidering contacts from node " UVAST_FIELDSPEC ".\n", arg.fromNode);
#endif
		for (oK(sm_rbt_search(ionwm, ionvdb->contactIndex,
				rfx_order_contacts, &arg, &elt));
				elt; elt = sm_rbt_next(ionwm, elt))
		{
			contact = (IonCXref *) psp(ionwm,
					sm_rbt_data(ionwm, elt));
#if CGRDEBUG
printf("Examining contact from node " UVAST_FIELDSPEC " to node "
UVAST_FIELDSPEC " starting at %u.\n", contact->fromNode, contact->toNode,
(unsigned int) (contact->fromTime));
#endif
			if (contact->fromNode > arg.fromNode)
			{
#if CGRDEBUG
printf("Contact is not from the node we're routing from (" UVAST_FIELDSPEC "\
).\n", arg.fromNode);
#endif
				/*	No more relevant contacts.	*/

				break;
			}

			if (contact->toTime <= currentWork->arrivalTime)
			{
#if CGRDEBUG
printf("Contact ends before current contact arrival time.\n");
#endif
				/*	Can't be a next-hop contact:
				 *	transmission has stopped by
				 *	the time of arrival of data
				 *	during the current contact.	*/

				continue;
			}

			work = (CgrContactNote *) psp(ionwm,
					contact->routingObject);
			CHKERR(work);
			if (work->suppressed || work->visited)
			{
#if CGRDEBUG
printf("Contact to node " UVAST_FIELDSPEC " is suppressed or visited.\n",
contact->toNode);
#endif
				continue;
			}

			/*	Exclude contact if its capacity is
			 *	less than the floor for this payload
			 *	class.					*/

			if (work->capacity == 0)
			{
				work->capacity = contact->xmitRate *
					(contact->toTime - contact->fromTime);
			}

			if (work->capacity < capacityFloor)
			{
#if CGRDEBUG
printf("Contact capacity " UVAST_FIELDSPEC " too low for payload class.\n",
work->capacity);
#endif
				continue;
			}

			/*	Get OWLT between the nodes in contact,
			 *	from applicable range in range index.	*/

			arg2.fromNode = arg.fromNode;
			arg2.toNode = contact->toNode;
			for (oK(sm_rbt_search(ionwm, ionvdb->rangeIndex,
					rfx_order_ranges, &arg2, &elt2));
					elt2; elt2 = sm_rbt_next(ionwm, elt2))
			{
				range = (IonRXref *)
					psp(ionwm, sm_rbt_data(ionwm, elt2));
				CHKERR(range);
				if (range->fromNode > arg2.fromNode
				|| range->toNode > arg2.toNode)
				{
					elt2 = 0;
					break;
				}

				if (range->toTime < contact->fromTime)
				{
					continue;	/*	Past.	*/
				}

				if (range->fromTime > contact->fromTime)
				{
					elt2 = 0;
				}

				break;
			}

			if (elt2 == 0)
			{
#if CGRDEBUG
printf("Don't have range for this contact.\n");
#endif
				/*	Don't know the OWLT between
				 *	these BP nodes at this time,
				 *	so can't consider in CGR.	*/

				continue;
			}

			/*	Allow for possible additional latency
			 *	due to the movement of the receiving
			 *	node during the propagation of signal
			 *	from the sending node.			*/

			owltMargin = ((MAX_SPEED_MPH / 3600) * range->owlt)
					/ 186282;
			owlt = range->owlt + owltMargin;

			/*	Compute cost of choosing this edge:
			 *	earliest bundle arrival time.		*/
#if CGRDEBUG
printf("currentWork->arrival time %lu, contact->fromTime %lu, owlt %u.\n",
currentWork->arrivalTime, contact->fromTime, owlt);
#endif
			if (contact->fromTime < currentWork->arrivalTime)
			{
				transmitTime = currentWork->arrivalTime;
			}
			else
			{
				transmitTime = contact->fromTime;
			}

			arrivalTime = transmitTime + owlt;
#if CGRDEBUG
printf("Computed arrival time %lu, work->arrivalTime %lu, \
earliestDeliveryTime %lu.\n", arrivalTime, work->arrivalTime,
earliestDeliveryTime);
#endif
			if (arrivalTime < work->arrivalTime)
			{
				work->arrivalTime = arrivalTime;
				work->predecessor = current;

				/*	Note contact if could be final.	*/

				if (contact->toNode == stationNode->nodeNbr)
				{
					if (work->arrivalTime
						< earliestDeliveryTime)
					{
						earliestDeliveryTime
							= work->arrivalTime;
						finalContact = contact;
#if CGRDEBUG
printf("Updated earliest delivery time.\n");
#endif
					}
				}
			}
		}

		currentWork->visited = 1;

		/*	Select next contact to consider, if any.	*/

		nextContact = NULL;
		earliestArrivalTime = MAX_TIME;
		for (elt = sm_rbt_first(ionwm, ionvdb->contactIndex); elt;
				elt = sm_rbt_next(ionwm, elt))
		{
			contact = (IonCXref *) psp(ionwm, sm_rbt_data(ionwm,
					elt));
			CHKERR(contact);
			work = (CgrContactNote *) psp(ionwm,
					contact->routingObject);
			CHKERR(work);
			if (work->suppressed || work->visited)
			{
				continue;	/*	Ineligible.	*/
			}

			if (work->arrivalTime > earliestDeliveryTime)
			{
				/*	Not on optimal path; ignore.	*/

				continue;
			}

			if (work->arrivalTime < earliestArrivalTime)
			{
				nextContact = contact;
				earliestArrivalTime = work->arrivalTime;
			}
		}

		/*	If search is complete, stop.  Else repeat,
		 *	with new value of "current".			*/

		if (nextContact == NULL)
		{
#if CGRDEBUG
printf("Dijkstra search has ended.\n");
#endif
			/*	End of search.				*/

			break;
		}

		current = nextContact;
#if CGRDEBUG
printf("Now continuing from node " UVAST_FIELDSPEC ".\n", current->toNode);
#endif
		currentWork = (CgrContactNote *)
				psp(ionwm, nextContact->routingObject);
	}

	/*	Have finished Dijkstra search of contact graph,
	 *	excluding those contacts that were suppressed.		*/

	if (finalContact)	/*	Found a route to station node.	*/
	{
		route->deliveryTime = earliestDeliveryTime;

		/*	Load the entire route into the "hops" list,
		 *	backtracking to root, and compute the time
		 *	at which the route will become unusable.	*/

		earliestEndTime = MAX_TIME;
		maxCapacity = (uvast) -1;
		for (contact = finalContact; contact != rootContact;
				contact = work->predecessor)
		{
			if (contact->toTime < earliestEndTime)
			{
				earliestEndTime = contact->toTime;
			}

			work = (CgrContactNote *) psp(ionwm,
					contact->routingObject);
			if (work->capacity < maxCapacity)
			{
				maxCapacity = work->capacity;
			}

			addr = psa(ionwm, contact);
			if (sm_list_insert_first(ionwm, route->hops, addr) == 0)
			{
				putErrmsg("Can't insert contact into route.",
						NULL);
				return -1;
			}
		}

		/*	Now use the first contact in the route to
		 *	characterize the route.				*/

		addr = sm_list_data(ionwm, sm_list_first(ionwm, route->hops));
		contact = (IonCXref *) psp(ionwm, addr);
		route->toNodeNbr = contact->toNode;
		route->fromTime = contact->fromTime;
		route->toTime = earliestEndTime;
		route->maxCapacity = maxCapacity;
		route->payloadClass = payloadClass;
	}

	return 0;
}

static int	findNextBestRoute(PsmPartition ionwm, IonCXref *rootContact,
			CgrContactNote *rootWork, IonNode *stationNode,
			int payloadClass, PsmAddress *routeAddr)
{
	PsmAddress	addr;
	CgrRoute	*route;

	*routeAddr = 0;		/*	Default.			*/
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

	/*	Run Dijkstra search.					*/

	if (computeDistanceToStation(rootContact, rootWork, stationNode,
			payloadClass, route) < 0)
	{
		putErrmsg("Can't finish Dijstra search.", NULL);
		return -1;
	}

	if (route->toNodeNbr == 0)
	{
#if CGRDEBUG
printf("----No more class-%d routes to node " UVAST_FIELDSPEC ".\n",
payloadClass, stationNode->nodeNbr);
#endif
		/*	No more routes found in graph.			*/

		sm_list_destroy(ionwm, route->hops, NULL, NULL);
		psm_free(ionwm, addr);
		*routeAddr = 0;
	}
	else
	{
#if CGRDEBUG
printf("----Found class-%d route via node " UVAST_FIELDSPEC
": start @ %lu, deliver @ %lu.\n", payloadClass, route->toNodeNbr,
route->fromTime, route->deliveryTime);
#endif
		/*	Found best route, given current exclusions.	*/

		*routeAddr = addr;
	}

	return 0;
}

static PsmAddress	loadRouteList(IonNode *stationNode, time_t currentTime)
{
	PsmPartition	ionwm = getIonwm();
	IonVdb		*ionvdb = getIonVdb();
	CgrVdb		*cgrvdb = _cgrvdb(NULL);
	int		payloadClass;
	PsmAddress	elt;
	IonCXref	*contact;
	CgrContactNote	*work;
	IonCXref	rootContact;
	CgrContactNote	rootWork;
	PsmAddress	routeAddr;
	CgrRoute	*route;
	IonCXref	*firstContact;

	CHKZERO(ionvdb);
	CHKZERO(cgrvdb);

	/*	First create route list for this destination node.	*/

	stationNode->routingObject = sm_list_create(ionwm);
	if (stationNode->routingObject == 0)
	{
		putErrmsg("Can't create CGR route list.", NULL);
		return 0;
	}

	oK(sm_list_user_data_set(ionwm, stationNode->routingObject,
			psa(ionwm, stationNode)));
	if (sm_list_insert_last(ionwm, cgrvdb->routeLists,
			stationNode->routingObject) == 0)
	{
		putErrmsg("Can't note CGR route list.", NULL);
		return 0;
	}

#if CGRDEBUG
printf(">>>Computing all routes from node " UVAST_FIELDSPEC " to \
node " UVAST_FIELDSPEC ".\n", getOwnNodeNbr(), stationNode->nodeNbr);
#endif
	/*	Now note the best routes (transmission sequences,
	 *	paths, itineraries) from the local node that can
	 *	result in delivery at the remote node.  To do this,
	 *	we run multiple series of Dijkstra searches (one
	 *	series per payload class) through the contact
	 *	graph, rooted at a dummy contact from the local node
	 *	to itself and terminating in the "final contact"
	 *	(which is the station node's contact with itself).
	 *	Each time we search, we exclude from consideration
	 *	the first contact in every previously computed route.	*/

	rootContact.fromNode = getOwnNodeNbr();
	rootContact.toNode = rootContact.fromNode;
	rootWork.arrivalTime = currentTime;
	for (payloadClass = 0; payloadClass < PAYLOAD_CLASSES; payloadClass++)
	{
		/*	For each series of searches, clear Dijkstra
		 *	work areas for all contacts.			*/

		for (elt = sm_rbt_first(ionwm, ionvdb->contactIndex); elt;
				elt = sm_rbt_next(ionwm, elt))
		{
			contact = (IonCXref *)
					psp(ionwm, sm_rbt_data(ionwm, elt));
			if ((work = (CgrContactNote *) psp(ionwm,
					contact->routingObject)) == 0)
			{
				contact->routingObject = psm_zalloc(ionwm,
						sizeof(CgrContactNote));
				work = (CgrContactNote *) psp(ionwm,
						contact->routingObject);
				if (work == 0)
				{
					putErrmsg("Can't create contact note.",
							NULL);
					return 0;
				}
			}

			memset((char *) work, 0, sizeof(CgrContactNote));
			work->arrivalTime = MAX_TIME;
		}

		while (1)
		{
			if (findNextBestRoute(ionwm, &rootContact, &rootWork,
				stationNode, payloadClass, &routeAddr) < 0)
			{
				putErrmsg("Can't load routes list.", NULL);
				return 0;
			}

			if (routeAddr == 0)
			{
				/*	No more routes for this class.	*/

				break;	/*	Move on to next class.	*/
			}

			/*	Found optimal route, given exclusion
			 *	of all contacts that are the initial
			 *	contacts on previously discovered
			 *	optimal routes.				*/

			if (sm_list_insert_last(ionwm,
				stationNode->routingObject, routeAddr) == 0)
			{
				putErrmsg("Can't add route to list.", NULL);
				return 0;
			}

			/*	Now exclude the initial contact in this
			 *	optimal route, re-clear, and try again.	*/

			route = (CgrRoute *) psp(ionwm, routeAddr);
			firstContact = (IonCXref *)
					psp(ionwm, sm_list_data(ionwm,
					sm_list_first(ionwm, route->hops)));
			work = (CgrContactNote *)
					psp(ionwm, firstContact->routingObject);
			work->suppressed = 1;
			for (elt = sm_rbt_first(ionwm, ionvdb->contactIndex);
				       	elt; elt = sm_rbt_next(ionwm, elt))
			{
				contact = (IonCXref *)
					psp(ionwm, sm_rbt_data(ionwm, elt));
				work = (CgrContactNote *)
					psp(ionwm, contact->routingObject);
				work->arrivalTime = MAX_TIME;
				work->predecessor = NULL;
				work->visited = 0;
			}
		}
	}

#if CGRDEBUG
printf("<<<Computed all routes from node " UVAST_FIELDSPEC " to node "
UVAST_FIELDSPEC ".\n\n", getOwnNodeNbr(), stationNode->nodeNbr);
#endif
	return stationNode->routingObject;
}

/*		Functions for identifying viable proximate nodes.	*/

static int	recomputeRouteForContact(uvast contactToNodeNbr,
			time_t contactFromTime, IonNode *stationNode,
			time_t currentTime, int payloadClass)
{
	PsmPartition	ionwm = getIonwm();
	IonVdb		*vdb = getIonVdb();
	PsmAddress	routes;
	IonCXref	arg;
	PsmAddress	cxelt;
	PsmAddress	nextElt;
	IonCXref	*contact;
	CgrContactNote	*work;
	PsmAddress	elt;
	CgrRoute	*route;
	IonCXref	rootContact;
	CgrContactNote	rootWork;
	PsmAddress	routeAddr;
	CgrRoute	*newRoute;

	routes = stationNode->routingObject;
	arg.fromNode = getOwnNodeNbr();
	arg.toNode = contactToNodeNbr;
	arg.fromTime = contactFromTime;
	cxelt = sm_rbt_search(ionwm, vdb->contactIndex, rfx_order_contacts,
			&arg, &nextElt);
	if (cxelt == 0)
	{
		return 0;	/*	Can't find the contact.		*/
	}

	contact = (IonCXref *) psp(ionwm, sm_rbt_data(ionwm, cxelt));
	if (contact->toTime <= currentTime)
	{
		return 0;	/*	Contact is expired.		*/
	}

	/*	Recompute route through this leading contact.  First
	 *	clear Dijkstra work areas for all contacts in the
	 *	contactIndex.						*/

	for (cxelt = sm_rbt_first(ionwm, vdb->contactIndex); cxelt;
			cxelt = sm_rbt_next(ionwm, cxelt))
	{
		contact = (IonCXref *) psp(ionwm, sm_rbt_data(ionwm, cxelt));
		if ((work = (CgrContactNote *) psp(ionwm,
				contact->routingObject)) == 0)
		{
			contact->routingObject = psm_zalloc(ionwm,
					sizeof(CgrContactNote));
			work = (CgrContactNote *) psp(ionwm,
					contact->routingObject);
			if (work == 0)
			{
				putErrmsg("Can't create CGR contact note.",
						NULL);
				return -1;
			}
		}

		memset((char *) work, 0, sizeof(CgrContactNote));
		work->arrivalTime = MAX_TIME;
	}

	/*	Now suppress from consideration as lead contact
	 *	every contact that is already the leading contact of
	 *	any remaining route in stationNode's list of routes.	*/

	for (elt = sm_list_first(ionwm, routes); elt; elt =
			sm_list_next(ionwm, elt))
	{
		route = (CgrRoute *) psp(ionwm, sm_list_data(ionwm, elt));
		arg.fromNode = getOwnNodeNbr();
		arg.toNode = route->toNodeNbr;
		arg.fromTime = route->fromTime;
		cxelt = sm_rbt_search(ionwm, vdb->contactIndex,
				rfx_order_contacts, &arg, &nextElt);
		if (cxelt == 0)
		{
			/*	This is an old route, for a contact
			 *	that is already ended, but the route
			 *	hasn't been purged yet because it
			 *	hasn't been used recently.  Ignore it.	*/

			continue;
		}

		contact = (IonCXref *) psp(ionwm, sm_rbt_data(ionwm, cxelt));
		work = (CgrContactNote *) psp(ionwm, contact->routingObject);
		work->suppressed = 1;
	}

	/*	Next invoke findNextBestRoute to produce a new route
	 *	starting at the subject contact.			*/

	rootContact.fromNode = getOwnNodeNbr();
	rootContact.toNode = rootContact.fromNode;
	rootWork.arrivalTime = currentTime;
	if (findNextBestRoute(ionwm, &rootContact, &rootWork,
			stationNode, payloadClass, &routeAddr) < 0)
	{
		putErrmsg("Can't recompute route.", NULL);
		return -1;
	}

	if (routeAddr == 0)		/*	No route computed.	*/
	{
		return 0;
	}

	/*	Finally, insert that route into the stationNode's
	 *	list of routes in deliveryTime order.			*/

	newRoute = (CgrRoute *) psp(ionwm, routeAddr);
	for (elt = sm_list_first(ionwm, routes); elt; elt =
			sm_list_next(ionwm, elt))
	{
		route = (CgrRoute *) psp(ionwm, sm_list_data(ionwm, elt));
		if (route->deliveryTime <= newRoute->deliveryTime)
		{
			continue;
		}

		break;		/*	Insert before this route.	*/
	}

	if (elt)
	{
		sm_list_insert_before(ionwm, elt, routeAddr);
	}
	else
	{
		sm_list_insert_last(ionwm, routes, routeAddr);
	}

	return 1;
}

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

static int	tryRoute(CgrRoute *route, time_t currentTime, Bundle *bundle,
			Object plans, CgrLookupFn getDirective,
			Lyst proximateNodes)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();
	IonVdb		*vdb = getIonVdb();
	Scalar		backlog;
	Scalar		aggregateCapacity;
	uvast		ownNodeNbr = getOwnNodeNbr();
	IonCXref	arg;
	PsmAddress	elt;
	IonCXref	*contact;
	FwdDirective	directive;
	Outduct		outduct;
	time_t		startTime;
	time_t		endTime;
	int		secRemaining;
	Scalar		xmitCapacity;
	int		eccc;	/*	Estimated capacity consumption.	*/
	ClProtocol	protocol;
	LystElt		elt2;
	int		hopCount;
	ProximateNode	*proxNode;

#if CGRDEBUG
printf("Trying class-%d route via node " UVAST_FIELDSPEC
": start @ %lu, deliver @ %lu.\n", route->payloadClass, route->toNodeNbr,
route->fromTime, route->deliveryTime);
#endif
	if (getDirective(route->toNodeNbr, plans, bundle, &directive) == 0)
	{
#if CGRDEBUG
puts("No applicable directive.");
#endif
		return 0;		/*	No applicable directive.*/
	}

	/*	Now determine whether or not the bundle could be sent
	 *	to this neighbor via the outduct for this directive
	 *	in time to follow the route that is being considered.
	 *	There are three criteria.  First, is the duct blocked
	 *	(e.g., no TCP connection)?				*/

	sdr_read(sdr, (char *) &outduct, sdr_list_data(sdr,
			directive.outductElt), sizeof(Outduct));
	if (outduct.blocked)
	{
#if CGRDEBUG
puts("Outduct is blocked.");
#endif
		return 0;		/*	Outduct is unusable.	*/
	}

	/*	Second: if the bundle is flagged "do not fragment",
	 *	does the length of its payload exceed the duct's
	 *	payload size limit (if any)?				*/

	if (bundle->bundleProcFlags & BDL_DOES_NOT_FRAGMENT
	&& outduct.maxPayloadLen != 0)
	{
		if (bundle->payload.length > outduct.maxPayloadLen)
		{
			return 0;	/*	Bundle can't be sent.	*/
		}
	}

	/*	Third: is the sum of the capacities of all scheduled
	 *	contact intervals with this route's initial proximate
	 *	destination, through the end of the initial contact
	 *	on this route, greater than the sum of the current
	 *	applicable backlog of pending transmissions on that
	 *	outduct plus the estimated transmission capacity
	 *	consumption of this bundle?  For this purpose we need
	 *	to scan the scheduled intervals of contact with the
	 *	candidate neighbor.					*/

	computeApplicableBacklog(&outduct, bundle, &backlog);
	loadScalar(&aggregateCapacity, 0);

	/*	Locate earliest contact from local node to neighbor.	*/

	memset((char *) &arg, 0, sizeof(IonCXref));
	arg.fromNode = ownNodeNbr;
	arg.toNode = route->toNodeNbr;
	for (oK(sm_rbt_search(ionwm, vdb->contactIndex, rfx_order_contacts,
			&arg, &elt)); elt; elt = sm_rbt_next(ionwm, elt))
	{
		contact = (IonCXref *) psp(ionwm, sm_rbt_data(ionwm, elt));
		if (contact->fromNode > ownNodeNbr)
		{
			/*	No more contacts for local node.	*/

			break;
		}

		if (contact->toNode > route->toNodeNbr)
		{
			/*	No more contacts with this neighbor.	*/

			break;
		}

		if (contact->toTime < currentTime)
		{
			/*	This contact has already terminated.	*/

			continue;
		}

		if (contact->fromTime >= route->toTime)
		{
			/*	No more contacts that contribute to
			 *	the aggregate capacity of the contacts
			 *	supporting the route we're considering.	*/

			break;
		}

		if (currentTime > contact->fromTime)
		{
			startTime = currentTime;
		}
		else
		{
			startTime = contact->fromTime;
		}

		if (route->toTime > contact->toTime)
		{
			endTime = contact->toTime;
		}
		else
		{
			endTime = route->toTime;
		}

		secRemaining = endTime - startTime;
		loadScalar(&xmitCapacity, secRemaining);
		multiplyScalar(&xmitCapacity, contact->xmitRate);
		addToScalar(&aggregateCapacity, &xmitCapacity);
	}

	subtractFromScalar(&aggregateCapacity, &backlog);
	if (!scalarIsValid(&aggregateCapacity))
	{
		/*	Contacts with this neighbor are already fully
		 *	subscribed; no available residual capacity.	*/
#if CGRDEBUG
puts("Contact with this neighbor is already fully subscribed.");
#endif
		return 0;
	}

	sdr_read(sdr, (char *) &protocol, outduct.protocol, sizeof(ClProtocol));
	eccc = computeECCC(guessBundleSize(bundle), &protocol);
	reduceScalar(&aggregateCapacity, eccc);
	if (!scalarIsValid(&aggregateCapacity))
	{
		/*	Available residual capacity is not enough
		 *	to get all of this bundle transmitted.		*/
#if CGRDEBUG
puts("Too little residual aggregate capacity for this bundle.");
#endif
		return 0;
	}

	/*	This route is a plausible opportunity for getting
	 *	the bundle forwarded to its destination before it
	 *	expires, so we look to see if the route's initial
	 *	proximate node is already in the list of candidate
	 *	proximate nodes for this bundle.  If not, we add
	 *	it; if so, we update the associated best delivery
	 *	time, minimum hop count, and forfeit time as
	 *	necessary.
	 *
	 *	The deliveryTime noted for a proximate node is the
	 *	earliest among the projected delivery times on all
	 *	plausible paths to the destination that start with
	 *	transmission to that neighbor, i.e., among all
	 *	plausible routes.
	 *
	 *	The hopCount noted here is the smallest among the
	 *	hopCounts projected on all plausible paths to the
	 *	destination, starting at the candidate proximate
	 *	node, that share the minimum deliveryTime.
	 *
	 *	We set forfeit time to the forfeit time associated
	 *	with the "best" (lowest-latency, shortest) path.
	 *	Note that the best path might not have the lowest
	 *	associated forfeit time.				*/

	hopCount = sm_list_length(ionwm, route->hops);
	for (elt2 = lyst_first(proximateNodes); elt2; elt2 = lyst_next(elt2))
	{
		proxNode = (ProximateNode *) lyst_data(elt2);
		if (proxNode->neighborNodeNbr == route->toNodeNbr)
		{
			/*	This route starts with contact with a
			 *	neighbor that's already in the list.	*/

			if (route->deliveryTime < proxNode->deliveryTime)
			{
				proxNode->deliveryTime = route->deliveryTime;
				proxNode->hopCount = hopCount;
				proxNode->forfeitTime = route->toTime;
			}
			else
			{
				if (route->deliveryTime
						== proxNode->deliveryTime)
				{
					if (hopCount < proxNode->hopCount)
					{
						proxNode->hopCount = hopCount;
						proxNode->forfeitTime =
							route->toTime;
					}
				}
			}

#if CGRDEBUG
printf("Additional opportunity via node " UVAST_FIELDSPEC ": %d hops, \
delivery at %u.\n", route->toNodeNbr, hopCount, (unsigned int)
(route->deliveryTime));
#endif
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

	proxNode->neighborNodeNbr = route->toNodeNbr;
	memcpy((char *) &(proxNode->directive), (char *) &directive,
			sizeof(FwdDirective));
	proxNode->deliveryTime = route->deliveryTime;
	proxNode->hopCount = hopCount;
	proxNode->forfeitTime = route->toTime;
#if CGRDEBUG
printf("Proposing proximate node " UVAST_FIELDSPEC ": %d hops, delivery at \
%u.\n", route->toNodeNbr, hopCount, (unsigned int) (route->deliveryTime));
#endif
	return 0;
}

static int	identifyProximateNodes(IonNode *stationNode, Bundle *bundle,
			Object bundleObj, Lyst excludedNodes, Object plans,
			CgrLookupFn getDirective, Lyst proximateNodes)
{
	PsmPartition	ionwm = getIonwm();
	unsigned int	deadline;
	time_t		currentTime;
	PsmAddress	routes;		/*	SmList of CgrRoutes.	*/
	PsmAddress	elt;
	PsmAddress	nextElt;
	PsmAddress	addr;
	CgrRoute	*route;
	uvast		contactToNodeNbr;
	time_t		contactFromTime;
	int		payloadClass;
	unsigned long	radiationLatency;
	PsmAddress	elt2;
	IonCXref	*contact;

	deadline = bundle->expirationTime + EPOCH_2000_SEC;

	/*	Examine all opportunities for transmission to any
	 *	neighboring node that would result in delivery to
	 *	the station node.  Walk the list in ascending final
	 *	delivery time order, stopping at the first route
	 *	for which the final delivery time would be after
	 *	the bundle's expiration time (deadline).		*/

#if CGRDEBUG
printf("In identifyProximateNodes for node " UVAST_FIELDSPEC ", deadline %u.\n",
stationNode->nodeNbr, deadline);
#endif
	currentTime = getUTCTime();
	routes = stationNode->routingObject;
	if (routes == 0)
	{
		if ((routes = loadRouteList(stationNode, currentTime)) == 0)
		{
			putErrmsg("Can't load routes for node.",
					utoa(stationNode->nodeNbr));
			return -1;
		}
	}

	for (elt = sm_list_first(ionwm, routes); elt; elt = nextElt)
	{
		nextElt = sm_list_next(ionwm, elt);
		addr = sm_list_data(ionwm, elt);
		route = (CgrRoute *) psp(ionwm, addr);
#if CGRDEBUG
printf("Checking class-%d route via node " UVAST_FIELDSPEC
": start @ %lu, deliver @ %lu.\n", route->payloadClass, route->toNodeNbr,
route->fromTime, route->deliveryTime);
#endif
		if (route->toTime < currentTime)
		{
			/*	This route leads off with a contact
			 *	that has already ended; delete it.	*/

			contactToNodeNbr = route->toNodeNbr;
			contactFromTime = route->fromTime;
			payloadClass = route->payloadClass;
			if (route->hops)
			{
				sm_list_destroy(ionwm, route->hops, NULL, NULL);
			}

			psm_free(ionwm, addr);
			sm_list_delete(ionwm, elt, NULL, NULL);
			switch (recomputeRouteForContact(contactToNodeNbr,
					contactFromTime, stationNode,
					currentTime, payloadClass))
			{
			case -1:
				putErrmsg("Route recomputation failed.", NULL);
				return -1;

			case 0:
				break;	/*	Lead contact defunct.	*/

			default:
				/*	Route through this lead contact
				 *	has been recomputed and inserted
				 *	into the list of routes.  Must
				 *	start again from the beginning
				 *	of the list.			*/

				nextElt = sm_list_first(ionwm, routes);
			}

			continue;
		}

		if (route->deliveryTime > deadline)
		{
			/*	No more plausible routes.		*/

			return 0;
		}

		/*	Never route to self unless self is the final
		 *	destination.					*/

		if (route->toNodeNbr == getOwnNodeNbr())
		{
			if (!(bundle->destination.cbhe
			&& bundle->destination.c.nodeNbr == route->toNodeNbr))
			{
				/*	Never route via self -- a loop.	*/
#if CGRDEBUG
puts("Route is via self; ignored.");
#endif
				continue;
			}

			/*	Self is final destination.		*/
		}

		/*	Is the bundle's size greater that the
		 *	capacity of whichever contact in this route
		 *	has the least capacity?  If so, can't use
		 *	this route.					*/

		if (bundle->payload.length > route->maxCapacity)
		{
#if CGRDEBUG
puts("Route includes a contact that's too small for this bundle.");
#endif
			continue;
		}

		/*	Is the neighbor that receives bundles during
		 *	this route's initial contact excluded for any
		 *	reason?						*/

		if (isExcluded(route->toNodeNbr, excludedNodes))
		{
#if CGRDEBUG
puts("First node on route is an excluded neighbor.");
#endif
			continue;
		}

		/*	Consider additional latency imposed by the
		 *	time required to transmit all bytes of the
		 *	bundle.  At each hop of the path, additional
		 *	radiation latency is computed as bundle size
		 *	divided by data rate.				*/

		radiationLatency = 0;
		for (elt2 = sm_list_first(ionwm, route->hops); elt2;
				elt2 = sm_list_next(ionwm, elt2))
		{
			contact = (IonCXref *)
				psp(ionwm, sm_list_data(ionwm, elt2));
			radiationLatency +=
				(bundle->payload.length / contact->xmitRate);
		}

		if (route->deliveryTime + radiationLatency > deadline)
		{
			/*	Contacts are too slow for final
			 *	delivery to occur before bundle
			 *	expiration.  Try another route.		*/
#if CGRDEBUG
puts("Route is too slow; radiation latency delays delivery time too much.");
#endif
			continue;
		}

		/*	Route might work.  If this route is supported
		 *	by contacts with enough aggregate capacity to
		 *	convey this bundle and all currently queued
		 *	bundles of equal or higher priority, then the
		 *	neighbor is a candidate proximate node for
		 *	forwarding the bundle to the station node.	*/

		if (tryRoute(route, currentTime, bundle, plans,
				getDirective, proximateNodes) < 0)
		{
			putErrmsg("Can't check route.", NULL);
			return -1;
		}
	}

	return 0;
}

/*		Functions for routing bundle to appropriate neighbor.	*/

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

static int	enqueueToNeighbor(ProximateNode *proxNode, Bundle *bundle,
			Object bundleObj, IonNode *stationNode)
{
	unsigned int	serviceNbr;
	char		stationEid[64];
	PsmPartition	ionwm;
	PsmAddress	snubElt;
	IonSnub		*snub;
	BpEvent		event;

	if (proxNode->neighborNodeNbr == bundle->destination.c.nodeNbr)
	{
		serviceNbr = bundle->destination.c.serviceNbr;
	}
	else
	{
		serviceNbr = 0;
	}

	isprintf(stationEid, sizeof stationEid, "ipn:" UVAST_FIELDSPEC ".%u",
			proxNode->neighborNodeNbr, serviceNbr);

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

int	cgr_forward(Bundle *bundle, Object bundleObj, uvast stationNodeNbr,
		Object plans, CgrLookupFn getDirective)
{
	IonVdb		*ionvdb = getIonVdb();
	CgrVdb		*cgrvdb = _cgrvdb(NULL);
	IonNode		*stationNode;
	PsmAddress	nextNode;
	int		ionMemIdx;
	Lyst		proximateNodes;
	Lyst		excludedNodes;
	PsmPartition	ionwm = getIonwm();
	PsmAddress	snubElt;
	IonSnub		*snub;
	LystElt		elt;
	LystElt		nextElt;
	ProximateNode	*proxNode;
	Bundle		newBundle;
	Object		newBundleObj;
	ProximateNode	*selectedNeighbor;
	time_t		earliestDeliveryTime = 0;
	int		smallestHopCount = 0;

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

	CHKERR(bundle && bundleObj && stationNodeNbr && plans && getDirective);
#if CGRDEBUG
printf("\n\n\nCGR looking for a route to node " UVAST_FIELDSPEC " for bundle \
of length %u.\n", stationNodeNbr, bundle->payload.length);
#endif
	if (ionvdb->lastEditTime > cgrvdb->lastLoadTime)
	{
		/*	Contact plan has been modified, so must discard
		 *	all route lists and reconstruct them as needed.	*/

		discardRouteLists(cgrvdb);
		cgrvdb->lastLoadTime = getUTCTime();
	}

	stationNode = findNode(ionvdb, stationNodeNbr, &nextNode);
	if (stationNode == NULL)
	{
#if CGRDEBUG
printf("No routing information for node " UVAST_FIELDSPEC ".\n",
stationNodeNbr);
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

	lyst_delete_set(proximateNodes, deleteObject, NULL);
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
			lyst_destroy(proximateNodes);
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

			if (excludeNode(excludedNodes, snub->nodeNbr))
			{
				putErrmsg("Can't note snub.", NULL);
				lyst_destroy(excludedNodes);
				lyst_destroy(proximateNodes);
				return -1;
			}
		}
	}

	/*	Consult the contact graph to identify the neighboring
	 *	node(s) to forward the bundle to.			*/

	if (identifyProximateNodes(stationNode, bundle, bundleObj,
			excludedNodes, plans, getDirective, proximateNodes) < 0)
	{
		putErrmsg("Can't identify proximate nodes for bundle.", NULL);
		lyst_destroy(excludedNodes);
		lyst_destroy(proximateNodes);
		return -1;
	}

	/*	Examine the list of proximate nodes.  If the bundle
	 *	is critical, enqueue it on the outduct to EACH
	 *	identified proximate destination node.
	 *
	 *	Otherwise, enqueue the bundle on the outduct to the
	 *	identified proximate destination for the path with
	 *	the earliest worst-case delivery time.			*/

	lyst_destroy(excludedNodes);
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
			lyst_destroy(proximateNodes);
				return -1;
			}

			MRELEASE(proxNode);
			if (nextElt)
			{
				/*	Every transmission after the
				 *	first must operate on a new
				 *	clone of the original bundle.	*/

				if (bpClone(bundle, &newBundle, &newBundleObj,
							0, 0) < 0)
				{
					putErrmsg("Can't clone bundle.", NULL);
					lyst_destroy(proximateNodes);
					return -1;
				}

				bundle = &newBundle;
				bundleObj = newBundleObj;
			}
		}

		lyst_destroy(proximateNodes);
		return 0;
	}

	/*	Non-critical bundle; send on the minimum-latency path.
	 *	In case of a tie, select the path of minimum hopCount
	 *	from the destination node.				*/

	selectedNeighbor = NULL;
	for (elt = lyst_first(proximateNodes); elt; elt = nextElt)
	{
		nextElt = lyst_next(elt);
		proxNode = (ProximateNode *) lyst_data_set(elt, NULL);
		lyst_delete(elt);
		if (selectedNeighbor == NULL)
		{
			earliestDeliveryTime = proxNode->deliveryTime;
			smallestHopCount = proxNode->hopCount;
			selectedNeighbor = proxNode;
		}
		else if (proxNode->deliveryTime < earliestDeliveryTime)
		{
			earliestDeliveryTime = proxNode->deliveryTime;
			smallestHopCount = proxNode->hopCount;
			MRELEASE(selectedNeighbor);
			selectedNeighbor = proxNode;
		}
		else if (proxNode->deliveryTime == earliestDeliveryTime)
		{
			if (proxNode->hopCount < smallestHopCount)
			{
				smallestHopCount = proxNode->hopCount;
				MRELEASE(selectedNeighbor);
				selectedNeighbor = proxNode;
			}
			else if (proxNode->hopCount == smallestHopCount)
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

	lyst_destroy(proximateNodes);
	if (selectedNeighbor)
	{
#if CGRDEBUG
printf("CGR selected node " UVAST_FIELDSPEC " for next hop.\n",
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
	return 0;
}

void	cgr_start()
{
	char	*name = "cgrvdb";

	oK(_cgrvdb(&name));
}

void	cgr_stop()
{
	PsmPartition	wm = getIonwm();
	char		*name = "cgrvdb";
	PsmAddress	vdbAddress;
	PsmAddress	elt;
	CgrVdb		*vdb;
	char		*stop = NULL;

	/*Clear Route Caches*/
	clearRoutingObjects(wm);

	/*Free volatile database*/
	if (psm_locate(wm, name, &vdbAddress, &elt) < 0)
	{
		putErrmsg("Failed searching for vdb.", NULL);
		return;
	}

	if (elt)
	{
		vdb = (CgrVdb *) psp(wm, vdbAddress);
		sm_list_destroy(wm, vdb->routeLists, NULL, NULL);
		psm_free(wm, vdbAddress);
		if (psm_uncatlg(wm, name) < 0)
		{
			putErrmsg("Failed Uncataloging vdb.",NULL);
		}
	}

	/*Reset pointer*/
	oK(_cgrvdb(&stop));
}
