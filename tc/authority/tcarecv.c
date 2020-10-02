/*
	tcarecv.c:	Trusted Collective authority daemon for
			reception of critical data records declared
			by clients.

	Author: Scott Burleigh, JPL

	Copyright (c) 2020, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
#include "tcaP.h"

typedef struct
{
	BpSAP	sap;
	int	running;
} TcarecvState;

static TcarecvState	*_tcarecvState(TcarecvState *newState)
{
	void		*value;
	TcarecvState	*state;
	
	if (newState)			/*	Add task variable.	*/
	{
		value = (void *) (newState);
		state = (TcarecvState *) sm_TaskVar(&value);
	}
	else				/*	Retrieve task variable.	*/
	{
		state = (TcarecvState *) sm_TaskVar(NULL);
	}

	return state;
}

static void	shutDown()	/*	Commands tcarecv termination.	*/
{
	TcarecvState	*state;

	isignal(SIGTERM, shutDown);
	writeMemo("TCA receiver daemon interrupted.");
	state = _tcarecvState(NULL);
	bp_interrupt(state->sap);
	state->running = 0;
}

static unsigned short	fetchRecord(Object recordsList, uvast nodeNbr,
				time_t effectiveTime, Object *recordElt,
				Object *nextRecordElt)
{
	Sdr		sdr = getIonsdr();
	Object		elt;
	TcaRecord	record;

	CHKERR(recordsList);
	CHKERR(nodeNbr);
	CHKERR(effectiveTime);
	CHKERR(recordElt);
	*recordElt = 0;			/*	Not found.  (Default)	*/
	if (nextRecordElt)
	{
		*nextRecordElt = 0;	/*	None.  (Default)	*/
	}

	for (elt = sdr_list_first(sdr, recordsList); elt;
			elt = sdr_list_next(sdr, elt))
	{
		sdr_read(sdr, (char *) &record, sdr_list_data(sdr, elt),
				TC_HDR_LEN);
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

		/*	Found it.					*/

		*recordElt = elt;
		return record.datLength;
	}

	if (nextRecordElt)
	{
		*nextRecordElt = elt;
	}

	return 0;			/*	Not found.		*/
}

static int	acquireRecord(Sdr sdr, TcaDB *db, char *src, Object adu)
{
	int		parsedOkay;
	MetaEid		metaEid;
	VScheme		*vscheme;
	PsmAddress	schemeElt;
	vast		recordLength;
	ZcoReader	reader;
	int		len;
	char		buffer[TC_MAX_REC];
	char		*cursor = buffer;
	Object		clientElt;
	uvast		client;
	int		auths;
	char		*acknowledged;
	TcaRecord	record;
	Object		recordElt;
	Object		nextRecordElt;
	Object		obj;

	parsedOkay = parseEidString(src, &metaEid, &vscheme, &schemeElt);
	if (!parsedOkay)
	{
		restoreEidString(&metaEid);
		writeMemoNote("[?] Can't parse source of TCA record", src);
		return 0;
	}

	recordLength = zco_source_data_length(sdr, adu);
	len = recordLength;
	if (len > TC_MAX_REC)
	{
		restoreEidString(&metaEid);
		writeMemoNote("[?] TCA record length error", itoa(len));
		return 0;
	}

#if TC_DEBUG
writeMemoNote("tcarecv: Got record from", itoa(metaEid.elementNbr));
#endif
	memset(&record, 0, sizeof record);
	zco_start_receiving(adu, &reader);
	recordLength = zco_receive_source(sdr, &reader, TC_MAX_REC, buffer);
	len = recordLength;
	if (tc_deserialize(&cursor, &len, TC_MAX_DATLEN, 
			&(record.nodeNbr), &(record.effectiveTime),
			&(record.assertionTime), &(record.datLength),
			record.datValue) == 0)
	{
#if TC_DEBUG
writeMemo("tcarecv: Deserialize failed.");
#endif
		writeMemo("[?] Unable to deserialize record.");
		restoreEidString(&metaEid);
		return 0;
	}

	if (db->validClients)
	{
		/*	Only authorized clients may submit records.	*/

		for (clientElt = sdr_list_first(sdr, db->validClients);
			clientElt; clientElt = sdr_list_next(sdr, clientElt))
		{
			client = (uvast) sdr_list_data(sdr, clientElt);
			if (client < metaEid.elementNbr)
			{
				continue;
			}

			break;
		}

		if (clientElt == 0 || client != metaEid.elementNbr)
		{
			restoreEidString(&metaEid);
			writeMemoNote("[?] TCA record posted by unauthorized \
client", src);
			return 0;
		}
	}
	else
	{
		/*	Nodes may only submit records for themselves.	*/

		if (record.nodeNbr != metaEid.elementNbr)
		{
			restoreEidString(&metaEid);
			writeMemoNote("[?] TCA record posted from unauthorized \
EID", src);
			return 0;
		}
	}

	restoreEidString(&metaEid);
	if (fetchRecord(db->pendingRecords, record.nodeNbr,
			record.effectiveTime, &recordElt, &nextRecordElt))
	{
#if TC_DEBUG
writeMemo("tcarecv: Record already pending; ignored.");
#endif
		return 0;	/*	Record already pending; ignore.	*/
	}

	/*	Record not previously submitted in this cycle.  Was
	 *	it submitted in an earlier cycle?			*/

	if (fetchRecord(db->currentRecords, record.nodeNbr,
			record.effectiveTime, &recordElt, NULL))
	{
#if TC_DEBUG
writeMemo("tcarecv: Record was previously posted.");
#endif
		return 0;	/*	Record already posted; ignore.	*/
	}

	/*	Record not previously submitted.  Okay to add it.	*/

#if TC_DEBUG
writeMemo("tcarecv: Adding record to list of pending records.");
#endif
	auths = sdr_list_length(sdr, db->authorities);
	CHKERR(auths > 0);
	acknowledged = MTAKE(auths);
	CHKERR(acknowledged);
	memset(acknowledged, 0, auths);
	acknowledged[db->ownAuthIdx] = 1;
	obj = sdr_malloc(sdr, sizeof(TcaRecord));
	if (obj == 0)
	{
		MRELEASE(acknowledged);
		return -1;
	}

	record.acknowledged = sdr_malloc(sdr, auths);
	if (record.acknowledged == 0)
	{
		sdr_free(sdr, obj);
		MRELEASE(acknowledged);
		return -1;
	}

	sdr_write(sdr, record.acknowledged, acknowledged, auths);
	MRELEASE(acknowledged);
	sdr_write(sdr, obj, (char *) &record, sizeof(TcaRecord));
	if (nextRecordElt)
	{
		recordElt = sdr_list_insert_before(sdr, nextRecordElt, obj);
	}
	else
	{
		recordElt = sdr_list_insert_last(sdr, db->pendingRecords, obj);
	}

	if (recordElt == 0)
	{
		return -1;
	}

	return record.datLength;
}

#if defined (ION_LWT)
int	tcarecv(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	int	blocksGroupNbr = (a1 ? atoi((char *) a1) : -1);
#else
int	main(int argc, char *argv[])
{
	int	blocksGroupNbr = (argc > 1 ? atoi(argv[1]) : -1);
#endif
	char		ownEid[32];
	TcarecvState	state = { NULL, 1 };
	Sdr		sdr;
	Object		dbobj;
	TcaDB		db;
	BpDelivery	dlv;

	if (blocksGroupNbr < 1)
	{
		puts("Usage: tcarecv <IMC group number for TC blocks>");
		return -1;
	}

	if (tcaAttach(blocksGroupNbr) < 0)
	{
		putErrmsg("tcarecv can't attach to tca system.",
				itoa(blocksGroupNbr));
		return 1;
	}

	sdr = getIonsdr();
	dbobj = getTcaDBObject(blocksGroupNbr);
	if (dbobj == 0)
	{
		putErrmsg("No TCA authority database.", itoa(blocksGroupNbr));
		ionDetach();
		return 1;
	}

	sdr_read(sdr, (char *) &db, dbobj, sizeof(TcaDB));
	isprintf(ownEid, sizeof ownEid, "imc:%d.0", db.recordsGroupNbr);
	if (bp_open(ownEid, &state.sap) < 0)
	{
		putErrmsg("Can't open own reception endpoint.", ownEid);
		ionDetach();
		return 1;
	}

	/*	Main loop: receive record, insert into list.		*/

	oK(_tcarecvState(&state));
	isignal(SIGTERM, shutDown);
	writeMemo("[i] tcarecv is running.");
	while (state.running)
	{
		if (bp_receive(state.sap, &dlv, BP_BLOCKING) < 0)
		{
			putErrmsg("tcarecv bundle reception failed.", NULL);
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
			if (acquireRecord(sdr, &db, dlv.bundleSourceEid,
					dlv.adu) < 0)
			{
				putErrmsg("Can't acquire record.", NULL);
				sdr_cancel_xn(sdr);
			}

			if (sdr_end_xn(sdr) < 0)
			{
				putErrmsg("Can't handle TCA record.", NULL);
				state.running = 0;
				continue;
			}
		}

		bp_release_delivery(&dlv, 1);
	}

	bp_close(state.sap);
	writeErrmsgMemos();
	writeMemo("[i] tcarecv has ended.");
	ionDetach();
	return 0;
}
