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

static int	deliverAtSource(Object bundleObj, Bundle *bundle)
{
	char		nss[64];
	VEndpoint	*vpoint;
	PsmAddress	vpointElt;
	Bundle		newBundle;
	Object		newBundleObj;

	isprintf(nss, sizeof nss, UVAST_FIELDSPEC ".%d",
			bundle->destination.c.nodeNbr,
			bundle->destination.c.serviceNbr);
	findEndpoint("imc", nss, NULL, &vpoint, &vpointElt);
	if (vpoint == NULL)
	{
		return 0;
	}

	if (bpClone(bundle, &newBundle, &newBundleObj, 0, 0) < 0)
	{
		putErrmsg("Failed on clone.", NULL);
		return -1;
	}

	if (deliverBundle(newBundleObj, &newBundle, vpoint) < 0)
	{
		putErrmsg("Bundle delivery failed.", NULL);
		return -1;
	}

	if ((getBpVdb())->watching & WATCH_z)
	{
		putchar('z');
		fflush(stdout);
	}

	/*	Authorize destruction of clone in case
	 *	deliverBundle didn't queue it for delivery.		*/

	oK(bpDestroyBundle(newBundleObj, 0));
	return 0;
}

static int	enqueueToNeighbor(Bundle *bundle, Object bundleObj,
			uvast nodeNbr)
{
	char		eid[MAX_EID_LEN + 1];
	VPlan		*vplan;
	PsmAddress	vplanElt;

	isprintf(eid, sizeof eid, "ipn:" UVAST_FIELDSPEC ".0", nodeNbr);
	findPlan(eid, &vplan, &vplanElt);
	if (vplanElt == 0)
	{
		return 0;
	}

	if (bpEnqueue(vplan, bundle, bundleObj) < 0)
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
	 *	bundles; CGR and static routing exits are of no use.	*/

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

	return bpAbandon(bundleObj, bundle, BP_REASON_NO_ROUTE);
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

			oK(bpAbandon(bundleAddr, &bundle, BP_REASON_NO_ROUTE));
		}
		else
		{
			if (bundle.clDossier.senderNodeNbr == 0
			&& bundle.id.source.c.nodeNbr != ownNodeNbr)
			{
				/*	Received from unknown node,
				 *	can't safely forward bundle.	*/

				oK(bpAbandon(bundleAddr, &bundle,
						BP_REASON_NO_ROUTE));
			}
			else
			{
				sdr_read(sdr, (char *) &group, groupAddr,
						sizeof(ImcGroup));
				if (group.isMember
				&& bundle.clDossier.senderNodeNbr == 0
				&& bundle.id.source.c.nodeNbr == ownNodeNbr)
				{
					/*	Must deliver locally.	*/
					if (deliverAtSource(bundleAddr,
							&bundle) < 0)
					{
						putErrmsg("Source delivery NG.",
								NULL);
						running = 0;
						break;	/*	Loop.	*/
					}
				}

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
					oK(bpAbandon(bundleAddr, &bundle,
							BP_REASON_NO_ROUTE));
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
