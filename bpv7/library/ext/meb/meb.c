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
	unsigned char	dataBuffer[11 + BP_MAX_METADATA_LEN];
	unsigned char	*cursor;
	uvast		uvtemp;

	if (bundle->ancillaryData.metadataType == 0)
	{
		return 0;	/*	MEB block is unnecessary.	*/
	}

	if (bundle->ancillaryData.metadataLen > BP_MAX_METADATA_LEN)
	{
		bundle->ancillaryData.metadataLen = BP_MAX_METADATA_LEN;
	}

	cursor = dataBuffer;
	uvtemp = 2;
	oK(cbor_encode_array_open(uvtemp, &cursor));
	uvtemp = bundle->ancillaryData.metadataType;
	oK(cbor_encode_integer(uvtemp, &cursor));
	uvtemp = bundle->ancillaryData.metadataLen;
	oK(cbor_encode_byte_string(bundle->ancillaryData.metadata,
			uvtemp, &cursor));
	blk->dataLength = cursor - dataBuffer;
	blk->size = 0;
	blk->object = 0;
	return serializeExtBlk(blk, (char *) dataBuffer);
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
	unsigned int	unparsedBytes = blk->dataLength;
	uvast		arrayLength;
	uvast		uvtemp;
	unsigned char	metadata[BP_MAX_METADATA_LEN];

	if (unparsedBytes < 3)
	{
		writeMemo("[?] Can't decode MEB block.");
		return 0;		/*	Malformed.		*/
	}

	/*	Data parsed out of the meb byte array go directly
	 *	into the bundle structure, not into a block-specific
	 *	workspace object.					*/

	blk->size = 0;
	blk->object = NULL;
	cursor = blk->bytes + (blk->length - blk->dataLength);
	arrayLength = 3;
	if (cbor_decode_array_open(&arrayLength, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Can't decode MEB block array.");
		return 0;
	}

	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Can't decode MEB metadata type.");
		return 0;
	}

	bundle->ancillaryData.metadataType = uvtemp;
	uvtemp = BP_MAX_METADATA_LEN;
	if (cbor_decode_byte_string(metadata, &uvtemp, &cursor, &unparsedBytes)
			< 1)
	{
		writeMemo("[?] Can't decode MEB metadata type.");
		return 0;
	}

	bundle->ancillaryData.metadataLen = uvtemp;
	memcpy(bundle->ancillaryData.metadata, metadata, uvtemp);
	if (unparsedBytes != 0)
	{
		writeMemo("[?] Excess bytes at end of MEB block.");
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
