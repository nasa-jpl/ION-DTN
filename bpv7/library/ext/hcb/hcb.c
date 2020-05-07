/*
 *	hcb.c:		implementation of the extension definition
 *			functions for the Hop Count extension block.
 *
 *	Copyright (c) 2019, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 */

#include "bpP.h"
#include "bei.h"
#include "hcb.h"

#ifndef	ION_HOP_LIMIT
#define	ION_HOP_LIMIT	15
#endif

int	hcb_offer(ExtensionBlock *blk, Bundle *bundle)
{
	unsigned char	dataBuffer[24];
	unsigned char	*cursor;
	uvast		uvtemp;

	bundle->hopCount = 0;
	bundle->hopLimit = ION_HOP_LIMIT;
	blk->blkProcFlags = BLK_MUST_BE_COPIED;
	cursor = dataBuffer;
	uvtemp = 2;
	oK(cbor_encode_array_open(uvtemp, &cursor));
	uvtemp = bundle->hopLimit;
	oK(cbor_encode_integer(uvtemp, &cursor));
	uvtemp = bundle->hopCount;
	oK(cbor_encode_integer(uvtemp, &cursor));
	blk->dataLength = cursor - dataBuffer;
	blk->size = 0;
	blk->object = 0;
	return serializeExtBlk(blk, (char *) dataBuffer);
}

void	hcb_release(ExtensionBlock *blk)
{
	return;
}

int	hcb_record(ExtensionBlock *sdrBlk, AcqExtBlock *ramBlk)
{
	return 0;
}

int	hcb_copy(ExtensionBlock *newBlk, ExtensionBlock *oldBlk)
{
	return 0;
}

int	hcb_processOnFwd(ExtensionBlock *blk, Bundle *bundle, void *ctxt)
{
	return 0;
}

int	hcb_processOnAccept(ExtensionBlock *blk, Bundle *bundle, void *ctxt)
{
	return 0;
}

int	hcb_processOnEnqueue(ExtensionBlock *blk, Bundle *bundle, void *ctxt)
{
	return 0;
}

int	hcb_processOnDequeue(ExtensionBlock *blk, Bundle *bundle, void *ctxt)
{
	unsigned char	dataBuffer[24];
	unsigned char	*cursor;
	uvast		uvtemp;

	bundle->hopCount += 1;
	if (bundle->hopCount > bundle->hopLimit)
	{
		bundle->corrupt = 1;	/*	Hop limit exceeded.	*/
		return 0;
	}

	cursor = dataBuffer;
	uvtemp = 2;
	oK(cbor_encode_array_open(uvtemp, &cursor));
	uvtemp = bundle->hopLimit;
	oK(cbor_encode_integer(uvtemp, &cursor));
	uvtemp = bundle->hopCount;
	oK(cbor_encode_integer(uvtemp, &cursor));
	blk->dataLength = cursor - dataBuffer;
	blk->size = 0;
	blk->object = 0;
	return serializeExtBlk(blk, (char *) dataBuffer);
}

int	hcb_parse(AcqExtBlock *blk, AcqWorkArea *wk)
{
	Bundle		*bundle = &wk->bundle;
	unsigned char	*cursor;
	unsigned int	unparsedBytes = blk->dataLength;
	uvast		arrayLength;
	uvast		uvtemp;

	if (unparsedBytes < 1)
	{
		writeMemo("[?] Can't decode Hop Count block.");
		return 0;		/*	Malformed.		*/
	}

	/*	Data parsed out of the hcb byte array go directly
	 *	into the bundle structure, not into a block-specific
	 *	workspace object.					*/

	blk->size = 0;
	blk->object = NULL;
	cursor = blk->bytes + (blk->length - blk->dataLength);
	arrayLength = 2;
	if (cbor_decode_array_open(&arrayLength, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Can't decode Hop Count block array.");
		return 0;
	}

	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Can't decode hop limit.");
	}

	bundle->hopLimit = uvtemp;
	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Can't decode hop count.");
	}

	bundle->hopCount = uvtemp;
	if (unparsedBytes != 0)
	{
		writeMemo("[?] Excess bytes at end of Hop Count block.");
		return 0;		/*	Malformed.		*/
	}

	getCurrentTime(&(bundle->arrivalTime));
	return 1;
}

int	hcb_check(AcqExtBlock *blk, AcqWorkArea *wk)
{
	return 1;
}

void	hcb_clear(AcqExtBlock *blk)
{
	return;
}
