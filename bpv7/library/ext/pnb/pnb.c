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
	unsigned char	dataBuffer[300];
	unsigned char	*cursor;
	uvast		uvtemp;
	int		result;

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
	cursor = dataBuffer;
	uvtemp = 2;
	oK(cbor_encode_array_open(uvtemp, &cursor));
	uvtemp = vscheme->codeNumber;
	oK(cbor_encode_integer(uvtemp, &cursor));
	switch (vscheme->codeNumber)
	{
	case dtn:
		if (strcmp(vscheme->adminEid, "dtn:none") == 0)
		{
			oK(cbor_encode_integer(0, &cursor));
		}
		else
		{
			uvtemp = vscheme->adminNSSLength;
			oK(cbor_encode_text_string(vscheme->adminEid + 4,
					uvtemp, &cursor));
		}

		break;

	case ipn:
		uvtemp = 2;
		oK(cbor_encode_array_open(uvtemp, &cursor));
		uvtemp = getOwnNodeNbr();
		oK(cbor_encode_integer(uvtemp, &cursor));
		uvtemp = 0;
		oK(cbor_encode_integer(uvtemp, &cursor));
		break;

	default:
		return 0;
	}

	blk->dataLength = cursor - dataBuffer;
	return serializeExtBlk(blk, (char *) dataBuffer);
}

int	pnb_parse(AcqExtBlock *blk, AcqWorkArea *wk)
{
	unsigned int	unparsedBytes = blk->dataLength;
	unsigned char	*cursor;
	uvast		arrayLength;
	uvast		uvtemp;
	unsigned char	*nss;

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
	arrayLength = 2;
	if (cbor_decode_array_open(&arrayLength, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Can't decode PN block array.");
		return 0;
	}

	if (cbor_decode_integer(&uvtemp, CborAny, & cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Can't decode PN block array.");
		return 0;
	}

	wk->senderEid.schemeCodeNbr = uvtemp;
	if (wk->senderEid.schemeCodeNbr == ipn)
	{
		arrayLength = 2;
		if (cbor_decode_array_open(&arrayLength, &cursor,
				&unparsedBytes) < 1)
		{
			writeMemo("[?] Can't decode PN block ipn EID array.");
			return 0;
		}

		if (cbor_decode_integer(&uvtemp, CborAny, & cursor,
					&unparsedBytes) < 1)
		{
			writeMemo("[?] Can't decode PN block node number.");
			return 0;
		}

		wk->senderEid.ssp.ipn.nodeNbr = uvtemp;
		if (cbor_decode_integer(&uvtemp, CborAny, & cursor,
					&unparsedBytes) < 1)
		{
			writeMemo("[?] Can't decode PN block service number.");
			return 0;
		}

		wk->senderEid.ssp.ipn.serviceNbr = uvtemp;
		return 1;
	}
	else
	{
		/*	Must be a "dtn" eid.				*/

		uvtemp = 255;
		if (cbor_decode_byte_string(NULL, &uvtemp, &cursor,
				&unparsedBytes) < 1)
		{
			writeMemo("[?] Can't decode PN block dtn NSS.");
			return 0;
		}

		wk->senderEid.ssp.dtn.nssLength = uvtemp;
		nss = MTAKE(uvtemp);
		if (nss == NULL)
		{
			putErrmsg("No space for NSS.", NULL);
			return -1;
		}

		memcpy(nss, cursor, uvtemp);
		wk->senderEid.ssp.dtn.endpointName.s = (char *) nss;
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
