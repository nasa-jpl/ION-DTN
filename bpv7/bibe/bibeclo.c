/*
	bibeclo.c:	BP BIBE-based convergence-layer output
			daemon, for use with BPv7.

	Author:		Scott Burleigh, JPL

	Copyright (c) 2020, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include "bpP.h"
#include "bibeP.h"

#ifndef	BIBE_SIGNAL_TIME_MARGIN
#define	BIBE_SIGNAL_TIME_MARGIN	10
#endif

static sm_SemId		bibecloSemaphore(sm_SemId *semid)
{
	static sm_SemId	semaphore = SM_SEM_NONE;
	
	if (semid)
	{
		semaphore = *semid;
	}

	return semaphore;
}

static BpSAP	_bpduSap(BpSAP *newSap)
{
	void	*value;
	BpSAP	sap;

	if (newSap)			/*	Add task variable.	*/
	{
		value = (void *) (*newSap);
		sap = (BpSAP) sm_TaskVar(&value);
	}
	else				/*	Retrieve task variable.	*/
	{
		sap = (BpSAP) sm_TaskVar(NULL);
	}

	return sap;
}

static BpSAP	_signalSap(BpSAP *newSap)
{
	void	*value;
	BpSAP	sap;

	if (newSap)			/*	Add task variable.	*/
	{
		value = (void *) (*newSap);
		sap = (BpSAP) sm_TaskVar(&value);
	}
	else				/*	Retrieve task variable.	*/
	{
		sap = (BpSAP) sm_TaskVar(NULL);
	}

	return sap;
}

static ReqAttendant	*_bpduAttendant(ReqAttendant *newAttendant)
{
	static ReqAttendant	*attendant = NULL;

	if (newAttendant)
	{
		attendant = newAttendant;
	}

	return attendant;
}

static ReqAttendant	*_signalAttendant(ReqAttendant *newAttendant)
{
	static ReqAttendant	*attendant = NULL;

	if (newAttendant)
	{
		attendant = newAttendant;
	}

	return attendant;
}

static void	shutDownClo()
{
	sm_SemEnd(bibecloSemaphore(NULL));
}

static void	handleQuit(int signum)
{
	isignal(SIGTERM, handleQuit);
	bp_interrupt(_bpduSap(NULL));
	bp_interrupt(_signalSap(NULL));
	ionPauseAttendant(_bpduAttendant(NULL));
	ionPauseAttendant(_signalAttendant(NULL));
	shutDownClo();
}

/*	*	Signal transmission thread functions	*	*	*/

typedef struct
{
	char		sourceEid[SDRSTRING_BUFSZ];
	char		peerEid[SDRSTRING_BUFSZ];
	Object		bclaAddr;
	BpSAP		sap;
	ReqAttendant	attendant;
	int		running;
} SignalThreadParms;

static int	sendSignal(SignalThreadParms *stp, int disposition)
{
	Sdr		sdr = getIonsdr();
	BpAncillaryData	ancillaryData;
	Bcla		bcla;
	CtSignal	*signal;
	int		sequenceCount;
	int		buflen;
	unsigned char	*buffer;
	unsigned char	*cursor;
	uvast		uvtemp;
	Object		elt;
	Object		sequenceAddr;
	CtSequence	sequence;
	int		msglen;
	Object		msg;
	Object		payloadZco;
	int		result = 0;

	memset((char *) &ancillaryData, 0, sizeof(BpAncillaryData));
	ancillaryData.flags = BP_RELIABLE | BP_BEST_EFFORT;
	oK(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &bcla, stp->bclaAddr, sizeof(Bcla));
	signal = bcla.signals + disposition;
	sequenceCount = sdr_list_length(sdr, signal->sequences);
	buflen = 1		/*	Admin record array open		*/
		+ 1		/*	Admin record type code		*/
		+ 1		/*	Signal array open		*/
		+ 1		/*	Disposition code		*/
		+ 9		/*	Scope report array open		*/
		+ (19 * sequenceCount);	/*	Sequences		*/
	buffer = MTAKE(buflen);
	if (buffer == NULL)
	{
		sdr_exit_xn(sdr);
		putErrmsg("Can't allocate memory for custody signal.", NULL);
		return -1;
	}

	cursor = buffer;

	/*	Serialize admin record.					*/

	uvtemp = 2;
	oK(cbor_encode_array_open(uvtemp, &cursor));
	uvtemp = BP_BIBE_SIGNAL;
	oK(cbor_encode_integer(uvtemp, &cursor));

	/*	Serialize BIBE custody signal.				*/

	uvtemp = 2;
	oK(cbor_encode_array_open(uvtemp, &cursor));
	uvtemp = disposition;
	oK(cbor_encode_integer(uvtemp, &cursor));

	/*	Serialize disposition scope report.			*/

	uvtemp = sdr_list_length(sdr, signal->sequences);
	oK(cbor_encode_array_open(uvtemp, &cursor));
	while ((elt = sdr_list_first(sdr, signal->sequences)))
	{
		sequenceAddr = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &sequence, sequenceAddr,
				sizeof(CtSequence));
		uvtemp = 2;
		oK(cbor_encode_array_open(uvtemp, &cursor));
		uvtemp = sequence.firstXmitId;
		oK(cbor_encode_integer(uvtemp, &cursor));
		uvtemp = (sequence.lastXmitId - sequence.firstXmitId) + 1;
		oK(cbor_encode_integer(uvtemp, &cursor));
		sdr_free(sdr, sequenceAddr);
		sdr_list_delete(sdr, elt, NULL, NULL);
	}

	/*	Record the custody signal as an application data unit.	*/

	msglen = cursor - buffer;
	msg = sdr_malloc(sdr, msglen);
	if (msg == 0)
	{
		putErrmsg("Can't allocate heap for custody signal.", NULL);
		MRELEASE(buffer);
		return -1;
	}

	sdr_write(sdr, msg, (char *) buffer, msglen);
	MRELEASE(buffer);

	/*	Reset the signal for future BPDU reception.		*/

	signal->deadline = (time_t) MAX_TIME;
	sdr_write(sdr, stp->bclaAddr, (char *) &bcla, sizeof(Bcla));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't write custody signal to heap.", NULL);
		return -1;
	}

	/*	Send the custody signal in a bundle.			*/

	payloadZco = ionCreateZco(ZcoSdrSource, msg, 0, msglen,
			bcla.classOfService, 0, ZcoOutbound,
			&(stp->attendant));
	if (payloadZco == 0 || payloadZco == (Object) ERROR)
	{
		putErrmsg("Can't create ZCO for custody signal.", NULL);
		return -1;
	}

	if (bpSend(&(stp->sap->endpointMetaEid), stp->peerEid, NULL,
			bcla.fwdLatency + BIBE_SIGNAL_TIME_MARGIN,
			bcla.classOfService, NoCustodyRequested, 0,
			0, &ancillaryData, payloadZco, NULL,
			BP_BIBE_SIGNAL) < 1)
	{
		putErrmsg("Can't send custody signal.", NULL);
		result = -1;
	}

	return result;
}

static void	*sendSignals(void *parm)
{
	/*	Main loop for transmission of BIBE CT signals.		*/

	Sdr			sdr = getIonsdr();
	Bcla			bcla;
	SignalThreadParms	*stp = (SignalThreadParms *) parm;
	int			i;
	DtnTime			arrivalTime;

	snooze(1);	/*	Let main thread become interruptible.	*/

	/*	Can now start sending custody signals.			*/

	if (bp_open_source(stp->sourceEid, &(stp->sap), 0) < 0)
	{
		putErrmsg("Can't open source SAP.", NULL);
		shutDownClo();
		stp->running = 0;
		return NULL;
	}

	_signalSap(&(stp->sap));
	if (ionStartAttendant(&(stp->attendant)))
	{
		bp_close(stp->sap);
		putErrmsg("Can't start attendant.", NULL);
		shutDownClo();
		stp->running = 0;
		return NULL;
	}

	_signalAttendant(&(stp->attendant));
	while (stp->running)
	{
		getCurrentDtnTime(&arrivalTime);
		sdr_read(sdr, (char *) &bcla, stp->bclaAddr, sizeof(Bcla));
		arrivalTime += (bcla.fwdLatency + BIBE_SIGNAL_TIME_MARGIN);
		for (i = 0; i < CT_DISPOSITIONS; i++)
		{
			if (bcla.signals[i].sequences == 0)
			{
				continue;
			}

			if (bcla.signals[i].deadline <= arrivalTime)
			{
				if (sendSignal(stp, i) < 0)
				{
					bp_close(stp->sap);
					ionStopAttendant(&(stp->attendant));
					shutDownClo();
					stp->running = 0;
					return NULL;
				}
			}
		}

		snooze(1);
	}

	bp_close(stp->sap);
	ionStopAttendant (&(stp->attendant));
	writeErrmsgMemos();
	writeMemo("[i] bibeclo signal xmit thread has ended.");
	return NULL;
}

/*	*	*	Main thread functions	*	*	*	*/

#if defined (VXWORKS) || defined (RTEMS) || defined (bionic)
int	bibeclo(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char			*peerEid = (char *) a1;
	char			*destEid = (char *) a2;
#else
int	main(int argc, char *argv[])
{
	char			*peerEid = argc > 1 ? argv[1] : NULL;
	char			*destEid = argc > 2 ? argv[2] : NULL;
#endif
	VOutduct		*vduct;
	PsmAddress		vductElt;
	Outduct			outduct;
	Object			bclaAddr;
	Object			bclaElt;
	Bcla			bcla;
	SignalThreadParms	stp;
	Sdr			sdr;
	int			ttl;
	BpSAP			sap;
	ReqAttendant		attendant;
	unsigned char		*buffer;
	pthread_t		signalThread;
	Object			bpduZco;
	vast			bpduZcoLength;
	BpAncillaryData		ancillaryData;
	int			protocolClassReq;
	unsigned char		*cursor;
	uvast			uvtemp;
	DtnTime			deadline;
	int			hdrlen;
	Object			newBundle;
	Bundle			bundle;
	BpEvent			event;
	Object			elt;

	if (peerEid == NULL || destEid == NULL)
	{
		PUTS("Usage: bibeclo <peer node's ID> <destination node's ID>");
		return 0;
	}

	istrcpy(stp.peerEid, peerEid, SDRSTRING_BUFSZ);
	if (bpAttach() < 0)
	{
		putErrmsg("bibeclo can't attach to BP.", NULL);
		return -1;
	}

	findOutduct("bibe", destEid, &vduct, &vductElt);
	if (vductElt == 0)
	{
		writeMemoNote("[?] No such bibe outduct", destEid);
		return -1;
	}

	if (vduct->cloPid != ERROR && vduct->cloPid != sm_TaskIdSelf())
	{
		writeMemoNote("[?] CLO task is already started for this duct",
				itoa(vduct->cloPid));
		return -1;
	}

	bibeFind(peerEid, &bclaAddr, &bclaElt);
	if (bclaElt == 0)
	{
		writeMemoNote("[?] No such bcla", peerEid);
		return -1;
	}

	stp.bclaAddr = bclaAddr;

	/*	All command-line arguments are now validated.		*/

	sdr = getIonsdr();
	CHKZERO(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &bcla, bclaAddr, sizeof(Bcla));
	sdr_string_read(sdr, stp.sourceEid, bcla.source);
	sdr_read(sdr, (char *) &outduct, sdr_list_data(sdr, vduct->outductElt),
			sizeof(Outduct));
	sdr_exit_xn(sdr);
	ttl = bcla.fwdLatency + bcla.rtnLatency;
	if (bcla.lifespan > ttl)
	{
		ttl = bcla.lifespan;
	}

	/*	Note: we open this SAP with the Detain flag set to 1
	 *	so that newly transmitted bundles can be tracked in
	 *	the bcla's list of bundles subject to custodial
	 *	retransmission driven by timeout expiration.		*/

	if (bp_open_source(stp.sourceEid, &sap, 1) < 0)
	{
		putErrmsg("Can't open source SAP.", stp.sourceEid);
		shutDownClo();
		return -1;
	}

	_bpduSap(&sap);
	if (ionStartAttendant(&attendant))
	{
		bp_close(sap);
		putErrmsg("Can't start attendant.", NULL);
		shutDownClo();
		return -1;
	}

	_bpduAttendant(&attendant);

	/*	Set up signal handling.  SIGTERM is shutdown signal.	*/

	oK(bibecloSemaphore(&(vduct->semaphore)));
	isignal(SIGTERM, handleQuit);

	/*	Allocate buffer for admin record header.		*/

	buffer = (unsigned char *) MTAKE(1	/*	Array (2) open	*/
					+ 9	/*	Admin rec type	*/
					+ 1	/*	Array (3) open	*/
					+ 9	/*	Xmit ID		*/
					+ 9);	/*	Deadline	*/
	if (buffer == NULL)
	{
		bp_close(sap);
		ionStopAttendant(&attendant);
		putErrmsg("Can't create buffer for CLO; stopping.", NULL);
		return -1;
	}

	/*	Start custody signal transmission thread.		*/

	stp.running = 1;
	if (pthread_begin(&signalThread, NULL, sendSignals, &stp))
	{
		bp_close(sap);
		ionStopAttendant(&attendant);
		putSysErrmsg("Can't start signal transmission thread.", NULL);
		MRELEASE(buffer);
		return -1;
	}

	/*	Can now begin transmitting to remote duct.		*/

	writeMemoNote("[i] bibeclo is running for", destEid);
	writeMemoNote("[i]        transmitting to", peerEid);
	while (!(sm_SemEnded(vduct->semaphore)))
	{
		if (bpDequeue(vduct, &bpduZco, &ancillaryData, 0) < 0)
		{
			putErrmsg("Can't dequeue bundle.", NULL);
			shutDownClo();
			break;
		}

		if (bpduZco == 0)	 /*	Outduct closed.		*/
		{
			writeMemo("[i] bibeclo outduct closed.");
			sm_SemEnd(bibecloSemaphore(NULL));/*	Stop.	*/
			continue;
		}


		if (bpduZco == 1)	/*	Got a corrupt bundle.	*/
		{
			continue;	/*	Get next bundle.	*/
		}

		protocolClassReq = ancillaryData.flags & BP_PROTOCOL_ANY;
		memcpy((char *) &ancillaryData, (char *) &bcla.ancillaryData,
				sizeof(BpAncillaryData));

		/*	The BPDU (an administrative record containing
		 *	the entire outbound bundle; the payload of
		 *	the encapsulating bundle) will be formed by
		 *	prepending an administrative record header
		 *	to the outbound bundle.				*/

		bpduZcoLength = zco_length(sdr, bpduZco);

		/*	Serialize the admin record, an array of 2
		 *	elements.					*/

		cursor = buffer;
		uvtemp = 2;
		oK(cbor_encode_array_open(uvtemp, &cursor));

		/*	First element of array is admin record type.	*/

		uvtemp = BP_BIBE_PDU;
		oK(cbor_encode_integer(uvtemp, &cursor));

		/*	Next element of array is admin record content.
		 *	For a BPDU, admin record content is an array
		 *	of 3 elements.					*/ 

		uvtemp = 3;
		oK(cbor_encode_array_open(uvtemp, &cursor));

		/*	First two elements of content array are xmit
		 *	count and deadline.				*/

		if (protocolClassReq & BP_RELIABLE)
		{
			bcla.count += 1;
			uvtemp = bcla.count;
			oK(cbor_encode_integer(uvtemp, &cursor));
			getCurrentDtnTime(&deadline);
			deadline += (bcla.fwdLatency + bcla.rtnLatency);
			uvtemp = deadline;
			oK(cbor_encode_integer(uvtemp, &cursor));
		}
		else
		{
			uvtemp = 0;
			oK(cbor_encode_integer(uvtemp, &cursor));
			uvtemp = 0;
			oK(cbor_encode_integer(uvtemp, &cursor));
		}

		/*	Last element of content array is the
		 *	encapsulated bundle, represented as a byte
		 *	string.						*/

		uvtemp = bpduZcoLength;
		cbor_encode_byte_string(NULL, uvtemp, &cursor);
		hdrlen = cursor - buffer;

		/*	Embed bundle in admin record by prepending
		 *	the admin record header to the encapsulated
		 *	bundle (a ZCO).					*/

		CHKZERO(sdr_begin_xn(sdr));
		zco_prepend_header(sdr, bpduZco, (char *) buffer, hdrlen);
		zco_bond(sdr, bpduZco);
		if (sdr_end_xn(sdr))
		{
			putErrmsg("Can't prepend header; CLO stopping.", NULL);
			shutDownClo();
			continue;
		}

		/*	Send bundle whose payload is the ZCO
		 *	comprising the admin record header and the
		 *	encapsulated bundle.				*/

		switch (bpSend(&(sap->endpointMetaEid), peerEid, NULL, ttl,
				bcla.classOfService, NoCustodyRequested, 0, 0,
			       	&ancillaryData, bpduZco, &newBundle,
				BP_BIBE_PDU))
		{
		case -1:	/*	System error.			*/
			putErrmsg("Can't send encapsulated bundle.", NULL);
			shutDownClo();
			continue;

		case 0:		/*	Malformed request.		*/
			writeMemo("[?] Encapsulated bundle not sent.");
			break;

		default:
			CHKZERO(sdr_begin_xn(sdr));
			if (protocolClassReq & BP_RELIABLE)
			{
				/*	Must record bundle's xmitId &
				 *	deadline to enable custodial
				 *	re-forwarding.
				 *
				 *	First update xmit ID count.	*/

				sdr_stage(sdr, (char *) &bcla, bclaAddr, 0);
				sdr_write(sdr, bclaAddr, (char *) &bcla,
						sizeof(Bcla));

				/*	Now record CT stuff in bundle.	*/

				if ((elt = sdr_list_insert_last(sdr,
					bcla.bundles, newBundle)) == 0
				|| bp_track(newBundle, elt) < 0)
				{
					putErrmsg("Can't track bundle.", NULL);
					sdr_cancel_xn(sdr);
					shutDownClo();
					continue;
				}

				sdr_stage(sdr, (char *) &bundle, newBundle,
					sizeof(Bundle));
				bundle.xmitId = bcla.count;
				bundle.deadline = deadline;
				event.type = ctOverdue;
				event.time = deadline + EPOCH_2000_SEC;
				event.ref = elt;
				if ((bundle.ctDueElt =
					insertBpTimelineEvent(&event)) == 0)
				{
					putErrmsg("Can't schedule rfwd.", NULL);
					sdr_cancel_xn(sdr);
					shutDownClo();
					continue;
				}

				sdr_write(sdr, newBundle, (char *) &bundle,
						sizeof(Bundle));
			}

			oK(bp_release(newBundle));
			if (sdr_end_xn(sdr))
			{
				putErrmsg("Can't release ZCO; CLO stopping.",
						NULL);
				shutDownClo();
				continue;
			}
		}	

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();
	}

	writeErrmsgMemos();
	writeMemo("[i] bibeclo duct has ended.");
	MRELEASE(buffer);
	bp_close(sap);
	ionStopAttendant(&attendant);
	stp.running = 0;
	bp_interrupt(_signalSap(NULL));
	ionPauseAttendant(_signalAttendant(NULL));
	pthread_join(signalThread, NULL);
	ionDetach();
	return 0;
}
