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

	CHKERR(endpointName);
	parseSocketSpec(endpointSpec, &portNbr, &ipAddress);
	if (portNbr == 0)
	{
		portNbr = 2357;		/*	Default.		*/
	}

	if (ipAddress == 0)		/*	Default to local host.	*/
	{
		ipAddress = getAddressOfHost();
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

	CHKERR(tsif);
	parseSocketSpec(tsif->endpointSpec, &portNbr, &ipAddress);
	if (ipAddress == 0)
	{
		if ((ipAddress = getAddressOfHost()) == 0)
		{
			putErrmsg("dgrts can't get own IP address.", NULL);
			return -1;
		}
	}

#if AMSDEBUG
printf("parsed endpoint spec to port %hu address %u.\n", portNbr, ipAddress);
#endif
	if (dgr_open(sm_TaskIdSelf(), DGRTS_CLIENT_SVC_ID, portNbr, ipAddress,
			memmgr_name(getIonMemoryMgr()), &dgrSap, &rc) < 0)
	{
		putErrmsg("dgrts can't open MAMS SAP.", NULL);
		return -1;
	}

	dgr_getsockname(dgrSap, &portNbr, &ipAddress);
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
		dgr_close(dgrSap);
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
	unsigned int	ipAddress;
	unsigned short	portNbr;
	int		length;
	int		errnbr;
	DgrRC		rc;

	CHKNULL(tsif);
	dgrSap = (Dgr) (tsif->sap);
	CHKNULL(dgrSap);
	buffer = MTAKE(DGRTS_MAX_MSG_LEN);
	CHKNULL(buffer);
#ifndef mingw
	sigset_t	signals;

	sigfillset(&signals);
	pthread_sigmask(SIG_BLOCK, &signals, NULL);
#endif
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
			writeMemo("[?] dgrts discarded MAMS message.");
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

	CHKERR(tsif);
	CHKERR(epspec);
	tsif->diligence = AmsAssured;
	tsif->sequence = AmsArrivalOrder;
	if (strcmp(epspec, "@") == 0)	/*	Default.		*/
	{
		epspec = NULL;	/*	Force default selection.	*/
	}

	parseSocketSpec(epspec, &portNbr, &ipAddress);
	if (ipAddress == 0)
	{
		if ((ipAddress = getAddressOfHost()) == 0)
		{
			putErrmsg("dgrts can't get own IP address.", NULL);
			return -1;
		}
	}

	if (dgr_open(sm_TaskIdSelf(), DGRTS_CLIENT_SVC_ID, portNbr, ipAddress,
			memmgr_name(getIonMemoryMgr()), &dgrSap, &rc) < 0)
	{
		putErrmsg("dgrts can't open AMS SAP.", NULL);
		return -1;
	}

	dgr_getsockname(dgrSap, &portNbr, &ipAddress);
	isprintf(endpointNameText, sizeof endpointNameText, "%u:%hu", ipAddress,
			portNbr);
	eptLen = strlen(endpointNameText) + 1;
	tsif->ept = MTAKE(eptLen);
	if (tsif->ept == NULL)
	{
		putErrmsg("Can't record endpoint name.", NULL);
		dgr_close(dgrSap);
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
	unsigned short	portNbr;
	unsigned int	ipAddress;
	int		length;
	int		errnbr;
	DgrRC		rc;

	CHKNULL(tsif);
	dgrSap = (Dgr) (tsif->sap);
	CHKNULL(dgrSap);
	amsSap = tsif->amsSap;
	CHKNULL(amsSap);
	buffer = MTAKE(DGRTS_MAX_MSG_LEN);
	CHKNULL(buffer);
#ifndef mingw
	sigset_t	signals;

	sigfillset(&signals);
	pthread_sigmask(SIG_BLOCK, &signals, NULL);
#endif
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
			writeMemo("[?] dgrts discarded AMS message.");
		}
	}
}

static int	dgrParseMamsEndpoint(MamsEndpoint *ep)
{
	DgrTsep	tsep;

	CHKERR(ep);
	CHKERR(ep->ept);
	if (sscanf(ep->ept, "%u:%hu", &tsep.ipAddress, &tsep.portNbr) != 2)
	{
		putErrmsg("dgrts found MAMS endpoint name invalid.", ep->ept);
		return -1;
	}

	ep->tsep = MTAKE(sizeof(DgrTsep));
	CHKERR(ep->tsep);
	memcpy((char *) (ep->tsep), (char *) &tsep, sizeof(DgrTsep));
#if AMSDEBUG
printf("parsed '%s' to port %hu address %u.\n", ep->ept, tsep.portNbr,
tsep.ipAddress);
#endif
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

	CHKERR(dp);
	CHKERR(dp->ept);
	if (sscanf(dp->ept, "%u:%hu", &tsep.ipAddress, &tsep.portNbr) != 2)
	{
		putErrmsg("dgrts found AMS endpoint name invalid.", dp->ept);
		return -1;
	}

	dp->tsep = MTAKE(sizeof(DgrTsep));
	CHKERR(dp->tsep);
	memcpy((char *) (dp->tsep), (char *) &tsep, sizeof(DgrTsep));

	/*	Also parse out the QOS of this endpoint.		*/

	dp->diligence = AmsAssured;
	dp->sequence = AmsArrivalOrder;
	return 0;
}

static void	dgrClearAmsEndpoint(AmsEndpoint *dp)
{
	CHKVOID(dp);
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

	CHKERR(ep);
	CHKERR(tsif);
	CHKERR(msg);
	CHKERR(msgLen >= 0);
	tsep = (DgrTsep *) (ep->tsep);
#if AMSDEBUG
printf("in dgrSendMams, tsep at %lu has port %hu, address %u.\n",
(unsigned long) tsep, tsep->portNbr, tsep->ipAddress);
#endif
	if (tsep == NULL)	/*	Lost connectivity to endpoint.	*/
	{
		return 0;
	}

	dgrSap = (Dgr) (tsif->sap);
	if (dgr_send(dgrSap, tsep->portNbr, tsep->ipAddress, 0, msg,
			msgLen, &rc) < 0)
	{
#if AMSDEBUG
PUTS("dgrSendMams failed.");
#endif
		return -1;
	}

#if AMSDEBUG
PUTS("dgrSendMams succeeded.");
#endif
	return 0;
}

static int	dgrSendAms(AmsEndpoint *dp, AmsSAP *sap,
			unsigned char flowLabel, char *header,
			int headerLen, char *content, int contentLen)
{
	char		*dgrAmsBuf;
	int		len;
	DgrTsep		*tsep;
	int		i;
	AmsInterface	*tsif;
	Dgr		dgrSap;
	unsigned short	checksum;
	DgrRC		rc;
	int		result;

	CHKERR(dp);
	CHKERR(sap);
	CHKERR(header);
	CHKERR(headerLen >= 0);
	CHKERR(contentLen == 0 || (contentLen > 0 && content != NULL));
	len = headerLen + contentLen + 2;
	CHKERR(len <= DGRTS_MAX_MSG_LEN);
	tsep = (DgrTsep *) (dp->tsep);
#if AMSDEBUG
printf("in dgrSendAms, tsep is %lu.\n", (unsigned long) tsep);
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

	dgrSap = (Dgr) (tsif->sap);
	dgrAmsBuf = MTAKE(headerLen + contentLen + 2);
	CHKERR(dgrAmsBuf);
	memcpy(dgrAmsBuf, header, headerLen);
	if (contentLen > 0)
	{
		memcpy(dgrAmsBuf + headerLen, content, contentLen);
	}

	checksum = computeAmsChecksum((unsigned char *) dgrAmsBuf,
			headerLen + contentLen);
	checksum = htons(checksum);
	memcpy(dgrAmsBuf + headerLen + contentLen, (char *) &checksum, 2);
	result = dgr_send(dgrSap, tsep->portNbr, tsep->ipAddress, 0, dgrAmsBuf,
			len, &rc);
	MRELEASE(dgrAmsBuf);
       	if (result < 0)
	{
#if AMSDEBUG
PUTS("dgrSendAms failed.");
#endif
		return -1;
	}

#if AMSDEBUG
PUTS("dgrSendAms succeeded.");
#endif
	return 0;
}

static void	dgrShutdown(void *sap)
{
	Dgr	dgrSap = (Dgr) sap;

	dgr_close(dgrSap);
}

void	dgrtsLoadTs(TransSvc *ts)
{
	CHKVOID(ts);
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
