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
	discovery->startOfContact = getUTCTime();
	discovery->lastContactTime = getUTCTime();
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
	IonCXref	arg;
	PsmAddress	elt;
	PsmAddress	contactAddr;
	IonCXref	*contact;

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
			contact->type = toType;
		}
	}
}

static int	noteContactAcquired(uvast discoveryNodeNbr,
			unsigned int xmitRate, unsigned int recvRate)
{
	PsmPartition	ionwm = getIonwm();
	IonVdb		*ionvdb = getIonVdb();
	uvast		ownNodeNbr = getOwnNodeNbr();
	int		regionIdx;
	IonCXref	arg;
	PsmAddress	elt;
	PsmAddress	contactAddr;
	IonCXref	*contact;
	time_t		currentTime;
	PsmAddress	xaddr;
	uvast		nodeNbrA;
	uvast		nodeNbrB;

	regionIdx = ionRegionOf(discoveryNodeNbr, ownNodeNbr);
	if (regionIdx < 0)
	{
		writeMemo("Not adding contact for node; region unknown.",
				itoa(discoveryNodeNbr));
		return 0;
	}

	currentTime = getUTCTime();

	/*	Find matching latent contact TO discovered neighbor.	*/

	contact = NULL;
	memset((char *) &arg, 0, sizeof(IonCXref));
	arg.fromNode = ownNodeNbr;
	arg.toNode = discoveryNodeNbr;
	oK(sm_rbt_search(ionwm, ionvdb->contactIndex, rfx_order_contacts,
			&arg, &elt));
	if (elt)
	{
		contactAddr = sm_rbt_data(ionwm, elt);
		contact = (IonCXref *) psp(ionwm, contactAddr);
		if (contact->fromNode > ownNodeNbr
		|| contact->toNode > discoveryNodeNbr)
		{
			contact = NULL;	/*	No latent contact.	*/
		}
	}

	if (contact == NULL)	/*	Must insert latent contact.	*/
	{
		if (rfx_insert_contact(regionIdx, 0, MAX_POSIX_TIME, ownNodeNbr,
			discoveryNodeNbr, 0, 0.0, &xaddr) < 0
		|| xaddr == 0)
		{
			putErrmsg("Can't add latent contact.", discoveryEid);
			return -1;
		}

		contact = (IonCXref *) psp(ionwm, xaddr);
	}

	if (contact->type == CtLatent)
	{
		contact->type = CtDiscovered;
		contact->xmitRate = xmitRate;
		contact->confidence = 1.0;
		toggleScheduledContacts(ownNodeNbr, discoveryNodeNbr,
				CtScheduled, CtSuppressed);
	}

	/*	Find matching latent contact FROM discovered neighbor.	*/

	contact = NULL;
	memset((char *) &arg, 0, sizeof(IonCXref));
	arg.fromNode = discoveryNodeNbr;
	arg.toNode = ownNodeNbr;
	oK(sm_rbt_search(ionwm, ionvdb->contactIndex, rfx_order_contacts,
			&arg, &elt));
	if (elt)
	{
		contactAddr = sm_rbt_data(ionwm, elt);
		contact = (IonCXref *) psp(ionwm, contactAddr);
		if (contact->fromNode > discoveryNodeNbr
		|| contact->toNode > ownNodeNbr)
		{
			contact = NULL;	/*	No latent contact.	*/
		}
	}

	if (contact == NULL)	/*	Must insert latent contact.	*/
	{
		if (rfx_insert_contact(regionIdx, 0, MAX_POSIX_TIME,
			discoveryNodeNbr, ownNodeNbr, 0, 0.0, &xaddr) < 0
		|| xaddr == 0)
		{
			putErrmsg("Can't add latent contact.", discoveryEid);
			return -1;
		}

		contact = (IonCXref *) psp(ionwm, xaddr);
	}

	if (contact->type == CtLatent)
	{
		contact->type = CtDiscovered;
		contact->xmitRate = recvRate;
		contact->confidence = 1.0;
		toggleScheduledContacts(discoveryNodeNbr, ownNodeNbr,
				CtScheduled, CtSuppressed);
	}

	return 0;
}

static int	discoveryAcquired(char *socketSpec, char *discoveryEid,
			char *claProtocol, unsigned int xmitRate,
			unsigned int recvRate)
{
	uvast		ownNodeNbr = getOwnNodeNbr();
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
		putErrmsg("Can't add discovered Neighbor.", discoveryEid);
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
	PsmPartition	ionwm = getIonwm();
	IonVdb		*ionvdb = getIonVdb();
	uvast		ownNodeNbr = getOwnNodeNbr();
	int		regionIdx;
	IonCXref	arg;
	PsmAddress	elt;
	PsmAddress	contactAddr;
	IonCXref	*contact;
	time_t		currentTime;
	PsmAddress	xaddr;
	uvast		nodeNbrA;
	uvast		nodeNbrB;

	currentTime = getUTCTime();
	regionIdx = ionRegionOf(discoveryNodeNbr, ownNodeNbr);

	/*	Find matching discovered contact TO neighbor.		*/

	contact = NULL;
	memset((char *) &arg, 0, sizeof(IonCXref));
	arg.fromNode = ownNodeNbr;
	arg.toNode = discoveryNodeNbr;
	oK(sm_rbt_search(ionwm, ionvdb->contactIndex, rfx_order_contacts,
			&arg, &elt));
	if (elt)
	{
		contactAddr = sm_rbt_data(ionwm, elt);
		contact = (IonCXref *) psp(ionwm, contactAddr);
		if (contact->fromNode > ownNodeNbr
		|| contact->toNode > discoveryNodeNbr)
		{
			contact = NULL;	/*	No discovered contact.	*/
		}
	}

	if (contact == NULL)		/*	Functional error.	*/
	{
		putErrmsg("Discovered contact not found!",
				itoa(discoveryNodeNbr));
	}
	else
	{
		if (contact->type == CtDiscovered)
		{
			rfx_log_discovered_contact(startTime, currentTime,
					ownNodeNbr, discoveryNodeNbr,
					contact->xmitRate, regionIdx);
			contact->type = CtLatent;
			contact->xmitRate = 0;
			contact->confidence = 0.0;
			toggleScheduledContacts(ownNodeNbr, discoveryNodeNbr,
					CtSuppressed, CtScheduled);
		}
	}

	/*	Find matching discovered contact FROM neighbor.		*/

	contact = NULL;
	memset((char *) &arg, 0, sizeof(IonCXref));
	arg.fromNode = discoveryNodeNbr;
	arg.toNode = ownNodeNbr;
	oK(sm_rbt_search(ionwm, ionvdb->contactIndex, rfx_order_contacts,
			&arg, &elt));
	if (elt)
	{
		contactAddr = sm_rbt_data(ionwm, elt);
		contact = (IonCXref *) psp(ionwm, contactAddr);
		if (contact->fromNode > discoveryNodeNbr
		|| contact->toNode > ownNodeNbr)
		{
			contact = NULL;	/*	No discovered contact.	*/
		}
	}

	if (contact == NULL)		/*	Functional error.	*/
	{
		putErrmsg("Discovered contact not found!",
				itoa(discoveryNodeNbr));
	}
	else
	{
		if (contact->type == CtDiscovered)
		{
			rfx_log_discovered_contact(startTime, currentTime,
					discoveryNodeNbr, ownNodeNbr
					contact->xmitRate, regionIdx);
			contact->type = CtLatent;
			contact->xmitRate = 0;
			contact->confidence = 0.0;
			toggleScheduledContacts(discoveryNodeNbr, ownNodeNbr,
					CtSuppressed, CtScheduled);
		}
	}

	return 0;
}

static int	discoveryLost(char *socketSpec, char *discoveryEid,
			char *claProtocol)
{
	uvast		ownNodeNbr = getOwnNodeNbr();
	PsmAddress	elt;
	PsmAddress	discoveryAddr;
	int		result;
	MetaEid		metaEid;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	uvast		discoveryNodeNbr;
	int		cbhe = 0;
	uvast		nodeNbrA;
	uvast		nodeNbrB;

	CHKERR(socketSpec);
	CHKERR(*socketSpec);
	CHKERR(discoveryEid);
	CHKERR(claProtocol);
	elt = bp_find_discovery(discoveryEid);
	if (elt == 0)
	{
		return 0;	/*	Neighbor not discovered.	*/
	}

	discoveryAddr = sm_list_data(wm, elt);
	discovery = (Discovery *) psp(wm, discoveryAddr);
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
	PsmPartition	wm = getIonwm();
	BpVdb		*vdb = getBpVdb();
	PsmAddress	discovery;

	oK(sdr_begin_xn(sdr));		/*	Just to lock database.	*/
	discovery = sm_list_search(wm, sm_list_first(wm, vdb->discoveries),
			compareDiscoveries, eid);
	sdr_exit_xn(sdr);
	return discovery;
}
