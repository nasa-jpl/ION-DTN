/*
 *	snid.c:		implementation of the extension definition
 *			functions for the Bundle Age Extension block.
 *
 *	Copyright (c) 2014, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 */

#include "bpP.h"
#include "bei.h"
#include "snid.h"

int	snid_offer(ExtensionBlock *blk, Bundle *bundle)
{
	Sdnv	nodeNbrSdnv;
	char	dataBuffer[32];

	blk->blkProcFlags = BLK_MUST_BE_COPIED | BLK_REMOVE_IF_NG;
	encodeSdnv(&nodeNbrSdnv, getOwnNodeNbr());
	blk->dataLength = nodeNbrSdnv.length;
	blk->size = 0;
	blk->object = 0;
	memcpy(dataBuffer, nodeNbrSdnv.text, nodeNbrSdnv.length);
	return serializeExtBlk(blk, NULL, dataBuffer);
}

void	snid_release(ExtensionBlock *blk)
{
	return;
}

int	snid_record(ExtensionBlock *sdrBlk, AcqExtBlock *ramBlk)
{
	return 0;
}

int	snid_copy(ExtensionBlock *newBlk, ExtensionBlock *oldBlk)
{
	return 0;
}

int	snid_processOnFwd(ExtensionBlock *blk, Bundle *bundle, void *ctxt)
{
	return 0;
}

int	snid_processOnAccept(ExtensionBlock *blk, Bundle *bundle, void *ctxt)
{
	return 0;
}

int	snid_processOnEnqueue(ExtensionBlock *blk, Bundle *bundle, void *ctxt)
{
	return 0;
}

int	snid_processOnDequeue(ExtensionBlock *blk, Bundle *bundle, void *ctxt)
{
	return 0;
}

int	snid_acquire(AcqExtBlock *blk, AcqWorkArea *wk)
{
	uvast		senderNodeNbr;
	unsigned char	*cursor;
	int		bytesRemaining;
	char		uriBuffer[32];
	int		eidLen;

	if (blk->dataLength < 1)
	{
		return 0;		/*	Malformed.		*/
	}

	blk->size = 0;
	blk->object = NULL;

	/*	Sender ID from SNID block doesn't override sender ID
	 *	determined from other sources, such as PHN block.	*/

	if (wk->senderEid)		/*	Provided another way.	*/
	{
		return 1;		/*	Ignore SNID block.	*/
	}

	cursor = blk->bytes + (blk->length - blk->dataLength);
	bytesRemaining = blk->dataLength;
	extractSdnv(&senderNodeNbr, &cursor, &bytesRemaining);
	if (bytesRemaining != 0)
	{
		return 0;		/*	Malformed.		*/
	}

	eidLen = _isprintf(uriBuffer, sizeof uriBuffer,
			"ipn:" UVAST_FIELDSPEC ".0", senderNodeNbr) + 1;
	wk->senderEid = MTAKE(eidLen);
	if (wk->senderEid == NULL)
	{
		putErrmsg("Can't record sender EID.", NULL);
		return -1;
	}

	istrcpy(wk->senderEid, uriBuffer, eidLen);
	return 1;
}

int	snid_check(AcqExtBlock *blk, AcqWorkArea *wk)
{
	discardExtensionBlock(blk);
	return 0;
}

void	snid_clear(AcqExtBlock *blk)
{
	return;
}
