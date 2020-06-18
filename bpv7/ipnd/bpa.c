/*
 *	bpa.c -- DTN IP Neighbor Discovery (IPND). Main IPND threads. Include:
 *	Send beacons thread.
 *	Receive beacon thread.
 *	Expire neighbors thread.
 *
 *	Copyright (c) 2015, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *	Author: Gerard Garcia, TrePe
 *	Version 1.0 2015/05/09 Gerard Garcia
 *	Version 2.0 DTN Neighbor Discovery 
 *		- ION IPND Implementation Assembly Part2
 *	Version 2.1 DTN Neighbor Discovery - ION IPND Fix Defects and Issues
 *	Version 2.2 Shared context ctx passed explicitely to threads to avoid shared library security change implications
 */

#include "platform.h"
#include "bpa.h"
#include "ipndP.h"
#include "eureka.h"

/**
 * Sets up sending sockets.
 * @param  multicastTTL TTL for multicast packets
 * @param  enableBroadcastSending: Enables sending broadcast packets
 *                                 from this socket.
 * @return     Socket on success, -1 on error.
 */
static int	setUpSendingSocket(const int multicastTTL,
		const int enabledBroadcastSending)
{
	/* Sets up sending sockets */

	int	sendSocket;
#ifdef sparc
	char	multicastLoopSockOption;
	char	multicastTTLSockOption;
#else
	int	multicastLoopSockOption;
	int	multicastTTLSockOption;
#endif
	int	broadcastDiscoverySockOption;

	/* Initialize sending socket */
	sendSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (sendSocket < 0)
	{
		putSysErrmsg("send-thread: Can't open beacon sending socket.",
				NULL);
		return -1;
	}

	/* Set multicast loop option to avoid receiving
	   our multicast sent beacons */
	multicastLoopSockOption = 0;
	if (setsockopt(sendSocket,
			IPPROTO_IP,
			IP_MULTICAST_LOOP,
			(void *) &multicastLoopSockOption,
			sizeof(multicastLoopSockOption)) < 0)
	{
		putSysErrmsg("send-thread: Can't set multicast loop option \
for beacon sending socket.", NULL);
	}

	/* Set multicast ttl option */
	multicastTTLSockOption = multicastTTL;
	if (setsockopt(sendSocket,
			IPPROTO_IP,
			IP_MULTICAST_TTL,
			(void *) &multicastTTLSockOption,
			sizeof(multicastTTLSockOption)) < 0)
	{
		putSysErrmsg("send-thread: Can't set multicast TTL option \
for beacon sending socket.", NULL);
	}

	if (enabledBroadcastSending)
	{
		/* Set broadcast sending options*/
		broadcastDiscoverySockOption = 1;
		if (setsockopt(sendSocket,
				SOL_SOCKET,
				SO_BROADCAST,
				(void *) &broadcastDiscoverySockOption,
				sizeof(broadcastDiscoverySockOption)) < 0)
		{
			putSysErrmsg("send-thread: Can't set broadcast \
sending option", NULL);
		}
	}

	return sendSocket;
}

/**
 * Sends a Beacon to dest using socket.
 * @param  beacon Beacon to send.
 * @param  dest   Beacon destination.
 * @param  socket Socket to use.
 * @return        0 on success, -1 on error.
 */
static int	sendBeacon(Beacon *beacon, Destination *dest, int socket)
{
	char		buffer[80];
	int		rawBeaconLength = 0;
	unsigned char	*rawBeacon = NULL;
	struct		sockaddr_in dest_addr;

	/* Serialize beacon */
	if ((rawBeaconLength = serializeBeacon(beacon, &rawBeacon)) < 0)
	{
		putErrmsg("send-thread: Can't serialize beacon.", NULL);
		MRELEASE(rawBeacon);
		return -1;
	}

	/* Send beacon */
	memset(&dest_addr, 0, sizeof(dest_addr));
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_addr.s_addr = inet_addr(dest->addr.ip);
	dest_addr.sin_port = htons(dest->addr.port);

	if (isendto(socket, (char *) rawBeacon,
			rawBeaconLength,
			0,
			(struct sockaddr *)&dest_addr,
			sizeof(dest_addr)) != rawBeaconLength)
	{
		isprintf(buffer, sizeof buffer, "send-thread: Error sending \
beacon (%dB) to %s:%d.", rawBeaconLength, dest->addr.ip, dest->addr.port);
		putSysErrmsg(buffer, NULL);
		MRELEASE(rawBeacon);
		return -1;
	}

#if IPND_DEBUG
	isprintf(buffer, sizeof buffer, "[i] send-thread: Beacon (%dB) sent \
to %s:%d correctly: ", rawBeaconLength, dest->addr.ip, dest->addr.port);
	printText(buffer);
#endif
	logBeacon(beacon);

	MRELEASE(rawBeacon);
	return 0;
}

/**
 * Send beacons thread.
 * @param  attr Thread parameters
 * @return      NULL on success, -1 on error. We return a casted -1 on error
 *              because this function is intended to be run as a thread and
 *              pthread_create expects a function with signature
 *              void *(*start_routine)
 */
void	*sendBeacons(void *attr)
{
#if IPND_DEBUG
	char		buffer[200];
#endif
	IPNDCtx		*ctx = (IPNDCtx *)attr;
	int		sendSocket;
	int		result;
	Lyst		destinations;
	LystElt		nDestinationElt = NULL;
	Destination	*nDestination = NULL;
	int		waitNextDestination = 0;
	Beacon		*oldBeacon = NULL;
	Beacon		newBeacon = {0};
	int		beaconHasChanged;

	CHKNULL(ctx);

	setIPNDCtx(ctx);

	destinations = ctx->destinations;

	printText("[i] send-thread: Send beacons thread started.");

	lockResource(&ctx->configurationLock);
	sendSocket = setUpSendingSocket(ctx->multicastTTL,
			ctx->enabledBroadcastSending);
	unlockResource(&ctx->configurationLock);

	if (sendSocket < 0)
	{
		putErrmsg("send-thread: Error initializing sending socket.",
				NULL);
		return (void *) - 1;
	}

	waitNextDestination = 0;
	while (1)
	{
		/* Wait until destinations lyst is not empty */
		result = llcv_wait(ctx->destinationsCV, llcv_lyst_not_empty,
				LLCV_BLOCKING);
		if (result < 0)
		{
			putErrmsg("send-thread: Error waiting for destinations \
lyst.", NULL);
			close(sendSocket);
			return (void *) - 1;
		}

		llcv_lock(ctx->destinationsCV);
		nDestinationElt = lyst_length(destinations) > 0 ?
				lyst_first(destinations) : NULL;
		if (nDestinationElt == NULL
		|| (nDestination = lyst_data(nDestinationElt)) == NULL)
		{
			llcv_unlock(ctx->destinationsCV);
			continue;
		}

		waitNextDestination = nDestination->nextAnnounceTimestamp
				- time(NULL);

		/* Wait until it is time to send the next beacon */
		if (waitNextDestination > 0)
		{
			llcv_unlock(ctx->destinationsCV);
			snooze(waitNextDestination);
			llcv_lock(ctx->destinationsCV);
		}	/* Otherwise, period has already expired. */

		/* Get first destination again in case previous destination
		 * has been deleted or a new destination has been added. */
		if (lyst_first(destinations) != nDestinationElt)
		{
			llcv_unlock(ctx->destinationsCV);
			continue;
		}

		/* Reset state */
		oldBeacon = NULL;

#if IPND_DEBUG
		isprintf(buffer, sizeof buffer, "[i] send-thread: Started \
beacon sending to %s:%d.", nDestination->addr.ip, nDestination->addr.port);
		printText(buffer);
#endif

		/* Get previously sent beacon */
		oldBeacon = &nDestination->beacon;
		if (!nDestination->beaconInitialized)
		{
#if IPND_DEBUG
			printText("[i] send-thread: Previously sent beacon not \
found.");
#endif

			/* If beacon has not been initialized,
			   just create a new beacon and send it */
			if (populateBeacon(&newBeacon,
					nDestination->announcePeriod) < 0)
			{
				putErrmsg("send-thread: Can't initialize \
beacon.", NULL);
				llcv_unlock(ctx->destinationsCV);
				continue;
			}

			if (sendBeacon(&newBeacon, nDestination, sendSocket)
					< 0)
			{
				putErrmsg("send-thread: Can't send beacon.",
						NULL);
				llcv_unlock(ctx->destinationsCV);
				continue;
			}

			/* Store newly created beacon as last sent beacon
			   to this destination */
			copyBeacon(&nDestination->beacon, &newBeacon);
			nDestination->beaconInitialized = 1;

			// free beacon's dynamic memory
			clearBeacon(&newBeacon);
		}
		else
		{
#if IPND_DEBUG
			isprintf(buffer, sizeof buffer, "[i] send-thread: \
Previously sent beacon found: ", NULL);
			printText(buffer);
#endif
			logBeacon(oldBeacon);

			beaconHasChanged = beaconChanged(oldBeacon,
					nDestination->announcePeriod);
			if (beaconHasChanged)
			{
#if IPND_DEBUG
				printText("[i] send-thread: Beacon changed.");
#endif

				/* Create a new beacon. */
				if (populateBeacon(&newBeacon,
					nDestination->announcePeriod) < 0)
				{
					putErrmsg("send-thread: Can't update \
beacon.", NULL);
					llcv_unlock(ctx->destinationsCV);
					continue;
				}

				/* Set old sequence number to new beacon
				 * and send it*/
				newBeacon.sequenceNumber =
						oldBeacon->sequenceNumber + 1;
				if (sendBeacon(&newBeacon, nDestination,
						sendSocket) < 0)
				{
					putErrmsg("send-thread: Can't send \
beacon.", NULL);
					llcv_unlock(ctx->destinationsCV);
					continue;
				}

				/* Store newly created beacon as last
				 * sent beacon to this destination */
				copyBeacon(&nDestination->beacon, &newBeacon);
				/* free beacon's dynamic memory */
				clearBeacon(&newBeacon);
			}
			/* else if (!beaconHasChanged
			 * && !hasAnActiveConnection
			 * (nDestination->beacon.canonicalEid)) */
			else if (!beaconHasChanged
			&& !hasAnActiveConnection(nDestination->eid,
					nDestination->announcePeriod))
			{
#if IPND_DEBUG
				isprintf(buffer, sizeof buffer, "[i] send-\
thread: Beacon has not changed and node does not have an active connection \
with %s:%d.", nDestination->addr.ip, nDestination->addr.port);
				printText(buffer);
#endif

				/*  If beacon has not changed and we
				 *  don't have an active connection
				 *  with this destination just update
				 *  the sequence number and send the old
				 *  beacon again. */

				oldBeacon->sequenceNumber++;
				if (sendBeacon(oldBeacon, nDestination,
						sendSocket) < 0)
				{
					putErrmsg("send-thread: Can't send \
beacon.", NULL);
					llcv_unlock(ctx->destinationsCV);
					continue;
				}
			}
			else
			{
#if IPND_DEBUG
				isprintf(buffer, sizeof buffer, "[i] send-\
thread: Beacon has not changed and node has an active connection with %s:%d. \
Not sending beacon.", nDestination->addr.ip, nDestination->addr.port);
				printText(buffer);
#endif
			}
		}

		/* Re-add destination to destinations list */
		nDestination->nextAnnounceTimestamp
				+= nDestination->announcePeriod;
		/* destinations lyst is sorted again. lyst_sort uses a stable
		   insertion sort that is very fast when the elements are
		   already in order. */
		lyst_sort(ctx->destinations);

		llcv_unlock(ctx->destinationsCV);
	}
}

/**
 * Join address on socket to all configured multicast addresses.
 * @param  listenAddresses  Lyst of configured listen addresses.
 * @param  socket           Socket to join mc groups.
 * @param  address          Address to join mc groups.
 * @return                  0 on success, -1 on error.
 */
static int	joinMulticastGroups(Lyst listenAddresses, const int socket,
			const char *address)
{
	int		i;
	LystElt		listenAddrElt;
	NetAddress	*listenAddr;
	struct ip_mreq	mcReq;

	CHKZERO(listenAddresses);

	listenAddrElt = lyst_first(listenAddresses);
	for (i = 0; i < lyst_length(listenAddresses); i++)
	{
		listenAddr = (NetAddress *) lyst_data(listenAddrElt);
		if (getIpv4AddressType(listenAddr->ip) == MULTICAST)
		{
			memset(&mcReq, 0, sizeof(mcReq));
			mcReq.imr_multiaddr.s_addr = inet_addr(listenAddr->ip);
			mcReq.imr_interface.s_addr = inet_addr(address);
			if ((setsockopt(socket, IPPROTO_IP, IP_ADD_MEMBERSHIP,
					(void *) &mcReq, sizeof(mcReq))) < 0)
			{
				putSysErrmsg("Error joining multicast group.",
						NULL);
				return -1;
			}
		}

		listenAddrElt = lyst_next(listenAddrElt);
	}

	return 0;
}

/**
 * Sets up listen sockets.
 * @param  listenAddresses           Lyst of Netaddress to bind.
 * @param  enabledBroadcastReceiving Enable broadcast receving on
 *                                   these addresses.
 * @param  numListenSockets          Returns the number of created sockets.
 * @return                           Array of created sockets.
 */
static int	*setUpListenSockets(Lyst listenAddresses,
			int enabledBroadcastReceiving, int *numListenSockets)
{
	int			receiveBroadcastSockOpt;
	int			i;
	int			numListenAddrs;
	int			listenSocket;
	int			*listenSockets;
	LystElt			listenAddrElt;
	NetAddress		*listenAddr;
	struct sockaddr_in	listenAddrStruct;

	/* Set up unicast listen sockets */
	numListenAddrs = lyst_length(listenAddresses);
	if (numListenAddrs == 0)
	{
		*numListenSockets = 0;
		return NULL;
	}

	listenSockets = MTAKE(numListenAddrs * sizeof(int));
	listenAddrElt = lyst_first(listenAddresses);
	for (i = 0; i < lyst_length(listenAddresses); i++)
	{
		/* Reset variables */
		listenAddr = (NetAddress *) lyst_data(listenAddrElt);
		memset(&listenAddrStruct, 0, sizeof(listenAddrStruct));
		listenSocket = -1;

		/* Create socket */
		listenSocket = socket(AF_INET, SOCK_DGRAM, 0);
		if (listenSocket < 0)
		{
			putSysErrmsg("receive-thread: Error creating receiving \
socket.", NULL);
			listenAddrElt = lyst_next(listenAddrElt);
			continue;
		}

		/* Bind addr to socket */
		listenAddrStruct.sin_family = AF_INET;
		listenAddrStruct.sin_port = htons(listenAddr->port);
		listenAddrStruct.sin_addr.s_addr =  inet_addr(listenAddr->ip);
		if ((bind(listenSocket, (struct sockaddr *) &listenAddrStruct,
				sizeof(listenAddrStruct))) < 0)
		{
			putSysErrmsg("receive-thread: Error binding.", NULL);
			closesocket(listenSocket);
			listenAddrElt = lyst_next(listenAddrElt);
			continue;
		}

		oK(reUseAddress(listenSocket));
		if (getIpv4AddressType(listenAddr->ip) == UNICAST)
		{
			/* Join multicast groups */
			if (joinMulticastGroups(listenAddresses,
					listenSocket, listenAddr->ip) < 0)
			{
				putSysErrmsg("receive-thread: Eror joining \
multicast groups", NULL);
			}

			/* Allow reception of multicast packets */
			if (enabledBroadcastReceiving)
			{
				receiveBroadcastSockOpt = 1;
				if ((setsockopt(listenSocket,
					SOL_SOCKET,
					SO_BROADCAST,
					(void *) &receiveBroadcastSockOpt,
					sizeof(receiveBroadcastSockOpt))) < 0)
				{
					putSysErrmsg("receive-thread: Error \
setting reception of broadcast beacons.", NULL);
				}
			}
		}

		listenSockets[i] = listenSocket;
		listenAddrElt = lyst_next(listenAddrElt);
	}

	*numListenSockets = i;
	return listenSockets;
}
/**
 * Call bp_discover_contact_acquired or bp_discover_contact_lost based on first parameter
 * @param  acquired Whether to call bp_discover_contact_acquired.
 * @param  ctx IPND context.
 * @param  eid EID of CLA.
 */
static void	bp_discover_contact(char acquired, IPNDCtx *ctx, char *eid)
{
	/* find CLA-TCP-v4 (64) service, as only that is supported */
	char			claProtocol[] = "tcp";
	char			socketSpec[40];
	char			buffer[256];
	LystElt			cur, next, curN, nextN;
	ServiceDefinition	*def;
	IpndNeighbor		*nb;
	/* WiFi raw rate + overhead is 6 Mbps, so default to 4 Mbps for now. */
	int			xmitRate = 4000000, recvRate = 4000000;

	lockResource(&ctx->neighborsLock);
	if (lyst_length(ctx->neighbors) <= 0)
	{
		unlockResource(&ctx->neighborsLock);
		return;
	}

	for (curN = lyst_first(ctx->neighbors); curN != NULL; curN = nextN)
	{
		nextN = lyst_next(curN);
		nb = (IpndNeighbor *) lyst_data(curN);
		if (strcmp(eid, nb->beacon.canonicalEid) != 0)
		{
			continue;
		}

		socketSpec[0] = '\0';
		if (lyst_length(nb->beacon.services) > 0)
		{
			for (cur = lyst_first(nb->beacon.services);
					cur != NULL; cur = next)
			{
				next = lyst_next(cur);
				def = (ServiceDefinition*) lyst_data(cur);
				if (def->number == 64)
				{
					if (def->data[2] == 4) /* IP is here */
					{
						isprintf(socketSpec,
							sizeof socketSpec,
							"%d.%d.%d.%d:%d",
							(unsigned char)
								def->data[3],
							(unsigned char)
								def->data[4],
							(unsigned char)
								def->data[5],
							(unsigned char)
								def->data[6],
							((unsigned char)
							 	def->data[8])
								* 256 +
							(unsigned char)
								def->data[9]);
					}
					else /* or swapped with port */
					{
						isprintf(socketSpec,
							sizeof socketSpec,
							"%d.%d.%d.%d:%d",
							(unsigned char)
								def->data[6],
							(unsigned char)
								def->data[7],
							(unsigned char)
								def->data[8],
							(unsigned char)
								def->data[9],
							((unsigned char)
							 	def->data[3])
								* 256 +
							(unsigned char)
								def->data[4]);
					}

					break;
				}
			}
		}

		if (socketSpec[0])
		{
			if (acquired)
			{
				isprintf(buffer, sizeof buffer,
					"[i] receive-thread: \
bp_discover_contact_acquired(%s, %s, %s, %d, %d)", socketSpec, eid, claProtocol,
					xmitRate, recvRate);
				bp_discovery_acquired(socketSpec, eid,
					claProtocol, xmitRate, recvRate);
			}
			else
			{
				isprintf(buffer, sizeof buffer,
					"[i] receive-thread: \
bp_discover_contact_lost(%s, %s, %s)", socketSpec, eid, claProtocol);
				bp_discovery_lost(socketSpec, eid,
					claProtocol);
			}

			printText(buffer);
		}

		break;
	}

	unlockResource(&ctx->neighborsLock);
}

/**
 * Receive beacons thread.
 * @param  attr Thread parameters.
 * @return      NULL on success, -1 on error. We return a casted -1 on error
 *              because this function is intended to be run as a thread and
 *              pthread_create expects a function with signature
 *              void *(*start_routine) */
void	*receiveBeacons(void *attr)
{
	char			buffer[1024];
	IPNDCtx			*ctx = (IPNDCtx *)attr;
	int			i, j;
	static int		numListenSockets;
	static int		*listenSockets;
	int			recevingSocket;
	fd_set			activeListenSocketsSet;
	fd_set			readListenSocketsSet;
	struct sockaddr_in	srcAddr;
	int			srcAddrLen;
	char			srcAddrStr[INET_ADDRSTRLEN];
	int			srcAddrType;
	int			srcAddrPort;
	time_t			timeOfReception;
#if MAX_BEACON_SIZE > 1024
	unsigned char		*recvDataBuffer;
#else
	unsigned char		recvDataBuffer[MAX_BEACON_SIZE];
#endif
	int			recvDataBufferLen;
	Beacon			recvBeacon;
	IpndNeighbor		*nb;
	LystElt			nbElt;
	int			newNb;
	Destination		*newDest;

	CHKNULL(ctx);

	setIPNDCtx(ctx);

	printText("[i] receive-thread: Receive beacons thread started.");

	lockResource(&ctx->configurationLock);
	listenSockets = setUpListenSockets(ctx->listenAddresses,
			ctx->enabledBroadcastReceiving, &numListenSockets);
	unlockResource(&ctx->configurationLock);
	ctx->listenSockets = listenSockets;
	ctx->numListenSockets = numListenSockets;

	if (numListenSockets == 0 || listenSockets == NULL)
	{
		writeMemo("[?] IPND receive-thread: No listen sockets \
configured. IPND will not receive any beacon.");
		return (void *) -1;
	}

	/* Prepare set of active sockets */
	FD_ZERO(&activeListenSocketsSet);
	for (i = 0; i < numListenSockets; i++)
	{
		if (listenSockets[i] > 0)
		{
			FD_SET(listenSockets[i], &activeListenSocketsSet);
		}
	}

	MRELEASE(listenSockets);

#if	MAX_BEACON_SIZE > 1024
	recvDataBuffer = MTAKE(MAX_BEACON_SIZE);
	CHKNULL(recvDataBuffer);

	/* Make sure memory is released when the thread is canceled. */
	pthread_cleanup_push(releaseToIonMemory, recvDataBuffer);
#endif
	memset((char *) &recvBeacon, 0, sizeof recvBeacon);
	while (1)
	{
		/* Reset variables */
		readListenSocketsSet = activeListenSocketsSet;
		nb = NULL;
		newNb = 0;

		if (select(FD_SETSIZE, &readListenSocketsSet, NULL, NULL, NULL)
				< 0)
		{
			putSysErrmsg("receive-thread: Error waiting for data.",
					NULL);
			continue;
		}

		for (i = 0; i < FD_SETSIZE; ++i)
		{
			if (!FD_ISSET(i, &readListenSocketsSet))
				continue;

			recevingSocket = i;
			srcAddrLen = sizeof(srcAddr);
			if ((recvDataBufferLen = irecvfrom(recevingSocket,
					(char *) recvDataBuffer,
					MAX_BEACON_SIZE, 0,
					(struct sockaddr *) &srcAddr,
					(socklen_t *) &srcAddrLen)) < 0)
			{
				putSysErrmsg("receive-thread: Error receiving \
data.", NULL);
				continue;
			}

			timeOfReception = time(NULL);
			istrcpy(srcAddrStr, inet_ntoa(srcAddr.sin_addr),
					INET_ADDRSTRLEN);
			/* We don't consider sender port. */
			lockResource(&ctx->configurationLock);
			srcAddrPort = ctx->port;
			srcAddrType = getIpv4AddressType(srcAddrStr);
			unlockResource(&ctx->configurationLock);

#if IPND_DEBUG
			isprintf(buffer, sizeof buffer,
					"[i] receive-thread: "
					"Beacon (%dB) received from %s",
					recvDataBufferLen, srcAddrStr);
			printText(buffer);
#endif

			if (*srcAddrStr == '\0')
			{
				putErrmsg("receive-thread: Error converting \
source address to string.", NULL);
				continue;
			}

			/* Check if we have received data from one of
			 * our unicast addresses */
			lockResource(&ctx->configurationLock);
			if (findAddr(srcAddrStr, ctx->listenAddresses) != NULL)
			{
				unlockResource(&ctx->configurationLock);
				continue;
			}

			unlockResource(&ctx->configurationLock);

			/* Check if received data is from a known neighbor */
			lockResource(&ctx->neighborsLock);
			nbElt = findIpndNeighbor(srcAddrStr, srcAddrPort,
					ctx->neighbors);
			unlockResource(&ctx->neighborsLock);

			if (nbElt != NULL)
			{
				/* Known neighbor. */
				nb = (IpndNeighbor *) lyst_data(nbElt);

#if IPND_DEBUG
				isprintf(buffer, sizeof buffer, "[i] \
receive-thread: Sender %s:%d is a known neighbor.", srcAddrStr, srcAddrPort);
				printText(buffer);
#endif
			}
			else
			{
#if IPND_DEBUG
				isprintf(buffer, sizeof buffer, "[i] \
receive-thread: Sender %s:%d is an unknown neighbor.", srcAddrStr, srcAddrPort);
				printText(buffer);
#endif
			}

			/* Process recv data */
			if (*recvDataBuffer == IPND_VERSION2)
			{
#if IPND_DEBUG
				printText("[i] receive-thread: Received \
IPND version 2 beacon.");
#endif
				continue;
			}
			else
			{
				if (*recvDataBuffer == IPND_VERSION4
				&& (recvDataBuffer[1] & 0xf0) == 0)
				{
					/* Deserialize beacon */
					j = deserializeBeacon(recvDataBuffer,
							recvDataBufferLen,
							&recvBeacon);
					if (j == -1)
					{
						isprintf(buffer, sizeof buffer,
							"[i] receive-thread: \
Received beacon's mandatory parts cannot be parsed.", NULL);
						printText(buffer);
						continue;
					}

					if (j == -2)
					{
						isprintf(buffer, sizeof buffer,
							"[i] receive-thread: \
Received beacon's optional part cannot be parsed.", NULL);
						printText(buffer);
					}

#if IPND_DEBUG
					isprintf(buffer, sizeof buffer,
						"[i] receive-thread: Received \
beacon (%dB) contents: ", recvDataBufferLen);
					printText(buffer);
#endif
					logBeacon(&recvBeacon);

					if (recvBeacon.period == 0)
					{
						isprintf(buffer, sizeof buffer,
							"[i] receive-thread: \
Received beacon interval is undefined.", recvDataBufferLen);
						printText(buffer);
					}

					/* If it is a new neighbor
					 * add it to neighbors list*/
					if (nb == NULL)
					{
						/* update NBF */
						lockResource
						(&ctx->configurationLock);
						updateCtxNbf
						(recvBeacon.canonicalEid,
						strlen
						(recvBeacon.canonicalEid));
						unlockResource
						(&ctx->configurationLock);
						lockResource
							(&ctx->neighborsLock);
						nb = (IpndNeighbor *)
							MTAKE(sizeof
							(IpndNeighbor));
						istrcpy(nb->addr.ip, srcAddrStr,
							INET_ADDRSTRLEN);
						nb->addr.port = srcAddrPort;
						lyst_insert(ctx->neighbors,
								(void *) nb);
						unlockResource
							(&ctx->neighborsLock);
						isprintf(buffer, sizeof buffer,
							"[i] receive-thread: \
Sender %s:%d added as new neighbor.", srcAddrStr, srcAddrPort);
						printText(buffer);
						newNb = 1;
					}
					else
					{
#if IPND_DEBUG
						printText("[i] receive-thread: \
Old beacon contents: ");
#endif
						logBeacon(&nb->beacon);
					}

					/* create bidirectional link if
					 * our eid is present */
					if (recvBeacon.bloom.ready)
					{
						switch (bloom_check
							(&recvBeacon.bloom,
							ctx->srcEid,
							strlen(ctx->srcEid)))
						{
						case 0:
							break;
						case 1:
							nb->link.bidirectional
								= 1;
#if IPND_DEBUG
							isprintf(buffer,
								sizeof buffer,
								"[i] receive-\
thread: Sender's %s:%d NBF contains our EID - bidirectional.",
								srcAddrStr,
								srcAddrPort);
							printText(buffer);
#endif
							break;
						default:
							putErrmsg("Can't check \
for EID in Bloom filter", ctx->srcEid);
						}
					}

					/* Update neighbor beacon */
					copyBeacon(&nb->beacon, &recvBeacon);
				}
				else
				{
					putErrmsg("[i] receive-thread: \
Received unknown beacon.", NULL);
					continue;
				}
			}

			/* ["2.1. Beacon Period", paragraph 3, pages 5 - 6] */
			nb->beaconReceptionTime = timeOfReception;

			srcAddrType = getIpv4AddressType(nb->addr.ip);
			lockResource(&ctx->configurationLock);
			if (time(NULL) < nb->beaconReceptionTime
					+ ctx->announcePeriods[srcAddrType])
			{
				nb->link.state = UP;
			}

			unlockResource(&ctx->configurationLock);

			/* Add destination (if it wasn't already in
			 * destinatons lyst) */
			llcv_lock(ctx->destinationsCV);
			nbElt = findDestinationByAddr(&nb->addr,
					ctx->destinations);
			if (nbElt == NULL)
			{
				newDest = (Destination *)
						MTAKE(sizeof(Destination));
				if (newDest == NULL)
				{
					putErrmsg("receive-thread: Error \
allocating memory for new destination.", NULL);
					llcv_unlock(ctx->destinationsCV);
					continue;
				}

				memset(newDest, 0, sizeof(*newDest));
				newDest->addr = nb->addr;

				lockResource(&ctx->configurationLock);
				newDest->announcePeriod = 
					ctx->announcePeriods[srcAddrType];
				unlockResource(&ctx->configurationLock);

				newDest->nextAnnounceTimestamp = time(NULL);
				lyst_insert(ctx->destinations,
						(void *) newDest);

				copyBeacon(&newDest->beacon, &nb->beacon);

				isprintf(buffer, sizeof buffer, "[i] receive-\
thread: Sender %s:%d added as new destination.", srcAddrStr, srcAddrPort);
				printText(buffer);
			}
			else
			{
				newDest = (Destination*)lyst_data(nbElt);
			}

			/* update eid of destination */
			istrcpy(newDest->eid, nb->beacon.canonicalEid,
					MAX_EID_LEN);

			/* Wake up sending thread to notify that a new
			 * destination has been added. */
			llcv_unlock(ctx->destinationsCV);
			llcv_signal(ctx->destinationsCV, llcv_lyst_not_empty);

			/* Announce the presence of this newly discovered
			 * neighbor.  ["2.7. IPND and CLAs", paragraph 2,
			 * page 16] */
			if (nb->link.state == UP && newNb)
			{
				bp_discover_contact(1, ctx,
					nb->beacon.canonicalEid);
				isprintf(buffer, sizeof buffer, "[i] receive-\
thread: CLA notified that link %s:%d is up.", srcAddrStr, srcAddrPort);
				printText(buffer);
			}

			/* Notify dependent CLAs that this neighbor's
			 * link has gone down.  ["2.7. IPND and CLAs",
			 * paragraph 3, page 16] */
			else
			{
				if (nb->link.state == DOWN)
				{
					bp_discover_contact(0, ctx,
						nb->beacon.canonicalEid);
					isprintf(buffer, sizeof buffer, "[i] \
receive-thread: CLA notified that link %s:%d is down.", srcAddrStr,
						srcAddrPort);
					printText(buffer);
				}
			}
		}
	}

#if	MAX_BEACON_SIZE > 1024
	pthread_cleanup_pop(1);
#endif

	return NULL;
}

/**
 * Expire neighbors thread.
 * @param  attr Thread parameters.
 * @return      NULL on success, -1 on error. We return a casted -1 on error
 *              because this function is intended to be run as a thread and
 *              pthread_create expects a function with signature
 *              void *(*start_routine) */
void	*expireNeighbors(void *attr)
{
	char		buffer[120];
	IPNDCtx		*ctx = (IPNDCtx *)attr;
	int		i;
	LystElt		nbOld, nbElt, destinationElt;
	Destination	*dest;
	IpndNeighbor	*nb;

	CHKNULL(ctx);

	setIPNDCtx(ctx);

	printText("[i] expire-thread: Expire neighbors thread started.");

	/* Purge the neighbors set of node whose beacon intervals have \
	 * expired.  See ["2.8. Disconnection", paragraph 1, page 16] */
	while (1)
	{
		snooze(MAX(MAX(ctx->announcePeriods[UNICAST],
				ctx->announcePeriods[MULTICAST]),
				ctx->announcePeriods[BROADCAST]));

		lockResource(&ctx->neighborsLock);
		nbElt = lyst_first(ctx->neighbors);
		for (i = 0; i < lyst_length(ctx->neighbors); i++)
		{
			nb = (IpndNeighbor *) lyst_data(nbElt);
			if (time(NULL) < nb->beaconReceptionTime
					+ nb->beacon.period * 2)
			{
				continue; /* Neighbor still not expired */
			}

			isprintf(buffer, sizeof buffer, "[i] expire-thread: \
Beacon not recv in %d seconds. Neighbor %s:%d has disappeared",
				nb->beacon.period, nb->addr.ip, nb->addr.port);
			printText(buffer);
			nb->link.state = DOWN;

			/* Notify dependent CLAs that this neighbor's
			 * link has gone down.  ["2.7. IPND and CLAs",
			 * paragraph 3, page 16] */
			bp_discover_contact(0, ctx, nb->beacon.canonicalEid);
			isprintf(buffer, sizeof buffer,
					"[i] expire-thread: "
					"CLA notified that link %s:%d is down.",
					nb->addr.ip, nb->addr.port);
			printText(buffer);

			/* Remove from destinations if it has not been
			 * added from the configuration file. */
			llcv_lock(ctx->destinationsCV);
			destinationElt = findDestinationByAddr(&nb->addr,
					ctx->destinations);
			if (destinationElt != NULL)
			{
				dest = lyst_data(destinationElt);
				if (!dest->fixed)
				{
					isprintf(buffer, sizeof buffer, "[i] \
expire-thread: Destination %s:%d removed", dest->addr.ip, dest->addr.port);
					printText(buffer);
					clearBeacon(&dest->beacon);
					lyst_delete(destinationElt);
				}
			}

			llcv_unlock(ctx->destinationsCV);

			nbOld = nbElt;
			nbElt = lyst_next(nbElt);
			lyst_delete(nbOld);
		}

		unlockResource(&ctx->neighborsLock);
	}

	return NULL;
}
