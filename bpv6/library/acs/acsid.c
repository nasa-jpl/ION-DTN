/*
	acsid.c: Implementation of the ACS Custody ID database.

	Authors: Andrew Jenkins, Sebastian Kuzminsky,
				University of Colorado at Boulder

	Copyright (c) 2008-2011, Regents of the University of Colorado.
	This work was supported by NASA contracts NNJ05HE10G, NNC06CB40C, and
	NNC07CB47C.
 											*/

#include "acsP.h"
#include "sdrhash.h"

static Sdr      acsSdr = NULL;

static void consume_cid(AcsCustodyId *cid)
{
	unsigned int newid = 0;

	sdr_peek(acsSdr, cid->id, acsConstants->id);
	newid = cid->id + 1;
	sdr_poke(acsSdr, acsConstants->id, newid);
}


int get_or_make_custody_id(const char *sourceEid,
		const BpTimestamp *creationTime, unsigned int fragmentOffset,
		unsigned int fragmentLength, AcsCustodyId *cid)
{
	AcsBundleId	bid;
	Address		cbidAddr;
	AcsCbidEntry	cbid;
	Object		cbidObj;
	int		rc;
	Object		hashEntry;

	if (acsAttach() < 0)
	{
		//Couldn't offerNoteAcs(): ACS SDR not available.
		return -2;
	}

	if ((acsSdr = getAcssdr()) == NULL)
	{
		ACSLOG_DEBUG("get_or_make_custody_id: ACS not initialized, skipping.");
		return -2;
	}

	memset(&bid, 0, sizeof(bid));
	strncpy(bid.sourceEid, sourceEid, MAX_EID_LEN);
	bid.sourceEid[MAX_EID_LEN - 1] = '\0';
	bid.creationTime.seconds = creationTime->seconds;
	bid.creationTime.count = creationTime->count;
	bid.fragmentOffset = fragmentOffset;
	bid.fragmentLength = fragmentLength;

	CHKERR(sdr_begin_xn(acsSdr));

	rc = sdr_hash_retrieve(acsSdr, acsConstants->bidHash,
			(char *)(&bid), &cbidAddr, &hashEntry);
	if (rc == -1)
	{
		ACSLOG_ERROR("Couldn't search for (%s,%u,%u,%u,%u) in bidHash",
			bid.sourceEid, (unsigned int) bid.creationTime.seconds,
			bid.creationTime.count, bid.fragmentOffset,
			bid.fragmentLength);
		sdr_cancel_xn(acsSdr);
		return -1;
	}

	/* If a CID already exists, return it. */
	if (rc == 1)
	{
		if (cid != NULL)
		{
			sdr_peek(acsSdr, cbid, cbidAddr);
			cid->id = cbid.custodyId.id;
		}
		sdr_exit_xn(acsSdr);
		return 1;
	}

	/* No CID exists; create one. */
	memcpy(&cbid.bundleId, &bid, sizeof(bid));
	consume_cid(&cbid.custodyId);

	cbidObj = sdr_stow(acsSdr, cbid);

	/* Add a ref to the bidHash to this cbid. */
	if (sdr_hash_insert(acsSdr, acsConstants->bidHash,
				(char *)(&cbid.bundleId), cbidObj, NULL) < 0)
	{
		ACSLOG_ERROR("Couldn't insert new CBID in bidHash");
		sdr_cancel_xn(acsSdr);
		return -1;
	}

	/* Add a ref to the cidHash to this cbid. */
	if (sdr_hash_insert(acsSdr, acsConstants->cidHash,
				(char *)(&cbid.custodyId), cbidObj, NULL) < 0)
	{
		ACSLOG_ERROR("Couldn't insert new CBID in cidHash");
		sdr_cancel_xn(acsSdr);
		return -1;
	}

	if(cid != NULL)
	{
		cid->id = cbid.custodyId.id;
	}

	return sdr_end_xn(acsSdr);
}

int get_bundle_id(AcsCustodyId *custodyId, AcsBundleId *id)
{
	int		rc;
	Address		cbidAddr;
	AcsCbidEntry	cbid;
	Object		hashEntry;

	if (acsAttach() < 0)
	{
		ACSLOG_ERROR("get_bundle_id: Couldn't attach to ACS.");
		return -1;
	}

	if ((acsSdr = getAcssdr()) == NULL)
	{
		ACSLOG_ERROR("get_bundle_id: ACS not initialized.");
		return -1;
	}

	CHKERR(sdr_begin_xn(acsSdr));
	rc = sdr_hash_retrieve(acsSdr, acsConstants->cidHash,
			(char *)(custodyId), &cbidAddr, &hashEntry);
	if (rc == 1)
	{
		sdr_peek(acsSdr, cbid, cbidAddr);
	}
	sdr_exit_xn(acsSdr);

	/* Error searching hash table. */
	if (rc == -1)
	{
		ACSLOG_ERROR("Couldn't search for (%u) in cidHash", custodyId->id);
		return -1;
	}

	/* Hash table searched, but no matching custody ID found. */
	if (rc == 0)
	{
		ACSLOG_WARN("Couldn't find cid (%u) in cidHash", custodyId->id);
		return 1;
	}

	/* Matching custody ID found.  Assign outputs if requested by caller. */
	if(id != NULL)
	{
		memcpy(id, &cbid.bundleId, sizeof(cbid.bundleId));
	}

	return 0;
}

int destroy_custody_id(AcsBundleId *bundleId)
{
	Address		cbidAddr;
	AcsCbidEntry	cbid;
	int		rc;
	Object		hashEntry;

	if (acsAttach() < 0)
	{
		//Couldn't destroy custody ID: ACS SDR not available.
		return 0;
	}

	if ((acsSdr = getAcssdr()) == NULL)
	{
		ACSLOG_DEBUG("destroy_custody_id: ACS not initialized, skipping.");
		return -2;
	}

	/* Lookup the cbid. */
	CHKERR(sdr_begin_xn(acsSdr));
	rc = sdr_hash_retrieve(acsSdr, acsConstants->bidHash,
			(char *)(bundleId), &cbidAddr, &hashEntry);
	if (rc == -1)
	{
		ACSLOG_ERROR("Couldn't search for (%s,%u,%u,%u,%u) in bidHash \
to destroy", bundleId->sourceEid,
			(unsigned int) (bundleId->creationTime.seconds),
			bundleId->creationTime.count, bundleId->fragmentOffset,
			bundleId->fragmentLength);
		sdr_exit_xn(acsSdr);
		return -1;
	}
	if (rc == 0)
	{
		ACSLOG_WARN("Couldn't find (%s,%u,%u,%u,%u) in bidHash to \
destroy", bundleId->sourceEid, (unsigned int) bundleId->creationTime.seconds,
			bundleId->creationTime.count,
			bundleId->fragmentOffset,
			bundleId->fragmentLength);
		sdr_exit_xn(acsSdr);
		return 0;
	}
	sdr_peek(acsSdr, cbid, cbidAddr);

	/* Found the CBID; destroy the entries in cidHash and bidHash. */
	rc = sdr_hash_remove(acsSdr, acsConstants->cidHash,
				(char *)(&cbid.custodyId), NULL);
	if (rc != 1)
	{
		ACSLOG_ERROR("Couldn't delete (%u) from cidHash", cbid.custodyId.id);
		sdr_cancel_xn(acsSdr);
		return -1;
	}
	rc = sdr_hash_remove(acsSdr, acsConstants->bidHash,
				(char *)(&cbid.bundleId), NULL);
	if (rc != 1)
	{
		ACSLOG_ERROR("Couldn't delete (%s,%u,%u,%u,%u) from bidHash",
			cbid.bundleId.sourceEid,
			(unsigned int) cbid.bundleId.creationTime.seconds,
			cbid.bundleId.creationTime.count,
		       	cbid.bundleId.fragmentOffset,
			cbid.bundleId.fragmentLength);
		sdr_cancel_xn(acsSdr);
		return -1;
	}

	/* Hash table entries destroyed; destroy the CBID Object in SDR they
	 * pointed to. */
	sdr_free(acsSdr, cbidAddr);

	/* Cleanup */
	if (sdr_end_xn(acsSdr) != 0)
	{
		ACSLOG_ERROR("Couldn't destroy custody ID.");
		return -1;
	}
	return 1;
}

int destroyAcsMetadata(Bundle *bundle)
{
	AcsBundleId 	bid;
	char 			*dictionary;
	char			*sourceEid;

	memset(&bid, 0, sizeof(bid));

	/* Get the source EID */
	if ((dictionary = retrieveDictionary(bundle))
			== (char *) bundle)
	{
		putErrmsg("Can't retrieve dictionary.", NULL);
		return -1;
	}

	if (printEid(&(bundle->id.source), dictionary, &sourceEid) < 0)
	{
		putErrmsg("Can't get sourceEid.", NULL);
		releaseDictionary(dictionary);
		return -1;
	}
	releaseDictionary(dictionary);
	strncpy(bid.sourceEid, sourceEid, MAX_EID_LEN);
	bid.sourceEid[MAX_EID_LEN - 1] = '\0';
	MRELEASE(sourceEid);

	/* Get the rest of the bundle ID */
	bid.creationTime.seconds = bundle->id.creationTime.seconds;
	bid.creationTime.count = bundle->id.creationTime.count;
	bid.fragmentOffset = bundle->id.fragmentOffset;
	bid.fragmentLength = bundle->totalAduLength == 
							0 ? 0 : bundle->payload.length;

	/* Destroy the metadata */
	return destroy_custody_id(&bid);
}
