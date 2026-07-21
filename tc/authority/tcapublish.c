/*
	tcapublish.c:	authority daemon for publication of Trusted
			Collective bulletins.

	Author: Scott Burleigh, JPL

	Copyright (c) 2020, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.

									*/
#include "tcaP.h"
#include "crypto.h"

#include "ion_fec.h"

typedef struct
{
	BpSAP	recvSAP;
	int	running;
} TcaPublishState;

static TcaPublishState	*_tcapublishState(TcaPublishState *newState)
{
	void		*value;
	TcaPublishState	*state;

	if (newState)			/*	Add task variable.	*/
	{
		value = (void *) (newState);
		state = (TcaPublishState *) sm_TaskVar(&value);
	}
	else				/*	Retrieve task variable.	*/
	{
		state = (TcaPublishState *) sm_TaskVar(NULL);
	}

	return state;
}

static void	shutDown(int signum)	/*	Commands tcapublish shutdown.	*/
{
	TcaPublishState	*state;

	/* Tell the compiler the parameter is intentionally unused. */
	(void)signum;

	isignal(SIGTERM, shutDown);
	writeMemo("tcapublish: TCA publisher daemon interrupted.");
	state = _tcapublishState(NULL);
	bp_interrupt(state->recvSAP);
	state->running = 0;
}

static SdrObject nextPendingRecord(Sdr sdr, SdrObject elt, SdrObject *obj,
		TcaRecord *record)
{
	elt = sdr_list_next(sdr, elt);
	if (elt)
	{
		*obj = sdr_list_data(sdr, elt);
		sdr_stage(sdr, (char *) record, *obj, sizeof(TcaRecord));
	}

	return elt;
}

static int	handleProposedBulletin(Sdr sdr, TcaDB *db, char *src,
			SdrObject adu)
{
	int		parsedOkay;
	MetaEid		metaEid;
#if TC_DEBUG
char	nbrBuf[FQN_MAX_LENGTH];
#endif
	VScheme		*vscheme;
	PsmAddress	schemeElt;
	char		msgBuffer[256];
	int		auths;
	SdrObject	elt;
	int		i;		/*	Authority array index.	*/
	SdrObject	authObj;
	TcaAuthority	auth;
	ZcoReader	reader;
	int		bulletinLength;
	uint32_t	bulletinId;
	uint32_t	currentBulletinId;
	char		timestamp1[TIMESTAMPBUFSZ];
	char		timestamp2[TIMESTAMPBUFSZ];
	SdrObject	obj;
	TcaRecord	record;
	char		*acknowledged;
	int		bytesRemaining;
	int		bytesBuffered;
	int		residualBytesBuffered;
	char		buffer[TC_MAX_REC];
	char		*cursor;
	int		len;
	int		recordLength;
	uvast		fqnn;
	time_t		effectiveTime;
	time_t		assertionTime;
	unsigned short	datLength;
	unsigned char	datValue[TC_MAX_DATLEN];
	uvast		priorFqnn = 0;
	time_t		priorEffTime = 0;

	parsedOkay = parseEidString(src, &metaEid, &vscheme, &schemeElt);
	if (!parsedOkay)
	{
#if TC_DEBUG
		isprintf(msgBuffer, sizeof msgBuffer, "tcapublish: Can't parse \
source of proposed bulletin: '%s'.", src);
		writeMemo(msgBuffer);
#endif
		return 0;
	}

	if (metaEid.elementNbr == getOwnFqnn())
	{
		/*	This is loopback multicast, which we ignore.	*/

		return 0;
	}

#if TC_DEBUG
putFqn(nbrBuf, metaEid.elementNbr);
writeMemoNote("tcapublish: Got proposed bulletin from node", nbrBuf);
#endif
	/*	Determine sending authority's position within array.	*/

	auths = sdr_list_length(sdr, db->authorities);
	for (elt = sdr_list_first(sdr, db->authorities), i = 0; elt;
			elt = sdr_list_next(sdr, elt), i++)
	{
		authObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &auth, authObj, sizeof(TcaAuthority));
		if (metaEid.elementNbr == auth.fqnn)
		{
			break;
		}
	}

	clearMetaEid(&metaEid);
	if (elt == 0)
	{
#if TC_DEBUG
isprintf(msgBuffer, sizeof msgBuffer, "tcapublish: Bulletin from unknown \
authority: '%s'.", src);
writeMemo(msgBuffer);
#endif
		return 0;
	}

	if (!auth.inService)
	{
		isprintf(msgBuffer, sizeof msgBuffer, "tcapublish: Bulletin \
from out-of-svc authority: '%s'.", src);
		writeMemo(msgBuffer);
		return 0;
	}

	/*	Make sure bulletin is for current compilation.		*/

	bulletinLength = zco_source_data_length(sdr, adu);
	if (bulletinLength < 4)
	{
		isprintf(msgBuffer, sizeof msgBuffer, "tcapublish: TCA \
bulletin ID missing: %d.", bulletinLength);
		writeMemo(msgBuffer);
		return 0;
	}

	zco_start_receiving(adu, &reader);
	len = zco_receive_source(sdr, &reader, 4, (char *) &bulletinId);
	bulletinId = ntohl(bulletinId);
	currentBulletinId = db->currentCompilationTime;
	if (bulletinId != currentBulletinId)
	{
		writeTimestampUTC(bulletinId, timestamp1);
		writeTimestampUTC(db->currentCompilationTime, timestamp2);
		isprintf(msgBuffer, sizeof msgBuffer, "tcapublish: TCA \
bulletin ID incorrect: '%s', s/b '%s'.", timestamp1, timestamp2);
		writeMemo(msgBuffer);
		return 0;
	}

	if (bulletinLength == 4)
	{
#ifdef TC_DEBUG
		writeTimestampUTC(bulletinId, timestamp1);
		isprintf(msgBuffer, sizeof msgBuffer, "tcapublish: TCA \
bulletin only contains bulletin ID: %s.", timestamp1);
		writeMemo(msgBuffer);
#endif
		return 0;
	}

	/*	Match records in bulletin against pending records.	*/

	acknowledged = MTAKE(auths);
	if (acknowledged == NULL)
	{
		putErrmsg("No memory for acknowledged array.", itoa(auths));
		return -1;
	}

	elt = sdr_list_first(sdr, db->pendingRecords);
	if (elt)
	{
		obj = sdr_list_data(sdr, elt);
		sdr_stage(sdr, (char *) &record, obj, sizeof(TcaRecord));
	}

	bytesBuffered = 0;
	residualBytesBuffered = 0;
	bytesRemaining = bulletinLength - 4;
	while (bytesRemaining > 0)
	{
		bytesBuffered += zco_receive_source(sdr, &reader,
				TC_MAX_REC - residualBytesBuffered,
				(char *) (buffer + residualBytesBuffered));
		cursor = buffer;
		len = bytesBuffered;
		recordLength = tc_deserialize(&cursor, &len, TC_MAX_DATLEN,
				&fqnn, &effectiveTime, &assertionTime,
				&datLength, datValue);
		if (recordLength == 0)
		{
			writeMemo("tcapublish: Malformed proposed bulletin.");
			MRELEASE(acknowledged);
			return 0;
		}

#if TC_DEBUG
putFqn(nbrBuf, fqnn);
writeMemoNote("tcapublish: Got record for node", nbrBuf);
#endif
		/*	Check record order in bulletin.			*/

		if (fqnn < priorFqnn
		|| (fqnn == priorFqnn && (effectiveTime < priorEffTime)))
		{
			isprintf(msgBuffer, sizeof msgBuffer, "tcapublish: \
Malformed bulletin (order): '%s'.", src);
			writeMemo(msgBuffer);
			MRELEASE(acknowledged);
			return 0;
		}

		/*	Find matching Pending record, if any.		*/

		for (; elt; elt = nextPendingRecord(sdr, elt, &obj, &record))
		{
			if (record.fqnn < fqnn)
			{
				continue;
			}

			if (record.fqnn > fqnn)
			{
				break;
			}

			if (record.effectiveTime < effectiveTime)
			{
				continue;
			}

			if (record.effectiveTime > effectiveTime)
			{
				break;
			}

#if TC_DEBUG
writeMemo("tcapublish: Found matching record.");
#endif
			/*	Remote authority has got the same
			 *	record in its pendingRecords list.	*/

			sdr_stage(sdr, acknowledged, record.acknowledged, auths);
			if (record.assertionTime != assertionTime
			|| record.datLength != datLength
			|| memcmp(record.datValue, datValue, datLength) != 0)
			{
				/*	Disagreement on asserted data.	*/

				acknowledged[i] = -1;
#if TC_DEBUG
writeMemo("tcapublish: Disagree on data.");
#endif
			}
			else	/*	Advancing toward consensus.	*/
			{
				acknowledged[i] = 1;
#if TC_DEBUG
writeMemo("tcapublish: Agree on data.");
#endif
			}

			sdr_write(sdr, record.acknowledged, acknowledged,
					auths);
		}

		/*	Now consider the next record in the bulletin.	*/

		priorFqnn = fqnn;
		priorEffTime = effectiveTime;

		/*	Move any residual buffered bytes to the front
		 *	of the buffer, to be processed as the start
		 *	of the next record.				*/

		bytesBuffered -= recordLength;
		residualBytesBuffered = bytesBuffered;
		if (residualBytesBuffered > 0)
		{
			memmove(buffer, buffer + recordLength,
					residualBytesBuffered);
		}

		bytesRemaining -= recordLength;
	}

	MRELEASE(acknowledged);
	return 0;
}

static void	noteNoConsensus(TcaDB *db, TcaRecord *rec)
{
	Sdr	sdr = getIonsdr();
	int	auths;
	char	*acknowledged;
	char	nbrBuf[FQN_MAX_LENGTH];
	char	msgbuf[3072];
	char	*cursor = msgbuf;
	int	bytesRemaining = sizeof msgbuf;
	int	i;
	int	len;

	putFqn(nbrBuf, rec->fqnn);
	len = _isprintf(__FILE__, __LINE__, cursor, bytesRemaining, "%s %lu %lu ", nbrBuf,
			rec->assertionTime, rec->effectiveTime);
	cursor += len;
	bytesRemaining -= len;
	if (rec->datLength == 0)
	{
		istrcpy(cursor, "[revoke]", bytesRemaining);
		cursor += 8;
		bytesRemaining -= 8;
	}
	else
	{
		for (i = 0; i < rec->datLength; i++)
		{
			len = _isprintf(__FILE__, __LINE__, cursor, bytesRemaining, "%02x",
					rec->datValue[i]);
			cursor += len;
			bytesRemaining -= len;
		}
	}

	auths = sdr_list_length(sdr, db->authorities);
	acknowledged = MTAKE(auths);
	CHKVOID(acknowledged);
	sdr_read(sdr, acknowledged, rec->acknowledged, auths);
	for (i = 0; i < auths; i++)
	{
		len = _isprintf(__FILE__, __LINE__, cursor, bytesRemaining, "  %d:%d", i,
				acknowledged[i]);
		cursor += len;
		bytesRemaining -= len;
	}

	MRELEASE(acknowledged);
	writeMemo(msgbuf);
}

static int	publishConsensusBulletin(Sdr sdr, TcaDB *db, BpSAP sap)
{
	char		nbrBuf[FQN_MAX_LENGTH];
	char		destEid[32];
	SdrObject	elt;
	int		i;
	SdrObject	authObj;
	TcaAuthority	auth;
	char		msgbuf[384];
	int		auths;
	int		fec_x;
	int		j;
	unsigned int	*secondaryBlockNbrs;
	int		buflen;
	char		*bulletin;
	char		*cursor;
	unsigned int	bytesRemaining;
	char		*acknowledged;
	SdrObject	elt2;
	SdrObject	nextElt;
	SdrObject	obj;
			OBJ_POINTER(TcaRecord, rec);
	int		recCount = 0;
	int		recLen;
	int		bulletinLen = 0;
	int		blksize;
	char		*buffer;
	char		**primaryBlocks;
	char		**secondaryBlocks;
	unsigned char	hash[32];
	ion_fec_t	*fec;
	unsigned int	sharenum;
	unsigned int	*sharenums;
	unsigned int	u4;
	SdrObject	zco;
	SdrObject	newBundle;
#if TC_DEBUG
char	bytes[256];
char	*byte;
int	n;
#endif

	putFqn(nbrBuf, db->blocksGroupNbr);
	isprintf(destEid, 32, "imc:%s.0", nbrBuf);
	fec_x = auths = sdr_list_length(sdr, db->authorities);
	writeMemo("tcapublish: ---Consensus bulletin report---");
	writeMemo("tcapublish: Authorities:");
	for (elt = sdr_list_first(sdr, db->authorities), i = 0; elt;
			elt = sdr_list_next(sdr, elt), i++)
	{
		authObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &auth, authObj, sizeof(TcaAuthority));
		putFqn(nbrBuf, auth.fqnn);
		isprintf(msgbuf, sizeof msgbuf, "\t%d\t%s\t%u", i, nbrBuf,
				auth.inService);
		writeMemo(msgbuf);
		if (auth.fqnn == getOwnFqnn())
		{
			fec_x = i;
		}
	}

	if (fec_x == auths)
	{
		putFqn(nbrBuf, getOwnFqnn());
		isprintf(msgbuf, sizeof msgbuf, "tcapublish: Can't send \
bulletin: not a declared authority -- %s", nbrBuf);
		writeMemo(msgbuf);
		return 0;
	}

	secondaryBlockNbrs = (unsigned int *) MTAKE(sizeof(int)
			* (db->fec_M - db->fec_K));
	primaryBlocks = (char **) MTAKE(sizeof(char *) * (db->fec_K));
	secondaryBlocks = (char **) MTAKE(sizeof(char *)
			* (db->fec_M - db->fec_K));
	sharenums = (unsigned int *) MTAKE(sizeof(unsigned int) * (db->fec_Q));
	if (secondaryBlockNbrs == NULL
	|| primaryBlocks == NULL
	|| secondaryBlocks == NULL
	|| sharenums == NULL)
	{
		writeMemo("tcapublish: Can't allocate arrays for bulletin \
publication.");
		return -1;
	}

	for (i = 0, j = db->fec_K; j < db->fec_M; i++, j++)
	{
		secondaryBlockNbrs[i] = j;
	}

	buflen = sdr_list_length(sdr, db->pendingRecords) * TC_MAX_REC;
	if (buflen == 0)
	{
#if TC_DEBUG
writeMemo("tcapublish: No records to publish.");
#endif
		MRELEASE(secondaryBlockNbrs);
		MRELEASE(primaryBlocks);
		MRELEASE(secondaryBlocks);
		MRELEASE(sharenums);
		return 0;
	}

	bulletin = MTAKE(buflen);
	if (bulletin == NULL)
	{
		putErrmsg("Not enough memory for consensus bulletin.",
				utoa(buflen));
		MRELEASE(secondaryBlockNbrs);
		MRELEASE(primaryBlocks);
		MRELEASE(secondaryBlocks);
		MRELEASE(sharenums);
		return -1;
	}

	acknowledged = MTAKE(auths);
	if (acknowledged == NULL)
	{
		putErrmsg("Not enough memory for acknowledged array.",
				itoa(auths));
		MRELEASE(secondaryBlockNbrs);
		MRELEASE(primaryBlocks);
		MRELEASE(secondaryBlocks);
		MRELEASE(sharenums);
		return -1;
	}

	writeMemo("tcapublish: No consensus on these records...");
	cursor = bulletin;
	bytesRemaining = buflen;
	for (elt = sdr_list_first(sdr, db->pendingRecords); elt; elt = nextElt)
	{
		nextElt = sdr_list_next(sdr, elt);
		obj = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, TcaRecord, rec, obj);
		sdr_read(sdr, acknowledged, rec->acknowledged, auths);
		for (elt2 = sdr_list_first(sdr, db->authorities), i = 0; elt2;
				elt2 = sdr_list_next(sdr, elt2), i++)
		{
			authObj = sdr_list_data(sdr, elt2);
			sdr_read(sdr, (char *) &auth, authObj,
					sizeof(TcaAuthority));
			if (auth.inService == 0)
			{
				continue;
			}

			if (acknowledged[i] == 1)
			{
				continue;
			}

			noteNoConsensus(db, rec);
			break;	/*	No consensus on this record.	*/
		}

		if (elt2)	/*	No consensus.			*/
		{
			/*	TODO: insert this record into the
			 *	db->currentRecords list for research
			 *	and future consideration.		*/

			sdr_free(sdr, obj);
			sdr_list_delete(sdr, elt, NULL, NULL);
			continue;	/*	Omit from bulletin.	*/
		}

		/*	Consensus; add record to bulletin.		*/

		recCount++;
		if (db->hijacked)
		{
			rec->effectiveTime = 0;
		}
#if TC_DEBUG
putFqn(nbrBuf, rec->fqnn);
isprintf(msgbuf, sizeof msgbuf, "tcapublish: Appending record to bulletin \
for node %s, effective time %lu.", nbrBuf, rec->effectiveTime);
writeMemo(msgbuf);
#endif
		recLen = tc_serialize(cursor, bytesRemaining, rec->fqnn,
				rec->effectiveTime, rec->assertionTime,
				rec->datLength, rec->datValue);
		if (recLen < 0)
		{
			putErrmsg("Can't serialize record.", NULL);
			MRELEASE(acknowledged);
			MRELEASE(bulletin);
			MRELEASE(secondaryBlockNbrs);
			MRELEASE(primaryBlocks);
			MRELEASE(secondaryBlocks);
			MRELEASE(sharenums);
			return -1;
		}

		cursor += recLen;
		bytesRemaining -= recLen;
		bulletinLen += recLen;

		/*	Remove from list of pending records.		*/

		sdr_free(sdr, obj);
		sdr_list_delete(sdr, elt, NULL, NULL);
	}

	writeMemo("tcapublish: ...consensus reached on all other records.");
#if TC_DEBUG
for (byte = bulletin, n = 0; n < bulletinLen && n < (int)((sizeof bytes / 2) - 1); byte++, n++)
{
	isprintf(bytes + (n * 2), sizeof bytes - (n * 2), "%02x", (unsigned char) *byte);
}
bytes[n * 2] = '\0'; /* Guarantee termination if loop skips (bulletinLen == 0) */

isprintf(msgbuf, sizeof msgbuf, "tcapublish: Bulletin '%s'", bytes);
writeMemo(msgbuf);
#endif

	MRELEASE(acknowledged);
	isprintf(msgbuf, sizeof msgbuf, "tcapublish: Number of records in \
consensus: %d", recCount);
	writeMemo(msgbuf);
	writeMemo("tcapublish: ---End of consensus bulletin report---");
	if (recCount == 0)
	{
		MRELEASE(bulletin);
		MRELEASE(secondaryBlockNbrs);
		MRELEASE(primaryBlocks);
		MRELEASE(secondaryBlocks);
		MRELEASE(sharenums);
		return 1;
	}

	blksize = (bulletinLen / db->fec_K)
			+ (bulletinLen % db->fec_K > 0 ? 1 : 0);
	buflen = blksize * db->fec_M;
	buffer = MTAKE(buflen);
	if (buffer == NULL)
	{
		putErrmsg("Not enough memory for erasure coding buffer.",
				utoa(buflen));
		MRELEASE(bulletin);
		MRELEASE(secondaryBlockNbrs);
		MRELEASE(primaryBlocks);
		MRELEASE(secondaryBlocks);
		MRELEASE(sharenums);
		return -1;
	}

	memset(buffer, 0, buflen);
	memcpy(buffer, bulletin, bulletinLen);
#if TC_DEBUG
for (byte = buffer, n = 0; n < buflen && n < (int)((sizeof bytes / 2) - 1); byte++, n++)
{
	isprintf(bytes + (n * 2), sizeof bytes - (n * 2), "%02x", (unsigned char) *byte);
}
bytes[n * 2] = '\0'; /* Prevent stale data leakage if loop skips (buflen == 0) */

isprintf(msgbuf, sizeof msgbuf, "tcapublish: Buffer '%s'", bytes);
writeMemo(msgbuf);
#endif

	MRELEASE(bulletin);

	/*	We initialize the hash output to a dummy value in
	 *	case we are running with NULL_SUITES crypto, which
	 *	cannot compute an actual hash.				*/

	memcpy(hash, buffer, 16);
	memcpy(hash + 16, buffer, 16);
	sha2((unsigned char *) buffer, db->fec_K * blksize, hash, 0);
	cursor = buffer;
	for (i = 0; i < db->fec_K; i++)
	{
		primaryBlocks[i] = cursor;
		cursor += blksize;
	}

	for (i = 0; i < (db->fec_M - db->fec_K); i++)
	{
		secondaryBlocks[i] = cursor;
		cursor += blksize;
	}

	fec = ion_fec_new(db->fec_K, db->fec_M);
	if (fec == NULL)
	{
		putErrmsg("Not enough memory for fec encoder.", NULL);
		MRELEASE(buffer);
		MRELEASE(secondaryBlockNbrs);
		MRELEASE(primaryBlocks);
		MRELEASE(secondaryBlocks);
		MRELEASE(sharenums);
		return -1;
	}

	ion_fec_encode(fec, (const unsigned char *const *) primaryBlocks,
			(unsigned char **) secondaryBlocks,
			secondaryBlockNbrs, db->fec_M - db->fec_K, blksize);
#if TC_DEBUG
writeMemoNote("tcapublish: Printing shares at", itoa(time(NULL)));
printf("Shares at %lu\n", time(NULL));
for (i = 0; i < db->fec_K; i++)
{
	printf("Primary block share number %d:\n", i);
	for (j = 0; j < blksize; j++)
	{
		printf("%02x", primaryBlocks[i][j]);
	}

	putchar('\n');
}

for (i = 0; i < (db->fec_M - db->fec_K); i++)
{
	printf("Parity block share %d:\n", i);
	for (j = 0; j < blksize; j++)
	{
		printf("%02x", secondaryBlocks[i][j]);
	}

	putchar('\n');
}
#endif
	sharenum = (db->fec_Q / 2) * fec_x;
	for (i = 0; i < db->fec_Q; i++)
	{
		sharenums[i] = sharenum;
		sharenum++;
		if (db->fec_M >= 0 && sharenum == (unsigned int)db->fec_M)
		{
			sharenum = 0;
		}
	}

	for (i = 0; i < db->fec_Q; i++)
	{
		obj = sdr_malloc(sdr, 40 + blksize);
		if (obj == 0)
		{
			putErrmsg("Not enough heap space for block ZCO.", NULL);
			ion_fec_free(fec);
			MRELEASE(buffer);
			MRELEASE(secondaryBlockNbrs);
			MRELEASE(primaryBlocks);
			MRELEASE(secondaryBlocks);
			MRELEASE(sharenums);
			return -1;
		}

		u4 = (unsigned int) db->currentCompilationTime;
		u4 = htonl(u4);
		sdr_write(sdr, obj, (char *) &u4, 4);
		sdr_write(sdr, obj + 4, (char *) hash, 32);
		sharenum = sharenums[i];
		u4 = htonl(sharenum);
		sdr_write(sdr, obj + 36, (char *) &u4, 4);
		sdr_write(sdr, obj + 40,
			(char *) (buffer + (sharenum * blksize)), blksize);
		zco = ionCreateZco(ZcoSdrSource, obj, 0, 40 + blksize,
				BP_STD_PRIORITY, 0, ZcoOutbound, NULL);
		if (zco == 0 || zco == (SdrObject) -1)
		{
			putErrmsg("Can't create block ZCO.", NULL);
			ion_fec_free(fec);
			MRELEASE(buffer);
			MRELEASE(secondaryBlockNbrs);
			MRELEASE(primaryBlocks);
			MRELEASE(secondaryBlocks);
			MRELEASE(sharenums);
			return -1;
		}
#if TC_DEBUG
writeMemoNote("tcapublish: Sending block for share number", itoa(sharenum));
#endif
		if (bp_send(sap, destEid, NULL, 360000, BP_STD_PRIORITY,
			NoCustodyRequested, 0, 0, NULL, zco, &newBundle) < 0)
		{
			putErrmsg("Failed publishing bulletin block.", NULL);
		}
	}

	ion_fec_free(fec);
	MRELEASE(buffer);
	MRELEASE(secondaryBlockNbrs);
	MRELEASE(primaryBlocks);
	MRELEASE(secondaryBlocks);
	MRELEASE(sharenums);
	return 0;
}

#if defined (ION_LWT)
int	tcapublish(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	int	blocksGroupNbr = (a1 ? atoi((char *) a1) : -1);
#else
int	main(int argc, char *argv[])
{
	int	blocksGroupNbr = (argc > 1 ? atoi(argv[1]) : -1);
#endif
	char		nbrBuf[FQN_MAX_LENGTH];
	char		ownEid[32];
	TcaPublishState	state = { NULL, 1 };
	BpSAP		sendSAP;
	Sdr		sdr;
	SdrObject	dbobj;
	TcaDB		db;
	time_t		currentTime;
	int		interval;
	BpDelivery	dlv;
	int		result;

	if (blocksGroupNbr < 1)
	{
		puts("Usage: tcapublish <IMC group number for TC blocks>");
		return -1;
	}

	if (tcaAttach(blocksGroupNbr) < 0)
	{
		putErrmsg("tcapublish can't attach to tca.", NULL);
		return 1;
	}

	sdr = getIonsdr();
	dbobj = getTcaDBObject(blocksGroupNbr);
	if (dbobj == 0)
	{
		putErrmsg("No TCA authority database.", NULL);
		ionDetach();
		return 1;
	}

	sdr_read(sdr, (char *) &db, dbobj, sizeof(TcaDB));
	isprintf(ownEid, sizeof ownEid, "imc:%d.0", db.bulletinsGroupNbr);
	if (bp_open(ownEid, &state.recvSAP) < 0)
	{
		putErrmsg("Can't open own reception endpoint.", ownEid);
		ionDetach();
		return 1;
	}

	putFqn(nbrBuf, getOwnFqnn());
	isprintf(ownEid, sizeof ownEid, "ipn:%s.0", nbrBuf);
	if (bp_open_source(ownEid, &sendSAP, 0) < 0)
	{
		putErrmsg("Can't open own transmission endpoint.", ownEid);
		ionDetach();
		return 1;
	}

	/*	Main loop: receive proposed bulletins, mark toward
	 *	consensus, until consensus interval ends.		*/

	oK(_tcapublishState(&state));
	isignal(SIGTERM, shutDown);
	writeMemo("[i] tcapublish is running.");
	while (state.running)
	{
		currentTime = getCtime();
		interval = (db.currentCompilationTime + db.consensusInterval)
				- currentTime;
		if (interval <= 0)
		{
#if TC_DEBUG
writeMemo("tcapublish: consensus grace period has ended.");
#endif
			CHKZERO(sdr_begin_xn(sdr));
			if (publishConsensusBulletin(sdr, &db, sendSAP) < 0
			|| sdr_end_xn(sdr) < 0)
			{
				putErrmsg("Failed publishing bulletin.", NULL);
			}

			state.running = 0;
			continue;
		}

		if (bp_receive(state.recvSAP, &dlv, interval) < 0)
		{
			putErrmsg("tcapublish bulletin reception failed.",
					NULL);
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
			result = handleProposedBulletin(sdr, &db,
					dlv.bundleSourceEid, dlv.adu);
			bp_release_delivery(&dlv, 1);
		       	if (result < 0)
			{
				putErrmsg("Can't handle proposed bulletin.",
						NULL);
				sdr_cancel_xn(sdr);
				state.running = 0;
				continue;
			}

			if (sdr_end_xn(sdr) < 0)
			{
				putErrmsg("Can't handle proposed bulletin.",
						NULL);
				state.running = 0;
				continue;
			}
		}

		bp_release_delivery(&dlv, 1);
	}

	bp_close(state.recvSAP);
	bp_close(sendSAP);
	writeErrmsgMemos();
	writeMemo("[i] tcapublish has ended.");
	ionDetach();
	return 0;
}
