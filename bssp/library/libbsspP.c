/*
 *	libbsspP.c:	private functions enabling the implementation of
 *			BSSP engines.
 *
 *	Copyright (c) 2013, California Institute of Technology.
 *	Copyright (c) 2013, Space Internetworking Center,
 *	Democritus University of Thrace.
 *	
 *	All rights reserved. U.S. Government and E.U. Sponsorship acknowledged.
 *				
 *	Author: Sotirios-Angelos Lenas, Space Internetworking Center
 */

#include "bsspP.h"

#define	EST_LINK_OHD	16

#ifndef BSSPDEBUG
#define	BSSPDEBUG	0
#endif

#define BSSP_VERSION	0

static void	getSessionContext(BsspDB *BsspDB, unsigned int sessionNbr,
			Object *sessionObj, ExportSession *sessionBuf,
			Object *spanObj, BsspSpan *spanBuf, BsspVspan **vspan,
			PsmAddress *vspanElt);

/*	*	*	Helpful utility functions	*	*	*/

static Object	_bsspdbObject(Object *newDbObj)
{
	static Object	obj = 0;

	if (newDbObj)
	{
		obj = *newDbObj;
	}

	return obj;
}

static BsspDB	*_bsspConstants()
{
	static BsspDB	buf;
	static BsspDB	*db = NULL;
	Sdr		sdr;
	Object		dbObject;

	if (db == NULL)
	{
		/*	Load constants into a conveniently accessed
		 *	structure.  Note that this CANNOT be treated
		 *	as a current database image in later
		 *	processing.					*/

		sdr = getIonsdr();
		CHKNULL(sdr);
		dbObject = _bsspdbObject(NULL);
		if (dbObject)
		{
			if (sdr_heap_is_halted(sdr))
			{
				sdr_read(sdr, (char *) &buf, dbObject,
						sizeof(BsspDB));
			}
			else
			{
				CHKNULL(sdr_begin_xn(sdr));
				sdr_read(sdr, (char *) &buf, dbObject,
						sizeof(BsspDB));
				sdr_exit_xn(sdr);
			}

			db = &buf;
		}
	}

	return db;
}

/*	*	*	BSSP service control functions	*	*	*/

static void	resetClient(BsspVclient *client)
{
	if (client->semaphore == SM_SEM_NONE)
	{
		client->semaphore = sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO);
	}
	else
	{
		sm_SemUnend(client->semaphore);
		sm_SemGive(client->semaphore);
	}

	sm_SemTake(client->semaphore);			/*	Lock.	*/
	client->pid = ERROR;				/*	None.	*/
}

static void	raiseClient(BsspVclient *client)
{
	client->semaphore = SM_SEM_NONE;
	resetClient(client);
}

static void	resetSpan(BsspVspan *vspan)
{
	if (vspan->bufOpenSemaphore == SM_SEM_NONE)
	{
		vspan->bufOpenSemaphore = sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO);
	}
	else
	{
		sm_SemUnend(vspan->bufOpenSemaphore);
		sm_SemGive(vspan->bufOpenSemaphore);
	}

	sm_SemTake(vspan->bufOpenSemaphore);		/*	Lock.	*/
	if (vspan->beSemaphore == SM_SEM_NONE)
	{
		vspan->beSemaphore = sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO);
	}
	else
	{
		sm_SemUnend(vspan->beSemaphore);
		sm_SemGive(vspan->beSemaphore);
	}

	sm_SemTake(vspan->beSemaphore);		/*	Lock.	*/
	if (vspan->rlSemaphore == SM_SEM_NONE)
	{
		vspan->rlSemaphore = sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO);
	}
	else
	{
		sm_SemUnend(vspan->rlSemaphore);
		sm_SemGive(vspan->rlSemaphore);
	}

	sm_SemTake(vspan->rlSemaphore);		/*	Lock.	*/
	vspan->bsoBEPid = ERROR;		/*	None.	*/
	vspan->bsoRLPid = ERROR;		/*	None.	*/
}

static int	raiseSpan(Object spanElt, BsspVdb *bsspvdb)
{
	Sdr		bsspSdr = getIonsdr();
	PsmPartition	bsspwm = getIonwm();
	Object		spanObj;
	BsspSpan	span;
	BsspVspan	*vspan;
	PsmAddress	vspanElt;
	PsmAddress	addr;

	spanObj = sdr_list_data(bsspSdr, spanElt);
	sdr_read(bsspSdr, (char *) &span, spanObj, sizeof(BsspSpan));
	findBsspSpan(span.engineId, &vspan, &vspanElt);
	if (vspanElt)	/*	Span is already raised.			*/
	{
		return 0;
	}

	addr = psm_zalloc(bsspwm, sizeof(BsspVspan));
	if (addr == 0)
	{
		return -1;
	}

	vspanElt = sm_list_insert_last(bsspwm, bsspvdb->spans, addr);
	if (vspanElt == 0)
	{
		psm_free(bsspwm, addr);
		return -1;
	}

	vspan = (BsspVspan *) psp(bsspwm, addr);
	memset((char *) vspan, 0, sizeof(BsspVspan));
	vspan->spanElt = spanElt;
	vspan->engineId = span.engineId;
	vspan->beBuffer = psm_malloc(bsspwm, span.maxBlockSize);
	if (vspan->beBuffer == 0)
	{
		oK(sm_list_delete(bsspwm, vspanElt, NULL, NULL));
		psm_free(bsspwm, addr);
		return -1;
	}

	vspan->rlBuffer = psm_malloc(bsspwm, span.maxBlockSize);
	if (vspan->rlBuffer == 0)
	{
		oK(sm_list_delete(bsspwm, vspanElt, NULL, NULL));
		psm_free(bsspwm, addr);
		return -1;
	}

	vspan->bufOpenSemaphore = SM_SEM_NONE;
	vspan->beSemaphore = SM_SEM_NONE;
	vspan->rlSemaphore = SM_SEM_NONE;
	resetSpan(vspan);
	return 0;
}

static void	dropSpan(BsspVspan *vspan, PsmAddress vspanElt)
{
	PsmPartition	bsspwm = getIonwm();
	PsmAddress	vspanAddr;

	vspanAddr = sm_list_data(bsspwm, vspanElt);
	if (vspan->bufOpenSemaphore != SM_SEM_NONE)
	{
		sm_SemEnd(vspan->bufOpenSemaphore);
		microsnooze(50000);
		sm_SemDelete(vspan->bufOpenSemaphore);
	}

	if (vspan->beSemaphore != SM_SEM_NONE)
	{
		sm_SemEnd(vspan->beSemaphore);
		microsnooze(50000);
		sm_SemDelete(vspan->beSemaphore);
	}

	if (vspan->rlSemaphore != SM_SEM_NONE)
	{
		sm_SemEnd(vspan->rlSemaphore);
		microsnooze(50000);
		sm_SemDelete(vspan->rlSemaphore);
	}

	psm_free(bsspwm, vspan->beBuffer);
	psm_free(bsspwm, vspan->rlBuffer);
	oK(sm_list_delete(bsspwm, vspanElt, NULL, NULL));
	psm_free(bsspwm, vspanAddr);
}

Object	getBsspDbObject()
{
	return _bsspdbObject(NULL);
}

int	startBsspExportSession(Sdr sdr, Object spanObj, BsspVspan *vspan)
{
	Object		dbobj;
	BsspSpan	span;
	BsspDB		bsspdb;
	unsigned int	sessionNbr;
	Object		sessionObj;
	Object		elt;
	ExportSession	session;

	CHKERR(vspan);
	CHKERR(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &span, spanObj, sizeof(BsspSpan));

	/*	Get next session number.				*/

	dbobj = getBsspDbObject();
	
	sdr_stage(sdr, (char *) &bsspdb, dbobj, sizeof(BsspDB));
	bsspdb.sessionCount++;
	sdr_write(sdr, dbobj, (char *) &bsspdb, sizeof(BsspDB));
	sessionNbr = bsspdb.sessionCount;

	/*	Record the session object in the database. The
	 *	exportSessions list element points to the session
	 *	structure.  exportSessionHash entry points to the
	 *	list element.						*/

	sessionObj = sdr_malloc(sdr, sizeof(ExportSession));
	if (sessionObj == 0
	|| (elt = sdr_list_insert_last(sdr, span.exportSessions,
			sessionObj)) == 0
	|| sdr_hash_insert(sdr, bsspdb.exportSessionsHash,
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
	session.svcDataObject = 0;
	session.block = 0;
	sdr_write(sdr, sessionObj, (char *) &session, sizeof(ExportSession));

	/*	Note session address in span, then finish: unless span
	 *	is currently inactive (i.e., localXmitRate is currently
	 *	zero), give the buffer-empty semaphore so that the
	 *	pending service data object (if any) can be inserted
	 *	into the buffer.					*/

	span.currentExportSessionObj = sessionObj;
	sdr_write(sdr, spanObj, (char *) &span, sizeof(BsspSpan));
	if (vspan->localXmitRate > 0)
	{
		sm_SemGive(vspan->bufOpenSemaphore);
	}
	if (sdr_end_xn(sdr))
	{
		putErrmsg("Can't start session.", NULL);
		return -1;
	}
	return 0;
}

static void	startSpan(BsspVspan *vspan)
{
	Sdr		bsspSdr = getIonsdr();
	BsspSpan	span;
	Object		spanObj;
	char		cmd[SDRSTRING_BUFSZ];
	char		engineIdString[11];
	char		bsoCmdString[SDRSTRING_BUFSZ + 64];

	CHKVOID(sdr_begin_xn(bsspSdr));
	if (vspan->spanElt == 0)
	{
		putErrmsg("No such engine in database.", NULL);
		return ;
	}

	spanObj = sdr_list_data(bsspSdr, vspan->spanElt);
	sdr_read(bsspSdr, (char *) &span, spanObj, sizeof(BsspSpan));
	
	if (span.currentExportSessionObj == 0)	/* New span.	*/
	{
		/*	Must start span's initial session.	*/
		sdr_exit_xn(bsspSdr);
		if (startBsspExportSession(bsspSdr, spanObj, vspan) < 0)
		{
			putErrmsg("Failed to initialize export session object.",
					NULL);
		}

		CHKVOID(sdr_begin_xn(bsspSdr));
	}

	sdr_string_read(bsspSdr, cmd, span.bsoBECmd);
	isprintf(engineIdString, sizeof engineIdString, UVAST_FIELDSPEC,
			span.engineId);
	isprintf(bsoCmdString, sizeof bsoCmdString, "%s %s", cmd,
			engineIdString);
	vspan->bsoBEPid = pseudoshell(bsoCmdString);

	sdr_string_read(bsspSdr, cmd, span.bsoRLCmd);
	isprintf(bsoCmdString, sizeof bsoCmdString, "%s %s", cmd,
			engineIdString);
	vspan->bsoRLPid = pseudoshell(bsoCmdString);

	sdr_exit_xn(bsspSdr);
}

static void	stopSpan(BsspVspan *vspan)
{
	if (vspan->bufOpenSemaphore != SM_SEM_NONE)
	{
		sm_SemEnd(vspan->bufOpenSemaphore);
	}

	if (vspan->beSemaphore != SM_SEM_NONE)
	{
		sm_SemEnd(vspan->beSemaphore);
	}

	if (vspan->rlSemaphore != SM_SEM_NONE)
	{
		sm_SemEnd(vspan->rlSemaphore);
	}
}

static void	waitForSpan(BsspVspan *vspan)
{
	if (vspan->bsoBEPid != ERROR)
	{
		while (sm_TaskExists(vspan->bsoBEPid))
		{
			microsnooze(100000);
		}
	}

	if (vspan->bsoRLPid != ERROR)
	{
		while (sm_TaskExists(vspan->bsoRLPid))
		{
			microsnooze(100000);
		}
	}
}

static char 	*_bsspvdbName()
{
	return "bsspvdb";
}

static BsspVdb		*_bsspvdb(char **name)
{
	static BsspVdb	*vdb = NULL;
	
	if (name)
	{
		if (*name == NULL)	/*	Terminating.		*/
		{
			vdb = NULL;
			return vdb;
		}

		/*	Attaching to volatile database.			*/

		PsmPartition	wm;
		PsmAddress	vdbAddress;
		PsmAddress	elt;

		wm = getIonwm();
		if (psm_locate(wm, *name, &vdbAddress, &elt) < 0)
		{
			putErrmsg("Failed searching for vdb.", NULL);
			return vdb;
		}

		if (elt)
		{
			vdb = (BsspVdb *) psp(wm, vdbAddress);
			return vdb;
		}

		/*	BSSP volatile database doesn't exist yet.	*/
		Sdr		sdr;
		BsspDB		*db;
		Object		sdrElt;
		int		i;
		BsspVclient	*client;

		sdr = getIonsdr();
		CHKNULL(sdr_begin_xn(sdr));	/*	To lock memory.	*/

		/*	Create and catalogue the BsspVdb object.	*/

		vdbAddress = psm_zalloc(wm, sizeof(BsspVdb));
		if (vdbAddress == 0)
		{
			sdr_exit_xn(sdr);
			putErrmsg("No space for dynamic database.", NULL);
			return NULL;
		}

		db = _bsspConstants();
		vdb = (BsspVdb *) psp(wm, vdbAddress);
		memset((char *) vdb, 0, sizeof(BsspVdb));
		vdb->ownEngineId = db->ownEngineId;
		vdb->beBsiPid = ERROR;		/*	None yet.	*/
		vdb->rlBsiPid = ERROR;		/*	None yet.	*/
		vdb->clockPid = ERROR;		/*	None yet.	*/
		if ((vdb->spans = sm_list_create(wm)) == 0
		|| psm_catlg(wm, *name, vdbAddress) < 0)
		{
			sdr_exit_xn(sdr);
			putErrmsg("Can't initialize volatile database.", NULL);
			return NULL;
		}

		/*	Raise all clients.				*/

		for (i = 0, client = vdb->clients; i < BSSP_MAX_NBR_OF_CLIENTS;
				i++, client++)
		{
			client->notices = db->clients[i].notices;
			raiseClient(client);
		}

		/*	Raise all spans.				*/

		for (sdrElt = sdr_list_first(sdr, db->spans);
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

static char	*_bsspdbName()
{
	return "bsspdb";
}

int	bsspInit(int estMaxExportSessions)
{
	Sdr	bsspSdr;
	Object	bsspdbObject;
	IonDB	iondb;
	BsspDB	bsspdbBuf;
	int	i;
	char	*bsspvdbName = _bsspvdbName();
	if (ionAttach() < 0)
	{
		putErrmsg("BSSP can't attach to ION.", NULL);
		return -1;
	}

	bsspSdr = getIonsdr();
	srand(time(NULL));

	/*	Recover the BSSP database, creating it if necessary.	*/

	CHKERR(sdr_begin_xn(bsspSdr));
	bsspdbObject = sdr_find(bsspSdr, _bsspdbName(), NULL);

	switch (bsspdbObject)
	{
	case -1:		/*	SDR error.			*/

		putErrmsg("Can't search for BSSP database in SDR.", NULL);
		sdr_cancel_xn(bsspSdr);
		return -1;

	case 0:			/*	Not found; must create new DB.	*/

		if (estMaxExportSessions <= 0)
		{
			sdr_exit_xn(bsspSdr);
			putErrmsg("Must supply estMaxExportSessions.", NULL);
			return -1;
		}
		
		sdr_read(bsspSdr, (char *) &iondb, getIonDbObject(),
				sizeof(IonDB));
		bsspdbObject = sdr_malloc(bsspSdr, sizeof(BsspDB));
		if (bsspdbObject == 0)
		{
			putErrmsg("No space for database.", NULL);
			sdr_cancel_xn(bsspSdr);
			return -1;
		}

		/*	Initialize the non-volatile database.		*/

		memset((char *) &bsspdbBuf, 0, sizeof(BsspDB));
		bsspdbBuf.ownEngineId = iondb.ownNodeNbr;
		encodeSdnv(&(bsspdbBuf.ownEngineIdSdnv), bsspdbBuf.ownEngineId);
		bsspdbBuf.estMaxExportSessions = estMaxExportSessions;
		bsspdbBuf.ownQtime = 1;		/*	Default.	*/
		bsspdbBuf.sessionCount = 0;
		for (i = 0; i < BSSP_MAX_NBR_OF_CLIENTS; i++)
		{
			bsspdbBuf.clients[i].notices = sdr_list_create(bsspSdr);
		}
		bsspdbBuf.exportSessionsHash = sdr_hash_create(bsspSdr,
				sizeof(unsigned int), estMaxExportSessions,
				BSSP_MEAN_SEARCH_LENGTH);
		bsspdbBuf.spans = sdr_list_create(bsspSdr);
		bsspdbBuf.timeline = sdr_list_create(bsspSdr);
		
		/*	Initialize sessionCount with a random value, 	*
		 *	to minimize the risk of DoS attacks since	*
		 *	this value also serves as the unique serial	*
		 *	number per ExportSession used in the		*
		 *	acknowledgement procedure.			*
		 */

		do
		{
			bsspdbBuf.sessionCount = rand();
		} while (bsspdbBuf.sessionCount == 0);
		sdr_write(bsspSdr, bsspdbObject, (char *) &bsspdbBuf,
				sizeof(BsspDB));
		sdr_catlg(bsspSdr, _bsspdbName(), 0, bsspdbObject);
		if (sdr_end_xn(bsspSdr))
		{
			putErrmsg("Can't create BSSP database.", NULL);
			return -1;
		}

		break;

	default:		/*	Found DB in the SDR.		*/

		sdr_exit_xn(bsspSdr);
	}

	oK(_bsspdbObject(&bsspdbObject));	/*	Save location.	*/
	oK(_bsspConstants());
	/*	Load volatile database, initializing as necessary.	*/
	if (_bsspvdb(&bsspvdbName) == NULL)
	{
		putErrmsg("BSSP can't initialize vdb.", NULL);
		return -1;
	}

	return 0;		/*	BSSP service is available.	*/
}

static void	dropVdb(PsmPartition wm, PsmAddress vdbAddress)
{
	BsspVdb		*vdb;
	int		i;
	BsspVclient	*client;
	PsmAddress	elt;
	BsspVspan	*vspan;

	vdb = (BsspVdb *) psp(wm, vdbAddress);
	for (i = 0, client = vdb->clients; i < BSSP_MAX_NBR_OF_CLIENTS;
			i++, client++)
	{
		if (client->semaphore != SM_SEM_NONE)
		{
			sm_SemEnd(client->semaphore);
			microsnooze(50000);
			sm_SemDelete(client->semaphore);
		}
	}

	while ((elt = sm_list_first(wm, vdb->spans)) != 0)
	{
		vspan = (BsspVspan *) psp(wm, sm_list_data(wm, elt));
		dropSpan(vspan, elt);
	}

	sm_list_destroy(wm, vdb->spans, NULL, NULL);
}

void	bsspDropVdb()
{
	PsmPartition	wm = getIonwm();
	char		*bsspvdbName = _bsspvdbName();
	PsmAddress	vdbAddress;
	PsmAddress	elt;
	char		*stop = NULL;

	/*	Destroy volatile database.				*/

	if (psm_locate(wm, bsspvdbName, &vdbAddress, &elt) < 0)
	{
		putErrmsg("BSSP failed searching for vdb.", NULL);
		return;
	}

	if (elt)
	{
		dropVdb(wm, vdbAddress);	/*	Destroy Vdb.	*/
		psm_free(wm,vdbAddress);
		if (psm_uncatlg(wm, bsspvdbName) < 0)
		{
			putErrmsg("BSSP failed uncataloging vdb.",NULL);
		}
	}

	oK(_bsspvdb(&stop));			/*	Forget old Vdb.	*/
}

void	bsspRaiseVdb()
{
	char	*bsspvdbName = _bsspvdbName();

	if (_bsspvdb(&bsspvdbName) == NULL)	/*	Create new Vdb.	*/
	{
		putErrmsg("BSSP can't reinitialize vdb.", NULL);
	}
}

BsspDB	*getBsspConstants()
{
	return _bsspConstants();
}

BsspVdb	*getBsspVdb()
{
	return _bsspvdb(NULL);
}

int	bsspStart(char *beBsiCmd, char *rlBsiCmd)
{
	Sdr		bsspSdr = getIonsdr();
	PsmPartition	bsspwm = getIonwm();
	BsspVdb		*bsspvdb = _bsspvdb(NULL);
	PsmAddress	elt;

	if (beBsiCmd == NULL || rlBsiCmd == NULL)
	{
		putErrmsg("BSSP can't start: no BE-BSI or RL-BSI command.",
				NULL);
		return -1;
	}

	CHKERR(sdr_begin_xn(bsspSdr));	/*	Just to lock memory.	*/

	/*	Start the BSSP events clock if necessary.		*/

	if (bsspvdb->clockPid == ERROR || sm_TaskExists(bsspvdb->clockPid) == 0)
	{
		bsspvdb->clockPid = pseudoshell("bsspclock");
	}

	/*	Start input link service if necessary.			*/

	if (bsspvdb->beBsiPid == ERROR || sm_TaskExists(bsspvdb->beBsiPid) == 0)
	{
		bsspvdb->beBsiPid = pseudoshell(beBsiCmd);
	}

	if (bsspvdb->rlBsiPid == ERROR || sm_TaskExists(bsspvdb->rlBsiPid) == 0)
	{
		bsspvdb->rlBsiPid = pseudoshell(rlBsiCmd);
	}

	 sdr_exit_xn(bsspSdr);		/* 	Unlock memory.		*/

	/*	Start output link services for remote spans.		*/

	for (elt = sm_list_first(bsspwm, bsspvdb->spans); elt;
			elt = sm_list_next(bsspwm, elt))
	{
		startSpan((BsspVspan *) psp(bsspwm, sm_list_data(bsspwm, elt)));
	}

	return 0;
}

void	bsspStop()		/*	Reverses bsspStart.		*/
{
	Sdr		bsspSdr = getIonsdr();
	PsmPartition	bsspwm = getIonwm();
	BsspVdb		*bsspvdb = _bsspvdb(NULL);
	int		i;
	BsspVclient	*client;
	PsmAddress	elt;
	BsspVspan	*vspan;

	/*	Tell all BSSP processes to stop.	*/

	CHKVOID(sdr_begin_xn(bsspSdr));	/*	Just to lock memory.	*/
	for (i = 0, client = bsspvdb->clients; i < BSSP_MAX_NBR_OF_CLIENTS;
			i++, client++)
	{
		if (client->semaphore != SM_SEM_NONE)
		{
			sm_SemEnd(client->semaphore);
		}
	}

	if (bsspvdb->beBsiPid != ERROR)
	{
		sm_TaskKill(bsspvdb->beBsiPid, SIGTERM);
	}

	if (bsspvdb->rlBsiPid != ERROR)
	{
		sm_TaskKill(bsspvdb->rlBsiPid, SIGTERM);
	}

	for (elt = sm_list_first(bsspwm, bsspvdb->spans); elt;
			elt = sm_list_next(bsspwm, elt))
	{
		vspan = (BsspVspan *) psp(bsspwm, sm_list_data(bsspwm, elt));
		stopSpan(vspan);
	}

	if (bsspvdb->clockPid != ERROR)
	{
		sm_TaskKill(bsspvdb->clockPid, SIGTERM);
	}

	sdr_exit_xn(bsspSdr);	/*	Unlock memory.			*/

	/*	Wait until all BSSP processes have stopped.		*/

	if (bsspvdb->beBsiPid != ERROR)
	{
		while (sm_TaskExists(bsspvdb->beBsiPid))
		{
			microsnooze(100000);
		}
	}

	if (bsspvdb->rlBsiPid != ERROR)
	{
		while (sm_TaskExists(bsspvdb->rlBsiPid))
		{
			microsnooze(100000);
		}
	}

	for (elt = sm_list_first(bsspwm, bsspvdb->spans); elt;
			elt = sm_list_next(bsspwm, elt))
	{
		vspan = (BsspVspan *) psp(bsspwm, sm_list_data(bsspwm, elt));
		waitForSpan(vspan);
	}

	if (bsspvdb->clockPid != ERROR)
	{
		while (sm_TaskExists(bsspvdb->clockPid))
		{
			microsnooze(100000);
		}
	}

	/*	Now erase all the tasks and reset the semaphores.	*/

	CHKVOID(sdr_begin_xn(bsspSdr));	/*	Just to lock memory.	*/
	bsspvdb->clockPid = ERROR;
	for (i = 0, client = bsspvdb->clients; i < BSSP_MAX_NBR_OF_CLIENTS;
			i++, client++)
	{
		resetClient(client);
	}

	bsspvdb->beBsiPid = ERROR;
	bsspvdb->rlBsiPid = ERROR;
	for (elt = sm_list_first(bsspwm, bsspvdb->spans); elt;
			elt = sm_list_next(bsspwm, elt))
	{
		vspan = (BsspVspan *) psp(bsspwm, sm_list_data(bsspwm, elt));
		resetSpan(vspan);
	}

	sdr_exit_xn(bsspSdr);	/*	Unlock memory.			*/
}

int	bsspAttach()
{
	Object	bsspdbObject = _bsspdbObject(NULL);
	BsspVdb	*bsspvdb = _bsspvdb(NULL);
	Sdr	bsspSdr;
	char	*bsspvdbName = _bsspvdbName();

	if (bsspdbObject && bsspvdb)
	{
		return 0;		/*	Already attached.	*/
	}

	if (ionAttach() < 0)
	{
		putErrmsg("BSSP can't attach to ION.", NULL);
		return -1;
	}

	bsspSdr = getIonsdr();
	srand(time(NULL));

	/*	Locate the BSSP database.				*/

	if (bsspdbObject == 0)
	{
		CHKERR(sdr_begin_xn(bsspSdr));
		bsspdbObject = sdr_find(bsspSdr, _bsspdbName(), NULL);
		sdr_exit_xn(bsspSdr);
		if (bsspdbObject == 0)
		{
			putErrmsg("Can't find BSSP database.", NULL);
			return -1;
		}

		oK(_bsspdbObject(&bsspdbObject));
	}

	oK(_bsspConstants());

	/*	Locate the BSSP volatile database.			*/

	if (bsspvdb == NULL)
	{
		if (_bsspvdb(&bsspvdbName) == NULL)
		{
			putErrmsg("BSSP volatile database not found.", NULL);
			return -1;
		}
	}

	return 0;		/*	BSSP service is available.	*/
}

void	bsspDetach()
{
	char	*stop = NULL;

	oK(_bsspvdb(&stop));
	return;
}

/*	*	*	BSSP span mgt and access functions	*	*/

void	findBsspSpan(uvast engineId, BsspVspan **vspan, PsmAddress *vspanElt)
{
	PsmPartition	bsspwm = getIonwm();
	PsmAddress	elt;

	CHKVOID(ionLocked());
	CHKVOID(vspan);
	CHKVOID(vspanElt);
	for (elt = sm_list_first(bsspwm, (_bsspvdb(NULL))->spans); elt;
			elt = sm_list_next(bsspwm, elt))
	{
		*vspan = (BsspVspan *) psp(bsspwm, sm_list_data(bsspwm, elt));
		if ((*vspan)->engineId == engineId)
		{
			break;
		}
	}

	*vspanElt = elt;	/*	(Zero if vspan was not found.)	*/
}
#if 0
void	checkReservationLimit()
{
	Sdr	bsspSdr = getIonsdr();
	Object	dbobj = getBsspDbObject();
	BsspDB	db;
	int	totalSessionsAvbl;
	Object	elt;
		OBJ_POINTER(BsspSpan, span);

	CHKVOID(sdr_begin_xn(bsspSdr));
	sdr_read(bsspSdr, (char *) &db, dbobj, sizeof(BsspDB));
	totalSessionsAvbl = db.estMaxExportSessions;
	for (elt = sdr_list_first(bsspSdr, db.spans); elt;
			elt = sdr_list_next(bsspSdr, elt))
	{
		GET_OBJ_POINTER(bsspSdr, BsspSpan, span, sdr_list_data(bsspSdr,
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

	sdr_exit_xn(bsspSdr);
}
#endif
int	addBsspSpan(uvast engineId, unsigned int maxExportSessions,
		unsigned int maxBlockSize, char *bsoBECmd, char *bsoRLCmd, 
		unsigned int qTime, int purge)
{
	Sdr		bsspSdr = getIonsdr();
	BsspVspan	*vspan;
	PsmAddress	vspanElt;
	BsspSpan	spanBuf;
	Object		addr;
	Object		spanElt = 0;

	if (bsoBECmd == NULL || *bsoBECmd == '\0' || 
		bsoRLCmd == NULL || *bsoRLCmd == '\0')
	{
		writeMemoNote("[?] BSO commands missing, can't add span",
				utoa(engineId));
		return 0;
	}

	if (engineId == 0 || maxExportSessions == 0)
	{
		writeMemoNote("[?] Missing span parameter(s)", utoa(engineId));
		return 0;
	}

	if (strlen(bsoBECmd) > MAX_SDRSTRING )
	{
		writeMemoNote("[?] Link service output command string too long",
				bsoBECmd);
		return 0;
	}

	if (strlen(bsoRLCmd) > MAX_SDRSTRING )
	{
		writeMemoNote("[?] Link service output command string too long",
				bsoRLCmd);
		return 0;
	}

	/*	Note: RFC791 says that IPv4 hosts cannot set maximum
	 *	IP packet length to any value less than 576 bytes (the
	 *	PPP MTU size).  IPv4 packet header length ranges from
	 *	20 to 60 bytes, and UDP header length is 8 bytes.  So
	 *	the maximum allowed size for a UDP datagram on a given
	 *	host should not be less than 508 bytes, so we warn if
	 *	maximum BSSP block size is less than 508.		*/

	if (maxBlockSize < 508)
	{
		writeMemoNote("[i] Note max segment size is less than 508",
				utoa(maxBlockSize));
	}

	CHKERR(sdr_begin_xn(bsspSdr));
	findBsspSpan(engineId, &vspan, &vspanElt);
	if (vspanElt)		/*	This is a known span.		*/
	{
		sdr_exit_xn(bsspSdr);
		writeMemoNote("[?] Duplicate span", itoa(engineId));
		return 0;
	}

	/*	All parameters validated, okay to add the span.		*/

	memset((char *) &spanBuf, 0, sizeof(BsspSpan));
	spanBuf.engineId = engineId;
	encodeSdnv(&(spanBuf.engineIdSdnv), spanBuf.engineId);
	spanBuf.remoteQtime = qTime;
	spanBuf.purge = purge ? 1 : 0;
	spanBuf.bsoBECmd = sdr_string_create(bsspSdr, bsoBECmd);
	spanBuf.bsoRLCmd = sdr_string_create(bsspSdr, bsoRLCmd);
	spanBuf.maxExportSessions = maxExportSessions;
	spanBuf.maxBlockSize = maxBlockSize;
	spanBuf.exportSessions = sdr_list_create(bsspSdr);
	spanBuf.beBlocks = sdr_list_create(bsspSdr);
	spanBuf.rlBlocks = sdr_list_create(bsspSdr);
	
	addr = sdr_malloc(bsspSdr, sizeof(BsspSpan));
	if (addr)
	{
		spanElt = sdr_list_insert_last(bsspSdr, _bsspConstants()->spans,
				addr);
		sdr_write(bsspSdr, addr, (char *) &spanBuf, sizeof(BsspSpan));
	}

	if (sdr_end_xn(bsspSdr) < 0 || spanElt == 0)
	{
		putErrmsg("Can't add span.", itoa(engineId));
		return -1;
	}

	CHKERR(sdr_begin_xn(bsspSdr));	/*	Just to lock memory.	*/
	if (raiseSpan(spanElt, _bsspvdb(NULL)) < 0)
	{
		sdr_exit_xn(bsspSdr);
		putErrmsg("Can't raise span.", NULL);
		return -1;
	}

	sdr_exit_xn(bsspSdr);
	return 1;
}

int	updateBsspSpan(uvast engineId, unsigned int maxExportSessions,
		unsigned int maxBlockSize, char *bsoBECmd, char *bsoRLCmd, 
		unsigned int qTime, int purge)
{
	Sdr		bsspSdr = getIonsdr();
	BsspVspan	*vspan;
	PsmAddress	vspanElt;
	Object		addr;
	BsspSpan		spanBuf;

	if (bsoBECmd)
	{
		if (*bsoBECmd == '\0')
		{
			writeMemoNote("[?] No BE LSO command, can't update \
span", utoa(engineId));
			return 0;
		}
		else
		{
			if (strlen(bsoBECmd) > MAX_SDRSTRING)
			{
				writeMemoNote("[?] Link service output command \
string too long.", bsoBECmd);
				return 0;
			}
		}
	}

	if (bsoRLCmd)
	{
		if (*bsoRLCmd == '\0')
		{
			writeMemoNote("[?] No RL LSO command, can't update \
span", utoa(engineId));
			return 0;
		}
		else
		{
			if (strlen(bsoRLCmd) > MAX_SDRSTRING)
			{
				writeMemoNote("[?] Link service output command \
string too long.", bsoRLCmd);
				return 0;
			}
		}
	}

	if (maxBlockSize)
	{
		if (maxBlockSize < 508)
		{
			writeMemoNote("[i] Note max segment size is less than \
508", utoa(maxBlockSize));
		}
	}

	CHKERR(sdr_begin_xn(bsspSdr));
	findBsspSpan(engineId, &vspan, &vspanElt);
	if (vspanElt == 0)	/*	This is an unknown span.	*/
	{
		sdr_exit_xn(bsspSdr);
		writeMemoNote("[?] Unknown span", itoa(engineId));
		return 0;
	}

	addr = (Object) sdr_list_data(bsspSdr, vspan->spanElt);
	sdr_stage(bsspSdr, (char *) &spanBuf, addr, sizeof(BsspSpan));
	if (maxExportSessions == 0)
	{
		maxExportSessions = spanBuf.maxExportSessions;
	}

	/*	All parameters validated, okay to update the span.	*/

	spanBuf.maxExportSessions = maxExportSessions;

	if (bsoBECmd)
	{
		if (spanBuf.bsoBECmd)
		{
			sdr_free(bsspSdr, spanBuf.bsoBECmd);
		}

		spanBuf.bsoBECmd = sdr_string_create(bsspSdr, bsoBECmd);
	}
	
	if (bsoRLCmd)
	{
		if (spanBuf.bsoRLCmd)
		{
			sdr_free(bsspSdr, spanBuf.bsoRLCmd);
		}

		spanBuf.bsoRLCmd = sdr_string_create(bsspSdr, bsoRLCmd);
	}

	spanBuf.remoteQtime = qTime;
	spanBuf.purge = purge ? 1 : 0;
	if (maxBlockSize)
	{
		spanBuf.maxBlockSize = maxBlockSize;
	}

	sdr_write(bsspSdr, addr, (char *) &spanBuf, sizeof(BsspSpan));
	if (sdr_end_xn(bsspSdr) < 0)
	{
		putErrmsg("Can't update span.", itoa(engineId));
		return -1;
	}

	return 1;
}

int	removeBsspSpan(uvast engineId)
{
	Sdr		bsspSdr = getIonsdr();
	BsspVspan	*vspan;
	PsmAddress	vspanElt;
	Object		spanElt;
	Object		spanObj;
			OBJ_POINTER(BsspSpan, span);

	/*	Must stop the span before trying to remove it.		*/

	CHKERR(sdr_begin_xn(bsspSdr));	/*	Lock memory.		*/
	findBsspSpan(engineId, &vspan, &vspanElt);
	if (vspanElt == 0)	/*	This is an unknown span.	*/
	{
		sdr_exit_xn(bsspSdr);
		writeMemoNote("[?] Unknown span", itoa(engineId));
		return 0;
	}

	/*	All parameters validated.				*/

	stopSpan(vspan);
	sdr_exit_xn(bsspSdr);
	waitForSpan(vspan);
	CHKERR(sdr_begin_xn(bsspSdr));
	resetSpan(vspan);
	spanElt = vspan->spanElt;
	spanObj = (Object) sdr_list_data(bsspSdr, spanElt);
	GET_OBJ_POINTER(bsspSdr, BsspSpan, span, spanObj);
	if (sdr_list_length(bsspSdr, span->beBlocks) != 0)
	{
		sdr_exit_xn(bsspSdr);
		writeMemoNote("[?] Span has backlog, can't be removed",
				itoa(engineId));
		return 0;
	}

	if (sdr_list_length(bsspSdr, span->rlBlocks) != 0)
	{
		sdr_exit_xn(bsspSdr);
		writeMemoNote("[?] Span has backlog, can't be removed",
				itoa(engineId));
		return 0;
	}

	if (sdr_list_length(bsspSdr, span->exportSessions) != 0)
	{
		sdr_exit_xn(bsspSdr);
		writeMemoNote("[?] Span has open sessions, can't be removed",
				itoa(engineId));
		return 0;
	}

	/*	Okay to remove this span from the database.		*/

	dropSpan(vspan, vspanElt);
	if (span->bsoBECmd)
	{
		sdr_free(bsspSdr, span->bsoBECmd);
	}

	if (span->bsoRLCmd)
	{
		sdr_free(bsspSdr, span->bsoRLCmd);
	}

	sdr_list_destroy(bsspSdr, span->exportSessions, NULL, NULL);
	sdr_list_destroy(bsspSdr, span->beBlocks, NULL, NULL);
	sdr_list_destroy(bsspSdr, span->rlBlocks, NULL, NULL);
	sdr_free(bsspSdr, spanObj);
	sdr_list_delete(bsspSdr, spanElt, NULL, NULL);
	if (sdr_end_xn(bsspSdr) < 0)
	{
		putErrmsg("Can't remove span.", itoa(engineId));
		return -1;
	}

	return 1;
}

int	bsspStartSpan(uvast engineId)
{
	Sdr		bsspSdr = getIonsdr();
	BsspVspan	*vspan;
	PsmAddress	vspanElt;

	CHKERR(sdr_begin_xn(bsspSdr));	/*	Just to lock memory.	*/
	findBsspSpan(engineId, &vspan, &vspanElt);
	if (vspanElt == 0)
	{
		sdr_exit_xn(bsspSdr);	/*	Unlock memory.		*/
		writeMemoNote("[?] Unknown span", itoa(engineId));
		return 0;
	}

	startSpan(vspan);
	sdr_exit_xn(bsspSdr);	/*	Unlock memory.			*/
	return 1;
}

void	bsspStopSpan(uvast engineId)
{
	Sdr		bsspSdr = getIonsdr();
	BsspVspan	*vspan;
	PsmAddress	vspanElt;

	CHKVOID(sdr_begin_xn(bsspSdr));	/*	Just to lock memory.	*/
	findBsspSpan(engineId, &vspan, &vspanElt);
	if (vspanElt == 0)	/*	This is an unknown span.	*/
	{
		sdr_exit_xn(bsspSdr);	/*	Unlock memory.		*/
		writeMemoNote("[?] Unknown span", itoa(engineId));
		return;
	}

	stopSpan(vspan);
	sdr_exit_xn(bsspSdr);	/*	Unlock memory.			*/
	waitForSpan(vspan);
	CHKVOID(sdr_begin_xn(bsspSdr));	/*	Just to lock memory.	*/
	resetSpan(vspan);
	sdr_exit_xn(bsspSdr);	/*	Unlock memory.			*/
}

/*	*	*	BSSP event mgt and access functions	*	*/

static Object	insertBsspTimelineEvent(BsspEvent *newEvent)
{
	Sdr	bsspSdr = getIonsdr();
	BsspDB	*bsspConstants = _bsspConstants();
	Object	eventObj;
	Object	elt;
		OBJ_POINTER(BsspEvent, event);

	CHKZERO(ionLocked());
	eventObj = sdr_malloc(bsspSdr, sizeof(BsspEvent));
	if (eventObj == 0)
	{
		putErrmsg("No space for timeline event.", NULL);
		return 0;
	}

	/*	Search list from newest to oldest, insert after last
		event with scheduled time less than or equal to that
		of the new event.					*/

	sdr_write(bsspSdr, eventObj, (char *) newEvent, sizeof(BsspEvent));
	for (elt = sdr_list_last(bsspSdr, bsspConstants->timeline); elt;
			elt = sdr_list_prev(bsspSdr, elt))
	{
		GET_OBJ_POINTER(bsspSdr, BsspEvent, event, sdr_list_data(bsspSdr,
				elt));
		if (event->scheduledTime <= newEvent->scheduledTime)
		{
			return sdr_list_insert_after(bsspSdr, elt, eventObj);
		}
	}

	return sdr_list_insert_first(bsspSdr, bsspConstants->timeline, eventObj);
}

static void	cancelEvent(BsspEventType type, uvast refNbr1,
			unsigned int refNbr2)
{
	Sdr	bsspSdr = getIonsdr();
	Object	elt;
	Object	eventObj;
		OBJ_POINTER(BsspEvent, event);

	for (elt = sdr_list_first(bsspSdr, (_bsspConstants())->timeline); elt;
			elt = sdr_list_next(bsspSdr, elt))
	{
		eventObj = sdr_list_data(bsspSdr, elt);
		GET_OBJ_POINTER(bsspSdr, BsspEvent, event, eventObj);
		if (event->type == type && event->refNbr1 == refNbr1
		&& event->refNbr2 == refNbr2)
		{
			sdr_free(bsspSdr, eventObj);
			sdr_list_delete(bsspSdr, elt, NULL, NULL);
			return;
		}
	}
}

/*	*	*	BSSP client mgt and access functions	*	*/

int	bsspAttachClient(unsigned int clientSvcId)
{
	Sdr		bsspSdr = getIonsdr();
	BsspVclient	*client;

	if (clientSvcId > MAX_BSSP_CLIENT_NBR)
	{
		putErrmsg("Client svc number over limit.", itoa(clientSvcId));
		return -1;
	}

	CHKERR(sdr_begin_xn(bsspSdr));	/*	Just to lock memory.	*/
	client = (_bsspvdb(NULL))->clients + clientSvcId;
	if (client->pid != ERROR)
	{
		if (sm_TaskExists(client->pid))
		{
			sdr_exit_xn(bsspSdr);
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
	sdr_exit_xn(bsspSdr);	/*	Unlock memory.			*/
	return 0;
}

void	bsspDetachClient(unsigned int clientSvcId)
{
	Sdr		bsspSdr = getIonsdr();
	BsspVclient	*client;

	if (clientSvcId > MAX_BSSP_CLIENT_NBR)
	{
		return;
	}

	CHKVOID(sdr_begin_xn(bsspSdr));	/*	Just to lock memory.	*/
	client = (_bsspvdb(NULL))->clients + clientSvcId;
	if (client->pid != sm_TaskIdSelf())
	{
		sdr_exit_xn(bsspSdr);
		putErrmsg("Can't close: not owner of client service.", NULL);
		return;
	}

	client->pid = -1;
	sdr_exit_xn(bsspSdr);	/*	Unlock memory.			*/
}

/*	*	*	Service interface functions	*	*	*/

int	enqueueBsspNotice(BsspVclient *client, uvast sourceEngineId,
		unsigned int sessionNbr, unsigned int dataLength, 
		BsspNoticeType type, unsigned char reasonCode,
		Object data)
{
	Sdr		bsspSdr = getIonsdr();
	Object		noticeObj;
	BsspNotice	notice;

	CHKERR(client);
	if (client->pid == ERROR)
	{
		return 0;	/*	No client task to report to.	*/
	}

	CHKERR(ionLocked());
	noticeObj = sdr_malloc(bsspSdr, sizeof(BsspNotice));
	if (noticeObj == 0)
	{
		return -1;
	}

	if (sdr_list_insert_last(bsspSdr, client->notices, noticeObj) == 0)
	{
		return -1;
	}

	notice.sessionId.sourceEngineId = sourceEngineId;
	notice.sessionId.sessionNbr = sessionNbr;
	notice.dataLength = dataLength;
	notice.type = type;
	notice.reasonCode = reasonCode;
	notice.data = data;
	sdr_write(bsspSdr, noticeObj, (char *) &notice, sizeof(BsspNotice));

	/*	Tell client that a notice is waiting.			*/

	sm_SemGive(client->semaphore);
	return 0;
}

/*	*	*	Session management functions	*	*	*/

static void	getExportSession(unsigned int sessionNbr, Object *sessionObj)
{
	Sdr	bsspSdr = getIonsdr();
	Object	elt;

	CHKVOID(ionLocked());
	if (sdr_hash_retrieve(bsspSdr, (_bsspConstants())->exportSessionsHash,
			(char *) &sessionNbr, (Address *) &elt, NULL) == 1)
	{
		*sessionObj = sdr_list_data(bsspSdr, elt);
		return; 
	}

	/*	Unknown session.					*/

	*sessionObj = 0;
}

static void	destroyDataXmitBlk(Object blockObj, BsspXmitBlock *blk)
{
	Sdr	bsspSdr = getIonsdr();

	CHKVOID(ionLocked());

	cancelEvent(BsspResendBlock, 0, blk->sessionNbr);

	if (blk->queueListElt)	/*	Queued for retransmission.	*/
	{
		sdr_list_delete(bsspSdr, blk->queueListElt, NULL, NULL);
	}

	sdr_free(bsspSdr, blockObj);
}

static void	stopExportSession(ExportSession *session)
{
	Sdr	bsspSdr = getIonsdr();
	Object	blkObj;
		OBJ_POINTER(BsspXmitBlock, blk);

	CHKVOID(ionLocked());
	blkObj = session->block;
	GET_OBJ_POINTER(bsspSdr, BsspXmitBlock, blk, blkObj);
	destroyDataXmitBlk(blkObj, blk);
}

static void	closeExportSession(Object sessionObj)
{
	Sdr		bsspSdr = getIonsdr();
	BsspVdb		*bsspvdb = _bsspvdb(NULL);
	Object		dbobj = getBsspDbObject();
			OBJ_POINTER(ExportSession, session);
			OBJ_POINTER(BsspSpan, span);
	BsspVspan	*vspan;
	PsmAddress	vspanElt;
	BsspDB		db;
	Object		elt;

	CHKVOID(ionLocked());
	GET_OBJ_POINTER(bsspSdr, ExportSession, session, sessionObj);
	GET_OBJ_POINTER(bsspSdr, BsspSpan, span, session->span);
	findBsspSpan(span->engineId, &vspan, &vspanElt);
	sdr_stage(bsspSdr, (char *) &db, dbobj, sizeof(BsspDB));

	/*	Note that cancellation of an export session causes
	 *	the block's service data object to be passed up to
	 *	the user in BsspXmitFailure notice, destroys the
	 *	svcDataObject, and sets the svcDataObject to zero.
	 *  	In that event, review of the service data object 
	 *	in this function is foregone.					
	 */

	if (session->svcDataObject)
	{
		if (enqueueBsspNotice(bsspvdb->clients
				+ session->clientSvcId, db.ownEngineId,
				session->sessionNbr, 0, 
				BsspXmitSuccess, 0, session->svcDataObject)
				< 0)
		{
			putErrmsg("Can't post ExportSessionComplete notice.",
					NULL);
			sdr_cancel_xn(bsspSdr);
			return;
		}
	}

	sdr_write(bsspSdr, dbobj, (char *) &db, sizeof(BsspDB));
	
	session->block = 0;

	/*	Finally erase the session itself, reducing the session
	 *	list length and thereby possibly enabling a blocked
	 *	client to append an SDU to the current block.		*/

	sdr_hash_remove(bsspSdr, db.exportSessionsHash,
			(char *) &(session->sessionNbr), (Address *) &elt);
	sdr_list_delete(bsspSdr, elt, NULL, NULL);
	sdr_free(bsspSdr, sessionObj);
#if BSSPDEBUG
putErrmsg("Closed export session.", itoa(session->sessionNbr));
#endif
	if (vspanElt == 0)
	{
		putErrmsg("Can't find vspan for engine.", utoa(span->engineId));
	}
	else
	{
		sm_SemGive(vspan->bufOpenSemaphore);
	}
}

/*	*	*	Segment issuance functions	*	*	*/

static void	serializeHeader(BsspXmitBlock *block, Sdnv *engineIdSdnv,
			char **cursor)
{
	char	firstByte = BSSP_VERSION;
	Sdnv	sessionNbrSdnv;

	firstByte <<= 1;
	firstByte += block->pdu.blkTypeCode;
	**cursor = firstByte;
	(*cursor)++;

	memcpy((*cursor), engineIdSdnv->text, engineIdSdnv->length);
	(*cursor) += engineIdSdnv->length;

	encodeSdnv(&sessionNbrSdnv, block->sessionNbr);
	memcpy((*cursor), sessionNbrSdnv.text, sessionNbrSdnv.length);
	(*cursor) += sessionNbrSdnv.length;

	**cursor = 0;		/*	Both extension counts = 0.	*/
	(*cursor)++;
}

static void	serializeDataPDU(BsspXmitBlock *block, char *buf)
{
	char	*cursor = buf;
	Sdnv	sdnv;

	/*	Origin is the local engine.				*/

	serializeHeader(block, &((_bsspConstants())->ownEngineIdSdnv),
			&cursor);

	/*	Append client service number.				*/

	encodeSdnv(&sdnv, block->pdu.clientSvcId);
	memcpy(cursor, sdnv.text, sdnv.length);
	cursor += sdnv.length;


	/*	Append length of data.					*/

	encodeSdnv(&sdnv, block->pdu.length);
	memcpy(cursor, sdnv.text, sdnv.length);
	cursor += sdnv.length;
	

	/*	Note: client service data was copied into the trailing
	 *	bytes of the buffer before this function was called.	*/
}

static void	serializeAck(BsspXmitBlock *block, char *buf)
{
	char	*cursor = buf;
	Sdnv	sdnv;

	/*	Report is from local engine, so origin is the remote
	 *	engine.							*/

	encodeSdnv(&sdnv, block->remoteEngineId);
	serializeHeader(block, &sdnv, &cursor);
}

static int	setTimer(BsspTimer *timer, Address timerAddr, time_t currentSec,
			BsspVspan *vspan, int blockLength, BsspEvent *event)
{
	Sdr	bsspSdr = getIonsdr();
	BsspDB	BsspDB;
	int	radTime;
		OBJ_POINTER(BsspSpan, span);

	CHKERR(ionLocked());
	sdr_read(bsspSdr, (char *) &BsspDB, getBsspDbObject(), sizeof(BsspDB));
	if (vspan->localXmitRate == 0)	/*	Should never be, but...	*/
	{
		radTime = 0;		/*	Avoid divide by zero.	*/
	}
	else
	{
		radTime = (blockLength + EST_LINK_OHD) / vspan->localXmitRate;
	}

	/*	Block should arrive at the remote node following
	 *	about half of the local node's telecom processing
	 *	turnaround time (ownQtime) plus the time consumed in
	 *	simply radiating all the bytes of the block
	 *	(including estimated link-layer overhead) at the
	 *	current transmission rate over this span, plus
	 *	the current outbound signal propagation time (owlt).	*/

	timer->pduArrivalTime = currentSec + radTime + vspan->owltOutbound
			+ ((BsspDB.ownQtime >> 1) & 0x7fffffff);
	GET_OBJ_POINTER(bsspSdr, BsspSpan, span, sdr_list_data(bsspSdr,
			vspan->spanElt));

	/*	Following arrival of the block, the response from
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

	timer->ackDeadline = timer->pduArrivalTime
			+ span->remoteQtime + vspan->owltInbound
			+ ((BsspDB.ownQtime >> 1) & 0x7fffffff);
	if (vspan->remoteXmitRate > 0)
	{
		event->scheduledTime = timer->ackDeadline;
		if (insertBsspTimelineEvent(event) == 0)
		{
			putErrmsg("Can't set timer.", NULL);
			return -1;
		}

		timer->state = BsspTimerRunning;
	}
	else
	{
		timer->state = BsspTimerSuspended;
	}

	sdr_write(bsspSdr, timerAddr, (char *) timer, sizeof(BsspTimer));
	return 0;
}

static int	readFromExportBlock(char *buffer, Object svcDataObject,
			unsigned int length)
{
	Sdr		bsspSdr = getIonsdr();
	unsigned int	svcDataLength;
	int		totalBytesRead = 0;
	ZcoReader	reader;
	unsigned int	bytesToRead;
	int		bytesRead;

	svcDataLength = zco_length(bsspSdr, svcDataObject);

	zco_start_transmitting(svcDataObject, &reader);

	bytesToRead = length;
	if (bytesToRead > svcDataLength)
	{
		bytesToRead = svcDataLength;
	}

	bytesRead = zco_transmit(bsspSdr, &reader, bytesToRead,
			buffer + totalBytesRead);
	if (bytesRead != bytesToRead)
	{
		putErrmsg("Failed reading SDU.", NULL);
		return -1;
	}

	totalBytesRead += bytesRead;
	return totalBytesRead;
}

int	bsspDequeueBEOutboundBlock(BsspVspan *vspan, char **buf)
{
	Sdr		bsspSdr = getIonsdr();
	BsspVdb		*bsspvdb = _bsspvdb(NULL);
	Object		spanObj;
	BsspSpan	spanBuf;
	Object		elt;
	char		memo[64];
	Object		blkAddr;
	BsspXmitBlock	block;
	int		blockLength;
	time_t		currentTime;
	BsspEvent	event;
	BsspTimer	*timer;

	CHKERR(vspan);
	CHKERR(buf);
	*buf = (char *) psp(getIonwm(), vspan->beBuffer);
	CHKERR(sdr_begin_xn(bsspSdr));
	spanObj = sdr_list_data(bsspSdr, vspan->spanElt);
	sdr_stage(bsspSdr, (char *) &spanBuf, spanObj, sizeof(BsspSpan));

	elt = sdr_list_first(bsspSdr, spanBuf.beBlocks);
	while (elt == 0 || vspan->localXmitRate == 0)
	{
		sdr_exit_xn(bsspSdr);

		/*	Wait until bssp_send has announced an outbound
		 *	block by giving span's beSemaphore.		*/

		if (sm_SemTake(vspan->beSemaphore) < 0)
		{
			putErrmsg("BSO can't take best-effort semaphore.",
					itoa(vspan->engineId));
			return -1;
		}

		if (sm_SemEnded(vspan->beSemaphore))
		{
			isprintf(memo, sizeof memo,
			"[i] BSO best-effort channel to engine " \
UVAST_FIELDSPEC " is stopped.", vspan->engineId);
			writeMemo(memo);
			return 0;
		}

		CHKERR(sdr_begin_xn(bsspSdr));
		sdr_stage(bsspSdr, (char *) &spanBuf, spanObj,
				sizeof(BsspSpan));
		elt = sdr_list_first(bsspSdr, spanBuf.beBlocks);
	}

	/*	Got next outbound block.  Remove it from the queue
	 *	for this span.						*/

	blkAddr = sdr_list_data(bsspSdr, elt);
	sdr_stage(bsspSdr, (char *) &block, blkAddr, sizeof(BsspXmitBlock));
	sdr_list_delete(bsspSdr, elt, NULL, NULL);
	block.queueListElt = 0;

	/*	Copy blocks's content into buffer.			*/

	blockLength = block.ohdLength;
	if (block.pduClass == BsspData)
	{
		blockLength += block.pdu.length;

		/*	Load client service data at the end of the
		 *	block first, before filling in the header.	*/

		if (readFromExportBlock((*buf) + block.ohdLength,
				block.pdu.svcData, block.pdu.length) < 0)
		{
			putErrmsg("Can't read data from export block.", NULL);
			sdr_cancel_xn(bsspSdr);
			return -1;
		}
	}

	/*	Now serialize the block overhead and prepend that
	 *	overhead to the content of the segment (if any).
	 *
	 *	If sending data, the block must be retained (it may
	 *	need to be re-sent using the reliable transport
	 *	service), so xmit session is still needed.  If
	 *	sending an ACK, the block is no longer needed and
	 *	the recv session is closed elsewhere.  So no
	 *	session closure at this point.				*/

	if (block.pdu.blkTypeCode == 0)		/*	Data block.	*/
	{
		serializeDataPDU(&block, *buf);

		/*	Need to retain the block in case it needs
		 *	to be re-sent using the reliable transport
		 *	service.  So must rewrite block to record
		 *	change: queueListElt is now 0.			*/

		sdr_write(bsspSdr, blkAddr, (char *) &block,
				sizeof(BsspXmitBlock));

		/*	Post timeout event.				*/

		currentTime = getCtime();

//	Temporary patch to prevent premature retransmission.
currentTime += 5;	/*	s/b += RTT from contact plan.	*/

		event.parm = 0;
		event.type = BsspResendBlock;
		event.refNbr1 = 0;
		event.refNbr2 = block.sessionNbr;
		timer = &block.pdu.timer;
		if (setTimer(timer, blkAddr + FLD_OFFSET(timer, &block),
			currentTime, vspan, blockLength, &event) < 0)
		{
			putErrmsg("Can't schedule event.", NULL);
			sdr_cancel_xn(bsspSdr);
			return -1;
		}
	}
	else					/*	ACK block.	*/
	{
		serializeAck(&block, *buf);

		/*	Block will never be re-sent, so it can be
		 *	deleted now.					*/

		sdr_free(bsspSdr, blkAddr);
	}

	if (sdr_end_xn(bsspSdr))
	{
		putErrmsg("Can't get best effort outbound block for span.",
				NULL);
		return -1;
	}

	if (bsspvdb->watching & WATCH_g)
	{
		putchar('g');
		fflush(stdout);
	}

	return blockLength;
}

int	bsspDequeueRLOutboundBlock(BsspVspan *vspan, char **buf)
{
	Sdr		bsspSdr = getIonsdr();
	BsspVdb		*bsspvdb = _bsspvdb(NULL);
	Object		spanObj;
	BsspSpan	spanBuf;
	Object		elt;
	char		memo[64];
	Object		blkAddr;
	BsspXmitBlock	block;
	int		blockLength;
	Object		sessionObj;
	ExportSession	sessionBuf;
	Object		spanObj2 = 0;
	BsspSpan	spanBuf2;
	BsspVspan	*vspan2;
	PsmAddress	vspanElt2;

	CHKERR(vspan);
	CHKERR(buf);
	*buf = (char *) psp(getIonwm(), vspan->rlBuffer);
	CHKERR(sdr_begin_xn(bsspSdr));
	spanObj = sdr_list_data(bsspSdr, vspan->spanElt);
	sdr_stage(bsspSdr, (char *) &spanBuf, spanObj, sizeof(BsspSpan));

	elt = sdr_list_first(bsspSdr, spanBuf.rlBlocks);
	while (elt == 0 || vspan->localXmitRate == 0)
	{
		sdr_exit_xn(bsspSdr);

		/*	Wait until bssp_send has announced an outbound
		 *	segment by giving span's segSemaphore.		*/

		if (sm_SemTake(vspan->rlSemaphore) < 0)
		{
			putErrmsg("BSO can't take reliable block semaphore.",
					itoa(vspan->engineId));
			return -1;
		}

		if (sm_SemEnded(vspan->rlSemaphore))
		{
			isprintf(memo, sizeof memo,
			"[i] BSO real-time channel to engine " UVAST_FIELDSPEC \
" is stopped.", vspan->engineId);
			writeMemo(memo);
			return 0;
		}

		CHKERR(sdr_begin_xn(bsspSdr));
		sdr_stage(bsspSdr, (char *) &spanBuf, spanObj,
				sizeof(BsspSpan));
		elt = sdr_list_first(bsspSdr, spanBuf.rlBlocks);
	}

	/*	Got next outbound segment.  Remove it from the queue
	 *	for this span.						*/

	blkAddr = sdr_list_data(bsspSdr, elt);
	sdr_stage(bsspSdr, (char *) &block, blkAddr, sizeof(BsspXmitBlock));
	sdr_list_delete(bsspSdr, elt, NULL, NULL);
	block.queueListElt = 0;

	/*	Copy blocks's content into buffer.			*/

	blockLength = block.ohdLength;
	if (block.pduClass == BsspData)
	{
		blockLength += block.pdu.length;

		/*	Load client service data at the end of the
		 *	block first, before filling in the header.	*/

		if (readFromExportBlock((*buf) + block.ohdLength,
				block.pdu.svcData, block.pdu.length) < 0)
		{
			putErrmsg("Can't read data from export block.", NULL);
			sdr_cancel_xn(bsspSdr);
			return -1;
		}
	}

	/*	Now serialize the segment overhead and prepend that
	 *	overhead to the content of the block.			*/

	serializeDataPDU(&block, *buf);

	/*	No longer need the xmit session at all, as block
	 *	content is being transmitted by reliable transport
	 *	service.  Rewrite the block first to record the
	 *	change of queueListElt to 0.				*/

	sdr_write(bsspSdr, blkAddr, (char *) &block, sizeof(BsspXmitBlock));
	getSessionContext(getBsspConstants(), block.sessionNbr, &sessionObj,
			&sessionBuf, &spanObj2, &spanBuf2, &vspan2, &vspanElt2);
	if (sessionObj)
	{
		stopExportSession(&sessionBuf);
		closeExportSession(sessionObj);
	}

	if (sdr_end_xn(bsspSdr))
	{
		putErrmsg("Can't get outbound block for reliable span.",
				NULL);
		return -1;
	}

	if (bsspvdb->watching & WATCH_t)
	{
		putchar('t');
		fflush(stdout);
	}

	return blockLength;
}

/*	*	Control segment construction functions		*	*/

static void	signalBeBso(unsigned int engineId)
{
	BsspVspan	*vspan;
	PsmAddress	vspanElt;

	findBsspSpan(engineId, &vspan, &vspanElt);
	if (vspan != NULL && vspan->localXmitRate > 0)
	{
		/*	Tell Best-Effort BSO that output is waiting.	*/

		sm_SemGive(vspan->beSemaphore);
	}
}

static void	signalRlBso(unsigned int engineId)
{
	BsspVspan	*vspan;
	PsmAddress	vspanElt;

	findBsspSpan(engineId, &vspan, &vspanElt);
	if (vspan != NULL && vspan->localXmitRate > 0)
	{
		/*	Tell Reliable BSO that output is waiting.	*/

		sm_SemGive(vspan->rlSemaphore);
	}
}

static int	cancelSessionBySender(ExportSession *session,
			Object sessionObj, BsspCancelReasonCode reasonCode)
{
	Sdr		bsspSdr = getIonsdr();
	BsspVdb		*bsspvdb = _bsspvdb(NULL);
	Object		dbobj = getBsspDbObject();
	BsspDB		db;
	Object		spanObj = session->span;
	BsspSpan		span;
	BsspVspan	*vspan;
	PsmAddress	vspanElt;
	Object		elt;

	CHKERR(ionLocked());
	session->reasonCode = reasonCode;	/*	(For CS resend.)*/
	sdr_stage(bsspSdr, (char *) &span, spanObj, sizeof(BsspSpan));
	findBsspSpan(span.engineId, &vspan, &vspanElt);
	if (vspanElt == 0)
	{
		putErrmsg("Can't find vspan for engine.", utoa(span.engineId));
		return -1;
	}

	if (sessionObj == span.currentExportSessionObj)
	{
		/*	Finish up session so it can be reported.	*/

		session->clientSvcId = span.clientSvcIdOfBufferedBlock;
		encodeSdnv(&(session->clientSvcIdSdnv), session->clientSvcId);
		session->totalLength = span.lengthOfBufferedBlock;
	}

	if (bsspvdb->watching & WATCH_CS)
	{
		putchar('{');
		fflush(stdout);
	}

	sdr_stage(bsspSdr, (char *) &db, dbobj, sizeof(BsspDB));
	stopExportSession(session);

	if (enqueueBsspNotice(bsspvdb->clients + session->clientSvcId,
		db.ownEngineId, session->sessionNbr, 0,
		BsspXmitFailure, reasonCode, session->svcDataObject) < 0)
	{
		putErrmsg("Can't post ExportSessionCanceled notice.",
				NULL);
		return -1;
	}

	sdr_write(bsspSdr, dbobj, (char *) &db, sizeof(BsspDB));

	session->svcDataObject = 0;
	sdr_write(bsspSdr, sessionObj, (char *) session, sizeof(ExportSession));

	/*	Remove session from active sessions pool, so that the
	 *	cancellation won't affect flow control.			*/

	sdr_hash_remove(bsspSdr, db.exportSessionsHash,
			(char *) &(session->sessionNbr), (Address *) &elt);
	sdr_list_delete(bsspSdr, elt, NULL, NULL);

	/*	Span now has room for another session to start.		*/

	if (sessionObj == span.currentExportSessionObj)
	{
		/*	Reinitialize span's block buffer.		*/

		span.lengthOfBufferedBlock = 0;
		span.clientSvcIdOfBufferedBlock = 0;
		span.currentExportSessionObj = 0;
		sdr_write(bsspSdr, spanObj, (char *) &span, sizeof(BsspSpan));

		/*	Re-start the current export session.		*/

		if (startBsspExportSession(bsspSdr, spanObj, vspan) < 0)
		{
			putErrmsg("Can't re-start the current session.",
					utoa(span.engineId));
			return -1;
		}
	}
	else
	{
		/*	The canceled session isn't the current
		 *	session, but cancelling this session
		 *	reduced the session list length, possibly
		 *	enabling a blocked client to append an SDU
		 *	to the current block.				*/

		sm_SemGive(vspan->bufOpenSemaphore);
	}


	return 0;
}

static Object	enqueueAckBlock(Object spanObj, Object blockObj)
{
	Sdr	bsspSdr = getIonsdr();
		OBJ_POINTER(BsspSpan, span);
	Object	elt;
		OBJ_POINTER(BsspXmitBlock, block);

	CHKZERO(ionLocked());
	GET_OBJ_POINTER(bsspSdr, BsspSpan, span, spanObj);
	for (elt = sdr_list_first(bsspSdr, span->beBlocks); elt;
			elt = sdr_list_next(bsspSdr, elt))
	{
		GET_OBJ_POINTER(bsspSdr, BsspXmitBlock, block,
				sdr_list_data(bsspSdr, elt));
		switch (block->pdu.blkTypeCode)
		{
		case BsspAck:
			continue;

		default:	/*	Found first non-ACK block.	*/
			break;			/*	Out of switch.	*/
		}

		break;				/*	Out of loop.	*/
	}

	if (elt)
	{
		elt = sdr_list_insert_before(bsspSdr, elt, blockObj);
	}
	else
	{
		elt = sdr_list_insert_last(bsspSdr, span->beBlocks, blockObj);
	}

	return elt;
}

static int	constructAck(BsspXmitBlock *rs, Object spanObj)
{
	Sdr	bsspSdr = getIonsdr();
	Object	rsObj;
		OBJ_POINTER(BsspSpan, span);

	CHKERR(ionLocked());
	rsObj = sdr_malloc(bsspSdr, sizeof(BsspXmitBlock));
	if (rsObj == 0)
	{
		return -1;
	}

	rs->queueListElt = enqueueAckBlock(spanObj, rsObj);
	if (rs->queueListElt == 0)
	{
		return -1;
	}

	sdr_write(bsspSdr, rsObj, (char *) rs, sizeof(BsspXmitBlock)); 
	GET_OBJ_POINTER(bsspSdr, BsspSpan, span, spanObj);
	signalBeBso(span->engineId);
#if BSSPDEBUG
putErrmsg("Sending Ack.", NULL);
#endif
	return 0;
}

static int	sendAck(unsigned int sessionNbr, Object spanObj)
{
	Sdr		bsspSdr = getIonsdr();
			OBJ_POINTER(BsspSpan, span);
	int		baseOhdLength;
	BsspXmitBlock	rsBuf;
	Sdnv		sessionNbrSdnv;

	CHKERR(ionLocked());
	CHKERR(spanObj);
	
	GET_OBJ_POINTER(bsspSdr, BsspSpan, span, spanObj);
	encodeSdnv(&sessionNbrSdnv, sessionNbr);
	baseOhdLength = 1 + span->engineIdSdnv.length 
			+ sessionNbrSdnv.length + 1;

	/*	Set the acknowledgement values.		*/

	memset((char *) &rsBuf, 0, sizeof(BsspXmitBlock));
	rsBuf.sessionNbr = sessionNbr;
	rsBuf.remoteEngineId = span->engineId;
	rsBuf.pduClass = BsspAckn;
	rsBuf.pdu.blkTypeCode = BsspAck;
	rsBuf.ohdLength = baseOhdLength;
	
	/*		Ship this Ack.			*/

	if (constructAck(&rsBuf, spanObj) < 0)
	{
		return -1;
	}

	return 0;
}

/*	*	*	Segment handling functions	*	*	*/

static int	handleDataBlock(uvast sourceEngineId, BsspDB *bsspdb,
			unsigned int sessionNbr, BsspRecvBlk *block,
			BsspPdu *pdu, char **cursor, int *bytesRemaining)
{
	Sdr		bsspSdr = getIonsdr();
	BsspVdb		*bsspvdb = _bsspvdb(NULL);
	BsspVspan	*vspan;
	PsmAddress	vspanElt;
	Object		spanObj;
	BsspVclient	*client;
	ReqTicket	ticket;
	Object		pduObj;
	vast		pduLength = 0;
	Object		clientSvcData = 0;

	/*	First finish parsing the block.			*/

	extractSmallSdnv(&(pdu->clientSvcId), cursor, bytesRemaining);
	extractSmallSdnv(&(pdu->length), cursor, bytesRemaining);

	/*	At this point, the remaining bytes should all be
	 *	client service data.					*/

	CHKERR(sdr_begin_xn(bsspSdr));
	findBsspSpan(sourceEngineId, &vspan, &vspanElt);
	if (vspanElt == 0)
	{
#if BSSPDEBUG
putErrmsg("Discarded data block.", itoa(sessionNbr));
#endif
		/*	Block is from an unknown engine, so we
		 *	can't process it.				*/

		return sdr_end_xn(bsspSdr);
	}

	if (pdu->length > *bytesRemaining)
	{
#if BSSPDEBUG
putErrmsg("Discarded data block.", itoa(sessionNbr));
#endif
		/*	Malformed block: data length is overstated.
		 *	Block must be discarded.			*/
		return sdr_end_xn(bsspSdr);
	}

	/*	Now process the data.					*/

	spanObj = sdr_list_data(bsspSdr, vspan->spanElt);
	block->pduClass = BsspData;
	if (pdu->clientSvcId > MAX_BSSP_CLIENT_NBR
	|| (client = bsspvdb->clients + pdu->clientSvcId)->pid == ERROR)
	{
		/*	Data block is for an unknown client service,	*
		 *	so must discard it.				*/

		if (sdr_end_xn(bsspSdr) < 0)
		{
			putErrmsg("Can't handle data Block.", NULL);
			return -1;
		}
#if BSSPDEBUG
putErrmsg("Discarded data block.", itoa(sessionNbr));
#endif
		return 0;
	}

	/*	Deliver the client service data.			*/

	if (ionRequestZcoSpace(ZcoInbound, 0, 0, pdu->length, 0, 0, NULL,
			&ticket) < 0)
	{
		putErrmsg("Failed on ionRequest.", NULL);
		sdr_cancel_xn(bsspSdr);
		return -1;
	}

	if (!(ionSpaceAwarded(ticket)))
	{
		/*	Couldn't service request immediately.	*/

		ionShred(ticket);		/*	Cancel request.	*/
#if BSSPDEBUG
putErrmsg("Can't handle data block, would exceed available ZCO space.",
utoa(pdu->length));
#endif
		return sdr_end_xn(bsspSdr);
	}

	/*	ZCO space has been awarded.				*/

	pduObj = sdr_insert(bsspSdr, *cursor, pdu->length);
	if (pduObj == 0)
	{
		putErrmsg("Can't record data block.", NULL);
		sdr_cancel_xn(bsspSdr);
		ionShred(ticket);		/*	Cancel request.	*/
		return -1;
	}

	/*	Pass additive inverse of length to zco_create to
 	*	indicate that space has already been awarded.		*/

	pduLength -= pdu->length;
	clientSvcData = zco_create(bsspSdr, ZcoSdrSource, pduObj, 0, pduLength,
			ZcoInbound);
	switch (clientSvcData)
	{
	case (Object) ERROR:
		putErrmsg("Can't record data block.", NULL);
		sdr_cancel_xn(bsspSdr);
		ionShred(ticket);		/*	Cancel request.	*/
		return -1;

	case 0:	/*	No ZCO space.  Silently discard block.	*/
#if BSSPDEBUG
putErrmsg("Can't handle data block, would exceed available ZCO space.",
utoa(pdu->length));
#endif
		ionShred(ticket);		/*	Cancel request.	*/
		return sdr_end_xn(bsspSdr);
	}

	if (enqueueBsspNotice(client, sourceEngineId, sessionNbr, pdu->length,
			BsspRecvSuccess, 0, clientSvcData) < 0)
	{
		putErrmsg("Can't enqueue notice to bssp client.", NULL);
		ionShred(ticket);		/*	Cancel request.	*/
		return sdr_end_xn(bsspSdr);
	}

	if (sendAck(sessionNbr, spanObj) < 0)
	{
		putErrmsg("Can't send reception acknowledgement.", NULL);
		sdr_cancel_xn(bsspSdr);
		ionShred(ticket);		/*	Cancel request.	*/
		return -1;
	}

	if (sdr_end_xn(bsspSdr) < 0)
	{
		putErrmsg("Can't handle data block.", NULL);
		ionShred(ticket);		/*	Cancel request.	*/
		return -1;
	}

	ionShred(ticket);		/*	Dismiss reservation.	*/
	return 0;	/*	 Data block handled okay.		*/
}

static int	constructDataBlock(Sdr sdr, ExportSession *session,
			Object sessionObj, BsspVspan *vspan, 
			BsspSpan *span, int inOrder)
{
	Sdr		bsspSdr = getIonsdr();
	Object		blockObj;
	BsspXmitBlock	block;
	int		length;
	int		dataBlockOverhead;
	int		worstCaseDataPDUSize;
	Sdnv		lengthSdnv;

#if BSSPDEBUG
char		buf[256];
#endif

	blockObj = sdr_malloc(bsspSdr, sizeof(BsspXmitBlock));
	if (blockObj == 0)
	{
		return -1;
	}

	memset((char *) &block, 0, sizeof(BsspXmitBlock));
	if (inOrder)
	{
		block.queueListElt = sdr_list_insert_last(bsspSdr,
				span->beBlocks, blockObj);
	}
	else
	{
		block.queueListElt = sdr_list_insert_last(bsspSdr, 
				span->rlBlocks, blockObj);
	}

	if (block.queueListElt == 0)
	{
		return -1;
	}

	/*	Compute length of segment's known overhead.		*/

	block.ohdLength = 1 + (_bsspConstants())->ownEngineIdSdnv.length
			+ session->sessionNbrSdnv.length + 1;
	block.ohdLength += session->clientSvcIdSdnv.length;

	/*	Determine length of block. 	*/

	/*	Compute worst-case block size.		*/
	
	length = session->totalLength;	
	encodeSdnv(&lengthSdnv, length);
	dataBlockOverhead = block.ohdLength + lengthSdnv.length;
	worstCaseDataPDUSize = length + dataBlockOverhead;
	if (worstCaseDataPDUSize > span->maxBlockSize)
	{
		putErrmsg("Bssp XmitDataBlock size exceeds maximum block size.",
		 NULL);
		return -1;
	}

	/*	Now have enough information to finish the segment.	*/

	session->block = blockObj;

	block.sessionNbr = session->sessionNbr;
	block.remoteEngineId = span->engineId;
	block.sessionObj = sessionObj;
	block.pduClass = BsspData;
	block.pdu.blkTypeCode = 0;
	block.pdu.clientSvcId = session->clientSvcId;
	block.pdu.length = length;
	encodeSdnv(&lengthSdnv, block.pdu.length);
	block.ohdLength += lengthSdnv.length;
	block.pdu.svcData = session->svcDataObject;

	sdr_write(bsspSdr, blockObj, (char *) &block, sizeof(BsspXmitBlock));

	if (inOrder)
	{
		signalBeBso(span->engineId);
	}
	else
	{
		signalRlBso(span->engineId);
	}

#if BSSPDEBUG
sprintf(buf, "Sent data block: session %u blkTypeCode %d length %d.",
session->sessionNbr, block.pdu.blkTypeCode, length);
putErrmsg(buf, itoa(session->sessionNbr));
#endif

	if ((_bsspvdb(NULL))->watching & WATCH_e)
	{
		putchar('e');
		fflush(stdout);
	}

	return 0;
}

int	issueXmitBlock(Sdr sdr, BsspSpan *span, BsspVspan *vspan,
			ExportSession *session, Object sessionObj, int inOrder)
{
	Sdr	bsspSdr = getIonsdr();

	CHKERR(session);
	if (session->svcDataObject == 0)	/*	Canceled.	*/
	{
		return 0;
	}

	CHKERR(ionLocked());
	CHKERR(span);
	CHKERR(vspan);
	
	if (constructDataBlock(bsspSdr, session, sessionObj, 
			vspan, span, inOrder) < 0)
	{
		putErrmsg("Can't construct data xmit block.", NULL);
		return -1;
	}
		
	/*	Block processing succeeded			*/

	return 1;
}

static void	getSessionContext(BsspDB *BsspDB, unsigned int sessionNbr,
			Object *sessionObj, ExportSession *sessionBuf,
			Object *spanObj, BsspSpan *spanBuf, BsspVspan **vspan,
			PsmAddress *vspanElt)
{
	Sdr	bsspSdr = getIonsdr();

	CHKVOID(ionLocked());
	*spanObj = 0;		/*	Default: no context.		*/
	getExportSession(sessionNbr, sessionObj);
	if (*sessionObj != 0)	/*	Known session.			*/
	{
		sdr_stage(bsspSdr, (char *) sessionBuf, *sessionObj,
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

	sdr_read(bsspSdr, (char *) spanBuf, *spanObj, sizeof(BsspSpan));
	findBsspSpan(spanBuf->engineId, vspan, vspanElt);
	if (*vspanElt == 0)
	{
#if BSSPDEBUG
putErrmsg("Discarding stray block.", itoa(sessionNbr));
#endif
		/*	Block is not from a known engine, so we 	*
		 *	treat it as a misdirected transmission.		*/

		*spanObj = 0;		/*	Disable acknowledgment.	*/
	}
}

static int	handleAck(BsspDB *BsspDB, unsigned int sessionNbr,
			BsspRecvBlk *block, BsspPdu *pdu, char **cursor,
			int *bytesRemaining)
{
	Sdr			bsspSdr = getIonsdr();
	BsspVdb			*bsspvdb = _bsspvdb(NULL);
	Object			sessionObj;
	ExportSession		sessionBuf;
	Object			spanObj = 0;
	BsspSpan		spanBuf;
	BsspVspan		*vspan;
	PsmAddress		vspanElt;
	BsspXmitBlock		dsBuf;
#if BSSPDEBUG
putErrmsg("Handling acknowledgment.", utoa(sessionNbr));
#endif

	CHKERR(sdr_begin_xn(bsspSdr));
	getSessionContext(BsspDB, sessionNbr, &sessionObj,
			&sessionBuf, &spanObj, &spanBuf, &vspan, &vspanElt);
	if (spanObj == 0)	/*	Unknown provenance, ignore.	*/
	{
		sdr_exit_xn(bsspSdr);
		return 0;
	}

	if (sessionObj == 0)
	{
		/*	Ack for an unknown session: must be in
		 *	response to arrival of retransmitted blocks
		 *	following session closure.  So the remote
		 *	import session is an erroneous resurrection
		 *	of a closed session and we need to help the
		 *	remote engine terminate it. We do so by
		 *	ignoring the acknowledgement. 			
	 	 */

		sdr_exit_xn(bsspSdr);
		return 0;
	}

	/*	Now process the acknowledgement if possible.		*/

	if (sessionBuf.totalLength == 0)/*	Reused session nbr.	*/
	{
#if BSSPDEBUG
putErrmsg("Discarding report.", NULL);
#endif
		return sdr_end_xn(bsspSdr);	/*	Ignore ACK.	*/
	}

	/*	Deactivate the retransmission timer.  Block has been
	 *	received so there will never be any need to retransmit
	 *	it.							*/

	sdr_stage(bsspSdr, (char *) &dsBuf, sessionBuf.block,
			sizeof(BsspXmitBlock));
	dsBuf.pdu.timer.pduArrivalTime = 0;
	sdr_write(bsspSdr, sessionBuf.block, (char *) &dsBuf,
			sizeof(BsspXmitBlock));
	
	stopExportSession(&sessionBuf);
	closeExportSession(sessionObj);
		
	if (sdr_end_xn(bsspSdr) < 0)
	{
		putErrmsg("Can't handle report block.", NULL);
		return -1;
	}

	if (bsspvdb->watching & WATCH_h)
	{
		putchar('h');
		fflush(stdout);
	}

	return 1;	/*	Complete, successful export.	*/
}

int	bsspHandleInboundBlock(char *buf, int length)
{
	BsspRecvBlk	block;
	BsspPdu		*pdu = &block.pdu;
	char		*cursor = buf;
	int		bytesRemaining = length;
	uvast		sourceEngineId;
	unsigned int	sessionNbr;
	Sdr		sdr;
			OBJ_POINTER(BsspDB, bsspdb);

	CHKERR(buf);
	CHKERR(length > 0);
	memset((char *) &block, 0, sizeof(BsspRecvBlk));

	/*	Get block type (flags).  Ignore version number.		*/
	pdu->blkTypeCode = (*cursor) & 0x01;
	/*	Based on the 1-bit shift performed in the
	 *	serializeHeader function.				*/
	cursor++;
	bytesRemaining--;

	/*	Get session ID.						*/

	extractSdnv(&sourceEngineId, &cursor, &bytesRemaining);
	extractSmallSdnv(&sessionNbr, &cursor, &bytesRemaining);

	cursor++;
	bytesRemaining--;

	/*	Handle segment according to its segment type code.	*/

	if ((_bsspvdb(NULL))->watching & WATCH_s)
	{
		putchar('s');
		fflush(stdout);
	}

	sdr = getIonsdr();
	CHKERR(sdr_begin_xn(sdr));
	GET_OBJ_POINTER(getIonsdr(), BsspDB, bsspdb, _bsspdbObject(NULL));
	sdr_exit_xn(sdr);

	/*	Since blkTypeCode is just 1-bit long one of the  	*
	 *	following "if" statements will result TRUE. 		*/

	if ((pdu->blkTypeCode & BSSP_ACK_FLAG) == 0)	/*	Data.	*/
	{
		return handleDataBlock(sourceEngineId, bsspdb, sessionNbr,
				&block, pdu, &cursor, &bytesRemaining);
	}

	/*	Check whether block is an ACK block.			*/
 
	if ((pdu->blkTypeCode & BSSP_ACK_FLAG) == 1)	/*	Ack.	*/
	{
		return handleAck(bsspdb, sessionNbr, &block, pdu, &cursor, 
				&bytesRemaining);
	}

	return 0;	 /*	Just to prevent compiler warning.	*/
}

/*	*	*	Functions that respond to events	*	*/

void	bsspStartXmit(BsspVspan *vspan)
{
	Sdr		bsspSdr = getIonsdr();
	Object		spanObj;
	BsspSpan	span;

	CHKVOID(ionLocked());
	CHKVOID(vspan);
	spanObj = sdr_list_data(bsspSdr, vspan->spanElt);
	sdr_read(bsspSdr, (char *) &span, spanObj, sizeof(BsspSpan));
	sm_SemGive(vspan->bufOpenSemaphore);
	if (sdr_list_length(bsspSdr, span.beBlocks) > 0)
	{
		sm_SemGive(vspan->beSemaphore);
	}

	if (sdr_list_length(bsspSdr, span.rlBlocks) > 0)
	{
		sm_SemGive(vspan->rlSemaphore);
	}
}

void	bsspStopXmit(BsspVspan *vspan)
{
	Sdr		bsspSdr = getIonsdr();
	Object		spanObj;
	BsspSpan	span;
	Object		elt;
	Object		nextElt;
	Object		sessionObj;
	ExportSession	session;

	CHKVOID(ionLocked());
	CHKVOID(vspan);
	spanObj = sdr_list_data(bsspSdr, vspan->spanElt);
	sdr_read(bsspSdr, (char *) &span, spanObj, sizeof(BsspSpan));
	if (span.purge)
	{
		/*	At end of transmission on this span we must
		 *	cancel all export sessions that are currently
		 *	in progress.  Notionally this forces re-
		 *	forwarding of the DTN bundles in each session's
		 *	block, to avoid having to wait for the restart
		 *	of transmission on this span before those
		 *	bundles can be successfully transmitted.	*/

		for (elt = sdr_list_first(bsspSdr, span.exportSessions); elt;
				elt = nextElt)
		{
			nextElt = sdr_list_next(bsspSdr, elt);
			sessionObj = sdr_list_data(bsspSdr, elt);
			sdr_stage(bsspSdr, (char *) &session, sessionObj,
					sizeof(ExportSession));
			if (session.svcDataObject == 0)
			{
				/*	Session is not yet populated
				 *	with any service data objects.	*/

				continue;
			}

			oK(cancelSessionBySender(&session, sessionObj,
					BsspCancelByEngine));
		}
	}
}

static void	suspendTimer(time_t suspendTime, BsspTimer *timer,
			Address timerAddr, unsigned int qTime,
			unsigned int remoteXmitRate, BsspEventType eventType,
			uvast eventRefNbr1, unsigned int eventRefNbr2)
{
	time_t	latestAckXmitStartTime;

	CHKVOID(ionLocked());
	latestAckXmitStartTime = timer->pduArrivalTime + qTime;
	if (latestAckXmitStartTime < suspendTime)
	{
		/*	Transmission of ack should have begun before
		 *	link was stopped.  Timer must not be suspended.	*/

		return;
	}

	/*	Must suspend timer while remote engine is unable to
	 *	transmit.						*/

	cancelEvent(eventType, eventRefNbr1, eventRefNbr2);

	/*	Change state of timer object and save it.		*/

	timer->state = BsspTimerSuspended;
	sdr_write(getIonsdr(), timerAddr, (char *) timer, sizeof(BsspTimer));
}

int	bsspSuspendTimers(BsspVspan *vspan, PsmAddress vspanElt,
		time_t suspendTime, unsigned int priorXmitRate)
{
	Sdr		bsspSdr = getIonsdr();
	Object		spanObj;
			OBJ_POINTER(BsspSpan, span);
	unsigned int	qTime;
	Object		elt;
	Object		sessionObj;
	BsspTimer	*timer;
	ExportSession	xsessionBuf;
	BsspXmitBlock	dsBuf;

	CHKERR(ionLocked());
	CHKERR(vspan);
	spanObj = sdr_list_data(bsspSdr, vspan->spanElt);
	GET_OBJ_POINTER(bsspSdr, BsspSpan, span, spanObj);
	qTime = span->remoteQtime;

	/*	Suspend relevant timers for export sessions.		*/

	for (elt = sdr_list_first(bsspSdr, span->exportSessions); elt;
			elt = sdr_list_next(bsspSdr, elt))
	{
		sessionObj = sdr_list_data(bsspSdr, elt);
		sdr_read(bsspSdr, (char *) &xsessionBuf, sessionObj,
				sizeof(ExportSession));

		if (xsessionBuf.block != 0)
		{

			/*	Suspend block retransmission timer
			 *	for each export session.		*/

			sdr_stage(bsspSdr, (char *) &dsBuf, xsessionBuf.block,
				sizeof(BsspXmitBlock));
			if (dsBuf.pdu.timer.pduArrivalTime == 0)
			{
				continue;	/*	Not active.	*/
			}

			timer = &dsBuf.pdu.timer;
			suspendTimer(suspendTime, timer,
				xsessionBuf.block + FLD_OFFSET(timer, &dsBuf),
				qTime, priorXmitRate, BsspResendBlock, 0,
				xsessionBuf.sessionNbr);
		}
	}

	return 0;
}

static int	resumeTimer(time_t resumeTime, BsspTimer *timer,
			Address timerAddr, unsigned int qTime,
			unsigned int remoteXmitRate, BsspEventType eventType,
			uvast refNbr1, unsigned int refNbr2)
{
	time_t		earliestAckXmitStartTime;
	int		additionalDelay;
	BsspEvent	event;

	CHKERR(ionLocked());
	earliestAckXmitStartTime = timer->pduArrivalTime + qTime;
	additionalDelay = resumeTime - earliestAckXmitStartTime;
	if (additionalDelay > 0)
	{
		/*	Must revise deadline.				*/

		timer->ackDeadline += additionalDelay;
	}

	/*	Change state of timer object and save it.		*/

	timer->state = BsspTimerRunning;
	sdr_write(getIonsdr(), timerAddr, (char *) timer, sizeof(BsspTimer));

	/*	Re-post timeout event.					*/

	event.type = eventType;
	event.refNbr1 = refNbr1;
	event.refNbr2 = refNbr2;
	event.parm = 0;
	event.scheduledTime = timer->ackDeadline;
	if (insertBsspTimelineEvent(&event) == 0)
	{
		putErrmsg("Can't insert timeout event.", NULL);
		return -1;
	}

	return 0;
}

int	bsspResumeTimers(BsspVspan *vspan, PsmAddress vspanElt,
		time_t resumeTime, unsigned int remoteXmitRate)
{
	Sdr		bsspSdr = getIonsdr();
	Object		spanObj;
			OBJ_POINTER(BsspSpan, span);
	unsigned int	qTime;
	Object		elt;
	Object		sessionObj;
	BsspTimer	*timer;
	ExportSession	xsessionBuf;
	BsspXmitBlock	dsBuf;

	CHKERR(ionLocked());
	CHKERR(vspan);
	spanObj = sdr_list_data(bsspSdr, vspan->spanElt);
	GET_OBJ_POINTER(bsspSdr, BsspSpan, span, spanObj);
	qTime = span->remoteQtime;

	/*	Resume relevant timers for export sessions.		*/

	for (elt = sdr_list_first(bsspSdr, span->exportSessions); elt;
			elt = sdr_list_next(bsspSdr, elt))
	{
		sessionObj = sdr_list_data(bsspSdr, elt);
		sdr_read(bsspSdr, (char *) &xsessionBuf, sessionObj,
				sizeof(ExportSession));

		/*	Resume block retransmission timer for each
		 *	session.					*/

		if (xsessionBuf.block != 0)
		{
			sdr_stage(bsspSdr, (char *) &dsBuf, xsessionBuf.block,
					sizeof(BsspXmitBlock));
			if (dsBuf.pdu.timer.pduArrivalTime == 0)
			{
				continue;	/*	Not active.	*/
			}

			if (dsBuf.pdu.timer.state != BsspTimerSuspended)
			{
				continue;	/*	Not suspended.	*/
			}

			/*	Must resume: re-insert timeout event.	*/

			timer = &dsBuf.pdu.timer;
			if (resumeTimer(resumeTime, timer,
				xsessionBuf.block + FLD_OFFSET(timer, &dsBuf),
				qTime, remoteXmitRate, BsspResendBlock, 0,
				xsessionBuf.sessionNbr) < 0)

			{
				putErrmsg("Can't resume timers for span.",
						itoa(span->engineId));
				sdr_cancel_xn(bsspSdr);
				return -1;
			}
		}
	}

	return 0;
}

int	bsspResendBlock(unsigned int sessionNbr)
{
	Sdr		bsspSdr = getIonsdr();
	Object		sessionObj;
	ExportSession	sessionBuf;
	BsspXmitBlock	dblkBuf;
			OBJ_POINTER(BsspSpan, span);

#if BSSPDEBUG
putErrmsg("Resending block.", itoa(sessionNbr));
#endif
	CHKERR(sdr_begin_xn(bsspSdr));
	getExportSession(sessionNbr, &sessionObj);
	if (sessionObj == 0)		/*	Session is gone.	*/
	{
#if BSSPDEBUG
putErrmsg("Session is gone.", itoa(sessionNbr));
#endif
		return sdr_end_xn(bsspSdr);
	}

	sdr_stage(bsspSdr, (char *) &sessionBuf, sessionObj,
			sizeof(ExportSession));
	
	if (sessionBuf.block == 0)	/*	Block is gone.		*/
	{
#if BSSPDEBUG
putErrmsg("Checkpoint is gone.", itoa(sessionNbr));
#endif
		return sdr_end_xn(bsspSdr);
	}

	sdr_stage(bsspSdr, (char *) &dblkBuf, sessionBuf.block,
			sizeof(BsspXmitBlock));
	if (dblkBuf.pdu.timer.pduArrivalTime == 0)
	{
#if BSSPDEBUG
putErrmsg("Checkpoint is already acknowledged.", itoa(sessionNbr));
#endif
		return sdr_end_xn(bsspSdr);
	}

	GET_OBJ_POINTER(bsspSdr, BsspSpan, span, sessionBuf.span);
	dblkBuf.queueListElt = sdr_list_insert_last(bsspSdr,
			span->rlBlocks, sessionBuf.block);
	sdr_write(bsspSdr, sessionBuf.block, (char *) &dblkBuf,
			sizeof(BsspXmitBlock));
	signalRlBso(span->engineId);
	if ((_bsspvdb(NULL))->watching & WATCH_resendBlk)
	{
		putchar('=');
		fflush(stdout);
	}

	if (sdr_end_xn(bsspSdr))
	{
		putErrmsg("Can't resend block.", NULL);
		return -1;
	}

	return 0;
}
