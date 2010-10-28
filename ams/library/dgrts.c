/*
	dgrts.c:	functions implementing DGR transport service
			for AMS.

	Author: Scott Burleigh, JPL

	Copyright (c) 2005, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
#include "amsP.h"
#include "dgr.h"

#define	DGRTS_MAX_MSG_LEN	65535
#define	DGRTS_CLIENT_SVC_ID	2

typedef struct
{
	unsigned int	ipAddress;
	unsigned short	portNbr;
} DgrTsep;

static void	handleNotice(DgrRC rc, Dgr dgrSap, unsigned short portNbr,
			unsigned int ipAddress, char *buffer, int length,
			int errnbr)
{
	return;		/*	Maybe do something with these later.	*/
}

static int	dgrComputeCsepName(char *endpointSpec, char *endpointName)
{
	unsigned short	portNbr;
	unsigned int	ipAddress;
	char		ownHostName[MAXHOSTNAMELEN + 1];

	if (endpointName == NULL)
	{
		putErrmsg(BadParmsMemo, NULL);
		return -1;
	}

	parseSocketSpec(endpointSpec, &portNbr, &ipAddress);
	if (portNbr == 0)
	{
		portNbr = 2357;		/*	Default.		*/
	}

	if (ipAddress == 0)		/*	Default to local host.	*/
	{
		getNameOfHost(ownHostName, sizeof ownHostName);
		ipAddress = getInternetAddress(ownHostName);
	}

	isprintf(endpointName, MAX_EP_NAME + 1, "%u:%hu", ipAddress, portNbr);
	return 0;
}

static int	dgrMamsInit(MamsInterface *tsif)
{
	unsigned short	portNbr;
	unsigned int	ipAddress;
	Dgr		dgrSap;
	char		endpointNameText[32];
	int		eptLen;
	DgrRC		rc;

	parseSocketSpec(tsif->endpointSpec, &portNbr, &ipAddress);
//printf("parsed endpoint spec to port %d address %d.\n", portNbr, ipAddress);
	if (dgr_open(sm_TaskIdSelf(), DGRTS_CLIENT_SVC_ID, portNbr, ipAddress,
			memmgr_name(amsMemory), &dgrSap, &rc) < 0)
	{
		putSysErrmsg("dgrts can't open MAMS SAP", NULL);
		return -1;
	}

	dgr_getsockname(dgrSap, &portNbr, &ipAddress);
	isprintf(endpointNameText, sizeof endpointNameText, "%u:%hu", ipAddress,
			portNbr);
//printf("resulting ept is '%s'.\n", endpointNameText);
	eptLen = strlen(endpointNameText) + 1;
	tsif->ept = MTAKE(eptLen);
	if (tsif->ept == NULL)
	{
		dgr_close(dgrSap);
		putSysErrmsg(NoMemoryMemo, NULL);
		return -1;
	}

	istrcpy(tsif->ept, endpointNameText, eptLen);
	tsif->sap = dgrSap;
	return 0;
}

static void	*dgrMamsReceiver(void *parm)
{
	MamsInterface	*tsif = (MamsInterface *) parm;
	Dgr		dgrSap;
	char		*buffer;
	sigset_t	signals;
	unsigned int	ipAddress;
	unsigned short	portNbr;
	int		length;
	int		errnbr;
	DgrRC		rc;

	dgrSap = (Dgr) (tsif->sap);
	buffer = MTAKE(DGRTS_MAX_MSG_LEN);
	if (buffer == NULL)
	{
		putSysErrmsg(NoMemoryMemo, NULL);
		return NULL;
	}

	sigfillset(&signals);
	pthread_sigmask(SIG_BLOCK, &signals, NULL);
	while (1)
	{
		oK(dgr_receive(dgrSap, &portNbr, &ipAddress,
				buffer, &length, &errnbr, DGR_BLOCKING, &rc));
		switch (rc)
		{
		case DgrFailed:
			dgr_close(dgrSap);
			MRELEASE(buffer);
			tsif->sap = NULL;
			return NULL;

		case DgrTimedOut:
		case DgrInterrupted:
			continue;

		case DgrDatagramReceived:
			break;

		default:
			handleNotice(rc, dgrSap, portNbr, ipAddress, buffer,
					length, errnbr);
			continue;
		}

		/*	Got a MAMS message.				*/

		if (enqueueMamsMsg(tsif->eventsQueue, length,
				(unsigned char *) buffer) < 0)
		{
			putErrmsg("dgrts discarded MAMS message.", NULL);
		}
	}
}

static int	dgrAmsInit(AmsInterface *tsif, char *epspec)
{
	unsigned short	portNbr;
	unsigned int	ipAddress;
	Dgr		dgrSap;
	DgrRC		rc;
	char		endpointNameText[32];
	int		eptLen;

	if (strcmp(epspec, "@") == 0)	/*	Default.		*/
	{
		epspec = NULL;	/*	Force default selection.	*/
	}

	parseSocketSpec(epspec, &portNbr, &ipAddress);
	if (dgr_open(sm_TaskIdSelf(), DGRTS_CLIENT_SVC_ID, portNbr, ipAddress,
			memmgr_name(amsMemory), &dgrSap, &rc) < 0)
	{
		putSysErrmsg("dgrts can't open AMS SAP", NULL);
		return -1;
	}

	dgr_getsockname(dgrSap, &portNbr, &ipAddress);
	tsif->diligence = AmsAssured;
	tsif->sequence = AmsArrivalOrder;
	isprintf(endpointNameText, sizeof endpointNameText, "%u:%hu", ipAddress,
			portNbr);
	eptLen = strlen(endpointNameText) + 1;
	tsif->ept = MTAKE(eptLen);
	if (tsif->ept == NULL)
	{
		dgr_close(dgrSap);
		putSysErrmsg(NoMemoryMemo, NULL);
		return -1;
	}

	istrcpy(tsif->ept, endpointNameText, eptLen);
	tsif->sap = dgrSap;
	return 0;
}

static void	*dgrAmsReceiver(void *parm)
{
	AmsInterface	*tsif = (AmsInterface *) parm;
	Dgr		dgrSap;
	AmsSAP		*amsSap;
	char		*buffer;
	sigset_t	signals;
	unsigned short	portNbr;
	unsigned int	ipAddress;
	int		length;
	int		errnbr;
	DgrRC		rc;

	dgrSap = (Dgr) (tsif->sap);
	amsSap = tsif->amsSap;
	buffer = MTAKE(DGRTS_MAX_MSG_LEN);
	if (buffer == NULL)
	{
		putSysErrmsg(NoMemoryMemo, NULL);
		return NULL;
	}

	sigfillset(&signals);
	pthread_sigmask(SIG_BLOCK, &signals, NULL);
	while (1)
	{
		oK(dgr_receive(dgrSap, &portNbr, &ipAddress, buffer,
				&length, &errnbr, DGR_BLOCKING, &rc));
		switch (rc)
		{
		case DgrFailed:
			dgr_close(dgrSap);
			MRELEASE(buffer);
			tsif->sap = NULL;
			return NULL;

		case DgrTimedOut:
		case DgrInterrupted:
			continue;

		case DgrDatagramReceived:
			break;

		default:
			handleNotice(rc, dgrSap, portNbr, ipAddress, buffer,
					length, errnbr);
			continue;
		}

		/*	Got an AMS message.				*/

		if (enqueueAmsMsg(amsSap, (unsigned char *) buffer, length) < 0)
		{
			putErrmsg("dgrts discarded AMS message.", NULL);
		}
	}
}

static int	dgrParseMamsEndpoint(MamsEndpoint *ep)
{
	DgrTsep	tsep;

	if (ep == NULL || ep->ept == NULL)
	{
		errno = EINVAL;
		putErrmsg("dgrts can't parse MAMS endpoint name.", NULL);
		return -1;
	}

	if (sscanf(ep->ept, "%u:%hu", &tsep.ipAddress, &tsep.portNbr) != 2)
	{
		errno = EINVAL;
		putErrmsg("dgrts found MAMS endpoint name invalid.", ep->ept);
		return -1;
	}

	ep->tsep = MTAKE(sizeof(DgrTsep));
	if (ep->tsep == NULL)
	{
		putSysErrmsg("dgrts can't record parsed MAMS endpoint name.",
				NULL);
		return -1;
	}

	memcpy((char *) (ep->tsep), (char *) &tsep, sizeof(DgrTsep));
//printf("parsed '%s' to port %d address %d.\n", ep->ept, tsep.portNbr,
//tsep.ipAddress);
	return 0;
}

static void	dgrClearMamsEndpoint(MamsEndpoint *ep)
{
	if (ep->tsep)
	{
		MRELEASE(ep->tsep);
	}
}

static int	dgrParseAmsEndpoint(AmsEndpoint *dp)
{
	DgrTsep	tsep;

	if (dp == NULL || dp->ept == NULL)
	{
		errno = EINVAL;
		putErrmsg("dgrts can't parse AMS endpoint.", NULL);
		return -1;
	}

	if (sscanf(dp->ept, "%u:%hu", &tsep.ipAddress, &tsep.portNbr) != 2)
	{
		errno = EINVAL;
		putErrmsg("dgrts found AMS endpoint name invalid.", dp->ept);
		return -1;
	}

	dp->tsep = MTAKE(sizeof(DgrTsep));
	if (dp->tsep == NULL)
	{
		putSysErrmsg("dgrts can't record parsed AMS endpoint name.",
				NULL);
		return -1;
	}

	memcpy((char *) (dp->tsep), (char *) &tsep, sizeof(DgrTsep));

	/*	Also parse out the QOS of this endpoint.		*/

	dp->diligence = AmsAssured;
	dp->sequence = AmsArrivalOrder;
	return 0;
}

static void	dgrClearAmsEndpoint(AmsEndpoint *dp)
{
	if (dp->tsep)
	{
		MRELEASE(dp->tsep);
	}
}

static int	dgrSendMams(MamsEndpoint *ep, MamsInterface *tsif, char *msg,
			int msgLen)
{
	DgrTsep	*tsep;
	Dgr	dgrSap;
	DgrRC	rc;

	if (ep == NULL || tsif == NULL || msg == NULL || msgLen < 0)
	{
		errno = EINVAL;
		putErrmsg("Can't use DGR to send MAMS message.", NULL);
		return -1;
	}

	tsep = (DgrTsep *) (ep->tsep);
//printf("in dgrSendMams, tsep at %d has port %d, address %d.\n", (int) tsep,
//tsep->portNbr, tsep->ipAddress);
	if (tsep == NULL)	/*	Lost connectivity to endpoint.	*/
	{
		return 0;
	}

	dgrSap = (Dgr) (tsif->sap);
	if (dgr_send(dgrSap, tsep->portNbr, tsep->ipAddress, 0, msg,
			msgLen, &rc) < 0)
	{
//PUTS("dgrSendMams failed.");
		return -1;
	}

//PUTS("dgrSendMams succeeded.");
	return 0;
}

static int	dgrSendAms(AmsEndpoint *dp, AmsSAP *sap,
			unsigned char flowLabel, char *header,
			int headerLen, char *content, int contentLen)
{
	static char	dgrAmsBuf[DGRTS_MAX_MSG_LEN];
	int		len;
	DgrTsep		*tsep;
	int		i;
	AmsInterface	*tsif;
	Dgr		dgrSap;
	unsigned short	checksum;
	DgrRC		rc;

	if (dp == NULL || sap == NULL || header == NULL || headerLen < 0
	|| contentLen < 0 || (contentLen > 0 && content == NULL)
	|| (len = (headerLen + contentLen + 2)) > DGRTS_MAX_MSG_LEN)
	{
		errno = EINVAL;
		putErrmsg("Can't use DGR to send AMS message.", NULL);
		return -1;
	}

	tsep = (DgrTsep *) (dp->tsep);
//printf("in dgrSendAms, tsep is %d.\n", (int) tsep);
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

	dgrSap = (Dgr) (tsif->sap);
	memcpy(dgrAmsBuf, header, headerLen);
	if (contentLen > 0)
	{
		memcpy(dgrAmsBuf + headerLen, content, contentLen);
	}

	checksum = computeAmsChecksum((unsigned char *) dgrAmsBuf,
			headerLen + contentLen);
	checksum = htons(checksum);
	memcpy(dgrAmsBuf + headerLen + contentLen, (char *) &checksum, 2);
	if (dgr_send(dgrSap, tsep->portNbr, tsep->ipAddress, 0, dgrAmsBuf,
			len, &rc) < 0)
	{
//PUTS("dgrSendAms failed.");
		return -1;
	}

//PUTS("dgrSendAms succeeded.");
	return 0;
}

static void	dgrShutdown(void *sap)
{
	Dgr	dgrSap = (Dgr) sap;

	dgr_close(dgrSap);
}

void	dgrtsLoadTs(TransSvc *ts)
{
	ts->name = "dgr";
	ts->csepNameFn = dgrComputeCsepName;
	ts->mamsInitFn = dgrMamsInit;
	ts->mamsReceiverFn = dgrMamsReceiver;
	ts->parseMamsEndpointFn = dgrParseMamsEndpoint;
	ts->clearMamsEndpointFn = dgrClearMamsEndpoint;
	ts->sendMamsFn = dgrSendMams;
	ts->amsInitFn = dgrAmsInit;
	ts->amsReceiverFn = dgrAmsReceiver;
	ts->parseAmsEndpointFn = dgrParseAmsEndpoint;
	ts->clearAmsEndpointFn = dgrClearAmsEndpoint;
	ts->sendAmsFn = dgrSendAms;
	ts->shutdownFn = dgrShutdown;
}
