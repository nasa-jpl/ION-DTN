/*
 *	meb.c:		implementation of the extension definition
 *			functions for the Metadata extension block.
 *
 *	Copyright (c) 2014, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 */

#include "bpP.h"
#include "bei.h"
#include "meb.h"

int	meb_offer(ExtensionBlock *blk, Bundle *bundle)
{
	Sdnv	lengthSdnv;
	char	dataBuffer[3 + BP_MAX_METADATA_LEN];

	if (bundle->ancillaryData.metadataType == 0)
	{
		return 0;	/*	ECOS block is unnecessary.	*/
	}

	if (bundle->ancillaryData.metadataLen > BP_MAX_METADATA_LEN)
	{
		bundle->ancillaryData.metadataLen = BP_MAX_METADATA_LEN;
	}

	blk->dataLength = 1;	/*	For metadata type byte.		*/
	encodeSdnv(&lengthSdnv, bundle->ancillaryData.metadataLen);
	blk->dataLength += lengthSdnv.length;

	/*	Note that lengthSdnv.length can't exceed 2 because
	 *	bundle->ancillaryData.metadataLen is an unsigned char
	 *	and therefore has a maximum value of 255, which can
	 *	always be encoded in a 2-byte SDNV.			*/

	blk->dataLength += bundle->ancillaryData.metadataLen;
	dataBuffer[0] = bundle->ancillaryData.metadataType;
	memcpy(dataBuffer + 1, lengthSdnv.text, lengthSdnv.length);
	memcpy(dataBuffer + 1 + lengthSdnv.length, bundle->ancillaryData.metadata,
			bundle->ancillaryData.metadataLen);
	return serializeExtBlk(blk, NULL, dataBuffer);
}

void	meb_release(ExtensionBlock *blk)
{
	return;
}

int	meb_record(ExtensionBlock *sdrBlk, AcqExtBlock *ramBlk)
{
	return 0;
}

int	meb_copy(ExtensionBlock *newBlk, ExtensionBlock *oldBlk)
{
	return 0;
}

int	meb_processOnFwd(ExtensionBlock *blk, Bundle *bundle, void *ctxt)
{
	return 0;
}

int	meb_processOnAccept(ExtensionBlock *blk, Bundle *bundle, void *ctxt)
{
	return 0;
}

int	meb_processOnEnqueue(ExtensionBlock *blk, Bundle *bundle, void *ctxt)
{
	return 0;
}

int	meb_processOnDequeue(ExtensionBlock *blk, Bundle *bundle, void *ctxt)
{
	return 0;
}

int	meb_acquire(AcqExtBlock *blk, AcqWorkArea *wk)
{
	Bundle		*bundle = &wk->bundle;
	unsigned char	*cursor;
	int		bytesRemaining = blk->dataLength;
	uvast		metadataLength;

	if (bytesRemaining < 2)
	{
		return 0;		/*	Malformed.		*/
	}

	/*	Data parsed out of the meb byte array go directly
	 *	into the bundle structure, not into a block-specific
	 *	workspace object.					*/

	blk->size = 0;
	blk->object = NULL;
	cursor = blk->bytes + (blk->length - blk->dataLength);
	bundle->ancillaryData.metadataType = *cursor;
	cursor++;
	bytesRemaining--;
	extractSdnv(&metadataLength, &cursor, &bytesRemaining);
	if (metadataLength > BP_MAX_METADATA_LEN
	|| metadataLength > 255)	/*	Max for unsigned char.	*/
	{
		return 0;		/*	Malformed.		*/
	}

	bundle->ancillaryData.metadataLen = metadataLength;
	memcpy(bundle->ancillaryData.metadata, cursor, metadataLength);
	cursor += metadataLength;
	bytesRemaining -= metadataLength;
	if (bytesRemaining != 0)
	{
		return 0;		/*	Malformed.		*/
	}

	return 1;
}

int	meb_check(AcqExtBlock *blk, AcqWorkArea *wk)
{
	return 1;
}

void	meb_clear(AcqExtBlock *blk)
{
	return;
}
