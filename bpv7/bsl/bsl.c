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
#include "policy_config.h"
#include "rules_registry.h"
#include "text_util.h"
#endif

/****** bsl "descriptors" - ION implementations of BSL callbacks ********/

/*	*	Functions that operate on bundles and blocks	*	*/

static int	getBlockNumbersFromSdr(Bundle *bundle,
			BSL_PrimaryBlock_t *result_primary_block)
{
	Sdr	sdr = getIonsdr();
	int	canonicalBlockCount;
	int	i;
	Object	elt;
	Object	addr;
		OBJ_POINTER(ExtensionBlock, blk);

	canonicalBlockCount = 1	+ sdr_list_length(sdr, bundle->extensions);
	result_primary_block->block_count = canonicalBlockCount;
	result_primary_block->block_numbers = BSL_CALLOC(canonicalBlockCount,
			sizeof(uint64_t));
	if (!result_primary_block->block_numbers)
	{
		return -1;
	}

	for (elt = sdr_list_first(sdr, bundle->extensions), i= 0; elt;
			elt = sdr_list_next(sdr, elt), i++)
	{
		addr = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, ExtensionBlock, blk, addr);
		result_primary_block->block_numbers[i] = blk->number;
	}

	result_primary_block->block_numbers[i] = 1;	/*	payload	*/
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
	result_primary_block->block_count = canonicalBlockCount;
	result_primary_block->block_numbers = BSL_CALLOC(canonicalBlockCount,
			sizeof(uint64_t));
	if (!result_primary_block->block_numbers)
	{
		return -1;
	}

	for (elt = lyst_first(work->extBlocks), i= 0; elt;
			elt = lyst_next(elt), i++)
	{
		blk = (AcqExtBlock *) lyst_data(elt);
		result_primary_block->block_numbers[i] = blk->number;
	}

	result_primary_block->block_numbers[i] = 1;	/*	payload	*/
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
	Object	elt;
	Object	addr;
		OBJ_POINTER(ExtensionBlock, blk);

	elt = getExtensionBlock(bundle, block_num);
	if (elt == 0)	/*	Block not found.		*/
	{
		return -1;
	}

	addr = sdr_list_data(sdr, elt);
	GET_OBJ_POINTER(sdr, ExtensionBlock, blk, addr);
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
	ExtensionBlock	newBlock;

	memset((char *) &newBlock, 0, sizeof(ExtensionBlock));
	newBlock.type = block_type_code;
	newBlock.crcType = 0;				/*	No CRC	*/
       	/*	Flag 1 for BCB, 0 for BIB.				*/
	newBlock.blkProcFlags = block_type_code == 12 ? 1 : 0;
	newBlock.length = 1;
	newBlock.dataLength = newBlock.length + sizeof(ExtensionBlock);
	if (attachExtensionBlock(block_type_code, &newBlock, bundle) == 0)
	{
		*result_block_num = 0;		/*	Error.		*/
		return -1;
	}

	*result_block_num = newBlock.number;
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
	Object		bundleBlkElt;
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
} BtsdIoRef;

static int	ion_bsl_ReallocBTSD(BSL_BundleRef_t *bundle_ref,
			uint64_t block_num, size_t bytesize)
{
	Sdr		sdr = getIonsdr();
	AcqWorkArea	*work;
	Bundle		*bundle;
	Object		bundleObj;
	Object		elt;
#if 0
	Object		addr;
	ExtensionBlock	blk;
	unsigned char	*buffer;
#endif
	CHKERR(bundle_ref);
	CHKERR(bundle_ref->data);
	work = (AcqWorkArea *) (bundle_ref->data);
	bundle = &(work->bundle);
	bundleObj = (Object) bundle_ref->data;
	CHKERR(block_num > 0);
	CHKERR(bytesize > 0);
	sdr_read(sdr, (char *) &bundle, bundleObj, sizeof(Bundle));
	elt = getExtensionBlock(bundle, block_num);
	if (elt == 0)		/*	Block not found.		*/
	{
		return -3;
	}

	/*	Ion ION, extension blocks reside in the SDR heap
	 *	rather than in system memory.  An implementation
	 *	would be required to read the bundle into a stack
	 *	space object, find the indicated block and read it
	 *	into a stack space object (blk), allocate bytesize
	 *	bytes of heap space, sdr_read the current BTSD of
	 *	the block (blk.bytes) into a temporary memory
	 *	buffer of that size, sdr_free blk.bytes, sdr_write
	 *	the current BTSD contents to the newly allocated
	 *	heap space object, write the address of that heap
	 *	space object into blk.bytes, write the modified
	 *	block back to its heap location, modify the database
	 *	overhead values in the bundle, and write the bundle
	 *	back to its heap location.  Since this function is
	 *	currently invoked nowhere in BSL, its implementation
	 *	is deferred.						*/

	writeMemo("[?]  ion_bsl_ReallocBTSD called.  Utility unknown, not \
yet implemented in ION BSL integration.");
	return -9;

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
	CHKVOID(user_data);	/*	This is a (BtsdIoRef *).	*/
	BSL_FREE(user_data);
}

static int	readBTSDfromSdr(BtsdIoRef *ref, void *buf, size_t *bufsize)
{
	Sdr		sdr = getIonsdr();
	AcqWorkArea	*work;
	Bundle		*bundle;
	ZcoReader	reader;
	vast		bytesReceived;
	Object		elt;
	Object		addr;
			OBJ_POINTER(ExtensionBlock, blk);
	Address		startOfBTSD;

	work = ref->work;
	bundle = &(work->bundle);
	if (ref->blockNbr == 1)	/*	Reading from payload (ADU).	*/
	{
		zco_start_receiving(bundle->payload.content, &reader);
		bytesReceived = zco_receive_source(sdr, &reader, *bufsize,
				buf);
		*bufsize = bytesReceived;
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
	startOfBTSD = blk->bytes + (blk->length - blk->dataLength);
	sdr_read(sdr, buf, startOfBTSD, *bufsize);
	return 0;
}

static int	readBTSDfromRAM(BtsdIoRef *ref, void *buf, size_t *bufsize)
{
	Sdr		sdr = getIonsdr();
	AcqWorkArea	*work;
	Bundle		*bundle;
	ZcoReader	reader;
	vast		bytesReceived;
	LystElt		elt;
	AcqExtBlock	*blk;
	unsigned char	*startOfBTSD;

	work = ref->work;
	bundle = &(work->bundle);
	if (ref->blockNbr == 1)	/*	Reading from payload (ADU).	*/
	{
		zco_start_receiving(bundle->payload.content, &reader);
		bytesReceived = zco_receive_source(sdr, &reader, *bufsize,
				buf);
		*bufsize = bytesReceived;
		return 0;
	}

	/*	Reading from extension block type specific data.	*/

	elt = getAcqExtensionBlock(work, ref->blockNbr);
	if (elt == NULL)	/*	Block not found.		*/
	{
		return -3;
	}

	blk = (AcqExtBlock *) lyst_data(elt);
	startOfBTSD = blk->bytes + (blk->length - blk->dataLength);
	memcpy(buf, startOfBTSD, *bufsize);
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
	BSL_SeqReader_t	*reader;
	BtsdIoRef	*ref;

	CHKNULL(bundle_ref);
	CHKNULL(block_num > 0);

	/*	Note: this function reads only canonical blocks
	 *	(extension blocks and payload), not primary block.
	 *	The primary block has no BTSD.				*/

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
	Object		elt;
	Object		addr;
			OBJ_POINTER(ExtensionBlock, blk);
	Address		startOfBTSD;
	int		bytesWritten;

	work = ref->work;
	bundle = &(work->bundle);
	if (ref->blockNbr == 1)	/*	Writing to payload (ADU);	*/
	{
		CHKERR(sdr_begin_xn(sdr));
		bytesWritten = zco_revise(sdr, bundle->payload.content, 0,
				(char *) buf, size);
		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Can't write to extension BTSD.",
					itoa(ref->blockNbr));
			return -1;
		}

		if (bytesWritten < size)
		{
			return BSL_ERR_FAILURE;
		}

		return BSL_SUCCESS;
	}

	/*	Writing to extension block type specific data.		*/

	elt = getExtensionBlock(bundle, ref->blockNbr);
	if (elt == 0)		/*	Block not found.		*/
	{
		return -3;
	}

	addr = sdr_list_data(sdr, elt);
	GET_OBJ_POINTER(sdr, ExtensionBlock, blk, addr);
	startOfBTSD = blk->bytes + (blk->length - blk->dataLength);
	CHKERR(sdr_begin_xn(sdr));
	sdr_write(sdr, startOfBTSD, (char *) buf, size);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't write to extension BTSD.",
				itoa(ref->blockNbr));
		return -1;
	}

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
	int		bytesWritten;

	work = ref->work;
	bundle = &(work->bundle);
	if (ref->blockNbr == 1)	/*	Writing to payload (ADU);	*/
	{
		CHKERR(sdr_begin_xn(sdr));
		bytesWritten = zco_revise(sdr, bundle->payload.content, 0,
				(char *) buf, size);
		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Can't write to extension BTSD.",
					itoa(ref->blockNbr));
			return -1;
		}

		if (bytesWritten < size)
		{
			return BSL_ERR_FAILURE;
		}

		return BSL_SUCCESS;
	}

	/*	Writing to extension block type specific data.		*/

	elt = getAcqExtensionBlock(work, ref->blockNbr);
	if (elt == 0)		/*	Block not found.		*/
	{
		return -3;
	}

	blk = (AcqExtBlock *) lyst_data(elt);
	startOfBTSD = blk->bytes + (blk->length - blk->dataLength);
	memcpy((char *) buf, startOfBTSD, size);
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

static int	 ion_bsl_eid_load(void *user_data _U_,
			BSL_HostEID_t *eidWrapper)
{
	CHKERR1(eidWrapper);
	memset(eidWrapper, 0, sizeof(BSL_HostEID_t));
	eidWrapper->handle = BSL_MALLOC(sizeof(EndpointId));
	if (!(eidWrapper->handle))
	{
		return 2;
	}

	ion_bsl_eid_init(eidWrapper->handle);
	return 0;
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

	CHKERR1(cborText);
	ASSERT_ARG_NONNULL(cborText->ptr);
	CHKERR1(eidWrapper);
	ASSERT_ARG_NONNULL(eidWrapper->handle);
	eid = (EndpointId *) (eidWrapper->handle);
	length = serializeEid(eid, buffer);
	if (length < 1)
	{
		return 0;	/*	Failure.			*/
	}

	if (length <= cborText->len)
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

	CHKERR1(cborText);
	ASSERT_ARG_NONNULL(cborText->ptr);
	CHKERR1(eidWrapper);
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

	CHKERR1(text);
	CHKERR1(eidWrapper);
	ASSERT_ARG_NONNULL(eidWrapper->handle);
	eid = (EndpointId *) (eidWrapper->handle);
	if (parseEidString((char *) text, &metaEid, &vscheme, &vschemeElt) == 0
	|| jotEid(eid, &metaEid) < 0)
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

	CHKERR1(text);
	CHKERR1(eidWrapper);
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
	static Object	bpDbObject = 0;
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
	CHKERR1(patWrapper); // GCOV_EXCL_LINE
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
	CHKERR1(patWrapper);
	CHKERR1(text);
	eidp = (EidPattern *) patWrapper->handle;
	CHKERR1(eidp);
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
	CHKERR1(patWrapper);
	CHKERR1(patWrapper->handle);
	CHKERR1(eidWrapper);
	CHKERR1(eidWrapper->handle);
	// GCOV_EXCL_STOP
	eidp = (EidPattern *) (patWrapper->handle);
	eid = (EndpointId *) (eidWrapper->handle);

	return (eidMatchesPattern(eidp, eid) ? true : false);
}

static BSL_HostEIDPattern_t	get_eid_pattern_from_text(const char *text)
{
	BSL_HostEIDPattern_t	pat;

	BSL_HostEIDPattern_Init(&pat);
	ASSERT_PROPERTY(0 == BSL_HostEIDPattern_DecodeFromText(&pat, text));
	return pat;
}

/******************* Functions that load BSL policies *******************/

static int	rfc9173_bcb_cek(unsigned char *buf, int len)
{
	/*	Lifted verbatim from mock_bpa_rfc9173_bcb_cek.		*/

	if (len == 12) // IV
	{
		uint8_t iv[] = { 0x54, 0x77, 0x65, 0x6c, 0x76, 0x65, 0x31, 0x32, 0x31, 0x32, 0x31, 0x32 };
		memcpy(buf, iv, 12);
	}
	else // A3 KEY
	{
		uint8_t rfc9173A3_key[] = { 0x71, 0x77, 0x65, 0x72, 0x74, 0x79, 0x75, 0x69, 0x6f, 0x70, 0x61, 0x73, 0x64, 0x66, 0x67, 0x68 };
		memcpy(buf, rfc9173A3_key, len);
	}

	return 1;
}

static void	rule_params_init(rule_params *params, int policy_num)
{
	params->param_integ_scope_flag	= BSL_CALLOC(1, BSL_SecParam_Sizeof());
	BSL_SecParam_Init(params->param_integ_scope_flag);
	params->param_sha_variant	= BSL_CALLOC(1, BSL_SecParam_Sizeof());
	BSL_SecParam_Init(params->param_sha_variant);
	params->param_aad_scope_flag	= BSL_CALLOC(1, BSL_SecParam_Sizeof());
	BSL_SecParam_Init(params->param_aad_scope_flag);
	params->param_init_vector	= BSL_CALLOC(1, BSL_SecParam_Sizeof());
	BSL_SecParam_Init(params->param_init_vector);
	params->param_aes_variant	= BSL_CALLOC(1, BSL_SecParam_Sizeof());
	BSL_SecParam_Init(params->param_aes_variant);
	params->param_test_key		= BSL_CALLOC(1, BSL_SecParam_Sizeof());
	BSL_SecParam_Init(params->param_test_key);
	params->param_use_wrapped_key	= BSL_CALLOC(1, BSL_SecParam_Sizeof());
	BSL_SecParam_Init(params->param_use_wrapped_key);
	params->active = true;
	BSL_LOG_DEBUG("Successfully Init policy rule number %d in registry",
			policy_num);
}

static void	rule_params_deinit(rule_params *params, int policy_num)
{
	BSL_SecParam_Deinit(params->param_integ_scope_flag);
	BSL_FREE(params->param_integ_scope_flag);
	BSL_SecParam_Deinit(params->param_sha_variant);
	BSL_FREE(params->param_sha_variant);
	BSL_SecParam_Deinit(params->param_aad_scope_flag);
	BSL_FREE(params->param_aad_scope_flag);
	BSL_SecParam_Deinit(params->param_init_vector);
	BSL_FREE(params->param_init_vector);
	BSL_SecParam_Deinit(params->param_aes_variant);
	BSL_FREE(params->param_aes_variant);
	BSL_SecParam_Deinit(params->param_test_key);
	BSL_FREE(params->param_test_key);
	BSL_SecParam_Deinit(params->param_use_wrapped_key);
	BSL_FREE(params->param_use_wrapped_key);
	params->active = false;
	BSL_LOG_DEBUG("Successfully De-init policy rule number %d in registry",
			policy_num);
}

static void	rules_registry_init(rules_registry *registry)
{
	for (int i = 0; i < MAX_RULES; ++i)
	{
		registry->in_use[i] = false;
	}

	registry->registry_count = 0;
}
#if 0
static int	rules_registry_size(const rules_registry *registry)
{
	return registry->registry_count;
}
#endif
static rule_params	*rules_registry_slot(rules_registry *registry)
{
	int 	i;
	int	index;

	for (i = 0; i < MAX_RULES; ++i)
	{
		index = registry->registry_count + i;
		if (!registry->in_use[index])
		{
			registry->in_use[index]  = true;
			registry->registry_count = index + 1;
			rule_params_init(&registry->param_sets[index],
					index);
			return &registry->param_sets[index];
		}
	}

	BSL_LOG_CRIT("POLICY COUNT FULL!");
	return NULL;
}

static void	rules_registry_deinit(rules_registry *registry)
{
	for (int i = 0; i < MAX_RULES; ++i)
	{
		if (registry->in_use[i])
		{
			rule_params_deinit(&registry->param_sets[i], i);
			registry->in_use[i] = false;
		}
	}

	registry->registry_count = 0;
}

/**
 * @TODO Handle BPA events as policy actions - dependent on other
 * BSL issues/ future changes.
 */

static void	register_sc_parms(json_t *sc_parms, long sc_id_l,
			rule_params *params, uint64_t *params_got)
{
	size_t				n;
	size_t				i;
	json_t				*entry;
	json_t				*id;
	const char			*id_str;
	json_t				*value;
	const char			*value_str;
	uint64_t			sha_var;
	uint64_t			flag;
	uint64_t			keywrap;
	rfc9173_bcb_aes_variant_e	aes_var;

	/*	Register the security context parameters for a single
	 *	policy.							*/

	n = json_array_size(sc_parms);
	BSL_LOG_DEBUG("	 sc_parms (%zu):", n);
	for (i = 0; i < n; ++i)
	{
		entry = json_array_get(sc_parms, i);
		if (!json_is_object(entry))
		{
			continue;
		}

		id = json_object_get(entry, "id");
		if (!id || !json_is_string(id))
		{
			continue;
		}

		id_str = json_string_value(id);
		value = json_object_get(entry, "value");
		if (!value || !json_is_string(value))
		{
			continue;
		}

		value_str = json_string_value(value);
		BSL_LOG_DEBUG("		 - id: %s, value: %s",
				id_str, value_str);

		// different valid param IDs for  different security contexts

		switch (sc_id_l)
		{
		case 1:		/*	Security context #1.		*/
			if (0 == strcmp(id_str, "key_name"))
			{
				BSL_SecParam_InitTextstr(params->param_test_key,
						BSL_SECPARAM_TYPE_KEY_ID,
						value_str);
				*params_got |= 0x1;
			}
			else if (0 == strcmp(id_str, "sha_variant"))
			{
				if (0 == strcmp (value_str, "5"))
				{
					sha_var = RFC9173_BIB_SHA_HMAC256;
				}
				else if (0 == strcmp(value_str, "6"))
				{
					sha_var = RFC9173_BIB_SHA_HMAC384;
				}
				else
				{
					sha_var = RFC9173_BIB_SHA_HMAC512;
				}

				BSL_SecParam_InitInt64
					(params->param_sha_variant,
					 RFC9173_BIB_PARAMID_SHA_VARIANT,
		 			sha_var);
				*params_got |= 0x2;
			}
			else if (0 == strcmp(id_str, "scope_flags"))
			{
				flag = strtol(value_str, NULL, 10); // FIXME
				BSL_SecParam_InitInt64
					(params->param_integ_scope_flag,
		 			RFC9173_BIB_PARAMID_INTEG_SCOPE_FLAG,
		 			flag);
				*params_got |= 0x4;
			}
			else if (0 == strcmp(id_str, "key_wrap"))
			{
				if (0 == strcmp (value_str, "0"))
				{
					keywrap = 0;
				}
				else
				{
					keywrap = 1;
				}

				BSL_SecParam_InitInt64
					(params->param_use_wrapped_key,
			 		BSL_SECPARAM_USE_KEY_WRAP,
  					keywrap);
				*params_got |= 0x8;
			}
			else
			{
				BSL_LOG_ERR("INVALID PARAM KEY %s FOR SC ID %d",
						id_str, sc_id_l);
				continue;
			}

			break;

		case 2:		/*	Security context #2.		*/
			if (0 == strcmp(id_str, "key_name"))
			{
				BSL_SecParam_InitTextstr(params->param_test_key,
					BSL_SECPARAM_TYPE_KEY_ID, value_str);
				*params_got |= 0x1;
			}
			else if (0 == strcmp(id_str, "iv"))
			{

			// TODO convert value_str to bstring
			// BSL_SecParam_InitBytestr(params->param_init_vector,
			// RFC9173_BCB_SECPARAM_IV);

				*params_got |= 0x2;
			}
			else if (0 == strcmp(id_str, "aes_variant"))
			{
				if (0 == strcmp(value_str, "1"))
				{
					aes_var =
						RFC9173_BCB_AES_VARIANT_A128GCM;
				}
				else
				{
					aes_var =
					       	RFC9173_BCB_AES_VARIANT_A256GCM;
				}

				BSL_SecParam_InitInt64
					(params->param_aes_variant,
					 RFC9173_BCB_SECPARAM_AESVARIANT,
					 aes_var);
				*params_got |= 0x4;
			}
			else if (0 == strcmp(id_str, "aad_scope"))
			{
				flag = strtol(value_str, NULL, 10); // FIXME
				BSL_SecParam_InitInt64
					(params->param_aad_scope_flag,
					 RFC9173_BCB_SECPARAM_AADSCOPE,
					 flag);
				*params_got |= 0x8;
			}
			else if (0 == strcmp(id_str, "key_wrap"))
			{
				if (0 == strcmp(value_str, "0"))
				{
					keywrap = 0;
				}
				else
				{
					keywrap = 1;
				}

				BSL_SecParam_InitInt64
					(params->param_use_wrapped_key,
					 BSL_SECPARAM_USE_KEY_WRAP,
					 keywrap);
				*params_got |= 0x10;
			}
			else
			{
				BSL_LOG_ERR("INVALID PARAM KEY %s FOR SC ID %d",
						id_str, sc_id_l);
				continue;
			}

			break;

		default:
			BSL_LOG_ERR("INVALID SC ID");
			continue;
		}
	}
}

static int	register_policy_from_json(const char *pp_cfg_file_path,
			BSLP_PolicyProvider_t *policy, rules_registry *reg)
{
	/*	Stolen from mock_bpa_register_policy_from_json.		*/

	uint32_t		sec_block_type;
	uint32_t		sec_ctx_id;
	BSL_SecRole_e		sec_role;
	uint32_t		target_block_type;
	BSL_PolicyLocation_e	policy_loc_enum;
	BSL_PolicyAction_e	policy_action_enum;
	const char		*src_str;
	const char		*dest_str;
	const char		*sec_src_str;
	BSL_HostEIDPattern_t	src_eid;
	BSL_HostEIDPattern_t	dest_eid;
	BSL_HostEIDPattern_t	sec_src_eid;
	json_t			*rule_id;
	const char		*rule_id_str;
	json_t			*root;
	json_error_t		err;
	json_t			*policyrule_set;
	size_t			policy_rule_idx;
	size_t			policy_rule_ct;
	json_t			*policy_rule_elm;
	json_t			*policyrule;
	rule_params		*params;
	json_t			*filter;
	json_t			*role;
	const char		*role_str;
	json_t			*src;
	json_t			*dest;
	json_t			*sec_src;
	json_t			*tgt;
	long			tgt_l;
	json_t			*loc;
	const char		*loc_str;
	json_t			*sc_id;
	long			sc_id_l;
	json_t			*es_ref;
	json_t			*policy_action_on_fail;
	const char		*policy_act_str;
	uint64_t		params_got;
	json_t			*spec;
	json_t			*sc_parms;
	json_t			*event_set;
	json_t			*es_ref_es;
	json_t			*events;
	size_t			n;
	size_t			i;
	json_t			*entry;
	json_t			*event_id;
	const char		*event_id_str;
	json_t			*actions;
	size_t			j;
	size_t			m;
	json_t			*act;
	const char		*act_str;
	BSLP_PolicyPredicate_t	*predicate;
	BSLP_PolicyRule_t	*rule;

	/*	Acquire policy rules from named file of JSON text.	*/

	root = json_load_file(pp_cfg_file_path, 0, &err);
	if (!root)
	{
		BSL_LOG_ERR("JSON error: line %d: %s", err.line, err.text);
		return -2;
	}

	// policyrule_set attr
	policyrule_set = json_object_get(root, "policyrule_set");
	if (!policyrule_set || !json_is_array(policyrule_set))
	{
		BSL_LOG_ERR("Missing policyrule set ");
		json_decref(root);
		return -3;
	}

	policy_rule_ct = json_array_size(policyrule_set);
	BSL_LOG_DEBUG(" got (%zu) policyrules:", policy_rule_ct);
	for (policy_rule_idx = 0; policy_rule_idx < policy_rule_ct;
			++policy_rule_idx)
	{
		/*	Register one security policy rule.		*/

		policy_rule_elm = json_array_get(policyrule_set,
				policy_rule_idx);
		if (!json_is_object(policy_rule_elm))
		{
			BSL_LOG_ERR("Policy rule not JSON object");
			continue;
		}

		// policyrule attr
		policyrule = json_object_get(policy_rule_elm, "policyrule");
		if (!policyrule || !json_is_object(policyrule))
		{
			BSL_LOG_ERR("Missing policyrule");
			continue;
		}

		/*	Obtain security policy registry slot for this
		 *	policy rule.					*/

		params = rules_registry_slot(reg);
		if (!params)
		{
			BSL_LOG_CRIT("POLICY COUNT EXCEEDED, NOT REGISTERING \
FURTHER");
			return -1;
		}

		// filter attr
		filter = json_object_get(policyrule, "filter");
		if (filter && json_is_object(filter))
		{
			BSL_LOG_DEBUG("filter:");

			// Get rule_id
			rule_id = json_object_get(filter, "rule_id");
			if (!rule_id)
			{
				BSL_LOG_ERR("No rule ID ");
				continue;
			}

			rule_id_str = json_string_value(rule_id);
			BSL_LOG_DEBUG("	 rule_id: %s", rule_id_str);

			// get sec role
			role = json_object_get(filter, "role");
			if (!role)
			{
				BSL_LOG_ERR("No sec role");
				continue;
			}

			role_str = json_string_value(role);
			BSL_LOG_DEBUG("	 role   : %s", role_str);

			// check for valid sec role
			if (0 == strcmp(role_str, "s"))
			{
				sec_role = BSL_SECROLE_SOURCE;
			}
			else if (0 == strcmp(role_str, "v"))
			{
				sec_role = BSL_SECROLE_VERIFIER;
			}
			else if (0 == strcmp(role_str, "a"))
			{
				sec_role = BSL_SECROLE_ACCEPTOR;
			}
			else
			{
				BSL_LOG_ERR("INVALID SEC ROLE %s", role_str);
				continue;
			}

			src = json_object_get(filter, "src");
			if (src)
			{
				src_str = json_string_value(src);
				BSL_LOG_DEBUG("	 src	: %s", src_str);
				src_eid = get_eid_pattern_from_text(src_str);
			}
			else
			{
				src_eid = get_eid_pattern_from_text("*:**");
			}

			dest = json_object_get(filter, "dest");
			if (dest)
			{
				dest_str = json_string_value(dest);
				BSL_LOG_DEBUG("	 dest	: %s", dest_str);
				dest_eid = get_eid_pattern_from_text(dest_str);
			}
			else
			{
				dest_eid = get_eid_pattern_from_text("*:**");
			}

			sec_src = json_object_get(filter, "sec_src");
			if (sec_src)
			{
				sec_src_str = json_string_value(sec_src);
				BSL_LOG_DEBUG("	 sec_src	: %s",
						sec_src_str);
				sec_src_eid = get_eid_pattern_from_text
						(sec_src_str);
			}
			else
			{
				sec_src_eid = get_eid_pattern_from_text("*:**");
			}

			// check tgt (target block type)
			tgt = json_object_get(filter, "tgt");
			if (!tgt)
			{
				BSL_LOG_ERR("No tgt");
				continue;
			}

			tgt_l = json_integer_value(tgt);
			BSL_LOG_DEBUG("	 tgt	: %" JSON_INTEGER_FORMAT,
					tgt_l);
			target_block_type = tgt_l;

			// check loc (sec location )
			loc = json_object_get(filter, "loc");
			if (!loc)
			{
				BSL_LOG_ERR("No loc");
				continue;
			}

			loc_str = json_string_value(loc);
			BSL_LOG_DEBUG("	 loc	: %s", loc_str);

			if (0 == strcmp(loc_str, "appin"))
			{
				policy_loc_enum = BSL_POLICYLOCATION_APPIN;
			}
			else if (0 == strcmp(loc_str, "appout"))
			{
				policy_loc_enum = BSL_POLICYLOCATION_APPOUT;
			}
			else if (0 == strcmp(loc_str, "clin"))
			{
				policy_loc_enum = BSL_POLICYLOCATION_CLIN;
			}
			else if (0 == strcmp(loc_str, "clout"))
			{
				policy_loc_enum = BSL_POLICYLOCATION_CLOUT;
			}
			else
			{
				BSL_LOG_ERR("INVALID POLICY LOCATION %s",
						loc_str);
				continue;
			}

			sc_id = json_object_get(filter, "sc_id");
			if (!sc_id || !json_is_integer(sc_id))
			{
				BSL_LOG_DEBUG("NO SEC CTX ID");
				continue;
			}

			sc_id_l = json_integer_value(sc_id);
			BSL_LOG_DEBUG("	 scid	: %" JSON_INTEGER_FORMAT,
					sc_id_l);
			sec_ctx_id	 = sc_id_l;
			sec_block_type = (sec_ctx_id == 1) ?
					BSL_SECBLOCKTYPE_BIB :
					BSL_SECBLOCKTYPE_BCB;
		}
		else
		{
			BSL_LOG_DEBUG("NO FILTER");
			continue;
		}

		// event set_ref
		es_ref = json_object_get(policyrule, "es_ref");
		if (!es_ref || !json_is_string(es_ref))
		{
			BSL_LOG_DEBUG("NO ES REF");
		}

		// _temp_not_ion_spec_policy_action_on_fail
		policy_action_on_fail = json_object_get(policyrule,
				"_temp_not_ion_spec_policy_action_on_fail");
		if (!policy_action_on_fail
		|| !json_is_string(policy_action_on_fail))
		{
			BSL_LOG_ERR("NO POLICY ACTION");
			continue;
		}

		policy_act_str = json_string_value(policy_action_on_fail);
		if (0 == strcmp(policy_act_str, "delete_bundle"))
		{
			policy_action_enum = BSL_POLICYACTION_DROP_BUNDLE;
		}
		else if (0 == strcmp(policy_act_str, "drop_block"))
		{
			policy_action_enum = BSL_POLICYACTION_DROP_BLOCK;
		}
		else if (0 == strcmp(policy_act_str, "nothing"))
		{
			policy_action_enum = BSL_POLICYACTION_NOTHING;
		}
		else
		{
			BSL_LOG_ERR("INVALID POLICY ACTION ENUM %s",
					policy_act_str);
			continue;
		}

		params_got = 0x0;	/*	Documents params rec'd.	*/

		// spec attr
		spec = json_object_get(policyrule, "spec");
		if (spec && json_is_object(spec))
		{
			// check sec ctx id
			sc_id = json_object_get(spec, "sc_id");
			sc_id_l = json_integer_value(sc_id);

			BSL_LOG_DEBUG("spec:");
			BSL_LOG_DEBUG("	 sc_id: %" JSON_INTEGER_FORMAT,
					sc_id_l ? sc_id_l : -1);

			sc_parms = json_object_get(spec, "sc_parms");
			if (sc_parms && json_is_array(sc_parms))
			{
				register_sc_parms(sc_parms, sc_id_l, params,
						&params_got);
			}
		}

		// event set
		// TODO currently not utilized
		event_set = json_object_get(root, "event_set");
		if (event_set && json_is_object(event_set))
		{
			es_ref_es = json_object_get(policyrule, "es_ref");
			if (!es_ref_es || !json_is_string(es_ref_es))
			{
				BSL_LOG_DEBUG("NO ES REF");
			}

			events = json_object_get(event_set, "events");
			if (events && json_is_array(events))
			{
				n = json_array_size(events);
				BSL_LOG_DEBUG("num events (%zu):", n);
				for (i = 0; i < n; ++i)
				{
					entry = json_array_get(events, i);
					if (!json_is_object(entry))
					{
						continue;
					}

					event_id = json_object_get(entry,
							"event_id");
					if (!event_id)
					{
						continue;
					}

					event_id_str = json_string_value
							(event_id);
					BSL_LOG_DEBUG("EVENT ID FOUND: %s",
							event_id_str);
					actions = json_object_get(entry,
							"actions");
					if (actions && json_is_array(actions))
					{
						m = json_array_size(actions);
						BSL_LOG_DEBUG("num actions \
in %s (%zu):", event_id_str, m);
						for (j = 0; j < m; ++j)
						{
							act = json_array_get
								(actions, j);
							if (json_is_string(act)
								== 0)
							{
								continue;
							}

							act_str =
							json_string_value(act);
							BSL_LOG_DEBUG("Action \
of %s: %s", event_id_str, act_str);
						}
					}
				}
			}
		}

		predicate = &policy->predicates[policy->predicate_count++];
		BSLP_PolicyPredicate_Init(predicate, policy_loc_enum, src_eid,
				sec_src_eid, dest_eid);
		rule = &policy->rules[policy->rule_count++];
		BSLP_PolicyRule_Init(rule, rule_id_str, predicate, sec_ctx_id,
				sec_role, sec_block_type, target_block_type,
				policy_action_enum);

		// TODO validate params_got
		(void) params_got;

		if (sec_ctx_id == 2) // BCB
		{
			BSLP_PolicyRule_CopyParam(rule,
					params->param_aes_variant);
			if (sec_role == BSL_SECROLE_SOURCE)
			{
				BSLP_PolicyRule_CopyParam(rule,
						params->param_aad_scope_flag);
				BSL_Crypto_SetRngGenerator(rfc9173_bcb_cek);
			}
		}
		else
		{
			BSLP_PolicyRule_CopyParam(rule,
					params->param_sha_variant);
			BSLP_PolicyRule_CopyParam(rule,
					params->param_integ_scope_flag);
		}

		BSLP_PolicyRule_CopyParam(rule, params->param_test_key);
		BSLP_PolicyRule_CopyParam(rule, params->param_use_wrapped_key);
	}

	json_decref(root);
	return 0;
}

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

	CHKERR1(out);
	CHKERR1(in);

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

	agent->transmit.policy = BSL_CALLOC(1, sizeof(BSLP_PolicyProvider_t));
	agent->transmit.policy->pp_id = 1;
	policy_callbacks.deinit_fn = BSLP_Deinit;
	policy_callbacks.query_fn = BSLP_QueryPolicy;
	policy_callbacks.finalize_fn = BSLP_FinalizePolicy;
	policy_callbacks.user_data = agent->transmit.policy;
	ASSERT_PROPERTY(BSL_SUCCESS ==
			BSL_API_RegisterPolicyProvider(agent->transmit.bsl,
			1, policy_callbacks));

	agent->deliver.policy = BSL_CALLOC(1, sizeof(BSLP_PolicyProvider_t));
	agent->deliver.policy->pp_id = 1;
	policy_callbacks.deinit_fn = BSLP_Deinit;
	policy_callbacks.query_fn = BSLP_QueryPolicy;
	policy_callbacks.finalize_fn = BSLP_FinalizePolicy;
	policy_callbacks.user_data = agent->deliver.policy;
	ASSERT_PROPERTY(BSL_SUCCESS ==
			BSL_API_RegisterPolicyProvider(agent->deliver.bsl,
			1, policy_callbacks));

	agent->receive.policy = BSL_CALLOC(1, sizeof(BSLP_PolicyProvider_t));
	agent->receive.policy->pp_id = 1;
	policy_callbacks.deinit_fn = BSLP_Deinit;
	policy_callbacks.query_fn = BSLP_QueryPolicy;
	policy_callbacks.finalize_fn = BSLP_FinalizePolicy;
	policy_callbacks.user_data = agent->receive.policy;
	ASSERT_PROPERTY(BSL_SUCCESS ==
			BSL_API_RegisterPolicyProvider(agent->receive.bsl,
			1, policy_callbacks));

	agent->forward.policy = BSL_CALLOC(1, sizeof(BSLP_PolicyProvider_t));
	agent->forward.policy->pp_id = 1;
	policy_callbacks.deinit_fn = BSLP_Deinit;
	policy_callbacks.query_fn = BSLP_QueryPolicy;
	policy_callbacks.finalize_fn = BSLP_FinalizePolicy;
	policy_callbacks.user_data = agent->forward.policy;
	ASSERT_PROPERTY(BSL_SUCCESS ==
			BSL_API_RegisterPolicyProvider(agent->forward.bsl,
			1, policy_callbacks));

	return 0;
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
}

int	bslInitialize(BslAgent *agent)
{
	Sdr			sdr = getIonsdr();
	Object			bpDbObject;
	BpDB			bpdb;
	char			keyRegistryFilePath[SDRSTRING_BUFSZ];
	char			policyConfigFilePath[SDRSTRING_BUFSZ];
	BSL_HostDescriptors_t	*descriptors;
	int			result;

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
	if (bpdb.bslKeyFile == 0)
	{
		sdr_exit_xn(sdr);
		writeMemo("[?] BSL init fails: no BSL key file name.");
		return 1;
	}

	sdr_string_read(sdr, keyRegistryFilePath, bpdb.bslKeyFile);
	if (bpdb.bslPolicyFile == 0)
	{
		sdr_exit_xn(sdr);
		writeMemo("[?] BSL init fails: no BSL policy file name.");
		return 1;
        }

	sdr_string_read(sdr, policyConfigFilePath, bpdb.bslPolicyFile);
	sdr_exit_xn(sdr);

	/*	BSL initialization parameters are now loaded.		*/

	BSL_openlog();
	BSL_CryptoInit();
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

	rules_registry_init(&agent->rulesRegistry);
	if (register_policy_from_json(policyConfigFilePath,
			agent->transmit.policy, &agent->rulesRegistry) != 0)
	{
		bslCleanup(agent);
		writeMemo("[?] Bsl host failed loading APPIN policy.");
		return 1;
	}

	if (register_policy_from_json(policyConfigFilePath,
			agent->deliver.policy, &agent->rulesRegistry) != 0)
	{
		bslCleanup(agent);
		writeMemo("[?] Bsl host failed loading APPOUT policy.");
		return 1;
	}

	if (register_policy_from_json(policyConfigFilePath,
			agent->receive.policy, &agent->rulesRegistry) != 0)
	{
		bslCleanup(agent);
		writeMemo("[?] Bsl host failed loading CLIN policy.");
		return 1;
	}

	if (register_policy_from_json(policyConfigFilePath,
			agent->forward.policy, &agent->rulesRegistry) != 0)
	{
		bslCleanup(agent);
		writeMemo("[?] Bsl host failed loading CLOUT policy.");
		return 1;
	}

	return 0;
}

int	bslProcess(BslAgent *agent, BslContext *ctx,
		BSL_PolicyLocation_e loc, void *bundleWorkArea)
{
	int				returncode = 0;
	BSL_SecurityActionSet_t		*malloced_action_set;
	BSL_SecurityResponseSet_t	*malloced_response_set;
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

	rules_registry_deinit(&agent->rulesRegistry);
	for (size_t ix = 0; ix < sizeof(ctxs) / sizeof(ctxs[0]); ++ix)
	{
		ctx = ctxs[ix];
		if (pthread_mutex_destroy(&ctx->mutex))
		{
			BSL_LOG_ERR("Failed pthread_mutex_destroy()");
		}

		if (BSL_API_DeinitLib(ctx->bsl))
		{
			BSL_LOG_ERR("Failed BSL_API_DeinitLib()");
		}

		BSL_FREE(ctx->bsl);
		ctx->bsl = NULL;
	}

	if (agent->app_eid.handle)
	{
		BSL_HostEID_Deinit(&agent->app_eid);
	}

	if (agent->sec_eid.handle)
	{
		BSL_HostEID_Deinit(&agent->sec_eid);
	}

	BSL_HostDescriptors_Clear();
	BSL_CryptoDeinit();
	BSL_closelog();
}
