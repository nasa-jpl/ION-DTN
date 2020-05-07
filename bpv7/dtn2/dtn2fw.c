/*
	dtn2fw.c:	scheme-specific forwarder for the "dtn2"
			scheme.

	Author: Scott Burleigh, JPL

	Copyright (c) 2006, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
#include "dtn2fw.h"

static sm_SemId	_dtn2fwSemaphore(sm_SemId *newValue)
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
	sm_SemEnd(_dtn2fwSemaphore(NULL));
}

static int	enqueueBundle(Bundle *bundle, Object bundleObj)
{
	Sdr		sdr = getIonsdr();
	Object		elt;
	char		eid[SDRSTRING_BUFSZ];
	MetaEid		metaEid;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	VPlan		*vplan;
	BpPlan		plan;

	elt = sdr_list_first(sdr, bundle->stations);
	if (elt == 0)
	{
		putErrmsg("Forwarding error; stations stack is empty.", NULL);
		return -1;
	}

	sdr_string_read(sdr, eid, sdr_list_data(sdr, elt));
	if (parseEidString(eid, &metaEid, &vscheme, &vschemeElt) == 0)
	{
		writeMemoNote("[?] Can't parse node EID string", eid);
		return bpAbandon(bundleObj, bundle, BP_REASON_NO_ROUTE);
	}

	if (strcmp(vscheme->name, "dtn") != 0)
	{
		putErrmsg("Forwarding error; EID scheme is not 'dtn'.",
				vscheme->name);
		return -1;
	}

	restoreEidString(&metaEid);
	lookupPlan(eid, &vplan);
	if (vplan == NULL)
	{
		writeMemoNote("[?] Can't find egress plan for EID", eid);
		return bpAbandon(bundleObj, bundle, BP_REASON_NO_ROUTE);
	}

	sdr_read(sdr, (char *) &plan, sdr_list_data(sdr, vplan->planElt),
			sizeof(BpPlan));
	if (plan.viaEid == 0)	/*	Must transmit to neighbor.	*/
	{
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

		if (bundle->planXmitElt)
		{
			/*	Enqueued.				*/

			return bpAccept(bundleObj, bundle);
		}
		else
		{
			return bpAbandon(bundleObj, bundle, BP_REASON_NO_ROUTE);
		}
	}

	/*	Can't transmit to indicated next node directly, must
	 *	forward through some other node.			*/

	sdr_write(sdr, bundleObj, (char *) &bundle, sizeof(Bundle));
	sdr_string_read(sdr, eid, plan.viaEid);
	return forwardBundle(bundleObj, bundle, eid);
}

#if defined (ION_LWT)
int	dtn2fw(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
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

	if (bpAttach() < 0)
	{
		putErrmsg("dtn2fw can't attach to BP.", NULL);
		return 1;
	}

	sdr = getIonsdr();
	findScheme("dtn", &vscheme, &vschemeElt);
	if (vschemeElt == 0)
	{
		putErrmsg("Scheme name for dtn2 is unknown.", "dtn");
		return 1;
	}

	CHKZERO(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &scheme, sdr_list_data(sdr,
			vscheme->schemeElt), sizeof(Scheme));
	sdr_exit_xn(sdr);
	oK(_dtn2fwSemaphore(&vscheme->semaphore));
	isignal(SIGTERM, shutDown);

	/*	Main loop: wait until forwarding queue is non-empty,
	 *	then drain it.						*/

	writeMemo("[i] dtn2fw is running.");
	while (running && !(sm_SemEnded(vscheme->semaphore)))
	{
		/*	We wrap forwarding in an SDR transaction to
		 *	prevent race condition with bpclock (which
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
	writeMemo("[i] dtn2fw forwarder has ended.");
	ionDetach();
	return 0;
}
