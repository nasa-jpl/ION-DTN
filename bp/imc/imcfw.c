/*
	imcfw.c:	scheme-specific forwarder for the "imc"
			scheme, used for Interplanetary Multicast.

	Author: Scott Burleigh, JPL

	Copyright (c) 2012, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
#include "ipnfw.h"
#include "imcP.h"

static sm_SemId		_imcfwSemaphore(sm_SemId *newValue)
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
	sm_SemEnd(_imcfwSemaphore(NULL));
}

static int	enqueueToNeighbor(Bundle *bundle, Object bundleObj,
			uvast nodeNbr)
{
	FwdDirective	directive;
	char		stationEid[64];
	IonNode		*stationNode;
	PsmAddress	nextElt;
	PsmPartition	ionwm;
	PsmAddress	snubElt;
	IonSnub		*snub;

	if (ipn_lookupPlanDirective(nodeNbr, 0, 0, &directive) == 0)
	{
		return 0;
	}

	/*	The station node is a neighbor.				*/

	isprintf(stationEid, sizeof stationEid, "ipn:" UVAST_FIELDSPEC ".0",
			nodeNbr);

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

static int	enqueueBundle(Bundle *bundle, Object bundleObj, uvast nodeNbr)
{
	/*	Note that the only way we can prevent multicast
	 *	forwarding loops is by knowing exactly which
	 *	neighboring node sent each bundle, so that we
	 *	can be sure not to echo it back to that node.
	 *	This means that every relative in the IMC database
	 *	must also be a neighbor.  The only way to guarantee
	 *	this is to use only direct transmission plans
	 *	(in the IPN routing database) for forwarding
	 *	bundles; CGR and static routing groups are of
	 *	no use.							*/

	if (enqueueToNeighbor(bundle, bundleObj, nodeNbr) < 0)
	{
		putErrmsg("Can't send bundle to neighbor.", NULL);
		return -1;
	}

	if (bundle->ductXmitElt)
	{
		/*	Enqueued.					*/

		return bpAccept(bundleObj, bundle);
	}

	/*	No plan for conveying bundle to this neighbor, so
	 *	must give up on forwarding it.				*/

	return bpAbandon(bundleObj, bundle);
}

#if defined (VXWORKS) || defined (RTEMS) || defined (bionic)
int	imcfw(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
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
	Object		elt;
	Object		bundleAddr;
	Bundle		bundle;
	int		copiesForwarded;
	Object		elt2;
	Object		elt3;
	Object		groupAddr;
	ImcGroup	group;
			OBJ_POINTER(NodeId, member);
	Bundle		newBundle;
	Object		newBundleObj;

	if (bpAttach() < 0)
	{
		putErrmsg("imcfw can't attach to BP.", NULL);
		return 1;
	}

	if (imcInit() < 0)
	{
		putErrmsg("imcfw can't load IMC routing database.", NULL);
		return 1;
	}

	if (ipnInit() < 0)
	{
		putErrmsg("imcfw can't load IPN routing database.", NULL);
		return 1;
	}

	sdr = getIonsdr();
	ownNodeNbr = getOwnNodeNbr();
	findScheme("imc", &vscheme, &vschemeElt);
	if (vschemeElt == 0)
	{
		putErrmsg("'imc' scheme is unknown.", NULL);
		return 1;
	}

	CHKZERO(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &scheme, sdr_list_data(sdr,
			vscheme->schemeElt), sizeof(Scheme));
	sdr_exit_xn(sdr);
	oK(_imcfwSemaphore(&vscheme->semaphore));
	isignal(SIGTERM, shutDown);

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
		sdr_list_delete(sdr, elt, NULL, NULL);
		bundle.fwdQueueElt = 0;

		/*	Must rewrite bundle to note removal of
		 *	fwdQueueElt, in case the bundle is abandoned
		 *	and bpDestroyBundle re-reads it from the
		 *	database.					*/

		sdr_write(sdr, bundleAddr, (char *) &bundle, sizeof(Bundle));
		copiesForwarded = 0;
		imcFindGroup(bundle.destination.c.nodeNbr, &groupAddr, &elt2);
		if (elt2 == 0)
		{
			/*	Nobody subscribes to bundles destined
			 *	for this group.				*/

			oK(bpAbandon(bundleAddr, &bundle));
		}
		else
		{
			if (bundle.clDossier.senderNodeNbr == 0
			&& bundle.id.source.c.nodeNbr != ownNodeNbr)
			{
				/*	Received from unknown node,
				 *	can't safely forward bundle.	*/

				oK(bpAbandon(bundleAddr, &bundle));
			}
			else
			{
				sdr_read(sdr, (char *) &group, groupAddr,
						sizeof(ImcGroup));
				for (elt3 = sdr_list_first(sdr, group.members);
					elt3; elt3 = sdr_list_next(sdr, elt3))
				{
					GET_OBJ_POINTER(sdr, NodeId, member,
						sdr_list_data(sdr, elt3));
					if (member->nbr ==
						bundle.clDossier.senderNodeNbr)
					{
						continue;
					}
	
					if (bpClone(&bundle, &newBundle,
						&newBundleObj, 0, 0) < 0)
					{
						putErrmsg("Failed on clone.",
							NULL);
						running = 0;
						break;	/*	Loop.	*/
					}

					if (enqueueBundle(&newBundle,
						newBundleObj, member->nbr) < 0)
					{
						sdr_cancel_xn(sdr);
						putErrmsg("Failed on enqueue.",
							NULL);
						running = 0;
						break;	/*	Loop.	*/
					}

					copiesForwarded++;
				}

				if (copiesForwarded == 0)
				{
					oK(bpAbandon(bundleAddr, &bundle));
				}
				else	/*	Destroy unused copy.	*/
				{
					oK(bpDestroyBundle(bundleAddr, 1));
				}
			}
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
	writeMemo("[i] imcfw forwarder has ended.");
	ionDetach();
	return 0;
}
