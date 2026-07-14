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

static int	decodeSegmentHeader(unsigned char **cursor,
			unsigned int *unparsedBytes, uvast *transferId,
			uvast *sourceBundleLength, uvast *offset)
{
	if (cbor_decode_integer(transferId, CborAny, cursor, unparsedBytes) < 1)
	{
		writeMemo("[?] BIBE can't decode BPDU transfer ID.");
		oK(sdr_end_xn(sdr));
		return -1;
	}

	if (cbor_decode_integer(sourceBundleLength, CborAny, cursor,
			unparsedBytes) < 1)
	{
		writeMemo("[?] BIBE can't decode length of source bundle.");
		oK(sdr_end_xn(sdr));
		return -1;
	}

	if (cbor_decode_integer(offset, CborAny, cursor, unparsedBytes) < 1)
	{
		writeMemo("[?] BIBE can't decode BPDU segment offset.");
		oK(sdr_end_xn(sdr));
		return -1;
	}

	return 0;
}

static int	stripBpduHeader(Object bpduZco, uvast *transferId,
			uvast *sourceBundleLength, uvast *offset,
			uvast *segmentLength)
{
	Sdr		sdr = getIonsdr();
	vast		bpduLength;
	ZcoReader	reader;
	unsigned char	headerBuf[18];
	unsigned int	bytesToParse;
	unsigned char	*cursor;
	unsigned int	unparsedBytes;
	uvast		uvtemp;
	vast		headerLength;

	/*	The payload of a BIBE bundle is a BIBE Protocol Data
	 *	Unit (BPDU), a message that encapsulates a serialized
	 *	bundle (a ZCO).  To access the encapsulated serialized
	 *	bundle in a BPDU we have to strip off the message
	 *	header.							*/

	bpduLength = zco_source_data_length(sdr, bpduZco);
	CHKERR(sdr_begin_xn(sdr));

	/*	Read and strip off the BPDU message's header:
	 *	-	array open (up to 9 bytes)
	 *	-	possible transfer ID (up to 9 bytes)
	 *	-	possible source bundle length (up to 9 bytes)
	 *	-	possible segment offset (up to 9 bytes)
	 *	-	byte string tag (up to 9 bytes) preceding the
	 *		encapsulated ZCO.				*/

	zco_start_receiving(bpduZco, &reader);
	bytesToParse = zco_receive_source(sdr, &reader, 45, (char *) headerBuf);
	if (bytesToParse < 4)
	{
		writeMemo("[?] bcli can't receive BPDU header.");
		oK(sdr_end_xn(sdr));
		return 0;
	}

	cursor = headerBuf;
	unparsedBytes = bytesToParse;
	uvtemp = 0;	/*	Decode array of size 1 or 4.		*/
	if (cbor_decode_array_open(&uvtemp, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] BIBE can't decode BPDU array open.");
		oK(sdr_end_xn(sdr));
		return 0;
	}

	switch (uvtemp)	
	{
	case 1:				/*	A simple BPDU.		*/
		*transferId = 0;
		*sourceBundleLength = 0;
		*offset = 0;
		break;

	case 4:				/*	A segment BPDU.		*/
		if (decodeSegmentHeader(&cursor, &unparsedBytes,
				transferId, sourceBundleLength, offset) < 0)
		{
			writeMemo("[?] Unintelligible BPDU segment header.");
			oK(sdr_end_xn(sdr));
			return 0;
		}

		break;

	default:
		writeMemoNote("[?] Invalid BPDU array length ", itoa(uvtemp));
		oK(sdr_end_xn(sdr));
		return 0;
	}

	uvtemp = (uvast) -1;	/*	Decode fixed-length byte string.*/
	if (cbor_decode_byte_string(NULL, &uvtemp, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] BIBE can't decode BPDU encapsulated bundle.");
		oK(sdr_end_xn(sdr));
		return 0;
	}

	*segmentLength = uvtemp;
	headerLength = cursor - headerBuf;
	if ((*segmentLength + headerLength) != bpduLength)
	{
		writeMemo("[?] BIBE encoding of BPDU payload is invalid.");
		writeMemoNote("[?]     bpduLength  ", itoa(bpduLength));
		writeMemoNote("[?]     headerLength", itoa(headerLength));
		writeMemoNote("[?]     segmentLength", itoa(*segmentLength));
		oK(sdr_end_xn(sdr));
		return 0;
	}

	/*	Now strip off the BPDU header, leaving just the
	 *	encapsulated serialized bundle.				*/

	zco_delimit_source(sdr, bpduZco, headerLength, *segmentLength);
	zco_strip(sdr, bpduZco);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Failed stripping header of BPDU.", NULL);
		return -1;
	}

	return 1;
}

static Object	getTransfer(BpDelivery *dlv, uvast transferId,
			uvast sourceBundleLength, BibeTransfer *transfer) 
{
	Sdr		sdr = getIonsdr();
	BpDB		*bpdb = getBpConstants();
	Object		transferElt;
	Object		obj;
	char		sourceEid[SDRSTRING_BUFSZ];
	uvast		bibeTimeLimit;
	unsigned int	timeLimit;
	BpEvent		event;

	/*	Search for the cited transfer.  Transfers are listed
	 *	in ascending transfer ID order; among transfers with
	 *	the same transfer ID, transfers are distinguished by
	 *	source endpoint ID.					*/

	obj = 0;			/*	Transfer not found yet.	*/
	for (transferElt = sdr_list_last(sdr, bpdb->bibeTransfers); transferElt;
			transferElt = sdr_list_prev(sdr, transferElt))
	{
		obj = sdr_list_data(sdr, transferElt);
		sdr_stage(sdr, (char *) transfer, obj, sizeof(BibeTransfer));
		if (transfer->transferId > transferId)
		{
			continue;	/*	Keep looking.		*/
		}

		if (transfer->transferId < transferId)
		{
			/*	New transfer; insert after this one.	*/

			obj = 0;	/*	Indicates "not found."	*/
			break;
		}

		/*	Transfer IDs match.  Same source?		*/

		if (sdr_string_read(sdr, sourceEid, transfer->source) < 0)
		{
			/*	Invalid transfer.  Clean up.		*/

			bibeDeleteTransfer(sdr, obj);
			sdr_list_delete(sdr, transferElt, NULL, NULL);
			continue;
		}

		if (strcmp(sourceEid, dlv->bundleSourceEid) == 0)
		{
			break;		/*	Found the transfer.	*/
		}

		obj = 0;		/*	Transfer not found yet.	*/
	}

	/*	Finished searching for the cited transfer.		*/

	if (obj)			/*	Found it.		*/
	{
		return transferElt;
	}

	/*	Must note new transfer.					*/

	transfer->totalLength = sourceBundleLength;
	transfer->acquiredLength = 0;
	transfer->transferId = transferId;

	/*	Reassembly time limit is restricted, for security.	*/

	timeLimit = dlv->timeToLive;
	bibeTimeLimit = sdr_list_user_data(sdr, bpdb->bibeTransfers);
	if (timeLimit > bibeTimeLimit)
	{
		timeLimit = bibeTimeLimit;
	}

	transfer->expirationTime = getCtime() + timeLimit;
	transfer->source = sdr_string_create(sdr, dlv->bundleSourceEid);
	if (transfer->source == 0)
	{
		putErrmsg("Can't store transfer source EID.", NULL);
		return -1;
	}

	transfer->segments = sdr_list_create(sdr);
	if (transfer->segments == 0)
	{
		sdr_free(sdr, transfer->source);
		putErrmsg("Can't create segments list for transfer.", NULL);
		return -1;
	}

	obj = sdr_malloc(sdr, sizeof(BibeTransfer));
	if (obj == 0)
	{
		sdr_list_destroy(sdr, transfer->segments, NULL, NULL);
		sdr_free(sdr, transfer->source);
		putErrmsg("Can't create new transfer object.", NULL);
		return -1;
	}

	if (transferElt)		/*	Insert after this one.	*/
	{
		transferElt = sdr_list_insert_after(sdr, transferElt, obj);
	}
	else			/*	Insert at end of list.	*/
	{
		transferElt = sdr_list_insert_last(sdr, bpdb->bibeTransfers,
				obj);
	}

	if (transferElt == 0)
	{
		sdr_free(sdr, obj);
		sdr_list_destroy(sdr, transfer->segments, NULL, NULL);
		sdr_free(sdr, transfer->source);
		putErrmsg("Can't record new transfer.", NULL);
		return -1;
	}

	event.type = reassemblyOverdue;
	event.time = transfer->expirationTime;
	event.ref = transferElt;
	transfer->timelineElt = insertBpTimelineEvent(&event);
	if (transfer->timelineElt == 0)
	{
		sdr_list_delete(sdr, transferElt, NULL, NULL);
		sdr_free(sdr, obj);
		sdr_list_destroy(sdr, transfer->segments, NULL, NULL);
		sdr_free(sdr, transfer->source);
		putErrmsg("Can't set reassembly deadline.", NULL);
		return -1;
	}

	/*	New transfer has been noted.				*/

	sdr_write(sdr, obj, (char *) transfer, sizeof(BibeTransfer));
	return transferElt;
}

static int	acquireSourceBundle(Object sourceBundleZco, VInduct *vinduct)
{
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

	if (bpLoadAcq(work, sourceBundleZco) < 0)
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

static int	handleSegment(BpDelivery *dlv, VInduct *vinduct, Object bpduZco,
			uvast transferId, uvast sourceBundleLength,
			uvast offset, uvast segmentLength)
{
	Sdr		sdr = getIonsdr();
	BpDB		*bpdb = getBpConstants();
	Object		transferElt;
	BibeTransfer	transfer;
	Object		elt;
	Object		obj;
	uvast		endOfSegment;
	BibeSegment	segment;
	uvast		endOfPrevious;
	uvast		offsetOfNext;
	Object		sourceBundleZco;

	/*	First, get the transfer to which this segment applies.	*/

	CHKERR(sdr_begin_xn(sdr));
	transferElt = getTransfer(dlv, transferId, sourceBundleLength,
			&transfer);
	if (transferElt == 0)
	{
		oK(sdr_end_xn(sdr));
		return -1;
	}

	/*	Have got the transfer to insert segment into.  Now
	 *	insert the segment into the right location in the
	 *	transfer.  Segments within a transfer are listed
	 *	in ascending offset order.				*/

	endOfSegment = offset + segmentLength;
	offsetOfNext = 0;
	endOfPrevious = 0;
	for (elt = sdr_list_last(sdr, transfer.segments); elt;
			elt = sdr_list_prev(sdr, elt))
	{
		obj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &segment, obj, sizeof(BibeSegment));
		if (offset == segment.offset)
		{
			writeMemoNote("[?] Duplicate BIBE segment",
					itoa(offset));
			zco_destroy(sdr, bpduZco);
			oK(sdr_end_xn(sdr));
			return 0;
		}

		endOfPrevious = segment.offset + segment.length;
		if (offset >= endOfPrevious)
		{
			break;		/*	Here's where it goes.	*/
		}

		/*	Check the predecessor to this segment.		*/

		offsetOfNext = segment.offset;
		endOfPrevious = 0;
	}

	if (sourceBundleLength != transfer.totalLength)
	{
		writeMemoNote("[?] Conflicting BIBE bundle lengths",
			itoa(transfer.totalLength - sourceBundleLength));
		zco_destroy(sdr, bpduZco);
		oK(sdr_end_xn(sdr));
		return 0;
	}

	/*	Have found this segment's location in the source
	 *	bundle; insert the segment into this location
	 *	within the transfer's list of segments.			*/

	segment.offset = offset;
	segment.length = segmentLength;
	segment.zco = bpduZco;
	obj = sdr_malloc(sdr, sizeof(BibeSegment));
	if (obj == 0)
	{
		putErrmsg("Can't create new segment.", NULL);
		oK(sdr_end_xn(sdr));
		return -1;
	}

	sdr_write(sdr, obj, (char *) segment, sizeof(BibeSegment));
	if (offsetOfNext == 0)		/*	No successor in list.	*/
	{
		elt = sdr_list_insert_last(sdr, transfer.segments, obj);
	}
	else				/*	Have a successor.	*/
	{
		if (endOfSegment > offsetOfNext)
		{
			sdr_free(sdr, obj);
			writeMemoNote("[?] Overlapping BIBE segment",
					itoa(endOfSegment - offsetOfNext));
			zco_destroy(sdr, bpduZco);
			oK(sdr_end_xn(sdr));
			return 0;
		}

		/*	Insert segment after predecessor, if any.	*/

		if (endOfPrevious == 0)	/*	No predecessor in list.	*/
		{
			elt = sdr_list_insert_first(sdr, transfer.segments,
					obj);
		}
		else
		{
			elt = sdr_list_insert_after(sdr, elt, obj);
		}
	}

	if (elt == 0)
	{
		sdr_free(sdr, obj);
		putErrmsg("Can't insert new segment.", NULL);
		oK(sdr_end_xn(sdr));
		return -1;
	}

	/*	Now check to see if transfer is complete.		*/

	transfer.acquiredLength += segmentLength;
	if (transfer.acquiredLength < totalLength)
	{
		/*	Not all segments of the source bundle
		 *	have been received yet.  Note progress
		 *	and wrap up.					*/

		sdr_write(sdr, obj, (char *) &transfer, sizeof(BibeTransfer));
		oK(sdr_end_xn(sdr));
		return 0;
	}

	/*	All segments of source bundle have been received.
	 *	Reassemble and receive that bundle.			*/

	elt = sdr_list_first(sdr, transfer.segments); 
	obj = sdr_list_data(sdr, elt);
	sdr_read(sdr, (char *) &segment, obj, sizeof(BibeSegment));
	sourceBundleZco = zco_clone(sdr, segment.segmentZco, 0, segment.length);
	if (sourceBundleZco == 0)
	{
		putErrmsg("Can't clone first segment.", NULL);
		oK(sdr_end_xn(sdr));
		return -1;
	}

	elt = sdr_list_next(sdr, elt);
	while (elt)
	{
		obj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &segment, obj, sizeof(BibeSegment));
		if (zco_clone_source_data(sdr, sourceBundleZco,
				segment.segmentZco, 0, segment.length)
				!= segment.length)
		{
			putErrmsg("Can't clone segment data.", NULL);
			zco_destroy(sdr, sourceBundleZco);
			oK(sdr_end_xn(sdr));
			return -1;
		}

		elt = sdr_list_next(sdr, elt);
	}

	zco_bond(sdr, sourceBundleZco);
	if (acquireSourceBundle(bpduZco, vinduct) < 0)
	{
		putErrmsg("Can't acquire reassembled bundle.", NULL);
		zco_destroy(sdr, sourceBundleZco);
		oK(sdr_end_xn(sdr));
		return -1;
	}

	/*	Source bundle has been acquired and dispatched.
	 *	This transfer is no longer needed.			*/

	bibeDeleteTransfer(sdr, sdr_list_data(sdr, transferElt));
	sdr_list_delete(sdr, transferElt, NULL, NULL);
	oK(sdr_end_xn(sdr));
	return 0;
}

static int	handleBpdu(BpDelivery *dlv, VInduct *vinduct)
{
	Sdr		sdr = getIonsdr();
	Object		bpduZco;
	uvast		transferId;
	uvast		sourceBundleLength;
	uvast		offset;
	uvast		segmentLength;
	AcqWorkArea	*work;

	/*	The ADU in the dlv structure is the ZCO representation
	 *	of the *payload* of a bundle sent by BIBE.  As such,
	 *	it is a BPDU, i.e., a structure that encapsulates a
	 *	bundle that is to be dispatched.			*/

	bpduZco = dlv->adu;
	CHKERR(sdr_begin_xn(sdr));
	switch (stripBpduHeader(bpduZco, &transferId, &sourceBundleLength,
			&offset, &segmentLength))
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

	if (sourceBundleLength > 0)	/*	It's a segment BPDU.	*/
	{
		return handleSegment(dlv, vinduct, bpduZco, transferId,
				sourceBundleLength, offset, segmentLength);
	}

	/*	Simple BPDU: acquire & dispatch the encapsulated bundle.*/

	return acquireSourceBundle(bpduZco, vinduct);
}

static void	*handleBibeBundles(void *parm)
{
	ReceiverThreadParms	*parms = (ReceiverThreadParms *) parm;
	int			*running = parms->running;
	BpSAP			sap = parms->sap;
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

		if (handleBpdu(&dlv, parms->vinduct) < 0)
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
