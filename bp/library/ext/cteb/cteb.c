/*
 *	cteb.c:		implementation of the Custody Transfer
 *			Enhancement Block (CTEB).
 *
 *  Copyright (c) 2010-2011, Regents of the University of Colorado.
 *  This work was supported by NASA contracts NNJ05HE10G, NNC06CB40C, and
 *  NNC07CB47C.
 *
 *	Author: Andrew Jenkins, University of Colorado
 */

#include "bpP.h"
#include "acsP.h"
#include "cteb.h"


int cteb_offer(ExtensionBlock *blk, Bundle *bundle)
{
	/* If the bundle doesn't request custody transfer, there is no benefit
	 * in attaching a CTEB. */
	if ((bundle->bundleProcFlags & BDL_IS_CUSTODIAL) == 0)
	{
		return 0;
	}

	/* If the ACS system is not initialized, we will be unable to get a
	 * custody ID in cteb_processOnDequeue() so we should not reserve space
	 * for a CTEB. */
	if (getAcssdr() == NULL)
	{
		return 0;
	}

	blk->blkProcFlags = BLK_MUST_BE_COPIED;
	blk->dataLength = 0;	/*	Will know length at dequeue.	*/
	blk->length = 3;	/*	Just to keep block alive.	*/
	blk->size = 0;
	blk->object = 0;
	return 0;
}

void cteb_release(ExtensionBlock *sdrBlk)
{
	Sdr bpSdr = getIonsdr();

	assert(sdr_in_xn(bpSdr) != 0);

	if(sdrBlk->object != 0)
	{
		sdr_free(bpSdr, sdrBlk->object);
		sdrBlk->object = 0;
		sdrBlk->size = 0;
	}
	
	return;
}

/* Store the CTEB to SDR so it can be used when accepting custody.
 * Returns -1 on failure, 0 on success, per Extensions API. */
int cteb_record(ExtensionBlock *sdrBlk, AcqExtBlock *ramBlk)
{
	Sdr bpSdr = getIonsdr();

	assert(sdr_in_xn(bpSdr) != 0);

	sdrBlk->object = sdr_insert(bpSdr, ramBlk->object, ramBlk->size);
	if(sdrBlk->object == 0)
	{
		putErrmsg("Can't store CTEB scratchpad in SDR",
				itoa(ramBlk->size));
		return -1;
	}
	sdrBlk->size = ramBlk->size;
	return 0;
}


/* Copies the CTEB scratchpad from oldBlk to newBlk, in case the bundle
 * containing oldBlk is duplicated during processing.  Returns 0 on success,
 * -1 on system error per the Extensions API. */
int cteb_copy(ExtensionBlock *newBlk, ExtensionBlock *oldBlk)
{
	Sdr bpSdr = getIonsdr();
	CtebScratchpad cteb;

	assert(sdr_in_xn(bpSdr) != 0);

	if(oldBlk->object == 0) return 0;

	sdr_peek(bpSdr, cteb, oldBlk->object);
	newBlk->object = sdr_stow(bpSdr, cteb);
	if(newBlk->object == 0) {
		putErrmsg("Can't copy CTEB", itoa(sizeof(CtebScratchpad)));
		return -1;
	}
	newBlk->size = sizeof(CtebScratchpad);
	
	return 0;
}

/* FIXME: This can probably be processOnEnqueue. Is that more efficient? */
int cteb_processOnDequeue(ExtensionBlock *blk, Bundle *bundle, void *ctxt)
{
	char	*ctebBytes;
	char	*dictionary;
	Sdnv	custodyIdSdnv;
	char	*custodianEid;
	int		custodianEidLen;
	char	*sourceEid;
	int		i = 0;
	int		result;
	AcsCustodyId cid;

	assert((bundle->bundleProcFlags & BDL_IS_CUSTODIAL) != 0);

	if(bundle->custodyTaken == 0)
	{
		/* Bundle is custodial, but we're not the custodian.
		 * We should copy the old CTEB. */
		putErrmsg("FIXME: Bundle is custodial but we're not custodian",
				NULL);
		return 0;
	}

	/* Construct our own CTEB for attaching. */

	/* Get a string representing the current custodian EID. */
	dictionary = retrieveDictionary(bundle);
	if ((void *)(dictionary) == (void *)(bundle))	/* Indicates error. */
	{
		putErrmsg("Can't get dictionary from bundle.", NULL);
        return -1;
	}
	if (printEid(&bundle->custodian, dictionary, &custodianEid) < 0)
	{
		putErrmsg("Can't print custodian EID.", NULL);
		if (dictionary)
		{
			MRELEASE(dictionary);
		}
		return -1;
	}
	if (printEid(&bundle->id.source, dictionary, &sourceEid) < 0)
	{
		putErrmsg("Can't print source EID.", NULL);
		MRELEASE(custodianEid);
		if (dictionary)
		{
			MRELEASE(dictionary);
		}
		return -1;
	}
	if (dictionary)
	{
		MRELEASE(dictionary);
	}
	if (custodianEid == NULL || sourceEid == NULL)
	{
		putErrmsg("Can't get custodian or source EID for CTEB.", NULL);
		return -1;
	}
	custodianEidLen = strlen(custodianEid);

	/* Get a custody identifier for this bundle. */
	if(get_or_make_custody_id(sourceEid, &bundle->id.creationTime,
			bundle->id.fragmentOffset, bundle->totalAduLength == 0 ?
			0 : bundle->payload.length, &cid) < 0)
	{
		ACSLOG_INFO("Can't get a custody ID for the CTEB, is ACS initialized?");

        /* Destroy this extension block */
        	suppressExtensionBlock(blk);
		MRELEASE(custodianEid);
		MRELEASE(sourceEid);
		return 0;
	}

	/* Encode the SDNVs that will be stuffed in the CTEB. */
	encodeSdnv(&custodyIdSdnv, cid.id);

	/* Allocate memory for our own CTEB. */
	blk->dataLength = custodyIdSdnv.length + custodianEidLen;
	ctebBytes = MTAKE(blk->dataLength);
	if (ctebBytes == NULL)
	{
		putErrmsg("Can't construct CTEB text.", itoa(blk->dataLength));
		MRELEASE(custodianEid);
		MRELEASE(sourceEid);
		return -1;
	}

	/* Serialize the CTEB. */
	memcpy(ctebBytes + i, custodyIdSdnv.text, custodyIdSdnv.length);
	i += custodyIdSdnv.length;
	memcpy(ctebBytes + i, custodianEid, custodianEidLen);
	i += custodianEidLen;
	assert(i == blk->dataLength);
	result = serializeExtBlk(blk, NULL, ctebBytes);
	MRELEASE(ctebBytes);
	MRELEASE(custodianEid);
	MRELEASE(sourceEid);
	return result;
}

/* Parses a CTEB into a CtebScratchpad object at blk->object.
 * 
 * Returns -1 on system error, 0 if the block isn't valid, 1 if a valid
 * CTEB is parsed per Extensions API.
 */
int cteb_parse(AcqExtBlock *blk, AcqWorkArea *wk)
{
	CtebScratchpad 	cteb;
	Bundle			*bundle = &wk->bundle;
	unsigned char	*cursor;
	char			*custodianEid;
	char			*bundleCustodianEid;
	int				bytesRemaining = blk->dataLength;

	if (bytesRemaining < 3)
	{
		return 0;		/* Malformed. */
	}

	/* Parse into a scratchpad on the stack. */
	cursor = blk->bytes + (blk->length - blk->dataLength);
	extractSmallSdnv(&cteb.id, &cursor, &bytesRemaining);

	/* FIXME: Handle fragments by continuing to parse here. */

	custodianEid = MTAKE(bytesRemaining + 1);
	if(custodianEid == NULL)
	{
		putErrmsg("Can't parse CTEB custodian EID.",
				itoa(bytesRemaining));
		return -1;		/* Error acquiring. */
	}
	memcpy(custodianEid, cursor, bytesRemaining);
	custodianEid[bytesRemaining] = '\0';
	cursor += bytesRemaining;
	bytesRemaining = 0;


	/* Verify the CTEB is still valid. */
	if (printEid(&bundle->custodian, wk->dictionary, &bundleCustodianEid)
			< 0)
	{
		putErrmsg("Can't print custodian EID string.", NULL);
		MRELEASE(custodianEid);
		return -1;
	}
	if(strcmp(bundleCustodianEid, custodianEid) != 0)
	{
		putErrmsg("Stale CTEB purged", bundleCustodianEid);
		MRELEASE(bundleCustodianEid);	
		MRELEASE(custodianEid);
		return 0;		/* Invalid.	*/
	}
	MRELEASE(bundleCustodianEid);	
	MRELEASE(custodianEid);


	/* Store the scratchpad. */
	blk->object = MTAKE(sizeof(CtebScratchpad));
	if(blk->object == NULL)
	{
		putErrmsg("Can't MTAKE for storing CTEB.",
				itoa(sizeof(CtebScratchpad)));
		return -1;		/* Error acquiring. */
	}
	memcpy(blk->object, &cteb, sizeof(CtebScratchpad));
	blk->size = sizeof(CtebScratchpad);
	return 1;
}

/* Release the CTEB scratchpad in working memory. */
void cteb_clear(AcqExtBlock *blk)
{
	if(blk->object == 0) return;

	MRELEASE(blk->object);
	blk->size = 0;
}

static int loadCtebScratchpadFromWm(Bundle *bundle, AcqWorkArea *work,
			CtebScratchpad *cteb)
{
	int				beforeOrAfter;
	LystElt			extLystElt;
	AcqExtBlock		*acqExtBlk;

	for(beforeOrAfter = 0; beforeOrAfter < 2; beforeOrAfter++)
	{
		for(extLystElt = lyst_first(work->extBlocks[beforeOrAfter]);
			extLystElt;
			extLystElt = lyst_next(extLystElt))
		{
			acqExtBlk = (AcqExtBlock *)(lyst_data(extLystElt));
			if(acqExtBlk->type == EXTENSION_TYPE_CTEB)
			{
				/* Store CTEB scratchpad to cteb */
				memcpy(cteb, acqExtBlk->object,
						sizeof(CtebScratchpad));
				return 0;
			}
		}
	}
	return -1;
}

static int loadCtebScratchpadFromSdr(Sdr sdr, Bundle *bundle, AcqWorkArea *work,
			CtebScratchpad *cteb)
{
	Object          extElt = 0;
	Object		blkAddr;
	ExtensionBlock  blk;

	assert(sdr_in_xn(sdr) != 0);

	/* Find the first CTEB in the bundle. */
	extElt = findExtensionBlock(bundle, EXTENSION_TYPE_CTEB, 0, 0, 0);

	if (extElt == 0)
	{
		/* Didn't find any CTEBs in this bundle. */
		return -1;
	}
	
	/* Load the CTEB's associated scratchpad. */
	blkAddr = sdr_list_data(sdr, extElt);
	sdr_stage(sdr, (char *)(&blk), blkAddr, sizeof(ExtensionBlock));

	/* If there is a CTEB, but it doesn't have a scratchpad, then it
	 * isn't valid:  Any bundle agent properly implementing CTEB would
	 * ensure there's only one CTEB, so there is no valid CTEB on this
	 * bundle.  Don't bother looking further. */
	if (blk.object == 0)
	{
		return -1;
	}

	sdr_stage(sdr, (char *)(cteb), blk.object, blk.size);
	return 0;
}

int loadCtebScratchpad(Sdr sdr, Bundle *bundle, AcqWorkArea *work,
			CtebScratchpad *cteb)
{
	int rc = 0;

	CHKERR(sdr_begin_xn(sdr));

	/* If the bundle hasn't been recorded, the CTEB is in the acquisition 
	 * working memory. */
	if (work != NULL)
	{
		if (loadCtebScratchpadFromWm(bundle, work, cteb) < 0)
		{
			rc = -1;
		}
	}
	else
	{
		if (loadCtebScratchpadFromSdr(sdr, bundle, work, cteb) < 0)
		{
			rc = -1;
		}
	}

	sdr_exit_xn(sdr);
	return rc;
}
