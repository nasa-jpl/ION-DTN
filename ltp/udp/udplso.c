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

#include "ion_atomic.h"

#define IPV4_HDR_SIZE	  20
#define IPV6_HDR_SIZE	  40
#define UDP_HDR_SIZE	  8
#define UDP_IPV4_HDR_SIZE (IPV4_HDR_SIZE + UDP_HDR_SIZE)
#define UDP_IPV6_HDR_SIZE (IPV6_HDR_SIZE + UDP_HDR_SIZE)

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

static void	shutDownLso(int signum)	/*	Commands LSO termination.	*/
{
	/* Tell the compiler that we are not using 'signum' */
	(void)signum;

	sm_SemEnd(udplsoSemaphore(NULL));
}

/*	*	*	Main thread functions	*	*	*	*/

static unsigned long	getUsecTimestamp(void)
{
	struct timeval	tv;

	getCurrentTime(&tv);
	return ((tv.tv_sec * 1000000) + tv.tv_usec);
}

typedef struct
{
	unsigned long	lastRefillTime;	/*	Microsec timestamp.	*/
	uvast		remoteEngineId;
	IonNeighbor	*neighbor;
	long		tokens;		/*	Available bytes to send.*/
	long		bucketSize;	/*	Max burst (bytes).	*/
} TokenBucketState;

/*	Conservative floor for nanosleep granularity (microseconds).
 *	Below this threshold the kernel oversleeps significantly;
 *	the deficit carries forward and is compensated on the next
 *	refill via the elapsed-time measurement.			*/

#ifndef NANOSLEEP_FLOOR_USEC
#define NANOSLEEP_FLOOR_USEC	250
#endif

static void	applyTokenBucket(TokenBucketState *tb, int bytesSent)
{
	unsigned long	now;
	unsigned long	elapsed;
	unsigned long	rate;
	unsigned long	added;
	PsmAddress	nextElt;

	now = getUsecTimestamp();
	elapsed = now - tb->lastRefillTime;
	tb->lastRefillTime = now;

	/*	Look up current xmit rate.				*/

	if (tb->neighbor == NULL)
	{
		tb->neighbor = findNeighbor(getIonVdb(),
				tb->remoteEngineId, &nextElt);
	}

	if (tb->neighbor && tb->neighbor->xmitRate > 0)
	{
		rate = tb->neighbor->xmitRate;
	}
	else	/*	No link service rate control.			*/
	{
		return;
	}

	/*	Refill tokens based on elapsed time.			*/

	added = (elapsed * rate) / 1000000;
	tb->tokens += added;
	if (tb->tokens > tb->bucketSize)
	{
		tb->tokens = tb->bucketSize;
	}

	/*	Deduct bytes sent.					*/

	tb->tokens -= bytesSent;

	/*	If bucket is empty, pace transmission.			*/

	if (tb->tokens < 0)
	{
		unsigned long	deficit;
		unsigned long	waitUsec;

		deficit = (unsigned long)(-(tb->tokens));
		waitUsec = (deficit * 1000000) / rate;

		/*	Only sleep if the wait exceeds the
		 *	nanosleep granularity floor.  Below
		 *	this threshold the kernel oversleeps
		 *	significantly; the deficit carries
		 *	forward and is compensated on the
		 *	next refill via elapsed time.		*/

		if (waitUsec > NANOSLEEP_FLOOR_USEC)
		{
			microsnooze(waitUsec);
		}
	}
}

#ifdef UDP_MULTISEND
static int sendBatch(int linkSocket, struct mmsghdr *msgs,
		unsigned int batchLength, int hdrSize)
{
	int		totalBytesSent = 0;
	int		bytesSent;
	unsigned int	i;
	int		result;
	int		savedErrno;
	static int	networkErrorState = 0;

	result = sendmmsg(linkSocket, msgs, batchLength, 0);
	if (result < 0)
	{
		savedErrno = errno;

		/*	Network-layer errors that indicate the
		 *	packets cannot be delivered right now.
		 *	For UDP (connectionless), these should
		 *	be treated as packet loss, not fatal
		 *	errors. The daemon should continue
		 *	running to handle temporary conditions
		 *	like iptables changes, link flaps, or
		 *	simulated down spacelinks.		*/

		if (savedErrno == ENETUNREACH
		|| savedErrno == EHOSTUNREACH
		|| savedErrno == ENETDOWN
		|| savedErrno == ENOBUFS
		|| savedErrno == EPERM
		|| savedErrno == EACCES
		|| savedErrno == EAGAIN
		|| savedErrno == EWOULDBLOCK
#ifdef ECONNREFUSED
		|| savedErrno == ECONNREFUSED
#endif
#ifdef EHOSTDOWN
		|| savedErrno == EHOSTDOWN
#endif
			)
		{
			/*	Log on state change only.	*/

			if (!networkErrorState)
			{
				char memoBuf[256];

				isprintf(memoBuf, sizeof(memoBuf),
					"[i] udplso: network error (errno=%d), treating as packet loss until recovered",
					savedErrno);
				writeMemo(memoBuf);
				networkErrorState = 1;
			}

			/*	Return 0 to indicate no bytes sent
			 *	but not a fatal error.		*/

			return 0;
		}

		/*	Fatal error - log and return -1.	*/

		putSysErrmsg("Failed in sendmmsg", itoa(batchLength));
		return -1;
	}

	/*	Success - log recovery if we were in error state.	*/

	if (networkErrorState)
	{
		writeMemo("[i] udplso: network recovered, sends succeeding");
		networkErrorState = 0;
	}

	for (i = 0; i < batchLength; i++)
	{
		bytesSent = msgs[i].msg_len;
		if (bytesSent > 0)
		{
			totalBytesSent += (hdrSize + bytesSent);
		}
	}

	return totalBytesSent;
}
#else

/*	Maximum retries for EAGAIN/EWOULDBLOCK before treating as loss.	*/
#ifndef UDPLSO_EAGAIN_RETRIES
#define UDPLSO_EAGAIN_RETRIES	10
#endif

/*	Microseconds to wait between EAGAIN retries.			*/
#ifndef UDPLSO_EAGAIN_WAIT_USEC
#define UDPLSO_EAGAIN_WAIT_USEC	1000
#endif

int	sendSegmentByUDP(int linkSocket, char *from, int length,
		struct sockaddr *destAddr, socklen_t addrLen)
{
	int		bytesWritten;
	int		eagainRetries = 0;
	int		savedErrno;
	static int	networkErrorState = 0;

	while (1)	/*	Continue until interrupted.		*/
	{
		bytesWritten = sendto(linkSocket, from, length, 0,
				destAddr, addrLen);
		if (bytesWritten < 0)
		{
			savedErrno = errno;

			if (savedErrno == EINTR)	/*	Interrupted.	*/
			{
				continue;	/*	Retry.		*/
			}

			/*	Buffer full - retry with backoff.	*/

			if (savedErrno == EAGAIN || savedErrno == EWOULDBLOCK)
			{
				if (eagainRetries < UDPLSO_EAGAIN_RETRIES)
				{
					eagainRetries++;
					microsnooze(UDPLSO_EAGAIN_WAIT_USEC);
					continue;
				}

				/*	Exhausted retries, treat as loss. */

				return length;
			}

			/*	Network-layer errors that indicate the
			 *	packet cannot be delivered right now.
			 *	For UDP (connectionless), these should
			 *	be treated as packet loss, not fatal
			 *	errors. The daemon should continue
			 *	running to handle temporary conditions
			 *	like iptables changes, link flaps, or
			 *	simulated down spacelinks.		*/

			if (savedErrno == ENETUNREACH
			|| savedErrno == EHOSTUNREACH
			|| savedErrno == ENETDOWN
			|| savedErrno == ENOBUFS
			|| savedErrno == EPERM
			|| savedErrno == EACCES
#ifdef ECONNREFUSED
			|| savedErrno == ECONNREFUSED
#endif
#ifdef EHOSTDOWN
			|| savedErrno == EHOSTDOWN
#endif
				)
			{
				/*	Log on state change only.	*/

				if (!networkErrorState)
				{
					char memoBuf[256];

					isprintf(memoBuf, sizeof(memoBuf),
						"[i] udplso: network error (errno=%d), treating as packet loss until recovered",
						savedErrno);
					writeMemo(memoBuf);
					networkErrorState = 1;
				}

				return length;	/*	Just data loss.	*/
			}

			/*	For other errors (e.g., EBADF, EFAULT,
			 *	EINVAL), log with details and return
			 *	error to trigger shutdown - these
			 *	indicate a programming/config error.	*/

			char memoBuf[1000];
			char addrStr[INET6_ADDRSTRLEN + 10];

			if (destAddr->sa_family == AF_INET)
			{
				struct sockaddr_in *sin = (struct sockaddr_in*)(void *)destAddr;
				inet_ntop(AF_INET, &sin->sin_addr, addrStr, sizeof(addrStr));
				isprintf(memoBuf, sizeof(memoBuf),
					"udplso sendto() error, dest=%s:%d, nbytes=%d, rv=%d, errno=%d",
					addrStr, ntohs(sin->sin_port), length, bytesWritten, savedErrno);
			}
			else if (destAddr->sa_family == AF_INET6)
			{
				struct sockaddr_in6 *sin6 = (struct sockaddr_in6*)(void *)destAddr;
				inet_ntop(AF_INET6, &sin6->sin6_addr, addrStr, sizeof(addrStr));
				isprintf(memoBuf, sizeof(memoBuf),
					"udplso sendto() error, dest=[%s]:%d, nbytes=%d, rv=%d, errno=%d",
					addrStr, ntohs(sin6->sin6_port), length, bytesWritten, savedErrno);
			}
			writeMemo(memoBuf);
		}
		else if (networkErrorState)
		{
			/*	Successful send after errors - log recovery. */

			writeMemo("[i] udplso: network recovered, sends succeeding");
			networkErrorState = 0;
		}

		return bytesWritten;
	}
}
#endif

#if defined (ION_LWT)
int	udplso(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char *endpointSpec = (char *) a1;
	uvast remoteEngineId = a3 != 0 ? getFqn((char *) a3) :
					 (a2 != 0 ? getFqn((char *) a2) : 0);
#else
int	main(int argc, char *argv[])
{
	char *endpointSpec = argc > 1 ? argv[1] : NULL;
	uvast remoteEngineId = argc > 3 ? getFqn(argv[3]) :
					  (argc > 2 ? getFqn(argv[2]) : 0);
#endif
	Sdr			       sdr;
	LtpVspan		      *vspan;
	PsmAddress		       vspanElt;
	char			       peerAddrStr[INET6_ADDRSTRLEN + 10];
	char			       localAddrStr[INET6_ADDRSTRLEN + 10];
	static udp_ReceiverThreadParms rtp; /* Don't create on main's stack */
	pthread_t		       receiverThread;
	int			       segmentLength;
	char			      *segment;
	int			       bytesSent;
	int			       fd;
	int			       hdrSize = UDP_IPV4_HDR_SIZE;
	char			       quit = '\0';
#ifdef UDP_MULTISEND
	SdrObject	spanObj;
	LtpSpan		spanBuf;
	unsigned int	batchLimit;
	char	       *buffers;
	char	       *buffer;
	struct iovec   *iovecs;
	struct iovec   *iovec;
	struct mmsghdr *msgs;
	struct mmsghdr *msg;
	unsigned int	batchLength;
#endif
	TokenBucketState tb;

	/* Initialize the mutex.  */

	pthread_mutex_init(&rtp.lock, NULL);

	if (remoteEngineId == 0 || endpointSpec == NULL)
	{
		PUTS("Usage: udplso {<remote engine's host name> | @}[:<its port number>] <remote engine ID>");
		PUTS("  IPv4: udplso 192.168.1.1:1113 42");
		PUTS("  IPv6: udplso [2001:db8::1]:1113 42");
		PUTS("  Auto: udplso node.example.com:1113 42");
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

	/*  Resolve peer's address resolution and retry if needed */
	{
		int retries = UDPLSO_DNS_RETRY_COUNT;
		int resolveResult = -1;

		while (retries > 0)
		{
			resolveResult = resolveNetworkAddressCached(endpointSpec, &rtp.peer_addr);
			if (resolveResult >= 0)
			{
				break;  /* Success */
			}

			char memoBuf[256];
			isprintf(memoBuf, sizeof(memoBuf),
					"udplso: DNS resolution failed for %s, retrying %d more times, retry interval %d second(s).",
					endpointSpec, retries - 1, UDPLSO_DNS_RETRY_DELAY);
			writeMemoNote("[i] udplso", memoBuf);
			snooze(UDPLSO_DNS_RETRY_DELAY);
			retries--;
		}

		if (resolveResult < 0)
		{
			putErrmsg("udplso: Maximum DNS retries reached, can't resolve peer address", endpointSpec);
			return 1;
		}
	}

	/* Set default port if not specified */
	if (rtp.peer_addr.family == AF_INET)
	{
		struct sockaddr_in *sin = (struct sockaddr_in*)&rtp.peer_addr.addr;
		if (sin->sin_port == 0)
		{
			sin->sin_port = htons(LtpUdpDefaultPortNbr);
		}
	}
	else if (rtp.peer_addr.family == AF_INET6)
	{
		hdrSize = UDP_IPV6_HDR_SIZE;
		struct sockaddr_in6 *sin6 = (struct sockaddr_in6*)&rtp.peer_addr.addr;
		if (sin6->sin6_port == 0)
		{
			sin6->sin6_port = htons(LtpUdpDefaultPortNbr);
		}
	}

	/* Prepare local socket address (any address, system-chosen port) */
	memset(&rtp.local_addr, 0, sizeof(rtp.local_addr));
	rtp.local_addr.family = rtp.peer_addr.family;  /* Match peer's family */

	if (rtp.peer_addr.family == AF_INET)
	{
		struct sockaddr_in *sin = (struct sockaddr_in*)&rtp.local_addr.addr;
		sin->sin_family = AF_INET;
		sin->sin_addr.s_addr = INADDR_ANY;
		sin->sin_port = 0;  /* Let OS choose */
		rtp.local_addr.addr_len = sizeof(struct sockaddr_in);
	}
	else if (rtp.peer_addr.family == AF_INET6)
	{
		struct sockaddr_in6 *sin6 = (struct sockaddr_in6*)&rtp.local_addr.addr;
		sin6->sin6_family = AF_INET6;
		sin6->sin6_addr = in6addr_any;
		sin6->sin6_port = 0;  /* Let OS choose */
		rtp.local_addr.addr_len = sizeof(struct sockaddr_in6);
	}

	/*
	 * Create and bind socket using helper function.  This is a sending
	 * socket, bound to the wildcard address of the peer's own family, so
	 * it must not be made dual-stack: the destination family always
	 * matches the peer, and widening the ephemeral port to IPv4 serves no
	 * purpose.
	 */
	if (createNetworkSocketEx(SOCK_DGRAM, &rtp.local_addr, ION_SOCK_V6ONLY,
			    &rtp.linkSocket) < 0)
	{
		putErrmsg("udplso: Can't create and bind UDP socket", NULL);
		return 1;
	}

	/* Get actual bound address for shutdown signaling */
	socklen_t addr_len = sizeof(rtp.local_addr.addr);
	if (getsockname(rtp.linkSocket, (struct sockaddr*)&rtp.local_addr.addr, &addr_len) < 0)
	{
		closesocket(rtp.linkSocket);
		putSysErrmsg("LSO can't get socket name", NULL);
		return 1;
	}
	rtp.local_addr.addr_len = addr_len;
	rtp.local_addr.family = rtp.local_addr.addr.ss_family;

	/*	Set up signal handling.  SIGTERM is shutdown signal.	*/

	oK(udplsoSemaphore(&(vspan->segSemaphore)));
	signal(SIGTERM, shutDownLso);

	/*	  Start the receiver thread.		*/

	/* lock not needed here (thread not running yet) */
	ion_atomic_init(&rtp.running, 1);

	if (pthread_begin(&receiverThread, NULL, udplsa_handle_datagrams,
			&rtp, "udplso_receiver"))
	{
		closesocket(rtp.linkSocket);
		putSysErrmsg("udplso can't create receiver thread", NULL);
		return 1;
	}

	/*  Log startup information */
	formatNetworkAddress(&rtp.peer_addr, peerAddrStr, sizeof(peerAddrStr));
	formatNetworkAddress(&rtp.local_addr, localAddrStr, sizeof(localAddrStr));
	{
		char memoBuf[1024];
		isprintf(memoBuf, sizeof(memoBuf),
			"[i] udplso is running, local=%s (%s), peer=%s (%s), rengine=%d.",
			localAddrStr,
			(rtp.local_addr.family == AF_INET6) ? "IPv6" : "IPv4",
			peerAddrStr,
			(rtp.peer_addr.family == AF_INET6) ? "IPv6" : "IPv4",
			(int) remoteEngineId);
		writeMemo(memoBuf);
	}

	/*	Can now begin transmitting to remote engine.		*/

#ifdef UDP_MULTISEND
	spanObj = sdr_list_data(sdr, vspan->spanElt);
	sdr_read(sdr, (char *) &spanBuf, spanObj, sizeof(LtpSpan));

	/*	For multi-send, we normally send about one LTP block
	 *	per system call.  But this can be overridden.		*/

#ifdef MULTISEND_BATCH_LIMIT
	batchLimit = MULTISEND_BATCH_LIMIT;
#else
	batchLimit = spanBuf.aggrSizeLimit / spanBuf.maxSegmentSize;
	if (batchLimit < 1)
	{
		batchLimit = 1;
	}
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

	tb.lastRefillTime = getUsecTimestamp();
	tb.tokens = 0;

	/*
	 * UDPLSA_BUFSZ can be increased for improved LTP performance
	 * on hosts with increased kernel buffer sizes. On Linux, x2
	 * can be considered safe with the default kernel buffer size.
	 */
	tb.bucketSize = UDPLSA_BUFSZ * 2;
	tb.remoteEngineId = remoteEngineId;
	tb.neighbor = NULL;

	/*
	 *  Loop on rtp.running and the segSemaphore.
	 *  We'll do a local keepRunning to read rtp.running
	 *  under the mutex.			*/
	static int dnsFailed = 0; /* Track DNS failure state */
	static time_t lastDnsCheck = 0; /* Track last DNS check time */

	while (1)
	{
		int keepRunning;
		keepRunning = (int) ion_atomic_get(&rtp.running);

		if (!keepRunning || sm_SemEnded(vspan->segSemaphore))
		{
			break;
		}

		/* Periodic DNS re-resolution */
		time_t now = time(NULL);
		if (now - lastDnsCheck >= UDPLSO_DNS_RECHECK_INTERVAL)
		{
			IonNetworkAddress newPeerAddr;
			if (resolveNetworkAddressCached(endpointSpec, &newPeerAddr) < 0)
			{
				char memoBuf[256];
				isprintf(memoBuf, sizeof(memoBuf),
						"udplso: Periodic DNS resolution failed for %s", endpointSpec);
				writeMemoNote("[i] udplso", memoBuf);
				dnsFailed = 1;
			}
			else
			{
				/* Check if address changed */
				int changed = 0;
				if (newPeerAddr.family != rtp.peer_addr.family)
				{
					changed = 1;
				}
				else if (newPeerAddr.family == AF_INET)
				{
					struct sockaddr_in *old_sin = (struct sockaddr_in*)&rtp.peer_addr.addr;
					struct sockaddr_in *new_sin = (struct sockaddr_in*)&newPeerAddr.addr;
					if (old_sin->sin_addr.s_addr != new_sin->sin_addr.s_addr)
					{
						changed = 1;
					}
				}
				else if (newPeerAddr.family == AF_INET6)
				{
					struct sockaddr_in6 *old_sin6 = (struct sockaddr_in6*)&rtp.peer_addr.addr;
					struct sockaddr_in6 *new_sin6 = (struct sockaddr_in6*)&newPeerAddr.addr;
					if (memcmp(&old_sin6->sin6_addr, &new_sin6->sin6_addr, sizeof(struct in6_addr)) != 0)
					{
						changed = 1;
					}
				}

				if (changed)
				{
					pthread_mutex_lock(&rtp.lock);
					rtp.peer_addr = newPeerAddr;
					pthread_mutex_unlock(&rtp.lock);

					if (dnsFailed)
					{
						formatNetworkAddress(&newPeerAddr, peerAddrStr, sizeof(peerAddrStr));
						char memoBuf[256];
						isprintf(memoBuf, sizeof(memoBuf),
								"udplso: Updated peer address to %s", peerAddrStr);
						writeMemoNote("[i] udplso", memoBuf);
						dnsFailed = 0;
					}
				}
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
							msgs, batchLength,
							hdrSize);
					if (bytesSent < 0)
					{
						putErrmsg("Failed sending \
segment batch.", NULL);
						ion_atomic_set(&rtp.running, 0);
						continue;
					}

					applyTokenBucket(&tb, bytesSent);
					batchLength = 0;
					buffer = buffers;

					/*	Let other tasks run.	*/

					sm_TaskYield();
				}
				else
				{
					sm_SemTake(vspan->segSemaphore);
				}
			}

			/*	Now see if a segment is waiting.	*/

			continue;
		}

		/*	A segment is waiting to be appended to batch.	*/

		segmentLength = ltpDequeueOutboundSegment(vspan, &segment);
		if (segmentLength < 0)
		{
			ion_atomic_set(&rtp.running, 0);
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
		msg->msg_hdr.msg_name = (struct sockaddr*)&rtp.peer_addr.addr;
		msg->msg_hdr.msg_namelen = rtp.peer_addr.addr_len;
		msg->msg_hdr.msg_iov = iovec;
		msg->msg_hdr.msg_iovlen = 1;
		batchLength++;
		buffer += spanBuf.maxSegmentSize;
		if (batchLength >= batchLimit)
		{
			bytesSent = sendBatch(rtp.linkSocket, msgs, batchLength,
					hdrSize);
			if (bytesSent < 0)
			{
				putErrmsg("Failed sending segment batch.",
						NULL);
				ion_atomic_set(&rtp.running, 0);
				continue;
			}

			applyTokenBucket(&tb, bytesSent);
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
	tb.lastRefillTime = getUsecTimestamp();
	tb.tokens = 0;
	tb.bucketSize = UDPLSA_BUFSZ * 2;
	tb.remoteEngineId = remoteEngineId;
	tb.neighbor = NULL;

	static int dnsFailed = 0; /* Track DNS failure state */
	static time_t lastDnsCheck = 0; /* Track last DNS check time */

	while (1)
	{
		int keepRunning;
		keepRunning = (int) ion_atomic_get(&rtp.running);

		if (!keepRunning || sm_SemEnded(vspan->segSemaphore))
		{
			break;
		}

		/* Periodic DNS re-resolution */
		time_t now = time(NULL);
		if (now - lastDnsCheck >= UDPLSO_DNS_RECHECK_INTERVAL)
		{
			IonNetworkAddress newPeerAddr;
			if (resolveNetworkAddressCached(endpointSpec, &newPeerAddr) < 0)
			{
				char memoBuf[256];
				isprintf(memoBuf, sizeof(memoBuf),
						"udplso: Periodic DNS resolution failed for %s", endpointSpec);
				writeMemoNote("[i] udplso", memoBuf);
				dnsFailed = 1;
			}
			else
			{
				/* Check if address changed */
				int changed = 0;
				if (newPeerAddr.family != rtp.peer_addr.family)
				{
					changed = 1;
				}
				else if (newPeerAddr.family == AF_INET)
				{
					struct sockaddr_in *old_sin = (struct sockaddr_in*)&rtp.peer_addr.addr;
					struct sockaddr_in *new_sin = (struct sockaddr_in*)&newPeerAddr.addr;
					if (old_sin->sin_addr.s_addr != new_sin->sin_addr.s_addr)
					{
						changed = 1;
					}
				}
				else if (newPeerAddr.family == AF_INET6)
				{
					struct sockaddr_in6 *old_sin6 = (struct sockaddr_in6*)&rtp.peer_addr.addr;
					struct sockaddr_in6 *new_sin6 = (struct sockaddr_in6*)&newPeerAddr.addr;
					if (memcmp(&old_sin6->sin6_addr, &new_sin6->sin6_addr, sizeof(struct in6_addr)) != 0)
					{
						changed = 1;
					}
				}

				if (changed)
				{
					pthread_mutex_lock(&rtp.lock);
					rtp.peer_addr = newPeerAddr;
					pthread_mutex_unlock(&rtp.lock);

					if (dnsFailed)
					{
						formatNetworkAddress(&newPeerAddr, peerAddrStr, sizeof(peerAddrStr));
						char memoBuf[256];
						isprintf(memoBuf, sizeof(memoBuf),
								"udplso: Updated peer address to %s", peerAddrStr);
						writeMemoNote("[i] udplso", memoBuf);
						dnsFailed = 0;
					}
				}
			}
			lastDnsCheck = now;
		}

		segmentLength = ltpDequeueOutboundSegment(vspan, &segment);
		if (segmentLength < 0)
		{
			ion_atomic_set(&rtp.running, 0);
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
			ion_atomic_set(&rtp.running, 0);
			continue;
		}

		bytesSent = sendSegmentByUDP(rtp.linkSocket, segment, segmentLength,
				(struct sockaddr*)&rtp.peer_addr.addr,
				rtp.peer_addr.addr_len);
		if (bytesSent < segmentLength)
		{
			ion_atomic_set(&rtp.running, 0);
			continue;
		}

		bytesSent += hdrSize;
		applyTokenBucket(&tb, bytesSent);

		/*	Let other tasks run.				*/

		sm_TaskYield();
	}
#endif
	/*	Time to shut down.					*/

	ion_atomic_set(&rtp.running, 0);

	/*	Wake up the receiver thread by opening a single-use
	 *	transmission socket and sending a 1-byte datagram
	 *	to the reception socket.				*/

	fd = socket(rtp.local_addr.family, SOCK_DGRAM, IPPROTO_UDP);
	if (fd >= 0)
	{
		/* Prepare shutdown target address */
		IonNetworkAddress shutdown_target = rtp.local_addr;

		/* Handle "any address" cases for shutdown signal */
		if (rtp.local_addr.family == AF_INET)
		{
			struct sockaddr_in *sin = (struct sockaddr_in*)&shutdown_target.addr;
			if (sin->sin_addr.s_addr == INADDR_ANY)
			{
				sin->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
			}
		}
		else if (rtp.local_addr.family == AF_INET6)
		{
			struct sockaddr_in6 *sin6 = (struct sockaddr_in6*)&shutdown_target.addr;
			if (IN6_IS_ADDR_UNSPECIFIED(&sin6->sin6_addr))
			{
				sin6->sin6_addr = in6addr_loopback;
			}
		}

		/* Send shutdown signal */
		if (sendto(fd, &quit, 1, 0,
				(struct sockaddr*)&shutdown_target.addr,
				shutdown_target.addr_len) == 1)
		{
			pthread_join(receiverThread, NULL);
		}
		else
		{
			writeMemo("[!] udplso: Shutdown signal failed, forcing thread termination");
			pthread_cancel(receiverThread);
			pthread_join(receiverThread, NULL);
		}

		closesocket(fd);
	}
	else
	{
		writeMemo("[!] udplso: Can't create shutdown socket, forcing thread termination");
		pthread_cancel(receiverThread);
		pthread_join(receiverThread, NULL);
	}

	closesocket(rtp.linkSocket);
	ion_atomic_mutex_destroy(&rtp.running);

	writeErrmsgMemos();
	writeMemo("[i] udplso has ended.");
	ionDetach();
	return 0;
}
