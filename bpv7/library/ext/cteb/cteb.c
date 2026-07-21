/*
 *	cteb.c:		implementation of the extension definition
 *			functions for the Custody Transfer Extension
 *			Block (CTEB).
 *
 *	Per CCSDS 734.6-O-1: Custody Transfer and Compressed Bundle
 *	Status Reporting (Experimental Specification, Issue 1, June 2026).
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
#include "creb.h"

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
 * processing. Stored in blk->object for outbound blocks (SDR) and
 * in AcqExtBlock->object for inbound blocks (working memory).
 *
 * custodianEid is stored as a char array to avoid SDR transaction
 * issues: cteb_clear() is called outside of SDR transactions, so
 * we cannot use sdr_free() there. This approach works for both
 * outbound (SDR-stored) and inbound (working memory) scratchpads.
 */
typedef struct {
	uvast		seqNum;			/* Bundle sequence number */
	uvast		seqId;			/* Sequence identifier */
	char		custodianEid[MAX_EID_LEN];	/* Current custodian */
} CtebScratchpad;

/*	*	*	Outbound Block Functions	*	*	*/

int	cteb_offer(ExtensionBlock *blk, Bundle *bundle)
{
	Sdr		sdr = getIonsdr();
	int		custodyMode;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	CtebScratchpad	scratch;
	uvast		seqNum;
	char		*sourceEidStr;
	char		*destEidStr;
	SdrObject	scratchAddr;

	/*	Check if Orange Book custody mode is active.
	 *	CTEB should only be attached when:
	 *	  1. Custody mode is set to BP_CUSTODY_ORANGEBOOK, AND
	 *	  2. Application requested custody (SourceCustodyRequired)
	 *
	 *	This ensures CTEB doesn't conflict with BIBE custody.	*/

	custodyMode = cbr_getCustodyMode(sdr);
#ifdef DEBUG_CUSTODY_SRC
	writeMemoNote("[DEBUG-CUSTODY-SRC] cteb_offer: custodyMode", itoa(custodyMode));
#endif

	if (custodyMode != BP_CUSTODY_ORANGEBOOK)
	{
		/*	Orange Book custody not active on this node.	*/
#ifdef DEBUG_CUSTODY_SRC
		writeMemo("[DEBUG-CUSTODY-SRC] cteb_offer: Orange Book custody not active");
#endif
		return 0;
	}

	if (!(bundle->ancillaryData.flags & BP_CT_REQUESTED))
	{
		/*	Custody transfer not requested by application.	*/
#ifdef DEBUG_CUSTODY_SRC
		writeMemo("[DEBUG-CUSTODY-SRC] cteb_offer: custody not requested");
#endif
		return 0;
	}

#ifdef DEBUG_CUSTODY_SRC
	writeMemo("[DEBUG-CUSTODY-SRC] cteb_offer: creating CTEB block");
#endif

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

	/*	Read destination EID for sequence counter lookup.
	 *	Per Orange Book, seqId 0 uses destination-specific counters. */

	readEid(&bundle->destination, &destEidStr);
	if (destEidStr == NULL)
	{
		MRELEASE(sourceEidStr);
		putErrmsg("CTEB: can't read destination EID.", NULL);
		return -1;
	}

	/*	Use bundle's cbrSeqId (0 = destination-specific counters).
	 *	forCustody = 1 to use custody-specific counter.		*/

	if (cbr_allocateSeqNum(sdr, sourceEidStr, destEidStr,
			bundle->ancillaryData.cbrSeqId, 1, &seqNum) < 0)
	{
		MRELEASE(sourceEidStr);
		MRELEASE(destEidStr);
		putErrmsg("CTEB: can't allocate sequence number.", NULL);
		return -1;
	}

	MRELEASE(destEidStr);
	MRELEASE(sourceEidStr);

	/*	Populate scratchpad with CTEB data.			*/

	memset(&scratch, 0, sizeof(CtebScratchpad));
	scratch.seqNum = seqNum;
	scratch.seqId = bundle->ancillaryData.cbrSeqId;
	istrcpy(scratch.custodianEid, vscheme->adminEid, MAX_EID_LEN);

	/*	Store scratchpad in SDR for later serialization.	*/

	scratchAddr = sdr_malloc(sdr, sizeof(CtebScratchpad));
	if (scratchAddr == 0)
	{
		putErrmsg("CTEB: can't allocate scratchpad.", NULL);
		return -1;
	}

	sdr_write(sdr, scratchAddr, (char *) &scratch, sizeof(CtebScratchpad));

	/*	Configure block.					*/

	blk->blkProcFlags = CTEB_BLOCK_PROC_FLAGS;
	blk->dataLength = 0;	/*	Will serialize at dequeue time.	*/

	/*	blk->length serves two purposes:
	 *	  1. Before serialization: non-zero indicates block is
	 *	     viable and should not be deleted (length=0 means
	 *	     "scratched" in processExtensionBlocks).
	 *	  2. After serialization: holds actual serialized length.
	 *	Set to 1 as placeholder; actual length set by
	 *	cteb_serialize called from cteb_processOnDequeue.	*/
	blk->length = 1;
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
	int		eidLen;

	(void) bundle;

	if (blk->object == 0)
	{
		putErrmsg("CTEB: no scratchpad for serialization.", NULL);
		return -1;
	}

	/*	Read scratchpad data.					*/

	sdr_read(sdr, (char *) &scratch, blk->object, sizeof(CtebScratchpad));

	/*	Serialize CTEB to CBOR: [seq_num, seq_id, blk_source]
	 *	where blk_source is an `eid` -- the same RFC 9171 /
	 *	RFC 9758 [uri-code, SSP] structure used by primary-block
	 *	EIDs (CCSDS 734.6-O-1, Annex E).			*/

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

	/*	Element 2: blk_source (custodian EID as structured eid)	*/

	if (scratch.custodianEid[0] == '\0')
	{
		/*	CDDL requires blk_source; treat as a bug.	*/

		putErrmsg("CTEB: missing custodian EID.", NULL);
		return -1;
	}

	eidLen = serializeEidString(scratch.custodianEid, cursor);
	if (eidLen < 1)
	{
		putErrmsg("CTEB: can't serialize custodian EID.",
				scratch.custodianEid);
		return -1;
	}

	cursor += eidLen;
	blk->dataLength = cursor - dataBuffer;
	return serializeExtBlk(blk, (char *) dataBuffer);
}

void	cteb_release(ExtensionBlock *blk)
{
	Sdr		sdr = getIonsdr();

	if (blk->object == 0)
	{
		return;
	}

	/*	Free the scratchpad. custodianEid is embedded in the
	 *	struct as a char array, so no separate free needed.	*/

	sdr_free(sdr, blk->object);
	blk->object = 0;
}

int	cteb_record(ExtensionBlock *sdrBlk, AcqExtBlock *ramBlk)
{
	Sdr		sdr = getIonsdr();
	CtebScratchpad	*ramScratch;
	SdrObject	scratchAddr;

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
	SdrObject	newScratchAddr;

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

	/*	Write copy - custodianEid is embedded char array,
	 *	so simple struct copy is sufficient.			*/

	sdr_write(sdr, newScratchAddr, (char *) &scratch, sizeof(CtebScratchpad));

	newBlk->object = newScratchAddr;
	newBlk->size = sizeof(CtebScratchpad);

	return 0;
}

/*	*	*	Processing Callbacks	*	*	*	*/

/*
 * If the bundle has a CREB block with flagBit set in requestFlags,
 * generate a CRS for the given statusCode.  Silently skips if the
 * CREB block is absent or the flag is not set.
 */
static int	sendCustodyCrsIfRequested(Sdr sdr, Bundle *bundle,
			int statusCode, unsigned char flagBit)
{
	SdrObject	crebElt;
	ExtensionBlock	crebBlk;
	uvast		seqId;
	uvast		seqNum;
	unsigned char	requestFlags;
	CrebBlk		creb;
	char		crebReportToEid[MAX_EID_LEN];

	crebElt = findExtensionBlock(bundle, CBR_BLOCK_TYPE_CREB, 0);
	if (crebElt == 0)
	{
		return 0;
	}

	sdr_read(sdr, (char *) &crebBlk, sdr_list_data(sdr, crebElt),
			sizeof(ExtensionBlock));

	if (creb_getReportInfo(&crebBlk, &seqId, &seqNum) < 0)
	{
		return 0;
	}

	if (creb_getRequestFlags(&crebBlk, &requestFlags) < 0)
	{
		return 0;
	}

	if (!(requestFlags & flagBit))
	{
		return 0;
	}

	memset(&creb, 0, sizeof(CrebBlk));
	creb.seqId = seqId;
	creb.seqNum = seqNum;
	creb.sourceEid = NULL;
	creb.reportToEid = NULL;
	if (creb_getReportToEid(&crebBlk, crebReportToEid,
			sizeof(crebReportToEid)) == 0
			&& crebReportToEid[0] != '\0')
	{
		creb.reportToEid = crebReportToEid;
	}

	return cbr_reportStatus(sdr, bundle, statusCode, &creb);
}

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
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	SdrObject	bundleAddr = (SdrObject) ctxt; /* From forwardBundle */

	if (blk->object == 0)
	{
		return 0;
	}

	/*	Read CTEB data for custody acceptance.			*/

	sdr_read(sdr, (char *) &scratch, blk->object, sizeof(CtebScratchpad));

	/*	Build CtebBlk for cbr_acceptCustody.			*/

	ctebData.seqNum = scratch.seqNum;
	ctebData.seqId = scratch.seqId;

	if (scratch.custodianEid[0] != '\0')
	{
		ctebData.custodianEid = scratch.custodianEid;
	}
	else
	{
		ctebData.custodianEid = NULL;
	}

	/*	Check if we are already the custodian (source-originated
	 *	bundle). If so, skip custody acceptance - we don't need
	 *	to accept custody from ourselves or send CCS to ourselves.
	 *	Custody was already tracked in bpSend.			*/

	findScheme("ipn", &vscheme, &vschemeElt);

#ifdef DEBUG_CUSTODY_SRC
	writeMemoNote("[DEBUG-CUSTODY-SRC] cteb_processOnAccept: custodianEid",
			ctebData.custodianEid ? ctebData.custodianEid : "(null)");
	writeMemoNote("[DEBUG-CUSTODY-SRC] cteb_processOnAccept: adminEid",
			(vschemeElt != 0) ? vscheme->adminEid : "(no scheme)");
#endif

	if (vschemeElt != 0 && ctebData.custodianEid != NULL)
	{
		if (strcmp(ctebData.custodianEid, vscheme->adminEid) == 0)
		{
			/*	We are already the custodian - this is
			 *	a source-originated bundle. Skip accept. */
#ifdef DEBUG_CUSTODY_SRC
			writeMemo("[DEBUG-CUSTODY-SRC] CTEB: We are custodian, skipping accept");
#endif
			return 0;
		}
	}

	/*	Whitelist check: if either whitelist is non-empty the
	 *	custodian/source EID must appear in it.			*/
	{
		char	*srcEid = NULL;
		int	accepted;

		readEid(&bundle->id.source, &srcEid);
		accepted = cbr_isCustodyAccepted(sdr, ctebData.custodianEid,
				srcEid);
		MRELEASE(srcEid);
		if (!accepted)
		{
			writeMemoNote("[i] CTEB: Custody refused by policy, custodian",
					ctebData.custodianEid ?
					ctebData.custodianEid : "(null)");
			if (cbr_refuseCustody(sdr, bundle, &ctebData,
					CbrRefusePolicy) < 0)
			{
				putErrmsg("CTEB: custody refusal failed.", NULL);
				return -1;
			}

			if (sendCustodyCrsIfRequested(sdr, bundle,
					CBR_STATUS_CUSTODY_REFUSED,
					CREB_REQUEST_CUSTODY_REFUSE) < 0)
			{
				putErrmsg("CTEB: custody-refused CRS failed.",
						NULL);
				/*	Not fatal; refusal already sent.	*/
			}

			return 0;
		}
	}

	/*	Accept custody - this will:
	 *	1. Send CCS acceptance to previous custodian
	 *	2. Add bundle to local custody tracking (if not destination) */

	writeMemoNote("[i] CTEB: Accepting custody from", ctebData.custodianEid ?
			ctebData.custodianEid : "(null)");

	if (cbr_acceptCustody(sdr, bundle, bundleAddr, &ctebData) < 0)
	{
		putErrmsg("CTEB: custody acceptance failed.", NULL);
		return -1;
	}

	if (sendCustodyCrsIfRequested(sdr, bundle,
			CBR_STATUS_CUSTODY_ACCEPTED,
			CREB_REQUEST_CUSTODY_ACCEPT) < 0)
	{
		putErrmsg("CTEB: custody-accepted CRS failed.", NULL);
		/*	Not fatal; acceptance already sent.		*/
	}

	/*	If bundleAddr == 0, this is destination delivery.
	 *	Don't detain - bundle will be delivered and destroyed.
	 *	For intermediate nodes, detain until CCS from next hop.	*/

	if (bundleAddr != 0)
	{
		bundle->detained = 1;
		writeMemo("[i] CTEB: Custody accepted, CCS queued, bundle detained.");
	}
	else
	{
		writeMemo("[i] CTEB: Destination custody accepted, CCS queued.");
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

	sdr_stage(sdr, (char *) &scratch, blk->object, sizeof(CtebScratchpad));

	/*	Set new custodian EID to our node.			*/

	istrcpy(scratch.custodianEid, vscheme->adminEid, MAX_EID_LEN);

	/*	Write updated scratchpad.				*/

	sdr_write(sdr, blk->object, (char *) &scratch, sizeof(CtebScratchpad));

	/*	Serialize CTEB block with updated custodian.		*/

	return cteb_serialize(blk, bundle);
}

/*	*	*	Inbound Block Functions	*	*	*	*/

int	cteb_parse(AcqExtBlock *blk, AcqWorkArea *wk)
{
	unsigned char	*cursor;
	unsigned int	unparsedBytes;
	uvast		arrayLength;
	uvast		uvtemp;
	CtebScratchpad	*scratch;

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

	/*	Element 2: blk_source (custodian EID as structured eid)	*/

	if (acquireEidString(scratch->custodianEid, MAX_EID_LEN, &cursor,
			&unparsedBytes) < 1)
	{
		writeMemo("[?] CTEB: can't decode custodian_eid.");
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
	if (blk->object != NULL)
	{
		/*	custodianEid is embedded char array in struct,
		 *	so just free the working memory scratchpad.	*/

		MRELEASE(blk->object);
		blk->object = NULL;
	}

	blk->size = 0;
}

/*	*	*	Utility Functions	*	*	*	*/

int	cteb_getCustodyInfo(ExtensionBlock *blk, uvast *seqId, uvast *seqNum)
{
	Sdr		sdr = getIonsdr();
	CtebScratchpad	scratch;

	if (blk == NULL || blk->object == 0)
	{
		return -1;
	}

	sdr_read(sdr, (char *) &scratch, blk->object, sizeof(CtebScratchpad));
	*seqId = scratch.seqId;
	*seqNum = scratch.seqNum;
	return 0;
}
