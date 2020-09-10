/*
	kapublish.c:	Key authority daemon for reception of key
			records asserted by nodes.

	Author: Scott Burleigh, JPL

	Copyright (c) 2013, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include "kauth.h"
#include "bpP.h"

typedef struct
{
	BpSAP	recvSAP;
	int	running;
} KapublishState;

static KapublishState	*_kapublishState(KapublishState *newState)
{
	void		*value;
	KapublishState	*state;
	
	if (newState)			/*	Add task variable.	*/
	{
		value = (void *) (newState);
		state = (KapublishState *) sm_TaskVar(&value);
	}
	else				/*	Retrieve task variable.	*/
	{
		state = (KapublishState *) sm_TaskVar(NULL);
	}

	return state;
}

static void	shutDown()	/*	Commands kapublish termination.	*/
{
	KapublishState	*state;

	isignal(SIGTERM, shutDown);
	PUTS("DTKA publisher daemon interrupted.");
	state = _kapublishState(NULL);
	bp_interrupt(state->recvSAP);
	state->running = 0;
}

static Object	nextPendingRecord(Sdr sdr, Object elt, Object *obj,
			DtkaRecord *record)
{
	elt = sdr_list_next(sdr, elt);
	if (elt)
	{
		*obj = sdr_list_data(sdr, elt);
		sdr_stage(sdr, (char *) record, *obj, sizeof(DtkaRecord));
	}

	return elt;
}

static int	handleProposedBulletin(Sdr sdr, DtkaAuthDB *db, char *src,
			Object adu)
{
	int		parsedOkay;
	MetaEid		metaEid;
	VScheme		*vscheme;
	PsmAddress	schemeElt;
	char		msgBuffer[256];
	DtkaAuthority	*auth;
	int		i;		/*	Authority array index.	*/
	ZcoReader	reader;
	int		bulletinLength;
	time_t		bulletinId;
	char		timestamp[TIMESTAMPBUFSZ];
	Object		elt;
	Object		obj;
	DtkaRecord	record;
	int		bytesRemaining;
	int		bytesBuffered;
	int		residualBytesBuffered;
	unsigned char	buffer[DTKA_MAX_REC];
	unsigned char	*cursor;
	int		len;
	int		recordLength;
	uvast		nodeNbr;
	time_t	effectiveTime;
	time_t		assertionTime;
	unsigned short	datLength;
	unsigned char	datValue[DTKA_MAX_DATLEN];
	uvast		priorNodeNbr = 0;
	time_t		priorEffTime = 0;

	parsedOkay = parseEidString(src, &metaEid, &vscheme, &schemeElt);
	if (!parsedOkay)
	{
		isprintf(msgBuffer, sizeof msgBuffer, "Can't parse source \
of proposed bulletin: '%s'.", src);
		PUTS(msgBuffer);
		return 0;
	}

	if (metaEid.nodeNbr == getOwnNodeNbr())
	{
		/*	This is loopback multicast, which we ignore.	*/

		return 0;
	}

#if DTKA_DEBUG
printf("Got bulletin from node " UVAST_FIELDSPEC ".\n", metaEid.nodeNbr);
fflush(stdout);
#endif
	/*	Determine sending authority's position within array.	*/

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
		isprintf(msgBuffer, sizeof msgBuffer, "Bulletin from unknown \
authority: '%s'.", src);
		PUTS(msgBuffer);
		return 0;
	}

	if (!auth->inService)
	{
		isprintf(msgBuffer, sizeof msgBuffer, "Bulletin from \
out-of-svc authority: '%s'.", src);
		PUTS(msgBuffer);
		return 0;
	}

	/*	Make sure bulletin is for current compilation.		*/

	bulletinLength = zco_source_data_length(sdr, adu);
	if (bulletinLength < 4)
	{
		isprintf(msgBuffer, sizeof msgBuffer, "DTKA bulletin ID \
missing: %d.", bulletinLength);
		PUTS(msgBuffer);
		return 0;
	}

	zco_start_receiving(adu, &reader);
	len = zco_receive_source(sdr, &reader, 4, (char *) &bulletinId);
	bulletinId = ntohl(bulletinId);
	if (bulletinId != db->currentCompilationTime)
	{
		writeTimestampUTC(bulletinId, timestamp);
		isprintf(msgBuffer, sizeof msgBuffer, "DTKA bulletin ID \
incorrect: '%s'.", timestamp);
		PUTS(msgBuffer);
		return 0;
	}

	/*	Match records in bulletin against pending records.	*/

	elt = sdr_list_first(sdr, db->pendingRecords);
	if (elt)
	{
		obj = sdr_list_data(sdr, elt);
		sdr_stage(sdr, (char *) &record, obj, sizeof(DtkaRecord));
	}

	bytesBuffered = 0;
	residualBytesBuffered = 0;
	bytesRemaining = bulletinLength - 4;
	while (bytesRemaining > 0)
	{
		bytesBuffered += zco_receive_source(sdr, &reader,
				DTKA_MAX_REC - residualBytesBuffered,
				(char *) (buffer + residualBytesBuffered));
		cursor = buffer;
		len = bytesBuffered;
		recordLength = dtka_deserialize(&cursor, &len, DTKA_MAX_DATLEN, 
				&nodeNbr, &effectiveTime, &assertionTime,
				&datLength, datValue);
		if (recordLength < 1)
		{
			PUTS("Malformed proposed bulletin.");
			return 0;
		}

#if DTKA_DEBUG
printf("\tGot record for node " UVAST_FIELDSPEC ".\n", nodeNbr);
fflush(stdout);
#endif
		/*	Check record order in bulletin.			*/

		if (nodeNbr < priorNodeNbr
		|| (nodeNbr == priorNodeNbr
		    && (effectiveTime < priorEffTime)))
		{
			isprintf(msgBuffer, sizeof msgBuffer, "Malformed \
bulletin (order): '%s'.", src);
			PUTS(msgBuffer);
			return 0;
		}

		/*	Find matching Pending record, if any.		*/

		for (; elt; elt = nextPendingRecord(sdr, elt, &obj, &record))
		{
			if (record.nodeNbr < nodeNbr)
			{
				continue;
			}

			if (record.nodeNbr > nodeNbr)
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

#if DTKA_DEBUG
puts("\tFound matching record.");
fflush(stdout);
#endif
			/*	Remote authority has got the same
			 *	record in its pendingRecords list.	*/

			if (record.assertionTime != assertionTime
			|| record.datLength != datLength
			|| memcmp(record.datValue, datValue, datLength) != 0)
			{
				/*	Disagreement on asserted key.	*/

				record.acknowledged[i] = -1;
#if DTKA_DEBUG
puts("\t\tDisagree on key.");
fflush(stdout);
#endif
			}
			else	/*	Advancing toward consensus.	*/
			{
				record.acknowledged[i] = 1;
#if DTKA_DEBUG
puts("\t\tAgree on key.");
fflush(stdout);
#endif
			}

			sdr_write(sdr, obj, (char *) &record,
					sizeof(DtkaRecord));
		}

		/*	Now consider the next record in the bulletin.	*/

		priorNodeNbr = nodeNbr;
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

	return 0;
}

static void	noteNoConsensus(DtkaRecord *rec)
{
	char	msgbuf[3072];
	char	*cursor = msgbuf;
	int	bytesRemaining = sizeof msgbuf;
	int	i;
	int	len;

	len = _isprintf(cursor, bytesRemaining, UVAST_FIELDSPEC " %lu %lu ",
			rec->nodeNbr, rec->assertionTime,
			rec->effectiveTime);
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
			len = _isprintf(cursor, bytesRemaining, "%02x",
					rec->datValue[i]);
			cursor += len;
			bytesRemaining -= len;
		}
	}

	for (i = 0; i < DTKA_NUM_AUTHS; i++)
	{
		len = _isprintf(cursor, bytesRemaining, "  %d:%d", i,
				rec->acknowledged[i]);
		cursor += len;
		bytesRemaining -= len;
	}

	PUTS(msgbuf);
}

static int	publishConsensusBulletin(Sdr sdr, DtkaAuthDB *db, BpSAP sap)
{
	char		destEid[32];
	int		i;
	DtkaAuthority	*auth;
	char		msgbuf[72];
	int		dtka_fec_x = DTKA_NUM_AUTHS;
	int		j;
	unsigned int	secondaryBlockNbrs[DTKA_FEC_M - DTKA_FEC_K];
	int		buflen;
	unsigned char	*bulletin;
	unsigned char	*cursor;
	unsigned int	bytesRemaining;
	Object		elt;
	Object		nextElt;
	Object		obj;
			OBJ_POINTER(DtkaRecord, rec);
	int		recCount = 0;
	int		recLen;
	int		bulletinLen = 0;
	int		blksize;
	unsigned char	*buffer;
	unsigned char	*primaryBlocks[DTKA_FEC_K];
	unsigned char	*secondaryBlocks[DTKA_FEC_M - DTKA_FEC_K];
	unsigned char	hash[32];
	fec_t		*fec;
	unsigned int	sharenum;
	unsigned int	sharenums[DTKA_FEC_Q];
	unsigned int	u4;
	Object		zco;
	Object		newBundle;

	isprintf(destEid, 32, "imc:%d.0", DTKA_ANNOUNCE);
	PUTS("\n---Consensus bulletin report---");
	PUTS("Authorities:");
	for (auth = db->authorities, i = 0; i < DTKA_NUM_AUTHS; auth++, i++)
	{
		isprintf(msgbuf, sizeof msgbuf, "\t%d\t" UVAST_FIELDSPEC "\t%u",
				i, auth->nodeNbr, auth->inService);
		PUTS(msgbuf);
		if (auth->nodeNbr == getOwnNodeNbr())
		{
			dtka_fec_x = i;
		}
	}

	if (dtka_fec_x == DTKA_NUM_AUTHS)
	{
		isprintf(msgbuf, sizeof msgbuf, "Can't send bulletin: not a \
declared key authority -- " UVAST_FIELDSPEC, getOwnNodeNbr());
		PUTS(msgbuf);
		return 0;
	}

	for (i = 0, j = DTKA_FEC_K; j < DTKA_FEC_M; i++, j++)
	{
		secondaryBlockNbrs[i] = j;
	}

	buflen = sdr_list_length(sdr, db->pendingRecords) * DTKA_MAX_REC;
	if (buflen == 0)
	{
		PUTS("No records to publish.");
		return 0;
	}

	bulletin = MTAKE(buflen);
	if (bulletin == NULL)
	{
		putErrmsg("Not enough memory for consensus bulletin.",
				utoa(buflen));
		return -1;
	}

	PUTS("No consensus on these records:");
	cursor = bulletin;
	bytesRemaining = buflen;
	for (elt = sdr_list_first(sdr, db->pendingRecords); elt; elt = nextElt)
	{
		nextElt = sdr_list_next(sdr, elt);
		obj = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, DtkaRecord, rec, obj);
		for (i = 0; i < DTKA_NUM_AUTHS; i++)
		{
			if (db->authorities[i].inService == 0)
			{
				continue;
			}

			if (rec->acknowledged[i] == 1)
			{
				continue;
			}

			noteNoConsensus(rec);
			break;	/*	No consensus on this record.	*/
		}

		if (i < DTKA_NUM_AUTHS)
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

		recLen = dtka_serialize(cursor, bytesRemaining, rec->nodeNbr,
				&(rec->effectiveTime), rec->assertionTime,
				rec->datLength, rec->datValue);
		if (recLen < 0)
		{
			putErrmsg("Can't serialize record.", NULL);
			MRELEASE(bulletin);
			return -1;
		}

		cursor += recLen;
		bytesRemaining -= recLen;
		bulletinLen += recLen;

		/*	Remove from list of pending records.		*/

		sdr_free(sdr, obj);
		sdr_list_delete(sdr, elt, NULL, NULL);
	}

	isprintf(msgbuf, sizeof msgbuf, "Number of records in consensus: %d",
			recCount);
	PUTS(msgbuf);
	PUTS("---End of consensus bulletin report---");
	if (recCount == 0)
	{
		MRELEASE(bulletin);
		return 1;
	}

	blksize = (bulletinLen / DTKA_FEC_K)
			+ (bulletinLen % DTKA_FEC_K > 0 ? 1 : 0);
	buflen = blksize * DTKA_FEC_M;
	buffer = MTAKE(buflen);
	if (buffer == NULL)
	{
		putErrmsg("Not enough memory for erasure coding buffer.",
				utoa(buflen));
		MRELEASE(bulletin);
		return -1;
	}

	memset(buffer, 0, buflen);
	memcpy(buffer, bulletin, bulletinLen);
	MRELEASE(bulletin);
	sha2(buffer, DTKA_FEC_K * blksize, hash, 0);
	cursor = buffer;
	for (i = 0; i < DTKA_FEC_K; i++)
	{
		primaryBlocks[i] = cursor;
		cursor += blksize;
	}

	for (i = 0; i < (DTKA_FEC_M - DTKA_FEC_K); i++)
	{
		secondaryBlocks[i] = cursor;
		cursor += blksize;
	}

	fec = fec_new(DTKA_FEC_K, DTKA_FEC_M);
	if (fec == NULL)
	{
		putErrmsg("Not enough memory for fec encoder.", NULL);
		MRELEASE(buffer);
		return -1;
	}

	fec_encode(fec, primaryBlocks, secondaryBlocks, secondaryBlockNbrs,
			DTKA_FEC_M - DTKA_FEC_K, blksize);
#if DTKA_DEBUG
for (i = 0; i < DTKA_FEC_K; i++)
{
	printf("Primary block share number %d:\n", i);
	for (j = 0; j < blksize; j++)
	{
		printf("%02x", primaryBlocks[i][j]);
	}

	putchar('\n');
}

for (i = 0; i < (DTKA_FEC_M - DTKA_FEC_K); i++)
{
	printf("Parity block share %d:\n", i);
	for (j = 0; j < blksize; j++)
	{
		printf("%02x", secondaryBlocks[i][j]);
	}

	putchar('\n');
}
#endif
	sharenum = (DTKA_FEC_Q / 2) * dtka_fec_x;
	for (i = 0; i < DTKA_FEC_Q; i++)
	{
		sharenums[i] = sharenum;
		sharenum++;
		if (sharenum == DTKA_FEC_M)
		{
			sharenum = 0;
		}
	}

	for (i = 0; i < DTKA_FEC_Q; i++)
	{
		obj = sdr_malloc(sdr, 40 + blksize);
		if (obj == 0)
		{
			putErrmsg("Not enough heap space for block ZCO.", NULL);
			fec_free(fec);
			MRELEASE(buffer);
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
		if (zco == 0 || zco == (Object) -1)
		{
			putErrmsg("Can't create block ZCO.", NULL);
			fec_free(fec);
			MRELEASE(buffer);
			return -1;
		}
#if DTKA_DEBUG
printf("Sending block for share number %d.\n", sharenum);
fflush(stdout);
#endif
		if (bp_send(sap, destEid, NULL, 360000, BP_STD_PRIORITY,
			NoCustodyRequested, 0, 0, NULL, zco, &newBundle) < 0)
		{
			putErrmsg("Failed publishing bulletin block.", NULL);
		}
	}

	fec_free(fec);
	MRELEASE(buffer);
	return 0;
}

#if defined (VXWORKS) || defined (RTEMS) || defined (bionic)
int	kapublish(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
#else
int	main(int argc, char *argv[])
{
#endif
	char		ownEid[32];
	KapublishState	state = { NULL, 1 };
	BpSAP		sendSAP;
	Sdr		sdr;
	Object		dbobj;
	DtkaAuthDB	db;
	time_t		currentTime;
	int		interval;
	BpDelivery	dlv;

	if (kauthAttach() < 0)
	{
		putErrmsg("kapublish can't attach to dtka.", NULL);
		return 1;
	}

	isprintf(ownEid, sizeof ownEid, "imc:%d.0", DTKA_CONFER);
	if (bp_open(ownEid, &state.recvSAP) < 0)
	{
		putErrmsg("Can't open own reception endpoint.", ownEid);
		ionDetach();
		return 1;
	}

	isprintf(ownEid, sizeof ownEid, "ipn:" UVAST_FIELDSPEC ".%d",
			getOwnNodeNbr(), DTKA_ANNOUNCE);
	if (bp_open(ownEid, &sendSAP) < 0)
	{
		putErrmsg("Can't open own transmission endpoint.", ownEid);
		ionDetach();
		return 1;
	}

	sdr = getIonsdr();
	dbobj = getKauthDbObject();
	if (dbobj == 0)
	{
		putErrmsg("No DTKA authority database.", NULL);
		ionDetach();
		return 1;
	}

	sdr_read(sdr, (char *) &db, dbobj, sizeof(DtkaAuthDB));

	/*	Main loop: receive proposed bulletins, mark toward
	 *	consensus, until consensus interval ends.		*/

	oK(_kapublishState(&state));
	isignal(SIGTERM, shutDown);
	writeMemo("[i] kapublish is running.");
	while (state.running)
	{
		currentTime = getCtime();
		interval = (db.currentCompilationTime + db.consensusInterval)
				- currentTime;
		if (interval <= 0)
		{
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
			putErrmsg("kapublish bulletin reception failed.", NULL);
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
			if (handleProposedBulletin(sdr, &db,
					dlv.bundleSourceEid, dlv.adu) < 0)
			{
				putErrmsg("Can't handle proposed bulletin.",
						NULL);
				sdr_cancel_xn(sdr);
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
	writeMemo("[i] kapublish has ended.");
	ionDetach();
	return 0;
}
