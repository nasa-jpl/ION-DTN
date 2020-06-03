/*

	bibe.c:	API for bundle-in-bundle encapsulation.

	Author:	Scott Burleigh, JPL

	Copyright (c) 2019, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.

									*/
#include "bibeP.h"

static void	getSchemeName(char *eid, char *schemeNameBuf)
{
	MetaEid		meid;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;

	if (parseEidString(eid, &meid, &vscheme, &vschemeElt) == 0)
	{
		*schemeNameBuf = '\0';
	}
	else
	{
		istrcpy(schemeNameBuf, meid.schemeName, MAX_SCHEME_NAME_LEN);
		restoreEidString(&meid);
	}
}

void	bibeAdd(char *peerEid, unsigned int fwdLatency,
		unsigned int rtnLatency, int lifespan,
		unsigned char priority, unsigned char ordinal,
		unsigned int label, unsigned char flags)
{
	Sdr		sdr = getIonsdr();
	Object		bclaAddr;
	Object		bclaElt;
	char		schemeName[MAX_SCHEME_NAME_LEN + 1];
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	Scheme		scheme;
	Bcla		bcla;

	bibeFind(peerEid, &bclaAddr, &bclaElt);
	if (bclaElt)
	{
		writeMemoNote("[?] Duplicate BIBE CLA", peerEid);
		return;
	}

	getSchemeName(peerEid, schemeName);
	if (schemeName[0] == '\0')
	{
		writeMemoNote("[?] No scheme name in bcla ID", peerEid);
		return;
	}

	findScheme(schemeName, &vscheme, &vschemeElt);
	if (vscheme == NULL)
	{
		writeMemoNote("[?] bcla ID scheme name unknown", peerEid);
		return;
	}

	sdr_read(sdr, (char *) &scheme, sdr_list_data(sdr, vscheme->schemeElt),
			sizeof(Scheme));
	CHKVOID(sdr_begin_xn(sdr));
	memset((char *) &bcla, 0, sizeof(Bcla));
	bcla.source = sdr_string_create(sdr, vscheme->adminEid);
	bcla.dest = sdr_string_create(sdr, peerEid);
	bcla.bpdus = sdr_list_create(sdr);
	bcla.signals[CT_ACCEPTED].deadline = (time_t) MAX_TIME;
	bcla.signals[CT_ACCEPTED].sequences = sdr_list_create(sdr);
	bcla.fwdLatency = fwdLatency;
	bcla.rtnLatency = rtnLatency;
	bcla.lifespan = lifespan;
	bcla.classOfService = priority;
	if (flags & BP_DATA_LABEL_PRESENT)
	{
		bcla.ancillaryData.dataLabel = label;
	}

	bcla.ancillaryData.flags = flags;
	bcla.ancillaryData.ordinal = ordinal;
	bclaAddr = sdr_malloc(sdr, sizeof(Bcla));
	if (bclaAddr)
	{
		sdr_write(sdr, bclaAddr, (char *) &bcla, sizeof(Bcla));
		oK(sdr_list_insert_last(sdr, scheme.bclas, bclaAddr));
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Failed adding bcla.", peerEid);
	}
}

void	bibeChange(char *peerEid, unsigned int fwdLatency,
		unsigned int rtnLatency, int lifespan,
		unsigned char priority, unsigned char ordinal,
		unsigned int label, unsigned char flags)
{
	Sdr		sdr = getIonsdr();
	Object		bclaAddr;
	Object		bclaElt;
	Bcla		bcla;

	bibeFind(peerEid, &bclaAddr, &bclaElt);
	if (bclaElt == 0)
	{
		writeMemoNote("[?] Can't find BIBE CLA to change", peerEid);
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &bcla, bclaAddr, sizeof(Bcla));
	bcla.fwdLatency = fwdLatency;
	bcla.rtnLatency = rtnLatency;
	bcla.lifespan = lifespan;
	bcla.classOfService = priority;
	if (flags & BP_DATA_LABEL_PRESENT)
	{
		bcla.ancillaryData.dataLabel = label;
	}

	bcla.ancillaryData.flags = flags;
	bcla.ancillaryData.ordinal = ordinal;
	sdr_write(sdr, bclaAddr, (char *) &bcla, sizeof(Bcla));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Failed changing bcla.", peerEid);
	}
}

void	bibeDelete(char *peerEid)
{
	Sdr	sdr = getIonsdr();
	Object	bclaAddr;
	Object	bclaElt;
	Bcla	bcla;
	Object	elt;
	Object	addr;
	Bpdu	bpdu;

	bibeFind(peerEid, &bclaAddr, &bclaElt);
	if (bclaElt == 0)
	{
		writeMemoNote("[?] Can't find BIBE CLA to delete", peerEid);
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &bcla, bclaAddr, sizeof(Bcla));
	sdr_free(sdr, bcla.dest);
	while ((elt = sdr_list_first(sdr, bcla.bpdus)))
	{
		addr = sdr_list_data(sdr, elt);

		/*	Custodial BIBE for this bundle has failed.	*/

		sdr_list_delete(sdr, elt, NULL, NULL);
		sdr_read(sdr, (char *) &bpdu, addr, sizeof(Bpdu));
		sdr_free(sdr, addr);
		oK(bpHandleXmitFailure(bpdu.bundleZco));
	}

	sdr_list_destroy(sdr, bcla.bpdus, NULL, NULL);
	while ((elt = sdr_list_first(sdr, bcla.signals[CT_ACCEPTED].sequences)))
	{
		addr = sdr_list_data(sdr, elt);
		sdr_free(sdr, addr);
		sdr_list_delete(sdr, elt, NULL, NULL);
	}

	sdr_list_destroy(sdr, bcla.signals[CT_ACCEPTED].sequences, NULL, NULL);
	sdr_free(sdr, bclaAddr);
	sdr_list_delete(sdr, bclaElt, NULL, NULL);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Failed deleting bcla.", peerEid);
	}
}

void	bibeFind(char *peerEid, Object *bclaAddr, Object *bclaElt)
{
	Sdr		sdr = getIonsdr();
	char		schemeName[MAX_SCHEME_NAME_LEN + 1];
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	Scheme		scheme;
	Object		elt;
	Object		addr;
			OBJ_POINTER(Bcla, bcla);
	char		dest[SDRSTRING_BUFSZ];

	CHKVOID(peerEid && bclaAddr && bclaElt);
	*bclaAddr = 0;
	*bclaElt = 0;
	getSchemeName(peerEid, schemeName);
	findScheme(schemeName, &vscheme, &vschemeElt);
	if (vscheme == NULL)
	{
		return;
	}

	sdr_read(sdr, (char *) &scheme, sdr_list_data(sdr, vscheme->schemeElt),
			sizeof(Scheme));
	if (scheme.bclas == 0)
	{
		return;
	}

	oK(sdr_begin_xn(sdr));
	for (elt = sdr_list_first(sdr, scheme.bclas); elt;
			elt = sdr_list_next(sdr, elt))
	{
		addr = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, Bcla, bcla, addr);
		CHKVOID(bcla);
		if (sdr_string_read(sdr, dest, bcla->dest) < 0)
		{
			continue;
		}

		if (strcmp(dest, peerEid) == 0)
		{
			*bclaAddr = addr;
			*bclaElt = elt;
			break;
		}

	}

	sdr_exit_xn(sdr);
}

static int	stripBpduHeader(Object bpduZco, unsigned int *xmitId,
			time_t *deadline)
{
	Sdr		sdr = getIonsdr();
	vast		bpduLength;
	ZcoReader	reader;
	unsigned char	headerBuf[36];
	unsigned int	bytesToParse;
	unsigned char	*cursor;
	unsigned int	unparsedBytes;
	uvast		uvtemp;
	vast		bundleLength;
	vast		headerLength;

	/*	The payload of a BIBE bundle is a BIBE Protocol Data
	 *	Unit (BPDU), an administrative record that encapsulates
	 *	a BPDU message that in turn encapsulates a serialized
	 *	bundle (a ZCO).  To access the encapsulated serialized
	 *	bundle in a BPDU message we have to strip off the
	 *	BPDU message header.					*/

	bpduLength = zco_source_data_length(sdr, bpduZco);
	CHKERR(sdr_begin_xn(sdr));

	/*	Read and strip off the BPDU message's header:
	 *	3-element array open (up to 9 bytes), xmitId (up
	 *	to 9 bytes), deadline (up to 9 bytes), byte string
	 *	tag (up to 9 bytes) preceding the encapsulated ZCO.	*/

	zco_start_receiving(bpduZco, &reader);
	bytesToParse = zco_receive_source(sdr, &reader, 36, (char *) headerBuf);
	if (bytesToParse < 4)
	{
		putErrmsg("Can't receive BPDU header.", NULL);
		oK(sdr_end_xn(sdr));
		return 0;
	}

	cursor = headerBuf;
	unparsedBytes = bytesToParse;
	uvtemp = 3;	/*	Decode array of size 3.			*/
	if (cbor_decode_array_open(&uvtemp, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] BIBE can't decode BPDU array open.");
		oK(sdr_end_xn(sdr));
		return 0;
	}

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
	uvtemp = (uvast) -1;
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

static int	handleCustodyTransfer(Object bclaObj, unsigned int xmitId,
			time_t deadline)
{
	Sdr		sdr = getIonsdr();
	Bcla		bcla;
	CtSignal	*signal;
	Object		elt;
	Object		sequenceAddr = 0;
	CtSequence	sequence;
	Object		elt2;
	Object		sequenceAddr2;
	CtSequence	sequence2;

	if ((getBpVdb())->watching & WATCH_w)
	{
		iwatch('w');
	}

	memset((char *) &sequence, 0, sizeof(CtSequence));
	sdr_stage(sdr, (char *) &bcla, bclaObj, sizeof(Bcla));
	signal = bcla.signals + CT_ACCEPTED;
	if (deadline < signal->deadline)
	{
		signal->deadline = deadline;
		sdr_write(sdr, bclaObj, (char *) &bcla, sizeof(Bcla));
	}

	for (elt = sdr_list_first(sdr, signal->sequences); elt;
			elt = sdr_list_next(sdr, elt))
	{
		sequenceAddr = sdr_list_data(sdr, elt);
		sdr_stage(sdr, (char *) &sequence, sequenceAddr,
				sizeof(CtSequence));
		if (sequence.firstXmitId < xmitId)
		{
			if (sequence.lastXmitId < xmitId)
			{
				if (sequence.lastXmitId == xmitId - 1)
				{
					/*	Append to sequence;
					 *	may connect to the
					 *	next sequence.		*/

					sequence.lastXmitId = xmitId;
					break;
				}

				/*	This xmitId belongs to a later
				 *	sequence.			*/

				continue;
			}

			/*	xmitID <= sequence.lastXmitId, so
			 *	it's already included in this sequence.
			 *	Nothing to do.				*/

			return 0;
		}

		if (sequence.firstXmitId == xmitId)
		{
			/*	This xmitId is already included in
			 *	this sequence.  Nothing to do.		*/

			return 0;
		}

		/*	This sequence's first xmitId is greater than
		 *	this xmitId.					*/

		if (sequence.firstXmitId == xmitId + 1)
		{
			/*	Prepend to sequence; will NOT connect
			 *	to the previous sequence.		*/

			sequence.firstXmitId = xmitId;
			sdr_write(sdr, sequenceAddr, (char *) &sequence,
					sizeof(CtSequence));
			return 0;
		}

		/*	This xmitId is outside of any currently
		 *	managed sequence.				*/

		break;
	}

	if (elt == 0)
	{
		/*	Must start a new sequence at end of list.	*/

		sequence.firstXmitId = xmitId;
		sequence.lastXmitId = xmitId;
		sequenceAddr = sdr_malloc(sdr, sizeof(CtSequence));
		if (sequenceAddr == 0)
		{
			putErrmsg("Can't allocate new CT sequence.", NULL);
			return -1;
		}

		sdr_write(sdr, sequenceAddr, (char *) &sequence,
				sizeof(CtSequence));
		if (sdr_list_insert_last(sdr, signal->sequences, sequenceAddr)
				== 0)
		{
			putErrmsg("Can't insert new CT sequence.", NULL);
			return -1;
		}

		return 0;
	}

	/*	This xmitId may have been appended to an existing
	 *	sequence.						*/

	if (sequence.lastXmitId == xmitId)
	{
		/*	Appended to existing sequence, might need
		 *	to merge with next sequence.			*/

		elt2 = sdr_list_next(sdr, elt);
		if (elt2)
		{
			sequenceAddr2 = sdr_list_data(sdr, elt2);
			sdr_read(sdr, (char *) &sequence2, sequenceAddr2,
					sizeof(CtSequence));
			if (sequence2.firstXmitId == sequence.lastXmitId)
			{
				sequence.lastXmitId = sequence2.lastXmitId;
				sdr_free(sdr, sequenceAddr2);
				sdr_list_delete(sdr, elt2, NULL, NULL);
			}
		}

		sdr_write(sdr, sequenceAddr, (char *) &sequence,
				sizeof(CtSequence));
		return 0;
	}

	/*	Must start a new sequence before the last sequence
	 *	visited.						*/

	sequence.firstXmitId = xmitId;
	sequence.lastXmitId = xmitId;
	sequenceAddr = sdr_malloc(sdr, sizeof(CtSequence));
	if (sequenceAddr == 0)
	{
		putErrmsg("Can't allocate new CT sequence.", NULL);
		return -1;
	}

	sdr_write(sdr, sequenceAddr, (char *) &sequence, sizeof(CtSequence));
	if (sdr_list_insert_before(sdr, elt, sequenceAddr) == 0)
	{
		putErrmsg("Can't insert new CT sequence.", NULL);
		return -1;
	}

	return 0;
}

int	bibeHandleBpdu(BpDelivery *dlv)
{
	Sdr		sdr = getIonsdr();
	Object		bpduZco;
	unsigned int	xmitId;
	time_t		deadline;
	Object		bclaObj;
	Object		bclaElt;
	VInduct		*vinduct;
	PsmAddress	vinductElt;	
	AcqWorkArea	*work;

	/*	The ADU in the dlv structure is the ZCO representation
	 *	of the *payload* of a bundle sent by BIBE.  As such,
	 *	it is a BPDU, i.e., an administrative record that
	 *	encapsulates a BPDU message, which itself encapsulates
	 *	a bundle that is to be dispatched.			*/

	bpduZco = dlv->adu;
	CHKERR(sdr_begin_xn(sdr));
	switch (stripBpduHeader(bpduZco, &xmitId, &deadline))
	{
	case -1:
		putErrmsg("Can't strip BPDU header.", NULL);
		oK(sdr_end_xn(sdr));
		return -1;

	case 0:
		writeMemo("[?] bibecli can't process BPDU.");
		oK(sdr_end_xn(sdr));
		return 0;
	}

	/*	Do custody transfer processing if necessary.		*/

	if (xmitId > 0)
	{
		bibeFind(dlv->bundleSourceEid, &bclaObj, &bclaElt);
		if (bclaElt)
		{
			if (handleCustodyTransfer(bclaObj, xmitId, deadline)
					< 0)
			{
				putErrmsg("Can't handle custody transfer.",
						NULL);
				oK(sdr_cancel_xn(sdr));
				return -1;
			}
		}
		else
		{
			writeMemoNote("No such BIBE duct; no custody transfer.",
					dlv->bundleSourceEid);
		}
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't handle BPDU.", NULL);
		return -1;
	}

	/*	Now acquire and dispatch the encapsulated bundle.	*/

	findInduct("bibe", "*", &vinduct, &vinductElt);
	if (vinductElt == 0)
	{
		putErrmsg("Can't get bibe Induct", "*");
		return -1;
	}

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

int	bibeHandleSignal(BpDelivery *dlv, unsigned char *cursor,
			unsigned int unparsedBytes)
{
	Sdr		sdr = getIonsdr();
	Object		bclaObj;
	Object		bclaElt;
	Bcla		bcla;
	uvast		uvtemp;
	int		reasonCode;
	int		sequenceCount;
	unsigned int	xmitId;
	unsigned int	seqLength;
	Object		elt;
	Object		nextElt;
	Object		bpduObj;
	Bpdu		bpdu;

	bibeFind(dlv->bundleSourceEid, &bclaObj, &bclaElt);
	if (bclaElt == 0)
	{
		writeMemoNote("No such BIBE duct; ignoring custody signal.",
				dlv->bundleSourceEid);
		return 0;
	}

	sdr_read(sdr, (char *) &bcla, bclaObj, sizeof(Bcla));
	CHKERR(sdr_begin_xn(sdr));

	/*	Start parsing signal: get array open.			*/

	uvtemp = 2;	/*	Decode array of size 2.			*/
	if (cbor_decode_array_open(&uvtemp, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] bibecli can't decode BIBE signal array open.");
		oK(sdr_end_xn(sdr));
		return 0;
	}

	/*	First element of array is reason code.			*/

	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] bibecli can't decode BIBE signal reason code.");
		oK(sdr_end_xn(sdr));
		return 0;
	}

	reasonCode = uvtemp;

	/*	Second element of array is disposition scope report,
	 *	a BCOR array: get array open.				*/

	uvtemp = 0;	/*	Decode array of unknown but fixed size.	*/
	if (cbor_decode_array_open(&uvtemp, &cursor, &unparsedBytes) < 1
	|| uvtemp == (uvast) -1)
	{
		writeMemo("[?] bibecli can't decode BIBE signal array open.");
		oK(sdr_end_xn(sdr));
		return 0;
	}

	sequenceCount = uvtemp;	/*	Length of array is now known.	*/

	/*	Parse and process the disposition scope report.		*/

	while (sequenceCount > 0)
	{
		sequenceCount--;

		/*	Each sequence is an array of two elements:
		 *	first xmitId followed by number of consecutive
		 *	xmitIds in the sequence (including the first).	*/

		uvtemp = 2;	/*	Decode array of size 2.		*/
		if (cbor_decode_array_open(&uvtemp, &cursor, &unparsedBytes)
				< 1)
		{
			writeMemo("[?] bibecli can't decode sequence array.");
			oK(sdr_end_xn(sdr));
			return 0;
		}

		/*	Decode start of sequence.			*/

		if (cbor_decode_integer(&uvtemp, CborAny, &cursor,
				&unparsedBytes) < 1)
		{
			writeMemo("[?] bibecli can't decode start of seq.");
			oK(sdr_end_xn(sdr));
			return 0;
		}

		xmitId = uvtemp;	/*	First xmitId in seq.	*/

		/*	Decode length of sequence (number of
		 *	consecutive xmitId acknowledged).		*/

		if (cbor_decode_integer(&uvtemp, CborAny, &cursor,
				&unparsedBytes) < 1)
		{
			writeMemo("[?] bibecli can't decode length of seq.");
			oK(sdr_end_xn(sdr));
			return 0;
		}

		seqLength = uvtemp;
		if (seqLength < 1)
		{
			writeMemo("[?] Invalid BIBE signal sequence length.");
			oK(sdr_end_xn(sdr));
			return 0;
		}

		/*	Acknowledge every CT item in this sequence.	*/

		for (elt = sdr_list_first(sdr, bcla.bpdus); elt;
				elt = nextElt)
		{
			nextElt = sdr_list_next(sdr, elt);

			/*	Get next bpdu that might be
			 *	acknowledged.				*/

			bpduObj = sdr_list_data(sdr, elt);
			sdr_read(sdr, (char *) &bpdu, bpduObj, sizeof(Bpdu));
			if (bpdu.xmitId < xmitId)
			{
				/*	This bundle not acknowledged.	*/
				continue;
			}

			while (xmitId < bpdu.xmitId)
			{
				/*	Acknowledged item is no
				 *	longer in bpdus list, so
				 *	ignore this acknowledgment.	*/
				seqLength -= 1;
				if (seqLength == 0)
				{
					xmitId = 0;	/*	Stop.	*/
					break;
				}

				xmitId += 1;
			}

			if (bpdu.xmitId == xmitId)
			{
				/*	Process custody acknowledgment.	*/

				sdr_list_delete(sdr, elt, NULL, NULL);
				sdr_free(sdr, bpduObj);
				if (reasonCode == CT_ACCEPTED
				|| reasonCode == CT_REDUNDANT)
				{
					if ((getBpVdb())->watching & WATCH_m)
					{
						iwatch('m');
					}

					if (bpHandleXmitSuccess
						(bpdu.bundleZco) < 0)
					{
						putErrmsg("Can't handle xmit \
success.", NULL);
						sdr_cancel_xn(sdr);
						return -1;
					}
				}
				else	/*	Custody refused.	*/
				{
					if ((getBpVdb())->watching
							& WATCH_refusal)
					{
						iwatch('&');
					}

					if (bpHandleXmitFailure
						(bpdu.bundleZco) < 0)
					{
						putErrmsg("Can't handle xmit \
success.", NULL);
						sdr_cancel_xn(sdr);
						return -1;
					}
				}

				/*	Bundle reference dispatched.	*/

				seqLength -= 1;
				xmitId += 1;
			}

			if (seqLength == 0)
			{
				/*	All applicable acknowledgments
				 *	have now been processed; no
				 *	need to look at more bpdus.	*/

				break;
			}
		}

	}

	return sdr_end_xn(sdr);
}
