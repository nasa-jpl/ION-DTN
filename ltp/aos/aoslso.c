/*
  aoslso.c - LTP over AOS Link Service Adapter, output
*/

/* 7/6/2010, copied from udplso, as per issue 101-LTP-over-AOS-via-UDP
   Greg Menke, Raytheon, under contract METS-MR-679-0909 with NASA GSFC */



#include "aoslsa.h"


static sm_SemId		aoslsoSemaphore(sm_SemId *semid)
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
	sm_SemEnd(aoslsoSemaphore(NULL));
}

int	sendSegmentByAOS(int linkSocket, char *from, int length)
{
	int	bytesWritten;

	while (1)	/*	Continue until not interrupted.		*/
	{
		bytesWritten = isend(linkSocket, from, length, 0);
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

#if defined (ION_LWT)
int	aoslso(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
	       saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char		*endpointSpec = (char *) a1;
	unsigned int	txbps = (a2 != 0 ? strtoul((char *) a2, NULL, 0) : 0);
	uvast		remoteEngineId = a3 != 0 ?  strtouvast((char *) a3) : 0;
#else
int	main(int argc, char *argv[])
{
	char		*endpointSpec = argc > 1 ? argv[1] : NULL;
	unsigned int	txbps = (argc > 2 ? strtoul(argv[2], NULL, 0) : 0);
	uvast		remoteEngineId = argc > 3 ?  strtouvast(argv[3]) : 0;
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
	int			bytesSent = 0;

	if( txbps != 0 && remoteEngineId == 0 )
	{
		remoteEngineId = txbps;
		txbps = 0;
	}

	if (remoteEngineId == 0 || endpointSpec == NULL)
	{
		PUTS("Usage: aoslso {<remote engine's host name> | @}[:\
		<its port number>] <txbps (0=unlimited)> <remote engine ID>");
		return 0;
	}

	/*	Note that ltpadmin must be run before the first
	 *	invocation of ltplso, to initialize the LTP database
	 *	(as necessary) and dynamic database.			*/

	if (ltpInit(0) < 0)
	{
		putErrmsg("aoslso can't initialize LTP.", NULL);
		return 1;
	}

	sdr = getIonsdr();
	CHKZERO(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	findSpan(remoteEngineId, &vspan, &vspanElt);
	if (vspanElt == 0)
	{
		sdr_exit_xn(sdr);
		putErrmsg("No such engine in database.", itoa(remoteEngineId));
		return 1;
	}

	if (vspan->lsoPid != ERROR && vspan->lsoPid != sm_TaskIdSelf())
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
		portNbr = LtpAosDefaultPortNbr;
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
		putSysErrmsg("LSO can't open AOS socket", NULL);
		return 1;
	}

	if (connect(linkSocket, &socketName, sizeof(struct sockaddr_in)) < 0)
	{
		closesocket( linkSocket );
		putSysErrmsg("LSO can't connect AOS socket", NULL);
		return 1;
	}

	/*	Set up signal handling.  SIGTERM is shutdown signal.	*/

	oK(aoslsoSemaphore(&(vspan->segSemaphore)));
	signal(SIGTERM, shutDownLso);

	/*	Can now begin transmitting to remote engine.		*/
	{
		char	txt[500];

		isprintf(txt, sizeof(txt),
			"[i] aolslso is running, spec=[%s:%d], txbps=%d \
(0=unlimited), rengine=" UVAST_FIELDSPEC ".",
			(char *) inet_ntoa(inetName->sin_addr), 
			ntohs(portNbr), txbps, remoteEngineId);
		writeMemo(txt);
	}

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

		if (segmentLength > AOSLSA_BUFSZ)
		{
			putErrmsg("Segment is too big for AOS LSO.",
				itoa(segmentLength));
			running = 0;	/*	Terminate LSO.		*/
		}
		else
		{
			bytesSent = sendSegmentByAOS(linkSocket, segment,
				segmentLength);
			if (bytesSent < segmentLength)
			{
				running = 0;	/*	Terminate LSO.	*/
			}
		}

		if( txbps != 0 )
		{
			unsigned int usecs;
			float sleep_secs = (1.0 / ((float)txbps)) *
				((float)(bytesSent*8));

			if( sleep_secs < 0.010 )
			{
				usecs = 10000;
			}
			else
			{
				usecs = (unsigned int)( sleep_secs * 1000000 );
			}

			microsnooze( usecs );
		}

	/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();
	}

	closesocket(linkSocket);
	writeErrmsgMemos();
	writeMemo("[i] aoslso duct has ended.");
	return 0;
}

