/*
 *	bae.c:		implementation of the extension definition
 *			functions for the Bundle Age Extension block.
 *
 *	Copyright (c) 2012, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 */

#include "bpP.h"
#include "bei.h"
#include "bae.h"

int	bae_offer(ExtensionBlock *blk, Bundle *bundle)
{
	bundle->age = 0;
	blk->blkProcFlags = BLK_MUST_BE_COPIED;
	blk->dataLength = 0;	/*	Will know length at dequeue.	*/
	blk->length = 3;	/*	Just to keep block alive.	*/
	blk->size = 0;
	blk->object = 0;
	return 0;
}

int	bae_serialize(ExtensionBlock *blk, Bundle *bundle)
{
	unsigned char	dataBuffer[32];
	unsigned char	*cursor;
	uvast		uvtemp;

	cursor = dataBuffer;
	uvtemp = bundle->age;
	oK(cbor_encode_integer(uvtemp, &cursor));
	blk->dataLength = cursor - dataBuffer;
	return serializeExtBlk(blk, (char *) dataBuffer);
}

int	bae_processOnDequeue(ExtensionBlock *blk, Bundle *bundle, void *ctxt)
{
	DtnTime	currentDtnTime;	/*	milliseconds past EPOCH 2000.	*/

	/* Parameter intentionally unused. */
	(void)ctxt;

	getCurrentDtnTime(&currentDtnTime);
	if (ionClockIsSynchronized() && bundle->id.creationTime.msec > 0)
	{
		bundle->age = currentDtnTime - bundle->id.creationTime.msec;
	}
	else
	{
		bundle->age += (currentDtnTime - bundle->arrivalTime);
	}

	return bae_serialize(blk, bundle);
}

int	bae_parse(AcqExtBlock *blk, AcqWorkArea *wk)
{
	Bundle		*bundle = &wk->bundle;
	unsigned char	*cursor;
	unsigned int	unparsedBytes = blk->dataLength;
	uvast		uvtemp;

	if (blk->parsed)	/*	E.g., by BSL.			*/
	{
		return 1;
	}

	if (unparsedBytes < 1)
	{
		writeMemo("[?] Can't decode Bundle Age block.");
		return 0;		/*	Malformed.		*/
	}

	/*	Data parsed out of the bae byte array go directly
	 *	into the bundle structure, not into a block-specific
	 *	workspace object.					*/

	blk->size = 0;
	blk->object = NULL;
	cursor = blk->bytes + (blk->length - blk->dataLength);
	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Can't decode bundle age.");
		return 0;		/*	Malformed.		*/
	}

	bundle->age = uvtemp;
	if (unparsedBytes != 0)
	{
		writeMemo("[?] Excess bytes at end of Bundle Age block.");
		return 0;		/*	Malformed.		*/
	}

	blk->parsed = 1;
	return 1;
}

int	bae_check(AcqExtBlock *blk, AcqWorkArea *wk)
{
	/* Parameter intentionally unused. */
	(void)blk;
	(void)wk;

	return 1;
}

int bae_review(AcqWorkArea *wk)
{
	Bundle		*bundle = &(wk->bundle);
	LystElt		elt;
	LystElt		nextElt;
	AcqExtBlock	*blk;
	ExtensionDef	*def;

	CHKERR(wk);
	for (elt = lyst_first(wk->extBlocks); elt; elt = nextElt)
	{
		nextElt = lyst_next(elt);
		blk = (AcqExtBlock *) lyst_data(elt);

		/*	Now do block-specific parsing.			*/

		def = findExtensionDef(blk->type);
		if (def == NULL || def->parse == NULL)
		{
			continue;
		}

		if (def->type == BundleAgeBlk)
		{
			return 1;
		}

	}

	// Don't have BAE, but creation time is defined
	if (bundle->id.creationTime.msec > 0)
	{
		return 1;
	}

	writeMemo("[?] No bundle age block or creation time set.");
	return 0;
}
