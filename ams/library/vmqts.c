/*
	vmqts.c:	functions implementing VMQ (vxWorks message
			queue) transport service for AMS.

	Author: Scott Burleigh, JPL

	Copyright (c) 2005, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
#if defined (VXWORKS)

#include "amsP.h"

#define	VMQTS_MAX_MSG_LEN	65535

typedef MSG_Q_ID		VmqTsep;

/*	*	*	*	MAMS stuff	*	*	*	*/

/*	VMQ is not suitable as a primary transport service: the
 *	endpoint ID for the configuration server can't be specified
 *	before the endpoint is created, so it can't be advertised in
 *	the MIB to registrars and modules.  There may be some clever
 *	way around this, using indirection and artificial message
 *	queue IDs, but the ability to use VMQ as the PTS doesn't
 *	seem to be worth the extra complexity.  We just don't do it.	*/

static int	vmqComputeCsepName(char *endpointSpec, char *endpointName)
{
	putErrmsg("Sorry, no PTS support implemented in vmqts.", NULL);
	return -1;
}

int	vmqMamsInit(MamsInterface *tsif)
{
	putErrmsg("Sorry, no PTS support implemented in vmqts.", NULL);
	return -1;
}

static void	*vmqMamsReceiver(void *parm)
{
	putErrmsg("Sorry, no PTS support implemented in vmqts.", NULL);
	return NULL;
}

static int	vmqParseMamsEndpoint(MamsEndpoint *ep)
{
	putErrmsg("Sorry, no PTS support implemented in vmqts.", NULL);
	return -1;
}

static void	vmqClearMamsEndpoint(MamsEndpoint *ep)
{
	putErrmsg("Sorry, no PTS support implemented in vmqts.", NULL);
}

static int	vmqSendMams(MamsEndpoint *ep, MamsInterface *tsif, char *msg,
			int msgLen)
{
	putErrmsg("Sorry, no PTS support implemented in vmqts.", NULL);
	return -1;
}

/*	*	*	*	AMS stuff	*	*	*	*/

static int	vmqAmsInit(AmsInterface *tsif, char *epspec)
{
	MSG_Q_ID	vmqSap;
	char		endpointNameText[32];
	int		eptLen;

	CHKERR(tsif);
	CHKERR(epspec);
#ifdef VXMP
	vmqSap = msgQSmCreate(1, VMQTS_MAX_MSG_LEN, MSG_Q_FIFO);
#else
	vmqSap = msgQCreate(1, VMQTS_MAX_MSG_LEN, MSG_Q_FIFO);
#endif
	if (vmqSap == NULL)
	{
		putSysErrmsg("vmqts can't open AMS SAP", NULL);
		return -1;
	}

	tsif->diligence = AmsAssured;
	tsif->sequence = AmsTransmissionOrder;
	isprintf(endpointNameText, sizeof endpointNameText, "%u", vmqSap);
	eptLen = strlen(endpointNameText) + 1;
	tsif->ept = MTAKE(eptLen);
	if (tsif->ept == NULL)
	{
		msgQDelete(vmqSap);
		putErrmsg("Can't record endpoint name.", NULL);
		return -1;
	}

	istrcpy(tsif->ept, endpointNameText, eptLen);
	tsif->sap = vmqSap;
	return 0;
}

static void	*vmqAmsReceiver(void *parm)
{
	AmsInterface	*tsif = (AmsInterface *) parm;
	MSG_Q_ID	vmqSap;
	AmsSAP		*amsSap;
	char		*buffer;
	sigset_t	signals;
	int		length;
	int		errnbr;

	CHKNULL(tsif);
	vmqSap = (MSG_Q_ID) (tsif->sap);
	CHKNULL(vmqSap);
	amsSap = tsif->amsSap;
	CHKNULL(amsSap);
	buffer = MTAKE(VMQTS_MAX_MSG_LEN);
	CHKNULL(buffer);
	sigfillset(&signals);
	pthread_sigmask(SIG_BLOCK, &signals, NULL);
	while (1)
	{
		length = msgQReceive(vmqSap, buffer, VMQTS_MAX_MSG_LEN,
				WAIT_FOREVER);
		if (length == ERROR)
		{
			msgQDelete(vmqSap);
			MRELEASE(buffer);
			tsif->sap = NULL;
			return NULL;
		}

		/*	Got an AMS message.				*/

		if (enqueueAmsMsg(amsSap, buffer, length) < 0)
		{
			writeMemo("[?] vmqts discarded AMS message.");
		}
	}
}

static int	vmqParseAmsEndpoint(AmsEndpoint *dp)
{
	VmqTsep	tsep;

	CHKERR(dp);
	CHKERR(dp->ept);
	if (sscanf(dp->ept, "%u", &tsep) != 1)
	{
		putErrmsg("vmqts found AMS endpoint name invalid.", dp->ept);
		return -1;
	}

	dp->tsep = MTAKE(sizeof(VmqTsep));
	CHKERR(dp->tsep);
	memcpy((char *) (dp->tsep), (char *) &tsep, sizeof(VmqTsep));

	/*	Also parse out the service mode of this endpoint.	*/

	dp->diligence = AmsAssured;
	dp->sequence = AmsTransmissionOrder;
	return 0;
}

static void	vmqClearAmsEndpoint(AmsEndpoint *dp)
{
	CHKERR(dp);
	if (dp->tsep)
	{
		MRELEASE(dp->tsep);
	}
}

static int	vmqSendAms(AmsEndpoint *dp, AmsSAP *sap,
			unsigned char flowLabel, char *header,
			int headerLen, char *content, int contentLen)
{
	char		*vmqAmsBuf;
	int		len;
	VmqTsep		*tsep;
	unsigned short	checksum;
	int		result;

	CHKERR(dp);
	CHKERR(sap);
	CHKERR(header);
	CHKERR(headerLen >= 0);
	CHKERR(contentLen == 0 || (contentLen > 0 && content != NULL));
	len = headerLen + contentLen + 2;
	CHKERR(contentLen <= VMQTS_MAX_MSG_LEN);
	tsep = (VmqTsep *) (dp->tsep);
#if AMSDEBUG
printf("in vmqSendAms, tsep is %lu.\n", (unsigned long) tsep);
#endif
	if (tsep == NULL)	/*	Lost connectivity to endpoint.	*/
	{
		return 0;
	}

	vmqAmsBuf = MTAKE(headerLen + contentLen + 2);
	CHKERR(vmqAmsBuf);
	memcpy(vmqAmsBuf, header, headerLen);
	if (contentLen > 0)
	{
		memcpy(vmqAmsBuf + headerLen, content, contentLen);
	}

	checksum = computeAmsChecksum((unsigned char *) vmqAmsBuf,
			headerLen + contentLen);
	checksum = htons(checksum);
	memcpy(vmqAmsBuf + headerLen + contentLen, (char *) &checksum, 2);
	result = msgQSend(*tsep, vmqAmsBuf, len, WAIT_FOREVER, MSG_PRI_NORMAL);
	MRELEASE(vmqAmsBuf);
	if (result == ERROR)
	{
#if AMSDEBUG
PUTS("vmqSendAms failed.");
#endif
		return -1;
	}

#if AMSDEBUG
PUTS("vmqSendAms succeeded.");
#endif
	return 0;
}

static void	vmqShutdown(void *sap)
{
	MSG_Q_ID	vmqSap = (MSG_Q_ID) sap;

	CHVOID(sap);
	msgQDelete(vmqSap);
}

void	vmqtsLoadTs(TransSvc *ts)
{
	char		ownHostName[MAXHOSTNAMELEN + 1];
	unsigned int	ipAddress;
	static char	vmqName[32];

	/*	NOTE: VX message queue endpoints are uniquely
	 *	identified by MSG_Q_ID (a pointer to a message
	 *	queue structure, as returned by msgQCreate or
	 *	msgQSmCreate) within transport service name,
	 *	which is different for each memory space within
	 *	which message queues can be created.  We assume
	 *	that IP address can be used as the unique identifier
	 *	of each such message space.  For a shared-memory
	 *	message queue, the message space identifier needs
	 *	to uniquely identify the set of processors sharing
	 *	access to the memory board in which the shared-memory
	 *	message queues are constructed; IP address may not
	 *	be sufficient for this purpose, in which case some
	 *	other unique identifier will be needed, e.g.,
	 *	spacecraft ID.  We haven't yet figured out how to
	 *	implement this.						*/

	CHKVOID(ts);
	getNameOfHost(ownHostName, sizeof ownHostName);
	ipAddress = getInternetAddress(ownHostName);
	isprintf(vmqName, sizeof vmqName, "vmq%u", ipAddress);
	ts->name = vmqName;
	ts->csepNameFn = vmqComputeCsepName;
	ts->mamsInitFn = vmqMamsInit;
	ts->mamsReceiverFn = vmqMamsReceiver;
	ts->parseMamsEndpointFn = vmqParseMamsEndpoint;
	ts->clearMamsEndpointFn = vmqClearMamsEndpoint;
	ts->sendMamsFn = vmqSendMams;
	ts->amsInitFn = vmqAmsInit;
	ts->amsReceiverFn = vmqAmsReceiver;
	ts->parseAmsEndpointFn = vmqParseAmsEndpoint;
	ts->clearAmsEndpointFn = vmqClearAmsEndpoint;
	ts->sendAmsFn = vmqSendAms;
	ts->shutdownFn = vmqShutdown;
}

#endif
