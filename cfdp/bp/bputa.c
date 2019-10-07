/*
	bputa.c:	UT-layer adapter using Bundle Protocol.
									*/
/*	Copyright (c) 2009, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include <cfdpP.h>
#include <bputa.h>

#define	CFDP_SEND_SVC_NBR	(64)
#define	CFDP_RECV_SVC_NBR	(65)

typedef struct
{
	pthread_t	mainThread;
	int		running;
	BpSAP		rxSap;
} RxThreadParms;

/*	*	*	Receiver thread functions	*	*	*/

static void	*receivePdus(void *parm)
{
	RxThreadParms	*parms = (RxThreadParms *) parm;
	char		ownEid[64];
	Sdr		sdr;
	BpDelivery	dlv;
	int		contentLength;
	ZcoReader	reader;
	unsigned char	*buffer;

	buffer = MTAKE(CFDP_MAX_PDU_SIZE);
	if (buffer == NULL)
	{
		putErrmsg("bputa receiver thread can't get buffer.", NULL);
		parms->running = 0;
		return NULL;
	}

	isprintf(ownEid, sizeof ownEid, "ipn:" UVAST_FIELDSPEC ".%u",
			getOwnNodeNbr(), CFDP_RECV_SVC_NBR);
	if (bp_open(ownEid, &(parms->rxSap)) < 0)
	{
		MRELEASE(buffer);
		putErrmsg("CFDP can't open own 'recv' endpoint.", ownEid);
		parms->running = 0;
		return NULL;
	}

	sdr = bp_get_sdr();
	writeMemo("[i] bputa input has started.");
	while (parms->running)
	{
		if (bp_receive(parms->rxSap, &dlv, BP_BLOCKING) < 0)
		{
			putErrmsg("bputa bundle reception failed.", NULL);
			parms->running = 0;
			continue;
		}

		switch (dlv.result)
		{
		case BpEndpointStopped:
			parms->running = 0;
			break;

		case BpPayloadPresent:
			contentLength = zco_source_data_length(sdr, dlv.adu);
			CHKNULL(sdr_begin_xn(sdr));
			zco_start_receiving(dlv.adu, &reader);
			if (zco_receive_source(sdr, &reader, contentLength,
					(char *) buffer) < 0)
			{
				sdr_cancel_xn(sdr);
				putErrmsg("bputa can't receive bundle ADU.",
						itoa(contentLength));
				parms->running = 0;
				continue;
			}

			if (sdr_end_xn(sdr) < 0)
			{
				putErrmsg("bputa can't handle bundle delivery.",
						NULL);
				parms->running = 0;
				continue;
			}

			if (cfdpHandleInboundPdu(buffer, contentLength) < 0)
			{
				putErrmsg("bputa can't handle inbound PDU.",
						NULL);
				parms->running = 0;
			}

			break;

		default:
			break;
		}

		bp_release_delivery(&dlv, 1);

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();
	}

	bp_close(parms->rxSap);
	MRELEASE(buffer);
	writeMemo("[i] bputa input has stopped.");
	return NULL;
}

static int	deletePdu(Object pduZco)
{
	Sdr	sdr = getIonsdr();

	if (sdr_begin_xn(sdr) == 0)
	{
		return -1;
	}

	zco_destroy(sdr, pduZco);
	return sdr_end_xn(sdr);
}

/*	*	*	Main thread functions	*	*	*	*/

#if defined (ION_LWT)
int	bputa(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
#else
int	main(int argc, char **argv)
{
#endif
	char		ownEid[64];
	BpSAP		txSap;
	RxThreadParms	parms;
	pthread_t	rxThread;
	int		haveRxThread = 0;
	Sdr		sdr;
	Object		pduZco;
	OutFdu		fduBuffer;
	FinishPdu	fpdu;
	int		direction;
	BpUtParms	utParms;
	uvast		destinationNodeNbr;
	char		destEid[64];
	char		reportToEidBuf[64];
	char		*reportToEid;
	Object		newBundle;

	if (bp_attach() < 0)
	{
		putErrmsg("CFDP can't attach to BP.", NULL);
		return 0;
	}

	isprintf(ownEid, sizeof ownEid, "ipn:" UVAST_FIELDSPEC ".%u",
			getOwnNodeNbr(), CFDP_SEND_SVC_NBR);
	if (bp_open_source(ownEid, &txSap, 1) < 0)
	{
		putErrmsg("CFDP can't open own 'send' endpoint.", ownEid);
		return 0;
	}

	if (txSap == NULL)
	{
		putErrmsg("bputa can't get Bundle Protocol SAP.", NULL);
		return 0;
	}

	if (cfdpAttach() < 0)
	{
		bp_close(txSap);
		putErrmsg("bputa can't attach to CFDP.", NULL);
		return 0;
	}

	parms.mainThread = pthread_self();
	parms.running = 1;
	if (pthread_begin(&rxThread, NULL, receivePdus, &parms, "bputa_receiver"))
	{
		bp_close(txSap);
		putSysErrmsg("bputa can't create receiver thread", NULL);
		return -1;
	}

	haveRxThread = 1;
	writeMemo("[i] bputa is running.");
	sdr = getIonsdr();
	while (parms.running)
	{
		/*	Get an outbound CFDP PDU for transmission.	*/

		if (cfdpDequeueOutboundPdu(&pduZco, &fduBuffer, &fpdu,
				&direction) < 0 || pduZco == 0)
		{
			writeMemo("[?] bputa can't dequeue outbound CFDP PDU; \
terminating.");
			parms.running = 0;
			continue;
		}

		/*	Determine quality of service for transmission.	*/

		if (direction == 0	/*	Toward file receiver.	*/
		&& fduBuffer.utParmsLength == sizeof(BpUtParms))
		{
			memcpy((char *) &utParms, (char *) &fduBuffer.utParms,
					sizeof(BpUtParms));
		}
		else
		{
			memset((char *) &utParms, 0, sizeof(BpUtParms));
			utParms.reportToNodeNbr = 0;
			utParms.lifespan = 86400;	/*	1 day.	*/
			utParms.classOfService = BP_STD_PRIORITY;
			utParms.custodySwitch = NoCustodyRequested;
			utParms.ctInterval = 0;
			utParms.srrFlags = 0;
			utParms.ackRequested = 0;
			utParms.ancillaryData.dataLabel = 0;
			utParms.ancillaryData.flags = 0;
			utParms.ancillaryData.ordinal = 0;
		}

		if (direction == 0)
		{
			cfdp_decompress_number(&destinationNodeNbr,
					&fduBuffer.destinationEntityNbr);
		}
		else
		{
			cfdp_decompress_number(&destinationNodeNbr,
					&fpdu.transactionId.sourceEntityNbr);
		}

		if (destinationNodeNbr == 0)
		{
			writeMemo("[?] bputa declining to send to node 0.");
			if (deletePdu(pduZco) < 0)
			{
				putErrmsg("bputa can't ditch PDU; terminated.",
						NULL);
				parms.running = 0;
			}

			continue;
		}

		isprintf(destEid, sizeof destEid, "ipn:" UVAST_FIELDSPEC ".%u",
				destinationNodeNbr, CFDP_RECV_SVC_NBR);
		if (utParms.reportToNodeNbr == 0)
		{
			reportToEid = NULL;
		}
		else
		{
			isprintf(reportToEidBuf, sizeof reportToEidBuf,
					"ipn:" UVAST_FIELDSPEC ".%u",
					utParms.reportToNodeNbr,
					CFDP_RECV_SVC_NBR);
			reportToEid = reportToEidBuf;
		}

		/*	Send PDU in a bundle.  Do this inside a
		 *	transaction to ensure that bundle forwarding
		 *	and transmission doesn't happen before the
		 *	bundle is released from detention (as needed).	*/

		if (sdr_begin_xn(sdr) == 0)
		{
			parms.running = 0;
			continue;
		}

		if (bp_send(txSap, destEid, reportToEid, utParms.lifespan,
			utParms.classOfService, utParms.custodySwitch,
			utParms.srrFlags, utParms.ackRequested,
			&utParms.ancillaryData, pduZco, &newBundle) <= 0)
		{
			putErrmsg("bputa can't send PDU in bundle; terminated.",
					NULL);
			sdr_cancel_xn(sdr);
			parms.running = 0;
			continue;
		}

		if (utParms.custodySwitch == SourceCustodyRequired
		&& utParms.ctInterval > 0)
		{
			if (bp_memo(newBundle, utParms.ctInterval) < 0)
			{
				putErrmsg("bputa can't schedule custodial \
retransmission; terminated.", NULL);
				sdr_cancel_xn(sdr);
				parms.running = 0;
				continue;
			}
		}

		/*	Bundle has been detained long enough for us to
		 *	track it as necessary, so we can now release it
		 *	for normal processing.				*/

		bp_release(newBundle);
		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("bputa error sending PDU; terminated.", NULL);
			parms.running = 0;
			continue;
		}

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();
	}

	if (haveRxThread)
	{
		bp_interrupt(parms.rxSap);
		pthread_join(rxThread, NULL);
	}

	bp_close(txSap);
	writeErrmsgMemos();
	writeMemo("[i] Stopping bputa.");
	ionDetach();
	return 0;
}
