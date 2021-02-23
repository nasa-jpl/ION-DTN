/*
 *	snw.c:		implementation of the extension definition
 *			functions for the Spray and Wait block.
 *
 *	Copyright (c) 2019, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 */

#include "bpP.h"
#include "bei.h"
#include "snw.h"

int	snw_offer(ExtensionBlock *blk, Bundle *bundle)
{
	/*	Block must be offered as a placeholder to enable
	 *	later extension block processing.			*/

	blk->blkProcFlags = BLK_MUST_BE_COPIED;
	blk->dataLength = 0;	/*	Will know length at dequeue.	*/
	blk->length = 0;
	blk->size = 1;		/*	Just to keep block alive.	*/
	blk->object = 0;
	return 0;
}

int	snw_serialize(ExtensionBlock *blk, Bundle *bundle)
{
	unsigned char	dataBuffer[9];
	unsigned char	*cursor;
	uvast		uvtemp;

	cursor = dataBuffer;
	uvtemp = bundle->permits;
	oK(cbor_encode_integer(uvtemp, &cursor));
	blk->dataLength = cursor - dataBuffer;
	return serializeExtBlk(blk, (char *) dataBuffer);
}

int	snw_processOnDequeue(ExtensionBlock *blk, Bundle *bundle, void *ctxt)
{
	uvast	permits;

	if (bundle->permits == 0)	/*	SNW block unnecessary.	*/
	{
		return 0;
	}

	if (bundle->permits > 1)
	{
		permits = (bundle->permits >> 1) & 0x7f;
		bundle->permits -= permits;
	}

	return snw_serialize(blk, bundle);
}

int	snw_parse(AcqExtBlock *blk, AcqWorkArea *wk)
{
	Bundle		*bundle = &wk->bundle;
	unsigned char	*cursor;
	unsigned int	unparsedBytes = blk->dataLength;
	uvast		uvtemp;

	if (unparsedBytes < 1)
	{
		writeMemo("[?] Can't decode Spray & Wait block.");
		return 0;		/*	Malformed.		*/
	}

	blk->size = 0;
	blk->object = NULL;
	cursor = blk->bytes + (blk->length - blk->dataLength);
	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Can't decode count of SNW permits.");
		return 0;
	}

	bundle->permits = uvtemp;
	blk->length = 0;		/*	No need to retain.	*/
	if (unparsedBytes != 0)
	{
		writeMemo("[?] Excess bytes at end of Spray & Wait block.");
		return 0;		/*	Malformed.		*/
	}

	return 1;
}

int	snw_check(AcqExtBlock *blk, AcqWorkArea *wk)
{
	return 1;
}
