/*
 *	qos.c:		implementation of the extension definition
 *			functions for the Quality of Service block.
 *
 *	Copyright (c) 2019, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 */

#include "bpP.h"
#include "bei.h"
#include "qos.h"

int	qos_offer(ExtensionBlock *blk, Bundle *bundle)
{
	Sdnv	dataLabelSdnv;
	char	dataBuffer[32];

	if (bundle->ancillaryData.flags == 0
	&& bundle->ancillaryData.ordinal == 0
	&& bundle->priority == 0)
	{
		return 0;	/*	QOS block is unnecessary.	*/
	}

	blk->blkProcFlags = BLK_MUST_BE_COPIED;
	blk->dataLength = 3;
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
	dataBuffer[1] = bundle->priority;
	dataBuffer[2] = bundle->ancillaryData.ordinal;
	memcpy(dataBuffer + 3, dataLabelSdnv.text, dataLabelSdnv.length);
	return serializeExtBlk(blk, dataBuffer);
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
	int		bytesRemaining = blk->dataLength;

	if (bytesRemaining < 3)
	{
		return 0;		/*	Malformed.		*/
	}

	/*	Data parsed out of the qos byte array go directly
	 *	into the bundle structure, not into a block-specific
	 *	workspace object.					*/

	blk->size = 0;
	blk->object = NULL;
	cursor = blk->bytes + (blk->length - blk->dataLength);
	bundle->ancillaryData.flags = *cursor;
	cursor++;
	bundle->priority = *cursor;	/*	Default.		*/
	cursor++;
	bundle->ancillaryData.ordinal = *cursor;
	bundle->ordinal = *cursor;	/*	Default.		*/
	cursor++;
	bytesRemaining -= 3;
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

int	qos_check(AcqExtBlock *blk, AcqWorkArea *wk)
{
	return 1;
}

void	qos_clear(AcqExtBlock *blk)
{
	return;
}
