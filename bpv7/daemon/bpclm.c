/*
	bpclm.c:	daemon for dispatching to outducts the
			bundles that are queued for forwarding to
			specified neighboring nodes.

	Author: Scott Burleigh, JPL

	Copyright (c) 2017, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
#include "bpP.h"
#include "ipnfw.h"

static sm_SemId	_bpclmSemaphore(sm_SemId *newValue)
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
	/* Tell the compiler that we are not using 'signum' */
	(void)signum;

	isignal(SIGTERM, shutDown);
	sm_SemEnd(_bpclmSemaphore(NULL));
}

static size_t	maxPayloadLengthKnown(VPlan *vplan, size_t *maxPayloadLength)
{
	size_t	secRemaining;
	size_t	xmitRate;

	*maxPayloadLength = 0;		/*	Default: unlimited.	*/
	if (vplan->neighborFqnn)	/*	Known neighbor node.	*/
	{
		/*	If neighbor node number is known, we may be
		 *	able to limit bundle size to the remaining
		 *	contact capacity.  But we can only do this
		 *	if the contact plan contains contacts for
		 *	transmission to this node.			*/

		rfx_contact_state(vplan->neighborFqnn, &secRemaining,
				&xmitRate);
		if (secRemaining == 0)	/*	No current contact.	*/
		{
			if (xmitRate == 0)
			{
				/*	No capacity right now. Try
				 *	again later.			*/

				return 0;	/*	Still unknown.	*/
			}

			/*	Otherwise the returned xmit rate is
			 *	(unsigned int) -1, i.e., unlimited.
			 *	This means the contact plan contains
			 *	no contacts for transmission to the
			 *	neighbor node.  So max payload length
			 *	is now known to be unlimited.		*/
		}
		else	/*	Currently in contact.			*/
		{
			*maxPayloadLength = xmitRate * secRemaining;
		}
	}

	/*	If neighbor node number for plan is unknown, then
	 *	there's no basis for limiting payload length.
	 *
	 *	So at this point the maxPayloadLength is now known,
	 *	one way or another.					*/

	return 1;
}

#ifdef ION_BANDWIDTH_RESERVED
static void	selectNextBundleForTransmission(Outflow *flows,
			Outflow **winner, SdrObject *eltp)
{
	Sdr		bpSdr = getIonsdr();
	Outflow		*flow;
	SdrObject	elt;
	Outflow		*selectedFlow;
	unsigned int	selectedFlowSvc;
	int		i;
	unsigned int	svcProvided;
	int             selectedFlowNbr;

	*winner = NULL;		/*	Default.			*/
	*eltp = 0;		/*	Default.			*/

	/*	If any priority traffic, choose it.			*/

	flow = flows + EXPEDITED_FLOW;
	elt = sdr_list_first(bpSdr, flow->outboundBundles);
	if (elt)
	{
		/*	*winner remains NULL to indicate that the
		 *	urgent flow was selected.  Prevents resync.	*/

		*eltp = elt;
		return;
	}

	/*	Otherwise send next PDU on the least heavily serviced
		non-empty non-urgent flow, if any.			*/

	selectedFlow = NULL;
	selectedFlowSvc = (unsigned int) -1;
	for (i = 0; i < EXPEDITED_FLOW; i++)
	{
		flow = flows + i;
		if (sdr_list_length(bpSdr, flow->outboundBundles) == 0)
		{
			continue;	/*	Nothing ready to send.	*/
		}

		/*	Consider flow as transmission candidate.	*/

		svcProvided = flow->totalBytesSent / flow->svcFactor;
		if (svcProvided < selectedFlowSvc)
		{
			selectedFlow = flow;
			selectedFlowSvc = svcProvided;
			selectedFlowNbr = i;
		}
	}

	/*	At this point, selectedFlow (if any) is the flow for
	 *	which the smallest value of svcProvided was calculated,
	 *	and selectedFlowSvc is that value.			*/

	if (selectedFlow)
	{
		*winner = selectedFlow;
		*eltp = sdr_list_first(bpSdr, selectedFlow->outboundBundles);
	}
}

static void	resyncFlows(Outflow *flows)
{
	unsigned int	minSvcProvided;
	unsigned int	maxSvcProvided;
	int		i;
	Outflow		*flow;
	unsigned int	svcProvided;

	/*	Reset all flows if too unbalanced.			*/

	minSvcProvided = (unsigned int) -1;
	maxSvcProvided = 0;
	for (i = 0; i < EXPEDITED_FLOW; i++)
	{
		flow = flows + i;
		svcProvided = flow->totalBytesSent / flow->svcFactor;
		if (svcProvided < minSvcProvided)
		{
			minSvcProvided = svcProvided;
		}

		if (svcProvided > maxSvcProvided)
		{
			maxSvcProvided = svcProvided;
		}
	}

	if ((maxSvcProvided - minSvcProvided) >
			(MAX_STARVATION * NOMINAL_BYTES_PER_SEC))
	{
	/*	The most heavily serviced flow is at least
		MAX_STARVATION seconds ahead of the least
		serviced flow, so any sustained spike in
		traffic on the least serviced flow would
		starve the most serviced flow for at least
		MAX_STARVATION seconds.  To prevent this,
		we level the playing field by resetting all
		flows' totalBytesSent to zero.				*/

		for (i = 0; i < EXPEDITED_FLOW; i++)
		{
			flows[i].totalBytesSent = 0;
		}
	}
}
#else		/*	Strict priority, which is the default.		*/
static void	selectNextBundleForTransmission(Outflow *flows,
			Outflow **winner, SdrObject *eltp)
{
	int	i;
	Outflow	*flow;
	SdrObject elt;

	*eltp = 0;		/*	Default: nothing ready.		*/
	i = EXPEDITED_FLOW;	/*	Start with highest priority.	*/
	while (1)
	{
		flow = flows + i;
		elt = sdr_list_first(getIonsdr(), flow->outboundBundles);
		if (elt)
		{
			*winner = flow;
			*eltp = elt;
			return;	/*	Got highest-priority bundle.	*/
		}

		i--;		/*	Try next lower priority flow.	*/
		if (i < 0)
		{
			return;	/*	Nothing is queued.		*/
		}
	}
}
#endif

static int	getOutboundBundle(Outflow *flows, VPlan *vplan,
			SdrObject *bundleElt, Bundle *bundle)
{
	Sdr		bpSdr = getIonsdr();
	Outflow		*selectedFlow;
	SdrObject	xmitElt;

	/* Parameter intentionally unused. */
	(void)bundle;

	while (1)
	{
		*bundleElt = 0;			/*	Default.	*/
		selectNextBundleForTransmission(flows, &selectedFlow, &xmitElt);
		if (xmitElt == 0)		/*	Nothing ready.	*/
		{
			sdr_exit_xn(bpSdr);

			/*	Wait until forwarder announces an
			 *	outbound bundle by giving plan's
			 *	semaphore.				*/

#ifdef DEBUG_CRASH_RECOVERY
			writeMemoNote("[DEBUG] bpclm: No bundles, waiting on semaphore", vplan->neighborEid);
#endif
			if (sm_SemTake(vplan->semaphore) < 0)
			{
				putErrmsg("bpclm can't take plan semaphore.",
						vplan->neighborEid);
				return -1;
			}
#ifdef DEBUG_CRASH_RECOVERY
			writeMemoNote("[DEBUG] bpclm: Semaphore taken", vplan->neighborEid);
#endif

			if (sm_SemEnded(vplan->semaphore))
			{
				writeMemoNote("[i] Plan has been stopped",
						vplan->neighborEid);

				/*	End task, but without error.	*/

				sm_SemGive(vplan->semaphore);
				return 0;
			}

			CHKERR(sdr_begin_xn(bpSdr));
			continue;	/*	Should succeed now.	*/
		}

		/*	Got next outbound bundle.  Resync flows as
		 *	necessary.					*/

		*bundleElt = xmitElt;
#ifdef ION_BANDWIDTH_RESERVED
		if (selectedFlow != NULL)	/*	Not urgent.	*/
		{
			selectedFlow->totalBytesSent += bundle->payload.length;
			resyncFlows(flows);
		}
#endif
		return 0;
	}
}

static int	outductSelected(BpPlan *plan, SdrObject planObj, Bundle *bundle,
			int classReqd, ClProtocol *protocol, Outduct *outduct)
{
	Sdr		sdr = getIonsdr();
	SdrObject	ovrdAddr;
	IpnOverride	ovrd;
	char		outductExpression[SDRSTRING_BUFSZ];
	char		*cursor;
	char		protocolNameBuf[MAX_CL_PROTOCOL_NAME_LEN + 1];
	char		*protocolName = NULL;
	char		*ductName = NULL;
	SdrObject	ductElt;
	SdrObject	outductElt;
	SdrObject	outductObj;

	/* Parameter intentionally unused. */
	(void)planObj;

	if (bundle->ovrdNeighbor)	/*	Routing override.	*/
	{
		if (ipn_lookupOvrd(bundle->ancillaryData.dataLabel,
			bundle->destination.ssp.ipn.fqnn,
			bundle->id.source.ssp.ipn.fqnn, &ovrdAddr) == 0)
		{
			writeMemo("[?] Routing override removed, bundle must \
be redirected.");
			bundle->ovrdNeighbor = 0;
			return 0;
		}

		sdr_read(sdr, (char *) &ovrd, ovrdAddr, sizeof(IpnOverride));
		if (ovrd.neighborFqnn != bundle->ovrdNeighbor)
		{
			writeMemo("[?] Routing override changed, bundle must \
be redirected.");
			bundle->ovrdNeighbor = ovrd.neighborFqnn;
			return 0;
		}

		if (ovrd.ductExpression)
		{
			/*      E.g., coerce to BIBE.           	*/

			if (sdr_string_read(sdr, outductExpression,
					ovrd.ductExpression) < 0)
			{
				putErrmsg("Unreadable override outduct \
expression", itoa(ovrd.ductExpression));
				return 0;
			}

			cursor = strchr(outductExpression, '/');
			if (cursor == NULL)
			{
				writeMemoNote("[?] Invalid override outduct \
expression", outductExpression);
				return 0;
			}

			*cursor = '\0';
			istrcpy(protocolNameBuf, outductExpression,
					sizeof(protocolNameBuf));
			*cursor = '/';
			protocolName = protocolNameBuf;
			ductName = cursor + 1;
		}
	}

	/*	Examine all outducts for this egress plan and
	 *	select the best one.					*/

	for (ductElt = sdr_list_first(sdr, plan->ducts); ductElt;
			ductElt = sdr_list_next(sdr, ductElt))
	{
		outductElt = sdr_list_data(sdr, ductElt);
		outductObj = sdr_list_data(sdr, outductElt);
		sdr_read(sdr, (char *) outduct, outductObj, sizeof(Outduct));
		sdr_read(sdr, (char *) protocol, outduct->protocol,
				sizeof(ClProtocol));
		if (protocolName)	/*	Specific duct needed.	*/
		{
			if (strcmp(protocolName, protocol->name) == 0
			&& strcmp(ductName, outduct->name) == 0)
			{
				/*	This is the required outduct.	*/

				return 1;
			}

			continue;
		}

#if defined(MULTIDUCTS)
#include "selectcla.c"
#else
		if ((protocol->protocolClass & classReqd))
		{
			return 1;	/*	Duct is usable.		*/
		}
#endif
	}

	if (protocolName)		/*	Specific duct needed.	*/
	{
		writeMemoNote("[?] Override outduct unknown for plan",
				outductExpression);
	}

	return 0;			/*	No ducts are usable.	*/
}

static void	getOutduct(VPlan *vplan, Bundle *bundle, VOutduct **vduct)
{
	Sdr		sdr = getIonsdr();
	int		protClassReqd;
	SdrObject	planObj;
	BpPlan		plan;
	ClProtocol	protocol;
	Outduct		outduct;
	PsmAddress	vductElt;

	*vduct = NULL;			/*	Default.		*/
	protClassReqd = bundle->qosFlags & BP_PROTOCOL_ANY;
#if 0
	/*	BSSP is deprecated.					*/

	if (protClassReqd & BP_RELIABLE_STREAMING)
	{
		/*	BSSP is required, no other protocol will do.
		 *	Exclude BP_BEST_EFFORT and BP_RELIABLE even if
		 *	(erroneously) asserted in ancillaryData flags.	*/

		protClassReqd = BP_RELIABLE_STREAMING;
	}
#endif
	if (protClassReqd == 0)		/*	Don't care.		*/
	{
		protClassReqd = BP_PROTOCOL_ANY;
	}

	planObj = sdr_list_data(sdr, vplan->planElt);
	sdr_read(sdr, (char *) &plan, planObj, sizeof(BpPlan));
	if (outductSelected(&plan, planObj, bundle, protClassReqd, &protocol,
			&outduct))
	{
		findOutduct(protocol.name, outduct.name, vduct, &vductElt);
		return;
	}

	/*	Fallback: if best effort only was requested but no
	 *	unreliable CLA is available, try reliable CLAs.
	 *	Conversely, if reliable only was requested but no
	 *	reliable CLA is available, try unreliable CLAs.	*/

	if (protClassReqd == BP_BEST_EFFORT)
	{
		if (outductSelected(&plan, planObj, bundle, BP_RELIABLE,
				&protocol, &outduct))
		{
			findOutduct(protocol.name, outduct.name, vduct,
					&vductElt);
		}
	}
	else if (protClassReqd == BP_RELIABLE)
	{
		if (outductSelected(&plan, planObj, bundle, BP_BEST_EFFORT,
				&protocol, &outduct))
		{
			findOutduct(protocol.name, outduct.name, vduct,
					&vductElt);
		}
	}
}

#if defined (ION_LWT)
int	bpclm(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char		*nodeName = (char *) a1;
#else
int	main(int argc, char *argv[])
{
	char		*nodeName = (argc > 1 ? argv[1] : NULL);
#endif
	Sdr		sdr;
	VPlan		*vplan;
	PsmAddress	vplanElt;
	SdrObject	planObj;
	BpPlan		plan;
	Throttle	*throttle;
	Outflow		outflows[3];
	int		i;
	int		running = 1;
	size_t		maxPayloadLength;
	SdrObject	bundleElt;
	SdrObject	bundleObj;
	Bundle		bundle;
	VOutduct	*vduct;
			OBJ_POINTER(Outduct, outduct);
	Bundle		firstBundle;
	SdrObject	firstBundleObj;
	Bundle		secondBundle;
	SdrObject	secondBundleObj;
	SdrObject	queue;

	if (!nodeName)
	{
		PUTS("Usage: bpclm <node name (EID)>");
		return 0;
	}

#ifdef DEBUG_CRASH_RECOVERY
	writeMemoNote("[DEBUG] bpclm: Starting for node", nodeName);
#endif

	if (bpAttach() < 0)
	{
		putErrmsg("bpclm can't attach to BP.", NULL);
		return -1;
	}

#ifdef DEBUG_CRASH_RECOVERY
	writeMemoNote("[DEBUG] bpclm: bpAttach succeeded", nodeName);
#endif

	if (ipnInit() < 0)
	{
		putErrmsg("bpclm can't initialize IPN scheme.", NULL);
		return -1;
	}

#ifdef DEBUG_CRASH_RECOVERY
	writeMemoNote("[DEBUG] bpclm: ipnInit succeeded", nodeName);
#endif

	sdr = getIonsdr();
#ifdef DEBUG_CRASH_RECOVERY
	writeMemoNote("[DEBUG] bpclm: About to lock SDR", nodeName);
#endif
	CHKZERO(sdr_begin_xn(sdr));	/*	Just to lock database.	*/
#ifdef DEBUG_CRASH_RECOVERY
	writeMemoNote("[DEBUG] bpclm: SDR locked", nodeName);
#endif
	findPlan(nodeName, &vplan, &vplanElt);
	if (vplanElt == 0)
	{
		sdr_exit_xn(sdr);
		putErrmsg("No egress plan for this node", nodeName);
		return -1;
	}

	if (vplan->clmPid != ERROR && vplan->clmPid != sm_TaskIdSelf())
	{
		sdr_exit_xn(sdr);
		putErrmsg("bpclm task is already started for this node",
				nodeName);
		return -1;
	}

	/*	All command-line arguments are now validated.		*/

	planObj = sdr_list_data(sdr, vplan->planElt);
	sdr_read(sdr, (char *) &plan, planObj, sizeof(BpPlan));
	memset((char *) outflows, 0, sizeof outflows);
	outflows[0].outboundBundles = plan.bulkQueue;
	outflows[1].outboundBundles = plan.stdQueue;
	outflows[2].outboundBundles = plan.urgentQueue;
	for (i = 0; i < 3; i++)
	{
		outflows[i].svcFactor = 1 << i;
	}

	sdr_exit_xn(sdr);		/*	Unlock the database.	*/
#ifdef DEBUG_CRASH_RECOVERY
	writeMemoNote("[DEBUG] bpclm: SDR unlocked", nodeName);
#endif

	/*	Set up signal handling.					*/

	oK(_bpclmSemaphore(&vplan->semaphore));
	isignal(SIGTERM, shutDown);

	/*	Main loop: exercise rate control as necessary, then
	 *	get next bundle that is bound for this neighboring
	 *	node, select outduct for transmission of this bundle
	 *	to this node, fragment the bundle if necessary, clear
	 *	any untransmitted bundle out of the outduct's buffer
	 *	(exactly as if convergence-layer transmission had
	 *	failed), insert the bundle into that buffer, and give
	 *	the semaphore for that Outduct.				*/

	writeMemoNote("[i] bpclm is running", nodeName);
#ifdef DEBUG_CRASH_RECOVERY
	writeMemoNote("[DEBUG] bpclm: Entering main loop", nodeName);
#endif
	while (running)
	{
		if (sdr_begin_xn(sdr) == 0)
		{
			putErrmsg("bpclm can't begin transaction.", nodeName);
			running = 0;
			continue;
		}

		throttle = applicableThrottle(vplan);
		CHKZERO(throttle);

		/*	Wait until (a) there is at least one outduct,
		 *	(b) maximum payload length is known, and (c)
		 *	rate control (if applicable) is satisfied.	*/

		if (sdr_list_length(sdr, plan.ducts) == 0
		|| maxPayloadLengthKnown(vplan, &maxPayloadLength) == 0
		|| (throttle->nominalRate == 0 && maxPayloadLength > 0)
		|| (throttle->nominalRate > 0 && throttle->capacity <= 0))
		{
			sdr_exit_xn(sdr);

			/*	Wait for conditions to change. Signal
			 *	sources: bpclock (rate capacity restored
			 *	or contact topology changed),
			 *	attachPlanDuct (outduct attached), or
			 *	sm_SemEnd (shutdown).			*/

			if (sm_SemTake(vplan->semaphore) < 0)
			{
				putErrmsg("bpclm can't take semaphore.",
						nodeName);
				running = 0;
				continue;
			}

			if (sm_SemEnded(vplan->semaphore))
			{
				running = 0;
			}

			continue;
		}

		/*	Get a transmittable bundle.			*/

		if (getOutboundBundle(outflows, vplan, &bundleElt, &bundle) < 0)
		{
			sdr_exit_xn(sdr);
			putErrmsg("bpclm can't get outbound bundle.", NULL);
			running = 0;		/*	Stop daemon.	*/
			continue;
		}

		if (bundleElt == 0)		/*	Plan stopped.	*/
		{
			sdr_exit_xn(sdr);
			running = 0;		/*	Stop daemon.	*/
			continue;
		}

		bundleObj = sdr_list_data(sdr, bundleElt);
		sdr_stage(sdr, (char *) &bundle, bundleObj, sizeof(Bundle));

		/*	Re-verify contact state after waking from semaphore.
		 *	This prevents a TOCTOU race condition where the contact
		 *	graph loads while bpclm is blocked waiting for bundles,
		 *	causing immediate transmission instead of waiting for
		 *	the scheduled future contact.  If contact state changed
		 *	to "not ready", exit transaction and loop back to wait.	*/

		if (maxPayloadLengthKnown(vplan, &maxPayloadLength) == 0)
		{
			/*	Contact state changed - not ready yet.	*/

			sdr_exit_xn(sdr);
			continue;	/*	Loop back, will wait on sem.	*/
		}

		/*	Allocate transmittable bundle to an outduct.	*/

		getOutduct(vplan, &bundle, &vduct);
		if (vduct == NULL)		/*	No usable duct.	*/
		{
			sdr_stage(sdr, (char *) &plan, planObj, sizeof(BpPlan));
			removeBundleFromQueue(&bundle, &plan);
			sdr_write(sdr, planObj, (char *) &plan, sizeof(BpPlan));
			sdr_write(sdr, bundleObj, (char *) &bundle,
					sizeof(Bundle));
			oK(enqueueToLimbo(&bundle, bundleObj));
			if (sdr_end_xn(sdr) < 0)
			{
				putErrmsg("Failed enqueue to limbo.", nodeName);
				running = 0;	/*	Stop daemon.	*/
			}

			continue;
		}

		/*	Fragment bundle as necessary.			*/

		GET_OBJ_POINTER(sdr, Outduct, outduct, sdr_list_data(sdr,
				vduct->outductElt));
		if (outduct->maxPayloadLen > 0
		&& (maxPayloadLength == 0
			|| outduct->maxPayloadLen < maxPayloadLength))
		{
			maxPayloadLength = outduct->maxPayloadLen;
		}

		if (maxPayloadLength > 0
		&& bundle.payload.length >= 0
		&& (size_t)bundle.payload.length > maxPayloadLength)
		{
			/*	Must fragment this bundle.		*/

			if (bundle.bundleProcFlags & BDL_DOES_NOT_FRAGMENT)
			{
				/*	Bundle can't be fragmented,
				 *	cannot be sent to this
				 *	neighboring node.  Reforward
				 *	it and get another bundle.	*/

				sdr_stage(sdr, (char *) &plan, planObj,
						sizeof(BpPlan));
				removeBundleFromQueue(&bundle, &plan);
				sdr_write(sdr, planObj, (char *) &plan,
						sizeof(BpPlan));
				sdr_write(sdr, bundleObj, (char *) &bundle,
						sizeof(Bundle));
				if (bpReforwardBundle(bundleObj) < 0)
				{
					sdr_cancel_xn(sdr);
					putErrmsg("Frag refwd failed.",
							nodeName);
					running = 0;
					continue;
				}

				/*	Try next bundle.	*/

				if (sdr_end_xn(sdr) < 0)
				{
					putErrmsg("Reforward failed.",
							nodeName);
					running = 0;
				}

				continue;
			}

			/*	Okay to fragment.			*/

			queue = sdr_list_list(sdr, bundle.planXmitElt);
			if (bpFragment(&bundle, bundleObj,
					&(bundle.planXmitElt), maxPayloadLength,
					&firstBundle, &firstBundleObj,
					&secondBundle, &secondBundleObj) < 0)
			{
				sdr_cancel_xn(sdr);
				putErrmsg("CLO can't fragment bundle.",
						nodeName);
				running = 0;
				continue;
			}

			/*	Insert the two fragments back into the
			 *	plan queue, with the segment that is
			 *	immediately transmittable at the front
			 *	of the queue.				*/

			secondBundle.planXmitElt = sdr_list_insert_first(sdr,
					queue, secondBundleObj);
			sdr_write(sdr, secondBundleObj, (char *)
					&secondBundle, sizeof(Bundle));
			firstBundle.planXmitElt = sdr_list_insert_first(sdr,
					queue, firstBundleObj);
			sdr_write(sdr, firstBundleObj, (char *) &firstBundle,
					sizeof(Bundle));
			bundleElt = firstBundle.planXmitElt;
			bundleObj = firstBundleObj;
			memcpy((char *) &bundle, (char *) &firstBundle,
					sizeof(Bundle));
		}

		/*	Pop the outbound bundle out of its issuance
		 *	queue.						*/

		sdr_stage(sdr, (char *) &plan, planObj, sizeof(BpPlan));
		removeBundleFromQueue(&bundle, &plan);
		sdr_write(sdr, planObj, (char *) &plan, sizeof(BpPlan));

		/*	Pass the bundle to the outduct.			*/

		bundle.ductXmitElt = sdr_list_insert_last(sdr,
				outduct->xmitBuffer, bundleObj);
		sdr_write(sdr, bundleObj, (char *) &bundle, sizeof(Bundle));
		sm_SemGive(vduct->semaphore);

		/*	Track this transmission event.			*/

		bpPlanTally(vplan, BP_PLAN_DEQUEUED, bundle.payload.length);
		bpXmitTally(bundle.priority, bundle.payload.length);
		if ((getBpVdb())->watching & WATCH_c)
		{
#if defined (EWCHAR)
			char ewchar[256];
			char nbrBuf[FQN_MAX_LENGTH];
			/* spec is for 64 bit, non-Window */
			putFqn(nbrBuf, bundle.id.source.ssp.ipn.fqnn);
			isprintf(ewchar,sizeof(ewchar),"(%s,%lu,%u)c",nbrBuf,
				bundle.id.source.ssp.ipn.serviceNbr,
				bundle.id.creationTime.count);
			iwatch_str(ewchar);
#else
			iwatch('c');
#endif

		}

		/*	Consume estimated transmission capacity.	*/

		throttle->capacity -= computeECCC(guessBundleSize(&bundle));
		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Failed requeuing bundle.", nodeName);
			running = 0;	/*	Terminate loop.		*/
			continue;
		}

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();
	}

	writeErrmsgMemos();
	writeMemoNote("[i] bpclm has ended", nodeName);
	ionDetach();
	return 0;
}
