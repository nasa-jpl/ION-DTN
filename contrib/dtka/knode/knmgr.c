/*
	knmgr.c:	Key node daemon for reception of bulletin
			blocks published by key authorities.

	Author: Scott Burleigh, JPL

	Copyright (c) 2013, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
#include "knode.h"
#include "bpP.h"

typedef struct
{
	BpSAP	sap;
	int	running;
} KmgrState;

static KmgrState	*_knmgrState(KmgrState *newState)
{
	void		*value;
	KmgrState	*state;
	
	if (newState)			/*	Add task variable.	*/
	{
		value = (void *) (newState);
		state = (KmgrState *) sm_TaskVar(&value);
	}
	else				/*	Retrieve task variable.	*/
	{
		state = (KmgrState *) sm_TaskVar(NULL);
	}

	return state;
}

static void	shutDown()	/*	Commands knmgr termination.	*/
{
	KmgrState	*state;

	isignal(SIGTERM, shutDown);
	PUTS("DTKA bulletin manager interrupted.");
	state = _knmgrState(NULL);
	bp_interrupt(state->sap);
	state->running = 0;
}

static Object	retrieveBulletin(DtkaNodeDB *db, DtkaBlockHeader *header,
			size_t blksize, DtkaBulletin *bulletin)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
	Object	obj;

	for (elt = sdr_list_first(sdr, db->bulletins); elt;
			elt = sdr_list_next(sdr, elt))
	{
		obj = sdr_list_data(sdr, elt);
		sdr_stage(sdr, (char *) bulletin, obj, sizeof(DtkaBulletin));
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

	obj = sdr_malloc(sdr, sizeof(DtkaBulletin));
	if (obj == 0)
	{
		return 0;
	}

	memset((char *) bulletin, 0, sizeof(DtkaBulletin));
	bulletin->timestamp = header->timestamp;
	memcpy(bulletin->hash, header->hash, 32);
	bulletin->blksize = blksize;
	sdr_write(sdr, obj, (char *) bulletin, sizeof(DtkaBulletin));
	if (elt)
	{
		elt = sdr_list_insert_before(sdr, elt, obj);
	}
	else
	{
		elt = sdr_list_insert_last(sdr, db->bulletins, obj);
	}

	return elt;
}

#if DTKA_DEBUG
static void	printBlockText(Object text, int offset, int blksize)
{
	Sdr		sdr = getIonsdr();
	ZcoReader	reader;
	unsigned char	datValue[DTKA_MAX_DATLEN];
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

static void	snap(DtkaBulletin *bulletin, unsigned int *inputSharenums,
		unsigned char **inputBlocks, unsigned char **outputBlocks,
		unsigned char *inputBuffer, unsigned char *outputBuffer)
{
	size_t		blksize = bulletin->blksize;
	DtkaShare	*share;
	DtkaBlock	*block;
	int		i;
	unsigned char	*blk;
	int		j;

	puts("\nBlocks from bulletin:");
	for (i = 0; i < DTKA_FEC_M; i++)
	{
		printf("\tfor share number %02d:\n", i);
		share = bulletin->shares + i;
		block = share->blocks + DTKA_PRIMARY;
		if (block->text == 0)
		{
			block = share->blocks + DTKA_BACKUP;
			if (block->text == 0)
			{
				puts("<none>");
				continue;
			}
		}

		printBlockText(block->text, 0, blksize);
	}


	puts("\nShare numbers:\n");
	for (i = 0; i < DTKA_FEC_K; i++)
	{
		printf("Buffer slot %02d contains block share number %02d.\n",
				i, inputSharenums[i]);
	}

	puts("\nOutput blocks:\n");
	for (i = 0; i < DTKA_FEC_K; i++)
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

static int	tryAuths(fec_t *fec, DtkaBulletin *bulletin,
			unsigned char *inputBuffer,
			unsigned char *outputBuffer,
			int bufSize,
			unsigned char **inputBlocks,
			unsigned char **outputBlocks,
			int suspectAuthNum1,
			int suspectAuthNum2)
{
	Sdr		sdr = getIonsdr();
	size_t		blksize = bulletin->blksize;
	unsigned int	inputSharenums[DTKA_FEC_K];
	unsigned char	inputSlotOccupied[DTKA_FEC_K];
	int		blocksLoaded;
	int		i;
	DtkaShare	*share;
	DtkaBlock	*block;
	int		slotNbr;
	int		j;
	unsigned char	*blk;
	ZcoReader	reader;
	vast		len;
	unsigned int	sharenum;
	unsigned char	**recoveredBlk;
	unsigned char	hash[32];
	int		match;

	memset(inputBuffer, 0, bufSize);
	memset(outputBuffer, 0, bufSize);
	memset(inputSharenums, 0, sizeof inputSharenums);
	memset(inputSlotOccupied, 0, sizeof inputSlotOccupied);
	blocksLoaded = 0;
	for (i = 0; i < DTKA_FEC_M; i++)
	{
		share = bulletin->shares + i;
		block = share->blocks + DTKA_PRIMARY;
		if (block->text == 0
		|| block->sourceAuthNum == suspectAuthNum1
		|| block->sourceAuthNum == suspectAuthNum2)
		{
			block = share->blocks + DTKA_BACKUP;
			if (block->text == 0
			|| block->sourceAuthNum == suspectAuthNum1
			|| block->sourceAuthNum == suspectAuthNum2)
			{
#if DTKA_DEBUG
printf("No block for share #%d.\n", i);
fflush(stdout);
#endif
				continue;
			}
		}

		if (i < DTKA_FEC_K)	/*	Primary block.		*/
		{
			inputSlotOccupied[i] = 1;
			slotNbr = i;
		}
		else			/*	Parity block.		*/
		{
			for (j = 0; j < DTKA_FEC_K; j++)
			{
				if (inputSlotOccupied[j] == 0)
				{
					inputSlotOccupied[j] = 1;
					slotNbr = j;
					break;
				}
			}

			if (j == DTKA_FEC_K)
			{
				/*	No empty slot for parity block.	*/

				continue;	/*	Don't use it.	*/
			}
		}

		inputSharenums[slotNbr] = i;
		zco_start_receiving(block->text, &reader);
		blk = inputBlocks[slotNbr];
		len = zco_receive_source(sdr, &reader, blksize, (char *) blk);
		if (len != blksize)
		{
			putErrmsg("Failure retrieving block text.", NULL);
			return -1;
		}

		blocksLoaded += 1;
#if DTKA_DEBUG
printf("Loaded block for share %d into input buffer slot %d.\n", i, slotNbr);
fflush(stdout);
#endif
	}

	if (blocksLoaded < DTKA_FEC_K)
	{
		return 0;	/*	Not enough blocks for decode.	*/
	}

	/*	Decode the input block array, causing one recovered
	 *	block to be placed in the output block array for each
	 *	parity block that was present in the input block array.
	 *	Then replace the parity blocks in the input block
	 *	array with recovered blocks, recovering the original
	 *	bulletin content.  Finally, overwrite the output
	 *	block array with the updated input block array.		*/

	fec_decode(fec, inputBlocks, outputBlocks, inputSharenums, blksize);
	for (i = 0, recoveredBlk = outputBlocks; i < DTKA_FEC_K; i++)
	{
		sharenum = inputSharenums[i];
	       	if (sharenum >= DTKA_FEC_K)
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
	sha2(outputBuffer, bufSize, hash, 0);
	match = (memcmp(hash, bulletin->hash, 32)) == 0;
#if DTKA_DEBUG
printf("Match=%d; bulletin ID %d, block size %d, %u blocks in this bulletin.\n",
match, (int) (bulletin->timestamp), (int) (bulletin->blksize),
bulletin->sharesAnnounced);
fflush(stdout);
if (match == 0)
{
	snap(bulletin, inputSharenums, inputBlocks, outputBlocks, inputBuffer,
			outputBuffer);
}
#endif
	return match;
}

static void	printRecord(uvast nodeNbr, time_t effectiveTime,
			time_t assertionTime, unsigned short datLength,
			unsigned char *datValue)
{
	char	msgbuf[1024];
	char	*cursor = msgbuf;
	int	bytesRemaining = sizeof msgbuf;
	int	i;
	int	len;

	len = _isprintf(cursor, bytesRemaining, UVAST_FIELDSPEC " %lu %lu ",
			nodeNbr, assertionTime, effectiveTime);
	cursor += len;
	bytesRemaining -= len;
	if (datLength == 0)
	{
		istrcpy(cursor, "[revoke]", bytesRemaining);
		cursor += 8;
		bytesRemaining -= 8;
	}
	else
	{
		for (i = 0; i < datLength; i++)
		{
			len = _isprintf(cursor, bytesRemaining, "%02x",
					datValue[i]);
			cursor += len;
			bytesRemaining -= len;
		}
	}

	PUTS(msgbuf);
}

static int	handleBulletin(unsigned char *buffer, int bufSize)
{
	unsigned char	*cursor = buffer;
	int		bytesRemaining = bufSize;
	uvast		nodeNbr;
	time_t		effectiveTime;
	time_t		assertionTime;
	unsigned short	datLength;
	unsigned char	datValue[DTKA_MAX_DATLEN];
	int		recCount = 0;
	char		msgbuf[72];

	PUTS("\n---Bulletin received---");
	while (bytesRemaining >= 14)
	{
		if (dtka_deserialize(&cursor, &bytesRemaining, DTKA_MAX_DATLEN,
				&nodeNbr, &effectiveTime, &assertionTime,
				&datLength, datValue) < 1)
		{
			writeMemo("[?] DTKA bulletin malformed, discarded.");
			break;
		}

		if (nodeNbr == 0)	/*	Block padding bytes.	*/
		{
			break;
		}

		recCount++;
		printRecord(nodeNbr, effectiveTime, assertionTime, datLength,
				datValue);
		if (datLength == 0)
		{
			if (sec_removePublicKey(nodeNbr, effectiveTime) < 0)
			{
				putErrmsg("Failed handling bulletin.", NULL);
				MRELEASE(buffer);
				return -1;
			}

			continue;
		}

		if (sec_addPublicKey(nodeNbr, effectiveTime, assertionTime,
				datLength, datValue) < 0)
		{
			putErrmsg("Failed handling bulletin.", NULL);
			MRELEASE(buffer);
			return -1;
		}
	}

	MRELEASE(buffer);
	isprintf(msgbuf, sizeof msgbuf, "Number of records received: %d",
			recCount);
	PUTS(msgbuf);
	return 1;
}

static int	reconstructBulletin(DtkaNodeDB *db, DtkaBulletin *bulletin)
{
	size_t		blksize = bulletin->blksize;
	int		bufSize;
	unsigned char	*inputBuffer;
	unsigned char	*outputBuffer;
	unsigned char	*inputBlocks[DTKA_FEC_K];
	unsigned char	*outputBlocks[DTKA_FEC_K];
	fec_t		*fec;
	int		i;
	int		j;
	int		k;

	bufSize = blksize * DTKA_FEC_K;
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

	for (i = 0; i < DTKA_FEC_K; i++)
	{
		inputBlocks[i] = inputBuffer + (i * blksize);
		outputBlocks[i] = outputBuffer + (i * blksize);
	}

	fec = fec_new(DTKA_FEC_K, DTKA_FEC_M);
	if (fec == NULL)
	{
		putErrmsg("Not enough memory for fec decoder.", NULL);
		MRELEASE(inputBuffer);
		MRELEASE(outputBuffer);
	}

	/*	Try first K blocks in the bulletin, all authorities.	*/

	switch (tryAuths(fec, bulletin, inputBuffer, outputBuffer,
			bufSize, inputBlocks, outputBlocks, -1, -1))
	{
	case -1:
		putErrmsg("Failure decoding bulletin.", NULL);
		fec_free(fec);
		MRELEASE(inputBuffer);
		MRELEASE(outputBuffer);
		return -1;

	case 1:
		fec_free(fec);
		MRELEASE(inputBuffer);
		return handleBulletin(outputBuffer, bufSize);

	default:
		break;
	}

#if DTKA_DEBUG
puts("First K blocks don't work.");
fflush(stdout);
#endif
	/*	At least one of the key authorities is compromised.	*/

	for (j = 0; j < DTKA_NUM_AUTHS; j++)
	{
		switch (tryAuths(fec, bulletin, inputBuffer, outputBuffer,
				bufSize, inputBlocks, outputBlocks, j, -1))
		{
		case -1:
			putErrmsg("Failure decoding bulletin.", NULL);
			fec_free(fec);
			MRELEASE(inputBuffer);
			MRELEASE(outputBuffer);
			return -1;

		case 1:
			writeMemoNote("[?] Compromised DTKA authority",
					itoa(db->authorities[j].nodeNbr));
			fec_free(fec);
			MRELEASE(inputBuffer);
			return handleBulletin(outputBuffer, bufSize);

		default:
			break;			/*	Switch		*/
		}
	}

#if DTKA_DEBUG
puts("Can't be just one compromised authority.");
fflush(stdout);
#endif
	/*	At least two of the key authorities are compromised.	*/

	for (j = 0; j < DTKA_NUM_AUTHS; j++)
	{
		for (k = 0; k < DTKA_NUM_AUTHS; k++)
		{
			if (k == j)
			{
				continue;
			}

			switch (tryAuths(fec, bulletin, inputBuffer,
					outputBuffer, bufSize,
					inputBlocks, outputBlocks, j, k))
			{
			case -1:
				putErrmsg("Failure decoding bulletin.", NULL);
				fec_free(fec);
				MRELEASE(inputBuffer);
				MRELEASE(outputBuffer);
				return -1;

			case 1:
				writeMemoNote("[?] Compromised DTKA authority",
					itoa(db->authorities[j].nodeNbr));
				writeMemoNote("[?] Compromised DTKA authority",
					itoa(db->authorities[k].nodeNbr));
				fec_free(fec);
				MRELEASE(inputBuffer);
				return handleBulletin(outputBuffer, bufSize);

			default:
				break;		/*	Switch		*/
			}
		}
	}

#if DTKA_DEBUG
puts("Can't be just two compromised authorities.");
fflush(stdout);
#endif
	/*	Need more blocks from non-compromised key authorities.	*/

	fec_free(fec);
	MRELEASE(inputBuffer);
	MRELEASE(outputBuffer);
	return 0;
}

static int	acquireBlock(Sdr sdr, Object dbobj, DtkaNodeDB *db, char *src,
			Object adu)
{
	int		parsedOkay;
	MetaEid		metaEid;
	VScheme		*vscheme;
	PsmAddress	schemeElt;
	DtkaAuthority	*auth;
	int		i;
	vast		aduLength;
	size_t		blksize;
	int		blockIdx = -1;
	ZcoReader	reader;
	int		len;
	DtkaBlockHeader	header;
	Object		bulletinElt;
	Object		bulletinObj;
	DtkaBulletin	bulletin;
	DtkaShare	*share;
	DtkaBlock	*block;
	int		reconstructionResult;
	int		j;

	parsedOkay = parseEidString(src, &metaEid, &vscheme, &schemeElt);
	if (!parsedOkay)
	{
		writeMemoNote("[?] Can't parse source of bulletin block", src);
		return 0;
	}

	for (auth = db->authorities, i = 0; i < DTKA_NUM_AUTHS; auth++, i++)
	{
		if (metaEid.nodeNbr == auth->nodeNbr)
		{
			break;
		}
	}

	restoreEidString(&metaEid);
	if (i == DTKA_NUM_AUTHS)
	{
		writeMemoNote("[?] Bulletin block from non-authority", src);
		return 0;
	}

	aduLength = zco_source_data_length(sdr, adu);
	if (aduLength <= 40)
	{
		len = aduLength;
		writeMemoNote("[?] DTKA bulletin block too small", itoa(len));
		return 0;
	}

	blksize = aduLength - 40;
	zco_start_receiving(adu, &reader);
	len = zco_receive_source(sdr, &reader, 4, (char *) &header.timestamp);
	header.timestamp = ntohl(header.timestamp);
	if (header.timestamp <= db->lastBulletinTime)
	{
		return 0;	/*	Late-arriving block.		*/
	}

	len = zco_receive_source(sdr, &reader, 32, (char *) header.hash);
	len = zco_receive_source(sdr, &reader, 4, (char *) &header.sharenum);
	header.sharenum = ntohl(header.sharenum);
#if DTKA_DEBUG
printf("knmgr received block for share #%d.\n", header.sharenum);
fflush(stdout);
if (header.sharenum >= DTKA_FEC_K)
{
	printBlockText(adu, 40, blksize);
}
#endif
	if (header.sharenum >= auth->firstPrimaryShare
	&& header.sharenum <= auth->lastPrimaryShare)
	{
		blockIdx = DTKA_PRIMARY;
	}
	else
	{
		if (header.sharenum >= auth->firstBackupShare
		&& header.sharenum <= auth->lastBackupShare)
		{
			blockIdx = DTKA_BACKUP;
		}
	}

	if (blockIdx < 0)
	{
#if DTKA_DEBUG
printf("Block %d out of range for authority %d.  Authority node nbr "
UVAST_FIELDSPEC ", primary %d - %d, backup %d - %d.\n", header.sharenum, i,
auth->nodeNbr, auth->firstPrimaryShare, auth->lastPrimaryShare,
auth->firstBackupShare, auth->lastBackupShare);
fflush(stdout);
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
	share = bulletin.shares + header.sharenum;
	block = share->blocks + blockIdx;
	if (share->blocksAnnounced == 0)	/*	1st block.	*/
	{
		bulletin.sharesAnnounced += 1;
#if DTKA_DEBUG
printf("Inserting first block for share %d, idx=%d, announced by authority \
%d.\n", header.sharenum, blockIdx, i);
fflush(stdout);
#endif
	}
	else	/*	Not the first block received for this share.	*/
	{
		if (block->text != 0)		/*	Duplicate.	*/
		{
#if DTKA_DEBUG
puts("Duplicates a previously received block.");
fflush(stdout);
#endif
			return 0;
		}
#if DTKA_DEBUG
else
{
printf("Inserting other block for share %d, idx=%d, announced by authority \
%d.\n", header.sharenum, blockIdx, i);
fflush(stdout);
}
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

	share->blocksAnnounced += 1;
	sdr_write(sdr, bulletinObj, (char *) &bulletin, sizeof(DtkaBulletin));
	if (bulletin.sharesAnnounced < DTKA_FEC_K)
	{
#if dTKA_DEBUG
printf("Bulletin can't be complete yet, sharesAnnounced = %u.\n",
bulletin.sharesAnnounced);
fflush(stdout);
#endif
		return 0;	/*	Bulletin can't be complete.	*/
	}

	/*	Must check to see if the entire bulletin has arrived.	*/

	reconstructionResult = reconstructBulletin(db, &bulletin);
	if (reconstructionResult < 1)
	{
		return reconstructionResult;
	}

	/*	Bulletin was reconstructed and handled successfully.
	 *	Now delete it.						*/

	sdr_stage(sdr, (char *) db, dbobj, sizeof(DtkaNodeDB));
	db->lastBulletinTime = bulletin.timestamp;
	sdr_write(sdr, dbobj, (char *) db, sizeof(DtkaNodeDB));
	sdr_list_delete(sdr, bulletinElt, NULL, NULL);
	for (i = 0, share = bulletin.shares; i < DTKA_FEC_M; i++, share++)
	{
		for (j = 0, block = share->blocks; j < 2; j++, block++)
		{
			if (block->text)
			{
				zco_destroy(sdr, block->text);
			}
		}
	}

	sdr_free(sdr, bulletinObj);
	return 0;
}

#if defined (VXWORKS) || defined (RTEMS) || defined (bionic)
int	knmgr(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
#else
int	main(int argc, char *argv[])
{
#endif
	char		ownEid[32];
	KmgrState	state = { NULL, 1 };
	Sdr		sdr;
	Object		dbobj;
	DtkaNodeDB	db;
	BpDelivery	dlv;

	if (knodeAttach() < 0)
	{
		putErrmsg("knmgr can't attach to dtka.", NULL);
		return 1;
	}

	/*	TODO: devise a way to make sure there is never
	 *	more than one knmgr daemon running in any node
	 *	at any moment.						*/

	isprintf(ownEid, sizeof ownEid, "imc:%d.0", DTKA_ANNOUNCE);
	if (bp_open(ownEid, &state.sap) < 0)
	{
		putErrmsg("Can't open own endpoint.", ownEid);
		ionDetach();
		return 1;
	}

	sdr = getIonsdr();
	dbobj = getKnodeDbObject();
	if (dbobj == 0)
	{
		putErrmsg("No DTKA node database.", NULL);
		ionDetach();
		return 1;
	}

	sdr_read(sdr, (char *) &db, dbobj, sizeof(DtkaNodeDB));

	/*	Main loop: receive block, try to build bulletin.	*/

	oK(_knmgrState(&state));
	isignal(SIGTERM, shutDown);
	writeMemo("[i] knmgr is running.");
	while (state.running)
	{
		if (bp_receive(state.sap, &dlv, BP_BLOCKING) < 0)
		{
			putErrmsg("knmgr bundle reception failed.", NULL);
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
			if (acquireBlock(sdr, dbobj, &db, dlv.bundleSourceEid,
					dlv.adu) < 0)
			{
				putErrmsg("Can't acquire block.", NULL);
				sdr_cancel_xn(sdr);
			}

			if (sdr_end_xn(sdr) < 0)
			{
				putErrmsg("Can't handle DTKA block.", NULL);
				state.running = 0;
				continue;
			}
		}

		bp_release_delivery(&dlv, 1);
	}

	bp_close(state.sap);
	writeErrmsgMemos();
	writeMemo("[i] knmgr has ended.");
	ionDetach();
	return 0;
}
