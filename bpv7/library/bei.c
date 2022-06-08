/******************************************************************************
 	bei.c:	Implementation of functions that manipulate extension blocks
 	        within bundles and acquisition structures.

	Author: Scott Burleigh, JPL

	Modification History:
	Date      Who   What

	Copyright (c) 2010, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
 ******************************************************************************/

/*****************************************************************************
 **
 ** File Name: bei.c
 **
 ** Subsystem: BP
 **
 ** Description: This file provides function implementations used to find and
 **              manipulate extension blocks within bundles and acquisition
 **              structures.
 **
 ** Assumptions:
 **      1.
 **
 ** Modification History:
 **
 **  MM/DD/YY  AUTHOR        IONWG#    DESCRIPTION
 **  --------  ------------  -------  ---------------------------------------
 **  Original  S. Burleigh            Initial Implementation
 **  04/15/10  E. Birrane     105     Built bei.c file libbpP.c
 *****************************************************************************/

#include "bpP.h"
#include "bei.h"
#include "bpsec_util.h"

/*	We hitchhike on the ZCO heap space management system to 
 *	manage the space occupied by Bundle objects.  In effect,
 *	the Bundle overhead objects compete with ZCOs for available
 *	SDR heap space.  We don't want this practice to become
 *	widespread, which is why these functions are declared
 *	privately here rather than publicly in the zco.h header.	*/

extern void	zco_increase_heap_occupancy(Sdr sdr, vast delta, ZcoAcct acct);
extern void	zco_reduce_heap_occupancy(Sdr sdr, vast delta, ZcoAcct acct);

/******************************************************************************
 *               OPERATIONS ON THE EXTENSION DEFINITIONS ARRAY                *
 ******************************************************************************/

static void	getExtInfo(ExtensionDef **definitions, int *definitionCount,
			ExtensionSpec **specs, int *specCount)
{
#ifdef BP_EXTENDED
#include "bpextensions.c"
#else
#include "noextensions.c"
#endif
	static int	nbrOfDefinitions = sizeof extensionDefs
				/ sizeof(ExtensionDef);
	static int	nbrOfSpecs = sizeof extensionSpecs
				/ sizeof(ExtensionSpec);

	*definitions = extensionDefs;
	*definitionCount = nbrOfDefinitions;
	*specs = extensionSpecs;
	*specCount = nbrOfSpecs;
}

void	getExtensionDefs(ExtensionDef **array, int *count)
{
	ExtensionSpec	*specs;		/*	For compatibility.	*/
	int		specCount;	/*	For compatibility.	*/

	getExtInfo(array, count, &specs, &specCount);
}

ExtensionDef	*findExtensionDef(BpBlockType type)
{
	ExtensionDef	*extensions;
	int		extensionsCt;
	int		i;
	ExtensionDef	*def;

	if (type == 0) return NULL;
	getExtensionDefs(&extensions, &extensionsCt);
	for (i = 0, def = extensions; i < extensionsCt; i++, def++)
	{
		if (def->type == type)
		{
			return def;
		}
	}

	return NULL;
}

void	getExtensionSpecs(ExtensionSpec **array, int *count)
{
	ExtensionDef	*definitions;	/*	For compatibility.	*/
	int		definitionCount;/*	For compatibility.	*/

	getExtInfo(&definitions, &definitionCount, array, count);
}

ExtensionSpec	*findExtensionSpec(BpBlockType type, char tag)
{
	ExtensionSpec	*extensions;
	int		extensionsCt;
	int		i;
	ExtensionSpec	*spec;

	if (type == 0) return NULL;
	getExtensionSpecs(&extensions, &extensionsCt);
	for (i = 0, spec = extensions; i < extensionsCt; i++, spec++)
	{
		if (spec->type == type && spec->tag == tag)
		{
			return spec;
		}
	}

	return NULL;
}

/******************************************************************************
 *                OPERATIONS ON OUTBOUND EXTENSION BLOCKS                     *
 ******************************************************************************/

static int	insertExtensionBlock(ExtensionBlock *newBlk, Object blkAddr,
			Bundle *bundle)
{
	Sdr	sdr = getIonsdr();

	CHKERR(newBlk);
	CHKERR(bundle);
	if (sdr_list_insert_last(sdr, bundle->extensions, blkAddr) == 0)
	{
		putErrmsg("Failed inserting extension block.", NULL);
		return -1;
	}

	sdr_write(sdr, blkAddr, (char *) newBlk, sizeof(ExtensionBlock));
	return 0;
}

static unsigned char	selectBlkNumber(Bundle *bundle)
{
	Sdr		sdr = getIonsdr();
	unsigned char	maxBlkNumber = 1;	/*	Payload.	*/
	Object		elt;
	Object		blkObj;
			OBJ_POINTER(ExtensionBlock, blk);

	if (bundle->lastBlkNumber == 0)
	{
		for (elt = sdr_list_first(sdr, bundle->extensions); elt;
				elt = sdr_list_next(sdr, elt))
		{
			blkObj = sdr_list_data(sdr, elt);
			GET_OBJ_POINTER(sdr, ExtensionBlock, blk, blkObj);
			if (blk->number > maxBlkNumber)
			{
				maxBlkNumber = blk->number;
			}
		}

		bundle->lastBlkNumber = maxBlkNumber;
	}

	bundle->lastBlkNumber += 1;
	return bundle->lastBlkNumber;
}

Object	attachExtensionBlock(BpBlockType type, ExtensionBlock *blk,
		Bundle *bundle)
{
	Object	blkAddr;
	int	additionalOverhead;

	CHKERR(blk);
	CHKERR(bundle);
	blk->type = type;
	blk->number = selectBlkNumber(bundle);
	blkAddr = sdr_malloc(getIonsdr(), sizeof(ExtensionBlock));
	CHKERR(blkAddr);
	if (insertExtensionBlock(blk, blkAddr, bundle) < 0)
	{
		putErrmsg("Failed attaching extension block.", NULL);
		return 0;
	}

	bundle->extensionsLength += blk->length;
	additionalOverhead = SDR_LIST_ELT_OVERHEAD + sizeof(ExtensionBlock)
			+ blk->length + blk->size;
	bundle->dbOverhead += additionalOverhead;
	return blkAddr;
}

int	copyExtensionBlocks(Bundle *newBundle, Bundle *oldBundle)
{
	Sdr		sdr = getIonsdr();
	Object		elt;
	Object		blkAddr;
			OBJ_POINTER(ExtensionBlock, oldBlk);
	Object		newBlkAddr;
	ExtensionBlock	newBlk;
	char		*buf = NULL;
	unsigned int	buflen = 0;
	ExtensionDef	*def;

	CHKERR(newBundle);
	CHKERR(oldBundle);
	newBundle->extensions = sdr_list_create(sdr);
	CHKERR(newBundle->extensions);
	newBundle->extensionsLength = oldBundle->extensionsLength;
//puts("...in copyExtensionBlocks...");
	if (oldBundle->extensions == 0)
	{
		return 0;
	}

	for (elt = sdr_list_first(sdr, oldBundle->extensions); elt;
			elt = sdr_list_next(sdr, elt))
	{
		blkAddr = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, ExtensionBlock, oldBlk, blkAddr);
		if (oldBlk->length > buflen)
		{
			if (buf)
			{
				MRELEASE(buf);
			}

			buflen = oldBlk->length;
			buf = MTAKE(buflen);
			if (buf == NULL)
			{
				putErrmsg("Extension too big to copy.",
						itoa(buflen));
				return -1;
			}
		}

		memset((char *) &newBlk, 0, sizeof(ExtensionBlock));
//printf("Copying extension block of type %d.\n", oldBlk->type);
		newBlk.type = oldBlk->type;
		newBlk.blkProcFlags = oldBlk->blkProcFlags;
		newBlk.dataLength = oldBlk->dataLength;
		newBlk.length = oldBlk->length;
		if (newBlk.length == 0)
		{
			newBlk.bytes = 0;
		}
		else
		{
			newBlk.bytes = sdr_malloc(sdr, newBlk.length);
			CHKERR(newBlk.bytes);
			sdr_read(sdr, buf, oldBlk->bytes, newBlk.length);
			sdr_write(sdr, newBlk.bytes, buf, newBlk.length);
		}

		def = findExtensionDef(oldBlk->type);
//if (def == NULL) puts("Block definition not found.");
		if (def && def->copy)
		{
//puts("Block definition has copy function.");
			/*	Must copy extension object.		*/

			if (def->copy(&newBlk, oldBlk) < 0)
			{
				putErrmsg("Can't copy extension obj.",
						utoa(oldBlk->size));
				return -1;
			}
		}

		newBlk.tag = oldBlk->tag;
		newBlkAddr = sdr_malloc(sdr, sizeof(ExtensionBlock));
		CHKERR(newBlkAddr);
//puts("Inserting extension block copy.");
		if (insertExtensionBlock(&newBlk, newBlkAddr, newBundle) < 0)
		{
			putErrmsg("Failed copying ext. block.", NULL);
			return -1;
		}
	}
//puts("...done with extension blocks...");

	if (buf)
	{
		MRELEASE(buf);
	}

	return 0;
}

void	deleteExtensionBlock(Object elt, int *lengthsTotal)
{
	Sdr		sdr = getIonsdr();
	Object		blkAddr;
			OBJ_POINTER(ExtensionBlock, blk);
	ExtensionDef	*def;

	CHKVOID(elt);
	blkAddr = sdr_list_data(sdr, elt);
	sdr_list_delete(sdr, elt, NULL, NULL);
	GET_OBJ_POINTER(sdr, ExtensionBlock, blk, blkAddr);
	*lengthsTotal -= blk->length;
	def = findExtensionDef(blk->type);
	if (def && def->release)
	{
		def->release(blk);
	}

	if (blk->bytes)
	{
		sdr_free(sdr, blk->bytes);
	}

	sdr_free(sdr, blkAddr);
}

void	destroyExtensionBlocks(Bundle *bundle)
{
	Sdr	sdr = getIonsdr();
	Object	elt;

	CHKVOID(bundle);
	if (bundle->extensions == 0)
	{
		return;
	}

	while (1)
	{
		elt = sdr_list_first(sdr, bundle->extensions);
		if (elt == 0)
		{
			break;
		}

		deleteExtensionBlock(elt, &bundle->extensionsLength);
	}

	sdr_list_destroy(sdr, bundle->extensions, NULL, NULL);
}

Object	findExtensionBlock(Bundle *bundle, BpBlockType type, char tag)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
	Object	addr;
		OBJ_POINTER(ExtensionBlock, blk);

	CHKZERO(bundle);
	for (elt = sdr_list_first(sdr, bundle->extensions); elt;
			elt = sdr_list_next(sdr, elt))
	{
		addr = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, ExtensionBlock, blk, addr);
		if (blk->type == type && blk->tag == tag)
		{
			return elt;
		}
	}

	return 0;
}

// TODO: Rename? THis is the same as findExtensionBlockByNumber?
// TODO Rename this getExtensionBlockElt
Object	getExtensionBlock(Bundle *bundle, unsigned char nbr)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
	Object	addr;
	OBJ_POINTER(ExtensionBlock, blk);

	CHKZERO(bundle);
	for (elt = sdr_list_first(sdr, bundle->extensions); elt;
			elt = sdr_list_next(sdr, elt))
	{
		addr = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, ExtensionBlock, blk, addr);
		if (blk->number == nbr)
		{
			return elt;
		}
	}

	return 0;
}

Object getExtensionBlockObj(Bundle *bundle, unsigned char blockNum)
{
	Sdr	bpSdr = getIonsdr();
	Object	elt = 0;
	Object	addr = 0;
	OBJ_POINTER(ExtensionBlock, blk);

	CHKZERO(bundle);
	for (elt = sdr_list_first(bpSdr, bundle->extensions); elt;
			elt = sdr_list_next(bpSdr, elt))
	{
		addr = sdr_list_data(bpSdr, elt);
		GET_OBJ_POINTER(bpSdr, ExtensionBlock, blk, addr);
		if (blk->number == blockNum)
		{
		    return addr;
		}
	}

	return 0;
}

int	patchExtensionBlocks(Bundle *bundle)
{
	ExtensionSpec	*extensions;
	int		extensionsCt;
	int		i;
	ExtensionSpec	*spec;
	ExtensionDef	*def;
	ExtensionBlock	blk;

	CHKERR(bundle);
	getExtensionSpecs(&extensions, &extensionsCt);
	for (i = 0, spec = extensions; i < extensionsCt; i++, spec++)
	{
		def = findExtensionDef(spec->type);
		if (def == NULL)	/*	Can't insert block.	*/
		{
			continue;
		}

		if (def->offer != NULL
		&& findExtensionBlock(bundle, spec->type, spec->tag) == 0)
		{
			/*	This is a type of extension block that
			 *	the local node normally offers, which
			 *	is missing from the received bundle.
			 *	Insert a block of this type if it is
			 *	appropriate.				*/

			memset((char *) &blk, 0, sizeof(ExtensionBlock));
			blk.type = spec->type;
			blk.tag = spec->tag;
			blk.crcType = spec->crcType;
			if (def->offer(&blk, bundle) < 0)
			{
				putErrmsg("Failed offering patch block.",
						NULL);
				return -1;
			}

			if (blk.length == 0 && blk.size == 0)
			{
				continue;
			}

			if (attachExtensionBlock(spec->type, &blk, bundle) == 0)
			{
				putErrmsg("Failed attaching patch block.",
						NULL);
				return -1;
			}
		}
	}

	return 0;
}

static void	adjustDbOverhead(Bundle *bundle, unsigned int oldLength,
			unsigned int newLength, unsigned int oldSize,
			unsigned int newSize)
{
	bundle->dbOverhead -= oldLength;
	bundle->dbOverhead += newLength;
	bundle->dbOverhead -= oldSize;
	bundle->dbOverhead += newSize;
}

int	processExtensionBlocks(Bundle *bundle, int fnIdx, void *context)
{
	Sdr			sdr = getIonsdr();
	int			oldDbOverhead;
	Object			elt;
	Object			nextElt;
	Object			blkAddr;
	ExtensionBlock		blk;
	ExtensionDef		*def;
	BpExtBlkProcessFn	processExtension;
	unsigned int		oldLength;
	unsigned int		oldSize;
	unsigned int		wasSuppressed;

	CHKERR(bundle);
	oldDbOverhead = bundle->dbOverhead;
	if (bundle->extensions == 0)
	{
		return 0;
	}

	for (elt = sdr_list_first(sdr, bundle->extensions); elt;
			elt = nextElt)
	{
		nextElt = sdr_list_next(sdr, elt);
		blkAddr = sdr_list_data(sdr, elt);
		sdr_stage(sdr, (char *) &blk, blkAddr,
				sizeof(ExtensionBlock));
		def = findExtensionDef(blk.type);
		if (def == NULL
		|| (processExtension = def->process[fnIdx]) == NULL)
		{
			continue;
		}

		oldLength = blk.length;
		oldSize = blk.size;
		wasSuppressed = blk.suppressed;
		if (processExtension(&blk, bundle, context) < 0)
		{
			putErrmsg("Failed processing extension block.",
					def->name);
			return -1;
		}

		if (blk.length == 0)		/*	Scratched.	*/
		{
			bundle->extensionsLength -= oldLength;
			adjustDbOverhead(bundle, oldLength, 0, oldSize, 0);
			deleteExtensionBlock(elt, &bundle->extensionsLength);
			continue;
		}

		/*	Revise aggregate extensions length as necessary.*/

		if (wasSuppressed)
		{
			if (!blk.suppressed)	/*	restore		*/
			{
				bundle->extensionsLength += blk.length;
			}

			/*	Still suppressed: no change.		*/
		}
		else	/*	Wasn't suppressed before.		*/
		{
			if (!blk.suppressed)
			{
				/*	Still not suppressed, but
				 *	length may have changed.
				 *	Subtract the old length and
				 *	add the new length.		*/

				bundle->extensionsLength -= oldLength;
				bundle->extensionsLength += blk.length;
			}
			else	/*	Newly suppressed.	*/
			{
				bundle->extensionsLength -= oldLength;
			}
		}

		if (blk.length != oldLength || blk.size != oldSize)
		{
			adjustDbOverhead(bundle, oldLength, blk.length,
					oldSize, blk.size);
		}

		sdr_write(sdr, blkAddr, (char *) &blk,
				sizeof(ExtensionBlock));
	}

	if (bundle->dbOverhead != oldDbOverhead)
	{
		zco_reduce_heap_occupancy(sdr, oldDbOverhead, bundle->acct);
		zco_increase_heap_occupancy(sdr, bundle->dbOverhead,
				bundle->acct);
	}

	return 0;
}

void	restoreExtensionBlock(ExtensionBlock *blk)
{
	CHKVOID(blk);
	blk->suppressed = 0;
}

void	scratchExtensionBlock(ExtensionBlock *blk)
{
	CHKVOID(blk);
	blk->length = 0;
}

int	serializeExtBlk(ExtensionBlock *blk, char *blockData)
{
	Sdr		sdr = getIonsdr();
	int		crcSize;
	unsigned int	buflen;
	unsigned char	*blkBuffer;
	unsigned char	*cursor;
	uvast		uvtemp;
	unsigned char	*startOfCrc;
	uvast		crcComputed;
	uint16_t	crc16;
	uint32_t	crc32;

	CHKERR(blk);
	switch(blk->crcType)
	{
	case NoCRC:
		crcSize = 0;
		break;

	case X25CRC16:
		crcSize = 3;
		break;

	case CRC32C:
		crcSize = 5;
		break;

	default:
		putErrmsg("Invalid crcType.", itoa(blk->crcType));
		return -1;
	}

	/*	First serialize the block into a temporary buffer
	 *	in working memory.  The size of the buffer is
	 *	computed as:
	 *		1 for size of CBOR array open
	 *		9 for size of CBOR integer (block type)
	 *		9 for size of CBOR integer (block number)
	 *		2 for size of CBOR integer (block proc flags)
	 *		1 for size of CBOR integer (CRC type)
	 * 9 + dataLength for size of CBOR byte string (block-specific data)
	 *        crcSize for size of CBOR byte string (CRC)		*/

	buflen = blk->dataLength + crcSize + 31;
	blkBuffer = MTAKE(buflen);
	if (blkBuffer == NULL)
	{
		putErrmsg("No space for block buffer.", itoa(buflen));
		return -1;
	}

	cursor = blkBuffer;
	uvtemp = (crcSize == 0 ? 5 : 6);
	oK(cbor_encode_array_open(uvtemp, &cursor));
	uvtemp = blk->type;
	oK(cbor_encode_integer(uvtemp, &cursor));
	uvtemp = blk->number;
	oK(cbor_encode_integer(uvtemp, &cursor));
	uvtemp = blk->blkProcFlags;
	oK(cbor_encode_integer(uvtemp, &cursor));
	uvtemp = blk->crcType;
	oK(cbor_encode_integer(uvtemp, &cursor));
	uvtemp = blk->dataLength;
	oK(cbor_encode_byte_string((unsigned char *) blockData, uvtemp,
			&cursor));

	/*	Compute and encode CRC as required.			*/

	if (blk->crcType != NoCRC)
	{
		startOfCrc = cursor + 1;
		uvtemp = 0;	/*	CRC value is 0 for computation.	*/
		if (blk->crcType == X25CRC16)
		{
			oK(cbor_encode_byte_string((unsigned char *) &uvtemp,
					2, &cursor));
		}
		else		/*	CRC32C.				*/
		{
			oK(cbor_encode_byte_string((unsigned char *) &uvtemp,
					4, &cursor));
		}

		crcComputed = computeBufferCrc(blk->crcType, blkBuffer,
				cursor - blkBuffer, 1, 0, NULL);
		if (blk->crcType == X25CRC16)
		{
			crc16 = crcComputed;
			crc16 = htons(crc16);
			memcpy(startOfCrc, (char *) &crc16, 2);
		}
		else		/*	CRC32C.				*/
		{
			crc32 = crcComputed;
			crc32 = htonl(crc32);
			memcpy(startOfCrc, (char *) &crc32, 4);
		}
	}

	/*	Then allocate enough SDR heap space to hold the
	 *	serialized block.					*/

	if (blk->bytes)
	{
		sdr_free(sdr, blk->bytes);
	}

	blk->length = cursor - blkBuffer;
	blk->bytes = sdr_malloc(sdr, blk->length);
	if (blk->bytes == 0)
	{
		putErrmsg("No space for block.", itoa(blk->length));
		return -1;
	}

	/*	Finally, copy the serialized block from the working
	 *	memory buffer into the SDR heap and free the buffer.	*/

	sdr_write(sdr, blk->bytes, (char *) blkBuffer, blk->length);
	MRELEASE(blkBuffer);
	return 0;
}

void	suppressExtensionBlock(ExtensionBlock *blk)
{
	CHKVOID(blk);
	blk->suppressed = 1;
}

/******************************************************************************
 *                  OPERATIONS ON INBOUND EXTENSION BLOCKS                    *
 ******************************************************************************/

int	acquireExtensionBlock(AcqWorkArea *work, ExtensionDef *def,
		unsigned char *startOfBlock, unsigned int blockLength,
		BpBlockType blkType, unsigned int blkNumber,
		unsigned char blkProcFlags, unsigned int dataLength)
{
	Bundle		*bundle = &(work->bundle);
	int		blkSize;
	AcqExtBlock	*blk;
	LystElt		elt;
	int		additionalOverhead;

	CHKERR(work);
	CHKERR(startOfBlock);
	blkSize = sizeof(AcqExtBlock) + (blockLength - 1);

	/*	(blockLength - 1) because blkSize is inflated by 1
	 *	due to the inclusion of the placeholding 1-byte
	 *	character array in "bytes".				*/

	blk = (AcqExtBlock *) MTAKE(blkSize);
	if (blk == NULL)
	{
		putErrmsg("Can't acquire extension block.", itoa(blkSize));
		return -1;
	}

	/*	Populate the extension block structure.  The entire
	 *	original block, except for any terminating CRC, is
	 *	copied into the bytes[] of the AcqExtBlock object.	*/

	memset((char *) blk, 0, sizeof(AcqExtBlock));
	blk->type = blkType;
	blk->number = blkNumber;
	blk->blkProcFlags = blkProcFlags;
	blk->dataLength = dataLength;
	blk->length = blockLength;
	memcpy(blk->bytes, startOfBlock, blockLength);

	/*	Store extension block within bundle.			*/

	elt = lyst_insert_last(work->extBlocks, blk);
	if (elt == NULL)
	{
		MRELEASE(blk);
		putErrmsg("Can't acquire extension block.", NULL);
		return -1;
	}

	/*	Block-specific processing.				*/

	if (def && def->acquire)
	{
		switch (def->acquire(blk, work))
		{
		case 0:		/*	Malformed block.		*/
			work->malformed = 1;
			break;

		case 1:		/*	No problem.			*/
			break;

		default:	/*	System failure.			*/
			lyst_delete(elt);
			MRELEASE(blk);
			putErrmsg("Can't acquire extension block.", def->name);
			return -1;
		}

		if (blk->length == 0)	/*	Discarded.		*/
		{
			deleteAcqExtBlock(elt);
			return 0;
		}
	}

	/*	Note that this calculation of dbObverhead is tentative,
	 *	to aid in making acquisition decisions.  The real
	 *	calculation happens when the bundle is accepted and
	 *	the extension blocks are recorded.			*/

	bundle->extensionsLength += blk->length;
	additionalOverhead = SDR_LIST_ELT_OVERHEAD + sizeof(ExtensionBlock)
			+ blk->length + blk->size;
	bundle->dbOverhead += additionalOverhead;
	return 0;
}

int	reviewExtensionBlocks(AcqWorkArea *work)
{
	ExtensionDef	*extensions;
	int		extensionsCt;
	int		i;
	ExtensionDef	*def;

	CHKZERO(work);
	getExtensionDefs(&extensions, &extensionsCt);
	for (i = 0, def = extensions; i < extensionsCt; i++, def++)
	{
		if (def->review)
		{
			if (def->review(work) == 0)
			{
				/*	A required extension block
					is missing.			*/

				return 0;
			}
		}
	}

	return 1;
}

int	parseExtensionBlocks(AcqWorkArea *work)
{
	Bundle		*bundle = &(work->bundle);
	LystElt		elt;
	LystElt		nextElt;
	AcqExtBlock	*blk;
	ExtensionDef	*def;
	unsigned int	oldLength;

	CHKERR(work);
	for (elt = lyst_first(work->extBlocks); elt; elt = nextElt)
	{
		nextElt = lyst_next(elt);
		blk = (AcqExtBlock *) lyst_data(elt);

		/*	Now do block-specific parsing.			*/

		def = findExtensionDef(blk->type);
		if (def == NULL || def->parse == NULL)
		{
			continue;
		}

		oldLength = blk->length;
		switch (def->parse(blk, work))
		{
		case 0:			/*	Malformed block.	*/
			work->malformed = 1;
			break;

		case 1:			/*	No problem.		*/
			break;

		default:		/*	System failure.		*/
			lyst_delete(elt);
			MRELEASE(blk);
			putErrmsg("Can't parse extension block.",
					def->name);
			return -1;
		}

		if (blk->length == 0)	/*	Block is discarded.	*/
		{
			bundle->extensionsLength -= oldLength;
			deleteAcqExtBlock(elt);
			return 0;
		}

		/*	Revise aggregate extensions length as necessary.*/

		if (blk->length != oldLength)
		{
			bundle->extensionsLength -= oldLength;
			bundle->extensionsLength += blk->length;
		}
	}

	return 0;
}

LystElt	getAcqExtensionBlock(AcqWorkArea *work, unsigned char nbr)
{
	LystElt		elt;
	AcqExtBlock	*blk;

	CHKZERO(work);
	for (elt = lyst_first(work->extBlocks); elt; elt = lyst_next(elt))
	{
		blk = (AcqExtBlock *) lyst_data(elt);
		if (blk->number == nbr)
		{
			return elt;
		}
	}

	return NULL;
}

void	deleteAcqExtBlock(LystElt elt)
{
	AcqExtBlock	*blk;
	ExtensionDef	*def;

	CHKVOID(elt);
	blk = (AcqExtBlock *) lyst_data(elt);
	def = findExtensionDef(blk->type);
	if (def && def->clear)
	{
		def->clear(blk);
	}

	MRELEASE(blk);
	lyst_delete(elt);
}

void	discardAcqExtensionBlock(AcqExtBlock *blk)
{
	CHKVOID(blk);
	blk->length = 0;
}

int	recordExtensionBlocks(AcqWorkArea *work)
{
	Sdr		sdr = getIonsdr();
	Bundle		*bundle = &(work->bundle);
	LystElt		elt;
	AcqExtBlock	*oldBlk;
	ExtensionBlock	newBlk;
	int		headerLength;
	ExtensionDef	*def;
	Object		newBlkAddr;
	int		additionalOverhead;

	CHKERR(work);
	bundle->extensions = sdr_list_create(sdr);
	CHKERR(bundle->extensions);
	bundle->extensionsLength = 0;
	for (elt = lyst_first(work->extBlocks); elt; elt = lyst_next(elt))
	{
		oldBlk = (AcqExtBlock *) lyst_data(elt);
		memset((char *) &newBlk, 0, sizeof(ExtensionBlock));
		newBlk.type = oldBlk->type;
		newBlk.number = oldBlk->number;
		newBlk.blkProcFlags = oldBlk->blkProcFlags;
		newBlk.dataLength = oldBlk->dataLength;
		headerLength = oldBlk->length - oldBlk->dataLength;
		if (serializeExtBlk(&newBlk, (char *)
				(oldBlk->bytes + headerLength)) < 0)
		{
			putErrmsg("No space for block.", NULL);
			return -1;
		}

		/*	Note that the new outbound extension block's
		 *	tag is set to zero by default.  The "record"
		 *	function for this type of block (def->record
		 *	below) may set it to a different value as
		 *	needed; tags have extension-specific semantics.	*/

		def = findExtensionDef(oldBlk->type);
		if (def && def->record)
		{
			/*	Record extension object.		*/

			if (def->record(&newBlk, oldBlk) < 0)
			{
				putErrmsg("Failed recording ext. obj.", NULL);
				return -1;
			}
		}

		newBlkAddr = sdr_malloc(sdr, sizeof(ExtensionBlock));
		CHKERR(newBlkAddr);
		if (insertExtensionBlock(&newBlk, newBlkAddr, bundle) < 0)
		{
			putErrmsg("Failed recording ext. block.", NULL);
			return -1;
		}

		bundle->extensionsLength += newBlk.length;
		additionalOverhead = SDR_LIST_ELT_OVERHEAD
				+ sizeof(ExtensionBlock)
				+ newBlk.length + newBlk.size;
		bundle->dbOverhead += additionalOverhead;
	}

	return 0;
}
