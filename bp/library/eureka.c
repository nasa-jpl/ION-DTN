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

static int	getOutduct(char *protocolName, char *outductName,
			char *daemon, VOutduct **vduct, PsmAddress *vductElt)
{
	char	namebuf[SDRSTRING_BUFSZ];

	findOutduct(protocolName, outductName, vduct, vductElt);
	if (*vductElt == 0)
	{
		isprintf(namebuf, sizeof namebuf, "%s/%s", protocolName,
				outductName);
		if (addOutduct(protocolName, outductName, daemon, 0) < 1)
		{
			putErrmsg("Can't add outduct.", namebuf);
			return -1;
		}

		if (bpStartOutduct(protocolName, outductName) < 0)
		{
			putErrmsg("Can't start outduct.", namebuf);
			return -1;
		}

		if (bpBlockOutduct(protocolName, outductName) < 0)
		{
			putErrmsg("Can't block outduct.", namebuf);
			return -1;
		}

		findOutduct(protocolName, outductName, vduct, vductElt);
		if (*vductElt == 0)
		{
			putErrmsg("Can't find newly added outduct.", namebuf);
			return -1;
		}
	}

	return 0;
}

static int	addIpnNeighbor(uvast nodeNbr, char *neighborEid,
			ClProtocol *claProtocol, char *socketSpec, 
			unsigned int xmitRate, unsigned int recvRate)
{
	uvast		ownNodeNbr = getOwnNodeNbr();
	char		*outductName;
	char		*outductDaemon;
	VOutduct	*vduct;
	PsmAddress	vductElt;
	DuctExpression	ductExpression;
	time_t		currentTime;
	PsmAddress	xaddr;

	/*	Add egress plan for the new neighbor.			*/

	if (strcmp(claProtocol->name, "udp") == 0)
	{
		/*	Egress plan cites the protocol's promiscuous
		 *	outduct.					*/

		outductName = "*";
		outductDaemon = "udpclo";
		ductExpression.destDuctName = socketSpec;
	}
	else	/*	TCP.						*/
	{
		outductName = socketSpec;
		outductDaemon = NULL;
		ductExpression.destDuctName = NULL;
	}

	if (getOutduct(claProtocol->name, outductName, outductDaemon, &vduct,
			&vductElt) < 0)
	{
		putErrmsg("No outduct for ipn neighbor.", neighborEid);
		return -1;
	}

	ductExpression.outductElt = vduct->outductElt;
	if (ipn_addPlan(nodeNbr, &ductExpression) < 0)
	{
		putErrmsg("Can't add plan for discovery.", neighborEid);
		return -1;
	}

	/*	Insert contact into contact plan.  This is to enable
	 *	CGR, regardless of outduct protocol.			*/

	currentTime = getUTCTime();
	if (rfx_insert_contact(currentTime, 0, ownNodeNbr, nodeNbr, xmitRate,
			1.0, &xaddr) < 0 || xaddr == 0)
	{
		putErrmsg("Can't add transmission contact.", neighborEid);
		return -1;
	}

	if (rfx_insert_contact(currentTime, 0, nodeNbr, ownNodeNbr, recvRate,
			1.0, &xaddr) < 0 || xaddr == 0)
	{
		putErrmsg("Can't add reception contact.", neighborEid);
		return -1;
	}

	/*	Add the NDP neighbor.					*/

	if (addNdpNeighbor(neighborEid) < 0)
	{
		putErrmsg("Can't add discovered Neighbor.", neighborEid);
		return -1;
	}

	/*	Unblock outduct to this neighbor, enabling reforward.	*/

	if (bpUnblockOutduct(claProtocol->name, outductName) < 0)
	{
		putErrmsg("Can't unblock outduct.", neighborEid);
		return -1;
	}

	return 0;
}

static int	addDtn2Neighbor(char *neighborEid, ClProtocol *claProtocol,
			char *socketSpec, unsigned int xmitRate,
			unsigned int recvRate)
{
	Sdr		sdr = getIonsdr();
	char		*outductName;
	char		*outductDaemon;
	VOutduct	*vduct;
	PsmAddress	vductElt;
	FwdDirective	directive;

	/*	Add egress plan for the new neighbor.			*/

	directive.action = xmit;
	if (strcmp(claProtocol->name, "udp") == 0)
	{
		/*	Egress plan cites the protocol's promiscuous
		 *	outduct.					*/

		directive.protocolClass = BP_PROTOCOL_UNRELIABLE;
		outductName = "*";
		outductDaemon = "udpclo";
		directive.destDuctName = sdr_string_create(sdr, socketSpec);
		if (directive.destDuctName == 0)
		{
			putErrmsg("Can't note UDP destination duct name.",
					socketSpec);
			return -1;
		}
	}
	else	/*	TCP.						*/
	{
		directive.protocolClass = BP_PROTOCOL_RELIABLE;
		outductName = socketSpec;
		outductDaemon = NULL;
		directive.destDuctName = 0;
	}

	if (getOutduct(claProtocol->name, outductName, outductDaemon, &vduct,
			&vductElt) < 0)
	{
		putErrmsg("No outduct for dtn2 neighbor.", neighborEid);
		return -1;
	}

	directive.outductElt = vduct->outductElt;
	if (dtn2_addPlan(neighborEid, &directive) < 0)
	{
		putErrmsg("Can't add plan for discovery.", neighborEid);
		return -1;
	}

	/*	Add the NDP neighbor.					*/

	if (addNdpNeighbor(neighborEid) < 0)
	{
		putErrmsg("Can't add discovered Neighbor.", neighborEid);
		return -1;
	}

	/*	Unblock outduct to this neighbor, enabling reforward.	*/

	if (bpUnblockOutduct(claProtocol->name, outductName) < 0)
	{
		putErrmsg("Can't unblock outduct.", neighborEid);
		return -1;
	}

	return 0;
}

static int	discoverContactAcquired(char *socketSpec, char *neighborEid,
			char *claProtocol, unsigned int xmitRate,
			unsigned int recvRate)
{
	int		result;
	MetaEid		metaEid;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	int		portNumber;
	char		*inductDaemon;
	ClProtocol	protocol;
	Object		elt;
	unsigned int	ipAddress;
	char		ipAddressString[16];
	char		inductName[32];
	uvast		nodeNbr;

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

	if (strcmp(claProtocol, "tcp") == 0)
	{
		portNumber = BpTcpDefaultPortNbr;
		inductDaemon = "tcpcli";
	}
	else if (strcmp(claProtocol, "udp") == 0)
	{
		portNumber = BpUdpDefaultPortNbr;
		inductDaemon = "udpcli";
	}
	else
	{
		writeMemoNote("[?] Neighbor discovery protocol name error",
				claProtocol);
		return -1;
	}

	fetchProtocol(claProtocol, &protocol, &elt);
	if (elt == 0)
	{
		if (addProtocol(claProtocol, 1400, 100, -1, 0) < 0)
		{
			putErrmsg("Can't add protocol for discovered contacts.",
					NULL);
			return -1;
		}

		fetchProtocol(claProtocol, &protocol, &elt);
		ipAddress = getAddressOfHost();
		if (ipAddress == 0)
		{
			putErrmsg("Can't get address of local host.", NULL);
			return -1;
		}

		/*	Add induct for CLA and start it.		*/

		printDottedString(ipAddress, ipAddressString);
		isprintf(inductName, sizeof inductName, "%s:%d",
				ipAddressString, portNumber);
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

	if (strcmp(metaEid.schemeName, "dtn") == 0)
	{
		restoreEidString(&metaEid);
		return addDtn2Neighbor(neighborEid, &protocol, socketSpec,
				xmitRate, recvRate);
	}
	else if (strcmp(metaEid.schemeName, "ipn") == 0)
	{
		nodeNbr = metaEid.nodeNbr;
		restoreEidString(&metaEid);
		return addIpnNeighbor(nodeNbr, neighborEid, &protocol,
				socketSpec, xmitRate, recvRate);
	}
	else
	{
		restoreEidString(&metaEid);
		writeMemoNote("[?] Neighbor discovery unsupported scheme name",
				neighborEid);
		return -1;
	}
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
	int		result;
	MetaEid		metaEid;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	uvast		neighborNodeNbr;

	CHKERR(socketSpec);
	CHKERR(*socketSpec);
	CHKERR(neighborEid);
	CHKERR(claProtocol);
	result = parseEidString(neighborEid, &metaEid, &vscheme, &vschemeElt);
	if (result == 0)
	{
		writeMemoNote("[?] Neighbor loss discovery neighbor EID error",
				neighborEid);
		return -1;
	}

	neighborNodeNbr = metaEid.nodeNbr;
	restoreEidString(&metaEid);
	deleteNdpNeighbor(neighborEid);
	if (strcmp(claProtocol, "tcp") == 0)
	{
		if (bpBlockOutduct("tcp", socketSpec) < 0)
		{
			putErrmsg("Can't block TCP outduct.", socketSpec);
			return -1;
		}
	}
	else
	{
		if (strcmp(claProtocol, "udp") != 0)
		{
			writeMemoNote("[?] Neighbor loss discovery protocol \
name error", claProtocol);

		/*	No need to return at this point; can continue
		 *	removing neighbor.				*/
		}
	}

	/*	Remove discovered egress plan for this neighbor's EID.	*/

	if (strcmp(metaEid.schemeName, "dtn") == 0)
	{
		return dtn2_removePlan(neighborEid);
	}

	if (strcmp(metaEid.schemeName, "ipn") != 0)
	{
		writeMemoNote("[?] Neighbor discovery unsupported scheme name",
				neighborEid);
		return -1;
	}

	/*	For discovered ipn-scheme EID's egress plan, must
	 *	manage contact and range as well as plan.		*/

	if (ipn_removePlan(neighborNodeNbr) < 0)
	{
		return -1;
	}

	if (rfx_remove_discovered_contacts(neighborNodeNbr) < 0)
	{
		putErrmsg("Can't remove applicable contacts.", neighborEid);
		return -1;
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
