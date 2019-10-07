/*
	dtpcd.c:	DTPC daemon for transmission and reception.

	Authors: Giorgos Papastergiou, SPICE
		 Ioannis Alexiadis, SPICE

	Copyright (c) 2011, Space Internetworking Center,
	Democritus University of Thrace. ALL RIGHTS RESERVED.
										*/

#include <dtpcP.h>

typedef struct
{
	pthread_t	mainThread;
	int		running;
	BpSAP		rxSap;
	BpSAP		txSap;
} RxThreadParms;

/*	*	*	Receiver thread functions	*	*	*/

static void	*getBundles(void *parm)
{
	RxThreadParms		*parms = (RxThreadParms *) parm;
	char			ownEid[64];
	Sdr			sdr = getIonsdr();
	BpDelivery		dlv;
	uvast			profNum;
	Scalar			seqNum;
	char			type;
	unsigned int		aduLength;
	int			bytesRemaining;
	ZcoReader		reader;
	unsigned char		*buffer;
	int			bytesToRead;
	int			sdnvLength;
	unsigned char		*cursor;

	isprintf(ownEid, sizeof ownEid, "ipn:" UVAST_FIELDSPEC ".%d",
			getOwnNodeNbr(), DTPC_RECV_SVC_NBR);
	if (bp_open(ownEid, &(parms->rxSap)) < 0)
	{
		putErrmsg("DTPC can't open own 'recv' endpoint.", ownEid);
		parms->running = 0;
		return NULL;
	}

	writeMemo("[i] dtpcd receiver thread has started.");
	while (parms->running)
	{
		if (bp_receive(parms->rxSap, &dlv, BP_BLOCKING) < 0)
		{
			putErrmsg("dtpcd bundle reception failed.", NULL);
			parms->running = 0;
			continue;
		}

		switch (dlv.result)
		{
		case BpEndpointStopped:
			parms->running = 0;
			break;

		case BpPayloadPresent:
			CHKNULL(sdr_begin_xn(sdr));

			/* Since the max length of a Sdnv is 10 bytes,
			 * read 21 bytes to be sure that the Profile
			 * and Sequence number Sdnvs plus the type
			 * were read.					*/

			aduLength = zco_source_data_length(sdr, dlv.adu);
			bytesRemaining = aduLength;
			if (aduLength < 21)	/* Just in case we receive
						 * a very small adu.	*/
			{			
				bytesToRead = aduLength;
			}
			else
			{
				bytesToRead = 21;
			}

			buffer = MTAKE(bytesToRead);
			if (buffer == NULL)
			{
				putErrmsg("Out of memory.",NULL);
				return NULL;
			}

			cursor = buffer;
			zco_start_receiving(dlv.adu, &reader);
			if (zco_receive_headers(sdr, &reader, bytesToRead,
					(char *) buffer) < 0)
			{
				putErrmsg("dtpcd can't receive ADU header.",
						itoa(bytesToRead));
				sdr_cancel_xn(sdr);
				MRELEASE(buffer);
				parms->running = 0;
				continue;
			}

			type = *cursor;		/* Get the type byte.	*/
			cursor++;
			bytesRemaining--;
			sdnvLength = decodeSdnv(&profNum, cursor);
			cursor += sdnvLength;
			bytesRemaining -= sdnvLength;
			sdnvLength = sdnvToScalar(&seqNum, cursor);
			cursor += sdnvLength;
			bytesRemaining -= sdnvLength;

			/*	Mark remaining bytes as source data.	*/

			zco_delimit_source(sdr, dlv.adu, cursor - buffer,
					bytesRemaining);
			zco_strip(sdr, dlv.adu);
			MRELEASE(buffer);
			if (sdr_end_xn(sdr) < 0)
			{
				putErrmsg("dtpcd can't handle bundle delivery.",
						NULL);
				parms->running = 0;
				continue;
			}

			switch (type)
			{
			case 0x00:	/*	Received an adu.	*/
				switch (handleInAdu(sdr, parms->txSap, &dlv,
						profNum, seqNum))
				{
				case -1:
					putErrmsg("dtpcd can't handle inbound \
adu.", NULL);
					parms->running = 0;
					continue;

				case 1:
					if (parseInAdus(sdr) < 0)
					{
						putErrmsg("dtpcd can't parse \
							inbound adus.", NULL);
						parms->running = 0;
						continue;
					}

					/*	Intentional fall-through
					 *	to remaining cases.	*/

				case 0: 
				default:
					break;	/*	Inner switch.	*/
				}

				break;		/*	Middle switch.	*/

			case 0x01:	/*	Received an ACK.	*/
				if (handleAck(sdr, &dlv, profNum, seqNum) < 0)
				{
					putErrmsg("dtpcd can't handle ACK.",
							NULL);
					parms->running = 0;
					continue;
				}

				break;		/*	Middle switch.	*/
			default:
				writeMemo("[?] Invalid item type. Corrupted \
item?");
				break;		/*	Middle switch.	*/
			}

		default:
			break;			/*	Outer switch.	*/
		}

		bp_release_delivery(&dlv, 0);	/* We manually delete the
						 * ZCO elsewhere.	*/

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();
	}

	bp_close(parms->rxSap);
	writeMemo("[i] dtpcd receiver has stopped.");
	return NULL;
}


/*	*	*	Main thread functions	*	*	*	*/

#if defined (ION_LWT)
int	dtpcd(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
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

	if (bp_attach() < 0)
	{
		putErrmsg("DTPC can't attach to BP.", NULL);
		return 0;
	}

	isprintf(ownEid, sizeof ownEid, "ipn:" UVAST_FIELDSPEC ".%d",
			getOwnNodeNbr(), DTPC_SEND_SVC_NBR);
	if (bp_open_source(ownEid, &txSap, 1) < 0)
	{
		putErrmsg("DTPC can't open own 'send' endpoint.", ownEid);
		return 0;
	}

	if (txSap == NULL)
	{
		putErrmsg("dtpcd can't get Bundle Protocol SAP.", NULL);
		return 0;
	}

	if (dtpcAttach() < 0)
	{
		bp_close(txSap);
		putErrmsg("dtpcd can't attach to DTPC.", NULL);
		return 0;
	}

	parms.mainThread = pthread_self();
	parms.running = 1;
	parms.txSap = txSap;
	if (pthread_begin(&rxThread, NULL, getBundles,
		&parms,"dtpcd_receiver"))
	{
		bp_close(txSap);
		putSysErrmsg("dtpcd can't create receiver thread", NULL);
		return -1;
	}
	
	writeMemo("[i] dtpcd is running.");
	while (parms.running)
	{
		/*	Get an outbound ADU for transmission.		*/

		if (sendAdu(txSap) < 0)
		{
			writeMemo("[?] dtpcd can't dequeue outbound ADU; \
terminating.");
			parms.running = 0;
			continue;
		}
		
		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();
	}
	
	bp_interrupt(parms.rxSap);
	pthread_join(rxThread, NULL);
	bp_close(txSap);
	writeErrmsgMemos();
	writeMemo("[i] dtpcd has ended.");
	ionDetach();
	return 0;
}
