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

static void	shutDown()	/*	Commands bpclm termination.	*/
{
	isignal(SIGTERM, shutDown);
	sm_SemEnd(_bpclmSemaphore(NULL));
}

static int	maxPayloadLengthKnown(VPlan *vplan,
			unsigned int *maxPayloadLength)
{
	Sdr		sdr = getIonsdr();
	unsigned int	secRemaining;
	unsigned int	xmitRate;

	*maxPayloadLength = 0;		/*	Default: unlimited.	*/
	if (vplan->neighborNodeNbr)	/*	Known neighbor node.	*/
	{
		/*	If neighbor node number is known, we may be
		 *	able to limit bundle size to the remaining
		 *	contact capacity.  But we can only do this
		 *	if the contact plan contains contacts for
		 *	transmission to this node.			*/

		rfx_contact_state(vplan->neighborNodeNbr, &secRemaining,
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
			Outflow **winner, Object *eltp)
{
	Sdr		bpSdr = getIonsdr();
	Outflow		*flow;
	Object		elt;
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
			Outflow **winner, Object *eltp)
{
	int	i;
	Outflow	*flow;
	Object	elt;

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

static int	getOutboundBundle(Outflow *flows, char *nodeName, VPlan *vplan,
			Object *bundleElt, Bundle *bundle)
{
	Sdr		bpSdr = getIonsdr();
	PsmPartition	bpwm = getIonwm();
	Outflow		*selectedFlow;
	Object		xmitElt;

	while (1)
	{
		*bundleElt = 0;			/*	Default.	*/
		selectNextBundleForTransmission(flows, &selectedFlow, &xmitElt);
		if (xmitElt == 0)		/*	Nothing ready.	*/
		{
			sdr_exit_xn(bpSdr);

			/*	Wait until forwarder announces an
			 *	outbound bundle by giving duct's
			 *	semaphore.  Duct might have changed
			 *	in the interim, so re-read it.		*/

			if (sm_SemTake(vplan->semaphore) < 0)
			{
				putErrmsg("bpclm can't take plan semaphore.",
						nodeName);
				return -1;
			}

			if (sm_SemEnded(vplan->semaphore))
			{
				writeMemoNote("[i] Plan has been stopped",
						nodeName);

				/*	End task, but without error.	*/

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

static int	checkEmbargo(VPlan *vplan, Bundle *bundle, Object bundleObj)
{
	PsmPartition	ionwm = getIonwm();
	IonNode		*destNode;
	PsmAddress	nextNode;
	PsmAddress	elt;
	PsmAddress	nextEmbargo;
	Embargo		*embargo;

	/*	If this neighbor has begun refusing bundles for the
	 *	indicated destination since this bundle was enqueued
	 *	for transmission, re-forward the bundle on a different
	 *	route (if possible).  Note that we can only track
	 *	refusals issued by neighbors identified by CBHE-
	 *	conformant endpoint IDs, for destinations that are
	 *	identified by CBHE-conformant unicast endpoint IDs.	*/

	if (vplan->neighborNodeNbr == 0)
	{
		return 0;		/*	No applicable embargo.	*/
	}

	if (!(bundle->destination.cbhe && bundle->destination.unicast))
	{
		return 0;		/*	No applicable embargo.	*/
	}

	destNode = findNode(getIonVdb(), bundle->destination.c.nodeNbr,
			&nextNode);
	if (destNode == NULL)
	{
		return 0;		/*	No applicable embargo.	*/
	}

	/*	Look for this neighbor in the list of neighboring nodes
	 *	identified by CBHE-conformant endpoint IDs that have
	 *	been refusing bundles for this destination.		*/

	for (elt = sm_list_first(bpwm, destNode->embargoes); elt;
			elt = nextEmbargo)
	{
		nextEmbargo = sm_list_next(bpwm, elt);
		embargo = (Embargo *) psp(bpwm, sm_list_data(bpwm, elt));
		if (embargo->nodeNbr < vplan->neighborNodeNbr)
		{
			continue;
		}

		if (embargo->nodeNbr == vplan->neighborNodeNbr)
		{
			break;
		}

		nextEmbargo = 0;	/*	No more to check.	*/
	}

	if (elt == 0)
	{
		return 0;		/*	No applicable embargo.	*/
	}

	/*	This neighbor has been refusing custody of bundles
	 *	for this destination.					*/

	if (bpReforwardBundle(bundleObj) < 0)
	{
		putErrmsg("Embargo reforward failed.", NULL);
		return -1;
	}

	return 1;			/*	Embargo applied.	*/
}

static VOutduct	*selectOutduct(Vplan *vplan, Bundle *bundle)
{
	Sdr		sdr = getIonsdr();
	Object		directiveElt;
	Object		directiveObj;
			OBJ_POINTER(FwdDirective, directive);
	char		ductExpression[SDRSTRING_BUFSZ];
	char		*cursor;
	char		*protocolName;
	char		*outductName;
	VOutduct	*vduct;
	PsmAddress	vductElt;
	Object		ductAddr;
	Outduct		duct;

	/*	Eventually the plan->directives list will be replaced
	 *	by a list of outduct selection rules pairing bundle
	 *	characteristics with duct expressions, but for now
	 *	there is only a single assigned outduct per plan.	*/

	directiveElt = sdr_list_first(sdr, plan->directives);
	CHKNULL(directiveElt);
	directiveObj = sdr_list_data(sdr, directiveElt);
	GET_OBJ_POINTER(sdr, FwdDirective, directive, directiveObj);
	CHKNULL(directive->action == xmit);
	CHKNULL(sdr_string_read(sdr, ductExpression, directive->spec) > 0);
	protocolName = ductExpression;
	cursor = strchr(ductExpression, '/');
	CHKNULL(cursor);
	*cursor = '\0';
	outductName = cursor + 1;
	findOutduct(protocolName, outductName, &vduct, &vductElt);
	ductAddr = sdr_list_data(sdr, vduct->outductElt);
	sdr_read(sdr, (char *) &duct, ductAddr, sizeof(Outduct));
	if (duct.blocked)

	
	return vduct;
}

static void	noteFragmentation(Bundle *bundle)
{
	Sdr	sdr = getIonsdr();
	Object	dbObject;
	BpDB	db;

	dbObject = getBpDbObject();
	sdr_stage(sdr, (char *) &db, dbObject, sizeof(BpDB));
	db.currentFragmentsProduced++;
	db.totalFragmentsProduced++;
	if (!(bundle->fragmented))
	{
		bundle->fragmented = 1;
		db.currentBundlesFragmented++;
		db.totalBundlesFragmented++;
	}

	sdr_write(sdr, dbObject, (char *) &db, sizeof(BpDB));
}

#if defined (ION_LWT)
int	bpclm(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	char		*nodeName = (char *) a1;
#else
int	main(int argc, char *argv[])
{
	char		*nodeName = (argc > 1 ? argv[1] : NULL);
#endif
	Sdr		sdr = getIonsdr();
	VPlan		*vplan;
	PsmAddress	vplanElt;
	Object		planObj;
	Plan		plan;
	Throttle	*throttle;
	Outflow		outflows[3];
	int		i;
	int		running = 1;
	unsigned int	maxPayloadLength;
	Object		bundleElt;
	Object		bundleObj;
	Bundle		bundle;
	VOutduct	*vduct;
			OBJ_POINTER(Outduct, outduct);
	Object		queue;
	Object		elt;

	if (!nodeName)
	{
		PUTS("Usage: bpclm <node name (EID)>");
		return 0;
	}

	if (bpAttach() < 0)
	{
		putErrmsg("bpclm can't attach to BP.", NULL);
		return -1;
	}

	CHKZERO(sdr_begin_xn(sdr));	/*	Just to lock database.	*/
	findPlan(nodeName, &vplan, &vplanElt);
	if (planElt == 0)
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
	sdr_read(sdr, (char *) &plan, planObj, sizeof(Plan));
	throttle = applicableThrottle(vplan);
	sdr_exit_xn(sdr);		/*	Unlock the database.	*/

	/*	NOTE: plan deletion doesn't remove a Plan until its
	 *	clm pid is -1, so we don't need to re-find the Plan
	 *	after transaction exit.					*/

	memset((char *) outflows, 0, sizeof outflows);
	outflow[0].outboundBundle = plan.bulkQueue;
	outflow[1].outboundBundle = plan.stdQueue;
	outflow[2].outboundBundle = plan.urgentQueue;
	for (i = 0; i < 3; i++)
	{
		outflows[i].svcFactor = 1 << i;
	}

	/*	Set up signal handling.					*/

	oK(_bpclmSemaphore(&vplan->semaphore));
	isignal(SIGTERM, shutDown);

	/*	Main loop: exercise rate control as necessary, then
	 *	get next bundle that is bound for this neighboring
	 *	node, impose embargo check, select outduct for
	 *	transmission of this bundle to this node, fragment
	 *	the bundle if necessary, clear any untransmitted
	 *	bundle out of the outduct's buffer (exactly as if
	 *	convergence-layer transmission had failed), insert
	 *	the bundle into that buffer, and give the semaphore
	 *	for that Outduct.					*/

	writeMemoNote("[i] bpclm is running", nodeName);
	while (running)
	{
		/*	Rate control: wait for capacity if necessary.	*/

		CHKZERO(sdr_begin_xn(sdr));
		if (throttle->nominalRate > 0)	/*	Rate-controlled.*/
		{
			while (throttle->capacity <= 0)
			{
				sdr_exit_xn(sdr);
				snooze(1);
				CHKZERO(sdr_begin_xn(sdr));
			}
		}

		/*	Wait until contact plan constraint on payload
		 *	length is known, if any.  (May be unlimited.)	*/

		while (maxPayloadLengthKnown(vplan, &maxPayloadLength) == 0)
		{
			sdr_exit_xn(sdr);
			snooze(1);		/*	Await contact.	*/
			CHKZERO(sdr_begin_xn(sdr));
		}

		/*	Get a transmittable bundle.			*/

		if (getOutboundBundle(outflows, &bundleElt, &bundle) < 0)
		{
			sdr_exit_xn(sdr);
			putErrmsg("CLO can't get next outbound bundle.", NULL);
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

		/*	Check for embargo on bundles like this one.	*/

		if (checkEmbargo(vplan, &bundle, bundleObj) == 1)
		{
			/*	bpReforwardBundle has already removed
			 *	bundle from issuance queue.		*/

			if (sdr_end_xn(sdr) < 0)
			{
				putErrmsg("Failed handling embargo.", nodeName);
				running = 0;	/*	Stop daemon.	*/
			}

			continue;		/*	Try again.	*/
		}

		/*	Fragment transmittable bundle as necessary.	*/

		if (selectOutduct(&plan, &bundle, &vduct) < 0)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("Can't select outduct.", nodeName);
			running = 0;
			continue;
		}

		if (vduct == NULL)		/*	No usable duct.	*/
		{
			oK(enqueueToLimbo(&bundle, bundleObj));
			if (sdr_end_xn(sdr) < 0)
			{
				putErrmsg("Failed enqueue to limbo.", nodeName);
				running = 0;	/*	Stop daemon.	*/
			}

			continue;
		}

		GET_OBJ_POINTER(sdr, Outduct, outduct, sdr_list_data(sdr,
				vduct->outductElt));
		if (vduct->maxPayloadLen > 0
		&& vduct->maxPayloadLen < maxPayloadLength)
		{
			maxPayloadLength = vduct->maxPayloadLen;
		}

		if (maxPayloadLength > 0
		&& bundle->payload.length > maxPayloadLength)
		{
			/*	Must fragment this bundle.		*/

			if (bundle->bundleProcFlags & BDL_DOES_NOT_FRAGMENT)
			{
				/*	Bundle can't be fragmented,
				 *	cannot be sent to this
				 *	neighboring node.  Reforward
				 *	it and get another bundle.	*/

				removeBundleFromQueue(&bundle, bundleObj,
						planObj, plan);
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

			if (bpClone(bundle, &firstBundle, &firstBundleObj, 0,
					maxPayloadLength) < 0
			|| bpClone(bundle, &secondBundle, &secondBundleObj,
					maxPayloadLength, bundle->payload.length
					- maxPayloadLength) < 0)
			{
				sdr_cancel_xn(sdr);
				putErrmsg("CLO can't fragment bundle.",
						nodeName);
				running = 0;
				continue;
			}

			/*	Lose the original bundle, inserting
			 *	the two fragments in its place.  No
			 *	significant change to backlog, so we
			 *	don't call purgeDuctXmitElt which calls
			 *	removeBundleFromQueue.			*/

			queue = sdr_list_list(sdr, bundle->planXmitElt);
			sdr_list_delete(sdr, bundle->planXmitElt, NULL, NULL);
			bundle->planXmitElt = 0;
			if (bundle->custodyTaken)
			{
				/*	Means that custody of the
				 *	two clones is taken instead.	*/

				releaseCustody(*bundleObj, bundle);
				bpCtTally(BP_CT_CUSTODY_ACCEPTED,
						firstBundle.payload.length);
				bpCtTally(BP_CT_CUSTODY_ACCEPTED,
						secondBundle.payload.length);
			}

			/*	Lose the original bundle.		*/

			sdr_write(sdr, bundleObj, (char *) bundle,
					sizeof(Bundle));
			if (bpDestroyBundle(*bundleObj, 0) < 0)
			{
				sdr_cancel_xn(sdr);
				putErrmsg("CLO can't destroy original bundle.",
						nodeName);
				running = 0;
				continue;
			}

			/*	Insert two new fragments.		*/

			noteFragmentation(&secondBundle);
			secondBundle.planXmitElt = sdr_list_insert_first(sdr,
					queue, secondBundleObj);
			sdr_write(sdr, secondBundleObj, (char *)
					&secondBundle, sizeof(Bundle));
			firstBundle.planXmitElt = sdr_list_insert_first(sdr,
					queue, firstBundleObj);
			sdr_write(bpSdr, firstBundleObj, (char *) &firstBundle,
					sizeof(Bundle));
			xmitElt = firstBundle.planXmitElt;
			bundleObj = firstBundleObj;
			memcpy((char *) bundle, (char *) &firstBundle,
					sizeof(Bundle));
		}

		/*	Pop the outbound bundle out of its issuance
		 *	queue.						*/

		removeBundleFromQueue(bundle, bundleObj, planObj, plan);

		/*	Any bundle previously enqueued for transmission
		 *	via this outduct that has not yet been
		 *	transmitted is treated as a convergence-layer
		 *	transmission failure: we try again.		*/

		for (elt = sdr_list_first(sdr, outduct.xmitBuffer); elt;
				elt = sdr_list_next(sdr, elt))
		{
			if (bpReforwardBundle(bundleObj) < 0)
			{
				putErrmsg("Inferred CL-fail procedure failed",
						nodeName);
				break;
			}
		}

		if (elt)	/*	Didn't finish queue cleanup.	*/
		{
			sdr_cancel_xn(sdr);
			running = 0;
			continue;
		}

		/*	Pass the bundle to the outduct.			*/

		bundle.ductXmitElt = sdr_list_insert_last(sdr,
				outduct->xmitBuffer, bundleObj);
		sdr_write(sdr, bundleObj, (char *) &bundle, sizeof(Bundle));
		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Failed requeuing bundle.", nodeName);
			running = 0;	/*	Terminate loop.		*/
			continue;
		}

		sm_SemGive(vduct->semaphore);

		/*	Track this transmission event.			*/

		bpPlanTally(vplan, BP_PLAN_DEQUEUED, bundle.payload.length);
		bpXmitTally(COS_FLAGS(bundle.bundleProcFlags) & 0x03,
				bundle.payload.length);
		if ((_bpvdb(NULL))->watching & WATCH_c)
		{
			iwatch('c');
		}

		/*	Consume estimated transmission capacity.	*/

		throttle->capacity -= computeECCC(guessBundleSize(&bundle));

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();
	}

	writeErrmsgMemos();
	writeMemoNote("[i] bpclm has ended", nodeName);
	ionDetach();
	return 0;
}
