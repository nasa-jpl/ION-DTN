/*
	bibecli.c:	Convergence-layer input process for the
			BIBE convergence-layer protocol adapter.

			Acting as a BP application, bibecli takes
			as input (via a BP SAP) a BIBE message
			(called a bpdu) that is delivered to it
			when the bundle protocol agent acquires a
			bundle whose payload (application data
			unit) is such a message.  Then, acting
			as a CLI, it uses an induct to pass the
			serialized bundle encapsulated in the
			BPDU to the bundle protocol agent as a
			new inbound bundle.

	Author: Scott Burleigh, JPL

	Copyright (c) 2026, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
#include "bibeP.h"

static void	interruptThread(int signum)
{
	/*	Tell the compiler that we are not using 'signum'.	*/
	(void) signum;

	isignal(SIGTERM, interruptThread);
	ionKillMainThread("bibecli");
}

/*	*	*	Receiver thread functions	*	*	*/

typedef struct
{
	VInduct	*vinduct;
	int	*running;
	BpSAP	sap;
} ReceiverThreadParms;

static int	stripBpduHeader(Object bpduZco)
{
	Sdr		sdr = getIonsdr();
	vast		bpduLength;
	ZcoReader	reader;
	unsigned char	headerBuf[18];
	unsigned int	bytesToParse;
	unsigned char	*cursor;
	unsigned int	unparsedBytes;
	uvast		uvtemp;
	vast		bundleLength;
	vast		headerLength;

	/*	The payload of a BIBE bundle is a BIBE Protocol Data
	 *	Unit (BPDU), a message that encapsulates a serialized
	 *	bundle (a ZCO).  To access the encapsulated serialized
	 *	bundle in a BPDU we have to strip off the message
	 *	header.							*/

	bpduLength = zco_source_data_length(sdr, bpduZco);
	CHKERR(sdr_begin_xn(sdr));

	/*	Read and strip off the BPDU message's header:
	 *	1-element array open (up to 9 bytes) followed by
	 *	byte string tag (up to 9 bytes) preceding the
	 *	encapsulated ZCO.					*/

	zco_start_receiving(bpduZco, &reader);
	bytesToParse = zco_receive_source(sdr, &reader, 18, (char *) headerBuf);
	if (bytesToParse < 4)
	{
		writeMemo("[?] bcli can't receive BPDU header.");
		oK(sdr_end_xn(sdr));
		return 0;
	}

	cursor = headerBuf;
	unparsedBytes = bytesToParse;
	uvtemp = 1;	/*	Decode array of size 1.			*/
	if (cbor_decode_array_open(&uvtemp, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] BIBE can't decode BPDU array open.");
		oK(sdr_end_xn(sdr));
		return 0;
	}
#if 0
	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] BIBE can't decode BPDU xmit ID.");
		oK(sdr_end_xn(sdr));
		return 0;
	}

	*xmitId = uvtemp;
	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] BIBE can't decode BPDU deadline.");
		oK(sdr_end_xn(sdr));
		return 0;
	}

	*deadline = uvtemp;
#endif
	uvtemp = (uvast) -1;	/*	Decode fixed-lenth byte string.	*/
	if (cbor_decode_byte_string(NULL, &uvtemp, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] BIBE can't decode BPDU encapsulated bundle.");
		oK(sdr_end_xn(sdr));
		return 0;
	}

	bundleLength = uvtemp;
	headerLength = cursor - headerBuf;
	if ((bundleLength + headerLength) != bpduLength)
	{
		writeMemo("[?] BIBE encoding of payload is invalid.");
		writeMemoNote("[?]     bpduLength  ", itoa(bpduLength));
		writeMemoNote("[?]     headerLength", itoa(headerLength));
		writeMemoNote("[?]     bundleLength", itoa(bundleLength));
		oK(sdr_end_xn(sdr));
		return 0;
	}

	/*	Now strip off the BPDU header, leaving just the
	 *	encapsulated serialized bundle.				*/

	zco_delimit_source(sdr, bpduZco, headerLength, bundleLength);
	zco_strip(sdr, bpduZco);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Failed stripping header of BPDU.", NULL);
		return -1;
	}

	return 1;
}

static int	bibeHandleBpdu(BpDelivery *dlv, VInduct *vinduct)
{
	Sdr		sdr = getIonsdr();
	Object		bpduZco;
	AcqWorkArea	*work;

	/*	The ADU in the dlv structure is the ZCO representation
	 *	of the *payload* of a bundle sent by BIBE.  As such,
	 *	it is a BPDU, i.e., a structure that encapsulates a
	 *	bundle that is to be dispatched.			*/

	bpduZco = dlv->adu;
	CHKERR(sdr_begin_xn(sdr));
	switch (stripBpduHeader(bpduZco))
	{
	case -1:
		writeMemo("[?] Can't strip BPDU header.");
		oK(sdr_end_xn(sdr));
		return 0;

	case 0:
		writeMemo("[?] bibecli can't process BPDU.");
		oK(sdr_end_xn(sdr));
		return 0;
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't handle BPDU.", NULL);
		return -1;
	}

	/*	Now acquire and dispatch the encapsulated bundle.	*/

	work = bpGetAcqArea(vinduct);
	if (work == NULL)
	{
		putErrmsg("Can't get acquisition work area", NULL);
		return -1;
	}

	if (bpBeginAcq(work, 0, NULL) < 0)
	{
		putErrmsg("Can't begin bundle acquisition.", NULL);
		return -1;
	}

	if (bpLoadAcq(work, bpduZco) < 0)
	{
		putErrmsg("Can't continue bundle acquisition.", NULL);
		return -1;
	}

	if (bpEndAcq(work) < 0)
	{
		putErrmsg("Can't complete bundle acquisition.", NULL);
		return -1;
	}

	bpReleaseAcqArea(work);
	return 0;
}

static void	*handleBibeBundles(void *parm)
{
	ReceiverThreadParms	*parms = (ReceiverThreadParms *) parm;
	int			*running = parms->running;
	BpSAP			sap = parms->sap;;
	BpDelivery		dlv;

	snooze(1);	/*	Let main thread become interruptable.	*/
	while (*running && !(sm_SemEnded(sap->recvSemaphore)))
	{
		if (bp_receive(sap, &dlv, BP_BLOCKING) < 0)
		{
			putErrmsg("BIBE bundle reception failed.", NULL);
			*running = 0;
			continue;
		}

		switch (dlv.result)
		{
		case BpPayloadPresent:
			break;

		case BpEndpointStopped:
			*running = 0;

			/*	Intentional fall-through to default.	*/

		default:
			continue;
		}

		if (bibeHandleBpdu(&dlv, parms->vinduct) < 0)
		{
			putErrmsg("BIBE PDU handler failed.", NULL);
			*running = 0;
		}

		bp_release_delivery(&dlv, 0);

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();
	}

	return NULL;
}

#if defined (ION_LWT)
int	bibecli(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char	*ownEid = (char *) a1;
#else
int	main(int argc, char *argv[])
{
	char	*ownEid = (argc > 1 ? argv[1] : NULL);
#endif
	ReceiverThreadParms	parms;
	PsmAddress		vinductElt;
	int			running = 1;
	pthread_t		receiverThread;

	if (ownEid == NULL)
	{
		PUTS("Usage: bibecli <BIBE endpoint ID>");
		return 0;
	}

	if (bpAttach() < 0)
	{
		putErrmsg("bibecli can't attach to BP.", NULL);
		return -1;
	}

	if (bp_open(ownEid, &parms.sap) < 0)
	{
		putErrmsg("Can't open bibecli endpoint.", ownEid);
		bpDetach();
		return -1;
	}

	/*	All command-line arguments are now validated.
	 *	Set up signal handling.  SIGTERM is shutdown signal.	*/

	ionNoteMainThread("bibecli");
	isignal(SIGTERM, interruptThread);

	/*	Start the receiver thread.				*/

	findInduct("bibe", ownEid, &parms.vinduct, &vinductElt);
	if (vinductElt == 0)
	{
		putErrmsg("Can't get bibe induct", ownEid);
		return -1;
	}

	parms.running = &running;
	if (pthread_begin(&receiverThread, NULL, handleBibeBundles, &parms))
	{
		bp_close(parms.sap);
		bpDetach();
	}

	writeMemo("[i] bibecli is running.");

	/*	Now sleep until interrupted by SIGTERM, at which point
	 *	it's time to stop the daemon.				*/

	ionPauseMainThread(-1);

	/*	Time to shut down.					*/

	running = 0;

	/*	Shut down the receiver thread.				*/

	bp_close(parms.sap);
	pthread_join(receiverThread, NULL);
	writeErrmsgMemos();
	writeMemo("[i] bibecli has ended.");
	bpDetach();
	return 0;
}
