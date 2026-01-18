/*
 *	cteb.c:		implementation of the extension definition
 *			functions for the Custody Transfer Extension
 *			Block (CTEB).
 *
 *	Per CCSDS Orange Book Draft K: Compressed Bundle Status
 *	Reporting and Custody Signaling.
 *
 *	CTEB block structure (CBOR array, always 3 elements):
 *	  [seq_num, seq_id, custodian_eid]
 *
 *	Copyright (c) 2026, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: ION Development Team
 */

#include "bpP.h"
#include "bei.h"
#include "cteb.h"
#include "cbr.h"

/*	Block processing flags for CTEB per Orange Book.
 *	Bit 0 (0x01): Block must be replicated in every fragment
 *	Bits 2,4 cleared: Allow transparent forwarding at non-supporting nodes
 */
#define CTEB_BLOCK_PROC_FLAGS	0x01

/*	Maximum size of serialized CTEB block data.
 *	array(3) + uint64 + uint64 + eid
 *	Estimate: 1 + 9 + 9 + 82 = ~101 bytes
 */
#define CTEB_MAX_SERIALIZED_LEN	128

/*	*	*	Scratchpad Structure	*	*	*	*/

/*
 * CtebScratchpad holds CTEB data in working memory during block
 * processing. Stored in blk->object for outbound blocks.
 */
typedef struct {
	uvast		seqNum;		/* Bundle sequence number */
	uvast		seqId;		/* Sequence identifier */
	Object		custodianEid;	/* SDR string: current custodian */
} CtebScratchpad;

/*	*	*	Outbound Block Functions	*	*	*/

int	cteb_offer(ExtensionBlock *blk, Bundle *bundle)
{
	Sdr		sdr = getIonsdr();
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	CtebScratchpad	scratch;
	uvast		seqNum;
	char		*sourceEidStr;
	Object		scratchAddr;

	/*	Check if custody transfer is requested for this bundle.
	 *	Per BPv7, custody transfer is indicated by the
	 *	BP_CT_REQUESTED flag (formerly in bundleProcFlags).
	 *
	 *	For now, we check if the bundle is marked for custody
	 *	in the internal custody tracking field.			*/

	if (!(bundle->qosFlags & BP_CT_REQUESTED))
	{
		/*	Custody transfer not requested.			*/
		return 0;
	}

	/*	Get our admin EID as initial custodian.			*/

	findScheme("ipn", &vscheme, &vschemeElt);
	if (vschemeElt == 0)
	{
		putErrmsg("CTEB: can't find ipn scheme for admin EID.", NULL);
		return -1;
	}

	/*	CTEB is needed. Allocate sequence number for custody.	*/

	readEid(&bundle->id.source, &sourceEidStr);
	if (sourceEidStr == NULL)
	{
		putErrmsg("CTEB: can't read source EID.", NULL);
		return -1;
	}

	/*	Use seqId 0 (destination-specific counters).
	 *	forCustody = 1 to use custody-specific counter.		*/

	if (cbr_allocateSeqNum(sdr, sourceEidStr, NULL, 0, 1, &seqNum) < 0)
	{
		putErrmsg("CTEB: can't allocate sequence number.", NULL);
		return -1;
	}

	/*	Populate scratchpad with CTEB data.			*/

	memset(&scratch, 0, sizeof(CtebScratchpad));
	scratch.seqNum = seqNum;
	scratch.seqId = 0;		/*	Default seqId		*/
	scratch.custodianEid = sdr_string_create(sdr, vscheme->adminEid);
	if (scratch.custodianEid == 0)
	{
		putErrmsg("CTEB: can't store custodian EID.", NULL);
		return -1;
	}

	/*	Store scratchpad in SDR for later serialization.	*/

	scratchAddr = sdr_malloc(sdr, sizeof(CtebScratchpad));
	if (scratchAddr == 0)
	{
		sdr_free(sdr, scratch.custodianEid);
		putErrmsg("CTEB: can't allocate scratchpad.", NULL);
		return -1;
	}

	sdr_write(sdr, scratchAddr, (char *) &scratch, sizeof(CtebScratchpad));

	/*	Configure block.					*/

	blk->blkProcFlags = CTEB_BLOCK_PROC_FLAGS;
	blk->dataLength = 0;	/*	Will serialize at dequeue time.	*/
	blk->length = 0;
	blk->size = sizeof(CtebScratchpad);
	blk->object = scratchAddr;

	/*	Per Orange Book: bundles with CTEB MUST NOT be fragmented. */

	bundle->bundleProcFlags |= BDL_DOES_NOT_FRAGMENT;

	return 0;
}

int	cteb_serialize(ExtensionBlock *blk, Bundle *bundle)
{
	Sdr		sdr = getIonsdr();
	CtebScratchpad	scratch;
	unsigned char	dataBuffer[CTEB_MAX_SERIALIZED_LEN];
	unsigned char	*cursor;
	uvast		uvtemp;
	char		eidBuf[MAX_EID_LEN];

	(void) bundle;

	if (blk->object == 0)
	{
		putErrmsg("CTEB: no scratchpad for serialization.", NULL);
		return -1;
	}

	/*	Read scratchpad data.					*/

	sdr_read(sdr, (char *) &scratch, blk->object, sizeof(CtebScratchpad));

	/*	Serialize CTEB to CBOR: [seq_num, seq_id, custodian_eid] */

	cursor = dataBuffer;

	/*	Open CBOR array with 3 elements.			*/

	uvtemp = 3;
	oK(cbor_encode_array_open(uvtemp, &cursor));

	/*	Element 0: seq_num					*/

	uvtemp = scratch.seqNum;
	oK(cbor_encode_integer(uvtemp, &cursor));

	/*	Element 1: seq_id					*/

	uvtemp = scratch.seqId;
	oK(cbor_encode_integer(uvtemp, &cursor));

	/*	Element 2: custodian_eid				*/

	if (scratch.custodianEid != 0)
	{
		sdr_string_read(sdr, eidBuf, scratch.custodianEid);
		oK(cbor_encode_text_string(eidBuf, strlen(eidBuf), &cursor));
	}
	else
	{
		/*	No custodian EID - this shouldn't happen.	*/
		putErrmsg("CTEB: missing custodian EID.", NULL);
		return -1;
	}

	blk->dataLength = cursor - dataBuffer;
	return serializeExtBlk(blk, (char *) dataBuffer);
}

void	cteb_release(ExtensionBlock *blk)
{
	Sdr		sdr = getIonsdr();
	CtebScratchpad	scratch;

	if (blk->object == 0)
	{
		return;
	}

	/*	Read scratchpad to release any SDR strings.		*/

	sdr_read(sdr, (char *) &scratch, blk->object, sizeof(CtebScratchpad));

	if (scratch.custodianEid != 0)
	{
		sdr_free(sdr, scratch.custodianEid);
	}

	sdr_free(sdr, blk->object);
	blk->object = 0;
}

int	cteb_record(ExtensionBlock *sdrBlk, AcqExtBlock *ramBlk)
{
	Sdr		sdr = getIonsdr();
	CtebScratchpad	*ramScratch;
	Object		scratchAddr;

	if (ramBlk->object == NULL || ramBlk->size == 0)
	{
		/*	No scratchpad data to record.			*/
		sdrBlk->object = 0;
		sdrBlk->size = 0;
		return 0;
	}

	ramScratch = (CtebScratchpad *) ramBlk->object;

	/*	Allocate SDR space for scratchpad.			*/

	scratchAddr = sdr_malloc(sdr, sizeof(CtebScratchpad));
	if (scratchAddr == 0)
	{
		putErrmsg("CTEB: can't allocate scratchpad for record.", NULL);
		return -1;
	}

	sdr_write(sdr, scratchAddr, (char *) ramScratch, sizeof(CtebScratchpad));

	sdrBlk->object = scratchAddr;
	sdrBlk->size = sizeof(CtebScratchpad);

	return 0;
}

int	cteb_copy(ExtensionBlock *newBlk, ExtensionBlock *oldBlk)
{
	Sdr		sdr = getIonsdr();
	CtebScratchpad	scratch;
	Object		newScratchAddr;
	char		eidBuf[MAX_EID_LEN];

	if (oldBlk->object == 0)
	{
		newBlk->object = 0;
		newBlk->size = 0;
		return 0;
	}

	/*	Read old scratchpad.					*/

	sdr_read(sdr, (char *) &scratch, oldBlk->object, sizeof(CtebScratchpad));

	/*	Allocate new scratchpad.				*/

	newScratchAddr = sdr_malloc(sdr, sizeof(CtebScratchpad));
	if (newScratchAddr == 0)
	{
		putErrmsg("CTEB: can't allocate scratchpad for copy.", NULL);
		return -1;
	}

	/*	Copy custodian EID string.				*/

	if (scratch.custodianEid != 0)
	{
		sdr_string_read(sdr, eidBuf, scratch.custodianEid);
		scratch.custodianEid = sdr_string_create(sdr, eidBuf);
	}

	sdr_write(sdr, newScratchAddr, (char *) &scratch, sizeof(CtebScratchpad));

	newBlk->object = newScratchAddr;
	newBlk->size = sizeof(CtebScratchpad);

	return 0;
}

/*	*	*	Processing Callbacks	*	*	*	*/

int	cteb_processOnFwd(ExtensionBlock *blk, Bundle *bundle, void *ctxt)
{
	(void) blk;
	(void) bundle;
	(void) ctxt;

	/*	No special processing on forward.			*/

	return 0;
}

int	cteb_processOnAccept(ExtensionBlock *blk, Bundle *bundle, void *ctxt)
{
	Sdr		sdr = getIonsdr();
	CtebScratchpad	scratch;
	CtebBlk		ctebData;
	char		custodianBuf[MAX_EID_LEN];

	(void) ctxt;

	if (blk->object == 0)
	{
		return 0;
	}

	/*	Read CTEB data for custody acceptance.			*/

	sdr_read(sdr, (char *) &scratch, blk->object, sizeof(CtebScratchpad));

	/*	Build CtebBlk for cbr_acceptCustody.			*/

	ctebData.seqNum = scratch.seqNum;
	ctebData.seqId = scratch.seqId;

	if (scratch.custodianEid != 0)
	{
		sdr_string_read(sdr, custodianBuf, scratch.custodianEid);
		ctebData.custodianEid = custodianBuf;
	}
	else
	{
		ctebData.custodianEid = NULL;
	}

	/*	Accept custody - this will:
	 *	1. Send CCS acceptance to previous custodian
	 *	2. Add bundle to local custody tracking		*/

	if (cbr_acceptCustody(sdr, bundle, &ctebData) < 0)
	{
		putErrmsg("CTEB: custody acceptance failed.", NULL);
		return -1;
	}

	return 0;
}

int	cteb_processOnEnqueue(ExtensionBlock *blk, Bundle *bundle, void *ctxt)
{
	(void) blk;
	(void) bundle;
	(void) ctxt;

	/*	No special processing on enqueue.			*/

	return 0;
}

int	cteb_processOnDequeue(ExtensionBlock *blk, Bundle *bundle, void *ctxt)
{
	Sdr		sdr = getIonsdr();
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	CtebScratchpad	scratch;

	(void) ctxt;

	if (blk->object == 0)
	{
		return cteb_serialize(blk, bundle);
	}

	/*	Get our admin EID as new custodian.			*/

	findScheme("ipn", &vscheme, &vschemeElt);
	if (vschemeElt == 0)
	{
		putErrmsg("CTEB: can't find ipn scheme for admin EID.", NULL);
		return -1;
	}

	/*	Update custodian EID to our node before transmission.
	 *	This implements the "replace custodian" semantics
	 *	per Orange Book: delete old, insert new.		*/

	sdr_read(sdr, (char *) &scratch, blk->object, sizeof(CtebScratchpad));

	/*	Free old custodian EID string.				*/

	if (scratch.custodianEid != 0)
	{
		sdr_free(sdr, scratch.custodianEid);
	}

	/*	Set new custodian EID to our node.			*/

	scratch.custodianEid = sdr_string_create(sdr, vscheme->adminEid);
	if (scratch.custodianEid == 0)
	{
		putErrmsg("CTEB: can't update custodian EID.", NULL);
		return -1;
	}

	/*	Write updated scratchpad.				*/

	sdr_write(sdr, blk->object, (char *) &scratch, sizeof(CtebScratchpad));

	/*	Serialize CTEB block with updated custodian.		*/

	return cteb_serialize(blk, bundle);
}

/*	*	*	Inbound Block Functions	*	*	*	*/

int	cteb_parse(AcqExtBlock *blk, AcqWorkArea *wk)
{
	Sdr		sdr = getIonsdr();
	unsigned char	*cursor;
	unsigned int	unparsedBytes;
	uvast		arrayLength;
	uvast		uvtemp;
	CtebScratchpad	*scratch;
	char		eidBuf[MAX_EID_LEN];
	uvast		eidLen;

	(void) wk;	/*	May use later for bundle access.	*/

	if (blk->dataLength < 3)
	{
		writeMemo("[?] CTEB block too short.");
		return 0;	/*	Malformed.			*/
	}

	/*	Allocate scratchpad in working memory.			*/

	scratch = MTAKE(sizeof(CtebScratchpad));
	if (scratch == NULL)
	{
		putErrmsg("CTEB: can't allocate parse scratchpad.", NULL);
		return -1;
	}

	memset(scratch, 0, sizeof(CtebScratchpad));
	blk->object = scratch;
	blk->size = sizeof(CtebScratchpad);

	/*	Parse CBOR array.					*/

	cursor = blk->bytes + (blk->length - blk->dataLength);
	unparsedBytes = blk->dataLength;

	/*	Decode array open - expect exactly 3 elements.		*/

	arrayLength = 0;	/*	Accept any length.		*/
	if (cbor_decode_array_open(&arrayLength, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] CTEB: can't decode array open.");
		return 0;
	}

	if (arrayLength != 3)
	{
		writeMemoNote("[?] CTEB: expected 3-element array, got",
				itoa(arrayLength));
		return 0;
	}

	/*	Element 0: seq_num					*/

	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] CTEB: can't decode seq_num.");
		return 0;
	}

	scratch->seqNum = uvtemp;

	/*	Element 1: seq_id					*/

	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] CTEB: can't decode seq_id.");
		return 0;
	}

	scratch->seqId = uvtemp;

	/*	Element 2: custodian_eid				*/

	eidLen = MAX_EID_LEN - 1;
	if (cbor_decode_text_string(eidBuf, &eidLen, &cursor,
			&unparsedBytes) < 1)
	{
		writeMemo("[?] CTEB: can't decode custodian_eid.");
		return 0;
	}

	eidBuf[eidLen] = '\0';

	/*	Store custodian EID in SDR.				*/

	scratch->custodianEid = sdr_string_create(sdr, eidBuf);
	if (scratch->custodianEid == 0)
	{
		writeMemo("[?] CTEB: can't store custodian EID.");
		return 0;
	}

	if (unparsedBytes != 0)
	{
		writeMemo("[?] CTEB: excess bytes at end of block.");
		return 0;
	}

	return 1;	/*	Block parsed successfully.		*/
}

int	cteb_check(AcqExtBlock *blk, AcqWorkArea *wk)
{
	(void) blk;
	(void) wk;

	/*	CTEB blocks are always valid if successfully parsed.	*/

	return 1;
}

void	cteb_clear(AcqExtBlock *blk)
{
	Sdr		sdr = getIonsdr();
	CtebScratchpad	*scratch;

	if (blk->object != NULL)
	{
		scratch = (CtebScratchpad *) blk->object;

		/*	Free SDR string if allocated during parse.	*/

		if (scratch->custodianEid != 0)
		{
			sdr_free(sdr, scratch->custodianEid);
		}

		MRELEASE(blk->object);
		blk->object = NULL;
	}

	blk->size = 0;
}
