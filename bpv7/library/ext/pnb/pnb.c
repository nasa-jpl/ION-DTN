/*
 *	pnb.c:		implementation of the extension definition
 *			functions for the Previous Node Block.
 *
 *	Copyright (c) 2019, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 */

#include "bpP.h"
#include "bei.h"
#include "pnb.h"

int	pnb_offer(ExtensionBlock *blk, Bundle *bundle)
{
	blk->blkProcFlags = BLK_REMOVE_IF_NG;
	blk->dataLength = 0;	/*	Will know length at dequeue.	*/
	blk->length = 3;	/*	Just to keep block alive.	*/
	blk->size = 0;
	blk->object = 0;
	return 0;
}

void	pnb_release(ExtensionBlock *blk)
{
	return;
}

int	pnb_record(ExtensionBlock *sdrBlk, AcqExtBlock *ramBlk)
{
	return 0;
}

int	pnb_copy(ExtensionBlock *newBlk, ExtensionBlock *oldBlk)
{
	return 0;
}

int	pnb_processOnFwd(ExtensionBlock *blk, Bundle *bundle, void *ctxt)
{
	return 0;
}

int	pnb_processOnAccept(ExtensionBlock *blk, Bundle *bundle, void *ctxt)
{
	return 0;
}

int	pnb_processOnEnqueue(ExtensionBlock *blk, Bundle *bundle, void *ctxt)
{
	return 0;
}

int	pnb_processOnDequeue(ExtensionBlock *blk, Bundle *bundle, void *ctxt)
{
	DequeueContext	*context = (DequeueContext *) ctxt;
	MetaEid		metaEid;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	int		result;
	EndpointId	eid;
	unsigned char	dataBuffer[300];
	int		length;

	suppressExtensionBlock(blk);	/*	Default.		*/

	/*	To figure out which admin EID to use as the previous-
	 *	hop node EID, look at the EID scheme of the proximate
	 *	node EID; use the same scheme, since you know the
	 *	receiver understands that scheme.			*/

	result = parseEidString(context->proxNodeEid, &metaEid, &vscheme,
			&vschemeElt);
	restoreEidString(&metaEid);
	if (result == 0)
	{
		/*	Can't know which admin EID to use.		*/

		return 0;
	}

	restoreExtensionBlock(blk);
	CHKZERO(parseEidString(vscheme->adminEid, &metaEid, &vscheme,
			&vschemeElt));
	result = jotEid(&eid, &metaEid);
	restoreEidString(&metaEid);
	if (result < 0)
	{
		putErrmsg("Can't jot eid.", NULL);
		return -1;
	}

	length = serializeEid(&eid, dataBuffer);
	eraseEid(&eid);
	if (length <= 0)
	{
		writeMemo("[?] Can't construct Previous Node block.");
		return 0;
	}

	blk->dataLength = length;
	return serializeExtBlk(blk, (char *) dataBuffer);
}

int	pnb_parse(AcqExtBlock *blk, AcqWorkArea *wk)
{
	unsigned char	*cursor;
	unsigned int	unparsedBytes = blk->dataLength;

	if (wk->senderEid.schemeCodeNbr != unknown)
	{
		/*	Sender EID was provided another way,
		 *	so ignore the PHN block.			*/

		return 1;
	}

	/*	Data parsed out of the pnb byte array go directly into
	 *	the work area structure, not into a block-specific
	 *	workspace object.					*/

	blk->size = 0;
	blk->object = NULL;
	cursor = blk->bytes + (blk->length - blk->dataLength);
	if (acquireEid(&(wk->senderEid), &cursor, &unparsedBytes) == 0)
	{
		writeMemo("[?] Invalid sender EID in Previous Node block.");
		return 0;
	}

	if (unparsedBytes != 0)
	{
		writeMemo("[?] Excess bytes at end of Previous Node block.");
		return 0;
	}

	return 1;
}

int	pnb_check(AcqExtBlock *blk, AcqWorkArea *wk)
{
	discardExtensionBlock(blk);
	return 1;
}

void	pnb_clear(AcqExtBlock *blk)
{
	return;
}
