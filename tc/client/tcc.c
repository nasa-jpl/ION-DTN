/*
	tcc.c:	TC Client daemon for reception of bulletin blocks
		published by TC authorities.

	Author: Scott Burleigh, JPL

	Copyright (c) 2020, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
#include "tccP.h"
#include "crypto.h"

#define restrict
#define const
#include "fec.h"

typedef struct
{
	BpSAP	sap;
	int	running;
} TccState;

static TccState	*_tccState(TccState *newState)
{
	void		*value;
	TccState	*state;
	
	if (newState)			/*	Add task variable.	*/
	{
		value = (void *) (newState);
		state = (TccState *) sm_TaskVar(&value);
	}
	else				/*	Retrieve task variable.	*/
	{
		state = (TccState *) sm_TaskVar(NULL);
	}

	return state;
}

static void	shutDown()	/*	Commands tcc termination.	*/
{
	TccState	*state;

	isignal(SIGTERM, shutDown);
	writeMemo("TC client daemon interrupted.");
	state = _tccState(NULL);
	bp_interrupt(state->sap);
	state->running = 0;
}

static Object	retrieveBulletin(TccDB *db, TccBlockHeader *header,
			size_t blksize, TccBulletin *bulletin)
{
	Sdr		sdr = getIonsdr();
	Object		elt;
	Object		obj;
	int		i;
	Object		shareObj;
	TccShare	share;

	for (elt = sdr_list_first(sdr, db->bulletins); elt;
			elt = sdr_list_next(sdr, elt))
	{
		obj = sdr_list_data(sdr, elt);
		sdr_stage(sdr, (char *) bulletin, obj, sizeof(TccBulletin));
		if (bulletin->timestamp < header->timestamp)
		{
			continue;
		}

		if (bulletin->timestamp > header->timestamp)
		{
			break;	/*	Insert new bulletin here.	*/
		}

		if (memcmp(bulletin->hash, header->hash, 32) < 0)
		{
			continue;
		}

		if (memcmp(bulletin->hash, header->hash, 32) > 0)
		{
			break;	/*	Insert new bulletin here.	*/
		}

		if (bulletin->blksize < blksize)
		{
			continue;
		}

		if (bulletin->blksize > blksize)
		{
			break;	/*	Insert new bulletin here.	*/
		}

		/*	Bulletin in buffer is the one we're seeking.	*/

		return elt;
	}

	/*	Bulletin not previously detected.  Okay to add it.	*/

	obj = sdr_malloc(sdr, sizeof(TccBulletin));
	if (obj == 0)
	{
		return 0;
	}

	memset((char *) bulletin, 0, sizeof(TccBulletin));
	bulletin->timestamp = header->timestamp;
	memcpy(bulletin->hash, header->hash, 32);
	bulletin->blksize = blksize;
	bulletin->shares = sdr_list_create(sdr);
	sdr_write(sdr, obj, (char *) bulletin, sizeof(TccBulletin));
	if (elt)
	{
		elt = sdr_list_insert_before(sdr, elt, obj);
	}
	else
	{
		elt = sdr_list_insert_last(sdr, db->bulletins, obj);
	}

	/*	Create "share" objects for all extents of bulletin.	*/

	for (i = 0; i < db->fec_M; i++)
	{
		shareObj = sdr_malloc(sdr, sizeof(TccShare));
		if (shareObj)
		{
			memset((char *) &share, 0, sizeof(TccShare));
			sdr_write(sdr, shareObj, (char *) &share,
					sizeof(TccShare));
			oK(sdr_list_insert_last(sdr, bulletin->shares,
					shareObj));
		}
	}

	return elt;
}

static Object	getShareObj(TccBulletin *bulletin, int shareNbr)
{
	Sdr	sdr = getIonsdr();
	int	i;
	Object	elt;

	for (i = 0, elt = sdr_list_first(sdr, bulletin->shares); elt;
			i++, elt = sdr_list_next(sdr, elt))
	{
		if (i == shareNbr)
		{
			return sdr_list_data(sdr, elt);
		}
	}

	return 0;
}

#if TC_DEBUG
static void	printBlockText(Object text, int offset, int blksize)
{
	Sdr		sdr = getIonsdr();
	ZcoReader	reader;
	char		datValue[TC_MAX_DATLEN];
	int		j;

	zco_start_receiving(text, &reader);
	oK(zco_receive_source(sdr, &reader, offset, NULL));
	oK(zco_receive_source(sdr, &reader, blksize, (char *) datValue));
	for (j = 0; j < blksize; j++)
	{
		printf("%02x", datValue[j]);
	}

	putchar('\n');
}

static void	snap(TccDB *db, TccBulletin *bulletin,
			unsigned int *inputSharenums, 
			char **outputBlocks) 
{
	Sdr		sdr = getIonsdr();
	size_t		blksize = bulletin->blksize;
	Object		shareObj;
	TccShare	share;
	TccBlock	*block;
	int		i;
	char		*blk;
	int		j;

	writeMemoNote("tcc: Blocks from bulletin at", itoa(time(NULL)));
	printf("\ntcc: Blocks from bulletin at %lu:\n", time(NULL));
	for (i = 0; i < db->fec_M; i++)
	{
		printf("\tfor share number %02d:\n", i);
		shareObj = getShareObj(bulletin, i);
		CHKVOID(shareObj);
		sdr_read(sdr, (char *) &share, shareObj, sizeof(TccShare));
		block = share.blocks + TCC_PRIMARY;
		if (block->text == 0)
		{
			block = share.blocks + TCC_BACKUP;
			if (block->text == 0)
			{
				puts("<none>");
				continue;
			}
		}

		printBlockText(block->text, 0, blksize);
	}


	puts("\nShare numbers:\n");
	for (i = 0; i < db->fec_K; i++)
	{
		printf("Buffer slot %02d contains block share number %02d.\n",
				i, inputSharenums[i]);
	}

	puts("\nOutput blocks:\n");
	for (i = 0; i < db->fec_K; i++)
	{
		printf("Buffer slot %02d:\n", i);
		blk = outputBlocks[i];
		for (j = 0; j < blksize; j++)
		{
			printf("%02x", blk[j]);
		}

		putchar('\n');
	}

	putchar('\n');
}
#endif

static int	tryAuths(TccDB *db, fec_t *fec, TccBulletin *bulletin,
			char *inputBuffer,
			char *outputBuffer,
			int bufSize,
			char **inputBlocks,
			char **outputBlocks,
			int suspectAuthNum1,
			int suspectAuthNum2)
{
	Sdr		sdr = getIonsdr();
	size_t		blksize = bulletin->blksize;
	int		sharenumsLen;
	unsigned int	*inputSharenums;
	int		inputslotsLen;
	unsigned char	*inputSlotOccupied;
	int		blocksLoaded;
	int		i;
	Object		shareObj;
	TccShare	share;
	TccBlock	*block;
	int		slotNbr;
	int		j;
	char		*blk;
	ZcoReader	reader;
	vast		len;
	unsigned int	sharenum;
	char		**recoveredBlk;
	unsigned char	hash[32];
	int		match;

	memset(inputBuffer, 0, bufSize);
	memset(outputBuffer, 0, bufSize);
	sharenumsLen = sizeof(int) * db->fec_K;
	inputSharenums = MTAKE(sharenumsLen);
	if (inputSharenums == NULL)
	{
		putErrmsg("No space for inputSharenums.", NULL);
		return -1;
	}

	inputslotsLen = sizeof(char) * db->fec_K;
	inputSlotOccupied = MTAKE(inputslotsLen);
	if (inputSlotOccupied == NULL)
	{
		MRELEASE(inputSharenums);
		putErrmsg("No space for inputSlotOccupied.", NULL);
		return -1;
	}

	memset(inputSharenums, 0, sharenumsLen);
	memset(inputSlotOccupied, 0, sizeof inputslotsLen);
	blocksLoaded = 0;
	for (i = 0; i < db->fec_M; i++)
	{
		slotNbr = -1;
		shareObj = getShareObj(bulletin, i);
		CHKERR(shareObj);
		sdr_read(sdr, (char *) &share, shareObj, sizeof(TccShare));
		block = share.blocks + TCC_PRIMARY;
		if (block->text == 0
		|| block->sourceAuthNum == suspectAuthNum1
		|| block->sourceAuthNum == suspectAuthNum2)
		{
			block = share.blocks + TCC_BACKUP;
			if (block->text == 0
			|| block->sourceAuthNum == suspectAuthNum1
			|| block->sourceAuthNum == suspectAuthNum2)
			{
#if TC_DEBUG
writeMemoNote("tcc: No block for this share", itoa(i));
#endif
				continue;
			}
		}

		if (i < db->fec_K)	/*	Primary block.		*/
		{
			inputSlotOccupied[i] = 1;
			slotNbr = i;
		}
		else			/*	Parity block.		*/
		{
			for (j = 0; j < db->fec_K; j++)
			{
				if (inputSlotOccupied[j] == 0)
				{
					inputSlotOccupied[j] = 1;
					slotNbr = j;
					break;
				}
			}

			if (j == db->fec_K)
			{
				/*	No empty slot for parity block.	*/

				continue;	/*	Don't use it.	*/
			}
		}

		if (slotNbr < 0)
		{
			continue;
		}

		inputSharenums[slotNbr] = i;
		zco_start_receiving(block->text, &reader);
		blk = inputBlocks[slotNbr];
		len = zco_receive_source(sdr, &reader, blksize, (char *) blk);
		if (len != blksize)
		{
			putErrmsg("Failure retrieving block text.", NULL);
			MRELEASE(inputSharenums);
			MRELEASE(inputSlotOccupied);
			return -1;
		}

		blocksLoaded += 1;
#if TC_DEBUG
writeMemoNote("tcc: Loaded block for share", itoa(i));
writeMemoNote("     into input buffer slot", itoa(slotNbr));
#endif
	}

	if (blocksLoaded < db->fec_K)
	{
#if TC_DEBUG
writeMemo("tcc: Not enough blocks for successful decode.");
#endif
		MRELEASE(inputSharenums);
		MRELEASE(inputSlotOccupied);
		return 0;	/*	Not enough blocks for decode.	*/
	}

	/*	Decode the input block array, causing one recovered
	 *	block to be placed in the output block array for each
	 *	parity block that was present in the input block array.
	 *	Then replace the parity blocks in the input block
	 *	array with recovered blocks, recovering the original
	 *	bulletin content.  Finally, overwrite the output
	 *	block array with the updated input block array.		*/

	fec_decode(fec, (unsigned char **) inputBlocks,
		(unsigned char **) outputBlocks, inputSharenums, blksize);
	for (i = 0, recoveredBlk = outputBlocks; i < db->fec_K; i++)
	{
		sharenum = inputSharenums[i];
	       	if (sharenum >= db->fec_K)
		{
			/*	This element of the input block array
			 *	was originally occupied by a parity
			 *	block.  It can now be replaced by one
			 *	of the blocks recovered by fec_decode().*/

			blk = inputBlocks[i];
			memmove(blk, *recoveredBlk, blksize);
			recoveredBlk++;
		}
	}

	memmove(outputBuffer, inputBuffer, bufSize);

	/*	We initialize the hash output to a dummy value in
	 *	case we are running with NULL_SUITES crypto, which
	 *	cannot compute an actual hash.				*/

	memcpy(hash, outputBuffer, 16);
	memcpy(hash + 16, outputBuffer, 16);
	sha2((unsigned char *) outputBuffer, bufSize, hash, 0);
	match = (memcmp(hash, bulletin->hash, 32)) == 0;
#if TC_DEBUG
writeMemoNote("tcc: Match", itoa(match));
writeMemoNote("   bulletin ID", itoa(bulletin->timestamp));
writeMemoNote("   block size", itoa(bulletin->blksize));
writeMemoNote("   blocks in bulletin", itoa(bulletin->sharesAnnounced));
if (match == 0)
{
	snap(db, bulletin, inputSharenums, outputBlocks);
}
#endif
	MRELEASE(inputSharenums);
	MRELEASE(inputSlotOccupied);
	return match;
}

static int	enqueueBulletin(TccDB *db, TccVdb *vdb, char *buffer,
			int bufSize)
{
	Sdr		sdr = getIonsdr();
	TccContent	content;
	Object		contentObj;

	CHKERR(sdr_begin_xn(sdr));
	content.length = bufSize;
	content.data = sdr_malloc(sdr, bufSize);
	if (content.data == 0)
	{
		sdr_exit_xn(sdr);
		putErrmsg("No heap space for bulletin content.", NULL);
		return -1;
	}

	contentObj = sdr_malloc(sdr, sizeof(TccContent));
	if (contentObj == 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("No heap space for content object.", NULL);
		return -1;
	}

	sdr_write(sdr, content.data, buffer, bufSize);
	sdr_write(sdr, contentObj, (char *) &content, sizeof(TccContent));
	if (sdr_list_insert_last(sdr, db->contents, contentObj) == 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't append content object to list.", NULL);
		return -1;
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Failed enqueuing bulletin content.", NULL);
		return -1;
	}

	MRELEASE(buffer);
	sm_SemGive(vdb->contentSemaphore);
	return 1;
}

static uvast	getAuthNodeNbr(TccDB *db, int idx)
{
	Sdr		sdr = getIonsdr();
	Object		elt;
	int		i;
	TccAuthority	auth;

	for (elt = sdr_list_first(sdr, db->authorities), i = 0; elt;
			elt = sdr_list_next(sdr, elt), i++)
	{
		if (i < idx)
		{
			continue;
		}

		sdr_read(sdr, (char *) &auth, sdr_list_data(sdr, elt),
				sizeof(TccAuthority));
		return auth.nodeNbr;
	}

	return ((uvast) -1);
}

static int	reconstructBulletin(TccDB *db, TccVdb *vdb,
			TccBulletin *bulletin)
{
	Sdr		sdr = getIonsdr();
	size_t		blksize = bulletin->blksize;
	int		bufSize;
	char		*inputBuffer;
	char		*outputBuffer;
	char		**inputBlocks;
	char		**outputBlocks;
	fec_t		*fec;
	int		auths;
	int		i;
	int		j;
	int		k;

	bufSize = blksize * db->fec_K;
	inputBuffer = MTAKE(bufSize);
	if (inputBuffer == NULL)
	{
		putErrmsg("Not enough memory for fec decode input.", NULL);
		return -1;
	}

	outputBuffer = MTAKE(bufSize);
	if (outputBuffer == NULL)
	{
		putErrmsg("Not enough memory for fec decode output.", NULL);
		MRELEASE(inputBuffer);
		return -1;
	}

	inputBlocks = MTAKE(sizeof(char *) * db->fec_K);
	if (inputBlocks == NULL)
	{
		putErrmsg("Not enough memory for fec decode input.", NULL);
		MRELEASE(outputBuffer);
		MRELEASE(inputBuffer);
		return -1;
	}

	outputBlocks = MTAKE(sizeof(char *) * db->fec_K);
	if (outputBlocks == NULL)
	{
		putErrmsg("Not enough memory for fec decode output.", NULL);
		MRELEASE(inputBlocks);
		MRELEASE(outputBuffer);
		MRELEASE(inputBuffer);
		return -1;
	}

	for (i = 0; i < db->fec_K; i++)
	{
		inputBlocks[i] = inputBuffer + (i * blksize);
		outputBlocks[i] = outputBuffer + (i * blksize);
	}

	fec = fec_new(db->fec_K, db->fec_M);
	if (fec == NULL)
	{
		putErrmsg("Not enough memory for fec decoder.", NULL);
		MRELEASE(inputBuffer);
		MRELEASE(outputBuffer);
		MRELEASE(inputBlocks);
		MRELEASE(outputBuffer);
	}

	auths = sdr_list_length(sdr, db->authorities);

	/*	Try first K blocks in the bulletin, all authorities.	*/

	switch (tryAuths(db, fec, bulletin, inputBuffer, outputBuffer,
			bufSize, inputBlocks, outputBlocks, -1, -1))
	{
	case -1:
		putErrmsg("Failure decoding bulletin.", NULL);
		fec_free(fec);
		MRELEASE(inputBuffer);
		MRELEASE(outputBuffer);
		MRELEASE(inputBlocks);
		MRELEASE(outputBlocks);
		return -1;

	case 1:
		fec_free(fec);
		MRELEASE(inputBuffer);
		MRELEASE(inputBlocks);
		MRELEASE(outputBlocks);
#if TC_DEBUG
writeMemo("tcc: Enqueuing bulletin from first K blocks.");
#endif
		return enqueueBulletin(db, vdb, outputBuffer, bufSize);

	default:
		break;
	}

#if TC_DEBUG
writeMemo("tcc: First K blocks don't work.");
#endif
	/*	At least one of the authorities is compromised.		*/

	for (j = 0; j < auths; j++)
	{
		switch (tryAuths(db, fec, bulletin, inputBuffer, outputBuffer,
				bufSize, inputBlocks, outputBlocks, j, -1))
		{
		case -1:
			putErrmsg("Failure decoding bulletin.", NULL);
			fec_free(fec);
			MRELEASE(inputBuffer);
			MRELEASE(outputBuffer);
			MRELEASE(inputBlocks);
			MRELEASE(outputBlocks);
			return -1;

		case 1:
			writeMemoNote("[?] Compromised TC authority",
					itoa(getAuthNodeNbr(db, j)));
			fec_free(fec);
			MRELEASE(inputBuffer);
			MRELEASE(inputBlocks);
			MRELEASE(outputBlocks);
#if TC_DEBUG
writeMemo("tcc: Enqueuing bulletin despite 1 compromised TC authority.");
#endif
			return enqueueBulletin(db, vdb, outputBuffer, bufSize);

		default:
			break;			/*	Switch		*/
		}
	}

#if TC_DEBUG
writeMemo("tcc: Can't be just one compromised authority.");
#endif
	/*	At least two of the authorities are compromised.	*/

	for (j = 0; j < auths; j++)
	{
		for (k = 0; k < auths; k++)
		{
			if (k == j)
			{
				continue;
			}

			switch (tryAuths(db, fec, bulletin, inputBuffer,
					outputBuffer, bufSize,
					inputBlocks, outputBlocks, j, k))
			{
			case -1:
				putErrmsg("Failure decoding bulletin.", NULL);
				fec_free(fec);
				MRELEASE(inputBuffer);
				MRELEASE(outputBuffer);
				MRELEASE(inputBlocks);
				MRELEASE(outputBlocks);
				return -1;

			case 1:
				writeMemoNote("[?] Compromised TC authority",
					itoa(getAuthNodeNbr(db, j)));
				writeMemoNote("[?] Compromised TC authority",
					itoa(getAuthNodeNbr(db, k)));
				fec_free(fec);
				MRELEASE(inputBuffer);
				MRELEASE(inputBlocks);
				MRELEASE(outputBlocks);
#if TC_DEBUG
writeMemo("tcc: Enqueuing bulletin despite 2 compromised TC authorities.");
#endif
				return enqueueBulletin(db, vdb, outputBuffer,
						bufSize);

			default:
				break;		/*	Switch		*/
			}
		}
	}

#if TC_DEBUG
writeMemo("tcc: Can't be just two compromised authorities; too little data.");
#endif
	/*	Need more blocks from non-compromised authorities.	*/

	fec_free(fec);
	MRELEASE(inputBuffer);
	MRELEASE(outputBuffer);
	MRELEASE(inputBlocks);
	MRELEASE(outputBlocks);
	return 0;
}

static int	acquireBlock(Sdr sdr, Object dbobj, TccDB *db, TccVdb *vdb,
			char *src, Object adu)
{
	int		parsedOkay;
	MetaEid		metaEid;
	VScheme		*vscheme;
	PsmAddress	schemeElt;
	Object		elt;
	Object		authObj;
	TccAuthority	auth;
	int		i;
	vast		aduLength;
	size_t		blksize;
	uint32_t	timestamp;
	int		blockIdx = -1;
	ZcoReader	reader;
	int		len;
	TccBlockHeader	header;
	Object		bulletinElt;
	Object		bulletinObj;
	TccBulletin	bulletin;
	Object		shareElt;
	Object		shareObj;
	TccShare	share;
	TccBlock	*block;
	int		reconstructionResult;
	int		j;
#if TC_DEBUG
	char		msgbuf[1024];
#endif

	parsedOkay = parseEidString(src, &metaEid, &vscheme, &schemeElt);
	if (!parsedOkay)
	{
		writeMemoNote("[?] Can't parse source of bulletin block", src);
		return 0;
	}

	for (i = 0, elt = sdr_list_first(sdr, db->authorities); elt;
			i++, elt = sdr_list_next(sdr, elt))
	{
		authObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &auth, authObj, sizeof(TccAuthority));
		if (metaEid.elementNbr == auth.nodeNbr)
		{
			break;
		}
	}

	restoreEidString(&metaEid);
	if (elt == 0)
	{
		writeMemoNote("[?] Bulletin block from non-authority", src);
		return 0;
	}

	aduLength = zco_source_data_length(sdr, adu);
	if (aduLength <= 40)
	{
		len = aduLength;
		writeMemoNote("[?] TC bulletin block too small", itoa(len));
		return 0;
	}

	blksize = aduLength - 40;
	zco_start_receiving(adu, &reader);
	len = zco_receive_source(sdr, &reader, 4, (char *) &timestamp);
	timestamp = ntohl(timestamp);
	header.timestamp = timestamp;
	if (header.timestamp <= db->lastBulletinTime)
	{
		return 0;	/*	Late-arriving block.		*/
	}

	len = zco_receive_source(sdr, &reader, 32, (char *) header.hash);
	len = zco_receive_source(sdr, &reader, 4, (char *) &header.sharenum);
	header.sharenum = ntohl(header.sharenum);
#if TC_DEBUG
writeMemoNote("tcc: received block for share", itoa(header.sharenum));
if (header.sharenum >= db->fec_K)
{
	writeMemoNote("tcc: Parity block printing at", itoa(time(NULL)));
	printf("\ntcc: Parity block printing at %lu:\n", time(NULL));
	printBlockText(adu, 40, blksize);
}
#endif
	if (header.sharenum >= auth.firstPrimaryShare
	&& header.sharenum <= auth.lastPrimaryShare)
	{
		blockIdx = TCC_PRIMARY;
	}
	else
	{
		if (header.sharenum >= auth.firstBackupShare
		&& header.sharenum <= auth.lastBackupShare)
		{
			blockIdx = TCC_BACKUP;
		}
	}

	if (blockIdx < 0)
	{
#if TC_DEBUG
isprintf(msgbuf, sizeof msgbuf, "tcc: Block %d out of range for authority %d.  \
Authority node nbr " UVAST_FIELDSPEC ", primary %d - %d, backup %d - %d.\n",
header.sharenum, i, auth.nodeNbr, auth.firstPrimaryShare, auth.lastPrimaryShare,
auth.firstBackupShare, auth.lastBackupShare);
writeMemo(msgbuf);
#endif
		/*	Block not within authority's purview.		*/

		return 0;
	}

	/*	TODO: devise defense against malicious stream of new
	 *	bogus bulletins.					*/

	bulletinElt = retrieveBulletin(db, &header, blksize, &bulletin);
	if (bulletinElt == 0)
	{
		putErrmsg("Can't add new bulletin.", NULL);
		return -1;
	}

	bulletinObj = sdr_list_data(sdr, bulletinElt);
	sdr_stage(sdr, (char *) &bulletin, bulletinObj, sizeof(TccBulletin));
	shareObj = getShareObj(&bulletin, header.sharenum);
	CHKERR(shareObj);
	sdr_stage(sdr, (char *) &share, shareObj, sizeof(TccShare));
	block = share.blocks + blockIdx;
	if (share.blocksAnnounced == 0)	/*	1st block.	*/
	{
		bulletin.sharesAnnounced += 1;
		sdr_write(sdr, bulletinObj, (char *) &bulletin,
				sizeof(TccBulletin));
#if TC_DEBUG
isprintf(msgbuf, sizeof msgbuf, "tcc: Inserting first block for share %d, \
idx=%d, announced by authority %d.\n", header.sharenum, blockIdx, i);
writeMemo(msgbuf);
#endif
	}
	else	/*	Not the first block received for this share.	*/
	{
		if (block->text != 0)		/*	Duplicate.	*/
		{
#if TC_DEBUG
writeMemo("tcc: Duplicates a previously received block.");
#endif
			return 0;
		}
#if TC_DEBUG
else
isprintf(msgbuf, sizeof msgbuf, "tcc: Inserting other block for share %d, \
idx=%d, announced by authority %d.\n", header.sharenum, blockIdx, i);
writeMemo(msgbuf);
#endif
	}

	/*	Must insert this block into the aggregated bulletin.	*/

	block->sourceAuthNum = i;
	block->text = zco_clone(sdr, adu, 40, blksize);
	if (block->text == 0)
	{
		putErrmsg("Can't acquire block.", NULL);
		return -1;
	}

	share.blocksAnnounced += 1;
	sdr_write(sdr, shareObj, (char *) &share, sizeof(TccShare));
	if (bulletin.sharesAnnounced < db->fec_K)
	{
#if TC_DEBUG
writeMemoNote("tcc: Bulletin can't be complete yet, sharesAnnounced",
itoa(bulletin.sharesAnnounced));
#endif
		return 0;	/*	Bulletin can't be complete.	*/
	}

	/*	Must check to see if the entire bulletin has arrived.	*/

	reconstructionResult = reconstructBulletin(db, vdb, &bulletin);
	if (reconstructionResult < 1)
	{
		return reconstructionResult;
	}

	/*	Bulletin was reconstructed and handled successfully.
	 *	Now delete it.						*/

	sdr_stage(sdr, (char *) db, dbobj, sizeof(TccDB));
	db->lastBulletinTime = bulletin.timestamp;
	sdr_write(sdr, dbobj, (char *) db, sizeof(TccDB));
	sdr_list_delete(sdr, bulletinElt, NULL, NULL);
	while (1)
	{
		shareElt = sdr_list_first(sdr, bulletin.shares);
		if (shareElt == 0)
		{
			break;
		}

		shareObj = sdr_list_data(sdr, shareElt);
		sdr_read(sdr, (char *) &share, shareObj, sizeof(TccShare));
		for (j = 0, block = share.blocks; j < 2; j++, block++)
		{
			if (block->text)
			{
				zco_destroy(sdr, block->text);
			}
		}

		sdr_free(sdr, shareObj);
		sdr_list_delete(sdr, shareElt, NULL, NULL);
	}

	sdr_list_destroy(sdr, bulletin.shares, NULL, NULL);
	sdr_free(sdr, bulletinObj);
	return 0;
}

#if defined (ION_LWT)
int	tcc(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	int	blocksGroupNbr = (a1 ? atoi((char *) a1) : -1);
#else
int	main(int argc, char *argv[])
{
	int	blocksGroupNbr = (argc > 1 ? atoi(argv[1]) : -1);
#endif
	char		ownEid[32];
	TccState	state = { NULL, 1 };
	Sdr		sdr;
	Object		dbobj;
	TccDB		db;
	TccVdb		*vdb;
	BpDelivery	dlv;

	if (blocksGroupNbr < 1)
	{
		puts("Usage: tcc <IMC group number for TC blocks>");
		return -1;
	}

	if (tccAttach(blocksGroupNbr) < 0)
	{
		putErrmsg("tcc can't attach to TC client support",
				itoa(blocksGroupNbr));
		return 1;
	}

	isprintf(ownEid, sizeof ownEid, "imc:%d.0", blocksGroupNbr);
	if (bp_open(ownEid, &state.sap) < 0)
	{
		putErrmsg("Can't open own endpoint.", ownEid);
		ionDetach();
		return 1;
	}

	sdr = getIonsdr();
	dbobj = getTccDBObj(blocksGroupNbr);
	if (dbobj == 0)
	{
		putErrmsg("No TC client database.", NULL);
		ionDetach();
		return 1;
	}

	sdr_read(sdr, (char *) &db, dbobj, sizeof(TccDB));
	vdb = getTccVdb(blocksGroupNbr);

	/*	Main loop: receive block, try to build bulletin.	*/

	oK(_tccState(&state));
	isignal(SIGTERM, shutDown);
	writeMemo("[i] tcc is running.");
	while (state.running)
	{
		if (bp_receive(state.sap, &dlv, BP_BLOCKING) < 0)
		{
			putErrmsg("tcc bundle reception failed.", NULL);
			state.running = 0;
			continue;
		}

		if (dlv.result == BpReceptionInterrupted)
		{
			bp_release_delivery(&dlv, 1);
			continue;
		}

		if (dlv.result == BpEndpointStopped)
		{
			bp_release_delivery(&dlv, 1);
			state.running = 0;
			continue;
		}

		if (dlv.result == BpPayloadPresent)
		{
			CHKZERO(sdr_begin_xn(sdr));
			if (acquireBlock(sdr, dbobj, &db, vdb,
					dlv.bundleSourceEid, dlv.adu) < 0)
			{
				putErrmsg("Can't acquire block.", NULL);
				sdr_cancel_xn(sdr);
			}

			if (sdr_end_xn(sdr) < 0)
			{
				putErrmsg("Can't handle TC block.", NULL);
				state.running = 0;
				continue;
			}
		}

		bp_release_delivery(&dlv, 1);
	}

	bp_close(state.sap);
	writeErrmsgMemos();
	writeMemo("[i] tcc has ended.");
	ionDetach();
	return 0;
}
