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

static sm_SemId		udpcloSemaphore(sm_SemId *semid)
{
	static sm_SemId	semaphore = -1;
	
	if (semid)
	{
		semaphore = *semid;
	}

	return semaphore;
}

static void	shutDownClo(int signum)
{
	sm_SemEnd(udpcloSemaphore(NULL));
}

/*	*	*	Main thread functions	*	*	*	*/

static unsigned long	getUsecTimestamp()
{
	struct timeval	tv;

	getCurrentTime(&tv);
	return ((tv.tv_sec * 1000000) + tv.tv_usec);
}

#if defined (ION_LWT)
int	udpclo(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	unsigned int		rtt = (a1 != 0 ? strtoul((char *) a1, NULL, 0)
		       				: 0);
	char			*endpointSpec = (char *) a2;
#else
int	main(int argc, char *argv[])
{
	unsigned int		rtt = (argc > 1 ? strtoul(argv[1], NULL, 0)
						: 0);
	char			*endpointSpec = argc > 2 ? argv[2] : NULL;
#endif
	unsigned short		portNbr;
	unsigned int		hostNbr;
	char			ownHostName[MAXHOSTNAMELEN];
	struct sockaddr		socketName;
	struct sockaddr_in	*inetName;
	unsigned char		*buffer;
	VOutduct		*vduct;
	PsmAddress		vductElt;
	Sdr			sdr;
	Outduct			outduct;
	Object			planDuctList;
	Object			planObj = 0;
	BpPlan			plan;
	IonNeighbor		*neighbor = NULL;
	PsmAddress		nextElt;
	Object			bundleZco;
	BpAncillaryData		ancillaryData;
	unsigned int		bundleLength;
	int			ductSocket = -1;
	int			bytesSent;

	/*	Rate control calculation is based on treating elapsed
	 *	time as a currency.					*/

	float			timeCostPerByte;/*	In seconds.	*/
	unsigned long		startTimestamp;	/*	Billing cycle.	*/
	unsigned int		totalPaid;	/*	Since last send.*/
	unsigned int		currentPaid;	/*	Sending seg.	*/
	float			totalCostSecs;	/*	For this seg.	*/
	unsigned int		totalCost;	/*	Microseconds.	*/
	unsigned int		balanceDue;	/*	Until next seg.	*/
	unsigned int		prevPaid = 0;	/*	Prior snooze.	*/

	if (endpointSpec == NULL)
	{
		PUTS("Usage: udpclo <round-trip time in seconds> {<remote \
node's host name> | @} [:<its port number>]");
		return 0;
	}

	parseSocketSpec(endpointSpec, &portNbr, &hostNbr);
	if (portNbr == 0)
	{
		portNbr = BpUdpDefaultPortNbr;
	}

	portNbr = htons(portNbr);
	if (hostNbr == 0)		/*	Default to local host.	*/
	{
		getNameOfHost(ownHostName, sizeof ownHostName);
		hostNbr = getInternetAddress(ownHostName);
	}

	hostNbr = htonl(hostNbr);
	memset((char *) &socketName, 0, sizeof socketName);
	inetName = (struct sockaddr_in *) &socketName;
	inetName->sin_family = AF_INET;
	inetName->sin_port = portNbr;
	memcpy((char *) &(inetName->sin_addr.s_addr), (char *) &hostNbr, 4);
	if (bpAttach() < 0)
	{
		putErrmsg("udpclo can't attach to BP.", NULL);
		return -1;
	}

	buffer = MTAKE(UDPCLA_BUFSZ);
	if (buffer == NULL)
	{
		putErrmsg("No memory for UDP buffer in udpclo.", NULL);
		return -1;
	}

	findOutduct("udp", endpointSpec, &vduct, &vductElt);
	if (vductElt == 0)
	{
		putErrmsg("No such udp duct.", endpointSpec);
		MRELEASE(buffer);
		return -1;
	}

	if (vduct->cloPid != ERROR && vduct->cloPid != sm_TaskIdSelf())
	{
		putErrmsg("CLO task is already started for this duct.",
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
		char	memoBuf[1024];

		isprintf(memoBuf, sizeof(memoBuf),
				"[i] udpclo is running, spec = '%s', rtt = %d",
				endpointSpec, rtt);
		writeMemo(memoBuf);
	}

	startTimestamp = getUsecTimestamp();
	while (!(sm_SemEnded(vduct->semaphore)))
	{
		if (bpDequeue(vduct, &bundleZco, &ancillaryData, rtt) < 0)
		{
			putErrmsg("Can't dequeue bundle.", NULL);
			break;
		}

		if (bundleZco == 0)	/*	Outduct closed.		*/
		{
			writeMemo("[i] udpclo outduct closed.");
			sm_SemEnd(udpcloSemaphore(NULL));/*	Stop.	*/
			continue;
		}

		if (bundleZco == 1)	/*	Got a corrupt bundle.	*/
		{
			continue;	/*	Get next bundle.	*/
		}

		CHKZERO(sdr_begin_xn(sdr));
		bundleLength = zco_length(sdr, bundleZco);
		sdr_exit_xn(sdr);
		bytesSent = sendBundleByUDP(&socketName, &ductSocket,
				bundleLength, bundleZco, buffer);
		if (bytesSent < bundleLength)
		{
			sm_SemEnd(udpcloSemaphore(NULL));/*	Stop.	*/
			continue;
		}

		/*	Rate control calculation is based on treating
		 *	elapsed time as a currency, the price you
		 *	pay (by microsnooze) for sending a segment
		 *	of a given size.  All cost figures are
		 *	expressed in microseconds except the computed
		 *	totalCostSecs of the segment.			*/

		totalPaid = getUsecTimestamp() - startTimestamp;

		/*	Start clock for next bill.			*/

		startTimestamp = getUsecTimestamp();

		/*	Compute time balance due.			*/

		if (totalPaid >= prevPaid)
		{
		/*	This should always be true provided that
		 *	clock_gettime() is supported by the O/S.	*/

			currentPaid = totalPaid - prevPaid;
		}
		else
		{
			currentPaid = 0;
		}

		/*	Get current time cost, in seconds, per byte.	*/

		if (neighbor == NULL)
		{
			if (planObj && plan.neighborNodeNbr)
			{
				neighbor = findNeighbor(getIonVdb(),
						plan.neighborNodeNbr, &nextElt);
			}
		}

		if (neighbor && neighbor->xmitRate > 0)
		{
			timeCostPerByte = 1.0 / (neighbor->xmitRate);
		}
		else	/*	No link service rate control.		*/ 
		{
			timeCostPerByte = 0.0;
		}

		totalCostSecs = timeCostPerByte * computeECCC(bundleLength);
		totalCost = totalCostSecs * 1000000.0;	/*	usec.	*/
		if (totalCost > currentPaid)
		{
			balanceDue = totalCost - currentPaid;
		}
		else
		{
			balanceDue = 1;
		}

		microsnooze(balanceDue);
		prevPaid = balanceDue;

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();
	}

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
