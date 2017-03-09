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

static int 	compareNeighbors(PsmPartition partition, PsmAddress eltData,
			void *argData)
{
	NdpNeighbor	 *neighbor = psp(partition, eltData);
	char		*targetEid = (char *) argData;

	return strncmp(neighbor->eid, targetEid, MAX_EID_LEN);
}

static int	addNdpNeighbor(char *eid)
{
	PsmPartition	wm = getIonwm();
	BpVdb		*vdb = getBpVdb();
	PsmAddress	elt;
	PsmAddress	neighborAddr;
	NdpNeighbor	*neighbor;

	elt = bp_discover_find_neighbor(eid);
	if (elt)
	{
		return 0;
	}

	neighborAddr = psm_malloc(wm, sizeof(NdpNeighbor));
	if (neighborAddr == 0)
	{
		putErrmsg("Can't add NdpNeighbor.", eid);
		return -1;
	}

	neighbor = (NdpNeighbor *) psp(wm, neighborAddr);
	istrcpy(neighbor->eid, eid, sizeof neighbor->eid);
	neighbor->lastContactTime = getUTCTime();
	if (sm_list_insert(wm, vdb->neighbors, neighborAddr, compareNeighbors,
			eid) == 0)
	{
		putErrmsg("Can't add NdpNeighbor.", eid);
		return -1;
	}

	return 0;
}

static void	deleteNdpNeighbor(char *eid)
{
	PsmPartition	wm = getIonwm();
	PsmAddress	elt;
	PsmAddress	neighborAddr;

	elt = bp_discover_find_neighbor(eid);
	if (elt)
	{
		neighborAddr = sm_list_data(wm, elt);
		psm_free(wm, neighborAddr);
		sm_list_delete(wm, elt, NULL, NULL);
	}
}

static int	discoverContactAcquired(char *socketSpec, char *neighborEid,
			char *claProtocol, unsigned int xmitRate,
			unsigned int recvRate)
{
	uvast		ownNodeNbr = getOwnNodeNbr();
	int		result;
	MetaEid		metaEid;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	uvast		neighborNodeNbr;
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
	time_t		currentTime;
	PsmAddress	xaddr;
	uvast		nodeNbrA;
	uvast		nodeNbrB;

	CHKERR(socketSpec);
	CHKERR(*socketSpec);
	CHKERR(neighborEid);
	CHKERR(claProtocol);
	CHKERR(xmitRate);
	CHKERR(recvRate);
	result = parseEidString(neighborEid, &metaEid, &vscheme, &vschemeElt);
	if (result == 0)
	{
		writeMemoNote("[?] Neighbor discovery neighbor EID error",
				neighborEid);
		return -1;
	}

	neighborNodeNbr = metaEid.nodeNbr;
	if (strcmp(metaEid.schemeName, "ipn") == 0)
	{
		cbhe = 1;
	}

	restoreEidString(&metaEid);
	elt = bp_discover_find_neighbor(neighborEid);
	if (elt)
	{
		writeMemoNote("[?] Neighbor previously discovered",
				neighborEid);
		return 0;
	}

	findPlan(neighborEid, &vplan, &vplanElt);
	if (vplanElt)
	{
		writeMemoNote("[?] Neighbor is managed; no discovery",
				neighborEid);
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

	/*	Insert contact and range into contact plan.  This is
	 *	to enable CGR, regardless of outduct protocol.		*/

	if (cbhe)
	{
		currentTime = getUTCTime();
		if (rfx_insert_contact(currentTime, 0, ownNodeNbr,
				neighborNodeNbr, xmitRate, 1.0, &xaddr) < 0
		|| xaddr == 0)
		{
			putErrmsg("Can't add transmission contact.",
					neighborEid);
			return -1;
		}

		if (rfx_insert_contact(currentTime, 0, neighborNodeNbr,
				ownNodeNbr, recvRate, 1.0, &xaddr) < 0
		|| xaddr == 0)
		{
			putErrmsg("Can't add reception contact.", neighborEid);
			return -1;
		}

		if (ownNodeNbr < neighborNodeNbr)
		{
			nodeNbrA = ownNodeNbr;
			nodeNbrB = neighborNodeNbr;
		}
		else
		{
			nodeNbrA = neighborNodeNbr;
			nodeNbrB = ownNodeNbr;
		}

		if (rfx_insert_range(currentTime, 0, nodeNbrA, nodeNbrB, 0,
				&xaddr) < 0 || xaddr == 0)
		{
			putErrmsg("Can't add range.", neighborEid);
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

	if (addPlan(neighborEid, ION_DEFAULT_XMIT_RATE) < 0)
	{
		putErrmsg("Can't add egress plan.", neighborEid);
		return -1;
	}

	if (attachPlanDuct(neighborEid, vduct->outductElt) < 0)
	{
		putErrmsg("Can't add plan duct.", neighborEid);
		return -1;
	}

	if (bpStartPlan(neighborEid) < 0)
	{
		putErrmsg("Can't start egress plan.", neighborEid);
		return -1;
	}

	/*	Add the NDP neighbor.					*/

	if (addNdpNeighbor(neighborEid) < 0)
	{
		putErrmsg("Can't add discovered Neighbor.", neighborEid);
		return -1;
	}

	return 0;
}

int	bp_discover_contact_acquired(char *socketSpec, char *neighborEid,
		char *claProtocol, unsigned int xmitRate, unsigned int recvRate)
{
	Sdr	sdr = getIonsdr();
	int	result;

	oK(sdr_begin_xn(sdr));
	result = discoverContactAcquired(socketSpec, neighborEid, claProtocol,
			xmitRate, recvRate);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("bp_discover_contact_acquired failed.", NULL);
		return -1;
	}

	return result;
}

static int	discoverContactLost(char *socketSpec, char *neighborEid,
			char *claProtocol)
{
	uvast		ownNodeNbr = getOwnNodeNbr();
	PsmAddress	elt;
	int		result;
	MetaEid		metaEid;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	uvast		neighborNodeNbr;
	int		cbhe = 0;
	uvast		nodeNbrA;
	uvast		nodeNbrB;

	CHKERR(socketSpec);
	CHKERR(*socketSpec);
	CHKERR(neighborEid);
	CHKERR(claProtocol);
	elt = bp_discover_find_neighbor(neighborEid);
	if (elt == 0)
	{
		return 0;	/*	Neighbor not discovered.	*/
	}

	result = parseEidString(neighborEid, &metaEid, &vscheme, &vschemeElt);
	if (result == 0)
	{
		writeMemoNote("[?] Neighbor loss discovery neighbor EID error",
				neighborEid);
		return -1;
	}

	neighborNodeNbr = metaEid.nodeNbr;
	if (strcmp(metaEid.schemeName, "ipn") == 0)
	{
		cbhe = 1;
	}

	restoreEidString(&metaEid);

	/*	Stop transmission of bundles via this contact.		*/

	bpStopPlan(neighborEid);

	/*	No need to detach ducts, because removePlan will
	 *	automatically do this.					*/

	removePlan(neighborEid);
	bpStopOutduct(claProtocol, socketSpec);
	removeOutduct(claProtocol, socketSpec);

	/*	Fix up NDP database.					*/

	deleteNdpNeighbor(neighborEid);

	/*	For discovered ipn-scheme EID's egress plan, must
	 *	manage contact and range as well as plan.		*/

	if (cbhe)
	{
		if (rfx_remove_discovered_contacts(neighborNodeNbr) < 0)
		{
			putErrmsg("Can't remove applicable contacts.",
					neighborEid);
			return -1;
		}

		if (ownNodeNbr < neighborNodeNbr)
		{
			nodeNbrA = ownNodeNbr;
			nodeNbrB = neighborNodeNbr;
		}
		else
		{
			nodeNbrA = neighborNodeNbr;
			nodeNbrB = ownNodeNbr;
		}

		if (rfx_remove_range(0, nodeNbrA, nodeNbrB) < 0)
		{
			putErrmsg("Can't remove range.", neighborEid);
			return -1;
		}
	}

	return 0;
}

int	bp_discover_contact_lost(char *socketSpec, char *neighborEid,
		char *claProtocol)
{
	Sdr	sdr = getIonsdr();
	int	result;

	oK(sdr_begin_xn(sdr));
	result = discoverContactLost(socketSpec, neighborEid, claProtocol);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("bp_discover_contact_lost failed.", NULL);
		return -1;
	}

	return result;
}

PsmAddress	bp_discover_find_neighbor(char *eid)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	wm = getIonwm();
	BpVdb		*vdb = getBpVdb();
	PsmAddress	neighbor;

	oK(sdr_begin_xn(sdr));		/*	Just to lock database.	*/
	neighbor = sm_list_search(wm, sm_list_first(wm, vdb->neighbors),
			compareNeighbors, eid);
	sdr_exit_xn(sdr);
	return neighbor;
}
