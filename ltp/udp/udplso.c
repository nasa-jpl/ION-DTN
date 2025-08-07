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

#endif 		/* end of if defined(linux) */


/* Macro definitions for DNS retry and re-resolution */
#ifndef UDPLSO_DNS_RETRY_COUNT
#define UDPLSO_DNS_RETRY_COUNT 1000
#endif

#ifndef UDPLSO_DNS_RETRY_DELAY
#define UDPLSO_DNS_RETRY_DELAY 10
#endif

#ifndef UDPLSO_DNS_RECHECK_INTERVAL
#define UDPLSO_DNS_RECHECK_INTERVAL 60
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

/*	*	*	Main thread functions	*	*	*	*/

#ifdef UDP_MULTISEND
static int	sendBatch(int linkSocket, struct mmsghdr *msgs,
			unsigned int batchLength)
{
	int	totalBytesSent = 0;
	int	bytesSent;
	int	i;

	if (sendmmsg(linkSocket, msgs, batchLength, 0) < 0)
	{
		putSysErrmsg("Failed in sendmmsg", itoa(batchLength));
		return -1;
	}

	for (i = 0; i < batchLength; i++)
	{
		bytesSent = msgs[i].msg_len;
		if (bytesSent > 0)
		{
			totalBytesSent += (IPHDR_SIZE + bytesSent);
		}
	}

	return totalBytesSent;
}
#else
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

			if (errno == ENETUNREACH)
			{
				return length;	/*	Just data loss.	*/
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

static unsigned long	getUsecTimestamp()
{
	struct timeval	tv;

	getCurrentTime(&tv);
	return ((tv.tv_sec * 1000000) + tv.tv_usec);
}

typedef struct
{
	unsigned long		startTimestamp;	/*	Billing cycle.	*/
	uvast			remoteEngineId;
	IonNeighbor		*neighbor;
	unsigned int		prevPaid;
} RateControlState;

static void	applyRateControl(RateControlState *rc, int bytesSent)
{
	/*	Rate control calculation is based on treating elapsed
	 *	time as a currency, the price you pay (by microsnooze)
	 *	for sending a given number of bytes.  All cost figures
	 *	are expressed in microseconds except the computed
	 *	totalCostSecs of the transmission.			*/

	unsigned int		totalPaid;	/*	Since last send.*/
	float			timeCostPerByte;/*	In seconds.	*/
	unsigned int		currentPaid;	/*	Sending seg.	*/
	PsmAddress		nextElt;
	float			totalCostSecs;	/*	For this seg.	*/
	unsigned int		totalCost;	/*	Microseconds.	*/
	unsigned int		balanceDue;	/*	Until next seg.	*/

	totalPaid = getUsecTimestamp() - rc->startTimestamp;

	/*	Start clock for next bill.				*/

	rc->startTimestamp = getUsecTimestamp();

	/*	Compute time balance due.				*/

	if (totalPaid >= rc->prevPaid)
	{
	/*	This should always be true provided that
	 *	clock_gettime() is supported by the O/S.		*/

		currentPaid = totalPaid - rc->prevPaid;
	}
	else
	{
		currentPaid = 0;
	}

	/*	Get current time cost, in seconds, per byte.		*/

	if (rc->neighbor == NULL)
	{
		rc->neighbor = findNeighbor(getIonVdb(), rc->remoteEngineId,
				&nextElt);
	}

	if (rc->neighbor && rc->neighbor->xmitRate > 0)
	{
		timeCostPerByte = 1.0 / (rc->neighbor->xmitRate);
	}
	else	/*	No link service rate control.			*/ 
	{
		timeCostPerByte = 0.0;
	}

	totalCostSecs = timeCostPerByte * bytesSent;
	totalCost = totalCostSecs * 1000000.0;		/*	usec.	*/
	if (totalCost > currentPaid)
	{
		balanceDue = totalCost - currentPaid;
	}
	else
	{
		balanceDue = 1;
	}

	microsnooze(balanceDue);
	rc->prevPaid = balanceDue;
}
#endif

#if defined (ION_LWT)
int	udplso(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
	       saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char		*endpointSpec = (char *) a1;
	uvast		remoteEngineId = a3 != 0 ? getFqn((char *) a3) :
	       				(a2 != 0 ? getFqn((char *) a2) : 0);
#else
int	main(int argc, char *argv[])
{
	char		*endpointSpec = argc > 1 ? argv[1] : NULL;
	uvast		remoteEngineId = argc > 3 ? getFqn(argv[3]) :
					(argc > 2 ? getFqn(argv[2]) : 0);
#endif
	Sdr			sdr;
	LtpVspan		*vspan;
	PsmAddress		vspanElt;
	unsigned short		portNbr = 0;
	unsigned int		ipAddress = 0;
	struct sockaddr		peerSockName;
	struct sockaddr_in	*peerInetName;
	char			ownHostName[MAXHOSTNAMELEN];
	struct sockaddr		ownSockName;
	struct sockaddr_in	*ownInetName;
	unsigned short		ownPortNbr = 0;
	socklen_t		nameLength;
	static udp_ReceiverThreadParms rtp;  /* Don't create on main's stack */
	pthread_t		receiverThread;
	int			segmentLength;
	char			*segment;
	int			bytesSent;
	int			fd;
	char			quit = '\0';
#ifdef UDP_MULTISEND
	Object			spanObj;
	LtpSpan			spanBuf;
	unsigned int		batchLimit;
	char			*buffers;
	char			*buffer;
	struct iovec		*iovecs;
	struct iovec		*iovec;
	struct mmsghdr		*msgs;
	struct mmsghdr		*msg;
	unsigned int		batchLength;
#else
	RateControlState	rc;
#endif

	/* Initialize the mutex.  */

	pthread_mutex_init(&rtp.lock, NULL); 
	if (remoteEngineId == 0 || endpointSpec == NULL)
	{
		PUTS("Usage: udplso {<remote engine's host name> | @}\
[:<its port number>] <remote engine ID>");
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

	/*	All command-line arguments are now validated.  First
	 *	compute the peer's socket address.			*/

	{
		int retries = UDPLSO_DNS_RETRY_COUNT;
		while (retries > 0 && parseSocketSpec(endpointSpec, &portNbr,
				&ipAddress) != 0)
		{
			char memoBuf[256];
			isprintf(memoBuf, sizeof(memoBuf), "udplso: initial DNS resolution failed for %s, retrying %d more times, retry interval %d second(s).",
			   endpointSpec, retries - 1, UDPLSO_DNS_RETRY_DELAY);
			writeMemoNote("[i] udplso", memoBuf);
			snooze(UDPLSO_DNS_RETRY_DELAY);
			retries--;
		}
		if (ipAddress == 0)
		{
			putErrmsg("udplso: Quit. Can't get IP address for host.", endpointSpec);
			return 1;
		}
	}

	if (portNbr == 0)
	{
		portNbr = LtpUdpDefaultPortNbr;
	}

	if (ipAddress == 0)	/*	Default to own IP address.	*/
	{
		getNameOfHost(ownHostName, sizeof ownHostName);
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
	 *	than to the advertised link service inpud socket.	*/

	ipAddress = INADDR_ANY;
	ownPortNbr = 0;	/*	Let O/S choose it.			*/

	/*	This socket needs to be bound to the local socket
	 *	address (just as in udplsi), so that the udplso
	 *	main thread can send a 1-byte datagram to that
	 *	socket to shut down the datagram handling thread.	*/

	ownPortNbr = htons(ownPortNbr);
	ipAddress = htonl(ipAddress);
	memset((char *) &ownSockName, 0, sizeof ownSockName);
	ownInetName = (struct sockaddr_in *) &ownSockName;
	ownInetName->sin_family = AF_INET;
	ownInetName->sin_port = ownPortNbr;
	memcpy((char *) &(ownInetName->sin_addr.s_addr),
			(char *) &ipAddress, 4);

	/*	Now create the socket that will be used for sending
	 *	datagrams to the peer LTP engine and possibly for
	 *	receiving datagrams from the peer LTP engine.		*/

	rtp.linkSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (rtp.linkSocket < 0)
	{
		putSysErrmsg("LSO can't open UDP socket", NULL);
		return 1;
	}

	/*	Bind the socket to own socket address so that we
	 *	can send a 1-byte datagram to that address to shut
	 *	down the datagram handling thread.			*/

	nameLength = sizeof(struct sockaddr);
	if (reUseAddress(rtp.linkSocket)
	|| bind(rtp.linkSocket, &ownSockName, nameLength) < 0
	|| getsockname(rtp.linkSocket, &ownSockName, &nameLength) < 0)
	{
		closesocket(rtp.linkSocket);
		putSysErrmsg("LSO can't initialize UDP socket", NULL);
		return 1;
	}

	/*	Set up signal handling.  SIGTERM is shutdown signal.	*/

	oK(udplsoSemaphore(&(vspan->segSemaphore)));
	signal(SIGTERM, shutDownLso);

	/*	  Start the receiver thread.		*/

	/* lock not needed here (thread not running yet) */
	rtp.running = 1;

	if (pthread_begin(&receiverThread, NULL, udplsa_handle_datagrams,
			&rtp, "udplso_receiver"))
	{
		closesocket(rtp.linkSocket);
		putSysErrmsg("udplso can't create receiver thread", NULL);
		return 1;
	}

	/*	Can now begin transmitting to remote engine.		*/

	{
		char	memoBuf[1024];

		isprintf(memoBuf, sizeof(memoBuf),
	"[i] udplso is running, spec=[%s:%d], rengine=" UVAST_FIELDSPEC,
			(char *) inet_ntoa(peerInetName->sin_addr),
			ntohs(portNbr), remoteEngineId);
		writeMemo(memoBuf);
	}

#ifdef UDP_MULTISEND
	spanObj = sdr_list_data(sdr, vspan->spanElt);
	sdr_read(sdr, (char *) &spanBuf, spanObj, sizeof(LtpSpan));

	/*	For multi-send, we normally send about one LTP block
	 *	per system call.  But this can be overridden.		*/

#ifdef MULTISEND_BATCH_LIMIT
	batchLimit = MULTISEND_BATCH_LIMIT;
#else
	batchLimit = spanBuf.aggrSizeLimit / spanBuf.maxSegmentSize;
#endif
	buffers = MTAKE(spanBuf.maxSegmentSize * batchLimit);
	if (buffers == NULL)
	{
		closesocket(rtp.linkSocket);
		putErrmsg("No space for segment buffer array.", NULL);
		return 1;
	}

	iovecs = MTAKE(sizeof(struct iovec) * batchLimit);
	if (iovecs == NULL)
	{
		MRELEASE(buffers);
		closesocket(rtp.linkSocket);
		putErrmsg("No space for iovec array.", NULL);
		return 1;
	}

	msgs = MTAKE(sizeof(struct mmsghdr) * batchLimit);
	if (msgs == NULL)
	{
		MRELEASE(iovecs);
		MRELEASE(buffers);
		closesocket(rtp.linkSocket);
		putErrmsg("No space for mmsghdr array.", NULL);
		return 1;
	}

	memset(msgs, 0, sizeof(struct mmsghdr) * batchLimit);
	batchLength = 0;
	buffer = buffers;

	/*
	 *  Loop on rtp.running and the segSemaphore.
	 *  We'll do a local keepRunning to read rtp.running
	 *  under the mutex.			*/
	static int dnsFailed = 0; /* Track DNS failure state */
	static time_t lastDnsCheck = 0; /* Track last DNS check time */

	while (1)
	{
		int keepRunning;

		pthread_mutex_lock(&rtp.lock);
		keepRunning = rtp.running;
		pthread_mutex_unlock(&rtp.lock);

		if (!keepRunning || sm_SemEnded(vspan->segSemaphore))
		{
			break;
		}

		/* Periodic DNS re-resolution */
		time_t now = time(NULL);
		if (now - lastDnsCheck >= UDPLSO_DNS_RECHECK_INTERVAL)
		{
			unsigned int newIpAddress = ipAddress;
			if (parseSocketSpec(endpointSpec, &portNbr,
					&newIpAddress) != 0)
			{
				char memoBuf[256];
				isprintf(memoBuf, sizeof(memoBuf), "Periodic DNS resolution failed for %s", endpointSpec);
				writeMemoNote("[i] udplso", memoBuf);
				dnsFailed = 1;
			}
			else if (newIpAddress != ipAddress)
			{
				pthread_mutex_lock(&rtp.lock);
				ipAddress = newIpAddress;
				ipAddress = htonl(ipAddress);
				memcpy((char *) &(peerInetName->sin_addr.s_addr), (char *) &ipAddress, 4);
				if (dnsFailed)
				{
					char memoBuf[256];
					isprintf(memoBuf, sizeof(memoBuf), "Updated peer IP address to %s", inet_ntoa(peerInetName->sin_addr));
					writeMemoNote("[i] udplso", memoBuf);
					dnsFailed = 0;
				}
				pthread_mutex_unlock(&rtp.lock);
			}
			lastDnsCheck = now;
		}

		/* If no segments ready, wait .1 sec,
		 * then eventually send partial batch if needed.	 */	
		if (sdr_list_length(sdr, spanBuf.segments) == 0)
		{
			/*	No segments ready to append to batch.	*/

			microsnooze(100000);	/*	Wait .1 sec.	*/
			if (sdr_list_length(sdr, spanBuf.segments) == 0)
			{
				/*	Still nothing read to add to
				 *	batch.  Send partial batch,
				 *	if any.				*/

				if (batchLength > 0)
				{
					bytesSent = sendBatch(rtp.linkSocket,
							msgs, batchLength);
					if (bytesSent < 0)
					{
						putErrmsg("Failed sending \
segment batch.", NULL);
						pthread_mutex_lock(&rtp.lock);
						rtp.running = 0;
						pthread_mutex_unlock(&rtp.lock);
						continue;
					}

					batchLength = 0;
					buffer = buffers;

					/*	Let other tasks run.	*/

					sm_TaskYield();
				}
				else
				{
					snooze(1);
				}
			}

			/*	Now see if a segment is waiting.	*/

			continue;
		}

		/*	A segment is waiting to be appended to batch.	*/

		segmentLength = ltpDequeueOutboundSegment(vspan, &segment);
		if (segmentLength < 0)
		{
			pthread_mutex_lock(&rtp.lock);
			rtp.running = 0;	/*	Terminate LSO.	*/
			pthread_mutex_unlock(&rtp.lock);			
			continue;
		}

		if (segmentLength == 0)		/*	Interrupted.	*/
		{
			continue;
		}

		/*	Append this segment to current batch.		*/

		memcpy(buffer, segment, segmentLength);
		iovec = iovecs + batchLength;
		iovec->iov_base = buffer;
		iovec->iov_len = segmentLength;
		msg = msgs + batchLength;
		msg->msg_hdr.msg_name = (struct sockaddr *) peerInetName;
		msg->msg_hdr.msg_namelen = sizeof(struct sockaddr);
		msg->msg_hdr.msg_iov = iovec;
		msg->msg_hdr.msg_iovlen = 1;
		batchLength++;
		buffer += spanBuf.maxSegmentSize;
		if (batchLength >= batchLimit)
		{
			bytesSent = sendBatch(rtp.linkSocket, msgs,
					batchLength);
			if (bytesSent < 0)
			{
				putErrmsg("Failed sending segment batch.",
						NULL);
				pthread_mutex_lock(&rtp.lock);
				rtp.running = 0;
				pthread_mutex_unlock(&rtp.lock);
				continue;
			}

			batchLength = 0;
			buffer = buffers;

			/*	Let other tasks run.			*/

			sm_TaskYield();
		}
	}

	MRELEASE(msgs);
	MRELEASE(iovecs);
	MRELEASE(buffers);
#else
	rc.startTimestamp = getUsecTimestamp();
	rc.prevPaid = 0;
	rc.remoteEngineId = remoteEngineId;
	rc.neighbor = NULL;
	
	static int dnsFailed = 0; /* Track DNS failure state */
	static time_t lastDnsCheck = 0; /* Track last DNS check time */
	
	while (1)
	{
		int keepRunning;

		pthread_mutex_lock(&rtp.lock);
		keepRunning = rtp.running;
		pthread_mutex_unlock(&rtp.lock);

		if (!keepRunning || sm_SemEnded(vspan->segSemaphore))
		{
			break;
		}

		/* Periodic DNS re-resolution */
		time_t now = time(NULL);
		if (now - lastDnsCheck >= UDPLSO_DNS_RECHECK_INTERVAL)
		{
			unsigned int newIpAddress = ipAddress;
			if (parseSocketSpec(endpointSpec, &portNbr,
					&newIpAddress) != 0)
			{
				char memoBuf[256];
				isprintf(memoBuf, sizeof(memoBuf), "udplso: Periodic DNS resolution failed for %s", endpointSpec);
				writeMemoNote("[i] udplso", memoBuf);
				dnsFailed = 1;
			}
			else if (newIpAddress != ipAddress)
			{
				pthread_mutex_lock(&rtp.lock);
				ipAddress = newIpAddress;
				ipAddress = htonl(ipAddress);
				memcpy((char *) &(peerInetName->sin_addr.s_addr), (char *) &ipAddress, 4);
				if (dnsFailed)
				{
					char memoBuf[256];
					isprintf(memoBuf, sizeof(memoBuf), "udplso: Updated peer IP address to %s", inet_ntoa(peerInetName->sin_addr));
					writeMemoNote("[i] udplso", memoBuf);
					dnsFailed = 0;
				}
				pthread_mutex_unlock(&rtp.lock);
			}
			lastDnsCheck = now;
		}

		segmentLength = ltpDequeueOutboundSegment(vspan, &segment);
		if (segmentLength < 0)
		{
			pthread_mutex_lock(&rtp.lock);
			rtp.running = 0;	/*	Terminate LSO.	*/
			pthread_mutex_unlock(&rtp.lock);
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
			pthread_mutex_lock(&rtp.lock);
			rtp.running = 0;	/*	Terminate LSO.	*/
			pthread_mutex_unlock(&rtp.lock);
			continue;
		}

		bytesSent = sendSegmentByUDP(rtp.linkSocket, segment,
				segmentLength, peerInetName);
		if (bytesSent < segmentLength)
		{
			pthread_mutex_lock(&rtp.lock);
			rtp.running = 0;	/*	Terminate LSO.	*/
			pthread_mutex_unlock(&rtp.lock);
			continue;
		}

		bytesSent += IPHDR_SIZE;
		applyRateControl(&rc, bytesSent);

		/*	Let other tasks run.				*/

		sm_TaskYield();
	}
#endif
	/*	Time to shut down.					*/

	pthread_mutex_lock(&rtp.lock);
	rtp.running = 0;
	pthread_mutex_unlock(&rtp.lock);

	/*	Wake up the receiver thread by opening a single-use
	 *	transmission socket and sending a 1-byte datagram
	 *	to the reception socket.				*/

	fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (fd >= 0)
	{
		oK(isendto(fd, &quit, 1, 0, &ownSockName,
				sizeof(struct sockaddr)));
		closesocket(fd);
	}
	pthread_join(receiverThread, NULL);
	closesocket(rtp.linkSocket);

	writeErrmsgMemos();
	writeMemo("[i] udplso has ended.");
	ionDetach();
	return 0;
}
