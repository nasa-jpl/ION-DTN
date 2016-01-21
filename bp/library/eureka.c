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
#include "tcpcla.h"
#include "udpcla.h"
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

static int	plansAgree(Object planObj, ClProtocol *claProtocol,
			char *outductName)
{
	Sdr		sdr = getIonsdr();
	IpnPlan		plan;
	Outduct		duct;
	ClProtocol	protocol;

	/*	Make sure this new contact matches the existing egress
	 *	plan for transmission to this neighboring node.		*/

	sdr_read(sdr, (char *) &plan, planObj, sizeof(IpnPlan));
	sdr_read(sdr, (char *) &duct, sdr_list_data(sdr,
			plan.defaultDirective.outductElt), sizeof(Outduct));
	sdr_read(sdr, (char *) &protocol, duct.protocol, sizeof(ClProtocol));
	if (strcmp(protocol.name, claProtocol->name) != 0)
	{
		writeMemoNote("[?] Neighbor discovery CL protocol conflict",
				claProtocol->name);
		return 0;
	}

	if (strcmp(duct.name, outductName) != 0)
	{
		writeMemoNote("[?] Neighbor discovery CL duct name conflict",
				outductName);
		return 0;
	}

	return 1;
}

static int	addOutductToNeighbor(ClProtocol *claProtocol, char *outductName,
			char *outductDaemon)
{
	if (strcmp(outductName, "*") == 0)
	{
		return 0;	/*	Promiscuous outduct exists.	*/
	}

	if (addOutduct(claProtocol->name, outductName, outductDaemon, 0) < 0)
	{
		putErrmsg("Can't add outduct.", outductName);
		return -1;
	}

	if (bpStartOutduct(claProtocol->name, outductName) < 0)
	{
		putErrmsg("Can't start outduct.", outductName);
		return -1;
	}

	if (bpBlockOutduct(claProtocol->name, outductName) < 0)
	{
		putErrmsg("Can't block outduct.", outductName);
		return -1;
	}

	return 0;
}

static int	addIpnNeighbor(uvast nodeNbr, char *neighborEid,
			ClProtocol *claProtocol, char *outductName,
			char *destDuctName, char *outductDaemon,
			unsigned int xmitRate, unsigned int recvRate)
{
	uvast		ownNodeNbr = getOwnNodeNbr();
	Object		planObj;
	Object		planElt;
	VOutduct	*vduct;
	PsmAddress	vductElt;
	DuctExpression	ductExpression;
	time_t		currentTime;

	ipn_findPlan(nodeNbr, &planObj, &planElt);
	if (planElt)	/*	Egress plan for this neighbor exists.	*/
	{
		if (!(plansAgree(planObj, claProtocol, outductName)))
		{
			writeMemo("[?] 'ipn' neighbor not added.");
			return 0;
		}
	}
	else	/*	This is a brand-new neighbor.			*/
	{
		/*	Add outduct as necessary.			*/

		if (addOutductToNeighbor(claProtocol, outductName,
				outductDaemon) < 0)
		{
			writeMemo("[?] 'ipn' neighbor outduct not added.");
			return 0;
		}

		/*	Add egress plan that cites the outduct.		*/

		findOutduct(claProtocol->name, outductName, &vduct, &vductElt);
		ductExpression.outductElt = vduct->outductElt;
		ductExpression.destDuctName = destDuctName;
		if (ipn_addPlan(nodeNbr, &ductExpression) < 0)
		{
			putErrmsg("Can't add plan for discovery.", outductName);
			return -1;
		}
	}

	/*	Insert contact into contact plan.			*/

	currentTime = getUTCTime();
	if (rfx_insert_contact(currentTime, MAX_POSIX_TIME, ownNodeNbr,
			nodeNbr, xmitRate, 1.0) == 0)
	{
		putErrmsg("Can't add transmission contact.", outductName);
		return -1;
	}

	if (rfx_insert_contact(currentTime, MAX_POSIX_TIME, nodeNbr,
			ownNodeNbr, recvRate, 1.0) == 0)
	{
		putErrmsg("Can't add reception contact.", outductName);
		return -1;
	}

	if (addNdpNeighbor(neighborEid) < 0)
	{
		putErrmsg("Can't add discovered Neighbor.", neighborEid);
		return -1;
	}

	/*	Unblock outduct to this neighbor, enabling reforward.	*/

	if (bpUnblockOutduct(claProtocol->name, outductName) < 0)
	{
		putErrmsg("Can't unblock outduct.", outductName);
		return -1;
	}

	return 0;
}

static int	addDtn2Neighbor(char *neighborEid, ClProtocol *claProtocol,
			char *outductName, char *destDuctName,
			char *outductDaemon, unsigned int xmitRate,
			unsigned int recvRate)
{
	Sdr		sdr = getIonsdr();
	Object		planObj;
	Object		planElt;
	VOutduct	*vduct;
	PsmAddress	vductElt;
	FwdDirective	directive;

	dtn2_findPlan(neighborEid, &planObj, &planElt);
	if (planElt)	/*	Egress plan for this neighbor exists.	*/
	{
		if (!(plansAgree(planObj, claProtocol, outductName)))
		{
			writeMemo("[?] 'dtn' neighbor not added.");
			return 0;
		}
	}
	else	/*	This is a brand-new neighbor.			*/
	{
		/*	Add outduct as necessary.			*/

		if (addOutductToNeighbor(claProtocol, outductName,
				outductDaemon) < 0)
		{
			writeMemo("[?] 'dtn' neighbor outduct not added.");
			return 0;
		}

		/*	Add egress plan that cites the outduct.		*/

		directive.action = xmit;
		directive.protocolClass = claProtocol->protocolClass;
		findOutduct(claProtocol->name, outductName, &vduct, &vductElt);
		directive.outductElt = vduct->outductElt;
		if (destDuctName == 0)
		{
			directive.destDuctName = 0;
		}
		else
		{
			CHKERR(sdr_begin_xn(sdr));
			directive.destDuctName = sdr_string_create(sdr,
					destDuctName);
			if (sdr_end_xn(sdr) < 0)
			{
				putErrmsg("Can't note destination duct name.",
						destDuctName);
				return -1;
			}
		}

		if (dtn2_addPlan(neighborEid, &directive) < 0)
		{
			putErrmsg("Can't add plan for discovery.", neighborEid);
			return -1;
		}
	}

	if (addNdpNeighbor(neighborEid) < 0)
	{
		putErrmsg("Can't add discovered Neighbor.", neighborEid);
		return -1;
	}

	/*	Unblock outduct to this neighbor, enabling reforward.	*/

	if (bpUnblockOutduct(claProtocol->name, outductName) < 0)
	{
		putErrmsg("Can't unblock outduct.", outductName);
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
	char		*outductDaemon;
	char		*outductName;
	char		*destDuctName;
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
		outductDaemon = "tcpclo";
		outductName = socketSpec;
		destDuctName = NULL;
	}
	else if (strcmp(claProtocol, "udp") == 0)
	{
		portNumber = BpUdpDefaultPortNbr;
		inductDaemon = "udpcli";
		outductDaemon = "udpclo";
		outductName = "*";
		destDuctName = socketSpec;
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
		return addDtn2Neighbor(neighborEid, &protocol, outductName,
			destDuctName, outductDaemon, xmitRate, recvRate);
	}
	else if (strcmp(metaEid.schemeName, "ipn") == 0)
	{
		nodeNbr = metaEid.nodeNbr;
		restoreEidString(&metaEid);
		return addIpnNeighbor(nodeNbr, neighborEid, &protocol,
			outductName, destDuctName, outductDaemon, xmitRate,
			recvRate);
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
	uvast		ownNodeNbr = getOwnNodeNbr();
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
	if (strcmp(claProtocol, "udp") == 0)
	{
		return 0;	/*	Nothing to do.			*/
	}

	if (strcmp(claProtocol, "tcp") != 0)
	{
		writeMemoNote("[?] Neighbor loss discovery protocol name error",
				claProtocol);
		return -1;
	}

	if (bpBlockOutduct("tcp", socketSpec) < 0)
	{
		putErrmsg("Can't block TCP outduct.", socketSpec);
		return -1;
	}

	if (rfx_remove_contact(0, ownNodeNbr, neighborNodeNbr) < 0)
	{
		putErrmsg("Can't remove transmission contact.", socketSpec);
		return -1;
	}

	if (rfx_remove_contact(0, neighborNodeNbr, ownNodeNbr) < 0)
	{
		putErrmsg("Can't remove reception contact.", socketSpec);
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
