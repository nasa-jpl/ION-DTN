/*
	udplso.c:	LTP UDP-based link service output daemon.
			Dedicated to UDP datagram transmission to
			a single remote LTP engine.

	Author: Scott Burleigh, JPL

	Copyright (c) 2007, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
	7/6/2010, modified as per issue 132-udplso-tx-rate-limit
	Greg Menke, Raytheon, under contract METS-MR-679-0909
	with NASA GSFC.
	
	5/31/2016, modified as per bug 0069-Improve rate control
	algorithms. Jeremy Pierce-Mayer, DLR/INSYEN.
*/

#include "udplsa.h"

#if defined(linux)

#define IPHDR_SIZE	(sizeof(struct iphdr) + sizeof(struct udphdr))

#elif defined(mingw)

#define IPHDR_SIZE	(20 + 8)

#else

#include "netinet/ip_var.h"
#include "netinet/udp_var.h"

#define IPHDR_SIZE	(sizeof(struct udpiphdr))

#endif

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

/*	*	*	Receiver thread functions	*	*	*/

typedef struct
{
	int		linkSocket;
	int		running;
} ReceiverThreadParms;
	
static void	*handleDatagrams(void *parm)
{
	/*	Main loop for UDP datagram reception and handling.	*/

	ReceiverThreadParms	*rtp = (ReceiverThreadParms *) parm;
	char			*buffer;
	int			segmentLength;
	struct sockaddr_in	fromAddr;
	socklen_t		fromSize;

	buffer = MTAKE(UDPLSA_BUFSZ);
	if (buffer == NULL)
	{
		putErrmsg("udplsi can't get UDP buffer.", NULL);
		shutDownLso();
		return NULL;
	}

	/*	Can now start receiving bundles.  On failure, take
	 *	down the LSO.						*/

	iblock(SIGTERM);
	while (rtp->running)
	{	
		fromSize = sizeof fromAddr;
		segmentLength = irecvfrom(rtp->linkSocket, buffer, UDPLSA_BUFSZ,
				0, (struct sockaddr *) &fromAddr, &fromSize);
		switch (segmentLength)
		{
		case -1:
			putSysErrmsg("Can't acquire segment", NULL);
			shutDownLso();

			/*	Intentional fall-through to next case.	*/

		case 1:				/*	Normal stop.	*/
			rtp->running = 0;
			continue;
		}

		if (ltpHandleInboundSegment(buffer, segmentLength) < 0)
		{
			putErrmsg("Can't handle inbound segment.", NULL);
			shutDownLso();
			rtp->running = 0;
			continue;
		}

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();
	}

	writeErrmsgMemos();
	writeMemo("[i] udplso receiver thread has ended.");

	/*	Free resources.						*/

	MRELEASE(buffer);
	return NULL;
}

/*	*	*	Main thread functions	*	*	*	*/

int	sendSegmentByUDP(int linkSocket, char *from, int length,
		struct sockaddr_in *destAddr )
{
	int	bytesWritten;

	while (1)	/*	Continue until not interrupted.		*/
	{
		bytesWritten = isendto(linkSocket, from, length, 0,
				(struct sockaddr *) destAddr,
				sizeof(struct sockaddr));
		if (bytesWritten < 0)
		{
			if (errno == EINTR)	/*	Interrupted.	*/
			{
				continue;	/*	Retry.		*/
			}

			{
				char			memoBuf[1000];
				struct sockaddr_in	*saddr = destAddr;

				isprintf(memoBuf, sizeof(memoBuf),
					"udplso sendto() error, dest=[%s:%d], \
nbytes=%d, rv=%d, errno=%d", (char *) inet_ntoa(saddr->sin_addr), 
					ntohs(saddr->sin_port), 
					length, bytesWritten, errno);
				writeMemo(memoBuf);
			}
		}

		return bytesWritten;
	}
}

#if defined (ION_LWT)
int	udplso(int a1, int a2, int a3, int a4, int a5,
	       int a6, int a7, int a8, int a9, int a10)
{
	char		*endpointSpec = (char *) a1;
	unsigned int	txbps = (a2 != 0 ?  strtoul((char *) a2, NULL, 0) : 0);
	unsigned int 	tokenWindowTime = (a3 != 0 ?  strtoul((char *) a3, NULL, 0) : 0);
	uvast		remoteEngineId = a4 != 0 ?  strtouvast((char *) a4) : 0;

#else
int	main(int argc, char *argv[])
{
	char		*endpointSpec = argc > 1 ? argv[1] : NULL;
	unsigned int	txbps = (argc > 2 ?  strtoul(argv[2], NULL, 0) : 0);
	unsigned int 	tokenWindowTime = (argc > 3 ? strtoul(argv[3], NULL, 0) : 0);
	uvast		remoteEngineId = argc > 4 ? strtouvast(argv[4]) : 0;

#endif
	Sdr			sdr;
	LtpVspan		*vspan;
	PsmAddress		vspanElt;
	unsigned short		portNbr = 0;
	unsigned int		ipAddress = 0;
	char			ownHostName[MAXHOSTNAMELEN];
	struct sockaddr		ownSockName;
	struct sockaddr_in	*ownInetName;
	struct sockaddr		bindSockName;
	struct sockaddr_in	*bindInetName;
	struct sockaddr		peerSockName;
	struct sockaddr_in	*peerInetName;
	socklen_t		nameLength;
	ReceiverThreadParms	rtp;
	pthread_t		receiverThread;
	int			segmentLength;
	char			*segment;
	int			bytesSent;
	int			fd;
	char			quit = '\0';
	/* For token bucket implementation */
	unsigned int remainingTokens = 0;
	unsigned int lastTokenAdditionTime = 0;
	unsigned int tokensToAdd = 0;
	unsigned int tokenMaxInBucket = 0;
	unsigned int tokenCurTime = 0;
	struct timeval tokenTimeVal;
	
	if (tokenWindowTime != 0 && remoteEngineId == 0)
	{
		remoteEngineId = tokenWindowTime;
		tokenWindowTime = 0;
	}
	else if( txbps != 0 && remoteEngineId == 0)
	{
		remoteEngineId = txbps;
		txbps = 0;
	}
	
	if (remoteEngineId == 0 || endpointSpec == NULL)
	{
		PUTS("Usage: udplso {<remote engine's host name> | @}[:\
		<its port number>] <txbps (0=unlimited)> <token bucket sliding window (in ms)> <remote engine ID> ");
		return 0;
	}

	/*	Note that ltpadmin must be run before the first
	 *	invocation of ltplso, to initialize the LTP database
	 *	(as necessary) and dynamic database.			*/

	if (ltpInit(0) < 0)
	{
		putErrmsg("udplso can't initialize LTP.", NULL);
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

	sdr_exit_xn(sdr);
	
	/* Perform the initial setup for the token bucket */
	if(txbps)
	{
		/* Internally, our math is all based upon bytes/sec, whereas this parameter is in bits/sec, so we change that */
		txbps = txbps/8;
		tokenMaxInBucket = txbps;
		/* Check to see if the txbps is less than a single segment. */
		if(txbps < vspan->maxXmitSegSize)
		{
			putErrmsg("txbps shouldn't ever be less than the Max Xmit Segment Size, attempting to compensate", NULL);
			tokenMaxInBucket = vspan->maxXmitSegSize;
		}
		if((tokenWindowTime == 0) | (tokenWindowTime * txbps < 1000000)) /*We need to add some sane defaults */
		{
			tokenWindowTime = 1000000/txbps; /*microseconds*/
			tokensToAdd = 1;
		}
		else
			tokensToAdd = txbps/(1000000/tokenWindowTime);
		
		/* Seed the bucket */
		remainingTokens = tokensToAdd;
	}	

	/*	All command-line arguments are now validated.  First
	 *	get peer's socket address.				*/

	parseSocketSpec(endpointSpec, &portNbr, &ipAddress);
	if (portNbr == 0)
	{
		portNbr = LtpUdpDefaultPortNbr;
	}

	getNameOfHost(ownHostName, sizeof ownHostName);
	if (ipAddress == 0)		/*	Default to local host.	*/
	{
		ipAddress = getInternetAddress(ownHostName);
	}

	portNbr = htons(portNbr);
	ipAddress = htonl(ipAddress);
	memset((char *) &peerSockName, 0, sizeof peerSockName);
	peerInetName = (struct sockaddr_in *) &peerSockName;
	peerInetName->sin_family = AF_INET;
	peerInetName->sin_port = portNbr;
	memcpy((char *) &(peerInetName->sin_addr.s_addr),
			(char *) &ipAddress, 4);

	/*	Now compute own socket address, used when the peer
	 *	responds to the link service output socket rather
	 *	than to the advertised link service input socket.	*/

	ipAddress = htonl(INADDR_ANY);
	memset((char *) &bindSockName, 0, sizeof bindSockName);
	bindInetName = (struct sockaddr_in *) &bindSockName;
	bindInetName->sin_family = AF_INET;
	bindInetName->sin_port = 0;	/*	Let O/S select it.	*/
	memcpy((char *) &(bindInetName->sin_addr.s_addr),
			(char *) &ipAddress, 4);

	/*	Now create the socket that will be used for sending
	 *	datagrams to the peer LTP engine and receiving
	 *	datagrams from the peer LTP engine.			*/

	rtp.linkSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (rtp.linkSocket < 0)
	{
		putSysErrmsg("LSO can't open UDP socket", NULL);
		return 1;
	}

	/*	Bind the socket to own socket address so that we can
	 *	send a 1-byte datagram to that address to shut down
	 *	the datagram handling thread.				*/

	nameLength = sizeof(struct sockaddr);
	if (bind(rtp.linkSocket, &bindSockName, nameLength) < 0
	|| getsockname(rtp.linkSocket, &bindSockName, &nameLength) < 0)
	{
		closesocket(rtp.linkSocket);
		putSysErrmsg("LSO can't bind UDP socket", NULL);
		return 1;
	}

	/*	Set up signal handling.  SIGTERM is shutdown signal.	*/


	oK(udplsoSemaphore(&(vspan->segSemaphore)));
	signal(SIGTERM, shutDownLso);

	/*	Start the echo handler thread.				*/

	rtp.running = 1;
	if (pthread_begin(&receiverThread, NULL, handleDatagrams, &rtp))
	{
		closesocket(rtp.linkSocket);
		putSysErrmsg("udplsi can't create receiver thread", NULL);
		return 1;
	}

	/*	Can now begin transmitting to remote engine.		*/
	{
		char	memoBuf[1024];

		isprintf(memoBuf, sizeof(memoBuf),
			"[i] udplso is running, spec=[%s:%d], txbps=%d \
(0=unlimited), tokenWindowTime=%d, rengine=%d.", (char *) inet_ntoa(peerInetName->sin_addr),
			ntohs(portNbr), txbps, tokenWindowTime, (int) remoteEngineId);
		writeMemo(memoBuf);
	}
	
	while (rtp.running && !(sm_SemEnded(vspan->segSemaphore)))
	{
		segmentLength = ltpDequeueOutboundSegment(vspan, &segment);
		if (segmentLength < 0)
		{
			rtp.running = 0;	/*	Terminate LSO.	*/
			continue;
		}

		if (segmentLength == 0)		/*	Interrupted.	*/
		{
			continue;
		}
		
		if (segmentLength > UDPLSA_BUFSZ)
		{
			putErrmsg("Segment is too big for UDP LSO.",
					itoa(segmentLength));
			rtp.running = 0;	/*	Terminate LSO.	*/
		}
		
		if(tokenWindowTime) /* A quick-and-dirty way to test if we're using token bucket */
		{
	
			/* First, add tokens if required */
			getCurrentTime(&tokenTimeVal);
			tokenCurTime = tokenTimeVal.tv_sec*1000000 + tokenTimeVal.tv_usec;
			
			unsigned int tokenTimeElapsed = tokenCurTime - lastTokenAdditionTime;
			
			if((tokenTimeElapsed > tokenWindowTime) && (remainingTokens < tokenMaxInBucket))
			{
				remainingTokens += tokensToAdd * (tokenTimeElapsed/tokenWindowTime);
				lastTokenAdditionTime = tokenCurTime;
				
				if(remainingTokens > tokenMaxInBucket)
					remainingTokens = tokenMaxInBucket;
			}
			
			if(segmentLength + IPHDR_SIZE > remainingTokens) /* We don't have enough tokens to transmit the entire segment */
			{
				/* Predict when we will, and sleep until then */
				if(tokensToAdd != 1)
				{
					microsnooze(tokenWindowTime);
				}
				else
					sm_TaskYield();
					
				continue;
			}
		}
		writeMemo("Sending");
		bytesSent = sendSegmentByUDP(rtp.linkSocket, segment,
					segmentLength, peerInetName);	
		
		if (bytesSent < segmentLength)
		{
			printf("Transmit error %d\n",bytesSent);
			rtp.running = 0;/*	Terminate LSO.	*/
		}
		else if (tokenWindowTime)
		{
			remainingTokens -= bytesSent + IPHDR_SIZE;
		}
		
		/*	Make sure other tasks have a chance to run.	*/
		sm_TaskYield();
	}

	/*	Create one-use socket for the closing quit byte.	*/

	portNbr = bindInetName->sin_port;	/*	From binding.	*/
	ipAddress = getInternetAddress(ownHostName);
	ipAddress = htonl(ipAddress);
	memset((char *) &ownSockName, 0, sizeof ownSockName);
	ownInetName = (struct sockaddr_in *) &ownSockName;
	ownInetName->sin_family = AF_INET;
	ownInetName->sin_port = portNbr;
	memcpy((char *) &(ownInetName->sin_addr.s_addr),
			(char *) &ipAddress, 4);

	/*	Wake up the receiver thread by sending it a 1-byte
	 *	datagram.						*/

	fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (fd >= 0)
	{
		isendto(fd, &quit, 1, 0, &ownSockName, sizeof(struct sockaddr));
		closesocket(fd);
	}

	pthread_join(receiverThread, NULL);
	closesocket(rtp.linkSocket);
	writeErrmsgMemos();
	writeMemo("[i] udplso has ended.");
	ionDetach();
	return 0;
}
