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
#include "ipnfw.h"

int	bp_discover_contact_acquired(char *socketSpec, uvast neighborNodeNbr,
		unsigned int xmitRate, unsigned int recvRate)
{
	Sdr		sdr = getIonsdr();
	uvast		ownNodeNbr = getOwnNodeNbr();
	ClProtocol	protocol;
	Object		elt;
	unsigned int	ipAddress;
	char		ipAddressString[16];
	char		inductName[32];
	Object		planObj;
	IpnPlan		plan;
	Outduct		duct;
	VOutduct	*vduct;
	PsmAddress	vductElt;
	DuctExpression	ductExpression;
	time_t		currentTime;

	CHKERR(socketSpec);
	CHKERR(*socketSpec);
	CHKERR(neighborNodeNbr);
	CHKERR(xmitRate);
	CHKERR(recvRate);
	fetchProtocol("tcp", &protocol, &elt);
	if (elt == 0)
	{
		/*	All discovered contacts are for TCP/IP.  If
		 *	no tcp convergence-layer adapter has been
		 *	defined yet, add it now.			*/

		if (addProtocol("tcp", 1400, 100, -1, 0) < 0)
		{
			putErrmsg("Can't add protocol for discovered contacts.",
					NULL);
			return -1;
		}

		ipAddress = getAddressOfHost();
		if (ipAddress == 0)
		{
			putErrmsg("Can't get address of local host.", NULL);
			return -1;
		}

		/*	Add induct for TCP CLA, and start it.		*/

		printDottedString(ipAddress, ipAddressString);
		isprintf(inductName, sizeof inductName, "%s:%d",
				ipAddressString, BpTcpDefaultPortNbr);
		if (addInduct("tcp", inductName, "tcpcli") < 0)
		{
			putErrmsg("Can't add TCP induct.", inductName);
			return -1;
		}

		if (bpStartInduct("tcp", inductName) < 0)
		{
			putErrmsg("Can't start TCP induct.", inductName);
			return -1;
		}
	}

	ipn_findPlan(neighborNodeNbr, &planObj, &elt);
	if (elt)
	{
		/*	Make sure this new contact matches the
		 *	existing egress plan for transmission to
		 *	this neighboring node.				*/

		sdr_read(sdr, (char *) &plan, planObj, sizeof(IpnPlan));
		sdr_read(sdr, (char *) &duct, sdr_list_data(sdr,
				plan.defaultDirective.outductElt),
				sizeof(Outduct));
		sdr_read(sdr, (char *) &protocol, duct.protocol,
				sizeof(ClProtocol));
		if (strcmp(protocol.name, "tcp") != 0)
		{
			writeMemoNote("[?] Contact rejected; not a TCP node",
					socketSpec);
			return 0;
		}

		if (strcmp(duct.name, socketSpec) != 0)
		{
			writeMemoNote("[?] Contact rejected; wrong host/port",
					socketSpec);
			return 0;
		}
	}
	else	/*	This is a brand-new neighbor.			*/
	{
		/*	Add new TCP outduct to this neighbor.		*/

		if (addOutduct("tcp", socketSpec, "tcpclo", 0) < 0)
		{
			putErrmsg("Can't add TCP outduct.", socketSpec);
			return -1;
		}

		if (bpStartOutduct("tcp", socketSpec) < 0)
		{
			putErrmsg("Can't start TCP outduct.", socketSpec);
			return -1;
		}

		if (bpBlockOutduct("tcp", socketSpec) < 0)
		{
			putErrmsg("Can't block TCP outduct.", socketSpec);
			return -1;
		}

		/*	Add egress plan that cites this new outduct.	*/

		findOutduct("tcp", socketSpec, &vduct, &vductElt);
		ductExpression.outductElt = vduct->outductElt;
		ductExpression.destDuctName = socketSpec;
		if (ipn_addPlan(neighborNodeNbr, &ductExpression) < 0)
		{
			putErrmsg("Can't add plan for discovery.", socketSpec);
			return -1;
		}
	}

	/*	Insert contact into contact plan.			*/

	currentTime = getUTCTime();
	if (rfx_insert_contact(currentTime, MAX_POSIX_TIME, ownNodeNbr,
			neighborNodeNbr, xmitRate, 1.0) == 0)
	{
		putErrmsg("Can't add transmission contact.", socketSpec);
		return -1;
	}

	if (rfx_insert_contact(currentTime, MAX_POSIX_TIME, neighborNodeNbr,
			ownNodeNbr, recvRate, 1.0) == 0)
	{
		putErrmsg("Can't add reception contact.", socketSpec);
		return -1;
	}

	/*	Unblock outduct to this neighbor, enabling reforward.	*/

	if (bpUnblockOutduct("tcp", socketSpec) < 0)
	{
		putErrmsg("Can't unblock TCP outduct.", socketSpec);
		return -1;
	}

	return 0;
}

int	bp_discover_contact_lost(char *socketSpec, uvast neighborNodeNbr)
{
	uvast	ownNodeNbr = getOwnNodeNbr();

	CHKERR(socketSpec);
	CHKERR(*socketSpec);
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

