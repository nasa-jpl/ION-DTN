/*
	acslist.c:	Dumps the ACS custody database, checking for inconsistencies
		between the bundle ID hash table and custody ID hash table.
	
	Author: Andrew Jenkins
				University of Colorado at Boulder

	Copyright (c) 2008-2010, Regents of the University of Colorado.
	This work was supported by NASA contracts NNJ05HE10G, NNC06CB40C, and
	NNC07CB47C.													*/

#include <acsP.h>
#include <sdrhash.h>

static int printToStdout = 0;

#ifdef SOLARIS_COMPILER

#define writeMemoOrStdout(args...) 					\
	if (printToStdout == 0)						\
	{								\
		char acslistBuf[1024];					\
		snprintf(acslistBuf, sizeof acslistBuf, __VA_ARGS__);	\
		writeMemo(acslistBuf);					\
	}								\
	else								\
	{								\
		printf(__VA_ARGS__);					\
		printf("\n");                                           \
	}

#else

#define writeMemoOrStdout(args...) 					\
	if (printToStdout == 0)						\
	{								\
		char acslistBuf[1024];					\
		snprintf(acslistBuf, sizeof acslistBuf, args);		\
		writeMemo(acslistBuf);					\
	}								\
	else								\
    	{								\
		printf(args);						\
		printf("\n");                                           \
	}

#endif

/* Counts total number of database errors discovered;
 * used to calculate task return code. */
static int errors = 0;

/* Takes a key/val pair from the hash table mapping custody IDs to CBID pairs,
 * printing the mapping and checking for consistency:
 *  1) The key (custody ID) maps to a custody ID/bundle ID pair with a
 *     matching custody ID.
 *  2) Using the value's bundle ID as the key in a search of the other hash
 *     table (mapping custody IDs to CBID pairs), we find the same result. */
static void printAndCheckByCid(Sdr acsSdr, Object hash, char *key, Address cbidAddr, void *args)
{
	int		*cidCount = (int *)(args);
	AcsCustodyId	*cid = (AcsCustodyId *)(key);
	AcsCbidEntry	cbid;
	int 		hasMismatch = 0;
	int 		rc;
	Object		hashEntry;

	/* These two variables should match cbidAddr & cbid; but they're looked up
	 * in the bid hash table rather than cid hash table as a consistency check. */
	Address         bidCbidAddr;
	AcsCbidEntry    bidCbid;

	/* Load the value and print the mapping. */
	sdr_peek(acsSdr, cbid, cbidAddr);
	writeMemoOrStdout("(%s,%u,%u,%u,%u)->(%u)",
			cbid.bundleId.sourceEid,
			(unsigned int) cbid.bundleId.creationTime.seconds,
			cbid.bundleId.creationTime.count,
			cbid.bundleId.fragmentOffset,
			cbid.bundleId.fragmentLength,
			cid->id);


    /* Verify cbid's idea of custody ID matches the custody ID used as a key in
	 * the hash table. */
	if (cid->id != cbid.custodyId.id)
	{
		writeMemoOrStdout("Mismatch: custody ID in key (%u) "
				"!= in database (%u)",
				cid->id,
				cbid.custodyId.id);
		hasMismatch = 1;
	}

	/* Verify that if we look up using the bundle ID we've found, we get the
	 * same cbid (there are no dangling entries in the cidHash) */
	rc = sdr_hash_retrieve(acsSdr, acsConstants->bidHash,
			(char *)(&cbid.bundleId), &bidCbidAddr, &hashEntry);
	if (rc == -1)
	{
		writeMemoOrStdout("Mismatch: can't find (%s,%u,%u,%u,%u) "
				"in bundle ID database.",
				cbid.bundleId.sourceEid,
				(unsigned int)
				cbid.bundleId.creationTime.seconds,
				cbid.bundleId.creationTime.count,
				cbid.bundleId.fragmentOffset,
				cbid.bundleId.fragmentLength);
		hasMismatch = 1;
	}
	else if (cbidAddr != bidCbidAddr)
	{
		sdr_peek(acsSdr, bidCbid, bidCbidAddr);
		writeMemoOrStdout("Mismatch: "
        		"lookup (%u) in cid: @%lu (%s,%u,%u,%u,%u)->(%u) != "
				"lookup (%s, %u, %u, %u, %u) in bid: @%lu "
				"(%s,%u,%u,%u,%u)->(%u)",
				cid->id,
				(unsigned long) cbidAddr,
				cbid.bundleId.sourceEid,
				(unsigned int)
				cbid.bundleId.creationTime.seconds,
				cbid.bundleId.creationTime.count,
				cbid.bundleId.fragmentOffset,
				cbid.bundleId.fragmentLength,
				cbid.custodyId.id,
				cbid.bundleId.sourceEid,
				(unsigned int)
				cbid.bundleId.creationTime.seconds,
				cbid.bundleId.creationTime.count,
				cbid.bundleId.fragmentOffset,
				cbid.bundleId.fragmentLength,
				(unsigned long) bidCbidAddr,
				bidCbid.bundleId.sourceEid,
				(unsigned int)
				bidCbid.bundleId.creationTime.seconds,
				bidCbid.bundleId.creationTime.count,
				bidCbid.bundleId.fragmentOffset,
				bidCbid.bundleId.fragmentLength,
				bidCbid.custodyId.id);
		hasMismatch = 1;
	}


	/* If any section had a mismatch, this database entry is in error. */
	if (hasMismatch != 0)
	{
		/* errors counts erroneous database entries; while it's helpful to
		 * print each mismatch separately, a set of mismatches for one bid/cbid
		 * pair only count as one error. */
		++errors;
	}
	++(*cidCount);
}
	
static void printAndCheckByCids(Sdr acsSdr)
{
	int cidCount = 0;
	sdr_hash_foreach(acsSdr, acsConstants->cidHash, printAndCheckByCid, &cidCount);
	writeMemoOrStdout("%d custody IDs", cidCount);
}

/* Takes a key/val pair from the hash table mapping bundle IDs to CBID pairs,
 * and checks it for consistency.  It doesn't print the mappings by default
 * because each mapping should be discovered by printAndCheckByCids().
 *
 * If a mapping is found that does not match the mapping when searching by
 * custody ID, that is an inconsistency and is reported as an error. */
static void checkByBid(Sdr acsSdr, Object hash, char *key, Address cbidAddr,
			void *args)
{
	AcsBundleId	*bid = (AcsBundleId *)(key);
	AcsCbidEntry	cbid;
	int 		hasMismatch = 0;
	int 		rc;
	Object		hashEntry;

	/* These two variables should match cbidAddr & cbid; but they're looked up
	 * in the cid hash table rather than bid hash table as a consistency check. */
	Address         cidCbidAddr;
	AcsCbidEntry    cidCbid;

	/* Load the value. */
	sdr_peek(acsSdr, cbid, cbidAddr);

	/* Verify cbid's idea of bundle ID matches the bundle ID used as a key in
	 * the hash table. */
	if (strcmp(cbid.bundleId.sourceEid, bid->sourceEid) != 0) {
		writeMemoOrStdout("Mismatch: source EID in database (%s) "
				"!= in key (%s)",
				cbid.bundleId.sourceEid, bid->sourceEid);
		hasMismatch = 1;
	}
	if (cbid.bundleId.creationTime.seconds != bid->creationTime.seconds  ||
		cbid.bundleId.creationTime.count   != bid->creationTime.count)
	{
		writeMemoOrStdout("Mismatch: creation time in database (%u,%u) "
				"!= in key (%u,%u)", 
				(unsigned int)
				cbid.bundleId.creationTime.seconds,
				cbid.bundleId.creationTime.count,
				(unsigned int) (bid->creationTime.seconds),
				bid->creationTime.count);
		hasMismatch = 1;
	}
	if (cbid.bundleId.fragmentOffset != bid->fragmentOffset ||
		cbid.bundleId.fragmentLength != bid->fragmentLength)
	{
		writeMemoOrStdout("Mismatch: fragment in database (%u,%u) "
				"!= in key (%u,%u)",
				cbid.bundleId.fragmentOffset, bid->fragmentLength,
				cbid.bundleId.fragmentOffset, bid->fragmentLength);
		hasMismatch = 1;
	}

	/* Verify that if we look up using the custody ID we've found, we get the
	 * same cbid (there are no dangling entries in the bidHash) */
	rc = sdr_hash_retrieve(acsSdr, acsConstants->cidHash,
			(char *)(&cbid.custodyId), &cidCbidAddr, &hashEntry);
	if (rc == -1)
	{
		writeMemoOrStdout("Mismatch: can't find (%u) "
				"in custody ID database.",
				cbid.custodyId.id);
				hasMismatch = 1;
	}
	else if (cbidAddr != cidCbidAddr)
	{
		sdr_peek(acsSdr, cidCbid, cidCbidAddr);
		writeMemoOrStdout("Mismatch: "
        		"lookup (%s, %u, %u, %u, %u) in bid: @%lu (%s,%u,%u,%u,%u)->(%u) != "
				"lookup (%u) in cid: @%lu (%s,%u,%u,%u,%u)->(%u)",
				bid->sourceEid,
				(unsigned int) (bid->creationTime.seconds),
				bid->creationTime.count,
				bid->fragmentOffset,
				bid->fragmentLength,
				(unsigned long) cbidAddr,
				cbid.bundleId.sourceEid,
				(unsigned int)
				cbid.bundleId.creationTime.seconds,
				cbid.bundleId.creationTime.count,
				cbid.bundleId.fragmentOffset,
				cbid.bundleId.fragmentLength,
				cbid.custodyId.id,
				cbid.custodyId.id,
				(unsigned long) cidCbidAddr,
				cidCbid.bundleId.sourceEid,
				(unsigned int)
				cidCbid.bundleId.creationTime.seconds,
				cidCbid.bundleId.creationTime.count,
				cidCbid.bundleId.fragmentOffset,
				cidCbid.bundleId.fragmentLength,
				cidCbid.custodyId.id);
		hasMismatch = 1;
	}

	/* If any section had a mismatch, this database entry is in error. */
	if (hasMismatch != 0)
	{
		/* errors counts erroneous database entries; while it's helpful to
		 * print each mismatch separately, a set of mismatches for one bid/cbid
		 * pair only count as one error. */
		++errors;
	}
}

static void checkByBids(Sdr acsSdr)
{
	sdr_hash_foreach(acsSdr, acsConstants->bidHash, checkByBid, NULL);
}
	

#if defined (ION_LWT)
int	acslist(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
#else
int	main(int argc, char **argv)
{
	if(argc > 1) {
		if (strcmp(argv[1], "-s") == 0 ||
				strcmp(argv[1], "--stdout") == 0) {
			printToStdout = 1;
			argc--;
			argv++;
		}
	}
#endif
	Sdr acsSdr;

	/* Attach to ACS database. */
	if (acsAttach() < 0)
	{
		putErrmsg("Can't attach to ACS.", NULL);
		return 1;
	}
	acsSdr = getAcssdr();


	/* Lock SDR and check the database. */
	CHKZERO(sdr_begin_xn(acsSdr));
	printAndCheckByCids(acsSdr);
	checkByBids(acsSdr);
	sdr_exit_xn(acsSdr);


	/* Cleanup */
	writeErrmsgMemos();
	acsDetach();
	bp_detach();
	return errors == 0 ? 0 : 1;
}
