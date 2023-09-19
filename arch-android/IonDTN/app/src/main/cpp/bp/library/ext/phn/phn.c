/*
 *	phn.c:		implementation of the extension definition
 *			functions for the Previous Hop Node block.
 *
 *	Copyright (c) 2009, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 */

#include "bpP.h"
#include "bei.h"
#include "phn.h"

int	phn_offer(ExtensionBlock *blk, Bundle *bundle)
{
	blk->blkProcFlags = BLK_REMOVE_IF_NG;
	blk->dataLength = 0;	/*	Will know length at dequeue.	*/
	blk->length = 3;	/*	Just to keep block alive.	*/
	blk->size = 0;
	blk->object = 0;
	return 0;
}

void	phn_release(ExtensionBlock *blk)
{
	return;
}

int	phn_record(ExtensionBlock *sdrBlk, AcqExtBlock *ramBlk)
{
	return 0;
}

int	phn_copy(ExtensionBlock *newBlk, ExtensionBlock *oldBlk)
{
	return 0;
}

int	phn_processOnFwd(ExtensionBlock *blk, Bundle *bundle, void *ctxt)
{
	return 0;
}

int	phn_processOnAccept(ExtensionBlock *blk, Bundle *bundle, void *ctxt)
{
	return 0;
}

int	phn_processOnEnqueue(ExtensionBlock *blk, Bundle *bundle, void *ctxt)
{
	return 0;
}

int	phn_processOnDequeue(ExtensionBlock *blk, Bundle *bundle, void *ctxt)
{
	DequeueContext	*context = (DequeueContext *) ctxt;
	MetaEid		metaEid;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	int		phnDataLength;
	char		*phnData;
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
	phnDataLength = vscheme->nameLength + 1 + vscheme->adminNSSLength;
	blk->dataLength = phnDataLength + 1;
	phnData = MTAKE(blk->dataLength);
	if (phnData == NULL)
	{
		putErrmsg("Can't construct phn text.", itoa(blk->dataLength));
		return -1;
	}

	memcpy(phnData, vscheme->adminEid, phnDataLength);

	/*	For PHN we don't actually send the EID itself; we
	 *	send the EID's scheme name and NSS as two NULL-
	 *	terminated strings, concatenated.  To do this, we
	 *	simply copy the adminEid and change the colon to
	 *	a NULL.							*/

	*(phnData + vscheme->nameLength) = '\0';
	*(phnData + phnDataLength) = '\0';
	result = serializeExtBlk(blk, NULL, phnData);
	MRELEASE(phnData);
	return result;
}

static int	getSenderEidFromDictionary(AcqExtBlock *blk, AcqWorkArea *wk)
{
	LystElt		elt;
	EndpointId	eid;

	if (lyst_length(blk->eidReferences) != 2 || wk->dictionary == NULL)
	{
		return 0;		/*	Malformed.		*/
	}

	memset((char *) &eid, 0, sizeof(EndpointId));
	elt = lyst_first(blk->eidReferences);
	eid.d.schemeNameOffset = (uaddr) lyst_data(elt);
	elt = lyst_next(elt);
	eid.d.nssOffset = (uaddr) lyst_data(elt);
	if (eid.d.schemeNameOffset > wk->bundle.dictionaryLength
	|| eid.d.nssOffset > wk->bundle.dictionaryLength)
	{
		return 0;		/*	Malformed.		*/
	}

	if (printEid(&eid, wk->dictionary, &(wk->senderEid)) < 0)
	{
		putErrmsg("No space for sender EID.", NULL);
		return -1;
	}

	return 1;
}

int	phn_parse(AcqExtBlock *blk, AcqWorkArea *wk)
{
	int	blkHeaderLen;
	char	*phnData;
	char	*lastByte;
	size_t	schemeNameLen;

	/*	Data parsed out of the phn byte array go directly into
	 *	the work area structure, not into a block-specific
	 *	workspace object.					*/

	blk->size = 0;
	blk->object = NULL;
	if (wk->senderEid)		/*	Provided another way.	*/
	{
		return 1;		/*	Ignore PHN block.	*/
	}

	if (blk->blkProcFlags & BLK_HAS_EID_REFERENCES)
	{
		return getSenderEidFromDictionary(blk, wk);
	}

	blkHeaderLen = blk->length - blk->dataLength;
	phnData = ((char *) (blk->bytes)) + blkHeaderLen;
	lastByte = phnData + (blk->dataLength - 1);
	if (*lastByte != '\0')		/*	S/b null-terminated.	*/
	{
		return 0;		/*	Malformed.		*/
	}

	/*	PHN data comprises two NULL-terminated strings, the
	 *	scheme name and scheme-specific part of the previous
	 *	hop node's endpoint ID.  To change the PHN data to an
	 *	EID we just replace the scheme name's terminal NULL
	 *	with a colon.						*/

	schemeNameLen = istrlen(phnData, blk->dataLength);
	if (schemeNameLen >= (blk->dataLength - 1))
	{
		/*	Scheme name is not NULL-terminated.		*/

		return 0;		/*	Malformed.		*/
	}

	*(phnData + schemeNameLen) = ':';
	wk->senderEid = MTAKE(blk->dataLength);
	if (wk->senderEid == NULL)
	{
		putErrmsg("No space for sender EID.", NULL);
		return -1;
	}

	memcpy(wk->senderEid, phnData, blk->dataLength);
	return 1;
}

int	phn_check(AcqExtBlock *blk, AcqWorkArea *wk)
{
	discardExtensionBlock(blk);
	return 1;
}

void	phn_clear(AcqExtBlock *blk)
{
	return;
}
