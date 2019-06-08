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
	unsigned char	dataBuffer[1];

	if (bundle->permits == 0)
	{
		return 0;	/*	SNW block is unnecessary.	*/
	}

	blk->blkProcFlags = BLK_MUST_BE_COPIED;
	blk->dataLength = 1;
	blk->size = 0;
	blk->object = 0;
	if (bundle->permits == 1)
	{
		dataBuffer[0] = 1;
	}
	else
	{
		dataBuffer[0] = (bundle->permits >> 1) & 0x7f;
		bundle->permits -= dataBuffer[0];
	}

	return serializeExtBlk(blk, NULL, (char *) dataBuffer);
}

int	snw_parse(AcqExtBlock *blk, AcqWorkArea *wk)
{
	Bundle		*bundle = &wk->bundle;
	unsigned char	*cursor;
	int		bytesRemaining = blk->dataLength;

	if (bytesRemaining < 1)
	{
		return 0;		/*	Malformed.		*/
	}

	blk->size = 0;
	blk->object = NULL;
	cursor = blk->bytes + (blk->length - blk->dataLength);
	bundle->permits = *cursor;
	cursor++;
	bytesRemaining -= 1;
	blk->length = 0;		/*	No need to retain.	*/
	if (bytesRemaining != 0)
	{
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
