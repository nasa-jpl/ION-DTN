/******************************************************************************
 	bei.c:	Implementation of functions that manipulate extension blocks
 	        within bundles and acquisition structures.

	Author: Scott Burleigh, JPL

	Modification History:
	Date      Who   What

	Copyright (c) 2010, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
 *******************************************************************************/

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

// TODO: This assumes the extension block is coming from a bundle and not
//       from an acquisition structure. Can we come up with some terminology to
//       identify extension blocks in bundles versus ext blks in acquisition
//       structures?


/******************************************************************************
 *                               BUNDLE OPERATIONS                            *
 ******************************************************************************/

/**
 * Adds a collaboration block with the current identifier to the list of
 * collaboration blocks for the current bundle.
 */
int 	addCollaborationBlock(Bundle *bundle,
		Object data)
{
	/* First, make sure we don't already have a collaboration block with
	 * this identifier in the bundle...
	 */
	Object newBlkAddr;
	CollabBlockHdr *blk_hdr = (CollabBlockHdr *) data;

	Object addr = findCollaborationBlock(bundle, blk_hdr->type, blk_hdr->id);

	Sdr bpSdr = getIonsdr();

	if(addr != 0)
	{
		putErrmsg("Collab block already exists in bundle.", NULL);
		return -1;
	}

	/*
	 * Otherwise, we are free to add this collaboration block to the bundle.
	 * Add to the end of the list of collab blocks. Order does not matter
	 * for collaboration blocks.
	 */

	newBlkAddr = sdr_malloc(bpSdr, blk_hdr->size);
	if(newBlkAddr == 0)
	{
		putErrmsg("Failed copying collab.", NULL);
		return -1;
	}

	/* Insert the new block. */
	if(sdr_list_insert_last(bpSdr,bundle->collabBlocks,newBlkAddr) == 0)
	{
		putErrmsg("Failed inserting collab block.", NULL);
		return -1;
	}


	/* Create the stored collab block and write it to the SDR */
	sdr_write(bpSdr, newBlkAddr, (char *) data, blk_hdr->size);
	return 0;
}

int		attachExtensionBlock(ExtensionDef *def, ExtensionBlock *blk,
		Bundle *bundle)
{
	Object	blkAddr;
	int	additionalOverhead;

	blk->type = def->type;
	blkAddr = sdr_malloc(getIonsdr(), sizeof(ExtensionBlock));
	if (blkAddr == 0
			|| insertExtensionBlock(def, blk, blkAddr, bundle, def->listIdx) < 0)
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
	OBJ_POINTER(CollabBlockHdr, blkHdr);
	char 		*dataPtr;
	Object		blkAddr;
	Object		elt;
	Sdr bpSdr = getIonsdr();

	/* Destroy collaboration blocks that might be in the new bundle. */
	destroyCollaborationBlocks(newBundle);

	/* Copy the collaboration blocks. */
	newBundle->collabLength = 0;
	newBundle->collabBlocks = sdr_list_create(bpSdr);
	if(newBundle->collabBlocks == 0)
	{
		putErrmsg("No space for collaboration blocks", NULL);
		return -1;
	}

	/* For each collaboration block that we need to copy... */
	for (elt = sdr_list_first(bpSdr, oldBundle->collabBlocks); elt;
			elt = sdr_list_next(bpSdr, elt))
	{

		/* Grab the collab block header to figure out the size */
		blkAddr = sdr_list_data(bpSdr, elt);
		GET_OBJ_POINTER(bpSdr, CollabBlockHdr, blkHdr, blkAddr);
		if(blkHdr == NULL)
		{
			putErrmsg("Unable to retrieve block header.", NULL);
			return -1;
		}

		dataPtr = MTAKE(blkHdr->size);
		if(dataPtr == NULL)
		{
			putErrmsg("Unable to copy collab block", NULL);
			return -1;
		}

		sdr_read(bpSdr, dataPtr, blkAddr, blkHdr->size);

		addCollaborationBlock(newBundle, (Object) dataPtr);

		MRELEASE(dataPtr);
		dataPtr = NULL;
		newBundle->collabLength++;
	}

	return 0;
}

int		copyExtensionBlocks(Bundle *newBundle, Bundle *oldBundle)
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

	copyCollaborationBlocks(newBundle, oldBundle);

	for (i = 0; i < 2; i++)
	{
		newBundle->extensions[i] = sdr_list_create(bpSdr);
		if (newBundle->extensions[i] == 0)
		{
			putErrmsg("No space for extensions list.", NULL);
			return -1;
		}

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
				if (newBlk.eidReferences == 0)
				{
					putErrmsg("No space for EID refs list.",
							NULL);
					return -1;
				}

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
			newBlk.bytes = sdr_malloc(bpSdr, newBlk.length);
			if (newBlk.bytes == 0)
			{
				putErrmsg("No space for block.",
						utoa(newBlk.length));
				return -1;
			}

			sdr_read(bpSdr, buf, oldBlk->bytes, buflen);
			sdr_write(bpSdr, newBlk.bytes, buf, buflen);
			def = findExtensionDef(oldBlk->type, i);
			if (def && def->copy)
			{
				/*	Must copy extension object.	*/

				def->copy(&newBlk, oldBlk);
			}
			else
			{
				newBlk.object = 0;
				newBlk.size = 0;
			}

			newBlkAddr = sdr_malloc(bpSdr, sizeof(ExtensionBlock));
			if (newBlkAddr == 0
					|| insertExtensionBlock(def, &newBlk, newBlkAddr,
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
	Object blkAddr;
	Sdr bpSdr = getIonsdr();

	if(bundle->collabBlocks == 0)
	{
		bundle->collabLength = 0;
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
	bundle->collabLength = 0;
}

void	destroyExtensionBlocks(Bundle *bundle)
{
	Sdr	bpSdr = getIonsdr();
	int	i;
	Object	elt;

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

Object  findCollaborationBlock(Bundle *bundle,
		unsigned char type,
		unsigned int id)
{
	Object	elt;
	Object	addr;
	OBJ_POINTER(CollabBlockHdr, blkHdr);
	Sdr bpSdr = getIonsdr();

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


int		insertExtensionBlock(ExtensionDef *def, ExtensionBlock *newBlk,
		Object blkAddr, Bundle *bundle, unsigned char listIdx)
{
	Sdr	bpSdr = getIonsdr();
	int	result;
	Object	elt;
	OBJ_POINTER(ExtensionBlock, blk);

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
		newBlk->rank =
				((unsigned long) def - (unsigned long) extensions)
				/ sizeof(ExtensionDef);
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

int		patchExtensionBlocks(Bundle *bundle)
{
	int		i;
	ExtensionDef	*def;
	ExtensionBlock	blk;

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

int		processExtensionBlocks(Bundle *bundle, int fnIdx, void *context)
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
		noteBundleRemoved(bundle);
		bundle->dbTotal -= oldDbOverhead;
		bundle->dbTotal += bundle->dbOverhead;
		noteBundleInserted(bundle);
	}

	return 0;
}

int  	resizeCollaborationBlock(Bundle *bundle, Object addr, Object data)
{
	// TODO: Implement.
	return 0;
}

void	restoreExtensionBlock(ExtensionBlock *blk)
{
	blk->suppressed = 0;
}

void	scratchExtensionBlock(ExtensionBlock *blk)
{
	blk->length = 0;
}

int		serializeExtBlk(ExtensionBlock *blk, Lyst eidReferences, char *blockData)
{
	Sdr		bpSdr = getIonsdr();
	unsigned long	blkProcFlags = blk->blkProcFlags;
	Sdnv		blkProcFlagsSdnv;
	unsigned long	dataLength = blk->dataLength;
	Sdnv		dataLengthSdnv;
	int		listLength;
	LystElt		elt;
	unsigned long	offset;
	Sdnv		offsetSdnv;
	unsigned long	referenceCount;
	Sdnv		referenceCountSdnv;
	Object		newElt;
	char		*blkBuffer;
	char		*cursor;

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
		if (blk->eidReferences == 0)
		{
			putErrmsg("No space for EID references list.", NULL);
			return -1;
		}

		for (elt = lyst_first(eidReferences); elt; elt = lyst_next(elt))
		{
			offset = (unsigned long) lyst_data(elt);
			encodeSdnv(&offsetSdnv, offset);
			blk->length += offsetSdnv.length;
			newElt = sdr_list_insert_last(bpSdr, blk->eidReferences, offset);
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
	blk->suppressed = 1;
}

int 	updateCollaborationBlock(Object collabAddr,
		Object data)
{
	CollabBlockHdr *blkHdr = (CollabBlockHdr *) data;
	Sdr bpSdr = getIonsdr();

	if((collabAddr == 0) || (blkHdr == NULL))
	{
		putErrmsg("Bad information given to update collab block.", NULL);
		return -1;
	}

	// TODO: Can we do a size check here to make sure size didn't change?
	sdr_write(bpSdr, collabAddr, (char *) data, blkHdr->size);

	return 0;
}

/******************************************************************************
 *                        ACQUISITION/WORK-AREA OPERATIONS                    *
 ******************************************************************************/


int		acquireExtensionBlock(AcqWorkArea *work,
		ExtensionDef *def,
		unsigned char *startOfBlock,
		unsigned long blockLength,
		unsigned char blkType,
		unsigned long blkProcFlags,
		Lyst *eidReferences,
		unsigned long dataLength,
		unsigned char *cursor)
{
	Bundle		*bundle = &(work->bundle);
	int		blkSize;
	AcqExtBlock	*blk;
	Sdnv		blkProcFlagsSdnv;
	int		i;
	LystElt		elt;
	int		additionalOverhead;

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

int 	addAcqCollabBlock(AcqWorkArea *work,
		void *data)
{

	CollabBlockHdr *blkHdr = (CollabBlockHdr *) data;
	LystElt		elt;
	void *dataPtr;

	elt = findAcqCollabBlock(work, blkHdr->type, blkHdr->id);

	if(elt != NULL)
	{
		putErrmsg("Collab block already exists in work area.", NULL);
		return -1;
	}

	dataPtr = MTAKE(blkHdr->size);
	if (dataPtr == NULL)
	{
		putErrmsg("Can't acquire collaboration block.", itoa(blkHdr->size));
		return -1;
	}

	/*	Populate the collaboration block structure.			*/
	memcpy((char *) dataPtr, data, blkHdr->size);

	/*	Store extension block within bundle.			*/
	elt = lyst_insert_last(work->collabBlocks, dataPtr);
	if (elt == NULL)
	{
		MRELEASE(dataPtr);
		putErrmsg("Can't acquire collaboration block.", NULL);
		char msg[50]; sprintf(msg,"size is %d", blkHdr->size);
		putErrmsg(msg,NULL);
		return -1;
	}

	return 0;
}

int		checkExtensionBlocks(AcqWorkArea *work)
{
	Bundle		*bundle = &(work->bundle);
	int		i;
	LystElt		elt;
	LystElt		nextElt;
	AcqExtBlock	*blk;
	ExtensionDef	*def;
	unsigned long	oldLength;

	initAuthenticity(work);		/*	Set default.		*/
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

void	deleteAcqExtBlock(LystElt elt,
		unsigned int listIdx)
{
	AcqExtBlock	*blk;
	ExtensionDef	*def;

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
	LystElt elt;
	void *dataPtr = NULL;

	while(1)
	{
		elt = lyst_first(work->collabBlocks);
		if (elt == NULL)
		{
			break;
		}

		dataPtr = lyst_data(elt);

		if(dataPtr != NULL)
		{
			MRELEASE(dataPtr);
			dataPtr = NULL;
		}

		lyst_delete(elt);
	}
}

void	discardExtensionBlock(AcqExtBlock *blk)
{
	blk->length = 0;
}

LystElt findAcqCollabBlock(AcqWorkArea *work,
		unsigned char type,
		unsigned int id)
{
	LystElt elt;
	CollabBlockHdr *blkHdr = NULL;

	for (elt = lyst_first(work->collabBlocks); elt; elt = lyst_next(elt))
	{
		blkHdr = (CollabBlockHdr*) lyst_data(elt);

		if((blkHdr->type == type) && (blkHdr->id == id))
		{
			break;
		}
	}

	return elt;
}

int		recordExtensionBlocks(AcqWorkArea *work)
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

	//EJB
	bundle->collabBlocks = sdr_list_create(bpSdr);
	if(bundle->collabBlocks == 0)
	{
		putErrmsg("No space for collaboration blocks.", NULL);
		return -1;
	}

	for (i = 0; i < 2; i++)
	{
		bundle->extensions[i] = sdr_list_create(bpSdr);
		if (bundle->extensions[i] == 0)
		{
			putErrmsg("No space for extensions list.", NULL);
			return -1;
		}

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
			if (newBlkAddr == 0
					|| insertExtensionBlock(def, &newBlk, newBlkAddr,
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



/******************************************************************************
 *                               GENERAL UTILITIES                            *
 ******************************************************************************/


ExtensionDef	*findExtensionDef(unsigned char type, unsigned char idx)
{
	int		i;
	ExtensionDef	*def;

	if (type == 0) return NULL;
	for (i = 0, def = extensions; i < extensionsCt; i++, def++)
	{
		if (def->type == type && def->listIdx == idx)
		{
			return def;
		}
	}

	return NULL;
}



