/*
 *	libltpP.c:	functions enabling the implementation of
 *			LTP engines.
 *
 *	Copyright (c) 2007, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 *
 */

#include "ltpP.h"

#define	EST_LINK_OHD	16

#ifndef LTPDEBUG
#define	LTPDEBUG	0
#endif

#define LTP_VERSION	0;

/*	*	*	Helpful utility functions	*	*	*/

static Object	_ltpdbObject(Object *newDbObj)
{
	static Object	obj = 0;

	if (newDbObj)
	{
		obj = *newDbObj;
	}

	return obj;
}

static LtpDB	*_ltpConstants()
{
	static LtpDB	buf;
	static LtpDB	*db = NULL;

	if (db == NULL)
	{
		/*	Load constants into a conveniently accessed
		 *	structure.  Note that this CANNOT be treated
		 *	as a current database image in later
		 *	processing.					*/

		sdr_read(getIonsdr(), (char *) &buf, _ltpdbObject(NULL),
				sizeof(LtpDB));
		db = &buf;
	}

	return db;
}

	/*	Note: to avoid running out of database heap space,
	 *	LTP uses flow control based on limiting (a) the number
	 *	of export sessions that can be concurrently active
	 *	and (b) the maximum block size for any single export
	 *	session.  The product of these two values constitutes
	 *	the flow control "window" for LTP.  The limits are
	 *	set at the time LTP is initialized and are used to
	 *	fix the size of the window at that time; specifically,
	 *	they fix the size of the exportSessions hash table.
	 *	(Note that the maximum block size for a given Span
	 *	can be less than the global limit, for purposes of
	 *	aggregation tuning.)					*/

/*	*	*	LTP service control functions	*	*	*/

static void	resetClient(LtpVclient *client)
{
	if (client->semaphore == SM_SEM_NONE)
	{
		client->semaphore = sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO);
	}
	else
	{
		sm_SemUnend(client->semaphore);
	}

	sm_SemTake(client->semaphore);			/*	Lock.	*/
	client->pid = ERROR;				/*	None.	*/
}

static void	raiseClient(LtpVclient *client)
{
	client->semaphore = SM_SEM_NONE;
	resetClient(client);
}

static void	resetSpan(LtpVspan *vspan)
{
	if (vspan->bufEmptySemaphore == SM_SEM_NONE)
	{
		vspan->bufEmptySemaphore = sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO);
	}
	else
	{
		sm_SemUnend(vspan->bufEmptySemaphore);
	}

	sm_SemTake(vspan->bufEmptySemaphore);		/*	Lock.	*/
	if (vspan->bufFullSemaphore == SM_SEM_NONE)
	{
		vspan->bufFullSemaphore = sm_SemCreate(SM_NO_KEY,
				SM_SEM_FIFO);
	}
	else
	{
		sm_SemUnend(vspan->bufFullSemaphore);
	}

	sm_SemTake(vspan->bufFullSemaphore);		/*	Lock.	*/
	if (vspan->segSemaphore == SM_SEM_NONE)
	{
		vspan->segSemaphore = sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO);
	}
	else
	{
		sm_SemUnend(vspan->segSemaphore);
	}

	sm_SemTake(vspan->segSemaphore);		/*	Lock.	*/
	vspan->meterPid = ERROR;			/*	None.	*/
	vspan->lsoPid = ERROR;				/*	None.	*/
}

static int	raiseSpan(Object spanElt, LtpVdb *ltpvdb)
{
	Sdr		ltpSdr = getIonsdr();
	PsmPartition	ltpwm = getIonwm();
	Object		spanObj;
	LtpSpan		span;
	LtpVspan	*vspan;
	PsmAddress	vspanElt;
	PsmAddress	addr;

	spanObj = sdr_list_data(ltpSdr, spanElt);
	sdr_read(ltpSdr, (char *) &span, spanObj, sizeof(LtpSpan));
	findSpan(span.engineId, &vspan, &vspanElt);
	if (vspanElt)	/*	Span is already raised.			*/
	{
		return 0;
	}

	addr = psm_zalloc(ltpwm, sizeof(LtpVspan));
	if (addr == 0)
	{
		return -1;
	}

	vspanElt = sm_list_insert_last(ltpwm, ltpvdb->spans, addr);
	if (vspanElt == 0)
	{
		psm_free(ltpwm, addr);
		return -1;
	}

	vspan = (LtpVspan *) psp(ltpwm, addr);
	memset((char *) vspan, 0, sizeof(LtpVspan));
	vspan->spanElt = spanElt;
	vspan->engineId = span.engineId;
	vspan->segmentBuffer = psm_malloc(ltpwm, span.maxSegmentSize);
	if (vspan->segmentBuffer == 0)
	{
		oK(sm_list_delete(ltpwm, vspanElt, NULL, NULL));
		psm_free(ltpwm, addr);
		return -1;
	}

	vspan->bufEmptySemaphore = SM_SEM_NONE;
	vspan->bufFullSemaphore = SM_SEM_NONE;
	vspan->segSemaphore = SM_SEM_NONE;
	resetSpan(vspan);
	return 0;
}

static void	dropSpan(LtpVspan *vspan, PsmAddress vspanElt)
{
	PsmPartition	ltpwm = getIonwm();
	PsmAddress	vspanAddr;

	vspanAddr = sm_list_data(ltpwm, vspanElt);
	if (vspan->bufEmptySemaphore != SM_SEM_NONE)
	{
		sm_SemDelete(vspan->bufEmptySemaphore);
	}

	if (vspan->bufFullSemaphore != SM_SEM_NONE)
	{
		sm_SemDelete(vspan->bufFullSemaphore);
	}

	if (vspan->segSemaphore != SM_SEM_NONE)
	{
		sm_SemDelete(vspan->segSemaphore);
	}

	psm_free(ltpwm, vspan->segmentBuffer);
	oK(sm_list_delete(ltpwm, vspanElt, NULL, NULL));
	psm_free(ltpwm, vspanAddr);
}

static void	startSpan(LtpVspan *vspan)
{
	Sdr	ltpSdr = getIonsdr();
	LtpSpan	span;
	char	ltpmeterCmdString[64];
	char	cmd[SDRSTRING_BUFSZ];
	char	engineIdString[11];
	char	lsoCmdString[SDRSTRING_BUFSZ + 64];

	sdr_read(ltpSdr, (char *) &span, sdr_list_data(ltpSdr, vspan->spanElt),
			sizeof(LtpSpan));
	isprintf(ltpmeterCmdString, sizeof ltpmeterCmdString, "ltpmeter %lu",
			span.engineId);
	vspan->meterPid = pseudoshell(ltpmeterCmdString);
	sdr_string_read(ltpSdr, cmd, span.lsoCmd);
	isprintf(engineIdString, sizeof engineIdString, "%lu", span.engineId);
	isprintf(lsoCmdString, sizeof lsoCmdString, "%s %s", cmd,
			engineIdString);
	vspan->lsoPid = pseudoshell(lsoCmdString);
}

static void	stopSpan(LtpVspan *vspan)
{
	if (vspan->bufEmptySemaphore != SM_SEM_NONE)
	{
		sm_SemEnd(vspan->bufEmptySemaphore);
	}

	if (vspan->bufFullSemaphore != SM_SEM_NONE)
	{
		sm_SemEnd(vspan->bufFullSemaphore);
	}

	if (vspan->segSemaphore != SM_SEM_NONE)
	{
		sm_SemEnd(vspan->segSemaphore);
	}
}

static void	waitForSpan(LtpVspan *vspan)
{
	if (vspan->lsoPid != ERROR)
	{
		while (sm_TaskExists(vspan->lsoPid))
		{
			microsnooze(100000);
		}
	}

	if (vspan->meterPid != ERROR)
	{
		while (sm_TaskExists(vspan->meterPid))
		{
			microsnooze(100000);
		}
	}
}

static char 	*_ltpvdbName()
{
	return "ltpvdb";
}

static LtpVdb		*_ltpvdb(char **name)
{
	static LtpVdb	*vdb = NULL;
	PsmPartition	wm;
	PsmAddress	vdbAddress;
	PsmAddress	elt;
	Sdr		sdr;
	Object		sdrElt;
	int		i;
	LtpVclient	*client;

	if (name)
	{
		if (*name == NULL)	/*	Terminating.		*/
		{
			vdb = NULL;
			return vdb;
		}

		/*	Attaching to volatile database.			*/

		wm = getIonwm();
		if (psm_locate(wm, *name, &vdbAddress, &elt) < 0)
		{
			putErrmsg("Failed searching for vdb.", NULL);
			return vdb;
		}

		if (elt)
		{
			vdb = (LtpVdb *) psp(wm, vdbAddress);
			return vdb;
		}

		/*	LTP volatile database doesn't exist yet.	*/

		sdr = getIonsdr();
		sdr_begin_xn(sdr);	/*	Just to lock memory.	*/

		/*	Create and catalogue the LtpVdb object.		*/

		vdbAddress = psm_zalloc(wm, sizeof(LtpVdb));
		if (vdbAddress == 0)
		{
			sdr_exit_xn(sdr);
			putErrmsg("No space for dynamic database.", NULL);
			return NULL;
		}

		vdb = (LtpVdb *) psp(wm, vdbAddress);
		memset((char *) vdb, 0, sizeof(LtpVdb));
		vdb->lsiPid = ERROR;		/*	None yet.	*/
		vdb->clockPid = ERROR;		/*	None yet.	*/
		vdb->sessionSemaphore = sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO);
		sm_SemGive(vdb->sessionSemaphore);
		if ((vdb->spans = sm_list_create(wm)) == 0
		|| psm_catlg(wm, *name, vdbAddress) < 0)
		{
			sdr_exit_xn(sdr);
			putErrmsg("Can't initialize volatile database.", NULL);
			return NULL;
		}

		/*	Raise all clients.				*/

		for (i = 0, client = vdb->clients; i < LTP_MAX_NBR_OF_CLIENTS;
				i++, client++)
		{
			client->notices = (_ltpConstants())->clients[i].notices;
			raiseClient(client);
		}

		/*	Raise all spans.				*/

		for (sdrElt = sdr_list_first(sdr, (_ltpConstants())->spans);
				sdrElt; sdrElt = sdr_list_next(sdr, sdrElt))
		{
			if (raiseSpan(sdrElt, vdb) < 0)
			{
				sdr_exit_xn(sdr);
				putErrmsg("Can't raise all spans.", NULL);
				return NULL;
			}
		}

		sdr_exit_xn(sdr);	/*	Unlock memory.		*/
	}

	return vdb;
}

static char	*_ltpdbName()
{
	return "ltpdb";
}

int	ltpInit(int estMaxExportSessions, int bytesReserved)
{
	Sdr		ltpSdr;
	Object		ltpdbObject;
	IonDB		iondb;
	long		avblForBP;
	char		avbltyMsg[160];
	LtpDB		ltpdbBuf;
	int		i;
	char		*ltpvdbName = _ltpvdbName();

	if (ionAttach() < 0)
	{
		putErrmsg("LTP can't attach to ION.", NULL);
		return -1;
	}

	ltpSdr = getIonsdr();
	srand(time(NULL));

	/*	Recover the LTP database, creating it if necessary.	*/

	sdr_begin_xn(ltpSdr);
	ltpdbObject = sdr_find(ltpSdr, _ltpdbName(), NULL);
	switch (ltpdbObject)
	{
	case -1:		/*	SDR error.			*/
		putErrmsg("Can't search for LTP database in SDR.", NULL);
		sdr_cancel_xn(ltpSdr);
		return -1;

	case 0:			/*	Not found; must create new DB.	*/
		if (estMaxExportSessions <= 0 || bytesReserved <= 0)
		{
			sdr_exit_xn(ltpSdr);
			putErrmsg("Must supply estMaxExportSessions and \
bytesReserved.", NULL);
			return -1;
		}

		sdr_read(ltpSdr, (char *) &iondb, getIonDbObject(),
				sizeof(IonDB));
		avblForBP = iondb.occupancyCeiling
				- (iondb.receptionSpikeReserve + bytesReserved);
		if (avblForBP < 1000000)
		{
			isprintf(avbltyMsg, sizeof avbltyMsg, "[?] LTP \
reservation size %d limits space available for bundles to %ld bytes.",
					bytesReserved, avblForBP);
			writeMemo(avbltyMsg);
		}

		ltpdbObject = sdr_malloc(ltpSdr, sizeof(LtpDB));
		if (ltpdbObject == 0)
		{
			putErrmsg("No space for database.", NULL);
			sdr_cancel_xn(ltpSdr);
			return -1;
		}

		/*	Initialize the non-volatile database.		*/

		memset((char *) &ltpdbBuf, 0, sizeof(LtpDB));
		ltpdbBuf.ownEngineId = iondb.ownNodeNbr;
		encodeSdnv(&(ltpdbBuf.ownEngineIdSdnv), ltpdbBuf.ownEngineId);
		ltpdbBuf.estMaxExportSessions = estMaxExportSessions;
		ltpdbBuf.heapSpaceBytesReserved = bytesReserved;
		ltpdbBuf.ownQtime = 1;		/*	Default.	*/
		ltpdbBuf.enforceSchedule = 1;	/*	Default.	*/
		for (i = 0; i < LTP_MAX_NBR_OF_CLIENTS; i++)
		{
			ltpdbBuf.clients[i].notices = sdr_list_create(ltpSdr);
		}

		ltpdbBuf.exportSessionsHash = sdr_hash_create(ltpSdr,
				sizeof(unsigned long), estMaxExportSessions,
				LTP_MEAN_SEARCH_LENGTH);
		ltpdbBuf.deadExports = sdr_list_create(ltpSdr);
		ltpdbBuf.spans = sdr_list_create(ltpSdr);
		ltpdbBuf.timeline = sdr_list_create(ltpSdr);
		sdr_write(ltpSdr, ltpdbObject, (char *) &ltpdbBuf,
				sizeof(LtpDB));
		sdr_catlg(ltpSdr, _ltpdbName(), 0, ltpdbObject);
		ionOccupy(bytesReserved);	/*	Reserve space.	*/
		if (sdr_end_xn(ltpSdr))
		{
			putErrmsg("Can't create LTP database.", NULL);
			return -1;
		}

		break;

	default:		/*	Found DB in the SDR.		*/
		sdr_exit_xn(ltpSdr);
	}

	oK(_ltpdbObject(&ltpdbObject));	/*	Save database location.	*/
	oK(_ltpConstants());

	/*	Load volatile database, initializing as necessary.	*/

	if (_ltpvdb(&ltpvdbName) == NULL)
	{
		putErrmsg("LTP can't initialize vdb.", NULL);
		return -1;
	}

	return 0;		/*	LTP service is available.	*/
}

Object	getLtpDbObject()
{
	return _ltpdbObject(NULL);
}

LtpDB	*getLtpConstants()
{
	return _ltpConstants();
}

LtpVdb	*getLtpVdb()
{
	return _ltpvdb(NULL);
}

int	ltpStart(char *lsiCmd)
{
	Sdr		ltpSdr = getIonsdr();
	PsmPartition	ltpwm = getIonwm();
	LtpVdb		*ltpvdb = _ltpvdb(NULL);
	PsmAddress	elt;

	if (lsiCmd == NULL)
	{
		putErrmsg("LTP can't start: no LSI command.", NULL);
		return -1;
	}

	sdr_begin_xn(ltpSdr);	/*	Just to lock memory.		*/

	/*	Start the LTP events clock if necessary.		*/

	if (ltpvdb->clockPid == ERROR || sm_TaskExists(ltpvdb->clockPid) == 0)
	{
		ltpvdb->clockPid = pseudoshell("ltpclock");
	}

	/*	Start input link service if necessary.			*/

	if (ltpvdb->lsiPid == ERROR || sm_TaskExists(ltpvdb->lsiPid) == 0)
	{
		ltpvdb->lsiPid = pseudoshell(lsiCmd);
	}

	/*	Start output link services for remote spans.		*/

	for (elt = sm_list_first(ltpwm, ltpvdb->spans); elt;
			elt = sm_list_next(ltpwm, elt))
	{
		startSpan((LtpVspan *) psp(ltpwm, sm_list_data(ltpwm, elt)));
	}

	sdr_exit_xn(ltpSdr);	/*	Unlock memory.			*/
	return 0;
}

void	ltpStop()		/*	Reverses ltpStart.		*/
{
	Sdr		ltpSdr = getIonsdr();
	PsmPartition	ltpwm = getIonwm();
	LtpVdb		*ltpvdb = _ltpvdb(NULL);
	int		i;
	LtpVclient	*client;
	PsmAddress	elt;
	LtpVspan	*vspan;

	/*	Tell all LTP processes to stop.				*/

	sdr_begin_xn(ltpSdr);	/*	Just to lock memory.		*/
	if (ltpvdb->sessionSemaphore != SM_SEM_NONE)
	{
		sm_SemEnd(ltpvdb->sessionSemaphore);
	}

	for (i = 0, client = ltpvdb->clients; i < LTP_MAX_NBR_OF_CLIENTS;
			i++, client++)
	{
		if (client->semaphore != SM_SEM_NONE)
		{
			sm_SemEnd(client->semaphore);
		}
	}

	if (ltpvdb->lsiPid != ERROR)
	{
		sm_TaskKill(ltpvdb->lsiPid, SIGTERM);
	}

	for (elt = sm_list_first(ltpwm, ltpvdb->spans); elt;
			elt = sm_list_next(ltpwm, elt))
	{
		vspan = (LtpVspan *) psp(ltpwm, sm_list_data(ltpwm, elt));
		stopSpan(vspan);
	}

	if (ltpvdb->clockPid != ERROR)
	{
		sm_TaskKill(ltpvdb->clockPid, SIGTERM);
	}

	sdr_exit_xn(ltpSdr);	/*	Unlock memory.			*/

	/*	Wait until all LTP processes have stopped.		*/

	if (ltpvdb->lsiPid != ERROR)
	{
		while (sm_TaskExists(ltpvdb->lsiPid))
		{
			microsnooze(100000);
		}
	}

	for (elt = sm_list_first(ltpwm, ltpvdb->spans); elt;
			elt = sm_list_next(ltpwm, elt))
	{
		vspan = (LtpVspan *) psp(ltpwm, sm_list_data(ltpwm, elt));
		waitForSpan(vspan);
	}

	if (ltpvdb->clockPid != ERROR)
	{
		while (sm_TaskExists(ltpvdb->clockPid))
		{
			microsnooze(100000);
		}
	}

	/*	Now erase all the tasks and reset the semaphores.	*/

	sdr_begin_xn(ltpSdr);	/*	Just to lock memory.		*/
	ltpvdb->clockPid = ERROR;
	if (ltpvdb->sessionSemaphore == SM_SEM_NONE)
	{
		ltpvdb->sessionSemaphore = sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO);
	}
	else
	{
		sm_SemUnend(ltpvdb->sessionSemaphore);
	}

	sm_SemGive(ltpvdb->sessionSemaphore);		/*	Unlock.	*/
	for (i = 0, client = ltpvdb->clients; i < LTP_MAX_NBR_OF_CLIENTS;
			i++, client++)
	{
		resetClient(client);
	}

	ltpvdb->lsiPid = ERROR;
	for (elt = sm_list_first(ltpwm, ltpvdb->spans); elt;
			elt = sm_list_next(ltpwm, elt))
	{
		vspan = (LtpVspan *) psp(ltpwm, sm_list_data(ltpwm, elt));
		resetSpan(vspan);
	}

	sdr_exit_xn(ltpSdr);	/*	Unlock memory.			*/
}

int	ltpAttach()
{
	Object	ltpdbObject = _ltpdbObject(NULL);
	LtpVdb	*ltpvdb = _ltpvdb(NULL);
	Sdr	ltpSdr;
	char	*ltpvdbName = _ltpvdbName();

	if (ltpdbObject && ltpvdb)
	{
		return 0;		/*	Already attached.	*/
	}

	if (ionAttach() < 0)
	{
		putErrmsg("LTP can't attach to ION.", NULL);
		return -1;
	}

	ltpSdr = getIonsdr();
	srand(time(NULL));

	/*	Locate the LTP database.				*/

	if (ltpdbObject == 0)
	{
		sdr_begin_xn(ltpSdr);
		ltpdbObject = sdr_find(ltpSdr, _ltpdbName(), NULL);
		sdr_exit_xn(ltpSdr);
		if (ltpdbObject == 0)
		{
			putErrmsg("Can't find LTP database.", NULL);
			return -1;
		}

		oK(_ltpdbObject(&ltpdbObject));
	}

	oK(_ltpConstants());

	/*	Locate the LTP volatile database.			*/

	if (ltpvdb == NULL)
	{
		if (_ltpvdb(&ltpvdbName) == NULL)
		{
			putErrmsg("LTP volatile database not found.", NULL);
			return -1;
		}
	}

	return 0;		/*	LTP service is available.	*/
}

/*	*	*	LTP span mgt and access functions	*	*/

void	findSpan(unsigned long engineId, LtpVspan **vspan,
		PsmAddress *vspanElt)
{
	PsmPartition	ltpwm = getIonwm();
	PsmAddress	elt;

	CHKVOID(ionLocked());
	CHKVOID(vspan);
	CHKVOID(vspanElt);
	for (elt = sm_list_first(ltpwm, (_ltpvdb(NULL))->spans); elt;
			elt = sm_list_next(ltpwm, elt))
	{
		*vspan = (LtpVspan *) psp(ltpwm, sm_list_data(ltpwm, elt));
		if ((*vspan)->engineId == engineId)
		{
			break;
		}
	}

	*vspanElt = elt;	/*	(Zero if vspan was not found.)	*/
}

void	checkReservationLimit()
{
	Sdr	ltpSdr = getIonsdr();
	Object	dbobj = getLtpDbObject();
	LtpDB	db;
	int	totalSessionsAvbl;
	Object	elt;
		OBJ_POINTER(LtpSpan, span);

	sdr_begin_xn(ltpSdr);
	sdr_read(ltpSdr, (char *) &db, dbobj, sizeof(LtpDB));
	totalSessionsAvbl = db.estMaxExportSessions;
	for (elt = sdr_list_first(ltpSdr, db.spans); elt;
			elt = sdr_list_next(ltpSdr, elt))
	{
		GET_OBJ_POINTER(ltpSdr, LtpSpan, span, sdr_list_data(ltpSdr,
				elt));
		totalSessionsAvbl -= span->maxExportSessions;
	}

	if (totalSessionsAvbl < 0)
	{
		writeMemoNote("[?] Total max export sessions exceeds \
estimate.  Session lookup speed may be degraded", itoa(totalSessionsAvbl));
	}
	else
	{
		writeMemo("[i] Total max export sessions does not exceed \
estimate.");
	}

	sdr_exit_xn(ltpSdr);
}

int	addSpan(unsigned long engineId, unsigned int maxExportSessions,
		unsigned int maxImportSessions, unsigned int maxSegmentSize,
		unsigned int aggrSizeLimit, unsigned int aggrTimeLimit,
		char *lsoCmd, unsigned int qTime, int purge)
{
	Sdr		ltpSdr = getIonsdr();
	LtpVspan	*vspan;
	PsmAddress	vspanElt;
	LtpSpan		spanBuf;
	Object		addr;
	Object		spanElt = 0;

	if (lsoCmd == NULL || *lsoCmd == '\0')
	{
		writeMemoNote("[?] No LSO command, can't add span",
				utoa(engineId));
		return 0;
	}

	if (engineId == 0
	|| maxExportSessions == 0 || maxImportSessions == 0
	|| aggrSizeLimit == 0 || aggrTimeLimit == 0)
	{
		writeMemoNote("[?] Missing span parameter(s)", utoa(engineId));
		return 0;
	}

	if (strlen(lsoCmd) > MAX_SDRSTRING)
	{
		writeMemoNote("[?] Link service output command string too long",
				lsoCmd);
		return 0;
	}

	/*	Note: RFC791 says that IPv4 hosts cannot set maximum
	 *	IP packet length to any value less than 576 bytes (the
	 *	PPP MTU size).  IPv4 packet header length ranges from
	 *	20 to 60 bytes, and UDP header length is 8 bytes.  So
	 *	the maximum allowed size for a UDP datagram on a given
	 *	host should not be less than 508 bytes, so we warn if
	 *	maximum LTP segment size is less than 508.		*/

	if (maxSegmentSize < 508)
	{
		writeMemoNote("[i] Note max segment size is less than 508",
				utoa(maxSegmentSize));
	}

	sdr_begin_xn(ltpSdr);
	findSpan(engineId, &vspan, &vspanElt);
	if (vspanElt)		/*	This is a known span.		*/
	{
		sdr_exit_xn(ltpSdr);
		writeMemoNote("[?] Duplicate span", itoa(engineId));
		return 0;
	}

	/*	All parameters validated, okay to add the span.		*/

	memset((char *) &spanBuf, 0, sizeof(LtpSpan));
	spanBuf.engineId = engineId;
	encodeSdnv(&(spanBuf.engineIdSdnv), spanBuf.engineId);
	spanBuf.remoteQtime = qTime;
	spanBuf.purge = purge ? 1 : 0;
	spanBuf.lsoCmd = sdr_string_create(ltpSdr, lsoCmd);
	spanBuf.maxExportSessions = maxExportSessions;
	spanBuf.maxImportSessions = maxImportSessions;
	spanBuf.aggrSizeLimit = aggrSizeLimit;
	spanBuf.aggrTimeLimit = aggrTimeLimit;
	spanBuf.maxSegmentSize = maxSegmentSize;
	spanBuf.exportSessions = sdr_list_create(ltpSdr);
	spanBuf.segments = sdr_list_create(ltpSdr);
	spanBuf.importSessions = sdr_list_create(ltpSdr);
	spanBuf.importSessionsHash = sdr_hash_create(ltpSdr,
			sizeof(unsigned long), maxImportSessions,
			LTP_MEAN_SEARCH_LENGTH);
	spanBuf.closedImports = sdr_list_create(ltpSdr);
	spanBuf.deadImports = sdr_list_create(ltpSdr);
	addr = sdr_malloc(ltpSdr, sizeof(LtpSpan));
	if (addr)
	{
		spanElt = sdr_list_insert_last(ltpSdr, _ltpConstants()->spans,
				addr);
		sdr_write(ltpSdr, addr, (char *) &spanBuf, sizeof(LtpSpan));
	}

	if (sdr_end_xn(ltpSdr) < 0 || spanElt == 0)
	{
		putErrmsg("Can't add span.", itoa(engineId));
		return -1;
	}

	sdr_begin_xn(ltpSdr);	/*	Just to lock memory.		*/
	if (raiseSpan(spanElt, _ltpvdb(NULL)) < 0)
	{
		sdr_exit_xn(ltpSdr);
		putErrmsg("Can't raise span.", NULL);
		return -1;
	}

	sdr_exit_xn(ltpSdr);
	return 1;
}

int	updateSpan(unsigned long engineId, unsigned int maxExportSessions,
		unsigned int maxImportSessions, unsigned int maxSegmentSize,
		unsigned int aggrSizeLimit, unsigned int aggrTimeLimit,
		char *lsoCmd, unsigned int qTime, int purge)
{
	Sdr		ltpSdr = getIonsdr();
	LtpVspan	*vspan;
	PsmAddress	vspanElt;
	Object		addr;
	LtpSpan		spanBuf;

	if (lsoCmd)
	{
		if (*lsoCmd == '\0')
		{
			writeMemoNote("[?] No LSO command, can't update span",
					utoa(engineId));
			return 0;
		}
		else
		{
			if (strlen(lsoCmd) > MAX_SDRSTRING)
			{
				writeMemoNote("[?] Link service output command \
string too long.", lsoCmd);
				return 0;
			}
		}
	}

	if (maxSegmentSize)
	{
		if (maxSegmentSize < 508)
		{
			writeMemoNote("[i] Note max segment size is less than \
508", utoa(maxSegmentSize));
		}
	}

	sdr_begin_xn(ltpSdr);
	findSpan(engineId, &vspan, &vspanElt);
	if (vspanElt == 0)	/*	This is an unknown span.	*/
	{
		sdr_exit_xn(ltpSdr);
		writeMemoNote("[?] Unknown span", itoa(engineId));
		return 0;
	}

	addr = (Object) sdr_list_data(ltpSdr, vspan->spanElt);
	sdr_stage(ltpSdr, (char *) &spanBuf, addr, sizeof(LtpSpan));
	if (maxExportSessions == 0)
	{
		maxExportSessions = spanBuf.maxExportSessions;
	}

	if (maxImportSessions == 0)
	{
		maxImportSessions = spanBuf.maxImportSessions;
	}

	if (aggrSizeLimit == 0)
	{
		aggrSizeLimit = spanBuf.aggrSizeLimit;
	}

	if (aggrTimeLimit == 0)
	{
		aggrTimeLimit = spanBuf.aggrTimeLimit;
	}

	/*	All parameters validated, okay to update the span.	*/

	spanBuf.maxExportSessions = maxExportSessions;
	spanBuf.maxImportSessions = maxImportSessions;
	if (lsoCmd)
	{
		if (spanBuf.lsoCmd)
		{
			sdr_free(ltpSdr, spanBuf.lsoCmd);
		}

		spanBuf.lsoCmd = sdr_string_create(ltpSdr, lsoCmd);
	}

	spanBuf.remoteQtime = qTime;
	spanBuf.purge = purge ? 1 : 0;
	if (maxSegmentSize)
	{
		spanBuf.maxSegmentSize = maxSegmentSize;
	}

	spanBuf.aggrSizeLimit = aggrSizeLimit;
	if (aggrTimeLimit)
	{
		spanBuf.aggrTimeLimit = aggrTimeLimit;
	}

	sdr_write(ltpSdr, addr, (char *) &spanBuf, sizeof(LtpSpan));
	if (sdr_end_xn(ltpSdr) < 0)
	{
		putErrmsg("Can't update span.", itoa(engineId));
		return -1;
	}

	return 1;
}

int	removeSpan(unsigned long engineId)
{
	Sdr		ltpSdr = getIonsdr();
	LtpVspan	*vspan;
	PsmAddress	vspanElt;
	Object		spanElt;
	Object		spanObj;
			OBJ_POINTER(LtpSpan, span);

	/*	Must stop the span before trying to remove it.		*/

	sdr_begin_xn(ltpSdr);	/*	Lock memory.			*/
	findSpan(engineId, &vspan, &vspanElt);
	if (vspanElt == 0)	/*	This is an unknown span.	*/
	{
		sdr_exit_xn(ltpSdr);
		writeMemoNote("[?] Unknown span", itoa(engineId));
		return 0;
	}

	/*	All parameters validated.				*/

	stopSpan(vspan);
	sdr_exit_xn(ltpSdr);
	waitForSpan(vspan);
	sdr_begin_xn(ltpSdr);
	resetSpan(vspan);
	spanElt = vspan->spanElt;
	spanObj = (Object) sdr_list_data(ltpSdr, spanElt);
	GET_OBJ_POINTER(ltpSdr, LtpSpan, span, spanObj);
	if (sdr_list_length(ltpSdr, span->segments) != 0)
	{
		sdr_exit_xn(ltpSdr);
		writeMemoNote("[?] Span has backlog, can't be removed",
				itoa(engineId));
		return 0;
	}

	if (sdr_list_length(ltpSdr, span->importSessions) != 0
	|| sdr_list_length(ltpSdr, span->exportSessions) != 0)
	{
		sdr_exit_xn(ltpSdr);
		writeMemoNote("[?] Span has open sessions, can't be removed",
				itoa(engineId));
		return 0;
	}

	if (sdr_list_length(ltpSdr, span->deadImports) != 0)
	{
		sdr_exit_xn(ltpSdr);
		writeMemoNote("[?] Span has canceled sessions, can't be \
removed yet.", itoa(engineId));
		return 0;
	}

	/*	Okay to remove this span from the database.		*/

	dropSpan(vspan, vspanElt);
	if (span->lsoCmd)
	{
		sdr_free(ltpSdr, span->lsoCmd);
	}

	sdr_list_destroy(ltpSdr, span->exportSessions, NULL, NULL);
	sdr_list_destroy(ltpSdr, span->segments, NULL, NULL);
	sdr_list_destroy(ltpSdr, span->importSessions, NULL, NULL);
	sdr_hash_destroy(ltpSdr, span->importSessionsHash);
	sdr_list_destroy(ltpSdr, span->closedImports, NULL, NULL);
	sdr_list_destroy(ltpSdr, span->deadImports, NULL, NULL);
	sdr_free(ltpSdr, spanObj);
	sdr_list_delete(ltpSdr, spanElt, NULL, NULL);
	if (sdr_end_xn(ltpSdr) < 0)
	{
		putErrmsg("Can't remove span.", itoa(engineId));
		return -1;
	}

	return 1;
}

int	ltpStartSpan(unsigned long engineId)
{
	Sdr		ltpSdr = getIonsdr();
	LtpVspan	*vspan;
	PsmAddress	vspanElt;
	int		result = 1;

	sdr_begin_xn(ltpSdr);	/*	Just to lock memory.		*/
	findSpan(engineId, &vspan, &vspanElt);
	if (vspanElt == 0)
	{
		sdr_exit_xn(ltpSdr);	/*	Unlock memory.		*/
		writeMemoNote("[?] Unknown span", itoa(engineId));
		return 0;
	}

	startSpan(vspan);
	sdr_exit_xn(ltpSdr);	/*	Unlock memory.			*/
	return result;
}

void	ltpStopSpan(unsigned long engineId)
{
	Sdr		ltpSdr = getIonsdr();
	LtpVspan	*vspan;
	PsmAddress	vspanElt;

	sdr_begin_xn(ltpSdr);	/*	Just to lock memory.		*/
	findSpan(engineId, &vspan, &vspanElt);
	if (vspanElt == 0)	/*	This is an unknown span.	*/
	{
		sdr_exit_xn(ltpSdr);	/*	Unlock memory.		*/
		writeMemoNote("[?] Unknown span", itoa(engineId));
		return;
	}

	stopSpan(vspan);
	sdr_exit_xn(ltpSdr);	/*	Unlock memory.			*/
	waitForSpan(vspan);
	sdr_begin_xn(ltpSdr);	/*	Just to lock memory.		*/
	resetSpan(vspan);
	sdr_exit_xn(ltpSdr);	/*	Unlock memory.			*/
}

int	startExportSession(Sdr sdr, Object spanObj, LtpVspan *vspan)
{
	Object		dbobj;
	LtpSpan		span;
	LtpDB		ltpdb;
	unsigned long	sessionNbr;
	Object		sessionObj;
	Object		elt;
	ExportSession	session;

	CHKERR(vspan);
	sdr_begin_xn(sdr);
	sdr_stage(sdr, (char *) &span, spanObj, sizeof(LtpSpan));

	/*	Get next session number.				*/

	dbobj = getLtpDbObject();
	sdr_stage(sdr, (char *) &ltpdb, dbobj, sizeof(LtpDB));
	ltpdb.sessionCount++;
	sdr_write(sdr, dbobj, (char *) &ltpdb, sizeof(LtpDB));
	sessionNbr = ltpdb.sessionCount;

	/*	Record the session object in the database. The
	 *	exportSessions list element points to the session
	 *	structure.  exportSessionHash entry points to the
	 *	list element.						*/

	sessionObj = sdr_malloc(sdr, sizeof(ExportSession));
	if (sessionObj == 0
	|| (elt = sdr_list_insert_last(sdr, span.exportSessions,
			sessionObj)) == 0
	|| sdr_hash_insert(sdr, ltpdb.exportSessionsHash,
			(char *) &sessionNbr, elt, NULL) < 0)
	{
		putErrmsg("Can't start session.", NULL);
		sdr_cancel_xn(sdr);
		return -1;
	}

	/*	Populate session object in database.			*/

	memset((char *) &session, 0, sizeof(ExportSession));
	session.span = spanObj;
	session.sessionNbr = sessionNbr;
	encodeSdnv(&(session.sessionNbrSdnv), session.sessionNbr);
	session.svcDataObjects = sdr_list_create(sdr);
	session.redSegments = sdr_list_create(sdr);
	session.greenSegments = sdr_list_create(sdr);
	session.claims = sdr_list_create(sdr);
	sdr_write(sdr, sessionObj, (char *) &session, sizeof(ExportSession));

	/*	Note session address in span, then finish: unless span
	 *	is currently inactive (i.e., localXmitRate is currently
	 *	zero), give the buffer-empty semaphore so that the
	 *	pending service data object (if any) can be inserted
	 *	into the buffer.					*/

	span.currentExportSessionObj = sessionObj;
	sdr_write(sdr, spanObj, (char *) &span, sizeof(LtpSpan));
	if (vspan->localXmitRate > 0)
	{
		sm_SemGive(vspan->bufEmptySemaphore);
	}

	if (sdr_end_xn(sdr))
	{
		putErrmsg("Can't start session.", NULL);
		return -1;
	}

	return 0;
}

/*	*	*	LTP event mgt and access functions	*	*/

static Object	insertLtpTimelineEvent(LtpEvent *newEvent)
{
	Sdr	ltpSdr = getIonsdr();
	LtpDB	*ltpConstants = _ltpConstants();
	Object	eventObj;
	Object	elt;
		OBJ_POINTER(LtpEvent, event);

	CHKZERO(ionLocked());
	eventObj = sdr_malloc(ltpSdr, sizeof(LtpEvent));
	if (eventObj == 0)
	{
		putErrmsg("No space for timeline event.", NULL);
		return 0;
	}

	/*	Search list from newest to oldest, insert after last
		event with scheduled time less than or equal to that
		of the new event.					*/

	sdr_write(ltpSdr, eventObj, (char *) newEvent, sizeof(LtpEvent));
	for (elt = sdr_list_last(ltpSdr, ltpConstants->timeline); elt;
			elt = sdr_list_prev(ltpSdr, elt))
	{
		GET_OBJ_POINTER(ltpSdr, LtpEvent, event, sdr_list_data(ltpSdr,
				elt));
		if (event->scheduledTime <= newEvent->scheduledTime)
		{
			return sdr_list_insert_after(ltpSdr, elt, eventObj);
		}
	}

	return sdr_list_insert_first(ltpSdr, ltpConstants->timeline, eventObj);
}

/*	*	*	LTP client mgt and access functions	*	*/

int	ltpAttachClient(unsigned long clientSvcId)
{
	Sdr		ltpSdr = getIonsdr();
	LtpVclient	*client;

	if (clientSvcId > MAX_LTP_CLIENT_NBR)
	{
		putErrmsg("Client svc number over limit.", itoa(clientSvcId));
		return -1;
	}

	sdr_begin_xn(ltpSdr);	/*	Just to lock memory.		*/
	client = (_ltpvdb(NULL))->clients + clientSvcId;
	if (client->pid != ERROR)
	{
		if (sm_TaskExists(client->pid))
		{
			sdr_exit_xn(ltpSdr);
			if (client->pid == sm_TaskIdSelf())
			{
				return 0;
			}

			putErrmsg("Client service already in use.",
					itoa(clientSvcId));
			return -1;
		}

		/*	Application terminated without closing the
		 *	endpoint, so simply close it now.		*/

		client->pid = ERROR;
	}

	client->pid = sm_TaskIdSelf();
	sdr_exit_xn(ltpSdr);	/*	Unlock memory.			*/
	return 0;
}

void	ltpDetachClient(unsigned long clientSvcId)
{
	Sdr		ltpSdr = getIonsdr();
	LtpVclient	*client;

	if (clientSvcId > MAX_LTP_CLIENT_NBR)
	{
		return;
	}

	sdr_begin_xn(ltpSdr);	/*	Just to lock memory.		*/
	client = (_ltpvdb(NULL))->clients + clientSvcId;
	if (client->pid != sm_TaskIdSelf())
	{
		sdr_exit_xn(ltpSdr);
		putErrmsg("Can't close: not owner of client service.", NULL);
		return;
	}

	client->pid = -1;
	sdr_exit_xn(ltpSdr);	/*	Unlock memory.			*/
}

/*	*	*	Service interface functions	*	*	*/

int	enqueueNotice(LtpVclient *client, unsigned long sourceEngineId,
		unsigned long sessionNbr, unsigned long dataOffset,
		unsigned long dataLength, LtpNoticeType type,
		unsigned char reasonCode, unsigned char endOfBlock,
		Object data)
{
	Sdr		ltpSdr = getIonsdr();
	Object		noticeObj;
	LtpNotice	notice;

	CHKERR(client);
	if (client->pid == ERROR)
	{
		return 0;	/*	No client task to report to.	*/
	}

	CHKERR(ionLocked());
	noticeObj = sdr_malloc(ltpSdr, sizeof(LtpNotice));
	if (noticeObj == 0)
	{
		return -1;
	}

	if (sdr_list_insert_last(ltpSdr, client->notices, noticeObj) == 0)
	{
		return -1;
	}

	notice.sessionId.sourceEngineId = sourceEngineId;
	notice.sessionId.sessionNbr = sessionNbr;
	notice.dataOffset = dataOffset;
	notice.dataLength = dataLength;
	notice.type = type;
	notice.reasonCode = reasonCode;
	notice.endOfBlock = endOfBlock;
	notice.data = data;
	sdr_write(ltpSdr, noticeObj, (char *) &notice, sizeof(LtpNotice));

	/*	Tell client that a notice is waiting.			*/

	sm_SemGive(client->semaphore);
	return 0;
}

/*	*	*	Session management functions	*	*	*/

static void	getExportSession(unsigned long sessionNbr, Object *sessionObj)
{
	Sdr	ltpSdr = getIonsdr();
	Object	elt;

	CHKVOID(ionLocked());
	if (sdr_hash_retrieve(ltpSdr, (_ltpConstants())->exportSessionsHash,
			(char *) &sessionNbr, (Address *) &elt) == 1)
	{
		*sessionObj = sdr_list_data(ltpSdr, elt);
		return; 
	}

	/*	Unknown session.					*/

	*sessionObj = 0;
}

static void	getCanceledExport(unsigned long sessionNbr, Object *sessionObj,
			Object *sessionElt)
{
	Sdr	ltpSdr = getIonsdr();
		OBJ_POINTER(ExportSession, session);
	Object	elt;
	Object	obj;

	CHKVOID(ionLocked());
	for (elt = sdr_list_first(ltpSdr, (_ltpConstants())->deadExports); elt;
			elt = sdr_list_next(ltpSdr, elt))
	{
		obj = sdr_list_data(ltpSdr, elt);
		GET_OBJ_POINTER(ltpSdr, ExportSession, session, obj);
		if (session->sessionNbr == sessionNbr)
		{
			*sessionObj = obj;
			*sessionElt = elt;
			return;
		}
	}

	/*	Not a known canceled export session.			*/

	*sessionObj = 0;
	*sessionElt = 0;
}

static void	cancelEvent(LtpEventType type, unsigned long refNbr1,
			unsigned long refNbr2, unsigned long refNbr3)
{
	Sdr	ltpSdr = getIonsdr();
	Object	elt;
	Object	eventObj;
		OBJ_POINTER(LtpEvent, event);
	for (elt = sdr_list_first(ltpSdr, (_ltpConstants())->timeline); elt;
			elt = sdr_list_next(ltpSdr, elt))
	{
		eventObj = sdr_list_data(ltpSdr, elt);
		GET_OBJ_POINTER(ltpSdr, LtpEvent, event, eventObj);
		if (event->type == type && event->refNbr1 == refNbr1
		&& event->refNbr2 == refNbr2 && event->refNbr3 == refNbr3)
		{
			sdr_free(ltpSdr, eventObj);
			sdr_list_delete(ltpSdr, elt, NULL, NULL);
			return;
		}
	}
}

static void	destroyDataXmitSeg(Object dsElt, Object dsObj, LtpXmitSeg *ds)
{
	Sdr	ltpSdr = getIonsdr();

	CHKVOID(ionLocked());
	if (ds->pdu.ckptSerialNbr != 0)
	{
		cancelEvent(LtpResendCheckpoint, 0, ds->sessionNbr,
				ds->pdu.ckptSerialNbr);
	}

	if (ds->queueListElt)	/*	Queued for retransmission.	*/
	{
		sdr_list_delete(ltpSdr, ds->queueListElt, NULL, NULL);
	}

	sdr_free(ltpSdr, dsObj);
	sdr_list_delete(ltpSdr, dsElt, NULL, NULL);
}

static void	stopExportSession(ExportSession *session)
{
	Sdr	ltpSdr = getIonsdr();
	Object	elt;
	Object	segObj;
		OBJ_POINTER(LtpXmitSeg, ds);

	CHKVOID(ionLocked());
	while ((elt = sdr_list_first(ltpSdr, session->redSegments)) != 0)
	{
		segObj = sdr_list_data(ltpSdr, elt);
		GET_OBJ_POINTER(ltpSdr, LtpXmitSeg, ds, segObj);
		destroyDataXmitSeg(elt, segObj, ds);
	}

	while ((elt = sdr_list_first(ltpSdr, session->greenSegments)) != 0)
	{
		segObj = sdr_list_data(ltpSdr, elt);
		GET_OBJ_POINTER(ltpSdr, LtpXmitSeg, ds, segObj);
		destroyDataXmitSeg(elt, segObj, ds);
	}
}

static void	clearExportSession(ExportSession *session)
{
	Sdr	ltpSdr = getIonsdr();
	Object	elt;

	sdr_list_destroy(ltpSdr, session->redSegments, NULL, NULL);
	session->redSegments = 0;
	sdr_list_destroy(ltpSdr, session->greenSegments, NULL, NULL);
	session->greenSegments = 0;
	if (session->redPartLength > 0)
	{
		for (elt = sdr_list_first(ltpSdr, session->claims); elt;
				elt = sdr_list_next(ltpSdr, elt))
		{
			sdr_free(ltpSdr, sdr_list_data(ltpSdr, elt));
		}
	}
	else
	{
		if (sdr_list_length(ltpSdr, session->claims) > 0)
		{
			writeMemoNote("[?] Investigate: LTP all-Green session \
has reception claims", itoa(sdr_list_length(ltpSdr, session->claims)));
		}
	}

	sdr_list_destroy(ltpSdr, session->claims, NULL, NULL);
	session->claims = 0;
}

static void	closeExportSession(Object sessionObj)
{
	Sdr	ltpSdr = getIonsdr();
	LtpVdb	*ltpvdb = _ltpvdb(NULL);
	Object	dbobj = getLtpDbObject();
		OBJ_POINTER(ExportSession, session);
	LtpDB	db;
	Object	elt;
	Object	sdu;	/*	A ZcoRef object.			*/

	CHKVOID(ionLocked());
	GET_OBJ_POINTER(ltpSdr, ExportSession, session, sessionObj);
	sdr_stage(ltpSdr, (char *) &db, dbobj, sizeof(LtpDB));

	/*	Note that cancellation of an export session causes
	 *	the block's service data objects to be passed up to
	 *	the user in LtpExportSessionCanceled notices, destroys
	 *	the svcDataObjects list, and sets the svcDataObjects
	 *	list variable in the session object to zero.  In that
	 *	event, review of the service data objects in this
	 *	function is foregone.					*/

	if (session->svcDataObjects)
	{
		for (elt = sdr_list_first(ltpSdr, session->svcDataObjects); elt;
				elt = sdr_list_next(ltpSdr, elt))
		{
			sdu = sdr_list_data(ltpSdr, elt);

			/*	All service data units are received
			 *	by the client, in either Complete or
			 *	Canceled notices, and the client is
			 *	responsible for destroying them, so
			 *	we don't zco_destroy_reference here.
			 *	And since ltp_get_notice will reduce
			 *	db.heapSpaceBytesOccupied, we don't
			 *	do that either.				*/

			if (enqueueNotice(ltpvdb->clients
					+ session->clientSvcId, db.ownEngineId,
					session->sessionNbr, 0, 0,
					LtpExportSessionComplete, 0, 0, sdu)
					< 0)
			{
				putErrmsg("Can't post ExportSessionComplete \
notice.", NULL);
				sdr_cancel_xn(ltpSdr);
				return;
			}
		}

		sdr_write(ltpSdr, dbobj, (char *) &db, sizeof(LtpDB));
		sdr_list_destroy(ltpSdr, session->svcDataObjects, NULL, NULL);
	}

	clearExportSession(session);

	/*	Finally erase the session itself and authorize the
	 *	initiation of a new export session.			*/

	sdr_hash_remove(ltpSdr, db.exportSessionsHash,
			(char *) &(session->sessionNbr), (Address *) &elt);
	sdr_list_delete(ltpSdr, elt, NULL, NULL);
	sdr_free(ltpSdr, sessionObj);
#if LTPDEBUG
putErrmsg("Closed export session.", itoa(session->sessionNbr));
#endif
	sm_SemGive((_ltpvdb(NULL))->sessionSemaphore);
}

static void	getImportSession(LtpVspan *vspan, unsigned long sessionNbr,
			Object *sessionObj)
{
	Sdr	ltpSdr = getIonsdr();
		OBJ_POINTER(LtpSpan, span);
	Object	elt;

	CHKVOID(ionLocked());
	GET_OBJ_POINTER(ltpSdr, LtpSpan, span, sdr_list_data(ltpSdr,
			vspan->spanElt));
	if (sdr_hash_retrieve(ltpSdr, span->importSessionsHash,
			(char *) &sessionNbr, (Address *) &elt) == 1)
	{
		*sessionObj = sdr_list_data(ltpSdr, elt);
		return; 
	}

	/*	Unknown session.					*/

	*sessionObj = 0;
}

static int	sessionIsClosed(LtpVspan *vspan, unsigned long sessionNbr)
{
	Sdr		ltpSdr = getIonsdr();
			OBJ_POINTER(LtpSpan, span);
	Object		elt;
	unsigned long	closedSessionNbr;

	GET_OBJ_POINTER(ltpSdr, LtpSpan, span, sdr_list_data(ltpSdr,
			vspan->spanElt));

	/*	Closed-sessions list is in ascending session number
	 *	order.  Incoming segments are most likely to apply
	 *	to more recent sessions, so we search from end of
	 *	list rather from start.					*/

	for (elt = sdr_list_last(ltpSdr, span->closedImports); elt;
			elt = sdr_list_prev(ltpSdr, elt))
	{
		closedSessionNbr = (unsigned long) sdr_list_data(ltpSdr, elt);
		if (closedSessionNbr > sessionNbr)
		{
			continue;
		}

		if (closedSessionNbr == sessionNbr)
		{
			return 1;
		}

		break;		/*	No need to search further.	*/
	}

	/*	Not a recently closed import session.			*/

	return 0;
}

static void	getCanceledImport(LtpVspan *vspan, unsigned long sessionNbr,
			Object *sessionObj, Object *sessionElt)
{
	Sdr	ltpSdr = getIonsdr();
		OBJ_POINTER(LtpSpan, span);
		OBJ_POINTER(ImportSession, session);
	Object	elt;
	Object	obj;

	CHKVOID(ionLocked());
	GET_OBJ_POINTER(ltpSdr, LtpSpan, span, sdr_list_data(ltpSdr,
			vspan->spanElt));
	for (elt = sdr_list_first(ltpSdr, span->deadImports); elt;
			elt = sdr_list_next(ltpSdr, elt))
	{
		obj = sdr_list_data(ltpSdr, elt);
		GET_OBJ_POINTER(ltpSdr, ImportSession, session, obj);
		if (session->sessionNbr == sessionNbr)
		{
			*sessionObj = obj;
			*sessionElt = elt;
			return;
		}
	}

	/*	Not a known canceled import session.			*/

	*sessionObj = 0;
	*sessionElt = 0;
}

static void	destroyRsXmitSeg(Object rsElt, Object rsObj, LtpXmitSeg *rs)
{
	Sdr	ltpSdr = getIonsdr();
	Object	elt;

	CHKVOID(ionLocked());
	cancelEvent(LtpResendReport, rs->remoteEngineId, rs->sessionNbr,
			rs->pdu.rptSerialNbr);

	/*	No need to change state of rs->pdu.timer because the
		whole segment is about to vanish.			*/

	while ((elt = sdr_list_first(ltpSdr, rs->pdu.receptionClaims)))
	{
		sdr_free(ltpSdr, sdr_list_data(ltpSdr, elt));
		sdr_list_delete(ltpSdr, elt, NULL, NULL);
	}

	sdr_list_destroy(ltpSdr, rs->pdu.receptionClaims, NULL, NULL);
	if (rs->queueListElt)	/*	Queued for retransmission.	*/
	{
		sdr_list_delete(ltpSdr, rs->queueListElt, NULL, NULL);
	}

	sdr_free(ltpSdr, rsObj);
	sdr_list_delete(ltpSdr, rsElt, NULL, NULL);
}

static void	destroyDataRecvSeg(Object dsElt, Object dsObj, LtpRecvSeg *ds)
{
	Sdr	ltpSdr = getIonsdr();

	CHKVOID(ionLocked());
	sdr_free(ltpSdr, dsObj);
	sdr_list_delete(ltpSdr, dsElt, NULL, NULL);
}

static void	stopImportSession(ImportSession *session)
{
	Sdr	ltpSdr = getIonsdr();
	Object	dbobj = getLtpDbObject();
	Object	elt;
	Object	segObj;
	LtpDB	db;
		OBJ_POINTER(LtpXmitSeg, rs);
		OBJ_POINTER(LtpRecvSeg, ds);

	CHKVOID(ionLocked());
	while ((elt = sdr_list_first(ltpSdr, session->rsSegments)) != 0)
	{
		segObj = sdr_list_data(ltpSdr, elt);
		GET_OBJ_POINTER(ltpSdr, LtpXmitSeg, rs, segObj);
		destroyRsXmitSeg(elt, segObj, rs);
	}

	sdr_list_destroy(ltpSdr, session->rsSegments, NULL, NULL);
	session->rsSegments = 0;

	/*	Terminate reception of red-part data, release space,
	 *	and reduce heap reservation occupancy.			*/

	if (session->redSegments)
	{
		sdr_stage(ltpSdr, (char *) &db, dbobj, sizeof(LtpDB));
		while ((elt = sdr_list_first(ltpSdr, session->redSegments)))
		{
			segObj = sdr_list_data(ltpSdr, elt);
			GET_OBJ_POINTER(ltpSdr, LtpRecvSeg, ds, segObj);
			db.heapSpaceBytesOccupied -= sizeof(LtpRecvSeg);
			if (ds->heapAddress)	/*	Stored in heap.	*/
			{
				sdr_free(ltpSdr, ds->heapAddress);
				db.heapSpaceBytesOccupied -= ds->pdu.length;
			}

			destroyDataRecvSeg(elt, segObj, ds);
		}

		sdr_write(ltpSdr, dbobj, (char *) &db, sizeof(LtpDB));
		sdr_list_destroy(ltpSdr, session->redSegments, NULL, NULL);
		session->redSegments = 0;
	}

	if (session->blockFileRef)
	{
		zco_destroy_file_ref(ltpSdr, session->blockFileRef);
		session->blockFileRef = 0;
	}

	/*	If service data not delivered, then destroying the
	 *	file ref immediately causes its cleanup script to
	 *	be executed, unlinking the file.  Otherwise, the
	 *	service data object passed to the client is a ZCO
	 *	whose extents reference this file ref; the file ref
	 *	is retained until the last reference to that ZCO
	 *	is destroyed, at which time the file ref is destroyed
	 *	and the file is consequently unlinked.			*/
#if LTPDEBUG
putErrmsg("Stopped import session.", itoa(session->sessionNbr));
#endif
}

static void	noteClosedImport(Sdr sdr, LtpSpan *span, ImportSession *session)
{
	Object		elt;
	unsigned long	closedSessionNbr;
	Object		elt2;
	LtpEvent	event;
	time_t		currentTime;
	LtpVspan	*vspan;
	PsmAddress	vspanElt;

	/*	The closed-sessions list is in ascending session
	 *	number order, so we insert at the end of the list.	*/

	for (elt = sdr_list_last(sdr, span->closedImports); elt;
			elt = sdr_list_prev(sdr, elt))
	{
		closedSessionNbr = (unsigned long) sdr_list_data(sdr, elt);
		if (closedSessionNbr > session->sessionNbr)
		{
			continue;
		}

		break;
	}

	if (elt)
	{
		elt2 = sdr_list_insert_after(sdr, elt, session->sessionNbr);
	}
	else
	{
		elt2 = sdr_list_insert_first(sdr, span->closedImports,
				session->sessionNbr);
	}

	/*	Schedule removal of this closed-session note from
	 *	the list after a single round-trip time (plus 10
	 *	seconds of margin to allow for processing delay).
	 *
	 *	In the event of the sender unnecessarily retransmitting
	 *	a checkpoint segment before receiving a final RS and
	 *	closing the export session, that late checkpoint will
	 *	arrive (and be discarded) before this scheduled event.
	 *
	 *	An additional checkpoint can arrive after the removal
	 *	event -- and thereby resurrect the import session -- 
	 *	only if the export session is still open.  In that
	 *	case the export session's timeout sequence will
	 *	eventually result in re-closure of the reanimated
	 *	import session; there will be erroneous duplicate
	 *	data delivery, but no heap space leak.			*/

	memset((char *) &event, 0, sizeof(LtpEvent));
	event.parm = elt2;
	currentTime = getUTCTime();
	findSpan(span->engineId, &vspan, &vspanElt);
	event.scheduledTime = currentTime + vspan->owltOutbound
			+ vspan->owltInbound + 10;
	event.type = LtpForgetSession;
	oK(insertLtpTimelineEvent(&event));
}

static void	closeImportSession(Object sessionObj)
{
	Sdr	ltpSdr = getIonsdr();
		OBJ_POINTER(ImportSession, session);
		OBJ_POINTER(LtpSpan, span);
	Object	elt;

	CHKVOID(ionLocked());
	GET_OBJ_POINTER(ltpSdr, ImportSession, session, sessionObj);
	GET_OBJ_POINTER(ltpSdr, LtpSpan, span, session->span);
	noteClosedImport(ltpSdr, span, session);
	sdr_hash_remove(ltpSdr, span->importSessionsHash,
			(char *) &(session->sessionNbr), (Address *) &elt);
	sdr_list_delete(ltpSdr, elt, NULL, NULL);
	sdr_free(ltpSdr, sessionObj);
#if LTPDEBUG
putErrmsg("Closed import session.", itoa(session->sessionNbr));
#endif
}

static void	findReport(ImportSession *session, unsigned long rptSerialNbr,
			Object *rsElt, Object *rsObj)
{
	Sdr	ltpSdr = getIonsdr();
	Object	elt;
	Object	obj;
		OBJ_POINTER(LtpXmitSeg, rs);

	for (elt = sdr_list_first(ltpSdr, session->rsSegments); elt;
			elt = sdr_list_next(ltpSdr, elt))
	{
		obj = sdr_list_data(ltpSdr, elt);
		GET_OBJ_POINTER(ltpSdr, LtpXmitSeg, rs, obj);
		if (rs->pdu.rptSerialNbr == rptSerialNbr)
		{
			*rsElt = elt;
			*rsObj = obj;
			return;
		}
	}

	*rsElt = 0;
	*rsObj = 0;
}

static void	findCheckpoint(ExportSession *session,
			unsigned long ckptSerialNbr,
			Object *dsElt, Object *dsObj)
{
	Sdr	ltpSdr = getIonsdr();
	Object	elt;
	Object	obj;
		OBJ_POINTER(LtpXmitSeg, ds);

	for (elt = sdr_list_first(ltpSdr, session->redSegments); elt;
			elt = sdr_list_next(ltpSdr, elt))
	{
		obj = sdr_list_data(ltpSdr, elt);
		GET_OBJ_POINTER(ltpSdr, LtpXmitSeg, ds, obj);
		if (ds->pdu.ckptSerialNbr == ckptSerialNbr)
		{
			*dsElt = elt;
			*dsObj = obj;
			return;
		}
	}

	*dsElt = 0;
	*dsObj = 0;
}

/*	*	*	Segment issuance functions	*	*	*/

static void	serializeHeader(LtpXmitSeg *segment, Sdnv *engineIdSdnv,
			char **cursor)
{
	char	firstByte = LTP_VERSION;
	Sdnv	sessionNbrSdnv;

	firstByte <<= 4;
	firstByte += segment->pdu.segTypeCode;
	**cursor = firstByte;
	(*cursor)++;

	memcpy((*cursor), engineIdSdnv->text, engineIdSdnv->length);
	(*cursor) += engineIdSdnv->length;

	encodeSdnv(&sessionNbrSdnv, segment->sessionNbr);
	memcpy((*cursor), sessionNbrSdnv.text, sessionNbrSdnv.length);
	(*cursor) += sessionNbrSdnv.length;

	**cursor = 0;		/*	Both extension counts = 0.	*/
	(*cursor)++;
}

static void	serializeDataSegment(LtpXmitSeg *segment, char *buf)
{
	char	*cursor = buf;
	Sdnv	sdnv;

	/*	Origin is the local engine.				*/

	serializeHeader(segment, &((_ltpConstants())->ownEngineIdSdnv),
			&cursor);

	/*	Append client service number.				*/

	encodeSdnv(&sdnv, segment->pdu.clientSvcId);
	memcpy(cursor, sdnv.text, sdnv.length);
	cursor += sdnv.length;

	/*	Append offset of data within block.			*/

	encodeSdnv(&sdnv, segment->pdu.offset);
	memcpy(cursor, sdnv.text, sdnv.length);
	cursor += sdnv.length;

	/*	Append length of data.					*/

	encodeSdnv(&sdnv, segment->pdu.length);
	memcpy(cursor, sdnv.text, sdnv.length);
	cursor += sdnv.length;

	/*	If checkpoint, append checkpoint and report serial
	 *	numbers.						*/

	if (!(segment->pdu.segTypeCode & LTP_EXC_FLAG)	/*	Red.	*/
	&& segment->pdu.segTypeCode > 0)	/*	Checkpoint.	*/
	{
		/*	Append checkpoint serial number.		*/

		encodeSdnv(&sdnv, segment->pdu.ckptSerialNbr);
		memcpy(cursor, sdnv.text, sdnv.length);
		cursor += sdnv.length;

		/*	Append report serial number.			*/

		encodeSdnv(&sdnv, segment->pdu.rptSerialNbr);
		memcpy(cursor, sdnv.text, sdnv.length);
		cursor += sdnv.length;
	}

	/*	Note: client service data was copied into the trailing
	 *	bytes of the buffer before this function was called.	*/
}

static void	serializeReportSegment(LtpXmitSeg *segment, char *buf)
{
	Sdr	ltpSdr = getIonsdr();
	char	*cursor = buf;
	Sdnv	sdnv;
	Object	elt;
		OBJ_POINTER(LtpReceptionClaim, claim);

	/*	Report is from local engine, so origin is the remote
	 *	engine.							*/

	encodeSdnv(&sdnv, segment->remoteEngineId);
	serializeHeader(segment, &sdnv, &cursor);

	/*	Append report serial number.				*/

	encodeSdnv(&sdnv, segment->pdu.rptSerialNbr);
	memcpy(cursor, sdnv.text, sdnv.length);
	cursor += sdnv.length;

	/*	Append checkpoint serial number.			*/

	encodeSdnv(&sdnv, segment->pdu.ckptSerialNbr);
	memcpy(cursor, sdnv.text, sdnv.length);
	cursor += sdnv.length;

	/*	Append report upper bound.				*/

	encodeSdnv(&sdnv, segment->pdu.upperBound);
	memcpy(cursor, sdnv.text, sdnv.length);
	cursor += sdnv.length;

	/*	Append report lower bound.				*/

	encodeSdnv(&sdnv, segment->pdu.lowerBound);
	memcpy(cursor, sdnv.text, sdnv.length);
	cursor += sdnv.length;

	/*	Append count of reception claims.			*/

	encodeSdnv(&sdnv, sdr_list_length(ltpSdr,
			segment->pdu.receptionClaims));
	memcpy(cursor, sdnv.text, sdnv.length);
	cursor += sdnv.length;

	/*	Append all reception claims.				*/

	for (elt = sdr_list_first(ltpSdr, segment->pdu.receptionClaims); elt;
			elt = sdr_list_next(ltpSdr, elt))
	{
		GET_OBJ_POINTER(ltpSdr, LtpReceptionClaim, claim,
				sdr_list_data(ltpSdr, elt));
		encodeSdnv(&sdnv, claim->offset);
		memcpy(cursor, sdnv.text, sdnv.length);
		cursor += sdnv.length;
		encodeSdnv(&sdnv, claim->length);
		memcpy(cursor, sdnv.text, sdnv.length);
		cursor += sdnv.length;
	}
}

static void	serializeReportAckSegment(LtpXmitSeg *segment, char *buf)
{
	char	*cursor = buf;
	Sdnv	serialNbrSdnv;

	/*	Report is from remote engine, so origin is the local
	 *	engine.							*/

	serializeHeader(segment, &((_ltpConstants())->ownEngineIdSdnv),
			&cursor);

	/*	Append report serial number.				*/

	encodeSdnv(&serialNbrSdnv, segment->pdu.rptSerialNbr);
	memcpy(cursor, serialNbrSdnv.text, serialNbrSdnv.length);
}

static void	serializeCancelSegment(LtpXmitSeg *segment, char *buf)
{
	char	*cursor = buf;
	Sdnv	engineIdSdnv;

	if (segment->pdu.segTypeCode == LtpCS)
	{
		/*	Cancellation by sender, so origin is the
		 *	local engine.					*/

		serializeHeader(segment, &((_ltpConstants())->ownEngineIdSdnv),
				&cursor);
	}
	else
	{
		encodeSdnv(&engineIdSdnv, segment->remoteEngineId);
		serializeHeader(segment, &engineIdSdnv, &cursor);
	}

	/*	Append reason code.					*/

	*cursor = segment->pdu.reasonCode;
}

static void	serializeCancelAckSegment(LtpXmitSeg *segment, char *buf)
{
	char	*cursor = buf;
	Sdnv	engineIdSdnv;

	if (segment->pdu.segTypeCode == LtpCAR)
	{
		/*	Acknowledging cancel by receiver, so origin
		 *	is the local engine.				*/

		serializeHeader(segment, &((_ltpConstants())->ownEngineIdSdnv),
				&cursor);
	}
	else
	{
		encodeSdnv(&engineIdSdnv, segment->remoteEngineId);
		serializeHeader(segment, &engineIdSdnv, &cursor);
	}

	/*	No content for cancel acknowledgment, just header.	*/
}

static int	setTimer(LtpTimer *timer, Address timerAddr, time_t currentSec,
			LtpVspan *vspan, int segmentLength, LtpEvent *event)
{
	Sdr	ltpSdr = getIonsdr();
	LtpDB	ltpdb;
	int	radTime;
		OBJ_POINTER(LtpSpan, span);

	CHKERR(ionLocked());
	sdr_read(ltpSdr, (char *) &ltpdb, getLtpDbObject(), sizeof(LtpDB));
	if (vspan->localXmitRate == 0)	/*	Should never be, but...	*/
	{
		radTime = 0;		/*	Avoid divide by zero.	*/
	}
	else
	{
		radTime = (segmentLength + EST_LINK_OHD) / vspan->localXmitRate;
	}

	/*	Segment should arrive at the remote node following
	 *	about half of the local node's telecom processing
	 *	turnaround time (ownQtime) plus the time consumed in
	 *	simply radiating all the bytes of the segment
	 *	(including estimated link-layer overhead) at the
	 *	current transmission rate over this span, plus
	 *	the current outbound signal propagation time (owlt).	*/

	timer->segArrivalTime = currentSec + radTime + vspan->owltOutbound
			+ ((ltpdb.ownQtime >> 1) & 0x7fffffff);
	GET_OBJ_POINTER(ltpSdr, LtpSpan, span, sdr_list_data(ltpSdr,
			vspan->spanElt));

	/*	Following arrival of the segment, the response from
	 *	the remote node should arrive here following the
	 *	remote node's entire telecom processing turnaround
	 *	time (remoteQtime) plus the current inbound signal
	 *	propagation time (owlt) plus the other half of the
	 *	local node's telecom processing turnaround time.
	 *
	 *	Technically, we should also include in this interval
	 *	the time consumed in simply transmitting all bytes
	 *	of the response at the current fire rate over this
	 *	span.  But in practice this interval is too small
	 *	to be worth the trouble of managing it (i.e, it is
	 *	not known unless the remote node is currently
	 *	transmitting, it needs to be backed out and later
	 *	restored on suspension/resumption of the link because
	 *	the remote fire rate might change, etc.).		*/

	timer->ackDeadline = timer->segArrivalTime
			+ span->remoteQtime + vspan->owltInbound
			+ ((ltpdb.ownQtime >> 1) & 0x7fffffff);
	if (vspan->remoteXmitRate > 0)
	{
		event->scheduledTime = timer->ackDeadline;
		if (insertLtpTimelineEvent(event) == 0)
		{
			putErrmsg("Can't set timer.", NULL);
			return -1;
		}

		timer->state = LtpTimerRunning;
	}
	else
	{
		timer->state = LtpTimerSuspended;
	}

	sdr_write(ltpSdr, timerAddr, (char *) timer, sizeof(LtpTimer));
	return 0;
}

static int	readFromExportBlock(char *buffer, Object svcDataObjects,
			unsigned long offset, unsigned long length)
{
	Sdr		ltpSdr = getIonsdr();
	Object		elt;
	Object		sdu;	/*	Each member of list is a ZCO.	*/
	unsigned int	sduLength;
	Object		handle;
	int		totalBytesRead = 0;
	ZcoReader	reader;
	unsigned int	bytesToRead;
	int		bytesRead;

	for (elt = sdr_list_first(ltpSdr, svcDataObjects); elt;
			elt = sdr_list_next(ltpSdr, elt))
	{
		sdu = sdr_list_data(ltpSdr, elt);
		sduLength = zco_length(ltpSdr, sdu);
		if (offset >= sduLength)
		{
			offset -= sduLength;	/*	Skip over SDU.	*/
			continue;
		}

		/*	Reading from a ZCO reference precludes future
		 *	re-reading of the same data from the same
		 *	reference.  So we make an additional reference
		 *	just for the purpose of this read request.	*/

		handle = zco_add_reference(ltpSdr, sdu);
		zco_start_transmitting(ltpSdr, handle, &reader);
		zco_track_file_offset(&reader);
		if (offset > 0)
		{
			if (zco_transmit(ltpSdr, &reader, offset, NULL) < 0)
			{
				putErrmsg("Failed skipping offset.", NULL);
				return -1;
			}

			sduLength -= offset;
			offset = 0;
		}

		bytesToRead = length;
		if (bytesToRead > sduLength)
		{
			bytesToRead = sduLength;
		}

		bytesRead = zco_transmit(ltpSdr, &reader, bytesToRead,
				buffer + totalBytesRead);
		if (bytesRead != bytesToRead)
		{
			putErrmsg("Failed reading SDU.", NULL);
			return -1;
		}

		totalBytesRead += bytesRead;
		length -= bytesRead;
		zco_destroy_reference(ltpSdr, handle);
		if (length == 0)	/*	Have read enough.	*/
		{
			break;
		}
	}

	return totalBytesRead;
}

int	ltpDequeueOutboundSegment(LtpVspan *vspan, char **buf)
{
	Sdr		ltpSdr = getIonsdr();
	LtpVdb		*ltpvdb = _ltpvdb(NULL);
	LtpDB		*ltpConstants = _ltpConstants();
	Object		spanObj;
	LtpSpan		spanBuf;
	Object		elt;
	char		memo[64];
	Object		segAddr;
	LtpXmitSeg	segment;
	int		segmentLength;
	Object		sessionObj;
	Object		sessionElt;
	ExportSession	xsessionBuf;
	time_t		currentTime;
	LtpEvent	event;
	LtpTimer	*timer;
	ImportSession	rsessionBuf;

	CHKERR(vspan);
	CHKERR(buf);
	*buf = (char *) psp(getIonwm(), vspan->segmentBuffer);
	sdr_begin_xn(ltpSdr);
	spanObj = sdr_list_data(ltpSdr, vspan->spanElt);
	sdr_stage(ltpSdr, (char *) &spanBuf, spanObj, sizeof(LtpSpan));
	elt = sdr_list_first(ltpSdr, spanBuf.segments);
	while (elt == 0 || vspan->localXmitRate == 0)
	{
		sdr_exit_xn(ltpSdr);

		/*	Wait until ltpmeter has announced an outbound
		 *	segment by giving span's segSemaphore.		*/

		if (sm_SemTake(vspan->segSemaphore) < 0)
		{
			putErrmsg("LSO can't take segment semaphore.",
					itoa(vspan->engineId));
			return -1;
		}

		if (sm_SemEnded(vspan->segSemaphore))
		{
			isprintf(memo, sizeof memo,
					"[i] LSO to engine %lu is stopped.",
					vspan->engineId);
			writeMemo(memo);
			return 0;
		}

		sdr_begin_xn(ltpSdr);
		sdr_stage(ltpSdr, (char *) &spanBuf, spanObj, sizeof(LtpSpan));
		elt = sdr_list_first(ltpSdr, spanBuf.segments);
	}

	/*	Got next outbound segment.  Remove it from the queue
	 *	for this span.						*/

	segAddr = sdr_list_data(ltpSdr, elt);
	sdr_stage(ltpSdr, (char *) &segment, segAddr, sizeof(LtpXmitSeg));
	sdr_list_delete(ltpSdr, elt, NULL, NULL);
	segment.queueListElt = 0;

	/*	If segment is a data segment other than a checkpoint,
	 *	remove it from the relevant list in its session.
	 *	(Note that segments are retained in these lists only
	 *	to support ExportSession cancellation prior to
	 *	transmission of the segments.)				*/

	if (segment.pdu.segTypeCode == LtpDsRed	/*	Non-ckpt red.	*/
	|| segment.pdu.segTypeCode == LtpDsGreen
	|| segment.pdu.segTypeCode == LtpDsGreenEOB)
	{
		sdr_list_delete(ltpSdr, segment.sessionListElt, NULL, NULL);
		segment.sessionListElt = 0;
	}

	/*	Copy segment's content into buffer.			*/

	segmentLength = segment.ohdLength;
	if (segment.segmentClass == LtpDataSeg)
	{
		segmentLength += segment.pdu.length;

		/*	Load client service data at the end of the
		 *	segment first, before filling in the header.	*/

		if (readFromExportBlock((*buf) + segment.ohdLength,
				segment.pdu.block, segment.pdu.offset,
				segment.pdu.length) < 0)
		{
			putErrmsg("Can't read data from export block.", NULL);
			sdr_cancel_xn(ltpSdr);
			return -1;
		}
	}

	/*	Remove segment from database if possible, i.e.,
	 *	if it needn't ever be retransmitted.  Otherwise
	 *	rewrite it to record change of queueListElt to 0.	*/

	switch (segment.pdu.segTypeCode)
	{
	case LtpDsRedCheckpoint:	/*	Checkpoint.		*/
	case LtpDsRedEORP:		/*	Checkpoint.		*/
	case LtpDsRedEOB:		/*	Checkpoint.		*/
	case LtpRS:			/*	Report.			*/
		sdr_write(ltpSdr, segAddr, (char *) &segment,
				sizeof(LtpXmitSeg));
		break;

	default:	/*	No need to retain this segment.		*/
		sdr_free(ltpSdr, segAddr);
	}

	/*	Post timeout event as necessary.			*/

	currentTime = getUTCTime();
	event.parm = 0;
	switch (segment.pdu.segTypeCode)
	{
	case LtpDsRedCheckpoint:	/*	Checkpoint.		*/
	case LtpDsRedEORP:		/*	Checkpoint.		*/
	case LtpDsRedEOB:		/*	Checkpoint.		*/
		event.type = LtpResendCheckpoint;
		event.refNbr1 = 0;
		event.refNbr2 = segment.sessionNbr;
		event.refNbr3 = segment.pdu.ckptSerialNbr;
		timer = &segment.pdu.timer;
		if (setTimer(timer, segAddr + FLD_OFFSET(timer, &segment),
				currentTime, vspan, segmentLength, &event) < 0)
		{
			putErrmsg("Can't schedule event.", NULL);
			sdr_cancel_xn(ltpSdr);
			return -1;
		}

		break;

	case 8:
		event.type = LtpResendReport;
		event.refNbr1 = segment.remoteEngineId;
		event.refNbr2 = segment.sessionNbr;
		event.refNbr3 = segment.pdu.rptSerialNbr;
		timer = &segment.pdu.timer;
		if (setTimer(timer, segAddr + FLD_OFFSET(timer, &segment),
				currentTime, vspan, segmentLength, &event) < 0)
		{
			putErrmsg("Can't schedule event.", NULL);
			sdr_cancel_xn(ltpSdr);
			return -1;
		}

		break;

	case 12:
		getCanceledExport(segment.sessionNbr, &sessionObj, &sessionElt);
		if (sessionObj == 0)
		{
			break;		/*	Session already closed.	*/
		}

		sdr_stage(ltpSdr, (char *) &xsessionBuf, sessionObj,
				sizeof(ExportSession));
		event.type = LtpResendXmitCancel;
		event.refNbr1 = 0;
		event.refNbr2 = segment.sessionNbr;
		event.refNbr3 = 0;
		timer = &(xsessionBuf.timer);
		if (setTimer(timer, sessionObj + FLD_OFFSET(timer,
				&xsessionBuf), currentTime, vspan,
				segmentLength, &event) < 0)
		{
			putErrmsg("Can't schedule event.", NULL);
			sdr_cancel_xn(ltpSdr);
			return -1;
		}

		break;

	case 14:
		if (segment.sessionObj == 0)
		{
			break;		/*	No need for timer.	*/
		}

		/*	An ImportSession has been started for this
		 *	session, so must assure that this cancellation
		 *	is delivered -- unless the session is already
		 *	closed.						*/

		getCanceledImport(vspan, segment.sessionNbr, &sessionObj,
				&sessionElt);
		if (sessionObj == 0)
		{
			break;		/*	Session already closed.	*/
		}

		sdr_stage(ltpSdr, (char *) &rsessionBuf, sessionObj,
				sizeof(ImportSession));
		event.type = LtpResendRecvCancel;
		event.refNbr1 = segment.remoteEngineId;
		event.refNbr2 = segment.sessionNbr;
		event.refNbr3 = 0;
		timer = &(rsessionBuf.timer);
		if (setTimer(timer, sessionObj + FLD_OFFSET(timer,
				&rsessionBuf), currentTime, vspan,
				segmentLength, &event) < 0)
		{
			putErrmsg("Can't schedule event.", NULL);
			sdr_cancel_xn(ltpSdr);
			return -1;
		}

	default:
		break;
	}

	/*	Handle end-of-block if necessary.			*/

	if (segment.pdu.segTypeCode < 8
	&& (segment.pdu.segTypeCode & LTP_FLAG_1)
	&& (segment.pdu.segTypeCode & LTP_FLAG_0))
	{
		/*	If initial transmission of EOB, notify the
		 *	client and release the span so that ltpmeter
		 *	can start segmenting the next block.		*/

		if (segment.pdu.timer.expirationCount == 0)
		{
			if (enqueueNotice(ltpvdb->clients 
				+ segment.pdu.clientSvcId,
				ltpConstants->ownEngineId, segment.sessionNbr,
				0, 0, LtpXmitComplete, 0, 0, 0) < 0)
			{
				putErrmsg("Can't post XmitComplete notice.",
						NULL);
				sdr_cancel_xn(ltpSdr);
				return -1;
			}

			sdr_write(ltpSdr, spanObj, (char *) &spanBuf,
					sizeof(LtpSpan));
		}

		/*	If entire block is green, close the session
		 *	(since normal session closure on red-part
		 *	completion or cancellation won't happen).	*/

		if (segment.pdu.segTypeCode == LtpDsGreenEOB)
		{
			getExportSession(segment.sessionNbr, &sessionObj);
			if (sessionObj)
			{
				sdr_read(ltpSdr, (char *) &xsessionBuf,
					sessionObj, sizeof(ExportSession));
				if (xsessionBuf.totalLength != 0)
				{
					/*	Found the session.	*/

					if (xsessionBuf.redPartLength == 0)
					{
						closeExportSession(sessionObj);
					}
				}
			}
		}
	}

	/*	Now serialize the segment overhead and prepend that
	 *	overhead to the content of the segment (if any), and
	 *	return to link service output process.			*/

	if (segment.pdu.segTypeCode < 8)
	{
		serializeDataSegment(&segment, *buf);
	}
	else
	{
		switch (segment.pdu.segTypeCode)
		{
			case 8:		/*	Report.			*/
				serializeReportSegment(&segment, *buf);
				break;

			case 9:		/*	Report acknowledgment.	*/
				serializeReportAckSegment(&segment, *buf);
				break;

			case 12:	/*	Cancel by sender.	*/
			case 14:	/*	Cancel by receiver.	*/
				serializeCancelSegment(&segment, *buf);
				break;

			case 13:	/*	Cancel acknowledgment.	*/
			case 15:	/*	Cancel acknowledgment.	*/
				serializeCancelAckSegment(&segment, *buf);
				break;

			default:
				break;
		}
	}

	if (sdr_end_xn(ltpSdr))
	{
		putErrmsg("Can't get outbound segment for span.", NULL);
		return -1;
	}

	if (ltpvdb->watching & WATCH_g)
	{
		putchar('g');
		fflush(stdout);
	}

	return segmentLength;
}

/*	*	Control segment construction functions		*	*/

static void	signalLso(unsigned int engineId)
{
	LtpVspan	*vspan;
	PsmAddress	vspanElt;

	findSpan(engineId, &vspan, &vspanElt);
	if (vspan != NULL && vspan->localXmitRate > 0)
	{
		/*	Tell LSO that output is waiting.	*/

		sm_SemGive(vspan->segSemaphore);
	}
}

static Object	enqueueCancelReqSegment(LtpXmitSeg *segment,
			LtpSpan *span, Sdnv *sourceEngineSdnv,
			unsigned long sessionNbr,
			LtpCancelReasonCode reasonCode)
{
	Sdr	ltpSdr = getIonsdr();
	Sdnv	sdnv;
	Object	segmentObj;

	CHKZERO(ionLocked());
	segment->sessionNbr = sessionNbr;
	segment->remoteEngineId = span->engineId;
	encodeSdnv(&sdnv, sessionNbr);
	segment->ohdLength = 1 + sourceEngineSdnv->length + sdnv.length + 1 + 1;
	segment->sessionListElt = 0;
	segment->segmentClass = LtpMgtSeg;
	segment->pdu.headerExtensionsCount = 0;
	segment->pdu.trailerExtensionsCount = 0;
	segment->pdu.reasonCode = reasonCode;
	segmentObj = sdr_malloc(ltpSdr, sizeof(LtpXmitSeg));
	if (segmentObj == 0)
	{
		return 0;
	}

	segment->queueListElt = sdr_list_insert_last(ltpSdr, span->segments,
			segmentObj);
	if (segment->queueListElt == 0)
	{
		return 0;
	}

	sdr_write(ltpSdr, segmentObj, (char *) segment, sizeof(LtpXmitSeg));
	return segmentObj;
}

static int	constructSourceCancelReqSegment(LtpSpan *span,
			Sdnv *sourceEngineSdnv, unsigned long sessionNbr,
			Object sessionObj, LtpCancelReasonCode reasonCode)
{
	LtpXmitSeg	segment;
	Object		segmentObj;

	/*	Cancellation by the local engine.			*/

	memset((char *) &segment, 0, sizeof(LtpXmitSeg));
	segment.pdu.segTypeCode = LtpCS;
	segmentObj = enqueueCancelReqSegment(&segment, span, sourceEngineSdnv,
			sessionNbr, reasonCode);
	if (segmentObj == 0)
	{
		return -1;
	}

	signalLso(span->engineId);
	return 0;
}

static int	cancelSessionBySender(ExportSession *session,
			Object sessionObj, LtpCancelReasonCode reasonCode)
{
	Sdr		ltpSdr = getIonsdr();
	LtpVdb		*ltpvdb = _ltpvdb(NULL);
	Object		dbobj = getLtpDbObject();
	LtpDB		db;
	Object		spanObj = session->span;
	LtpSpan		span;
	LtpVspan	*vspan;
	PsmAddress	vspanElt;
	Object		elt;
	Object		sdu;	/*	A ZcoRef object.		*/

	CHKERR(ionLocked());
	session->reasonCode = reasonCode;	/*	(For CS resend.)*/

	/*	Span now has room for another session to start.		*/

	sdr_stage(ltpSdr, (char *) &span, spanObj, sizeof(LtpSpan));
	if (sessionObj == span.currentExportSessionObj)
	{
		/*	Finish up session so it can be reported.	*/

		session->clientSvcId = span.clientSvcIdOfBufferedBlock;
		encodeSdnv(&(session->clientSvcIdSdnv), session->clientSvcId);
		session->totalLength = span.lengthOfBufferedBlock;
		session->redPartLength = span.redLengthOfBufferedBlock;

		/*	Reinitialize span's block buffer.		*/

		span.ageOfBufferedBlock = 0;
		span.lengthOfBufferedBlock = 0;
		span.redLengthOfBufferedBlock = 0;
		span.clientSvcIdOfBufferedBlock = 0;
		span.currentExportSessionObj = 0;
		sdr_write(ltpSdr, spanObj, (char *) &span, sizeof(LtpSpan));

		/*	Start a replacement export session.		*/

		findSpan(span.engineId, &vspan, &vspanElt);
		if (vspanElt == 0
		|| startExportSession(ltpSdr, spanObj, vspan) < 0)
		{
			putErrmsg("Can't start replacement session.",
					utoa(span.engineId));
			return -1;
		}
	}
	else	/*	Like closeExportSession, triggers ltpmeter.	*/
	{
		sm_SemGive(ltpvdb->sessionSemaphore);
	}

	if (ltpvdb->watching & WATCH_CS)
	{
		putchar('{');
		fflush(stdout);
	}

	sdr_stage(ltpSdr, (char *) &db, dbobj, sizeof(LtpDB));
	stopExportSession(session);
	for (elt = sdr_list_first(ltpSdr, session->svcDataObjects); elt;
			elt = sdr_list_next(ltpSdr, elt))
	{
		sdu = sdr_list_data(ltpSdr, elt);
		if (enqueueNotice(ltpvdb->clients + session->clientSvcId,
			db.ownEngineId, session->sessionNbr, 0, 0,
			LtpExportSessionCanceled, reasonCode, 0, sdu) < 0)
		{
			putErrmsg("Can't post ExportSessionCanceled notice.",
					NULL);
			return -1;
		}
	}

	sdr_write(ltpSdr, dbobj, (char *) &db, sizeof(LtpDB));
	sdr_list_destroy(ltpSdr, session->svcDataObjects, NULL, NULL);
	session->svcDataObjects = 0;
	clearExportSession(session);
	sdr_write(ltpSdr, sessionObj, (char *) session, sizeof(ExportSession));

	/*	Remove session from active sessions pool, so that the
	 *	cancellation won't affect flow control.			*/

	sdr_hash_remove(ltpSdr, db.exportSessionsHash,
			(char *) &(session->sessionNbr), (Address *) &elt);
	sdr_list_delete(ltpSdr, elt, NULL, NULL);

	/*	Insert into list of canceled sessions instead.		*/

	elt = sdr_list_insert_last(ltpSdr, db.deadExports, sessionObj);

	/*	Finally, inform receiver of cancellation.		*/

	return constructSourceCancelReqSegment(&span, &db.ownEngineIdSdnv,
			session->sessionNbr, sessionObj, reasonCode);
}

static int	constructDestCancelReqSegment(LtpSpan *span,
			Sdnv *sourceEngineSdnv, unsigned long sessionNbr,
			Object sessionObj, LtpCancelReasonCode reasonCode)
{
	LtpXmitSeg	segment;
	Object		segmentObj;

	/*	Cancellation by the local engine.			*/

	memset((char *) &segment, 0, sizeof(LtpXmitSeg));
	segment.pdu.segTypeCode = LtpCR;
	segment.sessionObj = sessionObj;
	segmentObj = enqueueCancelReqSegment(&segment, span, sourceEngineSdnv,
			sessionNbr, reasonCode);
	if (segmentObj == 0)	/*	Failed to send segment.		*/
	{
		return -1;
	}

	signalLso(span->engineId);
	return 0;
}

static int	cancelSessionByReceiver(ImportSession *session,
			Object sessionObj, LtpCancelReasonCode reasonCode)
{
	Sdr	ltpSdr = getIonsdr();
	LtpVdb	*ltpvdb = _ltpvdb(NULL);
		OBJ_POINTER(LtpSpan, span);
	Object	elt;

	CHKERR(ionLocked());
	GET_OBJ_POINTER(ltpSdr, LtpSpan, span, session->span);
	if (enqueueNotice(ltpvdb->clients + session->clientSvcId,
			span->engineId, session->sessionNbr, 0, 0,
			LtpImportSessionCanceled, reasonCode, 0, 0) < 0)
	{
		putErrmsg("Can't post ImportSessionCanceled notice.", NULL);
		return -1;
	}

	if (ltpvdb->watching & WATCH_CR)
	{
		putchar('[');
		fflush(stdout);
	}

	stopImportSession(session);
	session->reasonCode = reasonCode;	/*	For resend.	*/
	sdr_write(ltpSdr, sessionObj, (char *) session, sizeof(ImportSession));

	/*	Remove session from active sessions pool, so that the
	 *	cancellation won't affect flow control.			*/

	sdr_hash_remove(ltpSdr, span->importSessionsHash,
			(char *) &(session->sessionNbr), (Address *) &elt);
	sdr_list_delete(ltpSdr, elt, NULL, NULL);

	/*	Insert into list of canceled sessions instead.		*/

	elt = sdr_list_insert_last(ltpSdr, span->deadImports, sessionObj);

	/*	Finally, inform sender of cancellation.			*/

	return constructDestCancelReqSegment(span, &(span->engineIdSdnv),
			session->sessionNbr, sessionObj, reasonCode);
}

static Object	enqueueAckSegment(Object spanObj, Object segmentObj)
{
	Sdr	ltpSdr = getIonsdr();
		OBJ_POINTER(LtpSpan, span);
	Object	elt;
		OBJ_POINTER(LtpXmitSeg, segment);

	CHKZERO(ionLocked());
	GET_OBJ_POINTER(ltpSdr, LtpSpan, span, spanObj);
	for (elt = sdr_list_first(ltpSdr, span->segments); elt;
			elt = sdr_list_next(ltpSdr, elt))
	{
		GET_OBJ_POINTER(ltpSdr, LtpXmitSeg, segment,
				sdr_list_data(ltpSdr, elt));
		switch (segment->pdu.segTypeCode)
		{
		case LtpRS:
		case LtpRAS:
		case LtpCAS:
		case LtpCAR:
			continue;

		default:	/*	Found first non-ACK segment.	*/
			break;			/*	Out of switch.	*/
		}

		break;				/*	Out of loop.	*/
	}

	if (elt)
	{
		elt = sdr_list_insert_before(ltpSdr, elt, segmentObj);
	}
	else
	{
		elt = sdr_list_insert_last(ltpSdr, span->segments, segmentObj);
	}

	return elt;
}

static int	constructCancelAckSegment(LtpXmitSeg *segment, Object spanObj,
			Sdnv *sourceEngineSdnv, unsigned long sessionNbr)
{
	Sdr	ltpSdr = getIonsdr();
		OBJ_POINTER(LtpSpan, span);
	Sdnv	sdnv;
	Object	segmentObj;

	CHKERR(ionLocked());
	GET_OBJ_POINTER(ltpSdr, LtpSpan, span, spanObj);
	segment->sessionNbr = sessionNbr;
	segment->remoteEngineId = span->engineId;
	encodeSdnv(&sdnv, sessionNbr);
	segment->ohdLength = 1 + sourceEngineSdnv->length + sdnv.length + 1;
	segment->sessionListElt = 0;
	segment->segmentClass = LtpMgtSeg;
	segment->pdu.headerExtensionsCount = 0;
	segment->pdu.trailerExtensionsCount = 0;
	segmentObj = sdr_malloc(ltpSdr, sizeof(LtpXmitSeg));
	if (segmentObj == 0)
	{
		return -1;
	}

	segment->queueListElt = enqueueAckSegment(spanObj, segmentObj);
	if (segment->queueListElt == 0)
	{
		return -1;
	}

	sdr_write(ltpSdr, segmentObj, (char *) segment, sizeof(LtpXmitSeg));
	signalLso(span->engineId);
	return 0;
}

static int	constructSourceCancelAckSegment(Object spanObj,
			Sdnv *sourceEngineSdnv, unsigned long sessionNbr)
{
	LtpXmitSeg	segment;

	/*	Cancellation by the remote engine (source), ack by
	 *	local engine (destination).				*/

	memset((char *) &segment, 0, sizeof(LtpXmitSeg));
	segment.pdu.segTypeCode = LtpCAS;
	return constructCancelAckSegment(&segment, spanObj, sourceEngineSdnv,
			sessionNbr);
}

static int	constructDestCancelAckSegment(Object spanObj,
			Sdnv *sourceEngineSdnv, unsigned long sessionNbr)
{
	LtpXmitSeg	segment;

	/*	Cancellation by the remote engine (destination), ack
	 *	by local engine (source).				*/

	memset((char *) &segment, 0, sizeof(LtpXmitSeg));
	segment.pdu.segTypeCode = LtpCAR;
	return constructCancelAckSegment(&segment, spanObj, sourceEngineSdnv,
			sessionNbr);
}

static int	initializeRs(LtpXmitSeg *rs, int baseOhdLength,
			int checkpointSerialNbrSdnvLength,
			unsigned long rsLowerBound)
{
	Sdnv	sdnv;

	rs->ohdLength = baseOhdLength;
	if (rs->pdu.rptSerialNbr == 0)
	{
		do rs->pdu.rptSerialNbr = rand();
			while (rs->pdu.rptSerialNbr == 0);
	}
	else
	{
		rs->pdu.rptSerialNbr++;
	}
	encodeSdnv(&sdnv, rs->pdu.rptSerialNbr);
	rs->ohdLength += sdnv.length;
	rs->ohdLength += checkpointSerialNbrSdnvLength;
	rs->pdu.lowerBound = rsLowerBound;
	encodeSdnv(&sdnv, rs->pdu.lowerBound);
	rs->ohdLength += sdnv.length;
	rs->pdu.receptionClaims = sdr_list_create(getIonsdr());
	if (rs->pdu.receptionClaims == 0)
	{
		return -1;
	}

	return 0;
}

static int	constructReceptionClaim(LtpXmitSeg *rs, int lowerBound,
			int upperBound)
{
	Sdr			ltpSdr = getIonsdr();
	Object			claimObj;
	LtpReceptionClaim	claim;
	Sdnv			sdnv;

	CHKERR(ionLocked());
	claimObj = sdr_malloc(ltpSdr, sizeof(LtpReceptionClaim));
	if (claimObj == 0)
	{
		return -1;
	}

	claim.offset = lowerBound;
	encodeSdnv(&sdnv, claim.offset);
	rs->ohdLength += sdnv.length;
	claim.length = upperBound - lowerBound;
	encodeSdnv(&sdnv, claim.length);
	rs->ohdLength += sdnv.length;
	sdr_write(ltpSdr, claimObj, (char *) &claim, sizeof(LtpReceptionClaim));
	if (sdr_list_insert_last(ltpSdr, rs->pdu.receptionClaims, claimObj)
			== 0)
	{
		return -1;
	}

	return 0;
}

static int	constructRs(LtpXmitSeg *rs, int claimCount,
			ImportSession *session, Object spanObj)
{
	Sdr	ltpSdr = getIonsdr();
	Sdnv	sdnv;
	Object	rsObj;
		OBJ_POINTER(LtpSpan, span);

	CHKERR(ionLocked());
	encodeSdnv(&sdnv, rs->pdu.upperBound);
	rs->ohdLength += sdnv.length;
	encodeSdnv(&sdnv, claimCount);
	rs->ohdLength += sdnv.length;
	rsObj = sdr_malloc(ltpSdr, sizeof(LtpXmitSeg));
	if (rsObj == 0)
	{
		return -1;
	}

	rs->sessionListElt = sdr_list_insert_last(ltpSdr, session->rsSegments,
			rsObj);
	rs->queueListElt = enqueueAckSegment(spanObj, rsObj);
	if (rs->sessionListElt == 0 || rs->queueListElt == 0)
	{
		return -1;
	}

	sdr_write(ltpSdr, rsObj, (char *) rs, sizeof(LtpXmitSeg));
	GET_OBJ_POINTER(ltpSdr, LtpSpan, span, spanObj);
	signalLso(span->engineId);
	return 0;
}

static int	sendReport(ImportSession *session, Object sessionObj,
			unsigned long checkpointSerialNbr,
			unsigned long reportSerialNbr,
			unsigned long reportUpperBound)
{
	Sdr		ltpSdr = getIonsdr();
	unsigned long	reportLowerBound = 0;
	Object		elt;
	Object		obj;
			OBJ_POINTER(LtpXmitSeg, oldRpt);
			OBJ_POINTER(LtpSpan, span);
	int		baseOhdLength;
	LtpXmitSeg	rsBuf;
	Sdnv		checkpointSerialNbrSdnv;
	unsigned long	lowerBound;
	unsigned long	upperBound;
	int		claimCount;
			OBJ_POINTER(LtpRecvSeg, ds);
#if LTPDEBUG
int		shortfall;
char		buf[256];
#endif

	CHKERR(ionLocked());
	if (session->reportsCount >= MAX_NBR_OF_REPORTS)
	{
		/*	We can send one more report if it's the
		 *	one saying "got everything".  Otherwise,
		 *	time to give up.				*/

		if (session->redPartLength == 0
		|| session->redPartReceived != session->redPartLength)
		{
#if LTPDEBUG
putErrmsg("Too many reports, canceling session.", itoa(session->sessionNbr));
#endif
			return cancelSessionByReceiver(session, sessionObj,
					LtpRetransmitLimitExceeded);
		}
	}

	session->reportsCount++;
	if (reportSerialNbr != 0)
	{
		/*	Sending report in response to a checkpoint
		 *	that cites a prior report.  If that report
		 *	still exists (not yet acknowledged), use
		 *	its lower bound as the lower bound for this
		 *	report.						*/

		findReport(session, reportSerialNbr, &elt, &obj);
		if (elt)
		{
			GET_OBJ_POINTER(ltpSdr, LtpXmitSeg, oldRpt, obj);
			reportLowerBound = oldRpt->pdu.lowerBound;
		}
	}

	upperBound = lowerBound = reportLowerBound;
	GET_OBJ_POINTER(ltpSdr, LtpSpan, span, session->span);
	baseOhdLength = 1 + span->engineIdSdnv.length
			+ session->sessionNbrSdnv.length + 1;

	/*	Set all values that will be common to all report
	 *	segments of this report.				*/

	memset((char *) &rsBuf, 0, sizeof(LtpXmitSeg));
	rsBuf.sessionNbr = session->sessionNbr;
	rsBuf.remoteEngineId = span->engineId;
	rsBuf.segmentClass = LtpReportSeg;
	rsBuf.pdu.segTypeCode = LtpRS;
	rsBuf.pdu.ckptSerialNbr = checkpointSerialNbr;
	rsBuf.pdu.rptSerialNbr = reportSerialNbr;
	encodeSdnv(&checkpointSerialNbrSdnv, checkpointSerialNbr);

	/*	Initialize the first report segment and start adding
	 *	reception claims.					*/

	if (initializeRs(&rsBuf, baseOhdLength, checkpointSerialNbrSdnv.length,
			lowerBound) < 0)
	{
		return -1;
	}

	claimCount = 0;
	for (elt = sdr_list_first(ltpSdr, session->redSegments); elt;
			elt = sdr_list_next(ltpSdr, elt))
	{
		GET_OBJ_POINTER(ltpSdr, LtpRecvSeg, ds,
				sdr_list_data(ltpSdr, elt));
		if (ds->pdu.offset >= reportUpperBound)
		{
			break;	/*	No need to check any further.	*/
		}

		if (ds->pdu.offset == upperBound)
		{
			upperBound += ds->pdu.length;
			continue;	/*	Contiguous extents.	*/
		}

		/*	Gap found; end of reception claim, so post it
		 *	unless it is of zero length (i.e., at start).	*/

		if (upperBound != lowerBound)
		{
			if (constructReceptionClaim(&rsBuf, lowerBound,
					upperBound) < 0)
			{
				return -1;
			}

			claimCount++;
			rsBuf.pdu.upperBound = upperBound;
		}

		lowerBound = ds->pdu.offset;
		upperBound = lowerBound + ds->pdu.length;
		if (claimCount < MAX_CLAIMS_PER_RS)
		{
			continue;
		}

		/*	Must ship this RS and start another.		*/

		if (constructRs(&rsBuf, claimCount, session, session->span) < 0)
		{
			return -1;
		}

		claimCount = 0;
		if (initializeRs(&rsBuf, baseOhdLength,
			checkpointSerialNbrSdnv.length, lowerBound) < 0)
		{
			return -1;
		}
	}

	if (upperBound == lowerBound)	/*	Nothing to report.	*/
	{
#if LTPDEBUG
putErrmsg("No report, upper bound == lower bound.", itoa(session->sessionNbr));
#endif
		return 0;
	}

	/*	Add last reception claim.				*/

	if (constructReceptionClaim(&rsBuf, lowerBound, upperBound) < 0)
	{
		return -1;
	}

	claimCount++;
	rsBuf.pdu.upperBound = upperBound;

	/*	Ship final RS of this report.				*/

	if (constructRs(&rsBuf, claimCount, session, session->span) < 0)
	{
		return -1;
	}

#if LTPDEBUG
shortfall = session->redPartLength - session->redPartReceived;
if (shortfall)
{
sprintf(buf, "Reporting %d bytes missing.", shortfall);
putErrmsg(buf, itoa(session->sessionNbr));
}
else putErrmsg("Reporting all data received.", itoa(session->sessionNbr));
#endif
	return 0;
}

static int	constructReportAckSegment(LtpSpan *span, Object spanObj,
			unsigned long sessionNbr, unsigned long reportSerialNbr)
{
	Sdr		ltpSdr = getIonsdr();
	LtpXmitSeg	segment;
	Sdnv		sdnv;
	unsigned long	sessionNbrLength;
	unsigned long	serialNbrLength;
	Object		segmentObj;

	/*	Report acknowledgment by the local engine (sender).	*/

	CHKERR(ionLocked());
	memset((char *) &segment, 0, sizeof(LtpXmitSeg));
	segment.sessionNbr = sessionNbr;
	segment.remoteEngineId = span->engineId;
	encodeSdnv(&sdnv, sessionNbr);
	sessionNbrLength = sdnv.length;
	encodeSdnv(&sdnv, reportSerialNbr);
	serialNbrLength = sdnv.length;
	segment.ohdLength = 1 + (_ltpConstants())->ownEngineIdSdnv.length
			+ sessionNbrLength + 1 + serialNbrLength;
	segment.sessionListElt = 0;
	segment.segmentClass = LtpMgtSeg;
	segment.pdu.segTypeCode = LtpRAS;
	segment.pdu.headerExtensionsCount = 0;
	segment.pdu.trailerExtensionsCount = 0;
	segment.pdu.rptSerialNbr = reportSerialNbr;
	segmentObj = sdr_malloc(ltpSdr, sizeof(LtpXmitSeg));
	if (segmentObj == 0)
	{
		return -1;
	}

	segment.queueListElt = enqueueAckSegment(spanObj, segmentObj);
	if (segment.queueListElt == 0)
	{
		return -1;
	}

	sdr_write(ltpSdr, segmentObj, (char *) &segment, sizeof(LtpXmitSeg));
	signalLso(span->engineId);
	return 0;
}

/*	*	*	Segment handling functions	*	*	*/

static int	startImportSession(Object spanObj, unsigned long sessionNbr,
			ImportSession *sessionBuf, Object *sessionObj,
			unsigned long clientSvcId, LtpDB *db)
{
	Sdr	ltpSdr = getIonsdr();
	Object	elt;
		OBJ_POINTER(LtpSpan, span);

	CHKERR(ionLocked());
	GET_OBJ_POINTER(ltpSdr, LtpSpan, span, spanObj);
	while (sdr_list_length(ltpSdr, span->importSessions)
			>= span->maxImportSessions)
	{
		/*	Limit reached.  Must cancel oldest session.	*/

		*sessionObj = sdr_list_data(ltpSdr, sdr_list_first(ltpSdr,
				span->importSessions));
		sdr_stage(ltpSdr, (char *) sessionBuf, *sessionObj,
				sizeof(ImportSession));
		if (cancelSessionByReceiver(sessionBuf, *sessionObj,
				LtpCancelByEngine) < 0)
		{
			putErrmsg("LTP failed canceling oldest session.", NULL);
			return -1;
		}
	}

	/*	importSessions list element points to the session
	 *	structure.  importSessionsHash entry points to the
	 *	list element.						*/

	*sessionObj = sdr_malloc(ltpSdr, sizeof(ImportSession));
	if (*sessionObj == 0
	|| (elt = sdr_list_insert_last(ltpSdr, span->importSessions,
			*sessionObj)) == 0
	|| sdr_hash_insert(ltpSdr, span->importSessionsHash,
			(char *) &sessionNbr, elt, NULL) < 0)
	{
		return -1;
	}

#if LTPDEBUG
putErrmsg("Opened import session.", utoa(sessionNbr));
#endif
	memset((char *) sessionBuf, 0, sizeof(ImportSession));
	sessionBuf->sessionNbr = sessionNbr;
	encodeSdnv(&(sessionBuf->sessionNbrSdnv), sessionNbr);
	sessionBuf->clientSvcId = clientSvcId;
	sessionBuf->redSegments = sdr_list_create(ltpSdr);
	sessionBuf->rsSegments = sdr_list_create(ltpSdr);
	sessionBuf->span = spanObj;
	if (sessionBuf->redSegments == 0
	|| sessionBuf->rsSegments == 0)
	{
		putErrmsg("Can't create import session.", NULL);
		return -1;
	}

	/*	Make sure the initialized session is recorded to
	 *	the database.						*/

	sdr_write(ltpSdr, *sessionObj, (char *) sessionBuf,
			sizeof(ImportSession));
	return 0;
}

static int	createBlockFile(LtpSpan *span, ImportSession *session)
{
	Sdr	ltpSdr = getIonsdr();
	char	cwd[200];
	char	name[SDRSTRING_BUFSZ];
	int	fd;

	if (igetcwd(cwd, sizeof cwd) == NULL)
	{
		putErrmsg("Can't get CWD for block file name.", NULL);
		return -1;
	}

	isprintf(name, sizeof name, "%s%cltpblock.%lu.%lu", cwd,
		ION_PATH_DELIMITER, span->engineId, session->sessionNbr);
	fd = iopen(name, O_WRONLY | O_CREAT, 0666);
	if (fd < 0)
	{
		putSysErrmsg("Can't create block file", name);
		return -1;
	}

	close(fd);
	session->blockFileRef = zco_create_file_ref(ltpSdr, name, "");
	if (session->blockFileRef == 0)
	{
		putErrmsg("Can't create block file reference.", NULL);
		return -1;
	}

	return 0;
}

static long	insertDataSegment(ImportSession *session, LtpRecvSeg *segment,
			LtpPdu *pdu, Object *segmentObj)
{
	Sdr		ltpSdr = getIonsdr();
	long		segUpperBound;
	Object		elt;
			OBJ_POINTER(LtpRecvSeg, ds);
	unsigned long	redPartUpperBound;

	CHKERR(ionLocked());
	segUpperBound = segment->pdu.offset + segment->pdu.length;
	if (session->redPartLength > 0)	/*	EORP received.		*/
	{
		if (segUpperBound > session->redPartLength)
		{
#if LTPDEBUG
putErrmsg("discarded segment", itoa(segment->pdu.offset));
#endif
			return 0;	/*	Beyond end of red part.	*/
		}
	}

	for (elt = sdr_list_first(ltpSdr, session->redSegments); elt;
			elt = sdr_list_next(ltpSdr, elt))
	{
		GET_OBJ_POINTER(ltpSdr, LtpRecvSeg, ds,
				sdr_list_data(ltpSdr, elt));
		if (ds->pdu.offset < segment->pdu.offset)
		{
			redPartUpperBound = ds->pdu.offset + ds->pdu.length;
			if (redPartUpperBound > segment->pdu.offset)
			{
#if LTPDEBUG
putErrmsg("discarded segment", itoa(segment->pdu.offset));
#endif
				return 0;	/*	Overlapping.	*/
			}

			continue;
		}

		/*	Previously received red segment does not start
		 *	before start of this one.			*/

		if (ds->pdu.offset < segUpperBound)
		{
#if LTPDEBUG
putErrmsg("discarded segment", itoa(segment->pdu.offset));
#endif
			return 0;		/*	Overlapping.	*/
		}

		/*	Previously received red segment does not start
		 *	before end of this one.				*/

		break;
	}

	session->redPartReceived += segment->pdu.length;
	*segmentObj = sdr_malloc(ltpSdr, sizeof(LtpRecvSeg));
	if (*segmentObj == 0)
	{
		return -1;
	}

	if (elt)
	{
		segment->sessionListElt = sdr_list_insert_before(ltpSdr, elt,
				*segmentObj);
	}
	else
	{
		segment->sessionListElt = sdr_list_insert_last(ltpSdr,
				session->redSegments, *segmentObj);
	}

	return segUpperBound;
}

static int	writeBlockExtentToFile(ImportSession *session,
			LtpRecvSeg *segment, char *from, unsigned long length)
{
	Sdr	ltpSdr = getIonsdr();
	char	fileName[SDRSTRING_BUFSZ];
	int	fd;
	long	fileLength;

	segment->heapAddress = 0;
	oK(zco_file_ref_path(ltpSdr, session->blockFileRef, fileName,
				sizeof fileName));

	/*	Note: it's possible for a session to be closed,
	 *	causing the blockFileRef to be "destroyed", while
	 *	there are still ZCO references to the file in the
	 *	delivery queue -- and for a late retransmitted
	 *	segment for this session to arrive during this
	 *	window.  In that case a new session would be created
	 *	and a new blockFileRef for the same file would be
	 *	created, but the file itself would still exist and
	 *	therefore NOT be created.  Bust as soon as the last
	 *	ZCO reference was delivered the file would be
	 *	automatically unlinked by the destruction of the
	 *	old file reference, so the next retransmitted
	 *	segment for this old session would be recorded
	 *	in a file that no longer exists -- an error.  To
	 *	avert this, we retain the option to temporarily
	 *	recreate that file for as long as is needed to deal
	 *	with the leftover retransmitted segments.		*/

	fd = iopen(fileName, O_WRONLY | O_CREAT, 0666);
	if (fd < 0)
	{
		putSysErrmsg("Can't open block file", fileName);
		return -1;
	}

	fileLength = (long) lseek(fd, 0, SEEK_END);
	if (fileLength < 0)
	{
		putSysErrmsg("Can't seek to end of block file", fileName);
		return -1;
	}

	segment->fileOffset = fileLength;
	if (write(fd, from, length) < 0)
	{
		putSysErrmsg("Can't append to block file", fileName);
		return -1;
	}

	close(fd);
	return 0;
}

static int	deliverSvcData(LtpVclient *client, unsigned long sourceEngineId,
			unsigned long sessionNbr, ImportSession *session)
{
	Sdr	ltpSdr = getIonsdr();
	LtpVdb	*ltpvdb = _ltpvdb(NULL);
	Object	dbobj = getLtpDbObject();
	Object	svcDataObject;
	LtpDB	db;
	Object	elt;
	Object	segObj;
		OBJ_POINTER(LtpRecvSeg, segment);

	/*	Use the redSegments list to construct a ZCO that
	 *	encapsulates the concatenated content of all data
	 *	segments in the block in *transmission* order.
	 *
	 *	In the process, terminate reception of red-part data
	 *	for this session and adjust heap reservation occupancy:
	 *	back out the individual data segments' lengths and
	 *	replace with the size of the concatenated ZCO.		*/

	svcDataObject = zco_create(ltpSdr, 0, 0, 0, 0);
	if (svcDataObject == 0)
	{
		putErrmsg("Can't create service data object.", NULL);
		return -1;
	}

	sdr_stage(ltpSdr, (char *) &db, dbobj, sizeof(LtpDB));
	while ((elt = sdr_list_first(ltpSdr, session->redSegments)))
	{
		segObj = sdr_list_data(ltpSdr, elt);
		GET_OBJ_POINTER(ltpSdr, LtpRecvSeg, segment, segObj);
		db.heapSpaceBytesOccupied -= sizeof(LtpRecvSeg);
		if (segment->heapAddress)	/*	Data in heap.	*/
		{
			if (zco_append_extent(ltpSdr, svcDataObject,
					ZcoSdrSource, segment->heapAddress, 0,
					segment->pdu.length) < 0)
			{
				putErrmsg("Can't add heap ZCO extent.", NULL);
				return -1;
			}

			db.heapSpaceBytesOccupied -= segment->pdu.length;
		}
		else			/*	Data written to file.	*/
		{
			if (zco_append_extent(ltpSdr, svcDataObject,
					ZcoFileSource, session->blockFileRef,
					segment->fileOffset,
					segment->pdu.length) < 0)
			{
				putErrmsg("Can't add file ZCO extent.", NULL);
				return -1;
			}
		}

		destroyDataRecvSeg(elt, segObj, segment);
	}

	db.heapSpaceBytesOccupied += zco_occupancy(ltpSdr, svcDataObject);
	sdr_write(ltpSdr, dbobj, (char *) &db, sizeof(LtpDB));
	sdr_list_destroy(ltpSdr, session->redSegments, NULL, NULL);
	session->redSegments = 0;

	/*	Pass the block content ZCO to the client service.	*/

	if (enqueueNotice(client, sourceEngineId, sessionNbr, 0,
			session->redPartLength, LtpRecvRedPart, 0,
			session->endOfBlockRecd, svcDataObject) < 0)
	{
		putErrmsg("Can't post RecvRedPart notice.", NULL);
		return -1;
	}

	/*	Print watch character if necessary, and return.		*/

	if (ltpvdb->watching & WATCH_t)
	{
		putchar('t');
		fflush(stdout);
	}

	return 0;
}

static int	handleGreenDataSegment(LtpPdu *pdu, char *cursor,
			Object sessionObj, Object *clientSvcData)
{
	Sdr		ltpSdr = getIonsdr();
	Object		dbobj = _ltpdbObject(NULL);
	ImportSession	sessionBuf;
	Object		segmentElt;
	Object		segmentObj;
			OBJ_POINTER(LtpRecvSeg, seg);
	LtpDB		db;

	if (sessionObj)
	{
		sdr_stage(ltpSdr, (char *) &sessionBuf, sessionObj,
				sizeof(ImportSession));
		segmentElt = sdr_list_last(ltpSdr, sessionBuf.redSegments);
		if (segmentElt)
		{
			segmentObj = sdr_list_data(ltpSdr, segmentElt);
			GET_OBJ_POINTER(ltpSdr, LtpRecvSeg, seg, segmentObj);
			if (pdu->offset < (seg->pdu.offset + seg->pdu.length))
			{
				/*	Miscolored segment: green data
				 *	before end of red.		*/

#if LTPDEBUG
putErrmsg("Cancel by receiver.", itoa(sessionBuf.sessionNbr));
#endif
				cancelSessionByReceiver(&sessionBuf, sessionObj,
						LtpMiscoloredSegment);
				return 1;
			}
		}
	}

	sdr_stage(ltpSdr, (char *) &db, dbobj, sizeof(LtpDB));
	if (db.heapSpaceBytesOccupied + pdu->length
			> db.heapSpaceBytesReserved)
	{
		/*	To avert possible DOS attack, silently discard
		 *	this segment.					*/
#if LTPDEBUG
putErrmsg("Can't handle green data, would exceed LTP heap space reservation.",
utoa(pdu->length));
#endif
		return 0;
	}

	if ((*clientSvcData = zco_create(ltpSdr, ZcoSdrSource,
			sdr_insert(ltpSdr, cursor, pdu->length),
			0, pdu->length)) == 0)
	{
		putErrmsg("Can't record green segment data.", NULL);
		return -1;
	}

	db.heapSpaceBytesOccupied += zco_occupancy(ltpSdr, *clientSvcData);
	sdr_write(ltpSdr, dbobj, (char *) &db, sizeof(LtpDB));
	return 1;
}

static int	handleDataSegment(unsigned long sourceEngineId, LtpDB *ltpdb,
			unsigned long sessionNbr, LtpRecvSeg *segment,
			LtpPdu *pdu, char **cursor, int *bytesRemaining)
{
	Sdr		ltpSdr = getIonsdr();
	LtpVdb		*ltpvdb = _ltpvdb(NULL);
	Object		dbobj = _ltpdbObject(NULL);
	LtpDB		db;
	unsigned long	ckptSerialNbr;
	unsigned long	rptSerialNbr;
	LtpVspan	*vspan;
	PsmAddress	vspanElt;
	Object		sessionObj = 0;
	Object		sessionElt;
	ImportSession	sessionBuf;
	Object		spanObj;
			OBJ_POINTER(LtpSpan, span);
	LtpVclient	*client;
	int		result;
	Object		clientSvcData = 0;
	unsigned long	segUpperBound;
	Object		segmentObj = 0;

	/*	First finish parsing the segment.			*/

	extractSdnv(&(pdu->clientSvcId), cursor, bytesRemaining);
	extractSdnv(&(pdu->offset), cursor, bytesRemaining);
	extractSdnv(&(pdu->length), cursor, bytesRemaining);
	if (pdu->segTypeCode > 0 && !(pdu->segTypeCode & LTP_EXC_FLAG))
	{
		/*	This segment is an LTP checkpoint.		*/

		extractSdnv(&ckptSerialNbr, cursor, bytesRemaining);
		extractSdnv(&rptSerialNbr, cursor, bytesRemaining);
	}

	/*	At this point, the remaining bytes should all be
	 *	client service data.					*/

	if (pdu->length > *bytesRemaining)
	{
#if LTPDEBUG
putErrmsg("Discarded data segment.", itoa(sessionNbr));
#endif
		/*	Malformed segment: data length is overstated.
		 *	Segment must be discarded.			*/

		return 0;
	}

	/*	Now process the data.					*/

	sdr_begin_xn(ltpSdr);
	findSpan(sourceEngineId, &vspan, &vspanElt);
	if (vspanElt == 0)
	{
#if LTPDEBUG
putErrmsg("Discarded data segment.", itoa(sessionNbr));
#endif
		/*	Segment is from an unknown engine, so we
		 *	can't process it.				*/

		sdr_exit_xn(ltpSdr);
		return 0;
	}

	if (vspan->receptionRate == 0 && ltpdb->enforceSchedule == 1)
	{
#if LTPDEBUG
putErrmsg("Discarding stray segment.", itoa(sessionNbr));
#endif
		/*	Segment is from an engine that is not supposed
		 *	to be sending at this time, so we treat it as
		 *	a misdirected transmission.			*/

		sdr_exit_xn(ltpSdr);
		return 0;
	}

	if (sessionIsClosed(vspan, sessionNbr))
	{
#if LTPDEBUG
putErrmsg("Discarding late segment.", itoa(sessionNbr));
#endif
		/*	Segment is for a session that is already
		 *	closed, so we don't care about it.		*/

		sdr_exit_xn(ltpSdr);
		return 0;
	}

	spanObj = sdr_list_data(ltpSdr, vspan->spanElt);
	GET_OBJ_POINTER(ltpSdr, LtpSpan, span, spanObj);
	getImportSession(vspan, sessionNbr, &sessionObj);
	segment->segmentClass = LtpDataSeg;
	if (pdu->clientSvcId > MAX_LTP_CLIENT_NBR
	|| (client = ltpvdb->clients + pdu->clientSvcId)->pid == ERROR)
	{
		/*	Data segment is for an unknown client service,
		 *	so must discard it and cancel the session.	*/

		if (sessionObj)	/*	Session already exists.		*/
		{
			sdr_stage(ltpSdr, (char *) &sessionBuf, sessionObj,
					sizeof(ImportSession));
#if LTPDEBUG
putErrmsg("Cancel by receiver.", itoa(sessionBuf.sessionNbr));
#endif
			cancelSessionByReceiver(&sessionBuf, sessionObj,
					LtpClientSvcUnreachable);
		}
		else
		{
			if (constructDestCancelReqSegment(span,
					&(span->engineIdSdnv), sessionNbr, 0,
					LtpClientSvcUnreachable) < 0)
			{
				putErrmsg("Can't send CR segment.", NULL);
				sdr_cancel_xn(ltpSdr);
				return -1;
			}
		}

		if (sdr_end_xn(ltpSdr) < 0)
		{
			putErrmsg("Can't handle data segment.", NULL);
			return -1;
		}

#if LTPDEBUG
putErrmsg("Discarded data segment.", itoa(sessionNbr));
#endif
		return 0;
	}

	if (pdu->segTypeCode & LTP_EXC_FLAG)
	{
		/*	This is a green-part data segment; deliver
		 *	immediately to client service.			*/

		if (sessionNbr == vspan->greenSessionNbr)
		{
			if (pdu->offset < vspan->greenOffset)
			{
				vspan->greenOffset = pdu->offset;
			}
		}
		else
		{
			vspan->greenSessionNbr = sessionNbr;
			vspan->greenOffset = pdu->offset;
		}

		result = handleGreenDataSegment(pdu, *cursor, sessionObj,
				&clientSvcData);
		if (result < 1)
		{
			sdr_cancel_xn(ltpSdr);
			return result;
		}

		if (clientSvcData)
		{
			enqueueNotice(client, sourceEngineId, sessionNbr,
					pdu->offset, pdu->length,
					LtpRecvGreenSegment, 0,
					(pdu->segTypeCode == LtpDsGreenEOB),
					clientSvcData);
		}

		if (sdr_end_xn(ltpSdr) < 0)
		{
			putErrmsg("Can't handle green-part segment.", NULL);
			return -1;
		}

		return 1;	/*	Green-part data handled okay.	*/
	}

	/*	This is a red-part data segment.			*/

	if (sessionNbr == vspan->greenSessionNbr
	&& (pdu->offset + pdu->length) > vspan->greenOffset)
	{
		/*	Miscolored segment: red after start of green.	*/

		if (sessionObj)		/*	Session exists.		*/
		{
			sdr_stage(ltpSdr, (char *) &sessionBuf, sessionObj,
					sizeof(ImportSession));
#if LTPDEBUG
putErrmsg("Cancel by receiver.", itoa(sessionBuf.sessionNbr));
#endif
			cancelSessionByReceiver(&sessionBuf, sessionObj,
					LtpMiscoloredSegment);
		}
		else	/*	Just send cancel segment to sender.	*/
		{
			if (constructDestCancelReqSegment(span,
					&(span->engineIdSdnv), sessionNbr,
					0, LtpMiscoloredSegment) < 0)
			{
				putErrmsg("Can't send CR segment.", NULL);
				sdr_cancel_xn(ltpSdr);
				return -1;
			}
		}

		if (sdr_end_xn(ltpSdr) < 0)
		{
			putErrmsg("Can't handle miscolored red seg.", NULL);
			return -1;
		}

#if LTPDEBUG
putErrmsg("Discarded data segment.", itoa(sessionNbr));
#endif
		return 0;
	}

	sdr_stage(ltpSdr, (char *) &db, dbobj, sizeof(LtpDB));

	/*	Data segment must be accepted into an import session,
	 *	unless that session is already canceled.		*/

	if (sessionObj)	/*	Active import session found.		*/
	{
		sdr_stage(ltpSdr, (char *) &sessionBuf, sessionObj,
				sizeof(ImportSession));
		if (sessionBuf.redSegments == 0)
		{
			/*	Reception already completed, just
			 *	waiting for report acknowledgment.
			 *	Discard the segment.			*/

			if (sdr_end_xn(ltpSdr) < 0)
			{
				putErrmsg("Can't handle data segment.", NULL);
				return -1;
			}
#if LTPDEBUG
putErrmsg("Discarded data segment.", itoa(sessionNbr));
#endif
			return 0;
		}
	}
	else		/*	Active import session not found.	*/
	{
		getCanceledImport(vspan, sessionNbr, &sessionObj, &sessionElt);
		if (sessionObj)
		{
			/*	Session exists but has already been
			 *	canceled.  Discard the segment.		*/

			if (sdr_end_xn(ltpSdr) < 0)
			{
				putErrmsg("Can't handle data segment.", NULL);
				return -1;
			}
#if LTPDEBUG
putErrmsg("Discarded data segment.", itoa(sessionNbr));
#endif
			return 0;
		}

		/*	Must start a new import session.		*/

		if (startImportSession(spanObj, sessionNbr, &sessionBuf,
				&sessionObj, pdu->clientSvcId, &db) < 0)
		{
			putErrmsg("Can't create reception session.", NULL);
			sdr_cancel_xn(ltpSdr);
			return -1;
		}

		sdr_write(ltpSdr, dbobj, (char *) &db, sizeof(LtpDB));
		if (pdu->offset != 0	/*	Not segment #1.	*/
		|| (pdu->segTypeCode != LtpDsRedEORP
				&& pdu->segTypeCode != LtpDsRedEOB))
		{
			/*	This is a large (i.e., multi-segment)
			 *	block; must receive it into a file.	*/
		
			if (createBlockFile(span, &sessionBuf) < 0)
			{
				putErrmsg("Can't receive large block.", NULL);
				sdr_cancel_xn(ltpSdr);
				return -1;
			}
		}
	}

	segment->sessionObj = sessionObj;
	segUpperBound = insertDataSegment(&sessionBuf, segment, pdu,
			&segmentObj);
	switch (segUpperBound)
	{
	case 0:
		/*	Segment was found to be useless.  Discard it.	*/

		return sdr_end_xn(ltpSdr);

	case -1:
		putErrmsg("Can't insert segment into ImportSession.", NULL);
		sdr_cancel_xn(ltpSdr);
		return -1;
	}

	/*	Write the red-part reception segment to the database.	*/

	if (sessionBuf.blockFileRef == 0)	/*	Store in heap.	*/
	{
		if (db.heapSpaceBytesOccupied + pdu->length
				> db.heapSpaceBytesReserved)
		{
			/*	To avert possible DOS attack, silently
			 *	discard this segment.			*/
#if LTPDEBUG
putErrmsg("Can't handle red data, would exceed LTP heap space reservation.",
utoa(pdu->length));
#endif
			sdr_cancel_xn(ltpSdr);
			return 0;
		}

		segment->fileOffset = 0;
		segment->heapAddress = sdr_insert(ltpSdr, *cursor, pdu->length);
		if (segment->heapAddress == 0)
		{
			putErrmsg("Can't record block extent.", NULL);
			sdr_cancel_xn(ltpSdr);
			return -1;
		}

		db.heapSpaceBytesOccupied += pdu->length;
	}
	else					/*	Store in file.	*/
	{
		if (writeBlockExtentToFile(&sessionBuf, segment, *cursor,
				pdu->length) < 0)
		{
			putErrmsg("Can't record block extent.", NULL);
			sdr_cancel_xn(ltpSdr);
			return -1;
		}
	}

	sdr_write(ltpSdr, segmentObj, (char *) segment, sizeof(LtpRecvSeg));

	/*	Adjust heap space occupancy per insertion of extent.	*/

	db.heapSpaceBytesOccupied += sizeof(LtpRecvSeg);
	sdr_write(ltpSdr, dbobj, (char *) &db, sizeof(LtpDB));

	/*	Based on the segment type code, infer additional
	 *	information and do additional processing.		*/

	if (pdu->segTypeCode == LtpDsRedEORP
	|| pdu->segTypeCode == LtpDsRedEOB)
	{
		/*	This segment is the end of the red part of
		 *	the block, so the end of its data is the end
		 *	of the red part.				*/

		sessionBuf.redPartLength = segUpperBound;
	}

	if ((pdu->segTypeCode & LTP_FLAG_1)
	&& (pdu->segTypeCode & LTP_FLAG_0))
	{
		/*	This segment is the end of the block.		*/

		sessionBuf.endOfBlockRecd = 1;
	}

	if (pdu->segTypeCode > 0)
	{
		/*	This segment is a checkpoint, so we have to
		 *	send a report in response.			*/

		if (sendReport(&sessionBuf, sessionObj, ckptSerialNbr,
				rptSerialNbr, segUpperBound) < 0)
		{
			putErrmsg("Can't send reception report.", NULL);
			sdr_cancel_xn(ltpSdr);
			return -1;
		}
	}

	/*	Additional processing that depends on the additional
	 *	information inferred above.				*/

	if (sessionBuf.redPartReceived == sessionBuf.redPartLength)
	{
		/*	The entire red part of the block has been
		 *	received, so deliver it to the client service.	*/

		if (deliverSvcData(client, sourceEngineId, sessionNbr,
				&sessionBuf) < 0)
		{
			putErrmsg("Can't deliver service data.", NULL);
			sdr_cancel_xn(ltpSdr);
			return -1;
		}
	}

	/*	Processing of data segment is now complete.  Rewrite
	 *	session to preserve any changes made.			*/

	sdr_write(ltpSdr, sessionObj, (char *) &sessionBuf,
			sizeof(ImportSession));
	if (sdr_end_xn(ltpSdr) < 0)
	{
		putErrmsg("Can't handle data segment.", NULL);
		return -1;
	}

	return 1;	/*	Red-part data handled okay.		*/
}

static int	loadClaimsArray(char **cursor, int *bytesRemaining,
			unsigned long claimCount, LtpReceptionClaim *claims,
			unsigned long lowerBound, unsigned long upperBound)
{
	int			i;
	LtpReceptionClaim	*claim;
	unsigned long		dataEnd = lowerBound;

	for (i = 0, claim = claims; i < claimCount; i++, claim++)
	{
		extractSdnv(&(claim->offset), cursor, bytesRemaining);
		if (claim->offset < dataEnd)
		{
			return 0;
		}

		extractSdnv(&(claim->length), cursor, bytesRemaining);
		if (claim->length == 0)
		{
			return 0;
		}

		dataEnd = claim->offset + claim->length;
		if (dataEnd > upperBound)
		{
			return 0;
		}
	}

	return 1;
}

static int	insertClaim(ExportSession *session, LtpReceptionClaim *claim)
{
	Sdr	ltpSdr = getIonsdr();
	Object	claimObj;

	CHKERR(ionLocked());
	claimObj = sdr_malloc(ltpSdr, sizeof(LtpReceptionClaim));
	if (claimObj == 0)
	{
		return -1;
	}

	if (sdr_list_insert_last(ltpSdr, session->claims, claimObj) == 0)
	{
		return -1;
	}

	sdr_write(ltpSdr, claimObj, (char *) claim,
			sizeof(LtpReceptionClaim));
	return 0;
}

static int	constructDataSegment(Sdr sdr, ExportSession *session,
			Object sessionObj, unsigned long reportSerialNbr,
			LtpVspan *vspan, LtpSpan *span, LystElt extentElt)
{
	Sdr		ltpSdr = getIonsdr();
	int		lastExtent = (lyst_next(extentElt) == NULL);
	ExportExtent	*extent;
	Object		segmentObj;
	LtpXmitSeg	segment;
	Sdnv		offsetSdnv;
	int		remainingRedBytes;
	int		redBytesToSegment;
	int		length;
	int		dataSegmentOverhead;
	int		checkpointOverhead;
	int		worstCaseSegmentSize;
	Sdnv		rsnSdnv;
	unsigned long	checkpointSerialNbr = 0;
	Sdnv		cpsnSdnv;
	int		isCheckpoint = 0;
	int		isEor = 0;		/*	End of red part.*/
	int		isEob = 0;		/*	End of block.	*/
	Sdnv		lengthSdnv;
#if LTPDEBUG
char		buf[256];
#endif

	extent = (ExportExtent *) lyst_data(extentElt);
	segmentObj = sdr_malloc(ltpSdr, sizeof(LtpXmitSeg));
	if (segmentObj == 0)
	{
		return -1;
	}

	memset((char *) &segment, 0, sizeof(LtpXmitSeg));
	segment.queueListElt = sdr_list_insert_last(ltpSdr, span->segments,
			segmentObj);
	if (segment.queueListElt == 0)
	{
		return -1;
	}

	/*	Compute length of segment's known overhead.		*/

	segment.ohdLength = 1 + (_ltpConstants())->ownEngineIdSdnv.length
			+ session->sessionNbrSdnv.length + 1;
	segment.ohdLength += session->clientSvcIdSdnv.length;
	encodeSdnv(&offsetSdnv, extent->offset);
	segment.ohdLength += offsetSdnv.length;

	/*	Determine length of segment.   Note that any single
	 *	segmentation extent might encompass red data only,
	 *	green data only, or some red data followed by some
	 *	green data.						*/

	remainingRedBytes = session->redPartLength - extent->offset;
	if (remainingRedBytes > 0)	/*	This is a red segment.	*/
	{
		/*	Segment must be all one color, so the maximum
		 *	length of data in the segment is the number of
		 *	red-part bytes remaining in the extent that is
		 *	to be segmented.				*/

		if (remainingRedBytes > extent->length)
		{
			/*	This extent encompasses part (but not
			 *	all) of the block's remaining red data.	*/

			redBytesToSegment = extent->length;
		}
		else
		{
			/*	This extent encompasses all remaining
			 *	red data and zero or more bytes of
			 *	green data as well.			*/

			redBytesToSegment = remainingRedBytes;
		}

		length = redBytesToSegment;	/*	Initial guess.	*/

		/*	Compute worst-case segment size.		*/

		encodeSdnv(&lengthSdnv, length);
		dataSegmentOverhead = segment.ohdLength + lengthSdnv.length;
		checkpointOverhead = 0;

		/*	In the worst case, this segment will be the
		 *	end of this red-part transmission cycle and
		 *	will therefore contain a non-zero checkpoint
		 *	serial number of sdnv-encoded length up to
		 *	10 and also the report serial number.		*/

		if (lastExtent)
		{
			encodeSdnv(&rsnSdnv, reportSerialNbr);
			checkpointOverhead += rsnSdnv.length;
			do checkpointSerialNbr = rand();
				while (checkpointSerialNbr == 0);
			encodeSdnv(&cpsnSdnv, checkpointSerialNbr);
			checkpointOverhead += cpsnSdnv.length;
		}

		worstCaseSegmentSize = length
				+ dataSegmentOverhead + checkpointOverhead;
		if (worstCaseSegmentSize > span->maxSegmentSize)
		{
			/*	Must reduce length.  So this segment's
			 *	last data byte can't be the last data
			 *	byte of the red data we're sending in
			 *	this red-part transmission cycle, so
			 *	segment can't be a checkpoint.  So
			 *	forget checkpoint overhead and set
			 *	data length to (max segment size minus
			 *	ordinary data segment overhead) or the
			 *	total redBytesToSegment (minus 1 if
			 *	this would otherwise be the last segment
			 *	of the last extent; we must have one
			 *	more segment, serving as checkpoint,
			 *	which must have at least 1 byte of
			 *	data), whichever is less.		*/

			checkpointOverhead = 0;
			length = span->maxSegmentSize - dataSegmentOverhead;
			if (lastExtent)
			{
				if (length >= redBytesToSegment)
				{
					length = redBytesToSegment - 1;
				}
			}
			else
			{
				if (length > redBytesToSegment)
				{
					length = redBytesToSegment;
				}
			}

			encodeSdnv(&lengthSdnv, length);
		}
		else
		{
			/*	The red remainder of this extent fits
			 *	in one segment, no matter what.  If
			 *	this is the last extent, then this
			 *	segment is a checkpoint.		*/

			if (lastExtent)
			{
				isCheckpoint = 1;
				if (length == remainingRedBytes)
				{
					isEor = 1;
					if (session->redPartLength ==
							session->totalLength)
					{
						isEob = 1;
					}
				}
			}
		}
	}
	else
	{
		/*	No remaining red data in this extent, so the
		 *	segment will be green, so there are no serial
		 *	numbers, so segment overhead is now known.	*/

		length = extent->length;
		encodeSdnv(&lengthSdnv, length);
		dataSegmentOverhead = segment.ohdLength + lengthSdnv.length;
		worstCaseSegmentSize = length + dataSegmentOverhead;
		if (worstCaseSegmentSize > span->maxSegmentSize)
		{
			/*	Must reduce length, so cannot be end
			 *	of green part (which is end of block).	*/

			length = span->maxSegmentSize - dataSegmentOverhead;
			encodeSdnv(&lengthSdnv, length);
		}
		else	/*	Remainder of extent fits in one segment.*/
		{
			if (lastExtent)
			{
				isEob = 1;
			}
		}
	}

	/*	Now have enough information to finish the segment.	*/

	segment.sessionNbr = session->sessionNbr;
	segment.remoteEngineId = span->engineId;
	segment.segmentClass = LtpDataSeg;
	segment.pdu.segTypeCode = 0;
	if (remainingRedBytes > 0)	/*	Segment is in red part.	*/
	{
		segment.sessionListElt = sdr_list_insert_last(ltpSdr,
				session->redSegments, segmentObj);
		if (segment.sessionListElt == 0)
		{
			return -1;
		}

		/*	Set flags, depending on whether or not isEor.	*/

		if (isEor)	/*	End of red part.		*/
		{
			segment.pdu.segTypeCode |= LTP_FLAG_1;

			/*	End of red part is always a checkpoint.	*/

			segment.sessionObj = sessionObj;
			if (isEob)
			{
				segment.pdu.segTypeCode |= LTP_FLAG_0;
			}
		}
		else		/*	Not end of red part of block.	*/
		{
			if (isCheckpoint)	/*	Retransmission.	*/
			{
				segment.sessionObj = sessionObj;
				segment.pdu.segTypeCode |= LTP_FLAG_0;
			}
		}
	}
	else	/*	Green-part segment.				*/
	{
		segment.sessionListElt = sdr_list_insert_last(ltpSdr,
				session->greenSegments, segmentObj);
		if (segment.sessionListElt == 0)
		{
			return -1;
		}

		segment.pdu.segTypeCode |= LTP_EXC_FLAG;
		if (isEob)
		{
			segment.pdu.segTypeCode |= LTP_FLAG_1;
			segment.pdu.segTypeCode |= LTP_FLAG_0;
		}
	}

	segment.pdu.headerExtensionsCount = 0;
	segment.pdu.trailerExtensionsCount = 0;
	if (isCheckpoint)
	{
		segment.pdu.ckptSerialNbr = checkpointSerialNbr;
		segment.ohdLength += cpsnSdnv.length;
		segment.pdu.rptSerialNbr = reportSerialNbr;
		segment.ohdLength += rsnSdnv.length;
		session->checkpointsCount++;
	}

	segment.pdu.clientSvcId = session->clientSvcId;
	segment.pdu.offset = extent->offset;
	segment.pdu.length = length;
	encodeSdnv(&lengthSdnv, segment.pdu.length);
	segment.ohdLength += lengthSdnv.length;
	segment.pdu.block = session->svcDataObjects;
	sdr_write(ltpSdr, segmentObj, (char *) &segment, sizeof(LtpXmitSeg));
	signalLso(span->engineId);
#if LTPDEBUG
if (segment.pdu.segTypeCode > 0)
{
sprintf(buf, "Sent checkpoint: session %lu segTypeCode %d length %d offset %d.",
session->sessionNbr, segment.pdu.segTypeCode, length, extent->offset);
putErrmsg(buf, itoa(session->sessionNbr));
}
#endif
	extent->offset += length;
	extent->length -= length;
	if ((_ltpvdb(NULL))->watching & WATCH_e)
	{
		putchar('e');
		fflush(stdout);
	}

	return 0;
}

int	issueSegments(Sdr sdr, LtpSpan *span, LtpVspan *vspan,
		ExportSession *session, Object sessionObj, Lyst extents,
		unsigned long reportSerialNbr)
{
	Sdr		ltpSdr = getIonsdr();
	LystElt		extentElt;
	ExportExtent	*extent;

	CHKERR(session);
	if (session->svcDataObjects == 0)	/*	Canceled.	*/
	{
		return 0;
	}

	CHKERR(ionLocked());
	CHKERR(span);
	CHKERR(vspan);
	CHKERR(extents);

	/*	For each segment issuance extent, construct as many
	 *	data segments as are needed in order to send all
	 *	service data within that extent of the aggregate block.	*/

	for (extentElt = lyst_first(extents); extentElt;
			extentElt = lyst_next(extentElt))
	{
		extent = (ExportExtent *) lyst_data(extentElt);
		while (extent->length > 0)
		{
			if (constructDataSegment(ltpSdr, session, sessionObj,
				reportSerialNbr, vspan, span, extentElt) < 0)
			{
				putErrmsg("Can't segment block.",
						itoa(vspan->meterPid));
				return -1;
			}
		}
	}

	/*	Return the number of extents processed.			*/

	return lyst_length(extents);
}

static void	getSessionContext(LtpDB *ltpdb, unsigned long sessionNbr,
			Object *sessionObj, ExportSession *sessionBuf,
			Object *spanObj, LtpSpan *spanBuf, LtpVspan **vspan,
			PsmAddress *vspanElt)
{
	Sdr	ltpSdr = getIonsdr();

	CHKVOID(ionLocked());
	*spanObj = 0;		/*	Default: no context.		*/
	getExportSession(sessionNbr, sessionObj);
	if (*sessionObj != 0)	/*	Known session.			*/
	{
		sdr_stage(ltpSdr, (char *) sessionBuf, *sessionObj,
				sizeof(ExportSession));
		if (sessionBuf->totalLength > 0)/*	A live session.	*/
		{
			*spanObj = sessionBuf->span;
		}
	}

	if (*spanObj == 0)	/*	Can't set session context.	*/
	{
		return;
	}

	sdr_read(ltpSdr, (char *) spanBuf, *spanObj, sizeof(LtpSpan));
	findSpan(spanBuf->engineId, vspan, vspanElt);
	if (*vspanElt == 0
	|| ((*vspan)->receptionRate == 0 && ltpdb->enforceSchedule == 1))
	{
#if LTPDEBUG
putErrmsg("Discarding stray segment.", itoa(sessionNbr));
#endif
		/*	Segment is from an engine that is not supposed
		 *	to be sending at this time, so we treat it as
		 *	a misdirected transmission.			*/

		*spanObj = 0;		/*	Disable acknowledgment.	*/
	}
}

static int	handleRS(LtpDB *ltpdb, unsigned long sessionNbr,
			LtpRecvSeg *segment, LtpPdu *pdu, char **cursor,
			int *bytesRemaining)
{
	Sdr			ltpSdr = getIonsdr();
	LtpVdb			*ltpvdb = _ltpvdb(NULL);
	int			ltpMemIdx = getIonMemoryMgr();
	unsigned long		rptSerialNbr;
	unsigned long		ckptSerialNbr;
	unsigned long		rptUpperBound;
	unsigned long		rptLowerBound;
	unsigned long		claimCount;
	LtpReceptionClaim	*newClaims;
	Object			sessionObj;
	ExportSession		sessionBuf;
	Object			spanObj = 0;
	LtpSpan			spanBuf;
	LtpVspan		*vspan;
	PsmAddress		vspanElt;
	Object			elt;
	Object			dsObj;
	LtpXmitSeg		dsBuf;
	Lyst			claims;
	Object			claimObj;
	Object			nextElt;
	LtpReceptionClaim	*claim;
	unsigned int		claimEnd;
	LtpReceptionClaim	*newClaim;
	unsigned int		newClaimEnd;
	LystElt			elt2;
	LystElt			nextElt2;
	int			i;
	Lyst			extents;
	ExportExtent		*extent;
	unsigned int		startOfGap;
#if LTPDEBUG
char			buf[256];
putErrmsg("Handling report.", utoa(sessionNbr));
#endif

	/*	First finish parsing the segment.  Load all the
	 *	reception claims in the report into an array of new
	 *	claims.							*/

	extractSdnv(&rptSerialNbr, cursor, bytesRemaining);
	extractSdnv(&ckptSerialNbr, cursor, bytesRemaining);
	extractSdnv(&rptUpperBound, cursor, bytesRemaining);
	extractSdnv(&rptLowerBound, cursor, bytesRemaining);
	extractSdnv(&claimCount, cursor, bytesRemaining);
	newClaims = (LtpReceptionClaim *)
			MTAKE(claimCount* sizeof(LtpReceptionClaim));
	if (newClaims == NULL)
	{
		/*	Too many claims; could be a DOS attack.		*/
#if LTPDEBUG
putErrmsg("Discarding report.", NULL);
#endif
		return 0;		/*	Ignore report.		*/
	}

	if (loadClaimsArray(cursor, bytesRemaining, claimCount, newClaims,
			rptLowerBound, rptUpperBound) == 0)
	{
#if LTPDEBUG
putErrmsg("Discarding report.", NULL);
#endif
		MRELEASE(newClaims);
		return 0;		/*	Ignore report.		*/
	}

	/*	Acknowledge the report if possible.			*/

	sdr_begin_xn(ltpSdr);
	getSessionContext(ltpdb, sessionNbr, &sessionObj,
			&sessionBuf, &spanObj, &spanBuf, &vspan, &vspanElt);
	if (spanObj == 0)	/*	Unknown provenance, ignore.	*/
	{
		sdr_exit_xn(ltpSdr);
		MRELEASE(newClaims);
		return 0;
	}

	if (sessionObj == 0)
	{
		/*	Report for an unknown session: must be in
		 *	response to arrival of retransmitted segments
		 *	following session closure.  So the remote
		 *	import session is an erroneous resurrection
		 *	of a closed session and we need to help the
		 *	remote engine terminate it.  We do so by
		 *	ignoring the report; the report will time out
		 *	and be retransmitted N times and then will
		 *	cause the session to fail and be canceled
		 *	by receiver -- exactly the correct result.	*/

		sdr_exit_xn(ltpSdr);
		MRELEASE(newClaims);
		return 0;
	}

	if (constructReportAckSegment(&spanBuf, spanObj, sessionNbr,
			rptSerialNbr))
	{
		putErrmsg("Can't send RA segment.", NULL);
		sdr_cancel_xn(ltpSdr);
		MRELEASE(newClaims);
		return -1;
	}

	/*	Now process the report if possible.			*/

	if (sessionBuf.totalLength == 0)/*	Reused session nbr.	*/
	{
#if LTPDEBUG
putErrmsg("Discarding report.", NULL);
#endif
		MRELEASE(newClaims);
		return sdr_end_xn(ltpSdr);	/*	Ignore RS.	*/
	}

	/*	First apply the report to the cited checkpoint, if any.	*/

	if (ckptSerialNbr != 0)	/*	Not an asynchronous report.	*/
	{
		findCheckpoint(&sessionBuf, ckptSerialNbr, &elt, &dsObj);
		if (elt == 0)
		{
#if LTPDEBUG
putErrmsg("Discarding report.", NULL);
#endif
			/*	No such checkpoint; the report is
			 *	either erroneous or redundant.		*/

			MRELEASE(newClaims);
			return sdr_end_xn(ltpSdr);	/*	Ignore.	*/
		}

		/*	Deactivate the checkpoint segment.  It has been
		 *	received, so there will never be any need to
		 *	retransmit it.					*/

		sdr_stage(ltpSdr, (char *) &dsBuf, dsObj, sizeof(LtpXmitSeg));
		dsBuf.pdu.timer.segArrivalTime = 0;
		sdr_write(ltpSdr, dsObj, (char *) &dsBuf, sizeof(LtpXmitSeg));
	}

	/*	Now apply reception claims to the transmission session.	*/

	if (rptUpperBound > sessionBuf.redPartLength	/*	Bogus.	*/
	|| rptLowerBound >= rptUpperBound	/*	Malformed.	*/
	|| claimCount == 0)			/*	Malformed.	*/
	{
#if LTPDEBUG
putErrmsg("Discarding report.", NULL);
#endif
		MRELEASE(newClaims);
		return sdr_end_xn(ltpSdr);	/*	Ignore RS.	*/
	}

	/*	Retrieve all previously received reception claims
	 *	for this transmission session, loading them into a
	 *	temporary linked list within which the new and old
	 *	claims will be merged.  While loading the old claims
	 *	into the list, delete them from the database; they
	 *	will be replaced by the final contents of the linked
	 *	list.							*/

	if ((claims = lyst_create_using(ltpMemIdx)) == NULL)
	{
		putErrmsg("Can't start list of reception claims.", NULL);
		MRELEASE(newClaims);
		sdr_cancel_xn(ltpSdr);
		return -1;
	}

	for (elt = sdr_list_first(ltpSdr, sessionBuf.claims); elt;
			elt = nextElt)
	{
		nextElt = sdr_list_next(ltpSdr, elt);
		claimObj = sdr_list_data(ltpSdr, elt);
		claim = (LtpReceptionClaim *) MTAKE(sizeof(LtpReceptionClaim));
		if (claim == NULL || (lyst_insert_last(claims, claim)) == NULL)
		{
			putErrmsg("Can't insert reception claim.", NULL);
			MRELEASE(newClaims);
			sdr_cancel_xn(ltpSdr);
			return -1;
		}

		sdr_read(ltpSdr, (char *) claim, claimObj,
				sizeof(LtpReceptionClaim));
		sdr_free(ltpSdr, claimObj);
		sdr_list_delete(ltpSdr, elt, NULL, NULL);
	}

	/*	Now merge the new claims in the array with the old
	 *	claims in the list.  The final contents of the linked
	 *	list will be the consolidated claims resulting from
	 *	merging the old claims from the database with the new
	 *	claims in the report.					*/

	for (i = 0, newClaim = newClaims; i < claimCount; i++, newClaim++)
	{
		newClaimEnd = newClaim->offset + newClaim->length;
		for (elt2 = lyst_first(claims); elt2; elt2 = nextElt2)
		{
			nextElt2 = lyst_next(elt2);
			claim = (LtpReceptionClaim *) lyst_data(elt2);
			claimEnd = claim->offset + claim->length;
			if (claimEnd < newClaim->offset)
			{
				/*	This old claim is unaffected
				 *	by the new claims; it remains
				 *	in the list.			*/

				continue;
			}

			if (claim->offset > newClaimEnd)
			{
				/*	Must insert new claim into
				 *	list before this old one.	*/

				break;	/*	Out of old-claims loop.	*/
			}

			/*	New claim overlaps with this existing
			 *	claim, so consolidate the existing
			 *	claim with the new claim (in place,
			 *	in the array), delete the old claim
			 *	from the list, and look at the next
			 *	existing claim.				*/

			if (claim->offset < newClaim->offset)
			{
				/*	Start of consolidated claim
				 *	is earlier than start of new
				 *	claim.				*/

				newClaim->offset = claim->offset;
			}

			if (claimEnd > newClaimEnd)
			{
				/*	End of consolidated claim
				 *	is later than end of new
				 *	claim.				*/

				newClaim->length += (claimEnd - newClaimEnd);
			}

			MRELEASE(claim);
			lyst_delete(elt2);
		}

		/*	newClaim has now been consolidated with all
		 *	prior claims with which it overlapped, and all
		 *	of those prior claims have been removed from
		 *	the list.  Now we can insert the consolidated
		 *	new claim into the list.			*/

		claim = (LtpReceptionClaim *) MTAKE(sizeof(LtpReceptionClaim));
		if (claim == NULL)
		{
			putErrmsg("Can't create reception claim.", NULL);
			MRELEASE(newClaims);
			sdr_cancel_xn(ltpSdr);
			return -1;
		}

		claim->offset = newClaim->offset;
		claim->length = newClaim->length;
		if (elt2)
		{
			if (lyst_insert_before(elt2, claim) == NULL)
			{
				putErrmsg("Can't create reception claim.",
						NULL);
				MRELEASE(newClaims);
				sdr_cancel_xn(ltpSdr);
				return -1;
			}
		}
		else
		{
			if (lyst_insert_last(claims, claim) == NULL)
			{
				putErrmsg("Can't create reception claim.",
						NULL);
				MRELEASE(newClaims);
				sdr_cancel_xn(ltpSdr);
				return -1;
			}
		}
	}

	/*	The claims list now contains the consolidated claims
	 *	for all data reception for this export session.  The
	 *	array of new claims is no longer needed.		*/

	MRELEASE(newClaims);
	elt2 = lyst_first(claims);
	claim = (LtpReceptionClaim *) lyst_data(elt2);

	/*	If reception of all data in the block is claimed (i.e,
	 *	there is now only one claim in the list and that claim
	 *	-- the first -- encompasses the entire red part of the
	 *	block), end the export session.				*/

	if (claim->offset == 0 && claim->length == sessionBuf.redPartLength)
	{
		MRELEASE(claim);	/*	(Sole claim in list.)	*/
		lyst_destroy(claims);
		stopExportSession(&sessionBuf);
		closeExportSession(sessionObj);
		if (sdr_end_xn(ltpSdr) < 0)
		{
			putErrmsg("Can't handle report segment.", NULL);
			return -1;
		}

		if (ltpvdb->watching & WATCH_h)
		{
			putchar('h');
			fflush(stdout);
		}

		return 1;	/*	Complete, successful export.	*/
	}

	/*	Not all data in the block has yet been received.	*/

	if (sessionBuf.checkpointsCount == MAX_NBR_OF_CHECKPOINTS)
	{
		/*	Limit reached, can't retransmit any more.
		 *	Just destroy the claims list and cancel. 	*/

		while (1)
		{
			MRELEASE(claim);
			elt2 = lyst_next(elt2);
			if (elt2 == NULL)
			{
				break;
			}

			claim = (LtpReceptionClaim *) lyst_data(elt2);
		}

		lyst_destroy(claims);
		if (cancelSessionBySender(&sessionBuf, sessionObj,
				LtpRetransmitLimitExceeded))
		{
			putErrmsg("Can't cancel export session.", NULL);
			sdr_cancel_xn(ltpSdr);
			return -1;
		}

		if (sdr_end_xn(ltpSdr) < 0)
		{
			putErrmsg("Can't handle report segment.", NULL);
			return -1;
		}

		return 1;
	}

	/*	Must retransmit data to fill gaps ("extents") in
	 *	reception.  Start compiling list of retransmission
	 *	ExportExtents.						*/

#if LTPDEBUG
putErrmsg("Incomplete reception.  Claims:", utoa(claimCount));
#endif
	if ((extents = lyst_create_using(ltpMemIdx)) == NULL)
	{
		putErrmsg("Can't start list of retransmission extents.", NULL);
		sdr_cancel_xn(ltpSdr);
		return -1;
	}

	startOfGap = rptLowerBound;
	
	/*	Loop through the claims, writing them to the database
	 *	and adding retransmission extents for the gaps between
	 *	the claims.						*/

	while (1)
	{
#if LTPDEBUG
sprintf(buf, "-   offset %lu length %lu", claim->offset, claim->length);
putErrmsg(buf, itoa(sessionBuf.sessionNbr));
#endif
		claimEnd = claim->offset + claim->length;
		if (insertClaim(&sessionBuf, claim) < 0)
		{
			putErrmsg("Can't create new reception claim.", NULL);
			sdr_cancel_xn(ltpSdr);
			return -1;
		}

		if (claim->offset > startOfGap)	/*	Here's a gap.	*/
		{
			if ((extent = (ExportExtent *)
					MTAKE(sizeof(ExportExtent))) == NULL)
			{
				putErrmsg("Can't add retransmission extent.",
						NULL);
				sdr_cancel_xn(ltpSdr);
				return -1;
			}

			extent->offset = startOfGap;
			extent->length = claim->offset - extent->offset;
			if (lyst_insert_last(extents, extent) == NULL)
			{
				putErrmsg("Can't add retransmission extent.",
						NULL);
				sdr_cancel_xn(ltpSdr);
				return -1;
			}

			startOfGap = claimEnd;
		}
		else
		{
			if (claimEnd > startOfGap)
			{
				startOfGap = claimEnd;	/*	New gap.*/
			}
		}

		MRELEASE(claim);
		elt2 = lyst_next(elt2);
		if (elt2 == NULL)
		{
			break;
		}

		claim = (LtpReceptionClaim *) lyst_data(elt2);
	}

	lyst_destroy(claims);

	/*	List of retransmission extents is now complete;
	 *	retransmit data as needed.  				*/

	if (issueSegments(ltpSdr, &spanBuf, vspan, &sessionBuf, sessionObj,
			extents, rptSerialNbr) < 0)
	{
		putErrmsg("Can't retransmit data.", itoa(vspan->meterPid));
		sdr_cancel_xn(ltpSdr);
		return -1;
	}

	/*	Finally, destroy retransmission extents list and
	 *	return.							*/

	for (elt2 = lyst_first(extents); elt2; elt2 = lyst_next(elt2))
	{
		MRELEASE((char *) lyst_data(elt2));
	}

	lyst_destroy(extents);
	if (sdr_end_xn(ltpSdr) < 0)
	{
		putErrmsg("Can't handle report segment.", NULL);
		return -1;
	}

	if (ltpvdb->watching & WATCH_nak)
	{
		putchar('@');
		fflush(stdout);
	}

	return 1;	/*	Report handled successfully.		*/
}

static int	handleRA(unsigned long sourceEngineId, LtpDB *ltpdb,
			unsigned long sessionNbr, LtpRecvSeg *segment,
			LtpPdu *pdu, char **cursor, int *bytesRemaining)
{
	Sdr		ltpSdr = getIonsdr();
	unsigned long	rptSerialNbr;
	LtpVspan	*vspan;
	PsmAddress	vspanElt;
	Object		sessionObj;
	ImportSession	session;
	Object		elt;
	Object		rsObj;
			OBJ_POINTER(LtpXmitSeg, rs);
#if LTPDEBUG
putErrmsg("Handling report ack.", utoa(sessionNbr));
#endif

	/*	First finish parsing the segment.			*/

	extractSdnv(&rptSerialNbr, cursor, bytesRemaining);

	/*	Report is being acknowledged.				*/

	sdr_begin_xn(ltpSdr);
	findSpan(sourceEngineId, &vspan, &vspanElt);
	if (vspanElt == 0)	/*	Random segment.			*/
	{
		sdr_exit_xn(ltpSdr);
		return 0;
	}

	if (vspan->receptionRate == 0 && ltpdb->enforceSchedule == 1)
	{
#if LTPDEBUG
putErrmsg("Discarding stray segment.", itoa(sessionNbr));
#endif
		/*	Segment is from an engine that is not supposed
		 *	to be sending at this time, so we treat it as
		 *	a misdirected transmission.			*/

		sdr_exit_xn(ltpSdr);
		return 0;
	}

	getImportSession(vspan, sessionNbr, &sessionObj);
	if (sessionObj == 0)	/*	Nothing to apply ack to.	*/
	{
		return sdr_end_xn(ltpSdr);
	}

	/*	Session exists, so find the report.			*/

	sdr_stage(ltpSdr, (char *) &session, sessionObj, sizeof(ImportSession));
	findReport(&session, rptSerialNbr, &elt, &rsObj);
	if (rsObj)	/*	Found the report that is acknowledged.	*/
	{
		/*	In the event that we ever do accelerated
	 	 *	retransmission, we would want to retain the
		 *	RS (and just turn off its timer) to support
		 *	lookup of report lower bound on reception of
		 *	a checkpoint data segment that cites this
		 *	report.  But for now it's moot, since the
		 *	lower bound can never be anything other than
		 *	zero (which is the default on lookup failure),
		 *	so we destroy the RS immediately.  This makes
		 *	detection of session closure opportunity easy.	*/

		GET_OBJ_POINTER(ltpSdr, LtpXmitSeg, rs, rsObj);
		destroyRsXmitSeg(elt, rsObj, rs);
		if (session.redPartLength > 0
			/*	EORP has been received.			*/
		&& session.redPartReceived == session.redPartLength
		&& sdr_list_length(ltpSdr, session.rsSegments) == 0)
		{
			stopImportSession(&session);
			sdr_write(ltpSdr, sessionObj, (char *) &session,
					sizeof(ImportSession));
			closeImportSession(sessionObj);
		}

		if (sdr_end_xn(ltpSdr) < 0)
		{
			putErrmsg("Can't handle report ack.", NULL);
			return -1;
		}

		return 1;
	}

	/*	Anomaly: no match on report serial number, so ignore
	 *	the RA.							*/

	return sdr_end_xn(ltpSdr);
}

static int	handleCS(unsigned long sourceEngineId, LtpDB *ltpdb,
			unsigned long sessionNbr, LtpRecvSeg *segment,
			LtpPdu *pdu, char **cursor, int *bytesRemaining)
{
	Sdr		ltpSdr = getIonsdr();
	LtpVdb		*ltpvdb = _ltpvdb(NULL);
	LtpVspan	*vspan;
	PsmAddress	vspanElt;
	Object		spanObj;
			OBJ_POINTER(LtpSpan, span);
	Object		sessionObj;
			OBJ_POINTER(ImportSession, session);
#if LTPDEBUG
putErrmsg("Handling cancel by sender.", utoa(sessionNbr));
#endif

	/*	Source of block is requesting cancellation of session.	*/

	sdr_begin_xn(ltpSdr);
	findSpan(sourceEngineId, &vspan, &vspanElt);
	if (vspanElt == 0)
	{
		/*	Cancellation is from an unknown source engine,
		 *	so we can't even acknowledge.  Ignore it.	*/

		sdr_exit_xn(ltpSdr);
		return 0;
	}

	if (vspan->receptionRate == 0 && ltpdb->enforceSchedule == 1)
	{
#if LTPDEBUG
putErrmsg("Discarding stray segment.", itoa(sessionNbr));
#endif
		/*	Segment is from an engine that is not supposed
		 *	to be sending at this time, so we treat it as
		 *	a misdirected transmission.			*/

		sdr_exit_xn(ltpSdr);
		return 0;
	}

	spanObj = sdr_list_data(ltpSdr, vspan->spanElt);
	GET_OBJ_POINTER(ltpSdr, LtpSpan, span, spanObj);

	/*	Acknowledge the cancellation request.			*/

	if (constructSourceCancelAckSegment(spanObj, &(span->engineIdSdnv),
			sessionNbr) < 0)
	{
		putErrmsg("Can't send CAS segment.", NULL);
		sdr_cancel_xn(ltpSdr);
		return -1;
	}

	getImportSession(vspan, sessionNbr, &sessionObj);
	if (sessionObj)	/*	Can cancel session as requested.	*/
	{
		GET_OBJ_POINTER(ltpSdr, ImportSession, session, sessionObj);
		if (enqueueNotice(ltpvdb->clients + session->clientSvcId,
				sourceEngineId, sessionNbr, 0, 0,
				LtpImportSessionCanceled, **cursor, 0, 0) < 0)
		{
			putErrmsg("Can't post ImportSessionCanceled notice.",
					NULL);
			sdr_cancel_xn(ltpSdr);
			return -1;
		}

		if (ltpvdb->watching & WATCH_handleCS)
		{
			putchar('}');
			fflush(stdout);
		}

		stopImportSession(session);
		sdr_write(ltpSdr, sessionObj, (char *) session,
				sizeof(ImportSession));
		closeImportSession(sessionObj);
	}

	if (sdr_end_xn(ltpSdr) < 0)
	{
		putErrmsg("Can't handle cancel by source.", NULL);
		return -1;
	}

	return 1;
}

static int	handleCAS(LtpDB *ltpdb, unsigned long sessionNbr,
			LtpRecvSeg *segment, LtpPdu *pdu, char **cursor,
			int *bytesRemaining)
{
	Sdr		ltpSdr = getIonsdr();
	Object		sessionObj;
	Object		sessionElt;
#if LTPDEBUG
putErrmsg("Handling ack of cancel by sender.", utoa(sessionNbr));
#endif

	/*	Destination of block is acknowledging source's
	 *	cancellation of session.				*/

	sdr_begin_xn(ltpSdr);
	getCanceledExport(sessionNbr, &sessionObj, &sessionElt);
	if (sessionObj == 0)	/*	Nothing to apply ack to.	*/
	{
		sdr_exit_xn(ltpSdr);
		return 0;
	}

	cancelEvent(LtpResendXmitCancel, 0, sessionNbr, 0);

	/*	No need to change state of session's timer
	 *	because the whole session is about to vanish.		*/

	sdr_list_delete(ltpSdr, sessionElt, NULL, NULL);
	sdr_free(ltpSdr, sessionObj);
	if (sdr_end_xn(ltpSdr) < 0)
	{
		putErrmsg("Can't handle ack of cancel by source.", NULL);
		return -1;
	}

	return 1;
}

static int	handleCR(LtpDB *ltpdb, unsigned long sessionNbr,
			LtpRecvSeg *segment, LtpPdu *pdu, char **cursor,
			int *bytesRemaining)
{
	Sdr		ltpSdr = getIonsdr();
	LtpVdb		*ltpvdb = _ltpvdb(NULL);
	Object		dbobj = getLtpDbObject();
	LtpDB		db;
	Object		sessionObj;
	ExportSession	sessionBuf;
	Object		spanObj;
	LtpSpan		spanBuf;
	LtpVspan	*vspan;
	PsmAddress	vspanElt;
	Object		elt;
	Object		sdu;	/*	A ZcoRef object.		*/
#if LTPDEBUG
putErrmsg("Handling cancel by receiver.", utoa(sessionNbr));
#endif

	/*	Destination of block is requesting cancellation of
	 *	session.						*/

	sdr_begin_xn(ltpSdr);
	getSessionContext(ltpdb, sessionNbr, &sessionObj, &sessionBuf,
			&spanObj, &spanBuf, &vspan, &vspanElt);
	if (spanObj == 0)	/*	Unknown provenance, ignore.	*/
	{
		sdr_exit_xn(ltpSdr);
		return 0;
	}

	/*	Acknowledge the cancellation request.			*/

	sdr_stage(ltpSdr, (char *) &db, dbobj, sizeof(LtpDB));
	if (constructDestCancelAckSegment(spanObj,
			&db.ownEngineIdSdnv, sessionNbr) < 0)
	{
		putErrmsg("Can't send CAR segment.", NULL);
		sdr_cancel_xn(ltpSdr);
		return -1;
	}

	if (sessionObj)
	{
		sessionBuf.reasonCode = **cursor;
		if (ltpvdb->watching & WATCH_handleCR)
		{
			putchar(']');
			fflush(stdout);
		}

		stopExportSession(&sessionBuf);
		for (elt = sdr_list_first(ltpSdr, sessionBuf.svcDataObjects);
				elt; elt = sdr_list_next(ltpSdr, elt))
		{
			sdu = sdr_list_data(ltpSdr, elt);
			if (enqueueNotice(ltpvdb->clients
					+ sessionBuf.clientSvcId,
					db.ownEngineId, sessionBuf.sessionNbr,
					0, 0, LtpExportSessionCanceled,
					**cursor, 0, sdu) < 0)
			{
				putErrmsg("Can't post ExportSessionCanceled \
notice.", NULL);
				sdr_cancel_xn(ltpSdr);
				return -1;
			}
		}

		sdr_write(ltpSdr, dbobj, (char *) &db, sizeof(LtpDB));

		/*	The service data units in the svcDataObjects
		 *	list must be protected -- the client will be 
		 *	receiving them in notices and destroying them
		 *	-- so we must destroy the svcDataObject list
		 *	itself here and prevent closeExportSession
		 *	from accessing it.				*/

		sdr_list_destroy(ltpSdr, sessionBuf.svcDataObjects, NULL, NULL);
		sessionBuf.svcDataObjects = 0;
		sdr_write(ltpSdr, sessionObj, (char *) &sessionBuf,
				sizeof(ExportSession));

		/*	Now finish closing the export session.		*/

		closeExportSession(sessionObj);
	}

	if (sdr_end_xn(ltpSdr) < 0)
	{
		putErrmsg("Can't handle cancel by destination.", NULL);
		return -1;
	}

	return 1;
}

static int	handleCAR(unsigned long sourceEngineId, LtpDB *ltpdb,
			unsigned long sessionNbr, LtpRecvSeg *segment,
			LtpPdu *pdu, char **cursor, int *bytesRemaining)
{
	Sdr		ltpSdr = getIonsdr();
	LtpVspan	*vspan;
	PsmAddress	vspanElt;
	Object		sessionObj;
	Object		sessionElt;
#if LTPDEBUG
putErrmsg("Handling ack of cancel by receiver.", utoa(sessionNbr));
#endif

	/*	Source of block is acknowledging destination's
	 *	cancellation of session.				*/

	sdr_begin_xn(ltpSdr);
	findSpan(sourceEngineId, &vspan, &vspanElt);
	if (vspanElt == 0)	/*	Stray segment.			*/
	{
		sdr_exit_xn(ltpSdr);
		return 0;
	}

	if (vspan->receptionRate == 0 && ltpdb->enforceSchedule == 1)
	{
#if LTPDEBUG
putErrmsg("Discarding stray segment.", itoa(sessionNbr));
#endif
		/*	Segment is from an engine that is not supposed
		 *	to be sending at this time, so we treat it as
		 *	a misdirected transmission.			*/

		sdr_exit_xn(ltpSdr);
		return 0;
	}

	getCanceledImport(vspan, sessionNbr, &sessionObj, &sessionElt);
	if (sessionObj == 0)	/*	Nothing to apply ack to.	*/
	{
		return sdr_end_xn(ltpSdr);
	}

	cancelEvent(LtpResendRecvCancel, sourceEngineId, sessionNbr, 0);

	/*	No need to change state of session's timer because
	 *	the whole session is about to vanish.			*/

	sdr_list_delete(ltpSdr, sessionElt, NULL, NULL);
	sdr_free(ltpSdr, sessionObj);
	if (sdr_end_xn(ltpSdr) < 0)
	{
		putErrmsg("Can't handle ack of cancel by destination.", NULL);
		return -1;
	}

	return 1;
}

static int	ignoreHeaderExtensions(int extensionsCount, char **cursor,
			int *bytesRemaining)
{
	unsigned char	tag;
	unsigned long	extensionLength;

	while (extensionsCount > 0)
	{
		/*	Skip over extension's tag.			*/

		if (*bytesRemaining < 1)
		{
			return -1;
		}

		tag = (unsigned char) **cursor;
		(*cursor)++;
		(*bytesRemaining)--;

		/*	Get extension's length.				*/

		extractSdnv(&extensionLength, cursor, bytesRemaining);

		/*	Skip over extension's value.			*/

		if (*bytesRemaining < extensionLength)
		{
			return -1;
		}

		(*cursor) += extensionLength;
		(*bytesRemaining) -= extensionLength;

		/*	Have successfully ignored this extension.	*/

		extensionsCount--;
	}

	return 0;
}

int	ltpHandleInboundSegment(char *buf, int length)
{
	LtpRecvSeg	segment;
	LtpPdu		*pdu = &segment.pdu;
	char		*cursor = buf;
	int		bytesRemaining = length;
	unsigned char	versionNbr;
	unsigned long	sourceEngineId;
	unsigned long	sessionNbr;
	unsigned long	extensionLengths;
			OBJ_POINTER(LtpDB, ltpdb);

	CHKERR(buf);
	CHKERR(length > 0);
	memset((char *) &segment, 0, sizeof(LtpRecvSeg));

	/*	Get version number and segment type (flags).		*/

	versionNbr = ((*cursor) >> 4) & 0x0f;
	pdu->segTypeCode = (*cursor) & 0x0f;
	cursor++;
	bytesRemaining--;

	/*	Get session ID.						*/

	extractSdnv(&sourceEngineId, &cursor, &bytesRemaining);
	extractSdnv(&sessionNbr, &cursor, &bytesRemaining);

	/*	Get lengths of header and trailer extensions.		*/

	extensionLengths = *cursor;
	cursor++;
	bytesRemaining--;
	if (extensionLengths != 0)
	{
		if (ignoreHeaderExtensions((extensionLengths >> 4) & 0x0f,
				&cursor, &bytesRemaining) < 0)
		{
#if LTPDEBUG
			writeMemoNote("[?] LTP segment extensions malformed, \
segment discarded", itoa(extensionLengths));
#endif
			return 0;	/*	Ignore the segment.	*/
		}
#if LTPDEBUG
		else
		{
			writeMemoNote("[?] LTP segment extensions ignored",
					itoa(extensionLengths));
		}
#endif
	}

	/*	Handle segment according to its segment type code.	*/

	if ((_ltpvdb(NULL))->watching & WATCH_s)
	{
		putchar('s');
		fflush(stdout);
	}

	GET_OBJ_POINTER(getIonsdr(), LtpDB, ltpdb, _ltpdbObject(NULL));
	if ((pdu->segTypeCode & LTP_CTRL_FLAG) == 0)	/*	Data.	*/
	{
		return handleDataSegment(sourceEngineId, ltpdb, sessionNbr,
				&segment, pdu, &cursor, &bytesRemaining);
	}

	/*	Segment is a control segment.				*/
 
	switch (pdu->segTypeCode)
	{
	case LtpRS:
		return handleRS(ltpdb, sessionNbr,
				&segment, pdu, &cursor, &bytesRemaining);

	case LtpRAS:
		return handleRA(sourceEngineId, ltpdb, sessionNbr,
				&segment, pdu, &cursor, &bytesRemaining);

	case LtpCS:
		return handleCS(sourceEngineId, ltpdb, sessionNbr,
				&segment, pdu, &cursor, &bytesRemaining);

	case LtpCAS:
		return handleCAS(ltpdb, sessionNbr,
				&segment, pdu, &cursor, &bytesRemaining);

	case LtpCR:
		return handleCR(ltpdb, sessionNbr,
				&segment, pdu, &cursor, &bytesRemaining);

	case LtpCAR:
		return handleCAR(sourceEngineId, ltpdb, sessionNbr,
				&segment, pdu, &cursor, &bytesRemaining);
	default:
		break;
	}

	return 0;		/*	Ignore the segment.		*/
}

/*	*	*	Functions that respond to events	*	*/

void	ltpStartXmit(LtpVspan *vspan)
{
	Sdr	ltpSdr = getIonsdr();
	Object	spanObj;
	LtpSpan	span;

	CHKVOID(ionLocked());
	CHKVOID(vspan);
	spanObj = sdr_list_data(ltpSdr, vspan->spanElt);
	sdr_read(ltpSdr, (char *) &span, spanObj, sizeof(LtpSpan));
	if (span.lengthOfBufferedBlock == 0)
	{
		sm_SemGive(vspan->bufEmptySemaphore);
	}

	if (sdr_list_length(ltpSdr, span.segments) > 0)
	{
		sm_SemGive(vspan->segSemaphore);
	}
}

void	ltpStopXmit(LtpVspan *vspan)
{
	Sdr		ltpSdr = getIonsdr();
	Object		spanObj;
	LtpSpan		span;
	Object		elt;
	Object		nextElt;
	Object		sessionObj;
	ExportSession	session;

	CHKVOID(ionLocked());
	CHKVOID(vspan);
	spanObj = sdr_list_data(ltpSdr, vspan->spanElt);
	sdr_read(ltpSdr, (char *) &span, spanObj, sizeof(LtpSpan));
	if (span.purge)
	{
		/*	At end of transmission on this span we must
		 *	cancel all export sessions that are currently
		 *	in progress.  Notionally this forces re-
		 *	forwarding of the DTN bundles in each session's
		 *	block, to avoid having to wait for the restart
		 *	of transmission on this span before those
		 *	bundles can be successfully transmitted.	*/

		for (elt = sdr_list_first(ltpSdr, span.exportSessions); elt;
				elt = nextElt)
		{
			nextElt = sdr_list_next(ltpSdr, elt);
			sessionObj = sdr_list_data(ltpSdr, elt);
			sdr_stage(ltpSdr, (char *) &session, sessionObj,
					sizeof(ExportSession));
			if (session.svcDataObjects == 0
			|| sdr_list_length(ltpSdr, session.svcDataObjects) == 0)
			{
				/*	Session is not yet populated
				 *	with any service data objects.	*/

				continue;
			}

			oK(cancelSessionBySender(&session, sessionObj,
					LtpCancelByEngine));
		}
	}
}

static void	suspendTimer(time_t suspendTime, LtpTimer *timer,
			Address timerAddr, unsigned int qTime,
			unsigned long remoteXmitRate, LtpEventType eventType,
			unsigned long eventRefNbr1, unsigned long eventRefNbr2,
			unsigned long eventRefNbr3)
{
	time_t	latestAckXmitStartTime;

	CHKVOID(ionLocked());
	latestAckXmitStartTime = timer->segArrivalTime + qTime;
	if (latestAckXmitStartTime < suspendTime)
	{
		/*	Transmission of ack should have begun before
		 *	link was stopped.  Timer must not be suspended.	*/

		return;
	}

	/*	Must suspend timer while remote engine is unable to
	 *	transmit.						*/

	cancelEvent(eventType, eventRefNbr1, eventRefNbr2, eventRefNbr3);

	/*	Change state of timer object and save it.		*/

	timer->state = LtpTimerSuspended;
	sdr_write(getIonsdr(), timerAddr, (char *) timer, sizeof(LtpTimer));
}

int	ltpSuspendTimers(LtpVspan *vspan, PsmAddress vspanElt,
		time_t suspendTime, unsigned long priorXmitRate)
{
	Sdr		ltpSdr = getIonsdr();
	Object		spanObj;
			OBJ_POINTER(LtpSpan, span);
	unsigned int	qTime;
	Object		elt;
	Object		sessionObj;
	ImportSession	rsessionBuf;
	LtpTimer	*timer;
	Object		elt2;
	Object		segmentObj;
	LtpXmitSeg	rsBuf;
	ExportSession	xsessionBuf;
	LtpXmitSeg	dsBuf;

	CHKERR(ionLocked());
	CHKERR(vspan);
	spanObj = sdr_list_data(ltpSdr, vspan->spanElt);
	GET_OBJ_POINTER(ltpSdr, LtpSpan, span, spanObj);
	qTime = span->remoteQtime;

	/*	Suspend relevant timers for import sessions.		*/

	for (elt = sdr_list_first(ltpSdr, span->deadImports); elt;
			elt = sdr_list_next(ltpSdr, elt))
	{
		sessionObj = sdr_list_data(ltpSdr, elt);
		sdr_stage(ltpSdr, (char *) &rsessionBuf, sessionObj,
				sizeof(ImportSession));

		/*	Suspend receiver-cancel retransmit timer.	*/

		timer = &rsessionBuf.timer;
		suspendTimer(suspendTime, timer,
			sessionObj + FLD_OFFSET(timer, &rsessionBuf),
			qTime, priorXmitRate, LtpResendRecvCancel,
			span->engineId, rsessionBuf.sessionNbr, 0);
	}

	for (elt = sdr_list_first(ltpSdr, span->importSessions); elt;
			elt = sdr_list_next(ltpSdr, elt))
	{
		sessionObj = sdr_list_data(ltpSdr, elt);
		sdr_read(ltpSdr, (char *) &rsessionBuf, sessionObj,
				sizeof(ImportSession));

		/*	Suspend report retransmission timers.		*/

		for (elt2 = sdr_list_first(ltpSdr, rsessionBuf.rsSegments);
				elt2; elt2 = sdr_list_next(ltpSdr, elt2))
		{
			segmentObj = sdr_list_data(ltpSdr, elt2);
			sdr_stage(ltpSdr, (char *) &rsBuf, segmentObj,
					sizeof(LtpXmitSeg));
			if (rsBuf.pdu.timer.segArrivalTime == 0)
			{
				continue;	/*	Not active.	*/
			}

			timer = &rsBuf.pdu.timer;
			suspendTimer(suspendTime, timer,
				segmentObj + FLD_OFFSET(timer, &rsBuf),
				qTime, priorXmitRate, LtpResendReport,
				span->engineId, rsessionBuf.sessionNbr,
				rsBuf.pdu.rptSerialNbr);
		}
	}

	/*	Suspend relevant timers for export sessions.		*/

	for (elt = sdr_list_first(ltpSdr, (_ltpConstants())->deadExports); elt;
			elt = sdr_list_next(ltpSdr, elt))
	{
		sessionObj = sdr_list_data(ltpSdr, elt);
		sdr_stage(ltpSdr, (char *) &xsessionBuf, sessionObj,
				sizeof(ExportSession));
		if (xsessionBuf.span != spanObj)
		{
			continue;	/*	Not for this span.	*/
		}

		/*	Suspend sender-cancel retransmit timer.		*/

		timer = &xsessionBuf.timer;
		suspendTimer(suspendTime, timer,
			sessionObj + FLD_OFFSET(timer, &xsessionBuf),
			qTime, priorXmitRate, LtpResendXmitCancel, 0,
			xsessionBuf.sessionNbr, 0);
	}

	for (elt = sdr_list_first(ltpSdr, span->exportSessions); elt;
			elt = sdr_list_next(ltpSdr, elt))
	{
		sessionObj = sdr_list_data(ltpSdr, elt);
		sdr_read(ltpSdr, (char *) &xsessionBuf, sessionObj,
				sizeof(ExportSession));

		/*	Suspend chkpt retransmission timers.		*/

		for (elt2 = sdr_list_first(ltpSdr, xsessionBuf.redSegments);
				elt2; elt2 = sdr_list_next(ltpSdr, elt2))
		{
			segmentObj = sdr_list_data(ltpSdr, elt2);
			sdr_stage(ltpSdr, (char *) &dsBuf, segmentObj,
					sizeof(LtpXmitSeg));
			if (dsBuf.pdu.ckptSerialNbr == 0
			|| dsBuf.pdu.timer.segArrivalTime == 0)
			{
				continue;	/*	Not active.	*/
			}

			timer = &dsBuf.pdu.timer;
			suspendTimer(suspendTime, timer,
				segmentObj + FLD_OFFSET(timer, &dsBuf),
				qTime, priorXmitRate, LtpResendCheckpoint, 0,
				xsessionBuf.sessionNbr,
				dsBuf.pdu.ckptSerialNbr);
		}
	}

	return 0;
}

static int	resumeTimer(time_t resumeTime, LtpTimer *timer,
			Address timerAddr, unsigned int qTime,
			unsigned long remoteXmitRate, LtpEventType eventType,
			unsigned long refNbr1, unsigned long refNbr2,
			unsigned long refNbr3)
{
	time_t		earliestAckXmitStartTime;
	int		additionalDelay;
	LtpEvent	event;

	CHKERR(ionLocked());
	earliestAckXmitStartTime = timer->segArrivalTime + qTime;
	additionalDelay = resumeTime - earliestAckXmitStartTime;
	if (additionalDelay > 0)
	{
		/*	Must revise deadline.				*/

		timer->ackDeadline += additionalDelay;
	}

	/*	Change state of timer object and save it.		*/

	timer->state = LtpTimerRunning;
	sdr_write(getIonsdr(), timerAddr, (char *) timer, sizeof(LtpTimer));

	/*	Re-post timeout event.					*/

	event.type = eventType;
	event.refNbr1 = refNbr1;
	event.refNbr2 = refNbr2;
	event.refNbr3 = refNbr3;
	event.parm = 0;
	event.scheduledTime = timer->ackDeadline;
	if (insertLtpTimelineEvent(&event) == 0)
	{
		putErrmsg("Can't insert timeout event.", NULL);
		return -1;
	}

	return 0;
}

int	ltpResumeTimers(LtpVspan *vspan, PsmAddress vspanElt, time_t resumeTime,		unsigned long remoteXmitRate)
{
	Sdr		ltpSdr = getIonsdr();
	Object		spanObj;
			OBJ_POINTER(LtpSpan, span);
	unsigned int	qTime;
	Object		elt;
	Object		sessionObj;
	ImportSession	rsessionBuf;
	LtpTimer	*timer;
	Object		elt2;
	Object		segmentObj;
	LtpXmitSeg	rsBuf;
	ExportSession	xsessionBuf;
	LtpXmitSeg	dsBuf;

	CHKERR(ionLocked());
	CHKERR(vspan);
	spanObj = sdr_list_data(ltpSdr, vspan->spanElt);
	GET_OBJ_POINTER(ltpSdr, LtpSpan, span, spanObj);
	qTime = span->remoteQtime;

	/*	Resume relevant timers for import sessions.		*/

	for (elt = sdr_list_first(ltpSdr, span->deadImports); elt;
			elt = sdr_list_next(ltpSdr, elt))
	{
		sessionObj = sdr_list_data(ltpSdr, elt);
		sdr_stage(ltpSdr, (char *) &rsessionBuf, sessionObj,
				sizeof(ImportSession));
		if (rsessionBuf.timer.state != LtpTimerSuspended)
		{
			continue;		/*	Not suspended.	*/
		}

		/*	Must resume: re-insert timeout event.		*/

		timer = &rsessionBuf.timer;
		if (resumeTimer(resumeTime, timer,
			sessionObj + FLD_OFFSET(timer, &rsessionBuf),
			qTime, remoteXmitRate, LtpResendRecvCancel,
			span->engineId, rsessionBuf.sessionNbr, 0) < 0)

		{
			putErrmsg("Can't resume timers for span.",
					itoa(span->engineId));
			sdr_cancel_xn(ltpSdr);
			return -1;
		}
	}

	for (elt = sdr_list_first(ltpSdr, span->importSessions); elt;
			elt = sdr_list_next(ltpSdr, elt))
	{
		sessionObj = sdr_list_data(ltpSdr, elt);
		sdr_read(ltpSdr, (char *) &rsessionBuf, sessionObj,
				sizeof(ImportSession));

		/*	Resume report retransmission timers.		*/

		for (elt2 = sdr_list_first(ltpSdr, rsessionBuf.rsSegments);
				elt2; elt2 = sdr_list_next(ltpSdr, elt2))
		{
			segmentObj = sdr_list_data(ltpSdr, elt2);
			sdr_stage(ltpSdr, (char *) &rsBuf, segmentObj,
					sizeof(LtpXmitSeg));
			if (rsBuf.pdu.timer.segArrivalTime == 0)
			{
				continue;	/*	Not active.	*/
			}

			if (rsBuf.pdu.timer.state != LtpTimerSuspended)
			{
				continue;	/*	Not suspended.	*/
			}

			/*	Must resume: re-insert timeout event.	*/

			timer = &rsBuf.pdu.timer;
			if (resumeTimer(resumeTime, timer,
				segmentObj + FLD_OFFSET(timer, &rsBuf),
				qTime, remoteXmitRate, LtpResendReport,
				span->engineId, rsessionBuf.sessionNbr,
				rsBuf.pdu.rptSerialNbr) < 0)

			{
				putErrmsg("Can't resume timers for span.",
						itoa(span->engineId));
				sdr_cancel_xn(ltpSdr);
				return -1;
			}
		}
	}

	/*	Resume relevant timers for export sessions.		*/

	for (elt = sdr_list_first(ltpSdr, (_ltpConstants())->deadExports); elt;
			elt = sdr_list_next(ltpSdr, elt))
	{
		sessionObj = sdr_list_data(ltpSdr, elt);
		sdr_stage(ltpSdr, (char *) &xsessionBuf, sessionObj,
				sizeof(ExportSession));
		if (xsessionBuf.span != spanObj)
		{
			continue;	/*	Not for this span.	*/
		}

		if (xsessionBuf.timer.state != LtpTimerSuspended)
		{
			continue;		/*	Not suspended.	*/
		}

		/*	Must resume: re-insert timeout event.		*/

		timer = &xsessionBuf.timer;
		if (resumeTimer(resumeTime, timer,
			sessionObj + FLD_OFFSET(timer, &xsessionBuf),
			qTime, remoteXmitRate, LtpResendXmitCancel, 0,
			xsessionBuf.sessionNbr, 0) < 0)

		{
			putErrmsg("Can't resume timers for span.",
					itoa(span->engineId));
			sdr_cancel_xn(ltpSdr);
			return -1;
		}
	}

	for (elt = sdr_list_first(ltpSdr, span->exportSessions); elt;
			elt = sdr_list_next(ltpSdr, elt))
	{
		sessionObj = sdr_list_data(ltpSdr, elt);
		sdr_read(ltpSdr, (char *) &xsessionBuf, sessionObj,
				sizeof(ExportSession));

		/*	Resume chkpt retransmission timers.		*/

		for (elt2 = sdr_list_first(ltpSdr, xsessionBuf.redSegments);
				elt2; elt2 = sdr_list_next(ltpSdr, elt2))
		{
			segmentObj = sdr_list_data(ltpSdr, elt2);
			sdr_stage(ltpSdr, (char *) &dsBuf, segmentObj,
					sizeof(LtpXmitSeg));
			if (dsBuf.pdu.ckptSerialNbr == 0
			|| dsBuf.pdu.timer.segArrivalTime == 0)
			{
				continue;	/*	Not active.	*/
			}

			if (dsBuf.pdu.timer.state != LtpTimerSuspended)
			{
				continue;	/*	Not suspended.	*/
			}

			/*	Must resume: re-insert timeout event.	*/

			timer = &dsBuf.pdu.timer;
			if (resumeTimer(resumeTime, timer,
				segmentObj + FLD_OFFSET(timer, &dsBuf),
				qTime, remoteXmitRate, LtpResendCheckpoint, 0,
				xsessionBuf.sessionNbr, dsBuf.pdu.ckptSerialNbr)
				< 0)

			{
				putErrmsg("Can't resume timers for span.",
						itoa(span->engineId));
				sdr_cancel_xn(ltpSdr);
				return -1;
			}
		}
	}

	return 0;
}

int	ltpResendCheckpoint(unsigned long sessionNbr,
		unsigned long ckptSerialNbr)
{
	Sdr		ltpSdr = getIonsdr();
	Object		sessionObj;
	ExportSession	sessionBuf;
	Object		elt;
	Object		dsObj;
	LtpXmitSeg	dsBuf;
			OBJ_POINTER(LtpSpan, span);

	sdr_begin_xn(ltpSdr);
	getExportSession(sessionNbr, &sessionObj);
	if (sessionObj == 0)	/*	Session is gone.		*/
	{
#if LTPDEBUG
putErrmsg("Session is gone.", itoa(sessionNbr));
#endif
		return sdr_end_xn(ltpSdr);
	}

	sdr_stage(ltpSdr, (char *) &sessionBuf, sessionObj,
			sizeof(ExportSession));
	findCheckpoint(&sessionBuf, ckptSerialNbr, &elt, &dsObj);
	if (dsObj == 0)		/*	Checkpoint is gone.		*/
	{
#if LTPDEBUG
putErrmsg("Checkpoint is gone.", itoa(sessionNbr));
#endif
		return sdr_end_xn(ltpSdr);
	}

	sdr_stage(ltpSdr, (char *) &dsBuf, dsObj, sizeof(LtpXmitSeg));
	if (dsBuf.pdu.timer.segArrivalTime == 0)
	{
#if LTPDEBUG
putErrmsg("Checkpoint is already acknowledged.", itoa(sessionNbr));
#endif
		return sdr_end_xn(ltpSdr);
	}

	if (dsBuf.pdu.timer.expirationCount == MAX_RETRANSMISSIONS)
	{
#if LTPDEBUG
putErrmsg("Cancel by sender.", itoa(sessionNbr));
#endif
		cancelSessionBySender(&sessionBuf, sessionObj,
				LtpRetransmitLimitExceeded);
	}
	else
	{
#if LTPDEBUG
putErrmsg("Resending checkpoint.", itoa(sessionNbr));
#endif
		dsBuf.pdu.timer.expirationCount++;
		GET_OBJ_POINTER(ltpSdr, LtpSpan, span, sessionBuf.span);
		dsBuf.queueListElt = sdr_list_insert_last(ltpSdr,
				span->segments, dsObj);
		sdr_write(ltpSdr, dsObj, (char *) &dsBuf, sizeof(LtpXmitSeg));
		signalLso(span->engineId);
		if ((_ltpvdb(NULL))->watching & WATCH_resendCP)
		{
			putchar('=');
			fflush(stdout);
		}
	}

	if (sdr_end_xn(ltpSdr))
	{
		putErrmsg("Can't resend checkpoint.", NULL);
		return -1;
	}

	return 0;
}

int	ltpResendXmitCancel(unsigned long sessionNbr)
{
	Sdr		ltpSdr = getIonsdr();
	Object		sessionObj;
	Object		sessionElt;
	ExportSession	sessionBuf;
			OBJ_POINTER(LtpSpan, span);

#if LTPDEBUG
putErrmsg("Resending cancel by sender.", itoa(sessionNbr));
#endif
	sdr_begin_xn(ltpSdr);
	getCanceledExport(sessionNbr, &sessionObj, &sessionElt);
	if (sessionObj == 0)	/*	Session is gone.		*/
	{
#if LTPDEBUG
putErrmsg("Session is gone.", itoa(sessionNbr));
#endif
		sdr_exit_xn(ltpSdr);
		return 0;
	}

	sdr_stage(ltpSdr, (char *) &sessionBuf, sessionObj,
			sizeof(ExportSession));
	if (sessionBuf.timer.expirationCount == MAX_RETRANSMISSIONS)
	{
#if LTPDEBUG
putErrmsg("Retransmission limit exceeded.", itoa(sessionNbr));
#endif
		sdr_list_delete(ltpSdr, sessionElt, NULL, NULL);
		sdr_free(ltpSdr, sessionObj);
	}
	else	/*	Haven't given up yet.				*/
	{
		sessionBuf.timer.expirationCount++;
		sdr_write(ltpSdr, sessionObj, (char *) &sessionBuf,
				sizeof(ExportSession));
		GET_OBJ_POINTER(ltpSdr, LtpSpan, span, sessionBuf.span);
		if (constructSourceCancelReqSegment(span,
			&((_ltpConstants())->ownEngineIdSdnv), sessionNbr,
			sessionObj, sessionBuf.reasonCode) < 0)
		{
			putErrmsg("Can't resend cancel by sender.", NULL);
			sdr_cancel_xn(ltpSdr);
			return -1;
		}
	}

	if (sdr_end_xn(ltpSdr))
	{
		putErrmsg("Can't handle cancel request resend.", NULL);
		return -1;
	}

	return 0;
}

int	ltpResendReport(unsigned long engineId, unsigned long sessionNbr,
		unsigned long rptSerialNbr)
{
	Sdr		ltpSdr = getIonsdr();
	LtpVspan	*vspan;
	PsmAddress	vspanElt;
	Object		sessionObj;
	ImportSession	sessionBuf;
	Object		elt;
	Object		rsObj;
	LtpXmitSeg	rsBuf;
			OBJ_POINTER(LtpSpan, span);

#if LTPDEBUG
putErrmsg("Resending report.", itoa(sessionNbr));
#endif
	sdr_begin_xn(ltpSdr);
	findSpan(engineId, &vspan, &vspanElt);
	if (vspanElt == 0)	/*	Can't search for session.	*/
	{
		return sdr_end_xn(ltpSdr);
	}

	getImportSession(vspan, sessionNbr, &sessionObj);
	if (sessionObj == 0)	/*	Session is gone.		*/
	{
#if LTPDEBUG
putErrmsg("Session is gone.", itoa(sessionNbr));
#endif
		return sdr_end_xn(ltpSdr);
	}

	sdr_stage(ltpSdr, (char *) &sessionBuf, sessionObj,
			sizeof(ImportSession));
	findReport(&sessionBuf, rptSerialNbr, &elt, &rsObj);
	if (rsObj == 0)		/*	Report is gone.			*/
	{
		return sdr_end_xn(ltpSdr);
#if LTPDEBUG
putErrmsg("Report is gone.", itoa(sessionNbr));
#endif
	}

	sdr_stage(ltpSdr, (char *) &rsBuf, rsObj, sizeof(LtpXmitSeg));
	if (rsBuf.pdu.timer.expirationCount == MAX_RETRANSMISSIONS)
	{
#if LTPDEBUG
putErrmsg("Cancel by receiver.", itoa(sessionNbr));
#endif
		cancelSessionByReceiver(&sessionBuf, sessionObj,
				LtpRetransmitLimitExceeded);
	}
	else
	{
		rsBuf.pdu.timer.expirationCount++;
		GET_OBJ_POINTER(ltpSdr, LtpSpan, span, sdr_list_data(ltpSdr,
				vspan->spanElt));
		rsBuf.queueListElt = sdr_list_insert_last(ltpSdr,
				span->segments, rsObj);
		sdr_write(ltpSdr, rsObj, (char *) &rsBuf, sizeof(LtpXmitSeg));
		signalLso(span->engineId);
		if ((_ltpvdb(NULL))->watching & WATCH_resendRS)
		{
			putchar('+');
			fflush(stdout);
		}
	}

	if (sdr_end_xn(ltpSdr))
	{
		putErrmsg("Can't resend report.", NULL);
		return -1;
	}

	return 0;
}

int	ltpResendRecvCancel(unsigned long engineId, unsigned long sessionNbr)
{
	Sdr		ltpSdr = getIonsdr();
	LtpVspan	*vspan;
	PsmAddress	vspanElt;
	Object		sessionObj;
	Object		sessionElt;
	ImportSession	sessionBuf;
			OBJ_POINTER(LtpSpan, span);

#if LTPDEBUG
putErrmsg("Resending cancel by receiver.", itoa(sessionNbr));
#endif
	sdr_begin_xn(ltpSdr);
	findSpan(engineId, &vspan, &vspanElt);
	if (vspanElt == 0)	/*	Can't search for session.	*/
	{
		return sdr_end_xn(ltpSdr);
	}

	getCanceledImport(vspan, sessionNbr, &sessionObj, &sessionElt);
	if (sessionObj == 0)	/*	Session is gone.		*/
	{
#if LTPDEBUG
putErrmsg("Session is gone.", itoa(sessionNbr));
#endif
		return sdr_end_xn(ltpSdr);
	}

	sdr_stage(ltpSdr, (char *) &sessionBuf, sessionObj,
			sizeof(ImportSession));
	if (sessionBuf.timer.expirationCount == MAX_RETRANSMISSIONS)
	{
#if LTPDEBUG
putErrmsg("Retransmission limit exceeded.", itoa(sessionNbr));
#endif
		sdr_list_delete(ltpSdr, sessionElt, NULL, NULL);
		sdr_free(ltpSdr, sessionObj);
	}
	else	/*	Haven't given up yet.				*/
	{
		sessionBuf.timer.expirationCount++;
		sdr_write(ltpSdr, sessionObj, (char *) &sessionBuf,
			sizeof(ImportSession));
		GET_OBJ_POINTER(ltpSdr, LtpSpan, span, sdr_list_data(ltpSdr,
			vspan->spanElt));
		if (constructDestCancelReqSegment(span, &(span->engineIdSdnv),
			sessionNbr, sessionObj, sessionBuf.reasonCode) < 0)
		{
			putErrmsg("Can't resend cancel by receiver.", NULL);
			sdr_cancel_xn(ltpSdr);
			return -1;
		}
	}

	if (sdr_end_xn(ltpSdr))
	{
		putErrmsg("Can't handle cancel request resend.", NULL);
		return -1;
	}

	return 0;
}
