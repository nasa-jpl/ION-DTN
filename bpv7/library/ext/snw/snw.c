/*
 *	snw.c:		implementation of the extension definition
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
#include "snw.h"

int	snw_offer(ExtensionBlock *blk, Bundle *bundle)
{
	return 0;
}

void	snw_release(ExtensionBlock *blk)
{
	return;
}

int	snw_record(ExtensionBlock *sdrBlk, AcqExtBlock *ramBlk)
{
	return 0;
}

int	snw_copy(ExtensionBlock *newBlk, ExtensionBlock *oldBlk)
{
	return 0;
}

int	snw_processOnFwd(ExtensionBlock *blk, Bundle *bundle, void *ctxt)
{
	return 0;
}

int	snw_processOnAccept(ExtensionBlock *blk, Bundle *bundle, void *ctxt)
{
	return 0;
}

int	snw_processOnEnqueue(ExtensionBlock *blk, Bundle *bundle, void *ctxt)
{
	return 0;
}

int	snw_processOnDequeue(ExtensionBlock *blk, Bundle *bundle, void *ctxt)
{
	uvast		permits;
	unsigned char	dataBuffer[9];
	unsigned char	*cursor;
	uvast		uvtemp;

	if (bundle->permits == 0)
	{
		return 0;	/*	SNW block is unnecessary.	*/
	}

	blk->blkProcFlags = BLK_MUST_BE_COPIED;
	blk->size = 0;
	blk->object = 0;
	if (bundle->permits == 1)
	{
		permits = 1;
	}
	else
	{
		permits = (bundle->permits >> 1) & 0x7f;
		bundle->permits -= permits;
	}

	cursor = dataBuffer;
	uvtemp = bundle->permits;
	oK(cbor_encode_integer(uvtemp, &cursor));
	blk->dataLength = cursor - dataBuffer;
	return serializeExtBlk(blk, (char *) dataBuffer);
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

void	snw_clear(AcqExtBlock *blk)
{
	return;
}
