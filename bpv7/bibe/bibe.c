/*

	bibe.c:	API for bundle-in-bundle encapsulation.

	Author:	Scott Burleigh, JPL

	Copyright (c) 2019, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.

									*/
#include "bibeP.h"

static void	getSchemeName(char *eid, *schemNameBuf)
{
	MetaEid		meid;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;

	if (parseEidString(eid, &meid, &vscheme, &vschemeElt) == 0)
	{
		*schemNameBuf = '\0';
	}
	else
	{
		istrcpy(schemeNameBuf, meid.schemeName, MAX_SCHEME_NAME_LEN);
		restoreEidString(&meid);
	}
}

void	bibeAdd(char *destEid, unsigned int fwdLatency, unsigned int rtnLatency,
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

	bibeFind(destEid, &bclaAddr, &bclaElt);
	if (bclaElt)
	{
		writeMemoNote("[?] Duplicate BIBE CLA", destEid);
		return;
	}

	getSchemeName(destEid, schemeName);
	findScheme(schemeName, &vsheme, &vschemeElt);
	sdr_read(sdr, (char *) &scheme, sdr_list_data(sdr, vscheme->schemeElt),
			sizeof(Scheme));
	CHKERR(sdr_begin_xn(sdr));
	memset((char *) &bcla, 0, sizeof(Bcla));
	bcla.dest = sdr_string_create(sdr, destEid);
	bcla.ctis = sdr_list_create(sdr);
	bcla.signals[CT_ACCEPTED].deadline = (time_t) -1;
	bcla.signals[CT_ACCEPTED].sequences = sdr_list_create(sdr);
	bcla.fwdLatency = fwdLatency;
	bcla.rtnLatency = rtnLatency;
	bcla.classOfService = priority;
	if (flags & BP_DATA_LABEL_PRESENT)
	{
		bcla.ancillaryData.dataLabel = label;
	}

	bcla.ancillaryData.flags = flags;
	bcla.ancillaryData.ordinal = ordinal;
	bclaAddr = sdr_malloc(sdr, sizeof(Bcla));
	sdr_write(sdr, bclaAddr, (char *) &bcla, sizeof(Bcla));
	oK(sdr_list_insert_last(sdr, scheme.bclas, bclaAddr));
	oK(sdr_end_xn(sdr));
}

void	bibechange(char *destEid, unsigned int fwdLatency,
		unsigned int rtnLatency, unsigned char priority,
		unsigned char ordinal, unsigned int label,
		unsigned char flags)
{
	Sdr		sdr = getIonsdr();
	Object		bclaAddr;
	Object		bclaElt;
	char		schemeName[MAX_SCHEME_NAME_LEN + 1];
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	Scheme		scheme;
	Bcla		bcla;

	bibeFind(destEid, &bclaAddr, &bclaElt);
	if (bclaElt == 0)
	{
		writeMemoNote("[?] Can't find BIBE CLA to change", destEid);
		return;
	}

	getSchemeName(destEid, schemeName);
	findScheme(schemeName, &vsheme, &vschemeElt);
	sdr_read(sdr, (char *) &scheme, sdr_list_data(sdr, vscheme->schemeElt),
			sizeof(Scheme));
	CHKERR(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &bcla, bclaAddr, sizeof(Bcla));
	sdr_free(sdr, bcla.dest);
	bcla.dest = sdr_string_create(sdr, destEid);
	bcla.fwdLatency = fwdLatency;
	bcla.rtnLatency = rtnLatency;
	bcla.classOfService = priority;
	if (flags & BP_DATA_LABEL_PRESENT)
	{
		bcla.ancillaryData.dataLabel = label;
	}

	bcla.ancillaryData.flags = flags;
	bcla.ancillaryData.ordinal = ordinal;
	sdr_write(sdr, bclaAddr, (char *) &bcla, sizeof(Bcla));
	oK(sdr_end_xn(sdr));
}

void	bibeDelete(char *destEid)
{
	Sdr	sdr = getIonsdr();
	Object	bclaAddr;
	Object	bclaElt;
	Bcla	bcla;
	Object	elt;
	Object	addr;
	Cti	cti;
	Bundle	bundle;

	bibeFind(destEid, &bclaAddr, &bclaElt);
	if (bclaElt == 0)
	{
		writeMemoNote("[?] Can't find BIBE CLA to delete", destEid);
		return;
	}

	CHKERR(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &bcla, bclaAddr, sizeof(Bcla));
	sdr_free(sdr, bcla.dest);
	while ((elt = sdr_list_first(sdr, bcla.ctis)))
	{
		addr = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &cti, addr, sizeof(Cti));

		/*	Remove CT timeout event from bundle.		*/

		sdr_read(sdr, (char *) &Bundle, cti.bundle, sizeof(Bundle));
		destroyBpTImelineEvent(bundle.ctDueElt);
		bundle.ctDueElt = 0;
		sdr_write(sdr, cti.bundle, (char *) &Bundle, sizeof(Bundle));

		/*	Remove the Cti that the event pointed to.	*/

		sdr_free(sdr, addr);
		sdr_list_delete(sdr, elt, NULL, NULL);
	}

	sdr_list_destroy(sdr, bcla.ctis, NULL, NULL);
	while ((elt = sdr_list_first(sdr, bcla.signals[CT_ACCEPTED].sequences)))
	{
		addr = sdr_list_data(sdr, elt);
		sdr_free(sdr, addr);
		sdr_list_delete(sdr, elt, NULL, NULL);
	}

	sdr_list_destroy(sdr, bcla.signals[CT_ACCEPTED].sequences, NULL, NULL);
	sdr_free(sdr, bclaAddr);
	sdr_list_delete(sdr, bclaElt);
	oK(sdr_end_xn(sdr));
}

void	bibeFind(char *destEid, Object *bclaAddr, Object *bclaElt)
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

	CHKVOID(destEid && bclaAddr && bclaElt);
	*bclaAddr = 0;
	*bclaElt = 0;
	getSchemeName(destEid, schemeName);
	findScheme(schemeName, &vsheme, &vschemeElt);
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

		if (strcmp(dest, destEid) == 0)
		{
			*bclaAddr = addr;
			*bclaElt = elt;
			return;
		}
	}
}

static void	handleCustodyTransfer(Object bclaObj, unsigned int xmitId,
			time_t deadline)
{
	Sdr		sdr = getIonsdr();
	Bcla		bcla;
	CtSignal	*signal;
	Object		elt;
	Object		sequenceAddr;
	CtSequence	sequence;
	Object		elt2;
	Object		sequenceAddr2;
	CtSequence	sequence2;

	sdr_read(sdr, (char *) &bcla, bclaObj, sizeof(Bcla));
	signal = bcla.signals + CT_ACCEPTED;
	if (deadline < signal->deadline)
	{
		signal->deadline = deadline;
	}

	for (elt = sdr_list_first(sdr, signal->sequences); elt;
			elt = sdr_list_next(sdr, elt))
	{
		sequenceAddr = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &sequence, sequenceAddr,
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

			return;
		}

		if (sequence.firstXmitId == xmitId)
		{
			/*	This xmitId is already included in
			 *	this sequence.  Nothing to do.		*/

			return;
		}

		/*	This sequence's first xmitId is greater than
		 *	this xmitId.					*/

		if (sequence.firstXmitId == xmitId + 1)
		{
			/*	Prepend to sequence; will NOT connect
			 *	to the previous sequence.		*/

			sequence.firstXmitId = xmitId;
			break;
		}

		/*	This xmitId is outside of any currently
		 *	managed sequence.				*/

		break;
	}

	if (sequence.firstXmitId == xmitId)
	{
		/*	Prepended to existing sequence.			*/

		sdr_write(sdr, sequenceAddr, (char *) &sequence,
				sizeof(CtSequence));
		return;
	}

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
				sdr_list_delete(sdr, elt2);
			}
		}

		sdr_write(sdr, sequenceAddr, (char *) &sequence,
				sizeof(CtSequence));
		return;
	}

	/*	Must start a new sequence.				*/

	sequence.firstXmitId == xmitId;
	sequence.lastXmitId == xmitId;
	sequenceAddr = sdr_malloc(sdr, sizeof(CtSequence));
	if (sequenceAddr)
	{
		sdr_write(sdr, sequenceAddr, (char *) &sequence,
				sizeof(CtSequence));
		if (elt)
		{
			oK(sdr_list_insert_before(sdr, elt, sequenceAddr));
		}
		else
		{
			oK(sdr_list_insert_last(sdr, signal.sequences,
					sequenceAddr));
		}
	}
}
 
int	bibeHandleBpdu(BpDelivery *dlv)
{
	Sdr		sdr = getIonsdr();
	vast		bpduLength;
	ZcoReader	reader;
	char		headerBuf[19];
	unsigned int	bytesToParse;
	char		*cursor;
	unsigned int	unparsedBytes;
	uvast		uvtemp;
	unsigned int	xmitId;
	time_t		deadline;
	vast		headerLength;
	Object		bclaObj;
	Object		bclaElt;
	char		schemeName[MAX_SCHEME_NAME_LEN + 1];
	VInduct		*vinduct;
	PsmAddress	vinductElt;	
	AcqWorkArea	*work;

	/*	Read and strip off the BPDU header: array open (1
	 *	byte), xmitId (up to 9 bytes), deadline (up to 9
	 *	bytes.							*/

	CHKERR(sdr_begin_xn(sdr));
	bpduLength = zco_source_data_length(sdr, dlv->adu);
	zco_start_receiving(dlv->adu, &reader);
	bytesToParse = zco_receive_source(sdr, &reader, 19, headerBuf);
	if (bytesToParse < 3)
	{
		putErrmsg("Can't receive BPDU header.", NULL);
		oK(sdr_end_xn(sdr));
		return -1;
	}

	cursor = headerBuf;
	unparsedBytes = bytesToParse;
	uvtemp = 3;	/*	Decode array of size 3.			*/
	if (cbor_decode_array_open(&uvtemp, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] bibecli can't decode BPDU array open.");
		oK(sdr_end_xn(sdr));
		return 0;
	}

	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] bibecli can't decode BPDU xmit ID.");
		oK(sdr_end_xn(sdr));
		return 0;
	}

	xmitId = uvtemp;
	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] bibecli can't decode BPDU deadline.");
		oK(sdr_end_xn(sdr));
		return 0;
	}

	deadline = uvtemp;

	/*	Now strip off the BPDU header, leaving just the
	 *	encapsulated serialized bundle.				*/

	headerLength = cursor - headerBuf;
	zco_delimit_source(sdr, dlv->adu, headerLength,
			bpduLength - headerLength);
	zco_strip(sdr, dlv->adu);

	/*	Do custody transfer processing if necessary.		*/

	if (xmitId > 0)
	{
		bibeFind(dlv->bundleSourceEid, &bclaObj, &bclaElt);
		if (bclaElt)
		{
			handleCustodyTransfer(bclaObj, xmitId, deadline);
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

	getSchemeName(dlv->bundleSourceEid, schemeName);
	findInduct("bibe", schemeName, &vinduct, &vinductElt);
	if (vinductElt == 0)
	{
		putErrmsg("Can't get BIBE Induct", dlv->bundleSourceEid);
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

	if (bpLoadAcq(work, dlv->adu) < 0)
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
	return 0;
}
 
int	bibeHandleTimeout(Object ctDueElt)
{
}
 
void	bibeCancelCti(Object ctDueElt)
{
}
