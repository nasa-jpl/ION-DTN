/*
	ipnfw.c:	scheme-specific forwarder for the "ipn"
			scheme, used for Interplanetary Internet.

	Author: Scott Burleigh, JPL

	Copyright (c) 2006, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
#include "ipnfw.h"

static sm_SemId		_ipnfwSemaphore(sm_SemId *newValue)
{
	long		temp;
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

	temp = (long) value;
	sem = temp;
	return sem;
}

static void	shutDown()	/*	Commands forwarder termination.	*/
{
	isignal(SIGTERM, shutDown);
	sm_SemEnd(_ipnfwSemaphore(NULL));
}

static int	getDirective(uvast nodeNbr, Object plans, Bundle *bundle,
			FwdDirective *directive)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
	Object	planAddr;
	IpnPlan plan;

	for (elt = sdr_list_first(sdr, plans); elt;
			elt = sdr_list_next(sdr, elt))
	{
		planAddr = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &plan, planAddr, sizeof(IpnPlan));
		if (plan.nodeNbr < nodeNbr)
		{
			continue;
		}
		
		if (plan.nodeNbr > nodeNbr)
		{
			return 0;	/*	Same as end of list.	*/
		}

		memcpy((char *) directive, (char *) &plan.defaultDirective,
				sizeof(FwdDirective));
		return 1;
	}

	return 0;
}

static int	enqueueToNeighbor(Bundle *bundle, Object bundleObj,
			uvast nodeNbr, unsigned int serviceNbr)
{
	FwdDirective	directive;
	char		stationEid[64];
	IonNode		*stationNode;
	PsmAddress	nextElt;
	PsmPartition	ionwm;
	PsmAddress	snubElt;
	IonSnub		*snub;

	if (ipn_lookupPlanDirective(nodeNbr, bundle->id.source.c.serviceNbr, 
			bundle->id.source.c.nodeNbr, &directive) == 0)
	{
		return 0;
	}

	/*	The station node is a neighbor.				*/

	isprintf(stationEid, sizeof stationEid, "ipn:" UVAST_FIELDSPEC ".%u",
			nodeNbr, serviceNbr);

	/*	Is neighbor refusing to be a station for bundles?	*/

	stationNode = findNode(getIonVdb(), nodeNbr, &nextElt);
	if (stationNode)
	{
		ionwm = getIonwm();
		for (snubElt = sm_list_first(ionwm, stationNode->snubs);
				snubElt; snubElt = sm_list_next(ionwm, snubElt))
		{
			snub = (IonSnub *) psp(ionwm, sm_list_data(ionwm,
					snubElt));
			if (snub->nodeNbr < nodeNbr)
			{
				continue;
			}

			if (snub->nodeNbr > nodeNbr)
			{
				break;	/*	Not refusing bundles.	*/
			}

			/*	Neighbor is refusing bundles.  A
			 *	neighbor, but not a good neighbor;
			 *	give up.				*/

			return 0;
		}
	}

	if (bpEnqueue(&directive, bundle, bundleObj, stationEid) < 0)
	{
		putErrmsg("Can't enqueue bundle.", NULL);
		return -1;
	}

	return 0;
}

static int	blockedOutductsCount()
{
	Sdr	sdr = getIonsdr();
	BpDB	*db = getBpConstants();
	int	count = 0;
	Object	elt;
		OBJ_POINTER(ClProtocol, protocol);
	Object	elt2;
		OBJ_POINTER(Outduct, duct);

	for (elt = sdr_list_first(sdr, db->protocols); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, ClProtocol, protocol,
			       	sdr_list_data(sdr, elt));
		for (elt2 = sdr_list_first(sdr, protocol->outducts); elt2;
				elt2 = sdr_list_next(sdr, elt2))
		{
			GET_OBJ_POINTER(sdr, Outduct, duct,
			       		sdr_list_data(sdr, elt2));
			if (duct->blocked)
			{
				count++;
			}
		}
	}

	return count;
}

static int	enqueueBundle(Bundle *bundle, Object bundleObj)
{
	Sdr		sdr = getIonsdr();
	Object		elt;
	char		eidString[SDRSTRING_BUFSZ];
	MetaEid		metaEid;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	FwdDirective	directive;

	elt = sdr_list_first(sdr, bundle->stations);
	if (elt == 0)
	{
		putErrmsg("Forwarding error; stations stack is empty.", NULL);
		return -1;
	}

	sdr_string_read(sdr, eidString, sdr_list_data(sdr, elt));
	if (parseEidString(eidString, &metaEid, &vscheme, &vschemeElt) == 0)
	{
		putErrmsg("Can't parse node EID string.", eidString);
		return bpAbandon(bundleObj, bundle);
	}

	if (strcmp(vscheme->name, "ipn") != 0)
	{
		putErrmsg("Forwarding error; EID scheme is not 'ipn'.",
				vscheme->name);
		return -1;
	}

	if (cgr_forward(bundle, bundleObj, metaEid.nodeNbr,
			(getIpnConstants())->plans, getDirective) < 0)
	{
		putErrmsg("CGR failed.", NULL);
		return -1;
	}

	/*	If dynamic routing succeeded in enqueuing the bundle
	 *	to a neighbor, accept the bundle and return.		*/

	if (bundle->ductXmitElt)
	{
		/*	Enqueued.					*/

		return bpAccept(bundleObj, bundle);
	}

	/*	No luck using the contact graph to compute a route
	 *	to the destination node.  So see if destination node
	 *	is a neighbor; if so, enqueue for direct transmission.	*/

	if (enqueueToNeighbor(bundle, bundleObj, metaEid.nodeNbr,
			metaEid.serviceNbr) < 0)
	{
		putErrmsg("Can't send bundle to neighbor.", NULL);
		return -1;
	}

	if (bundle->ductXmitElt)
	{
		/*	Enqueued.					*/

		return bpAccept(bundleObj, bundle);
	}

	/*	Destination isn't a neighbor that accepts bundles.
	 *	So look for the narrowest applicable static route
	 *	(node range, i.e., "group") and forward to the
	 *	prescribed "via" endpoint for that group.		*/

	if (ipn_lookupGroupDirective(metaEid.nodeNbr,
			bundle->id.source.c.serviceNbr, 
			bundle->id.source.c.nodeNbr, &directive) == 1)
	{
		/*	Found directive; forward via the indicated
		 *	endpoint.					*/

		sdr_write(sdr, bundleObj, (char *) &bundle, sizeof(Bundle));
		sdr_string_read(sdr, eidString, directive.eid);
		return forwardBundle(bundleObj, bundle, eidString);
	}

	/*	No applicable group.  If there's at least one blocked
	 *	outduct, future outduct unblocking might enable CGR
	 *	to compute a route that's not currently plausible.
	 *	So place bundle in limbo.				*/

	if (blockedOutductsCount() > 0)
	{
		if (enqueueToLimbo(bundle, bundleObj) < 0)
		{
			putErrmsg("Can't put bundle in limbo.", NULL);
			return -1;
		}
	}

	if (bundle->ductXmitElt)
	{
		/*	Enqueued to limbo.				*/

		return bpAccept(bundleObj, bundle);
	}
	else
	{
		return bpAbandon(bundleObj, bundle);
	}
}

#if defined (VXWORKS) || defined (RTEMS) || defined (bionic)
int	ipnfw(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
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
