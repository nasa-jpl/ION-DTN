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

ExtensionDef	*findExtensionDef(unsigned char type)
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

ExtensionSpec	*findExtensionSpec(unsigned char type, unsigned char tag1,
			unsigned char tag2, unsigned char tag3)
{
	ExtensionSpec	*extensions;
	int		extensionsCt;
	int		i;
	ExtensionSpec	*spec;

	if (type == 0) return NULL;
	getExtensionSpecs(&extensions, &extensionsCt);
	for (i = 0, spec = extensions; i < extensionsCt; i++, spec++)
	{
		if (spec->type == type && spec->tag1 == tag1
				&& spec->tag2 == tag2
				&& spec->tag3 == tag3)
		{
			return spec;
		}
	}

	return NULL;
}

static unsigned int	getExtensionRank(ExtensionSpec *spec)
{
	ExtensionSpec	*extensions;
	int		extensionsCt;

	getExtensionSpecs(&extensions, &extensionsCt);
	return ((uaddr) spec - (uaddr) extensions) / sizeof(ExtensionSpec);
}

/******************************************************************************
 *                OPERATIONS ON OUTBOUND EXTENSION BLOCKS                     *
 ******************************************************************************/

static int	insertExtensionBlock(ExtensionSpec *spec,
			ExtensionBlock *newBlk, Object blkAddr, Bundle *bundle,
			unsigned char listIdx)
{
	Sdr	bpSdr = getIonsdr();
	int	result;
	Object	elt;
		OBJ_POINTER(ExtensionBlock, blk);

	CHKERR(newBlk);
	CHKERR(bundle);
	if (spec == NULL)	/*	Don't care where this goes.	*/
	{
		if (listIdx == 0)	/*	Pre-payload block.	*/
		{
			/*	Insert after all pre-payload blocks
			 *	inserted by the local node and after
			 *	all preceding pre-payload blocks
			 *	inserted by upstream nodes.		*/

			newBlk->rank = 255;
		}
		else			/*	Post-payload block.	*/
		{
			/*	Insert *before* all post-payload blocks
			 *	inserted by the local node and after
			 *	all preceding post-payload blocks
			 *	inserted by upstream nodes.		*/

			newBlk->rank = 0;
		}
	}
	else			/*	Order in list is important.	*/
	{
		newBlk->rank = getExtensionRank(spec);
	}

	for (elt = sdr_list_first(bpSdr, bundle->extensions[listIdx]);
			elt; elt = sdr_list_next(bpSdr, elt))
	{
		GET_OBJ_POINTER(bpSdr, ExtensionBlock, blk,
				sdr_list_data(bpSdr, elt));
		if (blk->rank > newBlk->rank)
		{
			break;	/*	Found a higher-ranking block.	*/
		}
	}

	if (elt)	/*	Add before first higher-ranking block.	*/
	{
		result = sdr_list_insert_before(bpSdr, elt, blkAddr);
	}
	else		/*	There is no higher-ranking block.	*/
	{
		result = sdr_list_insert_last(bpSdr,
				bundle->extensions[listIdx], blkAddr);
	}

	if (result == 0)
	{
		putErrmsg("Failed inserting extension block.", NULL);
		return -1;
	}

	sdr_write(bpSdr, blkAddr, (char *) newBlk, sizeof(ExtensionBlock));
	return 0;
}

int	attachExtensionBlock(ExtensionSpec *spec, ExtensionBlock *blk,
		Bundle *bundle)
{
	Object	blkAddr;
	int	additionalOverhead;

	CHKERR(spec);
	CHKERR(blk);
	CHKERR(bundle);
	blk->type = spec->type;
#ifdef ORIGINAL_BSP
	if (blk->type == BSP_BAB_TYPE)
#elif ORIGINAL_SBSP
	if (blk->type == EXTENSION_TYPE_BAB)
#else
	if(0)
#endif
	{
		blk->occurrence = spec->listIdx;	/*	0 or 1.	*/
	}
	else
	{
		blk->occurrence = 0;
	}

	/*	If we ever devise any extension blocks that occur
	 *	multiple times in a single bundle -- other than
	 *	the BAB -- then we need to insert here a procedure
	 *	that determines the maximum occurrence number among
	 *	all blocks of this type that are currently in the
	 *	bundle, adds 1 to that number, and inserts into the
	 *	block a bogus EID reference that documents this
	 *	assigned occurrence number.  (For that bogus EID
	 *	reference number, the scheme offset is the occurrence
	 *	number and the SSP offset is set to zero to indicate
	 *	that the EID reference is bogus).
	 *
	 *	For now, though, there are no such blocks and the
	 *	absence of a bogus EID reference is interpreted
	 *	as an indication that the occurrence number of
	 *	the block is zero, i.e., it is the first and only
	 *	block of its type in this bundle.			*/

	blkAddr = sdr_malloc(getIonsdr(), sizeof(ExtensionBlock));
	CHKERR(blkAddr);
	if (insertExtensionBlock(spec, blk, blkAddr, bundle, spec->listIdx) < 0)
	{
		putErrmsg("Failed attaching extension block.", NULL);
		return -1;
	}

	bundle->extensionsLength[spec->listIdx] += blk->length;
	additionalOverhead = SDR_LIST_ELT_OVERHEAD + sizeof(ExtensionBlock)
			+ blk->length + blk->size;
	bundle->dbOverhead += additionalOverhead;
	return 0;
}

int	copyExtensionBlocks(Bundle *newBundle, Bundle *oldBundle)
{
	Sdr		bpSdr = getIonsdr();
	int		i;
	Object		elt;
	Object		blkAddr;
			OBJ_POINTER(ExtensionBlock, oldBlk);
	Object		newBlkAddr;
	ExtensionBlock	newBlk;
	char		*buf = NULL;
	unsigned int	buflen = 0;
	ExtensionDef	*def;
	ExtensionSpec	*spec;

	CHKERR(newBundle);
	CHKERR(oldBundle);
#ifdef ORIGINAL_BSP
	copyCollaborationBlocks(newBundle, oldBundle);
#endif
	for (i = 0; i < 2; i++)
	{
		newBundle->extensions[i] = sdr_list_create(bpSdr);
		CHKERR(newBundle->extensions[i]);
		if (oldBundle->extensions[i] == 0)
		{
			continue;
		}

		for (elt = sdr_list_first(bpSdr, oldBundle->extensions[i]); elt;
				elt = sdr_list_next(bpSdr, elt))
		{
			blkAddr = sdr_list_data(bpSdr, elt);
			GET_OBJ_POINTER(bpSdr, ExtensionBlock, oldBlk, blkAddr);
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
			newBlk.type = oldBlk->type;
			newBlk.blkProcFlags = oldBlk->blkProcFlags;
			newBlk.dataLength = oldBlk->dataLength;
			if (oldBlk->eidReferences == 0)
			{
				newBlk.eidReferences = 0;
			}
			else
			{
				newBlk.eidReferences = sdr_list_create(bpSdr);
				CHKERR(newBlk.eidReferences);
				for (elt = sdr_list_first(bpSdr,
						oldBlk->eidReferences); elt;
						elt = sdr_list_next(bpSdr, elt))
				{
					if (sdr_list_insert_last(bpSdr,
						newBlk.eidReferences,
						sdr_list_data(bpSdr, elt)) == 0)
					{
						putErrmsg("No space for EIDR.",
								NULL);
						return -1;
					}
				}
			}

			newBlk.length = oldBlk->length;
			if (newBlk.length == 0)
			{
				newBlk.bytes = 0;
			}
			else
			{
				newBlk.bytes = sdr_malloc(bpSdr, newBlk.length);
				CHKERR(newBlk.bytes);
				sdr_read(bpSdr, buf, oldBlk->bytes,
						newBlk.length);
				sdr_write(bpSdr, newBlk.bytes, buf,
						newBlk.length);
			}

			def = findExtensionDef(oldBlk->type);
			if (def && def->copy)
			{
				/*	Must copy extension object.	*/

				if (def->copy(&newBlk, oldBlk) < 0)
				{
					putErrmsg("Can't copy extension obj.",
							utoa(oldBlk->size));
					return -1;
				}
			}

			newBlk.tag1 = oldBlk->tag1;
			newBlk.tag2 = oldBlk->tag2;
			newBlk.tag3 = oldBlk->tag3;
			spec = findExtensionSpec(newBlk.type, newBlk.tag1,
					newBlk.tag2, newBlk.tag3);
			newBlkAddr = sdr_malloc(bpSdr, sizeof(ExtensionBlock));
			CHKERR(newBlkAddr);
			if (insertExtensionBlock(spec, &newBlk, newBlkAddr,
					newBundle, i) < 0)
			{
				putErrmsg("Failed copying ext. block.", NULL);
				return -1;
			}
		}

		newBundle->extensionsLength[i] = oldBundle->extensionsLength[i];
	}

	if (buf)
	{
		MRELEASE(buf);
	}

	return 0;
}

void	deleteExtensionBlock(Object elt, int *lengthsTotal)
{
	Sdr		bpSdr = getIonsdr();
	Object		blkAddr;
			OBJ_POINTER(ExtensionBlock, blk);
	ExtensionDef	*def;

	CHKVOID(elt);
	blkAddr = sdr_list_data(bpSdr, elt);
	sdr_list_delete(bpSdr, elt, NULL, NULL);
	GET_OBJ_POINTER(bpSdr, ExtensionBlock, blk, blkAddr);
	*lengthsTotal -= blk->length;
	def = findExtensionDef(blk->type);
	if (def && def->release)
	{
		def->release(blk);
	}

	if (blk->eidReferences)
	{
		sdr_list_destroy(bpSdr, blk->eidReferences, NULL, NULL);
	}

	if (blk->bytes)
	{
		sdr_free(bpSdr, blk->bytes);
	}

	sdr_free(bpSdr, blkAddr);
}

void	destroyExtensionBlocks(Bundle *bundle)
{
	Sdr	bpSdr = getIonsdr();
	int	i;
	Object	elt;

	CHKVOID(bundle);
	for (i = 0; i < 2; i++)
	{
		if (bundle->extensions[i] == 0)
		{
			continue;
		}

		while (1)
		{
			elt = sdr_list_first(bpSdr, bundle->extensions[i]);
			if (elt == 0)
			{
				break;
			}

			deleteExtensionBlock(elt, &bundle->extensionsLength[i]);
		}

		sdr_list_destroy(bpSdr, bundle->extensions[i], NULL, NULL);
	}
}

Object	findExtensionBlock(Bundle *bundle, unsigned int type, 
		unsigned char tag1, unsigned char tag2, unsigned char tag3)
{
	Sdr	bpSdr = getIonsdr();
	int	idx;
	Object	elt;
	Object	addr;
		OBJ_POINTER(ExtensionBlock, blk);

	CHKZERO(bundle);
	for (idx = 0; idx < 2; idx++)
	{
		for (elt = sdr_list_first(bpSdr, bundle->extensions[idx]); elt;
				elt = sdr_list_next(bpSdr, elt))
		{
			addr = sdr_list_data(bpSdr, elt);
			GET_OBJ_POINTER(bpSdr, ExtensionBlock, blk, addr);
			if (blk->type == type && blk->tag1 == tag1
			&& blk->tag2 == tag2 && blk->tag3 == tag3)
			{
				return elt;
			}
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
		&& findExtensionBlock(bundle, spec->type, spec->tag1,
				spec->tag2, spec->tag3) == 0)
		{
			/*	This is a type of extension block that
			 *	the local node normally offers, which
			 *	is missing from the received bundle.
			 *	Insert a block of this type if it is
			 *	appropriate.				*/

			memset((char *) &blk, 0, sizeof(ExtensionBlock));
			blk.type = spec->type;
			blk.tag1 = spec->tag1;
			blk.tag2 = spec->tag2;
			blk.tag3 = spec->tag3;
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

			if (attachExtensionBlock(spec, &blk, bundle) < 0)
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
	Sdr			bpSdr = getIonsdr();
	int			oldDbOverhead;
	int			i;
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
	for (i = 0; i < 2; i++)
	{
		if (bundle->extensions[i] == 0)
		{
			continue;
		}

		for (elt = sdr_list_first(bpSdr, bundle->extensions[i]); elt;
				elt = nextElt)
		{
			nextElt = sdr_list_next(bpSdr, elt);
			blkAddr = sdr_list_data(bpSdr, elt);
			sdr_stage(bpSdr, (char *) &blk, blkAddr,
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

			if (blk.length == 0)	/*	Scratched.	*/
			{
				bundle->extensionsLength[i] -= oldLength;
				adjustDbOverhead(bundle, oldLength, 0,
						oldSize, 0);
				deleteExtensionBlock(elt,
						&bundle->extensionsLength[i]);
				continue;
			}

			/*	Revise aggregate extensions length as
			 *	necessary.				*/

			if (wasSuppressed)
			{
				if (!blk.suppressed)	/*	restore	*/
				{
					bundle->extensionsLength[i]
							+= blk.length;
				}

				/*	Still suppressed: no change.	*/
			}
			else	/*	Wasn't suppressed before.	*/
			{
				if (!blk.suppressed)
				{
					/*	Still not suppressed,
					 *	but length may have
					 *	changed.  Subtract the
					 *	old length and add the
					 *	new length.		*/

					bundle->extensionsLength[i]
							-= oldLength;
					bundle->extensionsLength[i]
							+= blk.length;
				}
				else	/*	Newly suppressed.	*/
				{
					bundle->extensionsLength[i]
							-= oldLength;
				}
			}

			if (blk.length != oldLength || blk.size != oldSize)
			{
				adjustDbOverhead(bundle, oldLength, blk.length,
						oldSize, blk.size);
			}

			sdr_write(bpSdr, blkAddr, (char *) &blk,
					sizeof(ExtensionBlock));
		}
	}

	if (bundle->dbOverhead != oldDbOverhead)
	{
		zco_reduce_heap_occupancy(bpSdr, oldDbOverhead, bundle->acct);
		zco_increase_heap_occupancy(bpSdr, bundle->dbOverhead,
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

int	serializeExtBlk(ExtensionBlock *blk, Lyst eidReferences,
		char *blockData)
{
	Sdr		bpSdr = getIonsdr();
	unsigned int	blkProcFlags;
	Sdnv		blkProcFlagsSdnv;
	unsigned int	dataLength;
	Sdnv		dataLengthSdnv;
	int		listLength;
	LystElt		elt;
	uaddr		offset;
	Sdnv		offsetSdnv;
	unsigned int	referenceCount;
	Sdnv		referenceCountSdnv;
	char		*blkBuffer;
	char		*cursor;

	CHKERR(blk);
	blkProcFlags = blk->blkProcFlags;
	dataLength = blk->dataLength;
	if (blk->bytes)
	{
		sdr_free(bpSdr, blk->bytes);
	}

	blk->bytes = 0;
	if (blk->eidReferences)
	{
		sdr_list_destroy(bpSdr, blk->eidReferences, NULL, NULL);
	}

	blk->eidReferences = 0;
	encodeSdnv(&blkProcFlagsSdnv, blkProcFlags);
	encodeSdnv(&dataLengthSdnv, dataLength);
	blk->length = 1 + blkProcFlagsSdnv.length + dataLengthSdnv.length
			+ dataLength;
	if (eidReferences != NULL)
	{
		listLength = lyst_length(eidReferences);

		/*	Each EID reference is a pair of dictionary
		 *	offsets, i.e., two list elements.		*/ 

		if (listLength & 0x00000001)	/*	Not pairs.	*/
		{
			putErrmsg("Invalid EID references list.", NULL);
			return -1;
		}

		blkProcFlags |= BLK_HAS_EID_REFERENCES;
		referenceCount = (listLength >> 1) & 0x7fffffff;
		encodeSdnv(&referenceCountSdnv, referenceCount);
		blk->length += referenceCountSdnv.length;
		blk->eidReferences = sdr_list_create(bpSdr);
		CHKERR(blk->eidReferences);
		for (elt = lyst_first(eidReferences); elt; elt = lyst_next(elt))
		{
			offset = (uaddr) lyst_data(elt);
			encodeSdnv(&offsetSdnv, offset);
			blk->length += offsetSdnv.length;
			oK(sdr_list_insert_last(bpSdr,
					blk->eidReferences, offset));
		}
	}

	blk->bytes = sdr_malloc(bpSdr, blk->length);
	if (blk->bytes == 0)
	{
		putErrmsg("No space for block.", itoa(blk->length));
		return -1;
	}

	blkBuffer = MTAKE(blk->length);
	if (blkBuffer == NULL)
	{
		putErrmsg("No space for block buffer.", itoa(blk->length));
		return -1;
	}

	*blkBuffer = blk->type;
	cursor = blkBuffer + 1;
	memcpy(cursor, blkProcFlagsSdnv.text, blkProcFlagsSdnv.length);
	cursor += blkProcFlagsSdnv.length;
	if (eidReferences)
	{
		memcpy(cursor, referenceCountSdnv.text,
				referenceCountSdnv.length);
		cursor += referenceCountSdnv.length;
		for (elt = lyst_first(eidReferences); elt; elt = lyst_next(elt))
		{
			offset = (uaddr) lyst_data(elt);
			encodeSdnv(&offsetSdnv, offset);
			memcpy(cursor, offsetSdnv.text, offsetSdnv.length);
			cursor += offsetSdnv.length;
		}
	}

	memcpy(cursor, dataLengthSdnv.text, dataLengthSdnv.length);
	cursor += dataLengthSdnv.length;
	memcpy(cursor, blockData, dataLength);
	sdr_write(bpSdr, blk->bytes, blkBuffer, blk->length);
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

static int	determineOccurrenceNbr(Lyst eidReferences)
{
	int	listLength;
	LystElt	elt;
	uaddr	schemeOffset;
	uaddr	sspOffset;

	if (eidReferences == NULL)
	{
		return 0;	/*	No eidReferences, so no number.	*/
	}

	/*	Occurrence number is encoded in an EID reference for
	 *	which SSP offset is zero; scheme offset of this
	 *	EID reference is the occurrence number.			*/

	listLength = lyst_length(eidReferences);

	/*	Each EID reference is a pair of dictionary offsets,
	 *	i.e., two list elements.				*/ 

	if (listLength & 0x00000001)	/*	Not pairs.		*/
	{
		return 0;	/*	No valid occurrence nbr.	*/
	}

	for (elt = lyst_first(eidReferences); elt; elt = lyst_next(elt))
	{
		schemeOffset = (uaddr) lyst_data(elt);
		elt = lyst_next(elt);
		sspOffset = (uaddr) lyst_data(elt);
		if (sspOffset == 0)
		{
			return schemeOffset;
		}
	}

	/*	No occurrence number encoded in block, so occurrence
	 *	number is zero.						*/

	return 0;
}

int	acquireExtensionBlock(AcqWorkArea *work, ExtensionDef *def,
		unsigned char *startOfBlock, unsigned int blockLength,
		unsigned char blkType, unsigned int blkProcFlags,
		Lyst *eidReferences, unsigned int dataLength)
{
	Bundle		*bundle = &(work->bundle);
	int		blkSize;
	AcqExtBlock	*blk;
	Sdnv		blkProcFlagsSdnv;
	int		i;
	LystElt		elt;
	int		additionalOverhead;

	CHKERR(work);
	CHKERR(startOfBlock);
	CHKERR(eidReferences);
	blkSize = sizeof(AcqExtBlock) + (blockLength - 1);
	blk = (AcqExtBlock *) MTAKE(blkSize);
	if (blk == NULL)
	{
		putErrmsg("Can't acquire extension block.", itoa(blkSize));
		return -1;
	}

	/*	Populate the extension block structure.			*/

	memset((char *) blk, 0, sizeof(AcqExtBlock));
	blk->type = blkType;
#ifdef ORIGINAL_BSP
	if (blkType == BSP_BAB_TYPE)
#elif ORIGINAL_SBSP
	if (blkType == EXTENSION_TYPE_BAB)
#else
	if(0)
#endif
	{
		blk->occurrence = work->currentExtBlocksList;
	}
	else
	{
		blk->occurrence = determineOccurrenceNbr(*eidReferences);
	}

	blk->blkProcFlags = blkProcFlags;
	blk->eidReferences = *eidReferences;
	*eidReferences = NULL;
	blk->dataLength = dataLength;
	blk->length = blockLength;
	memcpy(blk->bytes, startOfBlock, blockLength);

	/*	Block processing flags may already have been modified.	*/

	encodeSdnv(&blkProcFlagsSdnv, blkProcFlags);
	memcpy(blk->bytes + 1, blkProcFlagsSdnv.text, blkProcFlagsSdnv.length);

	/*	Store extension block within bundle.			*/

	i = work->currentExtBlocksList;
	elt = lyst_insert_last(work->extBlocks[i], blk);
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

	bundle->extensionsLength[i] += blk->length;
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

int	decryptPerExtensionBlocks(AcqWorkArea *work)
{
	Bundle		*bundle = &(work->bundle);
	int		i;
	LystElt		elt;
	LystElt		nextElt;
	AcqExtBlock	*blk;
	ExtensionDef	*def;
	unsigned int	oldLength;

	CHKERR(work);
	for (i = 0; i < 2; i++)
	{
		for (elt = lyst_first(work->extBlocks[i]); elt; elt = nextElt)
		{
			nextElt = lyst_next(elt);
			blk = (AcqExtBlock *) lyst_data(elt);

			/*	Now do block-specific decryption: if
			 *	block has a decrypt callback (i.e.,
			 *	it knows how to decrypt something,
			 *	nominally another block), then do
			 *	that decryption.			*/

			def = findExtensionDef(blk->type);
			if (def == NULL || def->decrypt == NULL)
			{
				continue;
			}

			oldLength = blk->length;
			switch (def->decrypt(blk, work))
			{
			case 0:		/*	Malformed block.	*/
				work->malformed = 1;
				break;

			case 1:		/*	No problem.		*/
				break;

			default:	/*	System failure.		*/
				lyst_delete(elt);
				MRELEASE(blk);
				putErrmsg("Can't decrypt extension block.",
						def->name);
				return -1;
			}

			if (blk->length == 0)	/*	Discarded.	*/
			{
				bundle->extensionsLength[i] -= oldLength;
				deleteAcqExtBlock(elt);
				return 0;
			}

			/*	Revise aggregate extensions length as
			 *	necessary.				*/

			if (blk->length != oldLength)
			{
				bundle->extensionsLength[i] -= oldLength;
				bundle->extensionsLength[i] += blk->length;
			}
		}
	}

	return 0;
}

int	parseExtensionBlocks(AcqWorkArea *work)
{
	Bundle		*bundle = &(work->bundle);
	int		i;
	LystElt		elt;
	LystElt		nextElt;
	AcqExtBlock	*blk;
	ExtensionDef	*def;
	unsigned int	oldLength;

	CHKERR(work);
	for (i = 0; i < 2; i++)
	{
		for (elt = lyst_first(work->extBlocks[i]); elt; elt = nextElt)
		{
			nextElt = lyst_next(elt);
			blk = (AcqExtBlock *) lyst_data(elt);

			/*	Now do block-specific parsing.		*/

			def = findExtensionDef(blk->type);
			if (def == NULL || def->parse == NULL)
			{
				continue;
			}

			oldLength = blk->length;
			switch (def->parse(blk, work))
			{
			case 0:		/*	Malformed block.	*/
				work->malformed = 1;
				break;

			case 1:		/*	No problem.		*/
				break;

			default:	/*	System failure.		*/
				lyst_delete(elt);
				MRELEASE(blk);
				putErrmsg("Can't parse extension block.",
						def->name);
				return -1;
			}

			if (blk->length == 0)	/*	Discarded.	*/
			{
				bundle->extensionsLength[i] -= oldLength;
				deleteAcqExtBlock(elt);
				return 0;
			}

			/*	Revise aggregate extensions length as
			 *	necessary.				*/

			if (blk->length != oldLength)
			{
				bundle->extensionsLength[i] -= oldLength;
				bundle->extensionsLength[i] += blk->length;
			}
		}
	}

	return 0;
}

int	checkPerExtensionBlocks(AcqWorkArea *work)
{
	Bundle		*bundle = &(work->bundle);
	int		i;
	LystElt		elt;
	LystElt		nextElt;
	AcqExtBlock	*blk;
	ExtensionDef	*def;
	unsigned int	oldLength;

	CHKERR(work);
	bundle->clDossier.authentic = work->authentic;
	for (i = 0; i < 2; i++)
	{
		for (elt = lyst_first(work->extBlocks[i]); elt; elt = nextElt)
		{
			nextElt = lyst_next(elt);
			blk = (AcqExtBlock *) lyst_data(elt);
			def = findExtensionDef(blk->type);
			if (def == NULL || def->check == NULL)
			{
				continue;
			}

			oldLength = blk->length;
			switch (def->check(blk, work))
			{
			case 0:		/*	BSP Block is invalid.	*/
				bundle->corrupt = 1;
				break;

			case 1:		/*	No additional info.	*/
				break;

			case 2:		/*	Bundle is inauthentic.	*/
				bundle->clDossier.authentic = 0;
				break;

			case 3:		/*	Bundle is authentic.	*/
				bundle->clDossier.authentic = 1;
				break;

			case 4:		/*	A block is altered.	*/
				bundle->altered = 1;
				break;

			default:
				lyst_delete(elt);
				MRELEASE(blk);
				putErrmsg("Failed checking extension block.",
						def->name);
				return -1;
			}

			if (blk->length == 0)	/*	Discarded.	*/
			{
				bundle->extensionsLength[i] -= oldLength;
				deleteAcqExtBlock(elt);
				continue;
			}

			/*	Revise aggregate extensions length as
			 *	necessary.				*/

			if (blk->length != oldLength)
			{
				bundle->extensionsLength[i] -= oldLength;
				bundle->extensionsLength[i] += blk->length;
			}
		}
	}

	return 0;
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

	if (blk->eidReferences)
	{
		lyst_destroy(blk->eidReferences);
	}

	MRELEASE(blk);
	lyst_delete(elt);
}

void	discardExtensionBlock(AcqExtBlock *blk)
{
	CHKVOID(blk);
	blk->length = 0;
}

LystElt	findAcqExtensionBlock(AcqWorkArea *work, unsigned int type, 
		unsigned int occurrence)
{
	int		idx;
	LystElt		elt;
	AcqExtBlock	*blk;

	CHKNULL(work);
	CHKNULL(type > 0);
	for (idx = 0; idx < 2; idx++)
	{
		for (elt = lyst_first(work->extBlocks[idx]); elt;
				elt = lyst_next(elt))
		{
			blk = (AcqExtBlock *) lyst_data(elt);
			if (blk->type == type && blk->occurrence == occurrence)
			{
				return elt;
			}
		}
	}

	return NULL;
}

int	recordExtensionBlocks(AcqWorkArea *work)
{
	Sdr		bpSdr = getIonsdr();
	Bundle		*bundle = &(work->bundle);
	int		i;
	LystElt		elt;
	AcqExtBlock	*oldBlk;
	ExtensionBlock	newBlk;
	int		headerLength;
	ExtensionDef	*def;
	ExtensionSpec	*spec;
	Object		newBlkAddr;
	int		additionalOverhead;

	CHKERR(work);
#ifdef ORIGINAL_BSP
	bundle->collabBlocks = sdr_list_create(bpSdr);
	CHKERR(bundle->collabBlocks);
#endif /* ORIGINAL_BSP */
	for (i = 0; i < 2; i++)
	{
		bundle->extensions[i] = sdr_list_create(bpSdr);
		CHKERR(bundle->extensions[i]);
		bundle->extensionsLength[i] = 0;
		for (elt = lyst_first(work->extBlocks[i]); elt;
				elt = lyst_next(elt))
		{
			oldBlk = (AcqExtBlock *) lyst_data(elt);
			memset((char *) &newBlk, 0, sizeof(ExtensionBlock));
			newBlk.type = oldBlk->type;
			newBlk.occurrence = oldBlk->occurrence;
			newBlk.blkProcFlags = oldBlk->blkProcFlags;
			newBlk.dataLength = oldBlk->dataLength;
			headerLength = oldBlk->length - oldBlk->dataLength;
			if (serializeExtBlk(&newBlk, oldBlk->eidReferences,
				(char *) (oldBlk->bytes + headerLength)) < 0)
			{
				putErrmsg("No space for block.", NULL);
				return -1;
			}

			def = findExtensionDef(oldBlk->type);
			if (def && def->record)
			{
				/*	Record extension object.	*/

				def->record(&newBlk, oldBlk);
			}

			spec = findExtensionSpec(newBlk.type, newBlk.tag1,
					newBlk.tag2, newBlk.tag3);
			newBlkAddr = sdr_malloc(bpSdr, sizeof(ExtensionBlock));
			CHKERR(newBlkAddr);
			if (insertExtensionBlock(spec, &newBlk, newBlkAddr,
					bundle, i) < 0)
			{
				putErrmsg("Failed recording ext. block.", NULL);
				return -1;
			}

			bundle->extensionsLength[i] += newBlk.length;
			additionalOverhead = SDR_LIST_ELT_OVERHEAD
					+ sizeof(ExtensionBlock)
					+ newBlk.length + newBlk.size;
			bundle->dbOverhead += additionalOverhead;
		}
	}

	return 0;
}

#ifdef ORIGINAL_BSP

/**
 * Adds a collaboration block with the current identifier to the list of
 * collaboration blocks for the current bundle.
 */
int 	addCollaborationBlock(Bundle *bundle, CollabBlockHdr *blkHdr)
{
	/*	First, make sure we don't already have a collaboration block
	 *	with this identifier in the bundle...
	 */

	Sdr		bpSdr = getIonsdr();
	Object		addr;
	CollabBlockHdr	oldHdr;
	Object		newBlkAddr;

	CHKERR(bundle);
	CHKERR(blkHdr);
	addr = findCollaborationBlock(bundle, blkHdr->type, blkHdr->id);
	if (addr != 0)
	{
		sdr_read(bpSdr, (char *) &oldHdr, addr, sizeof(CollabBlockHdr));
		if (oldHdr.size != blkHdr->size)
		{
			putErrmsg("Collab block size mismatch.",
					itoa(oldHdr.size));
			return -1;
		}

		/*	Re-use existing collaboration block.		*/

		sdr_write(bpSdr, addr, (char *) blkHdr, blkHdr->size);
		return 0;
	}

	/*
	 * Otherwise, we are free to add this collaboration block to the bundle.
	 * Add to the end of the list of collab blocks. Order does not matter
	 * for collaboration blocks.
	 */

	newBlkAddr = sdr_malloc(bpSdr, blkHdr->size);
	if (newBlkAddr == 0)
	{
		putErrmsg("Can't acquire collaboration block.",
				itoa(blkHdr->size));
		return -1;
	}

	/* TODO: This may now be a redundant check. Consider removing. */

	if (bundle->collabBlocks == 0)
	{
		bundle->collabBlocks = sdr_list_create(bpSdr);
	}

	/* Insert the new block. */

	if (sdr_list_insert_last(bpSdr, bundle->collabBlocks, newBlkAddr) == 0)
	{
		putErrmsg("Failed inserting collab block.", NULL);
		return -1;
	}

	/* Create the stored collab block and write it to the SDR */

	sdr_write(bpSdr, newBlkAddr, (char *) blkHdr, blkHdr->size);
	return 0;
}

int 	copyCollaborationBlocks(Bundle *newBundle, Bundle *oldBundle)
{
	Sdr	bpSdr = getIonsdr();
	Object	elt;
	Object	blkAddr;
		OBJ_POINTER(CollabBlockHdr, blkHdr);

	CHKERR(newBundle);
	CHKERR(oldBundle);

	/* Destroy collaboration blocks that might be in the new bundle. */
	destroyCollaborationBlocks(newBundle);

	/* Copy the collaboration blocks. */
	newBundle->collabBlocks = sdr_list_create(bpSdr);
	CHKERR(newBundle->collabBlocks);

	/* For each collaboration block that we need to copy... */
	for (elt = sdr_list_first(bpSdr, oldBundle->collabBlocks); elt;
			elt = sdr_list_next(bpSdr, elt))
	{
		blkAddr = sdr_list_data(bpSdr, elt);
		GET_OBJ_POINTER(bpSdr, CollabBlockHdr, blkHdr, blkAddr);
		CHKERR(blkHdr);
		if (addCollaborationBlock(newBundle, blkHdr) < 0)
		{
			putErrmsg("Failed copying collaboration block.", NULL);
			return -1;
		}
	}

	return 0;
}

void 	destroyCollaborationBlocks(Bundle *bundle)
{
	Object	elt;
	Object	blkAddr;
	Sdr	bpSdr = getIonsdr();

	CHKVOID(bundle);
	if (bundle->collabBlocks == 0)
	{
		return;
	}

	while (1)
	{
		elt = sdr_list_first(bpSdr, bundle->collabBlocks);
		if (elt == 0)
		{
			break;
		}

		blkAddr = sdr_list_data(bpSdr, elt);
		sdr_list_delete(bpSdr, elt, NULL, NULL);
		sdr_free(bpSdr, blkAddr);
	}

	sdr_list_destroy(bpSdr, bundle->collabBlocks, NULL, NULL);
	bundle->collabBlocks = 0;
}

Object  findCollaborationBlock(Bundle *bundle, unsigned char type,
		unsigned int id)
{
	Sdr	bpSdr = getIonsdr();
	Object	elt;
	Object	addr;
		OBJ_POINTER(CollabBlockHdr, blkHdr);

	CHKZERO(bundle);
	for (elt = sdr_list_first(bpSdr, bundle->collabBlocks); elt;
			elt = sdr_list_next(bpSdr, elt))
	{
		addr = sdr_list_data(bpSdr, elt);
		GET_OBJ_POINTER(bpSdr, CollabBlockHdr, blkHdr, addr);
		if ((blkHdr->type == type) && (blkHdr->id == id))
		{
			return addr;
		}
	}

	return 0;
}

int  	resizeCollaborationBlock(Bundle *bundle, Object addr, Object data)
{
	/* TODO: Implement. */
	return 0;
}

int 	updateCollaborationBlock(Object collabAddr, CollabBlockHdr *blkHdr)
{
	Sdr bpSdr = getIonsdr();

	CHKERR(collabAddr);
	CHKERR(blkHdr);

	/* TODO: When and if resizing collaboration blocks are supported, must
	 *       check to be sure size did not change if writing collab back to
	 *       sdr.  If it did change, must try and reallocate SDR storage.
	 */

	sdr_write(bpSdr, collabAddr, (char *) blkHdr, blkHdr->size);
	return 0;
}

int 	addAcqCollabBlock(AcqWorkArea *work, CollabBlockHdr *blkHdr)
{
	LystElt		elt;
	char		*blk;

	CHKERR(work);
	CHKERR(blkHdr);
	elt = findAcqCollabBlock(work, blkHdr->type, blkHdr->id);
	if (elt != NULL)
	{
		putErrmsg("Collab block already exists in work area.", NULL);
		return -1;
	}

	blk = MTAKE(blkHdr->size);
	if (blk == NULL)
	{
		putErrmsg("Can't acquire collaboration block.",
				itoa(blkHdr->size));
		return -1;
	}

	/*	Populate the collaboration block structure.		*/
	memcpy(blk, (char *) blkHdr, blkHdr->size);

	/* TODO: This may be a redundant check.  Consider removing. */
	if (work->collabBlocks == 0)
	{
		work->collabBlocks = lyst_create_using(getIonMemoryMgr());
	}

	/*	Store extension block within bundle.			*/
	elt = lyst_insert_last(work->collabBlocks, blk);
	if (elt == NULL)
	{
		MRELEASE(blk);
		putErrmsg("Can't acquire collaboration block.",
				itoa(blkHdr->size));
		return -1;
	}

	return 0;
}

void 	destroyAcqCollabBlocks(AcqWorkArea *work)
{
	LystElt	elt;
	void	*blk = NULL;

	CHKVOID(work);
	while (1)
	{
		elt = lyst_first(work->collabBlocks);
		if (elt == NULL)
		{
			return;
		}

		blk = lyst_data(elt);
		if (blk != NULL)	/*	Should always be true.	*/
		{
			MRELEASE(blk);
		}

		lyst_delete(elt);
	}
}

LystElt	findAcqCollabBlock(AcqWorkArea *work, unsigned char type,
		unsigned int id)
{
	LystElt		elt;
	CollabBlockHdr	*blkHdr;

	CHKNULL(work);
	for (elt = lyst_first(work->collabBlocks); elt; elt = lyst_next(elt))
	{
		blkHdr = (CollabBlockHdr *) lyst_data(elt);
		if (blkHdr == NULL)	/*	Should never happen.	*/
		{
			continue;
		}

		if ((blkHdr->type == type) && (blkHdr->id == id))
		{
			break;
		}
	}

	return elt;
}
#endif /* ORIGINAL_BSP */
