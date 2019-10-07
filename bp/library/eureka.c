/*
 *	eureka.c:	functions supporting contact discovery.
 *
 *	Copyright (c) 2014, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 *
 */

#include "eureka.h"
#include "bpP.h"
#include "ipnfw.h"
#include "dtn2fw.h"

static int 	compareDiscoveries(PsmPartition partition, PsmAddress eltData,
			void *argData)
{
	Discovery	*discovery = psp(partition, eltData);
	char		*targetEid = (char *) argData;

	return strncmp(discovery->eid, targetEid, MAX_EID_LEN);
}

static int	addDiscovery(char *eid)
{
	PsmPartition	wm = getIonwm();
	BpVdb		*vdb = getBpVdb();
	PsmAddress	elt;
	PsmAddress	discoveryAddr;
	Discovery	*discovery;

	elt = bp_find_discovery(eid);
	if (elt)
	{
		return 0;
	}

	discoveryAddr = psm_malloc(wm, sizeof(Discovery));
	if (discoveryAddr == 0)
	{
		putErrmsg("Can't add Discovery.", eid);
		return -1;
	}

	discovery = (Discovery *) psp(wm, discoveryAddr);
	istrcpy(discovery->eid, eid, sizeof discovery->eid);
	discovery->startOfContact = getCtime();
	discovery->lastContactTime = getCtime();
	if (sm_list_insert(wm, vdb->discoveries, discoveryAddr,
			compareDiscoveries, eid) == 0)
	{
		putErrmsg("Can't add Discovery.", eid);
		return -1;
	}

	return 0;
}

static void	deleteDiscovery(char *eid)
{
	PsmPartition	wm = getIonwm();
	PsmAddress	elt;
	PsmAddress	discoveryAddr;

	elt = bp_find_discovery(eid);
	if (elt)
	{
		discoveryAddr = sm_list_data(wm, elt);
		psm_free(wm, discoveryAddr);
		sm_list_delete(wm, elt, NULL, NULL);
	}
}

static void	toggleScheduledContacts(uvast fromNode, uvast toNode,
			ContactType fromType, ContactType toType)
{
	PsmPartition	ionwm = getIonwm();
	IonVdb		*ionvdb = getIonVdb();
	time_t		currentTime = getCtime();
	uvast		neighborNodeNbr = 0;
	IonNeighbor	*neighbor = NULL;
	IonCXref	arg;
	PsmAddress	elt;
	PsmAddress	contactAddr;
	IonCXref	*contact;

	if (toType == CtScheduled)
	{
		/*	We are restoring suppressed contacts.		*/

		if (fromNode == getOwnNodeNbr())
		{
			neighborNodeNbr = toNode;
		}
		else
		{
			neighborNodeNbr = fromNode;
		}
		
		neighbor = getNeighbor(ionvdb, neighborNodeNbr);
	}

	memset((char *) &arg, 0, sizeof(IonCXref));
	arg.fromNode = fromNode;
	arg.toNode = toNode;
	for (oK(sm_rbt_search(ionwm, ionvdb->contactIndex, rfx_order_contacts,
			&arg, &elt)); elt; elt = sm_rbt_next(ionwm, elt))
	{
		contactAddr = sm_rbt_data(ionwm, elt);
		contact = (IonCXref *) psp(ionwm, contactAddr);
		if (contact->fromNode > fromNode || contact->toNode > toNode)
		{
			return;		/*	Done.			*/
		}

		if (contact->type == fromType)
		{
			/*	Must toggle the type of this contact.	*/

			contact->type = toType;

			/*	May need to apply this contact, if
			 *	it was previously suppressed.		*/

			if (neighbor == NULL)
			{
				continue;
			}
			
			if (contact->fromTime <= currentTime
			&& contact->toTime > currentTime)
			{
				if (toNode == neighborNodeNbr)
				{
					neighbor->xmitRate = contact->xmitRate;
				}
				else 	/*	Contact FROM neighbor.	*/
				{
					neighbor->fireRate = contact->xmitRate;
					neighbor->recvRate = contact->xmitRate;
				}
			}
		}
	}
}

static int	noteContactAcquired(uvast discoveryNodeNbr,
			unsigned int xmitRate, unsigned int recvRate)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();
	IonVdb		*ionvdb = getIonVdb();
	uvast		ownNodeNbr = getOwnNodeNbr();
	time_t		fromTime = getCtime();
	double		volume = xmitRate * (MAX_POSIX_TIME - fromTime);
	int		regionIdx;
	IonNeighbor	*neighbor;
	IonCXref	arg;
	PsmAddress	elt;
	PsmAddress	contactAddr;
	IonCXref	*cxref;
	Object		contactObj;
	IonContact	contact;

	regionIdx = ionRegionOf(discoveryNodeNbr, ownNodeNbr);
	if (regionIdx < 0)
	{
		writeMemoNote("[?] Can't add contact for node; region unknown",
				itoa(discoveryNodeNbr));
		return 0;
	}

	neighbor = getNeighbor(ionvdb, discoveryNodeNbr);
	CHKZERO(neighbor);

	/*	Find matching hypothetical contact TO new neighbor.	*/

	cxref = NULL;
	memset((char *) &arg, 0, sizeof(IonCXref));
	arg.fromNode = ownNodeNbr;
	arg.toNode = discoveryNodeNbr;
	oK(sm_rbt_search(ionwm, ionvdb->contactIndex, rfx_order_contacts,
			&arg, &elt));
	if (elt)	/*	Found hypothetical contact.		*/
	{
		contactAddr = sm_rbt_data(ionwm, elt);
		cxref = (IonCXref *) psp(ionwm, contactAddr);
	}
	else		/*	Must insert hypothetical contact.	*/
	{
		if (rfx_insert_contact(regionIdx, 0, 0, ownNodeNbr,
				discoveryNodeNbr, 0, 0.0, &contactAddr) < 0
		|| contactAddr == 0)
		{
			putErrmsg("Can't add hypothetical contact.",
					itoa(discoveryNodeNbr));
			return -1;
		}

		cxref = (IonCXref *) psp(ionwm, contactAddr);
	}

	if (cxref->type == CtHypothetical)
	{
		CHKERR(sdr_begin_xn(sdr));
		contactObj = sdr_list_data(sdr, cxref->contactElt);
		sdr_stage(sdr, (char *) &contact, contactObj,
				sizeof(IonContact));
		contact.fromTime = fromTime;
		contact.xmitRate = xmitRate;
		contact.confidence = 1.0;
		contact.type = CtDiscovered;
		contact.mtv[0] = volume;
		contact.mtv[1] = volume;
		contact.mtv[2] = volume;
		sdr_write(sdr, contactObj, (char *) &contact,
				sizeof(IonContact));
		cxref->fromTime = fromTime;
		cxref->xmitRate = xmitRate;
		cxref->confidence = 1.0;
		cxref->type = CtDiscovered;
		neighbor->xmitRate = xmitRate;
		toggleScheduledContacts(ownNodeNbr, discoveryNodeNbr,
				CtScheduled, CtSuppressed);
		CHKERR(sdr_end_xn(sdr) == 0);
	}

	/*	Find matching hypothetical contact FROM new neighbor.	*/

	cxref = NULL;
	memset((char *) &arg, 0, sizeof(IonCXref));
	arg.fromNode = discoveryNodeNbr;
	arg.toNode = ownNodeNbr;
	oK(sm_rbt_search(ionwm, ionvdb->contactIndex, rfx_order_contacts,
			&arg, &elt));
	if (elt)	/*	Found hypothetical contact.		*/
	{
		contactAddr = sm_rbt_data(ionwm, elt);
		cxref = (IonCXref *) psp(ionwm, contactAddr);
	}
	else		/*	Must insert hypothetical contact.	*/
	{
		if (rfx_insert_contact(regionIdx, 0, 0, discoveryNodeNbr,
				ownNodeNbr, 0, 0.0, &contactAddr) < 0
		|| contactAddr == 0)
		{
			putErrmsg("Can't add hypothetical contact.",
					itoa(discoveryNodeNbr));
			return -1;
		}

		cxref = (IonCXref *) psp(ionwm, contactAddr);
	}

	if (cxref->type == CtHypothetical)
	{
		CHKERR(sdr_begin_xn(sdr));
		contactObj = sdr_list_data(sdr, cxref->contactElt);
		sdr_stage(sdr, (char *) &contact, contactObj,
				sizeof(IonContact));
		contact.fromTime = fromTime;
		contact.xmitRate = recvRate;
		contact.confidence = 1.0;
		contact.type = CtDiscovered;
		contact.mtv[0] = volume;
		contact.mtv[1] = volume;
		contact.mtv[2] = volume;
		sdr_write(sdr, contactObj, (char *) &contact,
				sizeof(IonContact));
		cxref->fromTime = fromTime;
		cxref->xmitRate = recvRate;
		cxref->confidence = 1.0;
		cxref->type = CtDiscovered;
		neighbor->fireRate = recvRate;
		neighbor->recvRate = recvRate;
		toggleScheduledContacts(discoveryNodeNbr, ownNodeNbr,
				CtScheduled, CtSuppressed);
		CHKERR(sdr_end_xn(sdr) == 0);
	}

	/*	Exchange discovered contact history with newly
	 *	identified (temporary) neighbor.			*/

	if (saga_send(discoveryNodeNbr, regionIdx) < 0)
	{
		putErrmsg("Can't send contact history message to neighbor.",
				itoa(discoveryNodeNbr));
		return -1;
	}

	return 0;
}

static int	discoveryAcquired(char *socketSpec, char *discoveryEid,
			char *claProtocol, unsigned int xmitRate,
			unsigned int recvRate)
{
	int		result;
	MetaEid		metaEid;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	uvast		discoveryNodeNbr;
	int		cbhe = 0;
	Object		elt;
	VPlan		*vplan;
	PsmAddress	vplanElt;
	char		ductExpression[SDRSTRING_BUFSZ];
	VOutduct	*vduct;
	PsmAddress	vductElt;
	int		portNumber;
	char		*inductDaemon;
	char		*outductDaemon;
	unsigned int	maxPayloadLength;
	ClProtocol	protocol;
	char		inductName[32];

	CHKERR(socketSpec);
	CHKERR(*socketSpec);
	CHKERR(discoveryEid);
	CHKERR(claProtocol);
	CHKERR(xmitRate);
	CHKERR(recvRate);
	result = parseEidString(discoveryEid, &metaEid, &vscheme, &vschemeElt);
	if (result == 0)
	{
		writeMemoNote("[?] Neighbor discovery EID error",
				discoveryEid);
		return -1;
	}

	discoveryNodeNbr = metaEid.nodeNbr;
	if (strcmp(metaEid.schemeName, "ipn") == 0)
	{
		cbhe = 1;
	}

	restoreEidString(&metaEid);
	elt = bp_find_discovery(discoveryEid);
	if (elt)
	{
		writeMemoNote("[?] Not a new discovery", discoveryEid);
		return 0;
	}

	findPlan(discoveryEid, &vplan, &vplanElt);
	if (vplanElt)
	{
		writeMemoNote("[?] Neighbor is managed; no discovery",
				discoveryEid);
		return 0;
	}

	isprintf(ductExpression, sizeof ductExpression, "%s/%s", claProtocol,
			socketSpec);
	findOutduct(claProtocol, socketSpec, &vduct, &vductElt);
	if (vductElt)
	{
		writeMemoNote("[?] Outduct is managed; no discovery",
				ductExpression);
		return 0;
	}

	if (strcmp(claProtocol, "tcp") == 0)
	{
		portNumber = BpTcpDefaultPortNbr;
		inductDaemon = "tcpcli";
		outductDaemon = "";
		maxPayloadLength = 0;
	}
	else if (strcmp(claProtocol, "udp") == 0)
	{
		portNumber = BpUdpDefaultPortNbr;
		inductDaemon = "udpcli";
		outductDaemon = "udpclo";
		maxPayloadLength = 65000;
	}
	else
	{
		writeMemoNote("[?] Neighbor discovery protocol name error",
				claProtocol);
		return -1;
	}

	fetchProtocol(claProtocol, &protocol, &elt);
	if (elt == 0)		/*	Protocol not yet loaded.	*/
	{
		if (addProtocol(claProtocol, 1400, 100, 0) < 0)
		{
			putErrmsg("Can't add protocol for discovered contacts.",
					NULL);
			return -1;
		}

		fetchProtocol(claProtocol, &protocol, &elt);

		/*	Add induct for CLA and start it.		*/

		isprintf(inductName, sizeof inductName, "0.0.0.0:%d",
				portNumber);
		if (addInduct(protocol.name, inductName, inductDaemon) < 0)
		{
			putErrmsg("Can't add induct.", inductName);
			return -1;
		}

		if (bpStartInduct(protocol.name, inductName) < 0)
		{
			putErrmsg("Can't start induct.", inductName);
			return -1;
		}
	}

	if (cbhe)
	{
		/*	Insert contact into contact plan.  This is
	 	 *	to enable CGR, regardless of outduct protocol.	*/

		if (noteContactAcquired(discoveryNodeNbr, xmitRate, recvRate))
		{
			putErrmsg("Can't note contact discovered.",
					discoveryEid);
			return -1;
		}
	}

	/*	Add outduct and start it.				*/

	if (addOutduct(claProtocol, socketSpec, outductDaemon,
				maxPayloadLength) < 0)
	{
		putErrmsg("Can't add outduct.", ductExpression);
		return -1;
	}

	findOutduct(claProtocol, socketSpec, &vduct, &vductElt);
	CHKERR(vductElt);
	if (bpStartOutduct(claProtocol, socketSpec) < 0)
	{
		putErrmsg("Can't start outduct.", ductExpression);
		return -1;
	}

	/*	Add plan, attach duct, start it.			*/

	if (addPlan(discoveryEid, ION_DEFAULT_XMIT_RATE) < 0)
	{
		putErrmsg("Can't add egress plan.", discoveryEid);
		return -1;
	}

	if (attachPlanDuct(discoveryEid, vduct->outductElt) < 0)
	{
		putErrmsg("Can't add plan duct.", discoveryEid);
		return -1;
	}

	if (bpStartPlan(discoveryEid) < 0)
	{
		putErrmsg("Can't start egress plan.", discoveryEid);
		return -1;
	}

	/*	Add the discovery.					*/

	if (addDiscovery(discoveryEid) < 0)
	{
		putErrmsg("Can't add discovered neighbor.", discoveryEid);
		return -1;
	}

	return 0;
}

int	bp_discovery_acquired(char *socketSpec, char *discoveryEid,
		char *claProtocol, unsigned int xmitRate, unsigned int recvRate)
{
	Sdr	sdr = getIonsdr();
	int	result;

	oK(sdr_begin_xn(sdr));
	result = discoveryAcquired(socketSpec, discoveryEid, claProtocol,
			xmitRate, recvRate);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("bp_discover_contact_acquired failed.", NULL);
		return -1;
	}

	return result;
}

static int	noteContactLost(uvast discoveryNodeNbr, time_t startTime)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();
	IonVdb		*ionvdb = getIonVdb();
	uvast		ownNodeNbr = getOwnNodeNbr();
	int		regionIdx;
	IonNeighbor	*neighbor;
	IonCXref	arg;
	PsmAddress	elt;
	PsmAddress	contactAddr;
	IonCXref	*cxref;
	Object		contactObj;
	IonContact	contact;
	time_t		currentTime;

	/*	Note: this is where discovered contacts get
	 *	inserted into ION's distributed history of
	 *	contacts.  However, to prevent double-posting
	 *	of contacts (which might disagree due to clock
	 *	misalignment between nodes), we only post new
	 *	discovered contacts from whichever participating
	 *	node has got the lower node number.			*/

	currentTime = getCtime();
	regionIdx = ionRegionOf(discoveryNodeNbr, ownNodeNbr);
	if (regionIdx < 0)
	{
		writeMemoNote("[?] Can't lose contact to node; region unknown.",
				itoa(discoveryNodeNbr));
		return 0;
	}

	neighbor = getNeighbor(ionvdb, discoveryNodeNbr);
	CHKZERO(neighbor);

	/*	Find matching discovered contact TO neighbor.		*/

	cxref = NULL;
	memset((char *) &arg, 0, sizeof(IonCXref));
	arg.fromNode = ownNodeNbr;
	arg.toNode = discoveryNodeNbr;
	oK(sm_rbt_search(ionwm, ionvdb->contactIndex, rfx_order_contacts,
			&arg, &elt));
	if (elt)		/*	Found discovered contact.	*/
	{
		contactAddr = sm_rbt_data(ionwm, elt);
		cxref = (IonCXref *) psp(ionwm, contactAddr);
	}

	if (cxref == NULL)		/*	Functional error.	*/
	{
		putErrmsg("Discovered contact not found!",
				itoa(discoveryNodeNbr));
	}
	else
	{
		if (cxref->type == CtDiscovered)
		{
			if (ownNodeNbr < discoveryNodeNbr)
			{
				saga_insert(startTime, currentTime,
					ownNodeNbr, discoveryNodeNbr,
					cxref->xmitRate, regionIdx);
			}

			CHKERR(sdr_begin_xn(sdr));
			contactObj = sdr_list_data(sdr, cxref->contactElt);
			sdr_stage(sdr, (char *) &contact, contactObj,
					sizeof(IonContact));
			contact.fromTime = 0;
			contact.xmitRate = 0;
			contact.confidence = 0.0;
			contact.type = CtHypothetical;
			contact.mtv[0] = 0.0;
			contact.mtv[1] = 0.0;
			contact.mtv[2] = 0.0;
			sdr_write(sdr, contactObj, (char *) &contact,
					sizeof(IonContact));
			cxref->fromTime = 0;
			cxref->xmitRate = 0;
			cxref->confidence = 0.0;
			cxref->type = CtHypothetical;
			neighbor->xmitRate = 0;
			toggleScheduledContacts(ownNodeNbr, discoveryNodeNbr,
					CtSuppressed, CtScheduled);
			CHKERR(sdr_end_xn(sdr) == 0);
		}
	}

	/*	Find matching discovered contact FROM neighbor.		*/

	cxref = NULL;
	memset((char *) &arg, 0, sizeof(IonCXref));
	arg.fromNode = discoveryNodeNbr;
	arg.toNode = ownNodeNbr;
	oK(sm_rbt_search(ionwm, ionvdb->contactIndex, rfx_order_contacts,
			&arg, &elt));
	if (elt)	/*	Found discovered contact.		*/
	{
		contactAddr = sm_rbt_data(ionwm, elt);
		cxref = (IonCXref *) psp(ionwm, contactAddr);
	}

	if (cxref == NULL)		/*	Functional error.	*/
	{
		putErrmsg("Discovered contact not found!",
				itoa(discoveryNodeNbr));
	}
	else
	{
		if (cxref->type == CtDiscovered)
		{
			if (ownNodeNbr < discoveryNodeNbr)
			{
				saga_insert(startTime, currentTime,
					discoveryNodeNbr, ownNodeNbr,
					cxref->xmitRate, regionIdx);
			}

			CHKERR(sdr_begin_xn(sdr));
			contactObj = sdr_list_data(sdr, cxref->contactElt);
			sdr_stage(sdr, (char *) &contact, contactObj,
					sizeof(IonContact));
			contact.fromTime = 0;
			contact.xmitRate = 0;
			contact.confidence = 0.0;
			contact.type = CtHypothetical;
			contact.mtv[0] = 0.0;
			contact.mtv[1] = 0.0;
			contact.mtv[2] = 0.0;
			sdr_write(sdr, contactObj, (char *) &contact,
					sizeof(IonContact));
			cxref->fromTime = 0;
			cxref->xmitRate = 0;
			cxref->confidence = 0.0;
			cxref->type = CtHypothetical;
			neighbor->fireRate = 0;
			neighbor->recvRate = 0;
			toggleScheduledContacts(discoveryNodeNbr, ownNodeNbr,
					CtSuppressed, CtScheduled);
			CHKERR(sdr_end_xn(sdr) == 0);
		}
	}

	return 0;
}

static int	discoveryLost(char *socketSpec, char *discoveryEid,
			char *claProtocol)
{
	PsmPartition	ionwm = getIonwm();
	PsmAddress	elt;
	PsmAddress	discoveryAddr;
	Discovery	*discovery;
	int		result;
	MetaEid		metaEid;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	uvast		discoveryNodeNbr;
	int		cbhe = 0;

	CHKERR(socketSpec);
	CHKERR(*socketSpec);
	CHKERR(discoveryEid);
	CHKERR(claProtocol);
	elt = bp_find_discovery(discoveryEid);
	if (elt == 0)
	{
		return 0;	/*	Neighbor not discovered.	*/
	}

	discoveryAddr = sm_list_data(ionwm, elt);
	discovery = (Discovery *) psp(ionwm, discoveryAddr);
	result = parseEidString(discoveryEid, &metaEid, &vscheme, &vschemeElt);
	if (result == 0)
	{
		writeMemoNote("[?] Neighbor loss discovery EID error",
				discoveryEid);
		return -1;
	}

	discoveryNodeNbr = metaEid.nodeNbr;
	if (strcmp(metaEid.schemeName, "ipn") == 0)
	{
		cbhe = 1;
	}

	restoreEidString(&metaEid);

	/*	Stop transmission of bundles via this contact.		*/

	bpStopPlan(discoveryEid);

	/*	No need to detach ducts, because removePlan will
	 *	automatically do this.					*/

	removePlan(discoveryEid);
	bpStopOutduct(claProtocol, socketSpec);
	removeOutduct(claProtocol, socketSpec);

	/*	Fix up NDP data.					*/

	if (cbhe)
	{
		/*	For discovered ipn-scheme EID's egress plan,
		 *	must manage contact as well as plan.		*/

		if (noteContactLost(discoveryNodeNbr,
				discovery->startOfContact) < 0)
		{
			putErrmsg("Can't note contact lost.", discoveryEid);
			return -1;
		}
	}

	deleteDiscovery(discoveryEid);
	return 0;
}

int	bp_discovery_lost(char *socketSpec, char *discoveryEid,
		char *claProtocol)
{
	Sdr	sdr = getIonsdr();
	int	result;

	oK(sdr_begin_xn(sdr));
	result = discoveryLost(socketSpec, discoveryEid, claProtocol);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("bp_discovery_lost failed.", NULL);
		return -1;
	}

	return result;
}

PsmAddress	bp_find_discovery(char *eid)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();
	BpVdb		*vdb = getBpVdb();
	PsmAddress	discovery;

	oK(sdr_begin_xn(sdr));		/*	Just to lock database.	*/
	discovery = sm_list_search(ionwm, sm_list_first(ionwm,
			vdb->discoveries), compareDiscoveries, eid);
	sdr_exit_xn(sdr);
	return discovery;
}
