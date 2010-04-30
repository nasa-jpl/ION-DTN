/*
	udplso.c:	LTP UDP-based link service output daemon.
			Dedicated to UDP datagram transmission to
			a single remote LTP engine.

	Author: Scott Burleigh, JPL

	Copyright (c) 2007, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include "udplsa.h"

static sm_SemId		udplsoSemaphore(sm_SemId *semid)
{
	static sm_SemId	semaphore = -1;
	
	if (semid)
	{
		semaphore = *semid;
	}

	return semaphore;
}

static void	shutDownLso()	/*	Commands LSO termination.	*/
{
	sm_SemEnd(udplsoSemaphore(NULL));
}

int	sendSegmentByUDP(int linkSocket, char *from, int length)
{
	int	bytesWritten;

	while (1)	/*	Continue until not interrupted.		*/
	{
		bytesWritten = send(linkSocket, from, length, 0);
		if (bytesWritten < 0)
		{
			if (errno == EINTR)	/*	Interrupted.	*/
			{
				continue;	/*	Retry.		*/
			}

			putSysErrmsg("LSO send() error on socket", NULL);
		}

		return bytesWritten;
	}
}

/*	*	*	Main thread functions	*	*	*	*/

#if defined (VXWORKS) || defined (RTEMS)
int	udplso(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	char			*endpointSpec = (char *) a1;
	unsigned long		remoteEngineId =
				a2 != 0 ? strtoul((char *) a2, NULL, 0) : 0;
#else
int	main(int argc, char *argv[])
{
	char			*endpointSpec = argc > 1 ? argv[1] : NULL;
	unsigned long		remoteEngineId =
				argc > 2 ? strtoul(argv[2], NULL, 0) : 0;
#endif
	Sdr			sdr;
	LtpVspan		*vspan;
	PsmAddress		vspanElt;
	unsigned short		portNbr = 0;
	unsigned int		ipAddress = 0;
	char			ownHostName[MAXHOSTNAMELEN];
	int			running = 1;
	int			segmentLength;
	char			*segment;
	struct sockaddr		socketName;
	struct sockaddr_in	*inetName;
	int			linkSocket;
	int			bytesSent;

	if (remoteEngineId == 0 || endpointSpec == NULL)
	{
		PUTS("Usage: udplso {<remote engine's host name> | @}[:\
<its port number>] <remote engine ID>");
		return 0;
	}

	/*	Note that ltpadmin must be run before the first
	 *	invocation of ltplso, to initialize the LTP database
	 *	(as necessary) and dynamic database.			*/

	if (ltpInit(0, 0) < 0)
	{
		putErrmsg("udplso can't initialize LTP.", NULL);
		return 1;
	}

	sdr = getIonsdr();
	sdr_begin_xn(sdr);	/*	Just to lock memory.		*/
	findSpan(remoteEngineId, &vspan, &vspanElt);
	if (vspanElt == 0)
	{
		sdr_exit_xn(sdr);
		putErrmsg("No such engine in database.", itoa(remoteEngineId));
		return 1;
	}

	if (vspan->lsoPid > 0 && vspan->lsoPid != sm_TaskIdSelf())
	{
		sdr_exit_xn(sdr);
		putErrmsg("LSO task is already started for this span.",
				itoa(vspan->lsoPid));
		return 1;
	}

	/*	All command-line arguments are now validated.		*/

	sdr_exit_xn(sdr);
	parseSocketSpec(endpointSpec, &portNbr, &ipAddress);
	if (portNbr == 0)
	{
		portNbr = LtpUdpDefaultPortNbr;
	}
	
	if (ipAddress == 0)		/*	Default to local host.	*/
	{
		getNameOfHost(ownHostName, sizeof ownHostName);
		ipAddress = getInternetAddress(ownHostName);
	}

	portNbr = htons(portNbr);
	ipAddress = htonl(ipAddress);
	memset((char *) &socketName, 0, sizeof socketName);
	inetName = (struct sockaddr_in *) &socketName;
	inetName->sin_family = AF_INET;
	inetName->sin_port = portNbr;
	memcpy((char *) &(inetName->sin_addr.s_addr), (char *) &ipAddress, 4);
	linkSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (linkSocket < 0)
	{
		putSysErrmsg("LSO can't open UDP socket", NULL);
		return 1;
	}

	if (connect(linkSocket, &socketName, sizeof(struct sockaddr_in)) < 0)
	{
		putSysErrmsg("LSO can't connect UDP socket", NULL);
		return 1;
	}

	/*	Set up signal handling.  SIGTERM is shutdown signal.	*/

	oK(udplsoSemaphore(&(vspan->segSemaphore)));
	signal(SIGTERM, shutDownLso);

	/*	Can now begin transmitting to remote engine.		*/

	writeMemo("[i] udplso is running.");
	while (running && !(sm_SemEnded(vspan->segSemaphore)))
	{
		segmentLength = ltpDequeueOutboundSegment(vspan, &segment);
		if (segmentLength < 0)
		{
			running = 0;	/*	Terminate LSO.		*/
			continue;
		}

		if (segmentLength == 0)	/*	Interrupted.		*/
		{
			continue;
		}

		if (segmentLength > UDPLSA_BUFSZ)
		{
			putErrmsg("Segment is too big for UDP LSO.",
					itoa(segmentLength));
			running = 0;	/*	Terminate LSO.		*/
		}
		else
		{
			bytesSent = sendSegmentByUDP(linkSocket, segment,
					segmentLength);
			if (bytesSent < segmentLength)
			{
				running = 0;	/*	Terminate LSO.	*/
			}
		}

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();
	}

	close(linkSocket);
	writeErrmsgMemos();
	writeMemo("[i] udplso duct has ended.");
	return 0;
}
