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

static void	shutDownClo()	/*	Commands CLO termination.	*/
{
	sm_SemEnd(udpcloSemaphore(NULL));
}

/*	*	*	Main thread functions	*	*	*	*/

#if defined (ION_LWT)
int	udpclo(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	char			*endpointSpec = (char *) a1;
	unsigned int		rtt = (a2 != 0 ? strtoul((char *) a2, NULL, 0)
		       				: 0);
#else
int	main(int argc, char *argv[])
{
	char			*endpointSpec = argc > 1 ? argv[1] : NULL;
	unsigned int		rtt = (argc > 2 ? strtoul(argv[2], NULL, 0)
						: 0);
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
	ClProtocol		protocol;
	Object			bundleZco;
	BpAncillaryData		ancillaryData;
	unsigned int		bundleLength;
	int			ductSocket = -1;
	int			bytesSent;

	if (endpointSpec == NULL)
	{
		PUTS("Usage: udpclo {<remote node's host name> | @}\
[:<its port number>] [<round-trip time in seconds>]");
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

	sdr = getIonsdr();
	CHKZERO(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &outduct, sdr_list_data(sdr, vduct->outductElt),
			sizeof(Outduct));
	sdr_read(sdr, (char *) &protocol, outduct.protocol, sizeof(ClProtocol));
	sdr_exit_xn(sdr);

	/*	Set up signal handling.  SIGTERM is shutdown signal.	*/

	oK(udpcloSemaphore(&(vduct->semaphore)));
	isignal(SIGTERM, shutDownClo);

	/*	Can now begin transmitting to remote duct.		*/

	writeMemo("[i] udpclo is running.");
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
