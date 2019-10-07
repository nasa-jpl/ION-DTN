/*
	tcpts.c:	functions implementing TCP transport service
			for AMS.

	Author: Scott Burleigh, JPL

	Copyright (c) 2005, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
#include "amsP.h"

#define	TCPTS_MAX_MSG_LEN	65535

typedef struct tcptsep
{
	int		fd;
	unsigned int	ipAddress;
	unsigned short	portNbr;
	struct tcptsep	*prev;		/*	In senders pool.	*/
	struct tcptsep	*next;		/*	In senders pool.	*/
	struct tcptssap	*sap;		/*	Back pointer.		*/
} TcpTsep;

typedef struct tcprcvr
{
	int		fd;
	pthread_t	thread;
	AmsSAP		*amsSap;
	struct tcprcvr	*prev;		/*	In receivers pool.	*/
	struct tcprcvr	*next;		/*	In receivers pool.	*/
	struct tcptssap	*sap;		/*	Back pointer.		*/
} TcpRcvr;

typedef struct tcptssap
{
	TcpTsep			*firstInSendPool;
	TcpTsep			*lastInSendPool;
	int			nbrInSendPool;
	pthread_mutex_t		sendPoolMutex;

	TcpRcvr			*firstInRcvrPool;
	TcpRcvr			*lastInRcvrPool;
	int			nbrInRcvrPool;
	pthread_mutex_t		rcvrPoolMutex;

	struct sockaddr		addrbuf;
	struct sockaddr_in	*nm;
	int			accessSocket;
	int			stopped;
} TcptsSap;

static void	removeReceiver(TcpRcvr *rcvr)
{
	TcptsSap	*sap = rcvr->sap;

	pthread_mutex_lock(&sap->rcvrPoolMutex);
	if (rcvr->fd != -1)
	{
		closesocket(rcvr->fd);
	}

	if (rcvr->next)
	{
		rcvr->next->prev = rcvr->prev;
	}
	else
	{
		sap->lastInRcvrPool = rcvr->prev;
	}

	if (rcvr->prev)
	{
		rcvr->prev->next = rcvr->next;
	}
	else
	{
		sap->firstInRcvrPool = rcvr->next;
	}

	sap->nbrInRcvrPool--;
	pthread_mutex_unlock(&sap->rcvrPoolMutex);
	MRELEASE(rcvr);
}

static int	receiveMsgByTCP(int *fd, char *buffer)
{
	unsigned short	preamble;
	char		*into;
	int		bytesToReceive;
	int		bytesReceived;
	unsigned short	msglen;

	/*	Receive length of transmitted message.			*/

	into = (char *) &preamble;
	bytesToReceive = sizeof preamble;
	while (bytesToReceive > 0)
	{
		bytesReceived = itcp_recv(fd, into, bytesToReceive);
		if (bytesReceived < 1)
		{
			return bytesReceived;
		}

		into += bytesReceived;
		bytesToReceive -= bytesReceived;
	}

	/*	Receive the message itself.				*/

	msglen = ntohs(preamble);
	into = buffer;
	bytesToReceive = msglen;
	while (bytesToReceive > 0)
	{
		bytesReceived = itcp_recv(fd, into, bytesToReceive);
		if (bytesReceived < 1)
		{
			return bytesReceived;
		}

		into += bytesReceived;
		bytesToReceive -= bytesReceived;
	}

	return msglen;
}

static void	removeSender(TcpTsep *tsep)
{
	TcptsSap	*sap = tsep->sap;

	pthread_mutex_lock(&sap->sendPoolMutex);
	closesocket(tsep->fd);
	tsep->fd = -1;
	if (tsep->next)
	{
		tsep->next->prev = tsep->prev;
	}
	else
	{
		sap->lastInSendPool = tsep->prev;
	}

	if (tsep->prev)
	{
		tsep->prev->next = tsep->next;
	}
	else
	{
		sap->firstInSendPool = tsep->next;
	}

	sap->nbrInSendPool--;
	pthread_mutex_unlock(&sap->sendPoolMutex);
}

/*	*	*	*	MAMS stuff	*	*	*	*/

/*	TCP could be used as a primary transport service, now that we
 *	include MAMS endpoint names in all messages to which replies
 *	are required.  But this hasn't been implemented yet.		*/

static int	tcpComputeCsepName(char *endpointSpec, char *endpointName)
{
	putErrmsg("Sorry, no PTS support implemented in tcpts.", NULL);
	return -1;
}

int	tcpMamsInit(MamsInterface *tsif)
{
	putErrmsg("Sorry, no PTS support implemented in tcpts.", NULL);
	return -1;
}

static void	*tcpMamsAccess(void *parm)
{
	putErrmsg("Sorry, no PTS support implemented in tcpts.", NULL);
	return NULL;
}

static int	tcpParseMamsEndpoint(MamsEndpoint *ep)
{
	putErrmsg("Sorry, no PTS support implemented in tcpts.", NULL);
	return -1;
}

static void	tcpClearMamsEndpoint(MamsEndpoint *ep)
{
	putErrmsg("Sorry, no PTS support implemented in tcpts.", NULL);
}

static int	tcpSendMams(MamsEndpoint *ep, MamsInterface *tsif, char *msg,
			int msgLen)
{
	putErrmsg("Sorry, no PTS support implemented in tcpts.", NULL);
	return -1;
}

/*	*	*	*	AMS stuff	*	*	*	*/

int	tcpAmsInit(AmsInterface *tsif, char *epspec)
{
	unsigned short		portNbr;
	unsigned int		ipAddress;
	TcptsSap		*sap;
	socklen_t		buflen;
	char			endpointNameText[32];
	int			eptLen;

	CHKERR(tsif);
	CHKERR(epspec);
	if (strcmp(epspec, "@") == 0)	/*	Default.		*/
	{
		epspec = NULL;	/*	Force default selection.	*/
	}

	parseSocketSpec(epspec, &portNbr, &ipAddress);
	if (ipAddress == 0)
	{
		if ((ipAddress = getAddressOfHost()) == 0)
		{
			putErrmsg("tcpcs can't get own IP address.", NULL);
			return -1;
		}
	}

	sap = (TcptsSap *) MTAKE(sizeof(TcptsSap));
	CHKERR(sap);
	memset((char *) sap, 0, sizeof(TcptsSap));
	pthread_mutex_init(&sap->sendPoolMutex, NULL);
	pthread_mutex_init(&sap->rcvrPoolMutex, NULL);
	memset((char *) &(sap->addrbuf), 0, sizeof(struct sockaddr));
	sap->nm = (struct sockaddr_in *) &(sap->addrbuf);
	sap->nm->sin_family = AF_INET;
	sap->nm->sin_port = htons(portNbr);
	sap->nm->sin_addr.s_addr = htonl(ipAddress);
	sap->accessSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sap->accessSocket < 0)
	{
		putSysErrmsg("tcpts can't open AMS access socket", NULL);
		MRELEASE(sap);
		return -1;
	}

	buflen = sizeof(struct sockaddr);
	if (reUseAddress(sap->accessSocket)
	|| bind(sap->accessSocket, &(sap->addrbuf), buflen) < 0
	|| listen(sap->accessSocket, 5) < 0
	|| getsockname(sap->accessSocket, &(sap->addrbuf), &buflen) < 0)
	{
		putSysErrmsg("tcpts can't configure AMS access socket", NULL);
		closesocket(sap->accessSocket);
		MRELEASE(sap);
		return -1;
	}

	ipAddress = sap->nm->sin_addr.s_addr;
	ipAddress = ntohl(ipAddress);
	portNbr = sap->nm->sin_port;
	portNbr = ntohs(portNbr);
	tsif->diligence = AmsAssured;
	tsif->sequence = AmsTransmissionOrder;
	isprintf(endpointNameText, sizeof endpointNameText, "%u:%hu", ipAddress,
			portNbr);
	eptLen = strlen(endpointNameText) + 1;
	tsif->ept = MTAKE(eptLen);
	if (tsif->ept == NULL)
	{
		putErrmsg("tcpts can't record endpoint name.", NULL);
		closesocket(sap->accessSocket);
		MRELEASE(sap);
		return -1;
	}

	istrcpy(tsif->ept, endpointNameText, eptLen);
	tsif->sap = (void *) sap;
	return 0;
}

static void	*tcpAmsReceiver(void *parm)
{
	TcpRcvr		*me = (TcpRcvr *) parm;
	TcptsSap	*sap;
	char		*buffer;
	int		length;

	CHKNULL(me);
	sap = me->sap;
	buffer = MTAKE(TCPTS_MAX_MSG_LEN);
	CHKNULL(buffer);
#ifndef mingw
	sigset_t	signals;

	sigfillset(&signals);
	pthread_sigmask(SIG_BLOCK, &signals, NULL);
#endif
	while (1)
	{
		length = receiveMsgByTCP(&(me->fd), buffer);
		switch (length)
		{
		case -1:
			writeMemo("[?] tcpts receiver crashed.");

			/*	Intentional fall-through to next case.	*/

		case 0:	/*	Connection simply closed by sender.	*/
			MRELEASE(buffer);
			removeReceiver(me);
			return NULL;

		default:
			break;
		}

		/*	Got an AMS message.				*/

		if (enqueueAmsMsg(me->amsSap, (unsigned char *) buffer, length)
				< 0)
		{
			writeMemo("[?] tcpts discarded AMS message.");
		}

		/*	Promote self to top of AMS receiver pool.	*/

		pthread_mutex_lock(&sap->rcvrPoolMutex);
		if (sap->firstInRcvrPool != me)
		{
			if (me->prev)
			{
				me->prev->next = me->next;
			}

			if (me->next)
			{
				me->next->prev = me->prev;
			}
			else
			{
				sap->lastInRcvrPool = me->prev;
			}

			me->next = sap->firstInRcvrPool;
			me->prev = NULL;
			sap->firstInRcvrPool->prev = me;
			sap->firstInRcvrPool = me;
		}

		pthread_mutex_unlock(&sap->rcvrPoolMutex);
	}
}

static void	*tcpAmsAccess(void *parm)
{
	AmsInterface		*tsif = (AmsInterface *) parm;
	TcptsSap		*sap;
	int			childSocket;
	struct sockaddr		clientSockname;
	socklen_t		len;
	TcpRcvr			*rcvr;

	CHKNULL(tsif);
	sap = (TcptsSap *) (tsif->sap);
	CHKNULL(sap);
#ifndef mingw
	sigset_t		signals;

	sigfillset(&signals);
	pthread_sigmask(SIG_BLOCK, &signals, NULL);
#endif
	while (1)
	{
		len = sizeof clientSockname;
		childSocket = accept(sap->accessSocket, &clientSockname, &len);
		if (childSocket < 0)
		{
			putSysErrmsg("tcpts failed on accept()", NULL);
			break;
		}

		if (sap->stopped)
		{
			closesocket(childSocket);
			break;
		}

		rcvr = MTAKE(sizeof(TcpRcvr));
		if (rcvr == NULL)
		{
			putErrmsg("tcpts out of memory.", NULL);
			closesocket(childSocket);
			break;
		}

		memset((char *) rcvr, 0, sizeof(TcpRcvr));
		rcvr->fd = childSocket;
		rcvr->amsSap = tsif->amsSap;
		rcvr->prev = NULL;
		rcvr->sap = sap;

		/*	Manage pool of 32 most active inbound sockets.	*/

		pthread_mutex_lock(&sap->rcvrPoolMutex);
		if (sap->nbrInRcvrPool < 32)
		{
			pthread_mutex_unlock(&sap->rcvrPoolMutex);
		}
		else
		{
			closesocket(sap->lastInRcvrPool->fd);
			sap->lastInRcvrPool->fd = -1;
			pthread_mutex_unlock(&sap->rcvrPoolMutex);
			pthread_join(sap->lastInRcvrPool->thread, NULL);
		}

		pthread_mutex_lock(&sap->rcvrPoolMutex);
		rcvr->next = sap->firstInRcvrPool;
		if (sap->firstInRcvrPool == NULL)
		{
			sap->lastInRcvrPool = rcvr;
		}
		else
		{
			sap->firstInRcvrPool->prev = rcvr;
		}

		sap->firstInRcvrPool = rcvr;
		sap->nbrInRcvrPool++;
		pthread_mutex_unlock(&sap->rcvrPoolMutex);

		/*	Animate the new receiver.			*/

		if (pthread_begin(&(rcvr->thread), NULL, tcpAmsReceiver,
			rcvr, "tcpts_receiver") < 0)
		{
			putSysErrmsg("tcpts can't start Mams receiver thread",
					NULL);
			removeReceiver(rcvr);
			break;
		}
	}

	/*	Shutting down this Tcpts service access point.		*/

	while (sap->firstInSendPool)
	{
		removeSender(sap->firstInSendPool);
	}

	pthread_mutex_lock(&sap->rcvrPoolMutex);
	while (sap->firstInRcvrPool)
	{
		closesocket(sap->firstInRcvrPool->fd);
		sap->firstInRcvrPool->fd = -1;
		pthread_mutex_unlock(&sap->rcvrPoolMutex);
		pthread_join(sap->firstInRcvrPool->thread, NULL);
		pthread_mutex_lock(&sap->rcvrPoolMutex);
	}

	pthread_mutex_unlock(&sap->rcvrPoolMutex);
	if (sap->accessSocket != -1)
	{
		closesocket(sap->accessSocket);
	}

	MRELEASE(sap);
	tsif->sap = NULL;
	return NULL;
}

static int	tcpParseAmsEndpoint(AmsEndpoint *dp)
{
	TcpTsep	tsep;

	CHKERR(dp);
	CHKERR(dp->ept);
	memset((char *) &tsep, 0, sizeof(TcpTsep));
	tsep.fd = -1;		/*	Not connected yet.		*/
	if (sscanf(dp->ept, "%u:%hu", &tsep.ipAddress, &tsep.portNbr) != 2)
	{
		putErrmsg("tcpts found AMS endpoint name invalid.", dp->ept);
		return -1;
	}

	/*	Note: fill in tsep.sap when the tsep is first used
	 *	for transmission.					*/

	dp->tsep = MTAKE(sizeof(TcpTsep));
	CHKERR(dp->tsep);
	memcpy((char *) (dp->tsep), (char *) &tsep, sizeof(TcpTsep));

	/*	Also parse out the QOS of this endpoint.		*/

	dp->diligence = AmsAssured;
	dp->sequence = AmsTransmissionOrder;
	return 0;
}

static void	tcpClearAmsEndpoint(AmsEndpoint *dp)
{
	TcpTsep	*tsep;

	CHKVOID(dp);
	tsep = dp->tsep;
	CHKVOID(tsep);
	if (tsep->fd != -1)
	{
		removeSender(tsep);
	}

	MRELEASE(tsep);
}

static int	tcpSendAms(AmsEndpoint *dp, AmsSAP *sap,
			unsigned char flowLabel, char *header, int headerLen,
			char *content, int contentLen)
{
	char			*tcpAmsBuf;
	unsigned int		xmitlen;
	TcpTsep			*tsep;
	int			i;
	AmsInterface		*tsif;
	TcptsSap		*tcpSap;
	struct sockaddr		buf;
	struct sockaddr_in	*nm = (struct sockaddr_in *) &buf;
	unsigned short		checksum;
	unsigned short		preamble;
	int			result;

	CHKERR(dp);
	CHKERR(sap);
	CHKERR(header);
	CHKERR(headerLen >= 0);
	CHKERR(contentLen == 0 || (contentLen > 0 && content != NULL));
	xmitlen = headerLen + contentLen + 2;
	CHKERR(xmitlen <= TCPTS_MAX_MSG_LEN);
	tsep = (TcpTsep *) (dp->tsep);
	if (tsep == NULL)	/*	Lost connectivity to endpoint.	*/
	{
		return 0;
	}

	for (i = 0, tsif = sap->amsTsifs; i < sap->transportServiceCount; i++,
			tsif++)
	{
		if (tsif->ts == dp->ts)
		{
			break;	/*	Have found interface to use.	*/
		}
	}

	if (i == sap->transportServiceCount)	/*	No match.	*/
	{
		return 0;	/*	Cannot send msg to endpoint.	*/
	}

	tcpSap = (TcptsSap *) (tsif->sap);
	if (tsep->fd < 0)	/*	Must open socket connection.	*/
	{
		memset((char *) &buf, 0, sizeof buf);
		nm->sin_family = AF_INET;
		nm->sin_port = htons(tsep->portNbr);
		nm->sin_addr.s_addr = htonl(tsep->ipAddress);
		tsep->fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (tsep->fd < 0)
		{
			putSysErrmsg("tcpts can't open TCP socket", NULL);
			return -1;
		}

		if (connect(tsep->fd, &buf, sizeof(struct sockaddr)) < 0)
		{
			closesocket(tsep->fd);
			tsep->fd = -1;
			putSysErrmsg("tcpts can't connect to TCP socket", NULL);
			return -1;
		}

		/*	Must be inserted into pool of 32 most
		 *	active outbound sockets.		*/

		pthread_mutex_lock(&tcpSap->sendPoolMutex);
		if (tcpSap->nbrInSendPool < 32)
		{
			pthread_mutex_unlock(&tcpSap->sendPoolMutex);
		}
		else	/*	Must first ditch least active.	*/
		{
			pthread_mutex_unlock(&tcpSap->sendPoolMutex);
			removeSender(tcpSap->lastInSendPool);
		}

		pthread_mutex_lock(&tcpSap->sendPoolMutex);
		tsep->next = tcpSap->firstInSendPool;
		if (tcpSap->firstInSendPool == NULL)
		{
			tcpSap->lastInSendPool = tsep;
		}
		else
		{
			tcpSap->firstInSendPool->prev = tsep;
		}

		tcpSap->firstInSendPool = tsep;
		tcpSap->nbrInSendPool++;
		pthread_mutex_unlock(&tcpSap->sendPoolMutex);
		tsep->sap = tcpSap;
	}

	preamble = xmitlen;
	preamble = htons(preamble);
	result = itcp_send(&tsep->fd, (char *) &preamble, sizeof preamble);
	if (result < 1)		/*	Data not transmitted.		*/
	{
		removeSender(tsep);
		return result;
	}

	tcpAmsBuf = MTAKE(headerLen + contentLen + 2);
	CHKERR(tcpAmsBuf);
	memcpy(tcpAmsBuf, header, headerLen);
	if (contentLen > 0)
	{
		memcpy(tcpAmsBuf + headerLen, content, contentLen);
	}

	checksum = computeAmsChecksum((unsigned char *) tcpAmsBuf,
			headerLen + contentLen);
	checksum = htons(checksum);
	memcpy(tcpAmsBuf + headerLen + contentLen, (char *) &checksum, 2);
	result = itcp_send(&tsep->fd, tcpAmsBuf, xmitlen);
	MRELEASE(tcpAmsBuf);
	if (result < 1)		/*	Data not transmitted.		*/
	{
		removeSender(tsep);
		return result;
	}

	/*	Succeeded; promote self to top of AMS sender pool.	*/

	pthread_mutex_lock(&tcpSap->sendPoolMutex);
	if (tcpSap->firstInSendPool != tsep)
	{
		if (tsep->prev == NULL)
		{
			/*	Apparently this can happen; don't
			 *	know how.  A race condition, somewhere,
			 *	that is not yet tracked down.		*/

			putErrmsg("tcpts sender pool corrupted; continuing.",
					NULL);
		}
		else
		{
			tsep->prev->next = tsep->next;
			if (tsep->next)
			{
				tsep->next->prev = tsep->prev;
			}
			else
			{
				tcpSap->lastInSendPool = tsep->prev;
			}

			tsep->next = tcpSap->firstInSendPool;
			tsep->prev = NULL;
			tcpSap->firstInSendPool->prev = tsep;
			tcpSap->firstInSendPool = tsep;
		}
	}

	pthread_mutex_unlock(&tcpSap->sendPoolMutex);
	return 0;
}

static void	tcpShutdown(void *abstract_sap)
{
	TcptsSap	*tcpSap = (TcptsSap *) (abstract_sap);
	int		fd;

	CHKVOID(tcpSap);
	tcpSap->stopped = 1;

	/*	Wake up our own access thread by connecting to it.	*/

	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (fd >= 0)
	{
		oK(connect(fd, &(tcpSap->addrbuf), sizeof(struct sockaddr)));

		/*	Immediately discard the connected socket.	*/

		closesocket(fd);
	}
}

void	tcptsLoadTs(TransSvc *ts)
{
	CHKVOID(ts);
	ts->name = "tcp";
	ts->csepNameFn = tcpComputeCsepName;
	ts->mamsInitFn = tcpMamsInit;
	ts->mamsReceiverFn = tcpMamsAccess;
	ts->parseMamsEndpointFn = tcpParseMamsEndpoint;
	ts->clearMamsEndpointFn = tcpClearMamsEndpoint;
	ts->sendMamsFn = tcpSendMams;
	ts->amsInitFn = tcpAmsInit;
	ts->amsReceiverFn = tcpAmsAccess;
	ts->parseAmsEndpointFn = tcpParseAmsEndpoint;
	ts->clearAmsEndpointFn = tcpClearAmsEndpoint;
	ts->sendAmsFn = tcpSendAms;
	ts->shutdownFn = tcpShutdown;
#ifndef mingw
	isignal(SIGPIPE, SIG_IGN);
#endif
}
