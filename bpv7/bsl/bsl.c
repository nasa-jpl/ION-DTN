/*

	bsl.c:	Implementation of the ION functions for accessing
		BPSec library functionality.

	Author:	Scott Burleigh

	Copyright (c) 2025, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.

									*/
#include "bpP.h"
#include "bei.h"
#include "ionpatch.h"
#include <BPSecLib_Public.h>
#include <BPSecLib_Private.h>
#include <backend/UtilDefs_SeqReadWrite.h>
#include <security_context/DefaultSecContext.h>
#include <policy_provider/SamplePolicyProvider.h>
#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <m-string.h>

#if 0
#include "agent.h"
#include "encode.h"
#include "decode.h"
#include "text_util.h"
#endif

/*	Reasons a block cannot be the target of a security operation,
 *	as reported by reportIneligibleTarget below, and the reason
 *	kinds used to rate-limit its memos.				*/

#define	CONTENT_DEFERRED	"is rewritten when the bundle is dequeued \
for transmission and has no settled content while the bundle is at rest"
#define	CONTENT_EMPTY		"has zero length and no content to protect"
#define	CONTENT_SUPPRESSED	"is suppressed and will not be included in \
the bundle as transmitted"

#define	KIND_DEFERRED		(0)
#define	KIND_EMPTY		(1)
#define	KIND_SUPPRESSED		(2)
#define	REASON_KINDS		(3)

/****** bsl "descriptors" - ION implementations of BSL callbacks ********/

/*	*	Functions that operate on bundles and blocks	*	*/

/*	Some ION extension blocks are rewritten at transmission time,
 *	so they have no settled content while the bundle is at rest.
 *	The offer functions for the previous-node block and the bundle
 *	age block, for example, reserve the block but leave its
 *	type-specific data empty; the content is produced when the
 *	bundle is dequeued for transmission, and the previous-node
 *	block may even be suppressed at that point if no proximate
 *	node EID applies.
 *
 *	A block in that state cannot carry a security service.  There
 *	is nothing to sign or encrypt yet, and whatever ION writes at
 *	dequeue time would replace the bytes a BIB or BCB was computed
 *	over, so the security result would never verify at the
 *	receiving node.
 *
 *	Blocks in this state are therefore withheld from the list of
 *	canonical blocks presented to BSL, and are reported as
 *	ineligible if their metadata is requested by block number, so
 *	that a policy rule naming such a block is declined instead of
 *	driving a zero-length BTSD read.				*/

/*	Note: a security block that BSL has just created also has a
 *	dataLength of zero until BSL writes its BTSD, but ION has
 *	already reserved storage for that block's bytes.  Keying on the
 *	absence of that storage - rather than on dataLength - keeps the
 *	newly created security block visible to BSL while withholding
 *	the blocks whose content ION has not yet produced.		*/

static int	sdrBlockHasNoBtsd(ExtensionBlock *blk)
{
	return (blk->bytes == 0);
}

static int	ramBlockHasNoBtsd(AcqExtBlock *blk)
{
	return (blk->length == 0);
}

/*	A block may also be withheld from this transmission even though
 *	its content is settled: an extension that decides at dequeue
 *	that the block does not apply to the outduct being used says so
 *	by suppressing the block, which keeps the block in the bundle
 *	but omits it from the bundle ION is about to serialize.  The
 *	previous-node block does this when no proximate node EID can be
 *	resolved.
 *
 *	catenateBundle is the only function that consults this flag, so
 *	a security block sourced over a suppressed target would name a
 *	block number that the receiving node never sees, and the
 *	security operation would fail there with nothing logged here.
 *	Withhold suppressed blocks as well, so that the security
 *	operation is never created rather than created and stranded.
 *
 *	Only the outbound side has this flag; a block being acquired is
 *	by definition one that was transmitted.				*/

static int	sdrBlockIsSuppressed(ExtensionBlock *blk)
{
	return (blk->suppressed != 0);
}

/*	Every locally sourced bundle carries these blocks, so reporting
 *	each one on each pass would flood the log with a fact that does
 *	not change.  One memo per block type per reason per process is
 *	enough to explain the "Cannot find target block type" warnings
 *	that BSL logs when a policy rule names one of them.
 *
 *	The reason is part of the key because one block type can be
 *	withheld for more than one reason over the life of a process:
 *	the same block type may have no content yet in a bundle this
 *	node sources and be suppressed in a bundle this node forwards.
 *	Reporting only the first reason seen would leave the second
 *	unexplained.
 *
 *	Note that withholding a block is not a bundle failure and so
 *	carries no status report reason code: the rule that named the
 *	block is declined, but the bundle is sourced and sent normally,
 *	and there is no status to assert.  The BpSrReason codes for
 *	security concern a service that was expected or attempted and
 *	did not succeed, which is a different condition from a policy
 *	rule that was never applied.					*/

static void	reportIneligibleTarget(unsigned int blockNbr,
			unsigned int blockType, int reasonKind,
			const char *reason)
{
	static char	reported[REASON_KINDS][256];
	char		msgbuf[256];

	if (reasonKind >= 0 && reasonKind < REASON_KINDS
	&& blockType < sizeof reported[reasonKind])
	{
		if (reported[reasonKind][blockType])
		{
			return;
		}

		reported[reasonKind][blockType] = 1;
	}

	isprintf(msgbuf, sizeof msgbuf, "[i] BSL: block %u (type %u) %s, so it \
cannot be the target of a BIB or BCB.", blockNbr, blockType, reason);
	writeMemo(msgbuf);
}

static int	getBlockNumbersFromSdr(Bundle *bundle,
			BSL_PrimaryBlock_t *result_primary_block)
{
	Sdr	sdr = getIonsdr();
	int	canonicalBlockCount;
	int	i;
	SdrObject elt;
	SdrObject addr;
		OBJ_POINTER(ExtensionBlock, blk);

	canonicalBlockCount = 1	+ sdr_list_length(sdr, bundle->extensions);
	result_primary_block->block_numbers = BSL_CALLOC(canonicalBlockCount,
			sizeof(uint64_t));
	if (!result_primary_block->block_numbers)
	{
		return -1;
	}

	for (elt = sdr_list_first(sdr, bundle->extensions), i= 0; elt;
			elt = sdr_list_next(sdr, elt))
	{
		addr = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, ExtensionBlock, blk, addr);
		if (sdrBlockHasNoBtsd(blk))
		{
			reportIneligibleTarget(blk->number, blk->type,
				KIND_DEFERRED, CONTENT_DEFERRED);
			continue;
		}

		if (sdrBlockIsSuppressed(blk))
		{
			reportIneligibleTarget(blk->number, blk->type,
				KIND_SUPPRESSED, CONTENT_SUPPRESSED);
			continue;
		}

		result_primary_block->block_numbers[i] = blk->number;
		i++;
	}

	result_primary_block->block_numbers[i] = 1;	/*	payload	*/
	result_primary_block->block_count = i + 1;
	return 0;
}

static int	getBlockNumbersFromRAM(AcqWorkArea *work,
			BSL_PrimaryBlock_t *result_primary_block)
{
	int		canonicalBlockCount;
	int		i;
	LystElt		elt;
	AcqExtBlock	*blk;

	canonicalBlockCount = 1	+ lyst_length(work->extBlocks);
	result_primary_block->block_numbers = BSL_CALLOC(canonicalBlockCount,
			sizeof(uint64_t));
	if (!result_primary_block->block_numbers)
	{
		return -1;
	}

	for (elt = lyst_first(work->extBlocks), i= 0; elt;
			elt = lyst_next(elt))
	{
		blk = (AcqExtBlock *) lyst_data(elt);
		if (ramBlockHasNoBtsd(blk))
		{
			reportIneligibleTarget(blk->number, blk->type,
				KIND_DEFERRED, CONTENT_DEFERRED);
			continue;
		}

		result_primary_block->block_numbers[i] = blk->number;
		i++;
	}

	result_primary_block->block_numbers[i] = 1;	/*	payload	*/
	result_primary_block->block_count = i + 1;
	return 0;
}

static int	ion_bsl_GetBundleMetadata(const BSL_BundleRef_t *bundle_ref,
			BSL_PrimaryBlock_t *result_primary_block)
{
	AcqWorkArea	*work;
	Bundle		*bundle;
	unsigned char	destinationEid[300];
	int		destinationEidLength;
	unsigned char	sourceEid[300];
	int		sourceEidLength;
	unsigned char	reportToEid[300];
	int		reportToEidLength;
	int		maxPrimaryBlockLength;
	unsigned char	*buffer;
	unsigned char	*cursor;

	/*	Note: the bundle structure itself may not yet reside
	 *	in the SDR heap at this point, but the in-heap copy
	 *	of the bundle structure will be identical to this copy
	 *	(which resides in memory) and all bundle elements that
	 *	are independently stored - notably extension blocks -
	 *	have already been written to the heap.			*/

	CHKERR(bundle_ref);
	CHKERR(bundle_ref->data);
	CHKERR(result_primary_block);
	work = (AcqWorkArea *) (bundle_ref->data);
	bundle = &(work->bundle);
	memset(result_primary_block, 0, sizeof(*result_primary_block));
	result_primary_block->field_version = 7;
	result_primary_block->field_flags = bundle->bundleProcFlags;
	result_primary_block->field_crc_type = bundle->primaryBlkCrcType;
	result_primary_block->field_dest_eid.handle = &(bundle->destination);
	result_primary_block->field_src_node_id.handle = &(bundle->id.source);
	result_primary_block->field_report_to_eid.handle = &(bundle->reportTo);
	result_primary_block->field_bundle_creation_time =
			bundle->id.creationTime.msec;
	result_primary_block->field_seq_num = bundle->id.creationTime.count;
	result_primary_block->field_lifetime = bundle->timeToLive;
	result_primary_block->field_frag_offset = bundle->id.fragmentOffset;
	result_primary_block->field_adu_length = bundle->totalAduLength;

	/*	Note information about canonical blocks in the bundle.	*/

	if (work->rawBundle == 0)
	{
		/*	Null AcqWorkArea.				*/

		if (getBlockNumbersFromSdr(bundle, result_primary_block) < 0)
		{
			return -2;
		}
	}
	else
	{
		if (getBlockNumbersFromRAM(work, result_primary_block) < 0)
		{
			return -2;
		}
	}

	/*	Provide serialized form of primary block.		*/

	destinationEidLength = serializeEid(&(bundle->destination),
			destinationEid);
	sourceEidLength = serializeEid(&(bundle->id.source),
			sourceEid);
	reportToEidLength = serializeEid(&(bundle->reportTo),
			reportToEid);
	if (destinationEidLength < 1 || sourceEidLength < 1 || reportToEidLength
			< 1)
	{
		BSL_FREE(result_primary_block->block_numbers);
		return -2;
	}

	maxPrimaryBlockLength = 50 + destinationEidLength + sourceEidLength
			+ reportToEidLength;
	buffer = BSL_CALLOC(1, maxPrimaryBlockLength);
	if (buffer == NULL)
	{
		BSL_FREE(result_primary_block->block_numbers);
		return -2;
	}

	cursor = buffer;
	serializePrimaryBlock(bundle, &cursor, destinationEid,
			destinationEidLength, sourceEid, sourceEidLength,
			reportToEid, reportToEidLength);

	/*	BSL will take ownership of the encoded buffer and free it
	 *	when BSL_PrimaryBlock_deinit() is called.  We allocate
	 *	the exact size needed and copy the serialized data, then
	 *	mark it as owned so BSL knows to free it.			*/

	if (BSL_Data_InitBuffer(&result_primary_block->encoded,
			cursor - buffer) != BSL_SUCCESS)
	{
		BSL_FREE(result_primary_block->block_numbers);
		BSL_FREE(buffer);
		return -2;
	}

	memcpy(result_primary_block->encoded.ptr, buffer, cursor - buffer);
	BSL_FREE(buffer);	/*	Free temporary serialization buffer.	*/
	return 0;
}

static int	getBlockMetadataFromSdr(Bundle *bundle, uint64_t block_num,
			BSL_CanonicalBlock_t *result_canonical_block)
{
	Sdr	sdr = getIonsdr();
	SdrObject elt;
	SdrObject addr;
		OBJ_POINTER(ExtensionBlock, blk);

	elt = getExtensionBlock(bundle, block_num);
	if (elt == 0)	/*	Block not found.		*/
	{
		return -1;
	}

	addr = sdr_list_data(sdr, elt);
	GET_OBJ_POINTER(sdr, ExtensionBlock, blk, addr);
	if (sdrBlockHasNoBtsd(blk))
	{
		reportIneligibleTarget(blk->number, blk->type,
				KIND_DEFERRED, CONTENT_DEFERRED);
		return -1;
	}

	if (sdrBlockIsSuppressed(blk))
	{
		reportIneligibleTarget(blk->number, blk->type,
				KIND_SUPPRESSED, CONTENT_SUPPRESSED);
		return -1;
	}

	result_canonical_block->block_num = blk->number;
	result_canonical_block->flags     = blk->blkProcFlags;
	result_canonical_block->crc_type  = blk->crcType;
	result_canonical_block->type_code = blk->type;
	result_canonical_block->btsd_len  = blk->dataLength;
	return 0;
}

static int	getBlockMetadataFromRAM(AcqWorkArea *work, uint64_t block_num,
			BSL_CanonicalBlock_t *result_canonical_block)
{
	LystElt		elt;
	AcqExtBlock	*blk;

	elt = getAcqExtensionBlock(work, block_num);
	if (elt == NULL)	/*	Block not found.		*/
	{
		return -1;
	}

	blk = (AcqExtBlock *) lyst_data(elt);
	if (ramBlockHasNoBtsd(blk))
	{
		reportIneligibleTarget(blk->number, blk->type,
				KIND_DEFERRED, CONTENT_DEFERRED);
		return -1;
	}

	result_canonical_block->block_num = blk->number;
	result_canonical_block->flags     = blk->blkProcFlags;
	result_canonical_block->crc_type  = blk->crcType;
	result_canonical_block->type_code = blk->type;
	result_canonical_block->btsd_len  = blk->dataLength;
	return 0;
}

static int	ion_bsl_GetBlockMetadata(const BSL_BundleRef_t *bundle_ref,
			uint64_t block_num, BSL_CanonicalBlock_t
			*result_canonical_block)
{
	AcqWorkArea	*work;
	Bundle		*bundle;

	CHKERR(bundle_ref);
	CHKERR(bundle_ref->data);
	CHKERR(result_canonical_block);
	work = (AcqWorkArea *) (bundle_ref->data);
	bundle = &(work->bundle);
	memset(result_canonical_block, 0, sizeof(*result_canonical_block));
	if (block_num == 1)	/*	Payload block			*/
	{
		/*	bpSend() accepts an application data unit of
		 *	zero length, and a payload block of zero length
		 *	is likewise ineligible: there are no bytes to
		 *	sign or encrypt.  Decline it here rather than
		 *	report a target whose BTSD is empty.		*/

		if (bundle->payload.length == 0)
		{
			reportIneligibleTarget(1, PayloadBlk, KIND_EMPTY,
					CONTENT_EMPTY);
			return -3;
		}

		result_canonical_block->block_num = 1;
		result_canonical_block->flags = bundle->payloadBlockProcFlags;
		result_canonical_block->crc_type = bundle->payload.crcType;
		result_canonical_block->type_code = PayloadBlk;
		result_canonical_block->btsd_len = bundle->payload.length;
		return 0;
	}

	/*	It's an extension block.				*/

	if (work->rawBundle == 0)
	{
		/*	Null AcqWorkArea.				*/

		if (getBlockMetadataFromSdr(bundle, block_num,
				result_canonical_block) < 0)
		{
			return -3;
		}
	}
	else
	{
		if (getBlockMetadataFromRAM(work, block_num,
				result_canonical_block) < 0)
		{
			return -3;
		}
	}

	return 0;
}

/*	 *	Operations on extension blocks	*	*	*	*/

static int	createBlockInSdr(uint64_t block_type_code,
			uint64_t *result_block_num, Bundle *bundle)
{
	Sdr		sdr = getIonsdr();
	ExtensionBlock	newBlock;
	SdrObject	bytesObj;

	memset((char *) &newBlock, 0, sizeof(ExtensionBlock));
	newBlock.type = block_type_code;
	newBlock.crcType = 0;				/*	No CRC	*/
       	/*	Flag 1 for BCB, 0 for BIB.				*/
	newBlock.blkProcFlags = block_type_code == 12 ? 1 : 0;
	newBlock.length = 1;				/*	Initial size	*/

	/*	Allocate SDR space for the bytes array. BSL will write
	 *	security block data here. The bytes field must be an SDR
	 *	address (offset), not a pointer to working memory.	*/

	bytesObj = sdr_malloc(sdr, newBlock.length);
	if (bytesObj == 0)
	{
		putErrmsg("Can't allocate SDR space for block bytes.", NULL);
		*result_block_num = 0;
		return -1;
	}

	newBlock.bytes = bytesObj;			/*	SDR address	*/
	newBlock.dataLength = 0;			/*	No data yet	*/

	{
		char	msgbuf[128];

		isprintf(msgbuf, sizeof msgbuf,
			"[BSL-CREATE-SDR] Created type %lu: bytes=%lu len=%lu dataLen=%lu",
			(unsigned long) block_type_code,
			(unsigned long) bytesObj,
			(unsigned long) newBlock.length,
			(unsigned long) newBlock.dataLength);
		writeMemo(msgbuf);
	}
	if (attachExtensionBlock(block_type_code, &newBlock, bundle) == 0)
	{
		sdr_free(sdr, bytesObj);		/*	Clean up	*/
		*result_block_num = 0;			/*	Error.		*/
		writeMemo("[?] Failed to attach extension block.");
		return -1;
	}

	*result_block_num = newBlock.number;
	if (newBlock.number == 0)
	{
		writeMemo("[?] WARNING: Block number is 0 after attach!");
	}
	else
	{
		char	msgbuf[128];

		isprintf(msgbuf, sizeof msgbuf,
				"[i] Created block type %d, number %d",
				(int) block_type_code, (int) newBlock.number);
		writeMemo(msgbuf);
	}

	return 0;
}

static int	createBlockInRAM(uint64_t block_type_code,
			uint64_t *result_block_num, AcqWorkArea *work)
{
	unsigned char	maxBlkNumber = 1;	/*	Payload.	*/
	LystElt		elt;
	AcqExtBlock	*blk;

	for (elt = lyst_first(work->extBlocks); elt; elt = lyst_next(elt))
	{
		blk = (AcqExtBlock *) lyst_data(elt);
		if (blk->number > maxBlkNumber)
		{
			maxBlkNumber = blk->number;
		}
	}

	blk = (AcqExtBlock *) MTAKE(sizeof(AcqExtBlock));
	CHKERR(blk);
	memset((char *) blk, 0, sizeof(AcqExtBlock));
	blk->type = block_type_code;
	blk->crcType = 0;				/*	No CRC	*/
       	/*	Flag 1 for BCB, 0 for BIB.				*/
	blk->blkProcFlags = block_type_code == 12 ? 1 : 0;
	blk->length = 1;
	blk->dataLength = blk->length + sizeof(AcqExtBlock);
	blk->number = maxBlkNumber + 1;
	elt = lyst_insert_last(work->extBlocks, blk);
	if (elt == NULL)
	{
		MRELEASE(blk);
		*result_block_num = 0;		/*	Error.		*/
		return -1;
	}

	*result_block_num = blk->number;
	return 0;
}

static int	ion_bsl_CreateBlock(BSL_BundleRef_t *bundle_ref,
			uint64_t block_type_code, uint64_t *result_block_num)
{
	AcqWorkArea	*work;
	Bundle		*bundle;

	CHKERR(bundle_ref);
	CHKERR(bundle_ref->data);
	CHKERR(result_block_num);
	work = (AcqWorkArea *) (bundle_ref->data);
	bundle = &work->bundle;
	if (work->rawBundle == 0)
	{
		/*	Null AcqWorkArea.				*/

		if (createBlockInSdr(block_type_code, result_block_num, bundle)
				< 0)
		{
			return -2;
		}
	}
	else
	{
		if (createBlockInRAM(block_type_code, result_block_num, work)
				< 0)
		{
			return -2;
		}
	}

	return 0;
}

static int	ion_bsl_RemoveBlock(BSL_BundleRef_t *bundle_ref,
			uint64_t block_num)
{
	AcqWorkArea	*work;
	Bundle		*bundle;
	SdrObject	bundleBlkElt;
	LystElt		workBlkElt;

	work = (AcqWorkArea *) (bundle_ref->data);
	bundle = &(work->bundle);
	if (work->rawBundle == 0)
	{
		/*	Null AcqWorkArea.				*/

		bundleBlkElt = getExtensionBlock(bundle, block_num);
		if (bundleBlkElt)		/*	Found it.	*/
		{
			deleteExtensionBlock(bundleBlkElt,
					&(bundle->extensionsLength));
			return 0;
		}
	}
	else
	{
		workBlkElt = getAcqExtensionBlock(work, block_num);
		if (workBlkElt)			/*	Found it.	*/
		{
			deleteAcqExtBlock(workBlkElt);
			return 0;
		}
	}

	return -2;
}

/*	 *	Operations on bundles	*	*	*	*	*/

int	ion_bsl_DeleteBundle(BSL_BundleRef_t *bundle_ref,
		BSL_ReasonCode_t reason)
{
	AcqWorkArea	*work;
	Bundle		*bundle;

	CHKERR(bundle_ref);
	CHKERR(bundle_ref->data);
	work = (AcqWorkArea *) (bundle_ref->data);
	bundle = &(work->bundle);

	// Mark the bundle for deletion

	bundle->insecure = 1;
	BSL_LOG_INFO("BSL indicated to delete bundle with reason code %" PRIi64,
			reason);
	return 0;
}

/*	 *	I/O functions for canonical block type-specific data	*/

typedef struct
{
	AcqWorkArea	*work;
	uvast		blockNbr;
	size_t		position;	/*	Current read/write position	*/
	ZcoReader	reader;		/*	Persistent ZCO reader		*/
	int		readerInitialized;	/*	Reader started?		*/
} BtsdIoRef;

static int	ion_bsl_ReallocBTSD(BSL_BundleRef_t *bundle_ref,
			uint64_t block_num, size_t bytesize)
{
	Sdr		sdr = getIonsdr();
	AcqWorkArea	*work;
	Bundle		*bundle;
	SdrObject	elt;
#if 0
	SdrObject	addr;
	ExtensionBlock	blk;
	unsigned char	*buffer;
#endif
	CHKERR(bundle_ref);
	CHKERR(bundle_ref->data);
	work = (AcqWorkArea *) (bundle_ref->data);
	bundle = &(work->bundle);
	/* During transmit, bundle is in AcqWorkArea (memory), not SDR. */
	CHKERR(block_num > 0);
	CHKERR(bytesize > 0);

	{
		char	msgbuf[128];

		isprintf(msgbuf, sizeof msgbuf,
			"[BSL-REALLOC] Block %lu: requested size=%lu",
			(unsigned long) block_num,
			(unsigned long) bytesize);
		writeMemo(msgbuf);
	}

	/*	Block 1 is the payload, stored as a ZCO (not an extension block).
	 *	zco_revise() handles dynamic expansion, so no realloc needed.	*/
	if (block_num == 1)
	{
		writeMemo("[BSL-REALLOC] Block 1 (payload): zco_revise will handle expansion");
		return 0;	/*	Success - no action needed	*/
	}

	/* During transmit, bundle is already in memory (AcqWorkArea),
	 * no need to sdr_read it. */
	/* sdr_read(sdr, (char *) &bundle, bundleObj, sizeof(Bundle)); */
	elt = getExtensionBlock(bundle, block_num);
	if (elt == 0)		/*	Block not found.		*/
	{
		return -3;
	}

	/*	In ION, extension blocks reside in the SDR heap
	 *	rather than in system memory. We need to:
	 *	1. Read bundle and find the block
	 *	2. Allocate new SDR space of requested size
	 *	3. Copy existing BTSD to new space
	 *	4. Free old space
	 *	5. Update block to point to new space
	 *	6. Update bundle overhead accounting		*/

	SdrObject	addr;
	ExtensionBlock	blk;
	SdrObject	newBytesObj;
	unsigned char	*tempBuf = NULL;
	int		oldSize;
	int		overhead_delta;

	addr = sdr_list_data(sdr, elt);
	CHKERR(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &blk, addr, sizeof(ExtensionBlock));

	/*	Allocate new SDR space for expanded bytes array.	*/
	newBytesObj = sdr_malloc(sdr, bytesize);
	if (newBytesObj == 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't allocate SDR space for block realloc.", NULL);
		return -1;
	}

	/*	Copy existing data to new location if any exists.	*/
	oldSize = blk.length;
	if (oldSize > 0 && blk.bytes != 0)
	{
		tempBuf = MTAKE(oldSize);
		if (tempBuf == NULL)
		{
			sdr_free(sdr, newBytesObj);
			sdr_cancel_xn(sdr);
			putErrmsg("No memory for block realloc buffer.", NULL);
			return -1;
		}

		sdr_read(sdr, (char *) tempBuf, blk.bytes, oldSize);
		sdr_write(sdr, newBytesObj, (char *) tempBuf, oldSize);
		MRELEASE(tempBuf);

		/*	Free old SDR space.				*/
		sdr_free(sdr, blk.bytes);
	}

	/*	Update block to use new bytes array.  For newly
	 *	created blocks (BSL source), bytes holds raw BTSD
	 *	only (no CBOR header yet), so dataLength == length.
	 *	This ensures the write overflow check passes and
	 *	the read/write offset (length - dataLength) is 0.	*/

	overhead_delta = bytesize - blk.length;
	blk.bytes = newBytesObj;
	blk.length = bytesize;
	blk.dataLength = bytesize;
	sdr_write(sdr, addr, (char *) &blk, sizeof(ExtensionBlock));

	/*	Update bundle overhead accounting.			*/
	bundle->dbOverhead += overhead_delta;
	/* During transmit, bundle is in memory (AcqWorkArea), not SDR.
	 * The in-memory update above is sufficient. */
	/* sdr_write(sdr, bundleObj, (char *) bundle, sizeof(Bundle)); */

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Failed to commit block realloc.", NULL);
		return -1;
	}

	{
		char	msgbuf[128];

		isprintf(msgbuf, sizeof msgbuf,
			"[BSL-REALLOC] Block %lu: OLD len=%d NEW len=%lu bytes=%lu overhead_delta=%d",
			(unsigned long) block_num, oldSize,
			(unsigned long) bytesize,
			(unsigned long) newBytesObj, overhead_delta);
		writeMemo(msgbuf);
	}

	return 0;

	/*	Note: this function is only able to resize
	 *	extension blocks, not all canonical blocks.  It
	 *	does not resize the payload block.  For reference,
	 *	see the MockBPA_ReallocBTSD function, shown below.	*/
#if 0
	if (!bundle_ref || !bundle_ref->data || block_num == 0 || bytesize == 0)
	{
		return -1;
	}

	MockBPA_Bundle_t *bundle = bundle_ref->data;

	MockBPA_CanonicalBlock_t **found_ptr = MockBPA_BlockByNum_get(bundle->blocks_num, block_num);
	if (found_ptr == NULL)
	{
		return -3;
	}
	MockBPA_CanonicalBlock_t *found_block = *found_ptr;

	if (found_block->btsd == NULL)
	{
		found_block->btsd	 = BSL_CALLOC(1, bytesize);
		found_block->btsd_len = bytesize;
	}
	else
	{
		found_block->btsd	 = BSL_REALLOC(found_block->btsd, bytesize);
		found_block->btsd_len = bytesize;
	}

	// Return -9 if malloc/realloc faile. Return 0 for success.
	return (found_block->btsd == NULL) ? -9 : 0;
#endif
}

static void	destroyBtsdIoRef(void *user_data)
{
	CHKVOID(user_data);
	BSL_FREE(user_data);
}

static int	readBTSDfromSdr(BtsdIoRef *ref, void *buf, size_t *bufsize)
{
	Sdr		sdr = getIonsdr();
	AcqWorkArea	*work;
	Bundle		*bundle;
	vast		bytesReceived;
	SdrObject	elt;
	SdrObject	addr;
			OBJ_POINTER(ExtensionBlock, blk);
	SdrAddress	startOfBTSD;
	size_t		bytesAvailable;

	work = ref->work;
	bundle = &(work->bundle);
	if (ref->blockNbr == 1)	/*	Reading from payload (ADU).	*/
	{
		/*	Initialize reader on first access.		*/

		if (!ref->readerInitialized)
		{
			zco_start_receiving(bundle->payload.content,
					&ref->reader);
			ref->readerInitialized = 1;
		}

		/*	Read from persistent reader.			*/

		bytesReceived = zco_receive_source(sdr, &ref->reader,
				*bufsize, buf);
		*bufsize = bytesReceived;
		ref->position += bytesReceived;

		return 0;
	}

	/*	Reading from extension block type specific data.	*/

	elt = getExtensionBlock(bundle, ref->blockNbr);
	if (elt == 0)		/*	Block not found.		*/
	{
		return -3;
	}

	addr = sdr_list_data(sdr, elt);
	GET_OBJ_POINTER(sdr, ExtensionBlock, blk, addr);

	/*	Never read outside the block.  A block whose content
	 *	ION has not yet produced has nothing to read at all,
	 *	and a caller that asks for more bytes than the block
	 *	holds gets only the bytes that are actually there;
	 *	otherwise the read would run into unrelated SDR heap.	*/

	if (sdrBlockHasNoBtsd(blk) || blk->length < blk->dataLength)
	{
		*bufsize = 0;
		return -3;
	}

	if (ref->position >= blk->dataLength)
	{
		*bufsize = 0;	/*	End of BTSD.			*/
		return 0;
	}

	bytesAvailable = blk->dataLength - ref->position;
	if (*bufsize > bytesAvailable)
	{
		*bufsize = bytesAvailable;
	}

	/*	blk->bytes contains the full CBOR extension block
	 *	(array header + fields + BTSD byte string + CRC).
	 *	The BTSD content starts at offset
	 *	(blk->length - blk->dataLength) within blk->bytes,
	 *	just like the RAM path in readBTSDfromRAM().		*/

	startOfBTSD = blk->bytes + (blk->length - blk->dataLength)
			+ ref->position;
	sdr_read(sdr, buf, startOfBTSD, *bufsize);
	ref->position += *bufsize;	/*	Update position after read	*/
	return 0;
}

static int	readBTSDfromRAM(BtsdIoRef *ref, void *buf, size_t *bufsize)
{
	Sdr		sdr = getIonsdr();
	AcqWorkArea	*work;
	Bundle		*bundle;
	vast		bytesReceived;
	LystElt		elt;
	AcqExtBlock	*blk;
	unsigned char	*startOfBTSD;
	size_t		bytesAvailable;

	work = ref->work;
	bundle = &(work->bundle);
	if (ref->blockNbr == 1)	/*	Reading from payload (ADU).	*/
	{
		/*	Initialize reader on first access.		*/

		if (!ref->readerInitialized)
		{
			zco_start_receiving(bundle->payload.content,
					&ref->reader);
			ref->readerInitialized = 1;
		}

		/*	Read from persistent reader.			*/

		bytesReceived = zco_receive_source(sdr, &ref->reader,
				*bufsize, buf);
		*bufsize = bytesReceived;
		ref->position += bytesReceived;

		return 0;
	}

	/*	Reading from extension block type specific data.	*/

	elt = getAcqExtensionBlock(work, ref->blockNbr);
	if (elt == NULL)	/*	Block not found.		*/
	{
		return -3;
	}

	blk = (AcqExtBlock *) lyst_data(elt);

	/*	Never read outside the block; see readBTSDfromSdr().	*/

	if (ramBlockHasNoBtsd(blk) || blk->length < blk->dataLength)
	{
		*bufsize = 0;
		return -3;
	}

	if (ref->position >= blk->dataLength)
	{
		*bufsize = 0;	/*	End of BTSD.			*/
		return 0;
	}

	bytesAvailable = blk->dataLength - ref->position;
	if (*bufsize > bytesAvailable)
	{
		*bufsize = bytesAvailable;
	}

	/*	blk->bytes contains the full CBOR extension block
	 *	(array header + fields + BTSD byte string + CRC).
	 *	The BTSD content starts at offset
	 *	(blk->length - blk->dataLength) within blk->bytes.	*/

	startOfBTSD = blk->bytes + (blk->length - blk->dataLength)
			+ ref->position;
	memcpy(buf, startOfBTSD, *bufsize);
	ref->position += *bufsize;	/*	Update position after read	*/
	return 0;
}

static int	readBTSD(void *user_data, void *buf, size_t *bufsize)
{
	BtsdIoRef	*ref;

	CHKERR(user_data);
	CHKERR(buf);
	CHKERR(bufsize);
	ref = (BtsdIoRef *) user_data;
	if (ref->work->rawBundle == 0)
	{
		return readBTSDfromSdr(ref, buf, bufsize);
	}
	else
	{
		return readBTSDfromRAM(ref, buf, bufsize);
	}
}

static struct BSL_SeqReader_s	*ion_bsl_BTSD_reader(const BSL_BundleRef_t
					*bundle_ref, uint64_t block_num)
{
	BSL_SeqReader_t		*reader;
	BtsdIoRef		*ref;
	BSL_CanonicalBlock_t	metadata;

	CHKNULL(bundle_ref);
	CHKNULL(block_num > 0);

	/*	Note: this function reads only canonical blocks
	 *	(extension blocks and payload), not primary block.
	 *	The primary block has no BTSD.				*/

	/*	Decline to open a reader on a block that has no BTSD
	 *	to read, so that a caller cannot be handed a reader
	 *	that can only fail.					*/

	if (ion_bsl_GetBlockMetadata(bundle_ref, block_num, &metadata) != 0)
	{
		return NULL;
	}

	reader = (BSL_SeqReader_t *) BSL_CALLOC(1, sizeof(BSL_SeqReader_t));
	CHKNULL(reader);
	ref = (BtsdIoRef *) BSL_MALLOC(sizeof(BtsdIoRef));
	if (ref == NULL)
	{
		BSL_FREE(reader);
		return NULL;
	}

	ref->work = (AcqWorkArea *) (bundle_ref->data);
	ref->blockNbr = block_num;
	ref->position = 0;	/*	Start at beginning		*/
	ref->readerInitialized = 0;	/*	Reader not started yet	*/
	reader->user_data = ref;
	reader->read = readBTSD;
	reader->deinit = destroyBtsdIoRef;
	return reader;
}

static int	writeBTSDtoSdr(BtsdIoRef *ref, const void *buf, size_t size)
{
	Sdr		sdr = getIonsdr();
	AcqWorkArea	*work;
	Bundle		*bundle;
	SdrObject	elt;
	SdrObject	addr;
			OBJ_POINTER(ExtensionBlock, blk);
	SdrAddress	startOfBTSD;
	char		*mutableBuf;

	work = ref->work;
	bundle = &(work->bundle);

	/*	ION's zco_revise and sdr_write APIs don't use const,
	 *	so we need a mutable copy of the buffer.		*/

	mutableBuf = MTAKE(size);
	if (mutableBuf == NULL)
	{
		return BSL_ERR_FAILURE;
	}

	memcpy(mutableBuf, buf, size);

	if (ref->blockNbr == 1)	/*	Writing to payload (ADU);	*/
	{
		int	result;
		char	msgbuf[256];
		vast	zcoLen;

		zcoLen = zco_length(sdr, bundle->payload.content);
		isprintf(msgbuf, sizeof msgbuf,
			"[BSL-WRITE-SDR] Payload write: zco=%lu zcoLen=%ld pos=%lu size=%lu",
			(unsigned long) bundle->payload.content, (long) zcoLen,
			(unsigned long) ref->position,
			(unsigned long) size);
		writeMemo(msgbuf);

		result = zco_revise(sdr, bundle->payload.content,
				ref->position, mutableBuf, size);
		MRELEASE(mutableBuf);

		isprintf(msgbuf, sizeof msgbuf,
			"[BSL-WRITE-SDR] zco_revise result=%d", result);
		writeMemo(msgbuf);

		if (result < 0)
		{
			putErrmsg("zco_revise failed for payload", NULL);
			return BSL_ERR_FAILURE;
		}

		ref->position += size;	/*	Update position after write	*/
		return BSL_SUCCESS;
	}

	/*	Writing to extension block type specific data.		*/

	elt = getExtensionBlock(bundle, ref->blockNbr);
	if (elt == 0)		/*	Block not found.		*/
	{
		char	msgbuf[128];

		isprintf(msgbuf, sizeof msgbuf,
			"[BSL-WRITE-SDR] ERROR: Block %llu not found",
			(unsigned long long) ref->blockNbr);
		writeMemo(msgbuf);
		MRELEASE(mutableBuf);
		return -3;
	}

	addr = sdr_list_data(sdr, elt);
	GET_OBJ_POINTER(sdr, ExtensionBlock, blk, addr);

	/*	blk->bytes contains the full CBOR extension block.
	 *	Offset past the header to reach the BTSD content.	*/

	startOfBTSD = blk->bytes + (blk->length - blk->dataLength)
			+ ref->position;

	{
		char	msgbuf[256];

		isprintf(msgbuf, sizeof msgbuf,
			"[BSL-WRITE-SDR] ExtBlk %lu: bytes=%lu len=%lu dataLen=%lu pos=%lu size=%lu addr=%lu",
			(unsigned long) ref->blockNbr,
			(unsigned long) blk->bytes,
			(unsigned long) blk->length,
			(unsigned long) blk->dataLength,
			(unsigned long) ref->position,
			(unsigned long) size,
			(unsigned long) startOfBTSD);
		writeMemo(msgbuf);

		/* Verify we're not writing beyond allocated space */
		if (ref->position + size > blk->dataLength)
		{
			isprintf(msgbuf, sizeof msgbuf,
				"[BSL-WRITE-SDR] ERROR: Write would overflow! pos=%lu size=%lu length=%lu",
				(unsigned long) ref->position,
				(unsigned long) size,
				(unsigned long) blk->length);
			writeMemo(msgbuf);
			MRELEASE(mutableBuf);
			return BSL_ERR_FAILURE;
		}
	}

	sdr_write(sdr, startOfBTSD, mutableBuf, size);
	ref->position += size;	/*	Update position after write	*/
	MRELEASE(mutableBuf);

	writeMemo("[BSL-WRITE-SDR] ExtBlk write completed successfully");
	return BSL_SUCCESS;
}

static int	writeBTSDtoRAM(BtsdIoRef *ref, const void *buf, size_t size)
{
	Sdr		sdr = getIonsdr();
	AcqWorkArea	*work;
	Bundle		*bundle;
	LystElt		elt;
	AcqExtBlock	*blk;
	unsigned char	*startOfBTSD;
	char		*mutableBuf;

	work = ref->work;
	bundle = &(work->bundle);
	if (ref->blockNbr == 1)	/*	Writing to payload (ADU);	*/
	{
		int	result;
		char	msgbuf[256];
		vast	zcoLen;

		/*	ION's zco_revise API doesn't use const,
		 *	so we need a mutable copy of the buffer.	*/

		mutableBuf = MTAKE(size);
		if (mutableBuf == NULL)
		{
			return BSL_ERR_FAILURE;
		}

		zcoLen = zco_length(sdr, bundle->payload.content);
		isprintf(msgbuf, sizeof msgbuf,
			"[BSL-WRITE-RAM] Payload write: zco=%lu zcoLen=%ld pos=%lu size=%lu",
			(unsigned long) bundle->payload.content, (long) zcoLen,
			(unsigned long) ref->position,
			(unsigned long) size);
		writeMemo(msgbuf);

		memcpy(mutableBuf, buf, size);
		result = zco_revise(sdr, bundle->payload.content,
				ref->position, mutableBuf, size);
		MRELEASE(mutableBuf);

		isprintf(msgbuf, sizeof msgbuf,
			"[BSL-WRITE-RAM] zco_revise result=%d", result);
		writeMemo(msgbuf);

		if (result < 0)
		{
			putErrmsg("zco_revise failed for payload", NULL);
			return BSL_ERR_FAILURE;
		}

		ref->position += size;	/*	Update position after write	*/
		return BSL_SUCCESS;
	}

	/*	Writing to extension block type specific data.		*/

	elt = getAcqExtensionBlock(work, ref->blockNbr);
	if (elt == 0)		/*	Block not found.		*/
	{
		char	msgbuf[128];

		isprintf(msgbuf, sizeof msgbuf,
			"[BSL-WRITE-RAM] ERROR: Block %llu not found",
			(unsigned long long) ref->blockNbr);
		writeMemo(msgbuf);
		return -3;
	}

	blk = (AcqExtBlock *) lyst_data(elt);
	startOfBTSD = blk->bytes + ref->position;

	{
		char	msgbuf[256];

		isprintf(msgbuf, sizeof msgbuf,
			"[BSL-WRITE-RAM] ExtBlk %lu: bytes=%p len=%lu dataLen=%lu pos=%lu size=%lu addr=%p",
			(unsigned long) ref->blockNbr,
			(void *) blk->bytes,
			(unsigned long) blk->length,
			(unsigned long) blk->dataLength,
			(unsigned long) ref->position,
			(unsigned long) size,
			(void *) startOfBTSD);
		writeMemo(msgbuf);

		/* Verify we're not writing beyond allocated space */
		if (ref->position + size > blk->length)
		{
			isprintf(msgbuf, sizeof msgbuf,
				"[BSL-WRITE-RAM] ERROR: Write would overflow! pos=%lu size=%lu length=%lu",
				(unsigned long) ref->position,
				(unsigned long) size,
				(unsigned long) blk->length);
			writeMemo(msgbuf);
			return BSL_ERR_FAILURE;
		}
	}

	memcpy(startOfBTSD, buf, size);
	ref->position += size;	/*	Update position after write	*/

	writeMemo("[BSL-WRITE-RAM] ExtBlk write completed successfully");
	return BSL_SUCCESS;
}

static int	writeBTSD(void *user_data, const void *buf, size_t size)
{
	BtsdIoRef	*ref;

	CHKERR(user_data);
	CHKERR(buf);
	ref = (BtsdIoRef *) user_data;
	if (ref->work->rawBundle == 0)
	{
		return writeBTSDtoSdr(ref, buf, size);
	}
	else
	{
		return writeBTSDtoRAM(ref, buf, size);
	}
}

static struct BSL_SeqWriter_s	*ion_bsl_BTSD_writer(BSL_BundleRef_t
					*bundle_ref, uint64_t block_num,
					size_t totalSize)
{
	BSL_SeqWriter_t	*writer;
	BtsdIoRef	*ref;

	CHKNULL(bundle_ref);
	CHKNULL(block_num > 0);

	/*	BSL v1.1 passes the total BTSD size upfront;
	 *	pre-allocate the block to avoid overflow on write.	*/
	if (totalSize > 0)
	{
		if (ion_bsl_ReallocBTSD(bundle_ref, block_num,
				totalSize) < 0)
		{
			return NULL;
		}
	}

	/*	Note: this function writes only canonical blocks
	 *	(extension blocks and payload), not primary block.
	 *	The primary block has no BTSD.				*/

	writer = (BSL_SeqWriter_t *) BSL_CALLOC(1, sizeof(BSL_SeqWriter_t));
	CHKNULL(writer);
	ref = (BtsdIoRef *) BSL_MALLOC(sizeof(BtsdIoRef));
	if (ref == NULL)
	{
		BSL_FREE(writer);
		return NULL;
	}

	ref->work = (AcqWorkArea *) (bundle_ref->data);
	ref->blockNbr = block_num;
	ref->position = 0;	/*	Start at beginning		*/
	ref->readerInitialized = 0;	/*	Reader not started yet	*/
	writer->user_data = ref;
	writer->write = writeBTSD;
	writer->deinit = destroyBtsdIoRef;
	return writer;
}

/*	 *	Operations on EIDs	*	*	*	*	*/

#include <inttypes.h>
#include <BSLConfig.h>
#include <BPSecLib_Private.h>

static void	 ion_bsl_eid_init(EndpointId *eid)
{
	CHKVOID(eid);
	memset(eid, 0, sizeof(EndpointId));
}

static void	 ion_bsl_eid_deinit(EndpointId *eid)
{
	CHKVOID(eid);
	eraseEid(eid);
}

static void	ion_bsl_eid_load(void *user_data _U_,
			BSL_HostEID_t *eidWrapper)
{
	CHKVOID(eidWrapper);
	memset(eidWrapper, 0, sizeof(BSL_HostEID_t));
	eidWrapper->handle = BSL_MALLOC(sizeof(EndpointId));
	if (!(eidWrapper->handle))
	{
		return;
	}

	ion_bsl_eid_init(eidWrapper->handle);
}

static void	ion_bsl_eid_unload(void *user_data _U_,
			BSL_HostEID_t *eidWrapper)
{
	CHKVOID(eidWrapper);
	if (eidWrapper->handle)
	{
		ion_bsl_eid_deinit(eidWrapper->handle);
		BSL_FREE(eidWrapper->handle);
	}

	memset(eidWrapper, 0, sizeof(BSL_HostEID_t));
}

static int	ion_bsl_encode_eid(const BSL_HostEID_t *eidWrapper,
			BSL_Data_t *cborText)
{
	unsigned char	buffer[300];
	EndpointId	*eid;
	int		length;

	BSL_CHKERR1(eidWrapper);
	ASSERT_ARG_NONNULL(eidWrapper->handle);
	eid = (EndpointId *) (eidWrapper->handle);
	length = serializeEid(eid, buffer);
	if (length < 1)
	{
		return 0;	/*	Failure.			*/
	}

	/*	If cborText is NULL this is a size query;
	 *	return the encoded length without copying.	*/

	if (cborText == NULL)
	{
		return length;
	}

	if ((size_t) length <= cborText->len)
	{
		memcpy(cborText->ptr, buffer, length);
	}

	/*	If encoded length exceeds available space we return
	 *	the length without copying the encoded data into
	 *	the BSL_Data_t object.					*/

	return length;
}

static int	ion_bsl_decode_eid(const BSL_Data_t *cborText,
			BSL_HostEID_t *eidWrapper)
{
	unsigned char	*buffer;
	unsigned int	bytesRemaining;
	unsigned char	*cursor;
	EndpointId	*eid;

	BSL_CHKERR1(cborText);
	ASSERT_ARG_NONNULL(cborText->ptr);
	BSL_CHKERR1(eidWrapper);
	ASSERT_ARG_NONNULL(eidWrapper->handle);
	buffer = cborText->ptr;
	bytesRemaining = cborText->len;
	eid = (EndpointId *) (eidWrapper->handle);
	cursor = buffer;
	if (acquireEid(eid, &cursor, &bytesRemaining) == 0)
	{
		return 2;	/*	Failure.			*/
	}

	return 0;
}

static int	ion_bsl_eid_from_text(BSL_HostEID_t *eidWrapper,
			const char *text, void *user_data _U_)
{
	EndpointId	*eid;
	MetaEid		metaEid;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	char		*mutableText;
	int		result;

	BSL_CHKERR1(text);
	BSL_CHKERR1(eidWrapper);
	ASSERT_ARG_NONNULL(eidWrapper->handle);
	eid = (EndpointId *) (eidWrapper->handle);

	/*	Create mutable copy for parseEidString.		*/
	mutableText = MTAKE(strlen(text) + 1);
	if (mutableText == NULL)
	{
		return 2;	/*	Failure.			*/
	}

	strcpy(mutableText, text);
	result = parseEidString(mutableText, &metaEid, &vscheme, &vschemeElt);
	MRELEASE(mutableText);

	if (result == 0 || jotEid(eid, &metaEid) < 0)
	{
		return 2;	/*	Failure.			*/
	}

	return 0;
}
#if 0
static int	ion_bsl_eid_to_text(char **text,
			const BSL_HostEID_t *eidWrapper, void *user_data _U_)
{
	EndpointId	*eid;

	BSL_CHKERR1(text);
	BSL_CHKERR1(eidWrapper);
	ASSERT_ARG_NONNULL(eidWrapper->handle);
	eid = (EndpointId *) (eidWrapper->handle);
	readEid(eid, text);
	if (*text == NULL)
	{
		return 2;	/*	Failure.			*/
	}

	return 0;
}
#endif
static int	ion_bsl_GetEid(void *user_data, BSL_HostEID_t *result_eid)
{
	static SdrObject bpDbObject = 0;
	Sdr		sdr;
	BpDB		bpdb;
	static char	localIpnEid[SDRSTRING_BUFSZ];

	/*	This function simply retrieves the canonical EID for
	 *	the BP nodes in which this BSL Agent resides.		*/

	if (bpDbObject == 0)
	{
		sdr = getIonsdr();
		if (sdr == NULL)
		{
			writeMemo("[?] BSL EID can't find SDR.");
			return 1;
		}

		bpDbObject = getBpDbObject();
		if (bpDbObject == 0)
		{
			writeMemo("[?] BSL EID can't find ION DB object.");
			return 1;
		}

		sdr_read(sdr, (char *) &bpdb, bpDbObject, sizeof(BpDB));
		if (bpdb.bslLocalEid == 0)
		{
			writeMemo("[?] BSL EID fails: no BSL key file name.");
                        return 1;
		}

		sdr_string_read(sdr, localIpnEid, bpdb.bslLocalEid);
	}

	return (ion_bsl_eid_from_text(result_eid, localIpnEid, user_data))
			== 0 ? 0 : -1;
}

/*	 *	Operations on EID patterns	*	*	*	*/

/*	NOTE: the implementations of EID pattern operations functions
 *	are in bpv7/library/libbp.c, alongside the implementations
 *	EID functions.							*/

static int	ion_bsl_eidpat_load(BSL_HostEIDPattern_t *patWrapper,
			void *user_data _U_)
{
	BSL_CHKERR1(patWrapper); // GCOV_EXCL_LINE
	memset(patWrapper, 0, sizeof(BSL_HostEIDPattern_t));
	patWrapper->handle = createEidPattern();
	return (patWrapper->handle == NULL ? 2 : 0);
}

static void	ion_bsl_eidpat_unload(BSL_HostEIDPattern_t *patWrapper,
			void *user_data _U_)
{
	EidPattern	*eidp;

	CHKVOID(patWrapper);
	if (patWrapper->handle)
	{
		eidp = (EidPattern *) (patWrapper->handle);
		destroyEidPattern(eidp);
	}

	memset(patWrapper, 0, sizeof(BSL_HostEIDPattern_t));
}

static int	ion_bsl_eidpat_from_text(BSL_HostEIDPattern_t *patWrapper,
			const char *text, void *user_data _U_)
{
	EidPattern	*eidp;

	// GCOV_EXCL_START
	BSL_CHKERR1(patWrapper);
	BSL_CHKERR1(text);
	eidp = (EidPattern *) patWrapper->handle;
	BSL_CHKERR1(eidp);
	// GCOV_EXCL_STOP

	char msgbuf[256];
	int result;
	isprintf(msgbuf, sizeof(msgbuf), "[DEBUG] Loading EID pattern: '%s'", text);
	writeMemo(msgbuf);
	result = loadEidPattern(eidp, text);
	isprintf(msgbuf, sizeof(msgbuf), "[DEBUG] loadEidPattern result: %d", result);
	writeMemo(msgbuf);

	return (result < 0 ? 2 : 0);
}

static bool	ion_bsl_eidpat_match(const BSL_HostEIDPattern_t *patWrapper,
			const BSL_HostEID_t *eidWrapper, void *user_data _U_)
{
	EidPattern	*eidp;
	EndpointId	*eid;

	// GCOV_EXCL_START
	BSL_CHKERR1(patWrapper);
	BSL_CHKERR1(patWrapper->handle);
	BSL_CHKERR1(eidWrapper);
	BSL_CHKERR1(eidWrapper->handle);
	// GCOV_EXCL_STOP
	eidp = (EidPattern *) (patWrapper->handle);
	eid = (EndpointId *) (eidWrapper->handle);

	char		msgbuf[512];
	char		*eidText = NULL;
	int		result;

	/* Debug: print the EID being matched */
	readEid(eid, &eidText);
	if (eidText != NULL)
	{
		isprintf(msgbuf, sizeof(msgbuf), "[DEBUG] Matching EID: '%s' against pattern", eidText);
		writeMemo(msgbuf);
		MRELEASE(eidText);
	}

	result = eidMatchesPattern(eidp, eid);
	isprintf(msgbuf, sizeof(msgbuf), "[DEBUG] eidMatchesPattern returned: %d", result);
	writeMemo(msgbuf);

	return (result ? true : false);
}


/******************* Functions that load BSL policies *******************/

#ifdef ION_BSL_TEST_VECTORS
/*
 * Test-only generator that returns the fixed RFC 9173 Appendix A key and IV
 * for byte-exact interop reproduction.  One hook shared by the key and IV
 * requests, disambiguated by length: a 12-byte request is the IV, anything
 * else is the content-encryption key.
 */
static int	rfc9173_bcb_cek(unsigned char *buf, int len)
{
	/*
	 * Adapted from mock_bpa_rfc9173_bcb_cek, with a bounds-checked key
	 * copy.
	 */

	static const uint8_t iv[] = { 0x54, 0x77, 0x65, 0x6c, 0x76, 0x65, 0x31,
		0x32, 0x31, 0x32, 0x31, 0x32 };
	static const uint8_t rfc9173A3_key[] = { 0x71, 0x77, 0x65, 0x72, 0x74,
		0x79, 0x75, 0x69, 0x6f, 0x70, 0x61, 0x73, 0x64, 0x66, 0x67,
		0x68 };

	if (len < 0)
	{
		return 0;
	}

	if (len == 12) // IV
	{
		memcpy(buf, iv, sizeof(iv));
	}
	else // A3 KEY
	{
		/*
		 * rfc9173A3_key is 16 bytes; a caller requesting a 32-byte key
		 * (AES-256, the RFC 9173 default when no AES variant is
		 * specified) must not read past it. Copy what we have and
		 * zero-fill the remainder.
		 */
		size_t avail = sizeof(rfc9173A3_key);
		size_t want = (size_t) len;
		size_t n = want < avail ? want : avail;

		memcpy(buf, rfc9173A3_key, n);
		if (want > n)
		{
			memset(buf + n, 0, want - n);
		}
	}

	return 1;
}
#endif /* ION_BSL_TEST_VECTORS */


/*************** Functions called by the host process *******************/

/// Size of the base16_decode_table
static const size_t	base64_decode_lim = 0x80;
// clang-format off
/// Decode table for base64 and base64uri
static const int base64_decode_table[0x80] = {
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -2, -2, -2, -2, -2, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, 62, -1, 63,
	52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
	-1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
	15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, 63,
	-1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
	41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
};
// clang-format on

/** Decode a single character.
 *
 * @param chr The character to decode.
 * @return If positive, the decoded value.
 * -1 to indicate error.
 * -2 to indicate whitespace.
 */
static int	base64_decode_char(uint8_t chr)
{
	if (chr >= base64_decode_lim)
	{
		return -1;
	}

	return base64_decode_table[chr];
}

static int	base64_decode(m_bstring_t out, const m_string_t in)
{
	/*	Stolen from mock_bpa_base64_decode().		*/

	size_t		in_len = m_string_size(in);
	const char 	*curs = m_string_get_cstr(in);

	BSL_CHKERR1(out);
	BSL_CHKERR1(in);

	size_t		out_len = (in_len / 4) * 3 + 2;
	m_bstring_resize(out, out_len);
	uint8_t		*out_curs = m_bstring_acquire_access(out, 0, out_len);

	int		retval = 0;
	for (; in_len >= 2; curs += 4, in_len -= 4)
	{
		// ignoring excess padding
		if (curs[0] == '=')
		{
			break;
		}

		const int seg0 = base64_decode_char(curs[0]);
		const int seg1 = base64_decode_char(curs[1]);
		if ((seg0 < 0) || (seg1 < 0))
		{
			retval = 3;
			break;
		}

		if (out_len)
		{
			const uint8_t byte = (seg0 << 2) | (seg1 >> 4);
			*(out_curs++)      = byte;
			--out_len;
		}

		if (in_len == 2)
		{
			// allow omitted padding
			in_len = 0;
			break;
		}
		else if (curs[2] == '=')
		{
			if (in_len != 4)
			{
				break;
			}

			if (curs[3] != '=')
			{
				break;
			}
		}
		else
		{
			const int seg2 = base64_decode_char(curs[2]);
			if (seg2 < 0)
			{
				retval = 3;
				break;
			}

			if (out_len)
			{
				const uint8_t byte =
					((seg1 << 4) & 0xF0) | (seg2 >> 2);
				*(out_curs++)      = byte;
				--out_len;
			}

			if (in_len == 3)
			{
				// allow omitted padding
				in_len = 0;
				break;
			}
			else if (curs[3] == '=')
			{
				if (in_len != 4)
				{
					break;
				}
			}
			else
			{
				const int seg3 = base64_decode_char(curs[3]);
				if (seg3 < 0)
				{
					retval = 3;
					break;
				}

				if (out_len)
				{
					const uint8_t byte = ((seg2 << 6)
							& 0xC0) | seg3;
					*(out_curs++) = byte;
					--out_len;
				}
			}
		}
	}

	// trim if necessary
	m_bstring_release_access(out);
	m_bstring_resize(out, m_bstring_size(out) - out_len);
	if (retval)
	{
		return retval;
	}

	// Per Section 3.3 of RFC 4648, ignoring excess padding
	while ((in_len > 0) && (*curs == '='))
	{
		++curs;
		--in_len;
	}

	return (in_len > 0) ? 4 : 0;
}

static int	key_registry_init(const char *pp_cfg_file_path)
{
	/*	Stolen from mock_bpa_key_registry_init().		*/

	int		retval = 0;
	json_t		*root;
	json_error_t	err;
	json_t		*keys;
	size_t		n;
	json_t		*key_obj;
	json_t		*kty;
	json_t		*kid;
	const char	*kid_str;
	json_t		*k;
	const char	*k_str;
	size_t		k_len;
	const uint8_t	*k_ptr;

	BSL_LOG_INFO("Reading keys from %s", pp_cfg_file_path);
	root = json_load_file(pp_cfg_file_path, 0, &err);
	if (!root)
	{
		BSL_LOG_ERR("JSON error: line %d: %s", err.line, err.text);
		json_decref(root);
		return 1;
	}

	keys = json_object_get(root, "keys");
	if (!keys || !json_is_array(keys))
	{
		BSL_LOG_ERR("Missing \"keys\" ");
		json_decref(root);
		return 1;
	}

	n = json_array_size(keys);
	BSL_LOG_INFO("Found %zu key objects", n);
	for (size_t i = 0; !retval && (i < n); ++i)
	{
		key_obj = json_array_get(keys, i);
		if (!json_is_object(key_obj))
		{
			continue;
		}

		kty = json_object_get(key_obj, "kty");
		if (!kty)
		{
			BSL_LOG_ERR("Missing \"kty\" ");
			continue;
		}

		if (0 != strcmp("oct", json_string_value(kty)))
		{
			BSL_LOG_ERR("Not a symmetric key set");
			continue;
		}

		kid = json_object_get(key_obj, "kid");
		if (!kid || !json_is_string(kid))
		{
			BSL_LOG_ERR("Missing \"kid\" ");
			continue;
		}

		kid_str = json_string_value(kid);
		BSL_LOG_DEBUG("kid: %s", kid_str);
		k = json_object_get(key_obj, "k");
		if (!k || !json_is_string(k))
		{
			BSL_LOG_ERR("Missing \"k\" ");
			continue;
		}

		k_str = json_string_value(k);
		BSL_LOG_DEBUG("k: %s", k_str);
		m_string_t k_text;
		m_string_init_set_cstr(k_text, k_str);
		m_bstring_t k_data;
		m_bstring_init(k_data);
		retval = base64_decode(k_data, k_text);
		if (!retval)
		{
			k_len = m_bstring_size(k_data);
			k_ptr = m_bstring_view(k_data, 0, k_len);
			retval = BSL_Crypto_AddRegistryKey(kid_str, k_ptr,
					k_len);
		}

		m_bstring_clear(k_data);
		m_string_clear(k_text);
	}

	json_decref(root);
	return retval;
}

#if 0
static void bslDumpTelemetry(BslAgent_t *agent)
{
	BSL_TlmCounters_t	tlm = BSL_TLM_COUNTERS_ZERO;
	int			result;

	{
		BslContext *ctxs[] = {
			&agent->transmit,
			&agent->deliver,
			&agent->receive,
			&agent->forward,
		};

		for (size_t ix = 0; ix < sizeof(ctxs) / sizeof(ctxs[0]); ++ix)
		{
			BslContext *ctx = ctxs[ix];

			if (pthread_mutex_lock(&ctx->mutex))
			{
				BSL_LOG_CRIT("failed to lock mutex");
				continue;
			}

			result = BSL_LibCtx_AccumulateTlmCounters(ctx->bsl,
					&tlm);
			if (result)
			{
				BSL_LOG_ERR("Error with reading telemetry \
from bsl context");
				// fall-through to unlock
			}

			if (pthread_mutex_unlock(&ctx->mutex))
			{
				BSL_LOG_CRIT("failed to unlock mutex");
			}
		}
	}

	BSL_LOG_INFO("---------------------------------------------------------");
	BSL_LOG_INFO("---------------------TELEMETRY INFO----------------------");
	BSL_LOG_INFO("					 FAIL COUNT:%" TLM_COUNTER_FMT, tlm.counters[BSL_TLM_SECOP_FAIL_COUNT]);
	BSL_LOG_INFO("		 BUNDLE INSPECTED COUNT:%" TLM_COUNTER_FMT, tlm.counters[BSL_TLM_BUNDLE_INSPECTED_COUNT]);
	BSL_LOG_INFO("			   ASB DECODE COUNT:%" TLM_COUNTER_FMT, tlm.counters[BSL_TLM_ASB_DECODE_COUNT]);
	BSL_LOG_INFO("			   ASB DECODE BYTES:%" TLM_COUNTER_FMT, tlm.counters[BSL_TLM_ASB_DECODE_BYTES]);
	BSL_LOG_INFO("			   ASB ENCODE COUNT:%" TLM_COUNTER_FMT, tlm.counters[BSL_TLM_ASB_ENCODE_COUNT]);
	BSL_LOG_INFO("			   ASB ENCODE BYTES:%" TLM_COUNTER_FMT, tlm.counters[BSL_TLM_ASB_ENCODE_BYTES]);
	BSL_LOG_INFO("			 SECOP SOURCE COUNT:%" TLM_COUNTER_FMT, tlm.counters[BSL_TLM_SECOP_SOURCE_COUNT]);
	BSL_LOG_INFO("		   SECOP VERIFIER COUNT:%" TLM_COUNTER_FMT, tlm.counters[BSL_TLM_SECOP_VERIFIER_COUNT]);
	BSL_LOG_INFO("		   SECOP ACCEPTOR COUNT:%" TLM_COUNTER_FMT, tlm.counters[BSL_TLM_SECOP_ACCEPTOR_COUNT]);
	BSL_LOG_INFO("				TOTAL TLM COUNT:%" TLM_COUNTER_FMT, tlm.counters[BSL_TLM_TOTAL_COUNT]);
	BSL_LOG_INFO("---------------------------------------------------------");
}
#endif

static int	initializeAgent(BslAgent *agent)
{
	/*	Note that each member of this array is a pointer to
	 *	one of the four BSL contexts of the agent.  We use
	 *	these pointers to initialize the agent's BSL contexts.	*/

	BslContext *ctxs[] =
	{
		&agent->transmit,
		&agent->deliver,
		&agent->receive,
		&agent->forward
	};

	BslContext		*ctx;
	BSL_SecCtxDesc_t	bib_sec_desc;
	BSL_SecCtxDesc_t	bcb_sec_desc;
	int			i;
	BSL_PolicyDesc_t	policy_callbacks;

	for (i = 0, ctx = ctxs[0]; i < 4; i++, ctx++)
	{
		ctx->bsl = BSL_CALLOC(1, BSL_LibCtx_Sizeof());
		if (BSL_API_InitLib(ctx->bsl))
		{
			BSL_LOG_ERR("Failed BSL_API_InitLib()");
			return -1;
		}

		if (pthread_mutex_init(&ctx->mutex, NULL))
		{
			BSL_LOG_ERR("Failed pthread_mutex_init()");
			return -1;
		}

		bib_sec_desc.execute  = BSLX_BIB_Execute;
		bib_sec_desc.validate = BSLX_BIB_Validate;
		ASSERT_PROPERTY(0 == BSL_API_RegisterSecurityContext(ctx->bsl,
				1, bib_sec_desc));
		bcb_sec_desc.execute  = BSLX_BCB_Execute;
		bcb_sec_desc.validate = BSLX_BCB_Validate;
		ASSERT_PROPERTY(0 == BSL_API_RegisterSecurityContext(ctx->bsl,
				2, bcb_sec_desc));
	}

	policy_callbacks.deinit_fn = BSLP_Deinit;
	policy_callbacks.query_fn = BSLP_QueryPolicy;
	policy_callbacks.finalize_fn = BSLP_FinalizePolicy;

	agent->transmit.policy = BSLP_PolicyProvider_Init(1);
	policy_callbacks.user_data = agent->transmit.policy;
	ASSERT_PROPERTY(BSL_SUCCESS ==
			BSL_API_RegisterPolicyProvider(agent->transmit.bsl,
			1, policy_callbacks));

	agent->deliver.policy = BSLP_PolicyProvider_Init(1);
	policy_callbacks.user_data = agent->deliver.policy;
	ASSERT_PROPERTY(BSL_SUCCESS ==
			BSL_API_RegisterPolicyProvider(agent->deliver.bsl,
			1, policy_callbacks));

	agent->receive.policy = BSLP_PolicyProvider_Init(1);
	policy_callbacks.user_data = agent->receive.policy;
	ASSERT_PROPERTY(BSL_SUCCESS ==
			BSL_API_RegisterPolicyProvider(agent->receive.bsl,
			1, policy_callbacks));

	agent->forward.policy = BSLP_PolicyProvider_Init(1);
	policy_callbacks.user_data = agent->forward.policy;
	ASSERT_PROPERTY(BSL_SUCCESS ==
			BSL_API_RegisterPolicyProvider(agent->forward.bsl,
			1, policy_callbacks));

	return 0;
}

static bool	ion_bsl_log_is_enabled(int severity)
{
	(void)severity;
	return true;
}

static void	ion_bsl_log_event(const struct timeval *timestamp,
			int severity, const char *filename, int lineno,
			const char *funcname, const char *format,
			va_list args)
{
	char	msgbuf[600];
	int	offset;

	(void)timestamp;
	offset = snprintf(msgbuf, sizeof(msgbuf), "[BSL %s:%d %s] ",
			filename, lineno, funcname);
	if (offset < 0 || (size_t)offset >= sizeof(msgbuf))
	{
		offset = 0;
	}

	vsnprintf(msgbuf + offset, sizeof(msgbuf) - offset, format, args);

	if (severity <= LOG_ERR)
	{
		writeMemo(msgbuf);
	}
	else
	{
		writeMemo(msgbuf);
	}
}

static void	loadCallbacks(BSL_HostDescriptors_t *descriptors)
{
	// New-style callbacks

	descriptors->get_sec_src_eid_fn		= ion_bsl_GetEid;
	descriptors->bundle_metadata_fn		= ion_bsl_GetBundleMetadata;
	descriptors->block_metadata_fn		= ion_bsl_GetBlockMetadata;
	descriptors->block_create_fn		= ion_bsl_CreateBlock;
	descriptors->block_remove_fn		= ion_bsl_RemoveBlock;
	descriptors->bundle_delete_fn		= ion_bsl_DeleteBundle;
	descriptors->block_realloc_btsd_fn	= ion_bsl_ReallocBTSD;
	descriptors->block_read_btsd_fn		= ion_bsl_BTSD_reader;
	descriptors->block_write_btsd_fn	= ion_bsl_BTSD_writer;

	// Old-style callbacks

	descriptors->eid_init			= ion_bsl_eid_load;
	descriptors->eid_deinit			= ion_bsl_eid_unload;
	descriptors->eid_to_cbor		= ion_bsl_encode_eid;
	descriptors->eid_from_cbor		= ion_bsl_decode_eid;
	descriptors->eid_from_text		= ion_bsl_eid_from_text;
#if 0
	descriptors->eid_to_text		= ion_bsl_eid_to_text;
#endif
	descriptors->eidpat_init		= ion_bsl_eidpat_load;
	descriptors->eidpat_deinit		= ion_bsl_eidpat_unload;
	descriptors->eidpat_from_text		= ion_bsl_eidpat_from_text;
	descriptors->eidpat_match		= ion_bsl_eidpat_match;

	// Dynamic memory callbacks (BSL v1.1)
	descriptors->dyn_mem_desc.malloc_cb	= ion_bsl_malloc_cb;
	descriptors->dyn_mem_desc.realloc_cb	= ion_bsl_realloc_cb;
	descriptors->dyn_mem_desc.calloc_cb	= ion_bsl_calloc_cb;
	descriptors->dyn_mem_desc.free_cb	= ion_bsl_free_cb;

	// Log callbacks (BSL v1.1)
	descriptors->log_is_enabled_for		= ion_bsl_log_is_enabled;
	descriptors->log_event			= ion_bsl_log_event;
}

int	bslInitialize(BslAgent *agent)
{
	static int		bslInitialized = 0;
	Sdr			sdr = getIonsdr();
	SdrObject		bpDbObject;
	BpDB			bpdb;
	char			keyRegistryFilePath[SDRSTRING_BUFSZ];
	char			policyConfigFilePath[SDRSTRING_BUFSZ];
	BSL_HostDescriptors_t	*descriptors;
	int			result;

	/*	Guard against repeated initialization.			*/

	if (bslInitialized)
	{
		return 0;	/*	Already initialized.		*/
	}

	if (sdr == NULL)
	{
		writeMemo("[?] BSL init can't find SDR.");
		return 1;
	}

	bpDbObject = getBpDbObject();
	if (bpDbObject == 0)
	{
		writeMemo("[?] BSL init can't find ION DB object.");
		return 1;
        }

	sdr_read(sdr, (char *) &bpdb, bpDbObject, sizeof(BpDB));
	oK(sdr_begin_xn(sdr));
	if (bpdb.bslKeyFile == 0 || bpdb.bslPolicyFile == 0)
	{
		sdr_exit_xn(sdr);
		writeMemo("[i] BSL not configured; security processing \
disabled.  Use 'm bsl' in bprc to enable.");
		bslInitialized = 1;
		return 0;
	}

	sdr_string_read(sdr, keyRegistryFilePath, bpdb.bslKeyFile);
	sdr_string_read(sdr, policyConfigFilePath, bpdb.bslPolicyFile);
	sdr_exit_xn(sdr);

	/*	BSL initialization parameters are now loaded.		*/

	BSL_CryptoInit();
#ifdef ION_BSL_TEST_VECTORS
	writeMemo("[!] ION_BSL_TEST_VECTORS build: content-encryption keys and \
IVs are FIXED RFC 9173 test vectors, not random.  NOT FOR OPERATIONAL USE.");
	BSL_Crypto_SetRngGenerator(rfc9173_bcb_cek);
#endif
	if (initializeAgent(agent) < 0)
	{
		writeMemo("[?] Bsl agent initialization failed.");
		return 1;
	}

	/*	NOTE: BSL_HostDescriptors_Set requires pass-by-value
	 *	rather than pass-by-reference.  We temporarily
	 *	allocate space for that object out of ION-managed
	 *	local memory, populate it, and pass it by value.
	 *	The function copies that object into a statically
	 *	allocated local object and returns, at which point
	 *	we immediately release the ION-managed local memory
	 *	occupied by the argument object.  No system memory
	 *	is allocated at any time.				*/

	descriptors = BSL_MALLOC(sizeof(BSL_HostDescriptors_t));
	if (descriptors == NULL)
	{
		writeMemo("[?] Can't create Bsl host descriptors object.");
		return 1;
	}

	descriptors->user_data = agent;
	loadCallbacks(descriptors);
	result = BSL_HostDescriptors_Set(*descriptors);	/*	Value.	*/
	BSL_FREE(descriptors);
	if (result < 0)
	{
		bslCleanup(agent);
		writeMemo("[?] Bsl host descriptors setting failed.");
		return 1;
	}

	/*	Initialize static global variables.			*/

	BSL_HostEID_Init(&agent->app_eid);
	BSL_HostEID_Init(&agent->sec_eid);
	if (key_registry_init(keyRegistryFilePath) != 0)
	{
		bslCleanup(agent);
		writeMemo("[?] Bsl host failed initializing key registry.");
		return 1;
	}

	/*	Load policy.						*/

	if (BSLP_RegisterPolicyFromJSON(policyConfigFilePath,
			agent->transmit.policy) != 0)
	{
		bslCleanup(agent);
		writeMemo("[?] Bsl host failed loading APPIN policy.");
		return 1;
	}

	if (BSLP_RegisterPolicyFromJSON(policyConfigFilePath,
			agent->deliver.policy) != 0)
	{
		bslCleanup(agent);
		writeMemo("[?] Bsl host failed loading APPOUT policy.");
		return 1;
	}

	if (BSLP_RegisterPolicyFromJSON(policyConfigFilePath,
			agent->receive.policy) != 0)
	{
		bslCleanup(agent);
		writeMemo("[?] Bsl host failed loading CLIN policy.");
		return 1;
	}

	if (BSLP_RegisterPolicyFromJSON(policyConfigFilePath,
			agent->forward.policy) != 0)
	{
		bslCleanup(agent);
		writeMemo("[?] Bsl host failed loading CLOUT policy.");
		return 1;
	}

	/*	Mark BSL as initialized.				*/

	bslInitialized = 1;
	return 0;
}

int	bslProcess(BslAgent *agent, BslContext *ctx,
		BSL_PolicyLocation_e loc, void *bundleWorkArea)
{
	int				returncode = 0;
	BSL_SecurityActionSet_t		*malloced_action_set;
	BSL_SecurityResponseSet_t	*malloced_response_set;

	(void)agent;	/*	Unused parameter			*/
	BSL_BundleRef_t			bundle_ref;

	malloced_action_set = BSL_CALLOC(1, BSL_SecurityActionSet_Sizeof());
	malloced_response_set = BSL_CALLOC(1, BSL_SecurityResponseSet_Sizeof());
	bundle_ref.data = bundleWorkArea;
	if (pthread_mutex_lock(&ctx->mutex))
	{
		BSL_LOG_CRIT("failed to lock mutex");
		return 2;
	}

	returncode = BSL_API_QuerySecurity(ctx->bsl, malloced_action_set,
			&bundle_ref, loc);
	if (returncode != 0)
	{
		BSL_LOG_ERR("Failed to query security: code=%d", returncode);
	}

	if (returncode == 0)
	{
		BSL_LOG_INFO("calling BSL_API_ApplySecurity");
		returncode = BSL_API_ApplySecurity(ctx->bsl,
				malloced_response_set, &bundle_ref,
				malloced_action_set);
		if (returncode < 0)
		{
			BSL_LOG_ERR("Failed to apply security: code=%d",
					returncode);
		}
	}

	if (pthread_mutex_unlock(&ctx->mutex))
	{
		BSL_LOG_CRIT("failed to unlock mutex");
	}

	/*	After BSL creates security blocks on the transmit path,
	 *	we must CBOR-serialize them. BSL writes raw BTSD into
	 *	blk->bytes, but ION expects blk->bytes to contain the
	 *	fully CBOR-encoded extension block (header + BTSD as
	 *	CBOR byte string + CRC). serializeExtBlk() performs
	 *	this encoding.
	 *
	 *	Both of the locations at which BSL can source a block -
	 *	APPIN for a locally sourced bundle, CLOUT for a bundle
	 *	being forwarded - need this pass.  Blocks that arrived
	 *	in the bundle, or that an earlier pass already encoded,
	 *	must be left alone; the loop below identifies them.	*/

	if (returncode == 0 && (loc == BSL_POLICYLOCATION_APPIN
			|| loc == BSL_POLICYLOCATION_CLOUT))
	{
		Sdr		sdr = getIonsdr();
		AcqWorkArea	*work;
		Bundle		*bundle;
		SdrObject	elt;
		SdrObject	blkAddr;
		ExtensionBlock	blk;
		char		*rawData;

		work = (AcqWorkArea *) bundleWorkArea;
		bundle = &(work->bundle);

		for (elt = sdr_list_first(sdr, bundle->extensions); elt;
				elt = sdr_list_next(sdr, elt))
		{
			blkAddr = sdr_list_data(sdr, elt);
			sdr_stage(sdr, (char *) &blk, blkAddr,
					sizeof(ExtensionBlock));
			if (blk.type != BlockIntegrityBlk
			&& blk.type != BlockConfidentialityBlk)
			{
				continue;
			}

			/*	ion_bsl_ReallocBTSD sets a block's
			 *	length equal to the size of the raw BTSD
			 *	that BSL is about to write into it, so a
			 *	block still awaiting encoding has length
			 *	equal to dataLength.  A block that has
			 *	already been encoded carries a CBOR
			 *	header as well, so its length exceeds
			 *	its dataLength.  Encode only the former:
			 *	at CLOUT the bundle can already hold
			 *	security blocks, and encoding one twice
			 *	would wrap it in a second CBOR header.	*/

			if (blk.bytes == 0 || blk.dataLength == 0
			|| blk.length != blk.dataLength)
			{
				continue;
			}

			/*	Read raw BTSD from SDR into memory.	*/

			rawData = MTAKE(blk.length);
			if (rawData == NULL)
			{
				putErrmsg("No memory for BSL block \
serialization.", NULL);
				returncode = -1;
				break;
			}

			sdr_read(sdr, rawData, blk.bytes,
					blk.length);

			/*	Set dataLength to actual BTSD size.
			 *	serializeExtBlk will replace blk.bytes
			 *	with fully CBOR-encoded block.		*/

			blk.dataLength = blk.length;
			if (serializeExtBlk(&blk, rawData) < 0)
			{
				MRELEASE(rawData);
				putErrmsg("Failed to serialize BSL \
block.", NULL);
				returncode = -1;
				break;
			}

			MRELEASE(rawData);

			/*	Write updated block back to SDR.	*/

			sdr_write(sdr, blkAddr, (char *) &blk,
					sizeof(ExtensionBlock));
			bundle->extensionsLength += blk.length;
		}
	}

	BSL_SecurityActionSet_Deinit(malloced_action_set);
	BSL_FREE(malloced_action_set);
	BSL_FREE(malloced_response_set);
	BSL_LOG_INFO("result code %d", returncode);
	return returncode;
}

void	bslCleanup(BslAgent *agent)
{
	BslContext *ctxs[] =
	{
			&agent->transmit,
			&agent->deliver,
			&agent->receive,
			&agent->forward
	};

	BslContext	*ctx;

	for (size_t ix = 0; ix < sizeof(ctxs) / sizeof(ctxs[0]); ++ix)
	{
		ctx = ctxs[ix];
		if (ctx->policy)
		{
			BSLP_PolicyProvider_Deinit(ctx->policy);
			ctx->policy = NULL;
		}
	}

	for (size_t ix = 0; ix < sizeof(ctxs) / sizeof(ctxs[0]); ++ix)
	{
		ctx = ctxs[ix];
		if (pthread_mutex_destroy(&ctx->mutex))
		{
			BSL_LOG_ERR("Failed pthread_mutex_destroy()");
		}

		if (ctx->bsl)
		{
			if (BSL_API_DeinitLib(ctx->bsl))
			{
				BSL_LOG_ERR("Failed BSL_API_DeinitLib()");
			}

			BSL_FREE(ctx->bsl);
			ctx->bsl = NULL;
		}
	}

	if (agent->app_eid.handle)
	{
		BSL_HostEID_Deinit(&agent->app_eid);
	}

	if (agent->sec_eid.handle)
	{
		BSL_HostEID_Deinit(&agent->sec_eid);
	}

	BSL_CryptoDeinit();
	BSL_HostDescriptors_Clear();
}
