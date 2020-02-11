/*
 *	bpq.c:		implementation of the extension definition
 *			functions for the BP Quality of Service block.
 *
 *	Copyright (c) 2019, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 */

#include "bpP.h"
#include "bei.h"
#include "bpq.h"

int	qos_offer(ExtensionBlock *blk, Bundle *bundle)
{
	unsigned char	dataBuffer[40];
	unsigned char	*cursor;
	uvast		uvtemp;

	blk->blkProcFlags = BLK_MUST_BE_COPIED;
	cursor = dataBuffer;
	uvtemp = 4;
	oK(cbor_encode_array_open(uvtemp, &cursor));
	uvtemp = bundle->ancillaryData.flags;
	oK(cbor_encode_integer(uvtemp, &cursor));
	uvtemp = bundle->classOfService;
	oK(cbor_encode_integer(uvtemp, &cursor));
	uvtemp = bundle->ancillaryData.ordinal;
	oK(cbor_encode_integer(uvtemp, &cursor));
	uvtemp = bundle->ancillaryData.dataLabel;
	oK(cbor_encode_integer(uvtemp, &cursor));
	blk->dataLength = cursor - dataBuffer;
	blk->size = 0;
	blk->object = 0;
	return serializeExtBlk(blk, (char *) dataBuffer);
}

void	qos_release(ExtensionBlock *blk)
{
	return;
}

int	qos_record(ExtensionBlock *sdrBlk, AcqExtBlock *ramBlk)
{
	return 0;
}

int	qos_copy(ExtensionBlock *newBlk, ExtensionBlock *oldBlk)
{
	return 0;
}

int	qos_processOnFwd(ExtensionBlock *blk, Bundle *bundle, void *ctxt)
{
	return 0;
}

int	qos_processOnAccept(ExtensionBlock *blk, Bundle *bundle, void *ctxt)
{
	return 0;
}

int	qos_processOnEnqueue(ExtensionBlock *blk, Bundle *bundle, void *ctxt)
{
	return 0;
}

int	qos_processOnDequeue(ExtensionBlock *blk, Bundle *bundle, void *ctxt)
{
	return 0;
}

int	qos_parse(AcqExtBlock *blk, AcqWorkArea *wk)
{
	Bundle		*bundle = &wk->bundle;
	unsigned char	*cursor;
	unsigned int	unparsedBytes = blk->dataLength;
	uvast		arrayLength;
	uvast		uvtemp;

	if (unparsedBytes < 5)
	{
		return 0;		/*	Malformed.		*/
	}

	/*	Data parsed out of the qos byte array go directly
	 *	into the bundle structure, not into a block-specific
	 *	workspace object.					*/

	blk->size = 0;
	blk->object = NULL;
	cursor = blk->bytes + (blk->length - blk->dataLength);
	arrayLength = 4;
	if (cbor_decode_array_open(&arrayLength, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Can't decode QOS block array.");
		return 0;
	}

	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Can't decode QOS flags.");
		return 0;
	}

	bundle->ancillaryData.flags = uvtemp;
	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Can't decode QOS class of service.");
		return 0;
	}

	bundle->classOfService = uvtemp;
	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Can't decode QOS ordinal.");
		return 0;
	}

	bundle->ancillaryData.ordinal = uvtemp;
	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Can't decode QOS data label.");
		return 0;
	}

	bundle->ancillaryData.dataLabel = uvtemp;
	if (unparsedBytes != 0)
	{
		writeMemo("[?] Excess bytes at end of QOS block.");
		return 0;		/*	Malformed.		*/
	}

	return 1;
}

int	qos_check(AcqExtBlock *blk, AcqWorkArea *wk)
{
	return 1;
}

void	qos_clear(AcqExtBlock *blk)
{
	return;
}
