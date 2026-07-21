/*
 *	creb.c:		implementation of the extension definition
 *			functions for the Compressed Reporting Extension
 *			Block (CREB).
 *
 *	Per CCSDS 734.6-O-1: Custody Transfer and Compressed Bundle
 *	Status Reporting (Experimental Specification, Issue 1, June 2026).
 *
 *	CREB block structure (CBOR array, variable length 1-5):
 *	  [seq_num]                                          - len 1
 *	  [seq_num, seq_id]                                  - len 2
 *	  [seq_num, seq_id, request_flags]                   - len 3
 *	  [seq_num, seq_id, request_flags, source_eid]       - len 4
 *	  [seq_num, seq_id, request_flags, source_eid, report_to_eid] - len 5
 *
 *	Copyright (c) 2026, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: ION Development Team
 */

#include "bpP.h"
#include "bei.h"
#include "creb.h"
#include "cbrP.h"

/*	Block processing flags for CREB per Orange Book.
 *	Bit 0 (0x01): Block must be replicated in every fragment
 *	Bits 2,4 cleared: Allow transparent forwarding at non-supporting nodes
 */
#define CREB_BLOCK_PROC_FLAGS	0x01

/*	Maximum size of serialized CREB block data.
 *	Worst case: array(5) + uint64 + uint64 + uint8 + eid + eid
 *	Estimate: 1 + 9 + 9 + 2 + 82 + 82 = ~185 bytes
 */
#define CREB_MAX_SERIALIZED_LEN	256

/*	*	*	Scratchpad Structure	*	*	*	*/

/*
 * CrebScratchpad holds CREB data in working memory during block
 * processing. Stored in blk->object for outbound blocks.
 */
typedef struct {
	uvast		seqNum;			/* Bundle sequence number */
	uvast		seqId;			/* Sequence identifier */
	unsigned char	requestFlags;		/* Status report request flags */
	char		sourceEid[MAX_EID_LEN];	/* "" if implicit (bundle source) */
	char		reportToEid[MAX_EID_LEN]; /* "" if implicit (bundle reportTo) */
	int		arrayLen;		/* CBOR array length (1-5) */
} CrebScratchpad;

/*	*	*	Helper Functions	*	*	*	*/

/*
 * Map BPv7 status report request flags (from bundleProcFlags) to
 * CREB request flags per Orange Book Section 5.1.4.1.
 *
 * BPv7 bundleProcFlags bits 14-20 contain:
 *   Bit 14: reception status requested
 *   Bit 15: (reserved, was custody transfer)
 *   Bit 16: forwarding status requested
 *   Bit 17: delivery status requested
 *   Bit 18: deletion status requested
 *
 * CREB request flags:
 *   Bit 0: Reception
 *   Bit 1: Forwarding
 *   Bit 2: Delivery
 *   Bit 3: Deletion
 *   Bit 4: Custody Acceptance
 *   Bit 5: Custody Rejection
 */
static unsigned char	mapSrrToCrebFlags(unsigned int bundleProcFlags)
{
	unsigned int	srrFlags = SRR_FLAGS(bundleProcFlags);
	unsigned char	crebFlags = 0;

	if (srrFlags & BP_RECEIVED_RPT)
	{
		crebFlags |= CREB_REQUEST_RECEPTION;
	}

	if (srrFlags & BP_FORWARDED_RPT)
	{
		crebFlags |= CREB_REQUEST_FORWARDING;
	}

	if (srrFlags & BP_DELIVERED_RPT)
	{
		crebFlags |= CREB_REQUEST_DELIVERY;
	}

	if (srrFlags & BP_DELETED_RPT)
	{
		crebFlags |= CREB_REQUEST_DELETION;
	}

	/*	If custody is requested, also request custody-event CRS.
	 *	BP_CT_REQUESTED sits below the SRR window so check it
	 *	directly on bundleProcFlags (not via SRR_FLAGS shift).	*/
	if (bundleProcFlags & BP_CT_REQUESTED)
	{
		crebFlags |= CREB_REQUEST_CUSTODY_ACCEPT
				| CREB_REQUEST_CUSTODY_REFUSE;
	}

	return crebFlags;
}

/*	*	*	Outbound Block Functions	*	*	*/

int	creb_offer(ExtensionBlock *blk, Bundle *bundle)
{
	Sdr		sdr = getIonsdr();
	int		mode;
	unsigned int	srrFlags;
	CrebScratchpad	scratch;
	uvast		seqNum;
	char		*sourceEidStr;
	char		*destEidStr;
	SdrObject	scratchAddr;

	/*	Check if CREB should be attached based on config mode.	*/

	mode = cbr_getStatusReportMode(sdr);
	if (mode < 0)
	{
		/*	CBR not available, fall back to traditional.	*/
		return 0;
	}

	if (mode == BP_SR_MODE_TRADITIONAL || mode == BP_SR_MODE_NONE)
	{
		/*	Traditional mode emits standard status reports and
		 *	NONE disables generation entirely; neither attaches
		 *	a CREB block (CREB drives compressed reporting).	*/
		return 0;
	}

	/*	Check if any status reports or custody events are requested.
	 *	BP_CT_REQUESTED sits below the SRR window (bit 6), so check
	 *	it directly on bundleProcFlags.				*/

	srrFlags = SRR_FLAGS(bundle->bundleProcFlags);
	if (srrFlags == 0 && !(bundle->bundleProcFlags & BP_CT_REQUESTED))
	{
		/*	Nothing to report, no CREB needed.		*/
		return 0;
	}

	/*	CREB is needed. Allocate sequence number.		*/

	readEid(&bundle->id.source, &sourceEidStr);
	if (sourceEidStr == NULL)
	{
		putErrmsg("CREB: can't read source EID.", NULL);
		return -1;
	}

	readEid(&bundle->destination, &destEidStr);
	if (destEidStr == NULL)
	{
		putErrmsg("CREB: can't read destination EID.", NULL);
		MRELEASE(sourceEidStr);
		return -1;
	}

	/*	Use bundle's cbrSeqId (0 = destination-specific counters).
	 *	When seqId = 0, destEid is required for counter lookup.	*/

	if (cbr_allocateSeqNum(sdr, sourceEidStr, destEidStr,
			bundle->ancillaryData.cbrSeqId, 0, &seqNum) < 0)
	{
		putErrmsg("CREB: can't allocate sequence number.", NULL);
		MRELEASE(sourceEidStr);
		MRELEASE(destEidStr);
		return -1;
	}

	/*	Populate scratchpad with CREB data.			*/

	memset(&scratch, 0, sizeof(CrebScratchpad));
	scratch.seqNum = seqNum;
	scratch.seqId = bundle->ancillaryData.cbrSeqId;
	scratch.requestFlags = mapSrrToCrebFlags(bundle->bundleProcFlags);
	scratch.arrayLen = 3;		/*	[seqNum, seqId, flags]	*/

	/*	When crebExplicitEid is set, include the source EID in
	 *	the block (arrayLen=4) rather than relying on implicit
	 *	convention.  Useful for interoperability testing.	*/

	if (cbr_getCrebExplicitEid())
	{
		char	*eidStr;

		readEid(&bundle->id.source, &eidStr);
		if (eidStr)
		{
			istrcpy(scratch.sourceEid, eidStr, MAX_EID_LEN);
			MRELEASE(eidStr);
			scratch.arrayLen = 4;
		}
	}

	MRELEASE(destEidStr);
	MRELEASE(sourceEidStr);

	/*	Apply CREB report-to EID override if configured.
	 *	This sets element 4 (arrayLen=5), directing relay and
	 *	destination nodes to send CRS to the overridden EID
	 *	rather than the bundle's reportTo field.
	 *	Element 3 (sourceEid) is required when arrayLen=5.	*/

	{
		CbrDb	*cbrConst = getCbrConstants();

		if (cbrConst != NULL
				&& cbrConst->crebDefaultReportToEid[0] != '\0')
		{
			if (scratch.sourceEid[0] == '\0')
			{
				char	*eidStr;

				readEid(&bundle->id.source, &eidStr);
				if (eidStr != NULL)
				{
					istrcpy(scratch.sourceEid, eidStr,
							MAX_EID_LEN);
					MRELEASE(eidStr);
				}
			}

			istrcpy(scratch.reportToEid,
					cbrConst->crebDefaultReportToEid,
					MAX_EID_LEN);
			scratch.arrayLen = 5;
		}
	}

	/*	Store scratchpad in SDR for later serialization.	*/

	scratchAddr = sdr_malloc(sdr, sizeof(CrebScratchpad));
	if (scratchAddr == 0)
	{
		putErrmsg("CREB: can't allocate scratchpad.", NULL);
		return -1;
	}

	sdr_write(sdr, scratchAddr, (char *) &scratch, sizeof(CrebScratchpad));

	/*	Configure block.					*/

	blk->blkProcFlags = CREB_BLOCK_PROC_FLAGS;
	blk->dataLength = 0;	/*	Will serialize at dequeue time.	*/

	/*	blk->length serves two purposes:
	 *	  1. Before serialization: non-zero indicates block is
	 *	     viable and should not be deleted (length=0 means
	 *	     "scratched" in processExtensionBlocks).
	 *	  2. After serialization: holds actual serialized length.
	 *	Set to 1 as placeholder; actual length set by
	 *	creb_serialize called from creb_processOnDequeue.	*/
	blk->length = 1;
	blk->size = sizeof(CrebScratchpad);
	blk->object = scratchAddr;

	/*	Per Orange Book: bundles with CREB MUST NOT be fragmented. */

	bundle->bundleProcFlags |= BDL_DOES_NOT_FRAGMENT;

	return 0;
}

int	creb_serialize(ExtensionBlock *blk, Bundle *bundle)
{
	Sdr		sdr = getIonsdr();
	CrebScratchpad	scratch;
	unsigned char	dataBuffer[CREB_MAX_SERIALIZED_LEN];
	unsigned char	*cursor;
	uvast		uvtemp;
	int		eidLen;

	(void) bundle;		/*	May use later for EID lookup.	*/

	if (blk->object == 0)
	{
		putErrmsg("CREB: no scratchpad for serialization.", NULL);
		return -1;
	}

	/*	Read scratchpad data.					*/

	sdr_read(sdr, (char *) &scratch, blk->object, sizeof(CrebScratchpad));

	/*	Serialize CREB to CBOR.					*/

	cursor = dataBuffer;

	/*	Open CBOR array with appropriate length.		*/

	uvtemp = scratch.arrayLen;
	oK(cbor_encode_array_open(uvtemp, &cursor));

	/*	Element 0: seq_num (always present)			*/

	uvtemp = scratch.seqNum;
	oK(cbor_encode_integer(uvtemp, &cursor));

	if (scratch.arrayLen >= 2)
	{
		/*	Element 1: seq_id				*/
		uvtemp = scratch.seqId;
		oK(cbor_encode_integer(uvtemp, &cursor));
	}

	if (scratch.arrayLen >= 3)
	{
		/*	Element 2: request_flags			*/
		uvtemp = scratch.requestFlags;
		oK(cbor_encode_integer(uvtemp, &cursor));
	}

	if (scratch.arrayLen >= 4 && scratch.sourceEid[0] != '\0')
	{
		/*	Element 3: blk_source (source EID as structured
		 *	eid per Orange Book Annex E).			*/

		eidLen = serializeEidString(scratch.sourceEid, cursor);
		if (eidLen < 1)
		{
			putErrmsg("CREB: can't serialize source EID.",
					scratch.sourceEid);
			return -1;
		}

		cursor += eidLen;
	}

	if (scratch.arrayLen >= 5 && scratch.reportToEid[0] != '\0')
	{
		/*	Element 4: report_to_eid (structured eid).	*/

		eidLen = serializeEidString(scratch.reportToEid, cursor);
		if (eidLen < 1)
		{
			putErrmsg("CREB: can't serialize report-to EID.",
					scratch.reportToEid);
			return -1;
		}

		cursor += eidLen;
	}

	blk->dataLength = cursor - dataBuffer;
	return serializeExtBlk(blk, (char *) dataBuffer);
}

void	creb_release(ExtensionBlock *blk)
{
	Sdr	sdr = getIonsdr();

	if (blk->object == 0)
	{
		return;
	}

	sdr_free(sdr, blk->object);
	blk->object = 0;
}

int	creb_record(ExtensionBlock *sdrBlk, AcqExtBlock *ramBlk)
{
	Sdr		sdr = getIonsdr();
	CrebScratchpad	*ramScratch;
	SdrObject	scratchAddr;

	if (ramBlk->object == NULL || ramBlk->size == 0)
	{
		/*	No scratchpad data to record.			*/
		sdrBlk->object = 0;
		sdrBlk->size = 0;
		return 0;
	}

	ramScratch = (CrebScratchpad *) ramBlk->object;

	/*	Allocate SDR space for scratchpad.			*/

	scratchAddr = sdr_malloc(sdr, sizeof(CrebScratchpad));
	if (scratchAddr == 0)
	{
		putErrmsg("CREB: can't allocate scratchpad for record.", NULL);
		return -1;
	}

	sdr_write(sdr, scratchAddr, (char *) ramScratch, sizeof(CrebScratchpad));

	sdrBlk->object = scratchAddr;
	sdrBlk->size = sizeof(CrebScratchpad);

	return 0;
}

int	creb_copy(ExtensionBlock *newBlk, ExtensionBlock *oldBlk)
{
	Sdr		sdr = getIonsdr();
	CrebScratchpad	scratch;
	SdrObject	newScratchAddr;

	if (oldBlk->object == 0)
	{
		newBlk->object = 0;
		newBlk->size = 0;
		return 0;
	}

	/*	Read old scratchpad.					*/

	sdr_read(sdr, (char *) &scratch, oldBlk->object, sizeof(CrebScratchpad));

	/*	Allocate new scratchpad.				*/

	newScratchAddr = sdr_malloc(sdr, sizeof(CrebScratchpad));
	if (newScratchAddr == 0)
	{
		putErrmsg("CREB: can't allocate scratchpad for copy.", NULL);
		return -1;
	}

	sdr_write(sdr, newScratchAddr, (char *) &scratch, sizeof(CrebScratchpad));

	newBlk->object = newScratchAddr;
	newBlk->size = sizeof(CrebScratchpad);

	return 0;
}

/*	*	*	Processing Callbacks	*	*	*	*/

int	creb_processOnFwd(ExtensionBlock *blk, Bundle *bundle, void *ctxt)
{
	/*	Parameters intentionally unused for now.
	 *	Future: extract CREB data for CRS generation.		*/

	(void) blk;
	(void) bundle;
	(void) ctxt;

	/*	CRS generation will be triggered from libbpP.c
	 *	when the bundle is actually forwarded.			*/

	return 0;
}

int	creb_processOnAccept(ExtensionBlock *blk, Bundle *bundle, void *ctxt)
{
	(void) blk;
	(void) bundle;
	(void) ctxt;

	return 0;
}

int	creb_processOnEnqueue(ExtensionBlock *blk, Bundle *bundle, void *ctxt)
{
	(void) blk;
	(void) bundle;
	(void) ctxt;

	return 0;
}

int	creb_processOnDequeue(ExtensionBlock *blk, Bundle *bundle, void *ctxt)
{
	(void) ctxt;

	/*	Serialize CREB block at dequeue time.			*/

	return creb_serialize(blk, bundle);
}

/*	*	*	Inbound Block Functions	*	*	*	*/

int	creb_parse(AcqExtBlock *blk, AcqWorkArea *wk)
{
	unsigned char	*cursor;
	unsigned int	unparsedBytes;
	uvast		arrayLength;
	uvast		uvtemp;
	CrebScratchpad	*scratch;
	char		eidBuf[MAX_EID_LEN];

	(void) wk;	/*	May use later for bundle access.	*/

	if (blk->dataLength < 2)
	{
		writeMemo("[?] CREB block too short.");
		return 0;	/*	Malformed.			*/
	}

	/*	Allocate scratchpad in working memory.			*/

	scratch = MTAKE(sizeof(CrebScratchpad));
	if (scratch == NULL)
	{
		putErrmsg("CREB: can't allocate parse scratchpad.", NULL);
		return -1;
	}

	memset(scratch, 0, sizeof(CrebScratchpad));
	blk->object = scratch;
	blk->size = sizeof(CrebScratchpad);

	/*	Parse CBOR array.					*/

	cursor = blk->bytes + (blk->length - blk->dataLength);
	unparsedBytes = blk->dataLength;

	/*	Decode array open - get actual length.
	 *	Set arrayLength to 0 to accept any size, then
	 *	the actual size is returned in arrayLength.		*/

	arrayLength = 0;
	if (cbor_decode_array_open(&arrayLength, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] CREB: can't decode array open.");
		return 0;
	}

	scratch->arrayLen = arrayLength;

	if (arrayLength < 1 || arrayLength > 5)
	{
		writeMemo("[?] CREB: invalid array length.");
		return 0;
	}

	/*	Element 0: seq_num (always present)			*/

	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] CREB: can't decode seq_num.");
		return 0;
	}

	scratch->seqNum = uvtemp;

	/*	Element 1: seq_id (if array len >= 2)			*/

	if (arrayLength >= 2)
	{
		if (cbor_decode_integer(&uvtemp, CborAny, &cursor,
				&unparsedBytes) < 1)
		{
			writeMemo("[?] CREB: can't decode seq_id.");
			return 0;
		}

		scratch->seqId = uvtemp;
	}

	/*	Element 2: request_flags (if array len >= 3)		*/

	if (arrayLength >= 3)
	{
		if (cbor_decode_integer(&uvtemp, CborAny, &cursor,
				&unparsedBytes) < 1)
		{
			writeMemo("[?] CREB: can't decode request_flags.");
			return 0;
		}

		scratch->requestFlags = (unsigned char) uvtemp;
	}

	/*	Element 3: blk_source (source EID as structured eid)	*/

	if (arrayLength >= 4)
	{
		if (acquireEidString(eidBuf, MAX_EID_LEN, &cursor,
				&unparsedBytes) < 1)
		{
			writeMemo("[?] CREB: can't decode source_eid.");
			return 0;
		}

		istrcpy(scratch->sourceEid, eidBuf, MAX_EID_LEN);
	}

	/*	Element 4: report_to_eid (structured eid)		*/

	if (arrayLength >= 5)
	{
		if (acquireEidString(eidBuf, MAX_EID_LEN, &cursor,
				&unparsedBytes) < 1)
		{
			writeMemo("[?] CREB: can't decode report_to_eid.");
			return 0;
		}

		istrcpy(scratch->reportToEid, eidBuf, MAX_EID_LEN);
	}

	if (unparsedBytes != 0)
	{
		writeMemo("[?] CREB: excess bytes at end of block.");
		return 0;
	}

	return 1;	/*	Block parsed successfully.		*/
}

int	creb_check(AcqExtBlock *blk, AcqWorkArea *wk)
{
	(void) blk;
	(void) wk;

	/*	CREB blocks are always valid if successfully parsed.	*/

	return 1;
}

void	creb_clear(AcqExtBlock *blk)
{
	if (blk->object != NULL)
	{
		MRELEASE(blk->object);
		blk->object = NULL;
	}

	blk->size = 0;
}

/*	*	*	Utility Functions	*	*	*	*/

int	creb_getReportInfo(ExtensionBlock *blk, uvast *seqId, uvast *seqNum)
{
	Sdr		sdr = getIonsdr();
	CrebScratchpad	scratch;

	if (blk == NULL || blk->object == 0)
	{
		return -1;
	}

	sdr_read(sdr, (char *) &scratch, blk->object, sizeof(CrebScratchpad));
	*seqId = scratch.seqId;
	*seqNum = scratch.seqNum;
	return 0;
}

int	creb_getRequestFlags(ExtensionBlock *blk, unsigned char *requestFlags)
{
	Sdr		sdr = getIonsdr();
	CrebScratchpad	scratch;

	if (blk == NULL || blk->object == 0 || requestFlags == NULL)
	{
		return -1;
	}

	sdr_read(sdr, (char *) &scratch, blk->object, sizeof(CrebScratchpad));
	*requestFlags = scratch.requestFlags;
	return 0;
}

int	creb_getReportToEid(ExtensionBlock *blk, char *buf, size_t bufLen)
{
	Sdr		sdr = getIonsdr();
	CrebScratchpad	scratch;

	CHKERR(buf);
	CHKERR(bufLen > 0);
	buf[0] = '\0';
	if (blk == NULL || blk->object == 0)
	{
		return 0;
	}

	sdr_read(sdr, (char *) &scratch, blk->object, sizeof(CrebScratchpad));
	if (scratch.arrayLen >= 5 && scratch.reportToEid[0] != '\0')
	{
		istrcpy(buf, scratch.reportToEid, bufLen);
	}

	return 0;
}
