/*
	udpts.c:	functions implementing UDP transport service
			for AMS.

	Author: Scott Burleigh, JPL

	Copyright (c) 2006, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
#include "amsP.h"

#define	UDPTS_MAX_MSG_LEN	65535

typedef struct
{
	unsigned int	ipAddress;
	unsigned short	portNbr;
} UdpTsep;

static int	udpComputeCsepName(char *endpointSpec, char *endpointName)
{
	unsigned short	portNbr;
	unsigned int	ipAddress;
	char		hostName[MAXHOSTNAMELEN + 1];

	CHKERR(endpointName);
	parseSocketSpec(endpointSpec, &portNbr, &ipAddress);
	if (portNbr == 0)
	{
		portNbr = 2357;		/*	Default.		*/
	}

	if (ipAddress == 0)		/*	Default to local host.	*/
	{
		getNameOfHost(hostName, sizeof hostName);
		ipAddress = getInternetAddress(hostName);
	}
	else
	{
		if (getInternetHostName(ipAddress, hostName) == NULL)
		{
			putErrmsg("Unknown host in endpoint.", endpointSpec);
			return -1;
		}
	}

	isprintf(endpointName, MAX_EP_NAME + 1, "%s:%hu", hostName, portNbr);
	return 0;
}

static int	udpMamsInit(MamsInterface *tsif)
{
	unsigned short		portNbr;
	unsigned int		ipAddress;
	char			hostName[MAXHOSTNAMELEN + 1];
	struct sockaddr		socketName;
	struct sockaddr_in	*inetName;
	socklen_t		nameLength;
	int			fd;
	char			endpointNameText[32];
	int			eptLen;
	saddr			longfd;

	CHKERR(tsif);
	parseSocketSpec(tsif->endpointSpec, &portNbr, &ipAddress);
	if (ipAddress == 0)
	{
		if ((ipAddress = getAddressOfHost()) == 0)
		{
			putErrmsg("udpts can't get own IP address.", NULL);
			return -1;
		}
	}

#if AMSDEBUG
printf("parsed endpoint spec to port %hu address %u.\n", portNbr, ipAddress);
#endif
	if (getInternetHostName(ipAddress, hostName) == NULL)
	{
		putErrmsg("Unknown host in endpoint.", tsif->endpointSpec);
		return -1;
	}

	portNbr = htons(portNbr);
	ipAddress = htonl(ipAddress);
	memset((char *) &socketName, 0, sizeof socketName);
	inetName = (struct sockaddr_in *) &socketName;
	inetName->sin_family = AF_INET;
	inetName->sin_port = portNbr;
	memcpy((char *) &(inetName->sin_addr.s_addr), (char *) &ipAddress, 4);
	fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (fd < 0)
	{
		putSysErrmsg("udpts can't open MAMS SAP", NULL);
		return -1;
	}

	nameLength = sizeof(struct sockaddr);
	if (reUseAddress(fd)
	|| bind(fd, &socketName, nameLength) < 0
	|| getsockname(fd, &socketName, &nameLength) < 0)
	{
		putSysErrmsg("udpts can't initialize AMS SAP", NULL);
		closesocket(fd);
		return -1;
	}

	portNbr = inetName->sin_port;
	portNbr = ntohs(portNbr);
	memcpy((char *) &ipAddress, (char *) &(inetName->sin_addr.s_addr), 4);
	ipAddress = ntohl(ipAddress);
	isprintf(endpointNameText, sizeof endpointNameText, "%u:%hu", ipAddress,
			portNbr);
#if AMSDEBUG
printf("resulting ept is '%s'.\n", endpointNameText);
#endif
	eptLen = strlen(endpointNameText) + 1;
	tsif->ept = MTAKE(eptLen);
	if (tsif->ept == NULL)
	{
		putErrmsg("Can't record endpoint name.", NULL);
		closesocket(fd);
		return -1;
	}

	istrcpy(tsif->ept, endpointNameText, eptLen);
	longfd = fd;
	tsif->sap = (void *) longfd;
	return 0;
}

static void	*udpMamsReceiver(void *parm)
{
	MamsInterface		*tsif = (MamsInterface *) parm;
	int			fd;
	char			*buffer;
	int			length;
	struct sockaddr_in	fromAddr;
	socklen_t		fromSize;

	CHKNULL(tsif);
	fd = (saddr) (tsif->sap);
	buffer = MTAKE(UDPTS_MAX_MSG_LEN);
	CHKNULL(buffer);
#ifndef mingw
	sigset_t		signals;

	sigfillset(&signals);
	pthread_sigmask(SIG_BLOCK, &signals, NULL);
#endif
	while (1)
	{
		fromSize = sizeof fromAddr;
		length = irecvfrom(fd, buffer, UDPTS_MAX_MSG_LEN, 0,
				(struct sockaddr *) &fromAddr, &fromSize);
		if (length < 2)	/*	length == 1 is "shutdown"	*/
		{
			if (length < 0)
			{
				if (errno == EINTR)
				{
					continue;
				}

				putSysErrmsg("udpts failed receiving MAMS \
message", NULL);
			}

			closesocket(fd);
			MRELEASE(buffer);
			tsif->sap = NULL;
			return NULL;
		}

		/*	Got a MAMS message.				*/

		if (enqueueMamsMsg(tsif->eventsQueue, length,
				(unsigned char *) buffer) < 0)
		{
			writeMemo("[?] udpts discarded MAMS message.");
		}
	}
}

static int	udpAmsInit(AmsInterface *tsif, char *epspec)
{
	unsigned short		portNbr;
	unsigned int		ipAddress;
	char			hostName[MAXHOSTNAMELEN + 1];
	struct sockaddr		socketName;
	struct sockaddr_in	*inetName;
	socklen_t		nameLength;
	int			fd;
	char			endpointNameText[32];
	int			eptLen;
	saddr			longfd;

	CHKERR(tsif);
	CHKERR(epspec);
	if (strcmp(epspec, "@") == 0)	/*	Default.		*/
	{
		epspec = NULL;	/*	Force default selection.	*/
	}

	parseSocketSpec(epspec, &portNbr, &ipAddress);
	if (ipAddress == 0)
	{
		getNameOfHost(hostName, sizeof hostName);
		if ((ipAddress = getInternetAddress(hostName)) == 0)
		{
			putErrmsg("udpts can't get own IP address.", NULL);
			return -1;
		}
	}
	else
	{
		if (getInternetHostName(ipAddress, hostName) == NULL)
		{
			putErrmsg("Unknown host in endpoint.", epspec);
			return -1;
		}
	}

	portNbr = htons(portNbr);
	ipAddress = htonl(ipAddress);
	memset((char *) &socketName, 0, sizeof socketName);
	inetName = (struct sockaddr_in *) &socketName;
	inetName->sin_family = AF_INET;
	inetName->sin_port = portNbr;
	memcpy((char *) &(inetName->sin_addr.s_addr), (char *) &ipAddress, 4);
	fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (fd < 0)
	{
		putSysErrmsg("udpts can't open AMS SAP", NULL);
		return -1;
	}

	nameLength = sizeof(struct sockaddr);
	if (reUseAddress(fd)
	|| bind(fd, &socketName, nameLength) < 0
	|| getsockname(fd, &socketName, &nameLength) < 0)
	{
		putSysErrmsg("udpts can't initialize AMS SAP", NULL);
		closesocket(fd);
		return -1;
	}

	portNbr = inetName->sin_port;
	portNbr = ntohs(portNbr);
	memcpy((char *) &ipAddress, (char *) &(inetName->sin_addr.s_addr), 4);
	ipAddress = ntohl(ipAddress);
	tsif->diligence = AmsBestEffort;
	tsif->sequence = AmsArrivalOrder;
	isprintf(endpointNameText, sizeof endpointNameText, "%u:%hu", ipAddress,
			portNbr);
	eptLen = strlen(endpointNameText) + 1;
	tsif->ept = MTAKE(eptLen);
	if (tsif->ept == NULL)
	{
		putErrmsg("Can't record endpoint name.", NULL);
		closesocket(fd);
		return -1;
	}

	istrcpy(tsif->ept, endpointNameText, eptLen);
	longfd = fd;
	tsif->sap = (void *) longfd;
	return 0;
}

static void	*udpAmsReceiver(void *parm)
{
	AmsInterface		*tsif = (AmsInterface *) parm;
	int			fd;
	AmsSAP			*amsSap;
	char			*buffer;
	int			length;
	struct sockaddr_in	fromAddr;
	socklen_t		fromSize;

	CHKNULL(tsif);
	fd = (saddr) (tsif->sap);
	amsSap = tsif->amsSap;
	buffer = MTAKE(UDPTS_MAX_MSG_LEN);
	CHKNULL(buffer);
#ifndef mingw
	sigset_t		signals;

	sigfillset(&signals);
	pthread_sigmask(SIG_BLOCK, &signals, NULL);
#endif
	while (1)
	{
		fromSize = sizeof fromAddr;
		length = irecvfrom(fd, buffer, UDPTS_MAX_MSG_LEN, 0,
				(struct sockaddr *) &fromAddr, &fromSize);
		if (length < 2)	/*	length == 1 is "shutdown"	*/
		{
			if (length < 0)
			{
				if (errno == EINTR)
				{
					continue;
				}

				putSysErrmsg("udpts failed receiving AMS \
message", NULL);
			}

			closesocket(fd);
			MRELEASE(buffer);
			tsif->sap = NULL;
			return NULL;
		}

		/*	Got an AMS message.				*/

		if (enqueueAmsMsg(amsSap, (unsigned char *) buffer, length) < 0)
		{
			writeMemo("[?] udpts discarded AMS message.");
		}
	}
}

static int	udpParseMamsEndpoint(MamsEndpoint *ep)
{
	char	*colon;
	char	hostName[MAXHOSTNAMELEN + 1];
	UdpTsep	tsep;

	CHKERR(ep);
	CHKERR(ep->ept);
	colon = strchr(ep->ept, ':');
	CHKERR(colon);
	*colon = '\0';
	istrcpy(hostName, ep->ept, sizeof hostName);
	*colon = ':';
	tsep.portNbr = atoi(colon + 1);
	tsep.ipAddress = getInternetAddress(hostName);
	ep->tsep = MTAKE(sizeof(UdpTsep));
	CHKERR(ep->tsep);
	memcpy((char *) (ep->tsep), (char *) &tsep, sizeof(UdpTsep));
#if AMSDEBUG
printf("parsed '%s' to port %hu address %u.\n", ep->ept, tsep.portNbr,
tsep.ipAddress);
#endif
	return 0;
}

static void	udpClearMamsEndpoint(MamsEndpoint *ep)
{
	CHKVOID(ep);
	if (ep->tsep)
	{
		MRELEASE(ep->tsep);
	}
}

static int	udpParseAmsEndpoint(AmsEndpoint *dp)
{
	char	*colon;
	char	hostName[MAXHOSTNAMELEN + 1];
	UdpTsep	tsep;

	CHKERR(dp);
	CHKERR(dp->ept);
	colon = strchr(dp->ept, ':');
	CHKERR(colon);
	*colon = '\0';
	istrcpy(hostName, dp->ept, sizeof hostName);
	*colon = ':';
	tsep.portNbr = atoi(colon + 1);
	tsep.ipAddress = getInternetAddress(hostName);
	dp->tsep = MTAKE(sizeof(UdpTsep));
	CHKERR(dp->tsep);
	memcpy((char *) (dp->tsep), (char *) &tsep, sizeof(UdpTsep));

	/*	Also parse out the QOS of this endpoint.		*/

	dp->diligence = AmsBestEffort;
	dp->sequence = AmsArrivalOrder;
	return 0;
}

static void	udpClearAmsEndpoint(AmsEndpoint *dp)
{
	CHKVOID(dp);
	if (dp->tsep)
	{
		MRELEASE(dp->tsep);
	}
}

static int	udpSendMams(MamsEndpoint *ep, MamsInterface *tsif, char *msg,
			int msgLen)
{
	UdpTsep			*tsep;
	unsigned short		portNbr;
	unsigned int		hostNbr;
	struct sockaddr		socketName;
	struct sockaddr_in	*inetName;
	int			fd;

	CHKERR(ep);
	CHKERR(tsif);
	CHKERR(msg);
	CHKERR(msgLen >= 0);
	tsep = (UdpTsep *) (ep->tsep);
#if AMSDEBUG
printf("in udpSendMams, tsep at %lu has port %hu, address %u.\n",
(unsigned long) tsep, tsep->portNbr, tsep->ipAddress);
#endif
	if (tsep == NULL)	/*	Lost connectivity to endpoint.	*/
	{
		return 0;
	}

	portNbr = htons(tsep->portNbr);
	hostNbr = htonl(tsep->ipAddress);
	memset((char *) &socketName, 0, sizeof socketName);
	inetName = (struct sockaddr_in *) &socketName;
	inetName->sin_family = AF_INET;
	inetName->sin_port = portNbr;
	memcpy((char *) &(inetName->sin_addr.s_addr), (char *) &hostNbr, 4);
	fd = (saddr) (tsif->sap);
	while (1)
	{
		if (sendto(fd, msg, msgLen, 0, &socketName,
				sizeof(struct sockaddr)) < 0)
		{
			if (errno == EINTR)	/*	Interrupted.	*/
			{
				continue;	/*	Retry.		*/
			}
#if AMSDEBUG
PUTS("udpSendMams failed.");
#endif
			return -1;
		}
#if AMSDEBUG
PUTS("udpSendMams succeeded.");
#endif
		return 0;
	}
}

static int	udpSendAms(AmsEndpoint *dp, AmsSAP *sap,
			unsigned char flowLabel, char *header,
			int headerLen, char *content, int contentLen)
{
	char			*udpAmsBuf;
	int			len;
	UdpTsep			*tsep;
	int			i;
	AmsInterface		*tsif;
	int			fd;
	unsigned short		portNbr;
	unsigned int		hostNbr;
	struct sockaddr		socketName;
	struct sockaddr_in	*inetName;
	unsigned short		checksum;

	CHKERR(dp);
	CHKERR(sap);
	CHKERR(header);
	CHKERR(headerLen >= 0);
	CHKERR(contentLen == 0 || (contentLen > 0 && content != NULL));
	len = headerLen + contentLen + 2;
	CHKERR(len <= UDPTS_MAX_MSG_LEN);
	tsep = (UdpTsep *) (dp->tsep);
#if AMSDEBUG
printf("in udpSendAms, tsep is %lu.\n", (unsigned long) tsep);
#endif
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

	fd = (saddr) (tsif->sap); 
	portNbr = htons(tsep->portNbr);
	hostNbr = htonl(tsep->ipAddress);
	memset((char *) &socketName, 0, sizeof socketName);
	inetName = (struct sockaddr_in *) &socketName;
	inetName->sin_family = AF_INET;
	inetName->sin_port = portNbr;
	memcpy((char *) &(inetName->sin_addr.s_addr), (char *) &hostNbr, 4);
	udpAmsBuf = MTAKE(headerLen + contentLen + 2);
	CHKERR(udpAmsBuf);
	memcpy(udpAmsBuf, header, headerLen);
	if (contentLen > 0)
	{
		memcpy(udpAmsBuf + headerLen, content, contentLen);
	}

	checksum = computeAmsChecksum((unsigned char *) udpAmsBuf,
			headerLen + contentLen);
	checksum = htons(checksum);
	memcpy(udpAmsBuf + headerLen + contentLen, (char *) &checksum, 2);
	while (1)
	{
		if (sendto(fd, udpAmsBuf, len, 0, &socketName,
				sizeof(struct sockaddr)) < 0)
		{
			if (errno == EINTR)	/*	Interrupted.	*/
			{
				continue;	/*	Retry.		*/
			}

#if AMSDEBUG
PUTS("udpSendAms failed.");
#endif
			MRELEASE(udpAmsBuf);
			return -1;
		}

#if AMSDEBUG
PUTS("udpSendAms succeeded.");
#endif
		MRELEASE(udpAmsBuf);
		return 0;
	}
}

static void	udpShutdown(void *abstract_sap)
{
	int		fd = (saddr) abstract_sap;
	struct sockaddr	sockName;
	socklen_t	sockNameLen = sizeof sockName;
	char		shutdown = 1;

	if (getsockname(fd, &sockName, &sockNameLen) == 0)
	{
		oK(sendto(fd, &shutdown, 1, 0, &sockName,
			sizeof(struct sockaddr_in)));
	}
}

void	udptsLoadTs(TransSvc *ts)
{
	CHKVOID(ts);
	ts->name = "udp";
	ts->csepNameFn = udpComputeCsepName;
	ts->mamsInitFn = udpMamsInit;
	ts->mamsReceiverFn = udpMamsReceiver;
	ts->parseMamsEndpointFn = udpParseMamsEndpoint;
	ts->clearMamsEndpointFn = udpClearMamsEndpoint;
	ts->sendMamsFn = udpSendMams;
	ts->amsInitFn = udpAmsInit;
	ts->amsReceiverFn = udpAmsReceiver;
	ts->parseAmsEndpointFn = udpParseAmsEndpoint;
	ts->clearAmsEndpointFn = udpClearAmsEndpoint;
	ts->sendAmsFn = udpSendAms;
	ts->shutdownFn = udpShutdown;
}
