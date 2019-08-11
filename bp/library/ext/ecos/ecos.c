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
	Sdnv	dataLabelSdnv;
	char	dataBuffer[32];

	if (bundle->ancillaryData.flags == 0 && bundle->ancillaryData.ordinal
			== 0)
	{
		return 0;	/*	ECOS block is unnecessary.	*/
	}

	blk->blkProcFlags = BLK_MUST_BE_COPIED;
	blk->dataLength = 2;
	if (bundle->ancillaryData.flags & BP_DATA_LABEL_PRESENT)
	{
		encodeSdnv(&dataLabelSdnv, bundle->ancillaryData.dataLabel);
	}
	else
	{
		dataLabelSdnv.length = 0;
	}

	blk->dataLength += dataLabelSdnv.length;
	blk->size = 0;
	blk->object = 0;
	dataBuffer[0] = bundle->ancillaryData.flags;
	dataBuffer[1] = bundle->ancillaryData.ordinal;
	memcpy(dataBuffer + 2, dataLabelSdnv.text, dataLabelSdnv.length);
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

int	ecos_parse(AcqExtBlock *blk, AcqWorkArea *wk)
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
	bundle->ancillaryData.flags = *cursor;
	cursor++;
	bundle->ancillaryData.ordinal = *cursor;
	bundle->ordinal = *cursor;	/*	Default.		*/
	cursor++;
	bytesRemaining -= 2;
	if (bundle->ancillaryData.flags & BP_DATA_LABEL_PRESENT)
	{
		extractSmallSdnv(&(bundle->ancillaryData.dataLabel), &cursor,
				&bytesRemaining);
	}
	else
	{
		bundle->ancillaryData.dataLabel = 0;
	}

	if (bytesRemaining != 0)
	{
		return 0;		/*	Malformed.		*/
	}

	return 1;
}

int	ecos_check(AcqExtBlock *blk, AcqWorkArea *wk)
{
	return 1;
}

void	ecos_clear(AcqExtBlock *blk)
{
	return;
}
