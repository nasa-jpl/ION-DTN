/*
 *	ecos.c:		implementation of the extension definition
 *			functions for the Extended Class of Service
 *			block.
 *
 *	Copyright (c) 2008, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 */

#include "bpP.h"
#include "bei.h"
#include "ecos.h"

int	ecos_offer(ExtensionBlock *blk, Bundle *bundle)
{
	Sdnv	flowLabelSdnv;
	char	dataBuffer[32];

	if (bundle->extendedCOS.flags == 0 && bundle->extendedCOS.ordinal == 0)
	{
		return 0;	/*	ECOS block is unnecessary.	*/
	}

	blk->blkProcFlags = BLK_MUST_BE_COPIED;
	blk->dataLength = 2;
	if (bundle->extendedCOS.flags & BP_FLOW_LABEL_PRESENT)
	{
		encodeSdnv(&flowLabelSdnv, bundle->extendedCOS.flowLabel);
	}
	else
	{
		flowLabelSdnv.length = 0;
	}

	blk->dataLength += flowLabelSdnv.length;
	blk->size = 0;
	blk->object = 0;
	dataBuffer[0] = bundle->extendedCOS.flags;
	dataBuffer[1] = bundle->extendedCOS.ordinal;
	memcpy(dataBuffer + 2, flowLabelSdnv.text, flowLabelSdnv.length);
	return serializeExtBlk(blk, NULL, dataBuffer);
}

void	ecos_release(ExtensionBlock *blk)
{
	return;
}

int	ecos_record(ExtensionBlock *sdrBlk, AcqExtBlock *ramBlk)
{
	return 0;
}

int	ecos_copy(ExtensionBlock *newBlk, ExtensionBlock *oldBlk)
{
	return 0;
}

int	ecos_processOnFwd(ExtensionBlock *blk, Bundle *bundle, void *ctxt)
{
	return 0;
}

int	ecos_processOnAccept(ExtensionBlock *blk, Bundle *bundle, void *ctxt)
{
	return 0;
}

int	ecos_processOnEnqueue(ExtensionBlock *blk, Bundle *bundle, void *ctxt)
{
	return 0;
}

int	ecos_processOnDequeue(ExtensionBlock *blk, Bundle *bundle, void *ctxt)
{
	return 0;
}

int	ecos_acquire(AcqExtBlock *blk, AcqWorkArea *wk)
{
	Bundle		*bundle = &wk->bundle;
	unsigned char	*cursor;
	int		bytesRemaining = blk->dataLength;

	if (bytesRemaining < 2)
	{
		return 0;		/*	Malformed.		*/
	}

	/*	Data parsed out of the ecos byte array go directly
	 *	into the bundle structure, not into a block-specific
	 *	workspace object.					*/

	blk->size = 0;
	blk->object = NULL;
	cursor = blk->bytes + (blk->length - blk->dataLength);
	bundle->extendedCOS.flags = *cursor;
	cursor++;
	bundle->extendedCOS.ordinal = *cursor;
	cursor++;
	bytesRemaining -= 2;
	if (bundle->extendedCOS.flags & BP_FLOW_LABEL_PRESENT)
	{
		extractSmallSdnv(&(bundle->extendedCOS.flowLabel), &cursor,
				&bytesRemaining);
	}
	else
	{
		bundle->extendedCOS.flowLabel = 0;
	}

	if (bytesRemaining != 0)
	{
		return 0;		/*	Malformed.		*/
	}

	return 1;
}

int	ecos_check(AcqExtBlock *blk, AcqWorkArea *wk)
{
	return 0;
}

void	ecos_clear(AcqExtBlock *blk)
{
	return;
}
