/*
	bputa.c:	UT-layer adapter using Bundle Protocol.
									*/
/*									*/
/*	Copyright (c) 2009, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include <cfdpP.h>

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

/*	*	*	Main thread functions	*	*	*	*/

#if defined (VXWORKS) || defined (RTEMS) || defined (bionic)
int	bputa(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
#else
int	main(int argc, char **argv)
{
#endif
	char		ownEid[64];
	BpSAP		txSap;
	RxThreadParms	parms;
	Sdr		sdr;
	pthread_t	rxThread;
	int		haveRxThread = 0;
	Object		pduZco;
	OutFdu		fduBuffer;
	BpUtParms	utParms;
	uvast		destinationNodeNbr;
	char		destEid[64];
	char		reportToEidBuf[64];
	char		*reportToEid;
	Object		newBundle;
	Object		pduElt;

	if (bp_attach() < 0)
	{
		putErrmsg("CFDP can't attach to BP.", NULL);
		return 0;
	}

	isprintf(ownEid, sizeof ownEid, "ipn:" UVAST_FIELDSPEC ".%u",
			getOwnNodeNbr(), CFDP_SEND_SVC_NBR);
	if (bp_open(ownEid, &txSap) < 0)
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

	sdr = bp_get_sdr();
	parms.mainThread = pthread_self();
	parms.running = 1;
	if (pthread_begin(&rxThread, NULL, receivePdus, &parms))
	{
		bp_close(txSap);
		putSysErrmsg("bputa can't create receiver thread", NULL);
		return -1;
	}

	haveRxThread = 1;
	writeMemo("[i] bputa is running.");
	while (parms.running)
	{
		/*	Get an outbound CFDP PDU for transmission.	*/

		if (cfdpDequeueOutboundPdu(&pduZco, &fduBuffer) < 0)
		{
			writeMemo("[?] bputa can't dequeue outbound CFDP PDU; \
terminating.");
			parms.running = 0;
			continue;
		}

		/*	Determine quality of service for transmission.	*/

		if (fduBuffer.utParmsLength == sizeof(BpUtParms))
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
			utParms.srrFlags = 0;
			utParms.ackRequested = 0;
			utParms.extendedCOS.flowLabel = 0;
			utParms.extendedCOS.flags = 0;
			utParms.extendedCOS.ordinal = 0;
		}

		cfdp_decompress_number(&destinationNodeNbr,
				&fduBuffer.destinationEntityNbr);
		if (destinationNodeNbr == 0)
		{
			writeMemo("[?] bputa declining to send to node 0.");
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

		/*	Send PDU in a bundle.				*/

		newBundle = 0;
		if (bp_send(txSap, destEid, reportToEid, utParms.lifespan,
				utParms.classOfService, utParms.custodySwitch,
				utParms.srrFlags, utParms.ackRequested,
				&utParms.extendedCOS, pduZco, &newBundle) <= 0)
		{
			putErrmsg("bputa can't send PDU in bundle; terminated.",
					NULL);
			parms.running = 0;
		}

		if (newBundle == 0)
		{
			continue;	/*	Must have stopped.	*/
		}

		/*	Enable cancellation of this PDU.		*/

		if (sdr_begin_xn(sdr) == 0)
		{
			parms.running = 0;
			continue;
		}

		pduElt = sdr_list_insert_last(sdr, fduBuffer.extantPdus,
				newBundle);
		if (pduElt)
		{
			bp_track(newBundle, pduElt);
		}

		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("bputa can't track PDU; terminated.", NULL);
			parms.running = 0;
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
