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

/******************************************************************************
 *               OPERATIONS ON THE EXTENSION DEFINITIONS ARRAY                *
 ******************************************************************************/

void	getExtensionDefs(ExtensionDef **array, int *count)
{
#ifdef BP_EXTENDED
#include "bpextensions.c"
#else
#include "noextensions.c"
#endif
	static int	extensionsCt = sizeof extensions / sizeof(ExtensionDef);

	*array = extensions;
	*count = extensionsCt;
}

static unsigned int	getExtensionRank(ExtensionDef *def)
{
	ExtensionDef	*extensions;
	int		extensionsCt;

	getExtensionDefs(&extensions, &extensionsCt);
	return ((unsigned long) def - (unsigned long) extensions)
			/ sizeof(ExtensionDef);
}

ExtensionDef	*findExtensionDef(unsigned char type, unsigned char idx)
{
	ExtensionDef	*extensions;
	int		extensionsCt;
	int		i;
	ExtensionDef	*def;

	if (type == 0) return NULL;
	getExtensionDefs(&extensions, &extensionsCt);
	for (i = 0, def = extensions; i < extensionsCt; i++, def++)
	{
		if (def->type == type && def->listIdx == idx)
		{
			return def;
		}
	}

	return NULL;
}

/******************************************************************************
 *                               BUNDLE OPERATIONS                            *
 ******************************************************************************/

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

int	attachExtensionBlock(ExtensionDef *def, ExtensionBlock *blk,
		Bundle *bundle)
{
	Object	blkAddr;
	int	additionalOverhead;

	CHKERR(def);
	CHKERR(blk);
	CHKERR(bundle);
	blk->type = def->type;
	blkAddr = sdr_malloc(getIonsdr(), sizeof(ExtensionBlock));
	CHKERR(blkAddr);
	if (insertExtensionBlock(def, blk, blkAddr, bundle, def->listIdx) < 0)
	{
		putErrmsg("Failed attaching extension block.", NULL);
		return -1;
	}

	bundle->extensionsLength[def->listIdx] += blk->length;
	additionalOverhead = SDR_LIST_ELT_OVERHEAD + sizeof(ExtensionBlock)
			+ blk->length + blk->size;
	bundle->dbOverhead += additionalOverhead;
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

	CHKERR(newBundle);
	CHKERR(oldBundle);
	copyCollaborationBlocks(newBundle, oldBundle);
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

			def = findExtensionDef(oldBlk->type, i);
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
			else
			{
				newBlk.object = 0;
				newBlk.size = 0;
			}

			newBlkAddr = sdr_malloc(bpSdr, sizeof(ExtensionBlock));
			CHKERR(newBlkAddr);
			if (insertExtensionBlock(def, &newBlk, newBlkAddr,
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

void	deleteExtensionBlock(Object elt, unsigned int listIdx)
{
	Sdr		bpSdr = getIonsdr();
	Object		blkAddr;
			OBJ_POINTER(ExtensionBlock, blk);
	ExtensionDef	*def;

	CHKVOID(elt);
	blkAddr = sdr_list_data(bpSdr, elt);
	sdr_list_delete(bpSdr, elt, NULL, NULL);
	GET_OBJ_POINTER(bpSdr, ExtensionBlock, blk, blkAddr);
	def = findExtensionDef(blk->type, listIdx);
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

			deleteExtensionBlock(elt, i);
		}

		sdr_list_destroy(bpSdr, bundle->extensions[i], NULL, NULL);
	}
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

Object	findExtensionBlock(Bundle *bundle, unsigned int type, unsigned int idx)
{
	Sdr	bpSdr = getIonsdr();
	Object	elt;
	Object	addr;
		OBJ_POINTER(ExtensionBlock, blk);

	CHKZERO(bundle);
	for (elt = sdr_list_first(bpSdr, bundle->extensions[idx]); elt;
			elt = sdr_list_next(bpSdr, elt))
	{
		addr = sdr_list_data(bpSdr, elt);
		GET_OBJ_POINTER(bpSdr, ExtensionBlock, blk, addr);
		if (blk->type == type)
		{
			return elt;
		}
	}

	return 0;
}

int	insertExtensionBlock(ExtensionDef *def, ExtensionBlock *newBlk,
		Object blkAddr, Bundle *bundle, unsigned char listIdx)
{
	Sdr	bpSdr = getIonsdr();
	int	result;
	Object	elt;
		OBJ_POINTER(ExtensionBlock, blk);

	CHKERR(newBlk);
	CHKERR(bundle);
	if (def == NULL)	/*	Don't care where this goes.	*/
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
		newBlk->rank = getExtensionRank(def);
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

int	patchExtensionBlocks(Bundle *bundle)
{
	ExtensionDef	*extensions;
	int		extensionsCt;
	int		i;
	ExtensionDef	*def;
	ExtensionBlock	blk;

	CHKERR(bundle);
	getExtensionDefs(&extensions, &extensionsCt);
	for (i = 0, def = extensions; i < extensionsCt; i++, def++)
	{
		if (def->type != 0 && def->offer != NULL
		&& findExtensionBlock(bundle, def->type, def->listIdx) == 0)
		{
			/*	This is a type of extension block that
			 *	the local node normally offers, which
			 *	is missing from the received bundle.
			 *	Insert a block of this type if it is
			 *	appropriate.				*/

			memset((char *) &blk, 0, sizeof(ExtensionBlock));
			blk.type = def->type;
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

			if (attachExtensionBlock(def, &blk, bundle) < 0)
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
			def = findExtensionDef(blk.type, i);
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
				deleteExtensionBlock(elt, i);
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
		zco_reduce_heap_occupancy(bpSdr, oldDbOverhead);
		zco_increase_heap_occupancy(bpSdr, bundle->dbOverhead);
	}

	return 0;
}

int  	resizeCollaborationBlock(Bundle *bundle, Object addr, Object data)
{
	/* TODO: Implement. */
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
	unsigned int	offset;
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

		/*	Each reference is a pair of offsets, i.e., two
		 *	list elements.					*/

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
			offset = (unsigned long) lyst_data(elt);
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
			offset = (unsigned long) lyst_data(elt);
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

/******************************************************************************
 *                        ACQUISITION WORK-AREA OPERATIONS                    *
 ******************************************************************************/

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
			putErrmsg("Can't accept extension block.",
					itoa(blkType));
			return -1;
		}

		if (blk->length == 0)	/*	Discarded.		*/
		{
			deleteAcqExtBlock(elt, i);
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

int	checkExtensionBlocks(AcqWorkArea *work)
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
			def = findExtensionDef(blk->type, i);
			if (def == NULL || def->check == NULL)
			{
				continue;
			}

			oldLength = blk->length;
			switch (def->check(blk, work))
			{
			case 3:		/*	Bundle is corrupt.	*/
				bundle->corrupt = 1;
				break;

			case 2:		/*	Bundle is authentic.	*/
				bundle->clDossier.authentic = 1;
				break;

			case 1:		/*	Bundle is inauthentic.	*/
				bundle->clDossier.authentic = 0;
				break;

			case 0:		/*	No additional info.	*/
				break;

			default:
				putErrmsg("Failed checking extension block.",
						def->name);
				return -1;
			}

			if (blk->length == 0)	/*	Discarded.	*/
			{
				bundle->extensionsLength[i] -= oldLength;
				deleteAcqExtBlock(elt, i);
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

void	deleteAcqExtBlock(LystElt elt, unsigned int listIdx)
{
	AcqExtBlock	*blk;
	ExtensionDef	*def;

	CHKVOID(elt);
	blk = (AcqExtBlock *) lyst_data(elt);
	def = findExtensionDef(blk->type, listIdx);
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

void	discardExtensionBlock(AcqExtBlock *blk)
{
	CHKVOID(blk);
	blk->length = 0;
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
	Object		newBlkAddr;
	int		additionalOverhead;

	CHKERR(work);
	bundle->collabBlocks = sdr_list_create(bpSdr);
	CHKERR(bundle->collabBlocks);
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
			newBlk.blkProcFlags = oldBlk->blkProcFlags;
			newBlk.dataLength = oldBlk->dataLength;
			headerLength = oldBlk->length - oldBlk->dataLength;
			if (serializeExtBlk(&newBlk, oldBlk->eidReferences,
				(char *) (oldBlk->bytes + headerLength)) < 0)
			{
				putErrmsg("No space for block.", NULL);
				return -1;
			}

			def = findExtensionDef(oldBlk->type, i);
			if (def && def->record)
			{
				/*	Record extension object.	*/

				def->record(&newBlk, oldBlk);
			}
			else
			{
				newBlk.object = 0;
			}

			newBlkAddr = sdr_malloc(bpSdr, sizeof(ExtensionBlock));
			CHKERR(newBlkAddr);
			if (insertExtensionBlock(def, &newBlk, newBlkAddr,
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
