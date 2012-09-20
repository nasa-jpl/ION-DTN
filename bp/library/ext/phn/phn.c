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

#include "../../bpP.h"
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
	int		nameLength;
	char		*adminEid;
	int		result;

	suppressExtensionBlock(blk);	/*	Default.		*/
	if (strcmp(context->protocolName, "ltp") == 0)
	{
		/*	The phn block isn't needed; ltpcli can compute
		 *	the previous-hop node EID from engine number.	*/

		return 0;
	}

	/*	The phn block is needed.  To figure out which
	 *	admin EID to use as the previous-hop node EID,
	 *	look at the EID scheme of the proximate node EID;
	 *	use the same scheme, since you know the receiver
	 *	understands that scheme.				*/

	result = parseEidString(context->proxNodeEid, &metaEid, &vscheme,
			&vschemeElt);
	restoreEidString(&metaEid);
	if (result == 0)
	{
		/*	Can't know which admin EID to use.		*/

		return 0;
	}

	restoreExtensionBlock(blk);
	nameLength = vscheme->nameLength + 1 + vscheme->adminNSSLength;
	blk->dataLength = nameLength + 1;
	adminEid = MTAKE(blk->dataLength);
	if (adminEid == NULL)
	{
		putErrmsg("Can't construct phn text.", itoa(blk->dataLength));
		return -1;
	}

	memcpy(adminEid, vscheme->adminEid, nameLength);
	*(adminEid + nameLength) = '\0';
	result = serializeExtBlk(blk, NULL, adminEid);
	MRELEASE(adminEid);
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
	eid.d.schemeNameOffset = (unsigned long) lyst_data(elt);
	elt = lyst_next(elt);
	eid.d.nssOffset = (unsigned long) lyst_data(elt);
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

int	phn_acquire(AcqExtBlock *blk, AcqWorkArea *wk)
{
	int	schemeLength;
	char	*lastByte;
	char	*scheme;
	int	nssLength;
	char	*nss;
	int	eidLen;

	/*	Data parsed out of the phn byte array go directly into
	 *	the work area structure, not into a block-specific
	 *	workspace object.					*/

	blk->size = 0;
	blk->object = NULL;
	if (wk->senderEid != NULL)	/*	Provided by CL.		*/
	{
		return 1;		/*	Ignore PHN block.	*/
	}

	if (blk->blkProcFlags & BLK_HAS_EID_REFERENCES)
	{
		return getSenderEidFromDictionary(blk, wk);
	}

	lastByte = ((char *) (blk->bytes)) + (blk->length - 1);
	if (*lastByte != '\0')		/*	S/b null-terminated.	*/
	{
		return 0;		/*	Malformed.		*/
	}

	scheme = ((char *) (blk->bytes)) + (blk->length - blk->dataLength);
	schemeLength = strlen(scheme);
	nss = scheme + (schemeLength + 1);
	if (nss > lastByte)		/*	No NSS in EID.		*/
	{
		return 0;		/*	Malformed.		*/
	}

	nssLength = strlen(nss);
	eidLen = schemeLength + 1 + nssLength + 1;
	wk->senderEid = MTAKE(eidLen);
	if (wk->senderEid == NULL)
	{
		putErrmsg("No space for sender EID.", NULL);
		return -1;
	}

	isprintf(wk->senderEid, eidLen, "%s:%s", scheme, nss);
	return 1;
}

int	phn_check(AcqExtBlock *blk, AcqWorkArea *wk)
{
	discardExtensionBlock(blk);
	return 0;
}

void	phn_clear(AcqExtBlock *blk)
{
	return;
}
