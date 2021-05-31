/*
	imcfw.c:	scheme-specific forwarder for the "imc"
			scheme, used for Interplanetary Multicast.

	Author: Scott Burleigh, JPL

	Copyright (c) 2012, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
#include "ipnfw.h"
#include "bei.h"
#include "imcfw.h"

typedef struct
{
	uvast	entryNode;
	Lyst	members;
} ImcGang;

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

static sm_SemId		_imcfwSemaphore(sm_SemId *newValue)
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
	sm_SemEnd(_imcfwSemaphore(NULL));
}

/*	*	imcfw clock thread functions	*	*	*	*/

static void	destroyGroup(Object groupElt)
{
	Sdr	sdr = getIonsdr();
	Object	groupAddr;
		OBJ_POINTER(ImcGroup, group);

	groupAddr = sdr_list_data(sdr, groupElt);
	GET_OBJ_POINTER(sdr, ImcGroup, group, groupAddr);
	sdr_list_destroy(sdr, group->members, NULL, NULL);
	sdr_free(sdr, groupAddr);
	sdr_list_delete(sdr, groupElt, NULL, NULL);
}

static void	*imcClock(void *parm)
{
	int		*running = (int *) parm;
	ImcDB		*imcConstants = getImcConstants();
	Sdr		sdr = getIonsdr();
	Object		elt;
	Object		nextElt;
	Object		groupAddr;
	ImcGroup	group;

	/*	Main loop for time-driven IMC functionality.		*/

	iblock(SIGTERM);
	while (*running)
	{
		/*	Destroy unused groups.				*/

		CHKNULL((sdr_begin_xn(sdr)));
		for (elt = sdr_list_first(sdr, imcConstants->groups); elt;
				elt = nextElt)
		{
			nextElt = sdr_list_next(sdr, elt);
			groupAddr = sdr_list_data(sdr, elt);
			sdr_stage(sdr, (char *) &group, groupAddr,
					sizeof(ImcGroup));
			switch (group.secUntilDelete)
			{
			case -1:	/*	Still has members.	*/
				continue;

			case 0:
				destroyGroup(elt);
				continue;

			default:
				group.secUntilDelete--;
				sdr_write(sdr, groupAddr, (char *) &group,
					sizeof(ImcGroup));
			}
		}

		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("imcClock failed.", NULL);
			shutDown(SIGTERM);
			*running = 0;
			continue;
		}

		snooze(1);
	}

	writeErrmsgMemos();
	writeMemo("[i] imcClock thread has ended.");
	return NULL;
}

/*	*	imcfw main thread functions	*	*	*	*/

static int	loadDestination(Bundle *bundle, uvast newNodeNbr)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
	uvast	nodeNbr;

	/*	Ensure no duplication in destinations list.		*/

	for (elt = sdr_list_first(sdr, bundle->destinations); elt;
			elt = sdr_list_next(sdr, elt))
	{
		nodeNbr = sdr_list_data(sdr, elt);
		if (nodeNbr < newNodeNbr)
		{
			continue;
		}

		if (nodeNbr == newNodeNbr)	/*	Duplicate.	*/
		{
			return 0;
		}

		break;
	}

	if (elt)
	{
		if (sdr_list_insert_before(sdr, elt, newNodeNbr) == 0)
		{
			putErrmsg("Can't add node to destinations.", NULL);
			return -1;
		}
	}
	else
	{
		if (sdr_list_insert_last(sdr, bundle->destinations, newNodeNbr)
				== 0)
		{
			putErrmsg("Can't add node to destinations.", NULL);
			return -1;
		}
	}

	return 0;
}

static void	deleteObject(LystElt elt, void *userData)
{
	void	*object = lyst_data(elt);

	if (object)
	{
		MRELEASE(object);
	}
}

static uvast 	getBestEntryNode(Bundle *bundle, IonNode *terminusNode,
			time_t atTime)
{
	IonVdb		*ionvdb = getIonVdb();
	CgrVdb		*cgrvdb = cgr_get_vdb();
	int		ionMemIdx;
	Lyst		bestRoutes;
	Lyst		excludedNodes;
	LystElt		elt;
	CgrRoute	*route;
#if CGR_DEBUG == 1
	CgrTrace	*trace = &(CgrTrace) { .fn = printCgrTraceLine };
#else
	CgrTrace	*trace = NULL;
#endif

	/*	Determine whether or not the contact graph for the
	 *	terminus node identifies one or more routes over
	 *	which the bundle may be sent in order to get it
	 *	delivered to the terminus node.  If so, return the
	 *	number of the entry node of the best route.		*/

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
	if (bestRoutes == NULL)
	{
		putErrmsg("Can't create list for route computation.", NULL);
		return 0;
	}

	lyst_delete_set(bestRoutes, deleteObject, NULL);
	excludedNodes = lyst_create_using(ionMemIdx);
	if (excludedNodes == NULL)
	{
		lyst_destroy(bestRoutes);
		putErrmsg("Can't create lists for route computation.", NULL);
		return 0;
	}

	/*	Must exclude sender of bundle from consideration as
	 *	a station on the route, to minimize routing loops.  	*/

	if (bundle->clDossier.senderNodeNbr != 0
	&& bundle->clDossier.senderNodeNbr != getOwnNodeNbr())
	{
		if (lyst_insert_last(excludedNodes, (void *)
			((uaddr) bundle->clDossier.senderNodeNbr)) == NULL)
		{
			putErrmsg("Can't exclude sender from routes.", NULL);
			lyst_destroy(excludedNodes);
			lyst_destroy(bestRoutes);
			return 0;
		}
	}

	/*	Consult the contact graph to identify the neighboring
	 *	node(s) to forward the bundle to.			*/

	if (terminusNode->routingObject == 0)
	{
		if (cgr_create_routing_object(terminusNode) < 0)
		{
			putErrmsg("Can't initialize routing object.", NULL);
			lyst_destroy(excludedNodes);
			lyst_destroy(bestRoutes);
			return 0;
		}
	}

	if (cgr_identify_best_routes(terminusNode, bundle, excludedNodes,
			atTime, NULL, trace, bestRoutes) < 0)
	{
		putErrmsg("Can't identify best route(s) for bundle.", NULL);
		lyst_destroy(excludedNodes);
		lyst_destroy(bestRoutes);
		return 0;
	}

	lyst_destroy(excludedNodes);
	elt = lyst_first(bestRoutes);
	if (elt)
	{
		route = (CgrRoute *) lyst_data_set(elt, NULL);
		lyst_destroy(bestRoutes);
#if IMCDEBUG
printf("Computed best route to " UVAST_FIELDSPEC " begins with transmission to " UVAST_FIELDSPEC ".\n", terminusNode->nodeNbr, route->toNodeNbr);
#endif
		return route->toNodeNbr;
	}

	lyst_destroy(bestRoutes);
	return 0;
}

static uvast	getViaNode(Bundle *bundle, uvast destinationNodeNbr)
{
	Sdr		sdr = getIonsdr();
	IonVdb		*ionvdb = getIonVdb();
	IonNode		*node;
	PsmAddress	nextNode;
	uvast		viaNodeNbr;
	char		eid[MAX_EID_LEN + 1];
	VPlan		*vplan;
	PsmAddress	vplanElt;
	BpPlan		plan;

	node = findNode(ionvdb, destinationNodeNbr, &nextNode);
	if (node == NULL)
	{
		node = addNode(ionvdb, destinationNodeNbr);
		if (node == NULL)
		{
			putErrmsg("Can't add node.", NULL);
			return -1;
		}
	}

	viaNodeNbr = getBestEntryNode(bundle, node, getCtime());
	if (viaNodeNbr)
	{
		return viaNodeNbr;
	}

	/*	No luck using the contact graph to compute a route
	 *	to the destination node, so see if destination node
	 *	is a neighbor (not identified in the contact plan);
	 *	if so, direct transmission works.			*/

	isprintf(eid, sizeof eid, "ipn:" UVAST_FIELDSPEC ".0",
			destinationNodeNbr);
	findPlan(eid, &vplan, &vplanElt);
	if (vplanElt == 0)
	{
		return 0;
	}

	sdr_read(sdr, (char *) &plan, sdr_list_data(sdr, vplan->planElt),
			sizeof(BpPlan));
	if (plan.blocked)
	{
		return 0;
	}

	return destinationNodeNbr;
}

static void	deleteGang(LystElt elt, void *userData)
{
	ImcGang	*gang = (ImcGang *) lyst_data(elt);

	lyst_destroy(gang->members);
	MRELEASE(gang);
}

static int	addNodeToGang(Lyst gangs, uvast viaNode, uvast nodeNbr)
{
	LystElt	elt;
	ImcGang	*gang;

	for (elt = lyst_first(gangs); elt; elt = lyst_next(elt))
	{
		gang = (ImcGang *) lyst_data(elt);
		if (gang->entryNode < viaNode)
		{
			continue;
		}

		if (gang->entryNode == viaNode)
		{
			/*	Join this gang.				*/

			if (lyst_insert_last(gang->members,
					(void *) ((uaddr) nodeNbr)) == NULL)
			{
				return -1;
			}

			return 0;
		}

		/*	Requisite gang not found.			*/

		break;
	}

	/*	Must create new gang.					*/

	gang = MTAKE(sizeof(ImcGang));
	if (gang == NULL)
	{
		return -1;
	}

	gang->entryNode = viaNode;
	gang->members = lyst_create_using(getIonMemoryMgr());
	if (gang->members == NULL)
	{
		return -1;
	}

	if (elt)
	{
		if (lyst_insert_before(elt, gang) == NULL)
		{
			return -1;
		}
	}
	else
	{
		if (lyst_insert_last(gangs, gang) == NULL)
		{
			return -1;
		}
	}

	/*	Now have got gang that this node can join.		*/

	if (lyst_insert_last(gang->members, (void *) ((uaddr) nodeNbr)) == NULL)
	{
		return -1;
	}

	return 0;
}

static int	enqueueToNeighbor(Bundle *bundle, Object bundleObj,
			uvast nodeNbr)
{
	char		eid[MAX_EID_LEN + 1];
	VPlan		*vplan;
	PsmAddress	vplanElt;

	isprintf(eid, sizeof eid, "ipn:" UVAST_FIELDSPEC ".0", nodeNbr);
#if IMCDEBUG
printf("Preparing to send to neighbor '%s'.\n", eid);
#endif
	findPlan(eid, &vplan, &vplanElt);
	if (vplanElt == 0)
	{
		return 0;
	}

#if IMCDEBUG
puts("Sending to neighbor.");
#endif
	if (bpEnqueue(vplan, bundle, bundleObj) < 0)
	{
		putErrmsg("Can't enqueue bundle.", NULL);
		return -1;
	}

	return 0;
}

static int	enqueueBundle(Bundle *bundle, Object bundleObj, uvast nodeNbr)
{
	/*	Entry node for Gang must be a neighbor.			*/

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

	/*	No plan for conveying bundle to this neighbor, so
	 *	must give up on forwarding it.				*/

#if IMCDEBUG
printf("enqueueBundle to node" UVAST_FIELDSPEC ".\n", nodeNbr);
#endif
	return bpAbandon(bundleObj, bundle, BP_REASON_NO_ROUTE);
}

static int	forwardImcBundle(Bundle *bundle, Object bundleAddr)
{
	Sdr		sdr = getIonsdr();
	unsigned int	memmgr = getIonMemoryMgr();
	uvast		ownNodeNbr = getOwnNodeNbr();
	Lyst		gangs;
	Object		elt;
	uvast		nodeNbr;
	int		regionIdx;
	uint32_t	regionNbr;
	uvast		viaNode = 0;
	LystElt		elt2;
	ImcGang		*gang;
	Bundle		newBundle;
	Object		newBundleObj;
	LystElt		elt3;

	gangs = lyst_create_using(memmgr);
	if (gangs == NULL)
	{
		putErrmsg("Can't create list of Gangs for CGR multicast.",
				NULL);
		return -1;
	}

	lyst_delete_set(gangs, deleteGang, NULL);

	/*	First, divide all of the bundle's destinations into
	 *	gangs.  Each gang is characterized by the entry node
	 *	number that is common to the best routes for
	 *	forwarding the bundle to all members of the gang.	*/

	for (elt = sdr_list_first(sdr, bundle->destinations); elt;
			elt = sdr_list_next(sdr, elt))
	{
		nodeNbr = sdr_list_data(sdr, elt);
#if IMCDEBUG
printf("Outbound destination is " UVAST_FIELDSPEC ".\n", nodeNbr);
#endif
		regionIdx = ionRegionOf(nodeNbr, ownNodeNbr, &regionNbr);
		if (regionIdx < 0)
		{
			/*	Some other node will be forwarding
			 *	the bundle to this destination node,
			 *	or else it's impossble to forward
			 *	the bundle to this destination node.	*/
#if IMCDEBUG
puts("No common region.");
#endif
			continue;
		}

		viaNode = getViaNode(bundle, nodeNbr);
		if (viaNode == 0)
		{
			/*	No way to get the bundle to this
			 *	destination.				*/
#if IMCDEBUG
puts("No via node.");
#endif
			continue;
		}

		/*	Add this node to the gang headed by this
		 *	viaNode.					*/

		if (addNodeToGang(gangs, viaNode, nodeNbr) < 0)
		{
			putErrmsg("Can't add node to gang.", NULL);
			lyst_destroy(gangs);
			return -1;
		}
	}

	/*	Then, for each gang, clone the bundle and set the
	 *	destinations list of the clone to all and only the
	 *	members of the gang, then enqueue the clone for
	 *	transmission to the gang's common entry node.		*/

	for (elt2 = lyst_first(gangs); elt2; elt2 = lyst_next(elt2))
	{
		gang = (ImcGang *) lyst_data(elt2);
		if (bpClone(bundle, &newBundle, &newBundleObj, 0, 0) < 0)
		{
			putErrmsg("Failed on clone.", NULL);
			lyst_destroy(gangs);
			return -1;
		}

		/*	Erase clone's original destinations list.	*/

		while ((elt = sdr_list_first(sdr, newBundle.destinations)))
		{
			sdr_list_delete(sdr, elt, NULL, NULL);
		}

		/*	Insert all new destinations.			*/

		for (elt3 = lyst_first(gang->members); elt3;
				elt3 = lyst_next(elt3))
		{
			nodeNbr = (uaddr) lyst_data(elt3);
#if IMCDEBUG
printf("Loading destination " UVAST_FIELDSPEC ".\n", nodeNbr);
#endif
			if (loadDestination(&newBundle, nodeNbr) < 0)
			{
				putErrmsg("Failed loading destination.", NULL);
				lyst_destroy(gangs);
				return -1;
			}
		}

		/*	Finally, enqueue the new bundle for xmit.	*/
#if IMCDEBUG
printf("Gang bundle sent to " UVAST_FIELDSPEC " has %lu members.\n", gang->entryNode, lyst_length(gang->members));
#endif
		if (enqueueBundle(&newBundle, newBundleObj, gang->entryNode)
				< 0)
		{
			putErrmsg("Failed on enqueue.", NULL);
			lyst_destroy(gangs);
			return -1;
		}
	}

	/*	Destroy gangs list and originally received multicast
	 *	bundle.							*/

	lyst_destroy(gangs);
	return bpDestroyBundle(bundleAddr, 2);
}

static int	relayImcBundle(Bundle *bundle, Object bundleAddr,
			ExtensionBlock *imcblock, Object imcblkAddr)
{
	Sdr		sdr = getIonsdr();
	uvast		ownNodeNbr = getOwnNodeNbr();
	int		destinationsCount;
	char		*nodeNbrsArray;
	int		i;
	uvast		*nodeNbrPtr;

	/*	Load the bundle's list of destinations from the
	 *	array of gang members in the bundle's IMC extension
	 *	block, except for self.					*/

	destinationsCount = imcblock->size / sizeof(uvast);
	if (destinationsCount < 1)
	{
		writeMemo("[?] IMC block has no destinations.");
#if IMCDEBUG
puts("no destinations");
#endif
		oK(bpAbandon(bundleAddr, bundle, BP_REASON_NO_ROUTE));
		return 0;
	}

	nodeNbrsArray = MTAKE(imcblock->size);
	if (nodeNbrsArray == NULL)
	{
		putErrmsg("Can't read node numbers array.",
				itoa(destinationsCount));
		return -1;
	}

	sdr_read(sdr, nodeNbrsArray, imcblock->object, imcblock->size);
	nodeNbrPtr = (uvast *) nodeNbrsArray;
	for (i = 0; i < destinationsCount; i++, nodeNbrPtr++)
	{
		if (*nodeNbrPtr == ownNodeNbr)
		{
			/*	Omit self from destinations list.	*/

			continue;
		}

		/*	Load this destination into the bundle.		*/

		if (loadDestination(bundle, *nodeNbrPtr) < 0)
		{
			MRELEASE(nodeNbrsArray);
			putErrmsg("Can't load from IMC extension block.", NULL);
			return -1;
		}
	}

	MRELEASE(nodeNbrsArray);

	/*	Reinitialize the IMC extension block.			*/

	bundle->extensionsLength -= imcblock->length;
	sdr_write(sdr, bundleAddr, (char *) bundle, sizeof(Bundle));
	sdr_free(sdr, imcblock->bytes);
	imcblock->bytes = 0;
	imcblock->length = 0;
	sdr_free(sdr, imcblock->object);
	imcblock->object = 0;
	imcblock->size = 1;
	sdr_write(sdr, imcblkAddr, (char *) imcblock, sizeof(ExtensionBlock));

	/*	Don't forward the bundle if there are no remaining
	 *	destinations.						*/

	if (sdr_list_length(sdr, bundle->destinations) == 0)
	{
		/*	No remaining gang members to forward to.	*/

		return bpDestroyBundle(bundleAddr, 2);
	}

	/*	Forward the bundle.					*/

	return forwardImcBundle(bundle, bundleAddr);
}

static int	loadRegionMembers(Bundle *bundle, uint32_t regionNbr, IonDB *db)
{
	Sdr		sdr = getIonsdr();
	Object		elt;
	Object		memberAddr;
	RegionMember	member;

	for (elt = sdr_list_first(sdr, db->rolodex); elt;
			elt = sdr_list_next(sdr, elt))
	{
		memberAddr = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &member, memberAddr,
				sizeof(RegionMember));
		if (member.homeRegionNbr == regionNbr
		|| member.outerRegionNbr == regionNbr)
		{
			if (loadDestination(bundle, member.nodeNbr) < 0)
			{
				putErrmsg("Can't add region member.", NULL);
				return -1;
			}
		}
	}

	return 0;
}

static int	originateImcBundle(Bundle *bundle, Object bundleAddr)
{
	Sdr		sdr = getIonsdr();
	Object		iondbObj;
	IonDB		iondb;
	uint32_t	regionNbr;
	int		regionIdx;
	uvast		groupNbr;
	Object		groupAddr;
	Object		groupElt;
	ImcGroup	group;
	Object		elt;
	uvast		nodeNbr;

	groupNbr = bundle->destination.ssp.imc.groupNbr;

	/*	Load the bundle's list of destinations, either from
	 *	region membership (for a petition) or from group
	 *	membership (for an application multicast message).	*/

	if (groupNbr == 0)	/*	Broadcast to region members.	*/
	{

		regionNbr = bundle->ancillaryData.imcRegionNbr;
		iondbObj = getIonDbObject();
		sdr_read(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
		if (regionNbr == 0)	/*	Fwd in both regions.	*/
		{
			/*	Send to all members of both home
			 *	region and (if any) outer region.	*/

			if (loadRegionMembers(bundle,
					iondb.regions[0].regionNbr, &iondb) < 0
			|| loadRegionMembers(bundle,
					iondb.regions[1].regionNbr, &iondb) < 0)
			{
				putErrmsg("Can't add IMC region member.", NULL);
				return -1;
			}
		}
		else			/*	Fwd within this region.	*/
		{
			/*	Send only to all members of the
			 *	specified region.			*/

			regionIdx = ionPickRegion(regionNbr);
			if (regionIdx < 0)
			{
				putErrmsg("IMC system error.", NULL);
				return -1;
			}

			if (loadRegionMembers(bundle, 
				iondb.regions[regionIdx].regionNbr, &iondb) < 0)
			{
				putErrmsg("Can't add IMC region member.", NULL);
				return -1;
			}
		}
	}
	else			/*	Multicast to group members.	*/
	{
		imcFindGroup(groupNbr, &groupAddr, &groupElt);
		if (groupElt == 0)
		{
			/*	Nobody subscribes to bundles destined
			 *	for this group.				*/
#if IMCDEBUG
puts("no such group");
#endif
			oK(bpAbandon(bundleAddr, bundle, BP_REASON_NO_ROUTE));
			return 0;
		}

		/*	Multicast bundle to all members of this group.	*/

		sdr_read(sdr, (char *) &group, groupAddr, sizeof(ImcGroup));
		for (elt = sdr_list_first(sdr, group.members); elt;
				elt = sdr_list_next(sdr, elt))
		{
			nodeNbr = sdr_list_data(sdr, elt);
			if (loadDestination(bundle, nodeNbr) < 0)
			{
				putErrmsg("Can't add IMC group member.", NULL);
				return -1;
			}
		}
	}

	/*	Forward the bundle.					*/

	return forwardImcBundle(bundle, bundleAddr);
}

#if defined (ION_LWT)
int	imcfw(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
#else
int	main(int argc, char *argv[])
{
#endif
	int		running = 1;
	Sdr		sdr;
	uvast		ownNodeNbr;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	Scheme		scheme;
	pthread_t	clockThread;
	Object		elt;
	Object		bundleAddr;
	Bundle		bundle;
	Object		imcblkElt;
	Object		imcblkAddr;
	ExtensionBlock	imcblock;

	if (bpAttach() < 0)
	{
		putErrmsg("imcfw can't attach to BP.", NULL);
		return 1;
	}

	sdr = getIonsdr();
	if (imcInit() < 0)
	{
		putErrmsg("imcfw can't load IMC routing database.", NULL);
		return 1;
	}

	ownNodeNbr = getOwnNodeNbr();
	findScheme("imc", &vscheme, &vschemeElt);
	if (vschemeElt == 0)
	{
		putErrmsg("'imc' scheme is unknown.", NULL);
		return 1;
	}

	sdr_read(sdr, (char *) &scheme, sdr_list_data(sdr,
			vscheme->schemeElt), sizeof(Scheme));

	/*	Set up signal handling.  SIGTERM is shutdown signal.	*/

	oK(_imcfwSemaphore(&vscheme->semaphore));
	isignal(SIGTERM, shutDown);

	/*	Start the clock thread, for deleting empty multicast
	 *	groups and filling in new nodes on current multicast
	 *	group membership.					*/

	if (pthread_begin(&clockThread, NULL, imcClock, &running, "imcClock"))
	{
		putSysErrmsg("imcfw can't create clock thread", NULL);
		return 1;
	}

	/*	Main loop: wait until forwarding queue is non-empty,
	 *	then drain it.						*/

	writeMemo("[i] imcfw is running.");
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
		bundle.priority = bundle.classOfService;
		bundle.ordinal = bundle.ancillaryData.ordinal;
		bundle.qosFlags = bundle.ancillaryData.flags;

		/*	No override mechanism at this time.		*/

		sdr_list_delete(sdr, elt, NULL, NULL);
		bundle.fwdQueueElt = 0;

		/*	Clear the bundle's imc destinations list.	*/

		while ((elt = sdr_list_first(sdr, bundle.destinations)))
		{
			sdr_list_delete(sdr, elt, NULL, NULL);
		}

		/*	Must rewrite bundle to note removal of
		 *	fwdQueueElt, in case the bundle is abandoned
		 *	and bpDestroyBundle re-reads it from the
		 *	database.					*/

		sdr_write(sdr, bundleAddr, (char *) &bundle, sizeof(Bundle));

		/*	Is bundle being relayed or sourced?		*/

		imcblkElt = findExtensionBlock(&bundle, ImcDestinationsBlk, 0);
		if (imcblkElt == 0)
		{
			writeMemo("[?] IMC extension block is missing.");
#if IMCDEBUG
puts("IMC extension block missing");
#endif
			oK(bpAbandon(bundleAddr, &bundle, BP_REASON_NO_ROUTE));
			continue;
		}

		imcblkAddr = sdr_list_data(sdr, imcblkElt);
		sdr_stage(sdr, (char *) &imcblock, imcblkAddr,
				sizeof(ExtensionBlock));
		if (imcblock.object == 0)
		{
			/*	Bundle was never previously forwarded.	*/

			if (originateImcBundle(&bundle, bundleAddr) < 0)
			{
				putErrmsg("Can't source IMC bundle.", NULL);
				sdr_cancel_xn(sdr);
				running = 0;
				continue;
			}
		}
		else	/*	Received from some node, possibly self.	*/
		{
			if (bundle.id.source.ssp.ipn.nodeNbr == ownNodeNbr)
			{
				if (bundle.clDossier.senderNodeNbr == 0)
				{
					/*	Received from unknown
					 *	node, can't safely
					 *	relay the bundle.	*/
#if IMCDEBUG
puts("received from unknown node");
#endif
					oK(bpAbandon(bundleAddr, &bundle,
						BP_REASON_NO_ROUTE));
				}
				else
				{
					/*	Bundle has been
					 *	locally delivered,
					 *	must now be destroyed:
					 *	it was either received
					 *	via loopback, in which
					 *	case no need to relay
				 	*	(self can't be on
					*	CGR path to any other
					*	node) or received
				 	*	from some other node,
					*	in which case relaying
					*	would introduce a
					*	routing loop.		*/

					oK(bpDestroyBundle(bundleAddr, 2));
				}
			}
			else	/*	Bundle sourced by another node.	*/
			{
				if (relayImcBundle(&bundle, bundleAddr,
						&imcblock, imcblkAddr) < 0)
				{
					putErrmsg("Can't relay IMC bundle.",
							NULL);
					sdr_cancel_xn(sdr);
					running = 0;
					continue;
				}
			}
		}

		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Can't forward IMC bundle.", NULL);
			sdr_cancel_xn(sdr);
			running = 0;
			continue;
		}

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();
	}

	/*	Stop the clock thread.					*/

	running = 0;
	pthread_join(clockThread, NULL);

	/*	Wrap up.						*/

	writeErrmsgMemos();
	writeMemo("[i] imcfw forwarder has ended.");
	ionDetach();
	return 0;
}
