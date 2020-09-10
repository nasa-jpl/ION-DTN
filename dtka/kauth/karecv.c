/*
	karecv.c:	Key authority daemon for reception of key
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
	BpSAP	sap;
	int	running;
} KarecvState;

static KarecvState	*_karecvState(KarecvState *newState)
{
	void		*value;
	KarecvState	*state;
	
	if (newState)			/*	Add task variable.	*/
	{
		value = (void *) (newState);
		state = (KarecvState *) sm_TaskVar(&value);
	}
	else				/*	Retrieve task variable.	*/
	{
		state = (KarecvState *) sm_TaskVar(NULL);
	}

	return state;
}

static void	shutDown()	/*	Commands karecv termination.	*/
{
	KarecvState	*state;

	isignal(SIGTERM, shutDown);
	PUTS("DTKA receiver daemon interrupted.");
	state = _karecvState(NULL);
	bp_interrupt(state->sap);
	state->running = 0;
}

static unsigned short	fetchRecord(Object recordsList, uvast nodeNbr,
				time_t effectiveTime, Object *recordElt,
				Object *nextRecordElt)
{
	Sdr		sdr = getIonsdr();
	Object		elt;
	DtkaRecord	record;

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
				DTKA_HDR_LEN);
		if (record.nodeNbr < nodeNbr)
		{
			continue;
		}
		
		if (record.nodeNbr > nodeNbr)
		{
			break;
		}

		if (record.effectiveTime.seconds < effectiveTime->seconds)
		{
			continue;
		}
		
		if (record.effectiveTime.seconds > effectiveTime->seconds)
		{
			break;
		}

		if (record.effectiveTime.count < effectiveTime->count)
		{
			continue;
		}
		
		if (record.effectiveTime.count > effectiveTime->count)
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

static int	acquireRecord(Sdr sdr, DtkaAuthDB *db, char *src, Object adu)
{
	int		parsedOkay;
	MetaEid		metaEid;
	VScheme		*vscheme;
	PsmAddress	schemeElt;
	vast		recordLength;
	ZcoReader	reader;
	int		len;
	char		buffer[DTKA_MAX_REC];
	unsigned char	*cursor = (unsigned char *) buffer;
	DtkaRecord	record;
	Object		recordElt;
	Object		nextRecordElt;
	Object		obj;

	parsedOkay = parseEidString(src, &metaEid, &vscheme, &schemeElt);
	if (!parsedOkay)
	{
		restoreEidString(&metaEid);
		writeMemoNote("[?] Can't parse source of DTKA record", src);
		return 0;
	}

	recordLength = zco_source_data_length(sdr, adu);
	len = recordLength;
	if (len > DTKA_MAX_REC)
	{
		restoreEidString(&metaEid);
		writeMemoNote("[?] DTKA record length error", itoa(len));
		return 0;
	}

#if DTKA_DEBUG
printf("Got pubkey record from " UVAST_FIELDSPEC " in karecv.\n",
metaEid.nodeNbr);
fflush(stdout);
#endif
	memset(&record, 0, sizeof record);
	zco_start_receiving(adu, &reader);
	recordLength = zco_receive_source(sdr, &reader, DTKA_MAX_REC, buffer);
	len = recordLength;
	if (dtka_deserialize(&cursor, &len, DTKA_MAX_DATLEN, 
			&(record.nodeNbr), &(record.effectiveTime),
			&(record.assertionTime), &(record.datLength),
			record.datValue) < 1)
	{
#if DTKA_DEBUG
puts("Deserialize failed.");
fflush(stdout);
#endif
		writeMemo("Unable to deserialize record.");
		restoreEidString(&metaEid);
		return 0;
	}

	if (record.nodeNbr != metaEid.nodeNbr)
	{
		restoreEidString(&metaEid);
		writeMemoNote("[?] DTKA record posted from unauthorized EID",
				src);
		return 0;
	}

	restoreEidString(&metaEid);
	if (fetchRecord(db->pendingRecords, record.nodeNbr,
			record.effectiveTime, &recordElt, &nextRecordElt))
	{
#if DTKA_DEBUG
puts("Record already pending; ignored.");
fflush(stdout);
#endif
		return 0;	/*	Record already pending; ignore.	*/
	}

	/*	Record not previously submitted in this cycle.  Was
	 *	it submitted in an earlier cycle?			*/

	if (fetchRecord(db->currentRecords, record.nodeNbr,
			record.effectiveTime, &recordElt, NULL))
	{
		return 0;	/*	Record already posted; ignore.	*/
	}

	/*	Record not previously submitted.  Okay to add it.	*/

	obj = sdr_malloc(sdr, sizeof(DtkaRecord));
	if (obj == 0)
	{
		return -1;
	}

	record.acknowledged[db->ownAuthIdx] = 1;
	sdr_write(sdr, obj, (char *) &record, sizeof(DtkaRecord));
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

#if defined (VXWORKS) || defined (RTEMS) || defined (bionic)
int	karecv(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
#else
int	main(int argc, char *argv[])
{
#endif
	char		ownEid[32];
	KarecvState	state = { NULL, 1 };
	Sdr		sdr;
	Object		dbobj;
	DtkaAuthDB	db;
	BpDelivery	dlv;

	if (kauthAttach() < 0)
	{
		putErrmsg("karecv can't attach to dtka.", NULL);
		return 1;
	}

	isprintf(ownEid, sizeof ownEid, "imc:%d.0", DTKA_DECLARE);
	if (bp_open(ownEid, &state.sap) < 0)
	{
		putErrmsg("Can't open own endpoint.", ownEid);
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

	/*	Main loop: receive record, insert into list.		*/

	oK(_karecvState(&state));
	isignal(SIGTERM, shutDown);
	writeMemo("[i] karecv is running.");
	while (state.running)
	{
		if (bp_receive(state.sap, &dlv, BP_BLOCKING) < 0)
		{
			putErrmsg("karecv bundle reception failed.", NULL);
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
				putErrmsg("Can't handle DTKA record.", NULL);
				state.running = 0;
				continue;
			}
		}

		bp_release_delivery(&dlv, 1);
	}

	bp_close(state.sap);
	writeErrmsgMemos();
	writeMemo("[i] karecv has ended.");
	ionDetach();
	return 0;
}
