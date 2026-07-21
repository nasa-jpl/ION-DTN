/*
        udpclo.c:	BP UDP-based convergence-layer output
                        daemon.

        Author: Ted Piotrowski, APL
                Scott Burleigh, JPL

        Copyright (c) 2006, California Institute of Technology.
        ALL RIGHTS RESERVED.  U.S. Government Sponsorship
        acknowledged.

                                                                        */
#include "udpcla.h"

static sm_SemId udpcloSemaphore(sm_SemId *semid)
{
	static sm_SemId semaphore = -1;

	if (semid)
	{
		semaphore = *semid;
	}

	return semaphore;
}

static void shutDownClo(int signum)
{
	/* Tell the compiler that we are not using 'signum' */
	(void)signum;

	sm_SemEnd(udpcloSemaphore(NULL));
}

/*	*	*	Main thread functions	*	*	*	*/

static unsigned long getUsecTimestamp(void)
{
	struct timeval tv;

	getCurrentTime(&tv);
	return ((tv.tv_sec * 1000000) + tv.tv_usec);
}

/* Define the maximum number of failed lookups before stopping (default: 10000)
 */
#ifndef MAX_FAILED_LOOKUPS
#define MAX_FAILED_LOOKUPS 10000
#endif

#if defined(ION_LWT)
int udpclo(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5, saddr a6, saddr a7,
		saddr a8, saddr a9, saddr a10)
{
	char *rttString = (a1 != 0 ? (char *) a1 : NULL);
	char *endpointSpec = (char *) a2;
#else
int main(int argc, char *argv[])
{
	char *rttString = (argc > 1 ? argv[1] : NULL);
	char *endpointSpec = (argc > 2 ? argv[2] : NULL);
#endif
	IonNetworkAddress destAddr; /* Dual-stack destination */
	unsigned char    *buffer;
	VOutduct         *vduct;
	PsmAddress        vductElt;
	Sdr               sdr;
	Outduct           outduct;
	SdrObject	  planDuctList;
	SdrObject	  planObj = 0;
	BpPlan            plan;
	IonNeighbor      *neighbor = NULL;
	PsmAddress        nextElt;
	SdrObject	  bundleZco;
	BpAncillaryData   ancillaryData;
	unsigned int      bundleLength;
	int               ductSocket = -1;
	int               bytesSent;
	char destAddrStr[INET6_ADDR_WITH_PORT_STRLEN]; /* For logging */

	/*	Rate control calculation is based on treating elapsed
	 *	time as a currency.					*/

	float         timeCostPerByte; /*	In seconds.	*/
	unsigned long startTimestamp;  /*	Billing cycle.	*/
	unsigned int  totalPaid;       /*	Since last send.*/
	unsigned int  currentPaid;     /*	Sending seg.	*/
	float         totalCostSecs;   /*	For this seg.	*/
	unsigned int  totalCost;       /*	Microseconds.	*/
	unsigned int  balanceDue;      /*	Until next seg.	*/
	unsigned int  prevPaid = 0;    /*	Prior snooze.	*/

	/*	Note: for backward compatibility, we accept and ignore
	 *	a round-trip time value that precedes the endpointSpec.	*/

	if (endpointSpec == NULL)
	{
		if (rttString == NULL)
		{
			PUTS("Usage: udpclo {<remote node's host name> | @} [:<its port number>]");
			PUTS("  IPv4: udpclo 192.168.1.1:4556");
			PUTS("  IPv6: udpclo [2001:db8::1]:4556 or udpclo 2001:db8::1");
			PUTS("  Auto: udpclo node.example.com:4556");
			return 0;
		}
		else
		{
			endpointSpec = rttString;
		}
	}

	/* Attach to ION */
	if (bpAttach() < 0)
	{
		putErrmsg("udpclo can't attach to BP.", NULL);
		return -1;
	}

	/* Initial address resolution using dual-stack */
	int resolveResult = resolveNetworkAddressCached(endpointSpec, &destAddr);
	if (resolveResult == -2)
	{
		putErrmsg("udpclo: Maximum DNS failures reached", endpointSpec);
		return -1;
	}
	else if (resolveResult < 0)
	{
		writeMemoNote("[?] udpclo: Initial hostname resolution failed",
				endpointSpec);
		/* Continue - will retry during operation */
	}
	else
	{
		/* Log what we resolved to */
		formatNetworkAddress(&destAddr, destAddrStr, sizeof(destAddrStr));
		char memo[256];
		snprintf(memo, sizeof(memo), "udpclo resolved %s to %s (%s)",
				endpointSpec, destAddrStr,
				(destAddr.family == AF_INET6) ? "IPv6" : "IPv4");
		writeMemo(memo);
	}

	buffer = MTAKE(UDPCLA_BUFSZ);
	if (buffer == NULL)
	{
		putErrmsg("udpclo: No memory for UDP buffer in udpclo.", NULL);
		return -1;
	}

	findOutduct("udp", endpointSpec, &vduct, &vductElt);
	if (vductElt == 0)
	{
		putErrmsg("udpclo: No such udp duct.", endpointSpec);
		MRELEASE(buffer);
		return -1;
	}

	if (vduct->cloPid != ERROR && vduct->cloPid != sm_TaskIdSelf())
	{
		putErrmsg("udpclo: CLO task is already started for this duct.",
				itoa(vduct->cloPid));
		MRELEASE(buffer);
		return -1;
	}

	/*	All command-line arguments are now validated.		*/

	neighbor = NULL;
	sdr = getIonsdr();
	CHKZERO(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &outduct, sdr_list_data(sdr, vduct->outductElt),
			sizeof(Outduct));
	if (outduct.planDuctListElt)
	{
		planDuctList = sdr_list_list(sdr, outduct.planDuctListElt);
		planObj = sdr_list_user_data(sdr, planDuctList);
		if (planObj)
		{
			sdr_read(sdr, (char *) &plan, planObj, sizeof(BpPlan));
		}
	}

	sdr_exit_xn(sdr);

	/*	Set up signal handling.  SIGTERM is shutdown signal.	*/

	oK(udpcloSemaphore(&(vduct->semaphore)));
	isignal(SIGTERM, shutDownClo);

	/*	Can now begin transmitting to remote duct.		*/

	{
		char memoBuf[1024];

		if (resolveResult >= 0)
		{
			formatNetworkAddress(&destAddr, destAddrStr,
					sizeof(destAddrStr));
			isprintf(memoBuf, sizeof(memoBuf),
					"[i] udpclo is running, spec='%s' resolved to %s (%s)",
					endpointSpec, destAddrStr,
					(destAddr.family == AF_INET6) ? "IPv6" :
									"IPv4");
		}
		else
		{
			isprintf(memoBuf, sizeof(memoBuf),
					"[i] udpclo is running, spec='%s' (address resolution pending)",
					endpointSpec);
		}
		writeMemo(memoBuf);
	}

	startTimestamp = getUsecTimestamp();

	/* Main Loop */
	while (!(sm_SemEnded(vduct->semaphore)))
	{
		if (bpDequeue(vduct, &bundleZco, &ancillaryData, 0) < 0)
		{
			putErrmsg("udpclo: Can't dequeue bundle.", NULL);
			break;
		}

		if (bundleZco == 0)                       /* Outduct closed. */
		{
			writeMemo("[i] udpclo outduct closed.");
			sm_SemEnd(udpcloSemaphore(NULL)); /* Stop. */
			continue;
		}

		if (bundleZco == 1) /* Got a corrupt bundle. */
		{
			continue;   /* Get next bundle. */
		}

		/* Get bundle length */
		CHKZERO(sdr_begin_xn(sdr));
		bundleLength = zco_length(sdr, bundleZco);
		sdr_exit_xn(sdr); /* Short transaction, no nested calls expected */

		/* Periodic address re-resolution using dual-stack cache */
		int innerResolveResult = resolveNetworkAddressCached(endpointSpec,
				&destAddr);
		if (innerResolveResult == -2)
		{
			/* Fatal DNS failure - stop daemon */
			putErrmsg("udpclo: Maximum DNS failures reached",
					endpointSpec);
			if (sdr_begin_xn(sdr) >= 0)
			{
				zco_destroy(sdr, bundleZco);
				sdr_end_xn(sdr);
			}
			sm_SemEnd(udpcloSemaphore(NULL));
			break;
		}

		if (innerResolveResult < 0)
		{
			/* Address resolution failed - abandon this bundle */
			writeMemoNote("[?] udpclo: Address resolution failed, abandoning bundle",
					endpointSpec);
			if (sdr_begin_xn(sdr) >= 0)
			{
				zco_destroy(sdr, bundleZco);
				sdr_end_xn(sdr);
			}
			continue;
		}

		/* Send via Dual-stack */
		bytesSent = sendBundleByUDPDualStack(&destAddr, &ductSocket,
				bundleLength, bundleZco, buffer);
		if (bytesSent < 0 || (unsigned int)bytesSent < bundleLength)
		{
			sm_SemEnd(udpcloSemaphore(NULL)); /* Stop. */
			continue;
		}

		/* Rate control calculation */
		totalPaid = getUsecTimestamp() - startTimestamp;
		startTimestamp = getUsecTimestamp();

		if (totalPaid >= prevPaid)
		{
			currentPaid = totalPaid - prevPaid;
		}
		else
		{
			currentPaid = 0;
		}

		if (neighbor == NULL)
		{
			if (planObj && plan.neighborFqnn)
			{
				neighbor = findNeighbor(getIonVdb(),
					plan.neighborFqnn, &nextElt);
			}
		}

		if (neighbor && neighbor->xmitRate > 0)
		{
			timeCostPerByte = 1.0 / (neighbor->xmitRate);
		}
		else
		{
			timeCostPerByte = 0.0;
		}

		totalCostSecs = timeCostPerByte * computeECCC(bundleLength);
		totalCost = totalCostSecs * 1000000.0;

		if (totalCost > currentPaid)
		{
			balanceDue = totalCost - currentPaid;
		}
		else
		{
			balanceDue = 0;
		}

		if (balanceDue > 0)
		{
			microsnooze(balanceDue);
		}

		prevPaid = balanceDue;
		sm_TaskYield();
	}

	/* Clean up */
	if (ductSocket != -1)
	{
		closesocket(ductSocket);
	}

	writeErrmsgMemos();
	writeMemo("[i] udpclo duct has ended.");
	MRELEASE(buffer);
	ionDetach();
	return 0;
}
