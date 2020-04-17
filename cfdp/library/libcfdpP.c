/*
 *	libcfdpP.c:	functions enabling the implementation of
 *			CFDP entities.
 *
 *	Copyright (c) 2009, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 *
 */

#include "cfdpP.h"
#include "lyst.h"

#ifndef CFDPDEBUG
#define	CFDPDEBUG		0
#endif

#ifndef	CFDP_FILLBUF_LIMIT
#define	CFDP_FILLBUF_LIMIT	65536
#endif

/*	*	*	Helpful utility functions	*	*	*/

static Object	_cfdpdbObject(Object *newDbObj)
{
	static Object	obj = 0;

	if (newDbObj)
	{
		obj = *newDbObj;
	}

	return obj;
}

static CfdpDB	*_cfdpConstants()
{
	static CfdpDB	buf;
	static CfdpDB	*db = NULL;
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
		dbObject = _cfdpdbObject(NULL);
		if (dbObject)
		{
			if (sdr_heap_is_halted(sdr))
			{
				sdr_read(sdr, (char *) &buf, dbObject,
						sizeof(CfdpDB));
			}
			else
			{
				CHKNULL(sdr_begin_xn(sdr));
				sdr_read(sdr, (char *) &buf, dbObject,
						sizeof(CfdpDB));
				sdr_exit_xn(sdr);
			}

			db = &buf;
		}
	}

	return db;
}

#ifdef TargetFFS
typedef struct
{
	char		*fileName;
	int		fileExists;
	pthread_cond_t	*cv;
} StatParms;

static void	*checkFileExists(void *parm)
{
	StatParms	*parms = (StatParms *) parm;
	int		result;
	struct stat	statbuf;

	result = stat(parms->fileName, &statbuf);

	/*	If hang on trying to find and open the file, wait
	 *	until parent thread times out and cancels this one.
	 *	Otherwise, depending on stat() result, indicate
	 *	whether or not the file exists and return.		*/

	if (result < 0)
	{
		parms->fileExists = 0;
	}
	else
	{
		parms->fileExists = 1;
	}

	/*	If no delay on opening the file, then the parent
	 *	thread is still waiting on the condition variable,
	 *	in which case we need to signal it right away.  In
	 *	any case, no harm in signaling it.			*/

	pthread_cond_signal(parms->cv);
	return NULL;
}
#endif

int	checkFile(char *fileName)
{
#ifdef TargetFFS
	StatParms	parms;
	pthread_mutex_t	mutex;
	pthread_cond_t	cv;
	pthread_attr_t	attr;
	pthread_t	statThread;
	struct timeval	workTime;
	struct timespec	deadline;
	int		result;

	parms.fileName = fileName;
	parms.fileExists = -1;	/*	Unknown.			*/
	memset((char *) &mutex, 0, sizeof mutex);
	if (pthread_mutex_init(&mutex, NULL))
	{
		putSysErrmsg("Can't initialize mutex", NULL);
		return -1;
	}

	memset((char *) &cv, 0, sizeof cv);
	if (pthread_cond_init(&cv, NULL))
	{
		oK(pthread_mutex_destroy(&mutex));
		putSysErrmsg("Can't initialize condition variable", NULL);
		return -1;
	}

	parms.cv = &cv;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	getCurrentTime(&workTime);
	deadline.tv_sec = workTime.tv_sec + 2;	/*	2 sec timeout.	*/
	deadline.tv_nsec = 0;

	/*	Spawn a separate thread that hangs on opening the file
	 *	if there's an error in the file system.			*/

	if (pthread_begin(&statThread, &attr, checkFileExists, &parms))
	{
		oK(pthread_mutex_destroy(&mutex));
		oK(pthread_cond_destroy(&cv));
		putSysErrmsg("Can't create stat thread", NULL);
		return -1;
	}

	pthread_attr_destroy(&attr);

	/*	At this point the child might already have finished
	 *	looking for the file and terminated, in which case
	 *	we want NOT to wait for a signal from it.		*/

	if (parms.fileExists == -1)	/*	Child not ended yet.	*/
	{
		/*	Wait for all-OK signal from child; if none,
		 *	cancel the child thread and note that the file
		 *	does not exist.					*/

		oK(pthread_mutex_lock(&mutex));
		result = pthread_cond_timedwait(&cv, &mutex, &deadline);
		oK(pthread_mutex_unlock(&mutex));
		if (result)	/*	NOT signaled by child thread.	*/
		{
			if (result != ETIMEDOUT)
			{
				errno = result;
				oK(pthread_mutex_destroy(&mutex));
				oK(pthread_cond_destroy(&cv));
				putSysErrmsg("pthread_cond_timedwait failed",
						NULL);
				return -1;
			}

			/*	Timeout: child stuck, file undefined.	*/

			pthread_end(statThread);
			parms.fileExists = 0;
		}
	}

	oK(pthread_mutex_destroy(&mutex));
	oK(pthread_cond_destroy(&cv));
	return parms.fileExists;
#else
	struct stat	statbuf;

	if (stat(fileName, &statbuf) < 0)
	{
		return 0;
	}

	return 1;
#endif
}

static void	addToModularChecksum(unsigned char octet, vast *offset,
			unsigned int *checksum)
{
	unsigned int	octetVal;
	unsigned int	N;

	/*	To get the value to add to the checksum:
	 *
	 *	-	The octet needs to be inserted into the
	 *		correct position in a 4-byte word based
	 *		on its offset from the start of the file.
	 *		Needs to be shifted left by 8N bits for
	 *		this purpose.
	 *
	 *	-	We calculate N as 3 minus the octet's
	 *		offset from the start of the file
	 *		modulo 4, then multiply it by 8 to
	 *		convert from bytes to bits.			*/

	N = 3 - (*offset & 0x03);
	octetVal = octet << (N << 3);	/*	Multiply N by 8.	*/
	*checksum += octetVal;
	(*offset)++;
}

void	addToChecksum(unsigned char octet, vast *offset,
		unsigned int *checksum, CfdpCksumType ckType)
{
	CHKVOID(checksum);
	switch (ckType)
	{
	case ModularChecksum:
		CHKVOID(offset);
		addToModularChecksum(octet, offset, checksum);
		break;

	case CRC32CChecksum:
		*checksum = ion_CRC32_1EDC6F41_C((char *) &octet, 1, *checksum);
		break;

	case NullChecksum:
		*checksum = 0;
		break;

	default:			/*	Unsupported checksum	*/
		break;
	}

	return;
}

int	getReqNbr()
{
	Sdr	sdr = getIonsdr();
	Object	dbObj = getCfdpDbObject();
	CfdpDB	db;

	CHKERR(ionLocked());
	sdr_stage(sdr, (char *) &db, dbObj, sizeof(CfdpDB));
	db.requestCounter++;
	sdr_write(sdr, dbObj, (char *) &db, sizeof(CfdpDB));
	return db.requestCounter;
}

static unsigned char	*_crcComputationBuf()
{
	static unsigned char	buffer[CFDP_MAX_PDU_SIZE];

	return buffer;
}

static unsigned short	computeCRC(unsigned char *buffer, int length)
{
	static int		crcCalcValuesInitialized = 0;
    	int			i;
    	unsigned int		tmp;
	static unsigned int	crcCalcValues[256];
	unsigned char		*cursor = buffer;
	unsigned int		crc = 0xffff;

	if (!crcCalcValuesInitialized)
	{
		for (i = 0; i < 256; i++)
		{
			tmp = 0;
			if ((i & 1) != 0) tmp = tmp ^ 0x1021;
			if ((i & 2) != 0) tmp = tmp ^ 0x2042;
			if ((i & 4) != 0) tmp = tmp ^ 0x4084;
			if ((i & 8) != 0) tmp = tmp ^ 0x8108;
			if ((i & 16) != 0) tmp = tmp ^ 0x1231;
			if ((i & 32) != 0) tmp = tmp ^ 0x2462;
			if ((i & 64) != 0) tmp = tmp ^ 0x48c4;
			if ((i & 128) != 0) tmp = tmp ^ 0x9188;
			crcCalcValues[i] = tmp;
		}

		crcCalcValuesInitialized = 1;
	}

	while (length > 0)
	{
	    crc = (((crc << 8) & 0xff00)
			^ crcCalcValues[(((crc >> 8) ^ (*cursor)) & 0x00ff)]);
	    cursor++;
	    length--;
	}

	return crc;
}

/*	*	*	CFDP service control functions	*	*	*/

static char	*_cfdpvdbName()
{
	return "cfdpvdb";
}

static CfdpVdb	*_cfdpvdb(char **name)
{
	static CfdpVdb	*vdb = NULL;
	PsmPartition	wm;
	PsmAddress	vdbAddress;
	PsmAddress	elt;
	Sdr		sdr;
	char		*corruptionModulusString;

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
			vdb = (CfdpVdb *) psp(wm, vdbAddress);
			return vdb;
		}

		/*	CFDP volatile database doesn't exist yet.	*/

		sdr = getIonsdr();
		CHKNULL(sdr_begin_xn(sdr));	/*	To lock memory.	*/
		vdbAddress = psm_zalloc(wm, sizeof(CfdpVdb));
		if (vdbAddress == 0)
		{
			sdr_exit_xn(sdr);
			putErrmsg("No space for volatile database.", NULL);
			return NULL;
		}

		vdb = (CfdpVdb *) psp(wm, vdbAddress);
		memset((char *) vdb, 0, sizeof(CfdpVdb));
		vdb->utaPid = ERROR;		/*	None yet.	*/
		vdb->clockPid = ERROR;		/*	None yet.	*/
		vdb->eventSemaphore = sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO);
		vdb->fduSemaphore = sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO);
		if (vdb->eventSemaphore == SM_SEM_NONE
		|| vdb->fduSemaphore == SM_SEM_NONE
		|| ionStartAttendant(&(vdb->attendant)) < 0
		|| psm_catlg(wm, *name, vdbAddress) < 0)
		{
			sdr_exit_xn(sdr);
			putErrmsg("Can't initialize volatile database.", NULL);
			return NULL;
		}

		sm_SemTake(vdb->eventSemaphore);/*	Lock.		*/
		sm_SemTake(vdb->fduSemaphore);	/*	Lock.		*/
		vdb->currentFile = -1;		/*	Nothing open.	*/
		corruptionModulusString = getenv("CFDP_CORRUPTION_MODULUS");
		if (corruptionModulusString)
		{
			vdb->corruptionModulus = strtol(corruptionModulusString,
					NULL, 0);
			writeMemoNote("[?] Non-zero CFDP corruption modulus!",
					utoa(vdb->corruptionModulus));
		}

		sdr_exit_xn(sdr);	/*	Unlock wm.		*/
	}

	return vdb;
}

static char	*_cfdpdbName()
{
	return "cfdpdb";
}

int	cfdpInit()
{
	Sdr		sdr;
	Object		cfdpdbObject;
	IonDB		iondb;
	CfdpDB		cfdpdbBuf;
	int		i;
	char		*cfdpvdbName = _cfdpvdbName();

	if (ionAttach() < 0)
	{
		putErrmsg("CFDP can't attach to ION.", NULL);
		return -1;
	}

	sdr = getIonsdr();

	/*	Recover the CFDP database, creating it if necessary.	*/

	CHKERR(sdr_begin_xn(sdr));
	cfdpdbObject = sdr_find(sdr, _cfdpdbName(), NULL);
	switch (cfdpdbObject)
	{
	case -1:		/*	SDR error.			*/
		sdr_cancel_xn(sdr);
		putErrmsg("Can't search for CFDP database in SDR.", NULL);
		return -1;

	case 0:			/*	Not found; must create new DB.	*/
		cfdpdbObject = sdr_malloc(sdr, sizeof(CfdpDB));
		if (cfdpdbObject == 0)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("No space for database.", NULL);
			return -1;
		}

		/*	Initialize the non-volatile database.		*/

		memset((char *) &cfdpdbBuf, 0, sizeof(CfdpDB));
		sdr_read(sdr, (char *) &iondb, getIonDbObject(),
				sizeof(IonDB));
		cfdpdbBuf.ownEntityId = iondb.ownNodeNbr;
		cfdp_compress_number(&cfdpdbBuf.ownEntityNbr,
				cfdpdbBuf.ownEntityId);

		/*	Default values.					*/

		cfdpdbBuf.maxTransactionNbr = 999999999;
		cfdpdbBuf.fillCharacter = 0xaa;
		cfdpdbBuf.discardIncompleteFile = 1;
		cfdpdbBuf.crcRequired = 0;
		cfdpdbBuf.maxFileDataLength = 65000;
		cfdpdbBuf.transactionInactivityLimit = 86400;
		cfdpdbBuf.checkTimerPeriod = 86400;	/*	1 day.	*/
		cfdpdbBuf.checkTimeoutLimit = 7;
		cfdpdbBuf.maxQueuedEvents = 20;

		/*	Management information.				*/

		for (i = 0; i < 16; i++)
		{
			cfdpdbBuf.faultHandlers[i] = CfdpIgnore;
		}

		cfdpdbBuf.faultHandlers[CfdpFilestoreRejection] = CfdpCancel;
		cfdpdbBuf.faultHandlers[CfdpCheckLimitReached] = CfdpCancel;
		cfdpdbBuf.usrmsgLists = sdr_list_create(sdr);
		cfdpdbBuf.fsreqLists = sdr_list_create(sdr);
		cfdpdbBuf.fsrespLists = sdr_list_create(sdr);
		cfdpdbBuf.outboundFdus = sdr_list_create(sdr);
		cfdpdbBuf.events = sdr_list_create(sdr);
		cfdpdbBuf.entities = sdr_list_create(sdr);
		cfdpdbBuf.finishPdus = sdr_list_create(sdr);
		cfdpdbBuf.finsPending = sdr_list_create(sdr);
		sdr_write(sdr, cfdpdbObject, (char *) &cfdpdbBuf,
				sizeof(CfdpDB));
		sdr_catlg(sdr, _cfdpdbName(), 0, cfdpdbObject);
		if (sdr_end_xn(sdr))
		{
			putErrmsg("Can't create CFDP database.", NULL);
			return -1;
		}

		break;

	default:			/*	Found DB in the SDR.	*/
		sdr_exit_xn(sdr);
	}

	oK(_cfdpdbObject(&cfdpdbObject));/*	Save database location.	*/
	oK(_cfdpConstants());

	/*	Locate volatile database, initializing as necessary.	*/

	if (_cfdpvdb(&cfdpvdbName) == NULL)
	{
		putErrmsg("CFDP can't initialize vdb.", NULL);
		return -1;
	}

	return 0;		/*	CFDP service is available.	*/
}

static void	dropVdb(PsmPartition wm, PsmAddress vdbAddress)
{
	CfdpVdb		*vdb;

	vdb = (CfdpVdb *) psp(wm, vdbAddress);
	if (vdb->eventSemaphore != SM_SEM_NONE)
	{
		sm_SemEnd(vdb->eventSemaphore);
		microsnooze(50000);
		sm_SemDelete(vdb->eventSemaphore);
	}

	if (vdb->fduSemaphore != SM_SEM_NONE)
	{
		sm_SemEnd(vdb->fduSemaphore);
		microsnooze(50000);
		sm_SemDelete(vdb->fduSemaphore);
	}

	if (vdb->currentFile != -1)
	{
		close(vdb->currentFile);
	}
}

void	cfdpDropVdb()
{
	PsmPartition	wm = getIonwm();
	char		*cfdpvdbName = _cfdpvdbName();
	PsmAddress	vdbAddress;
	PsmAddress	elt;
	char		*stop = NULL;

	if (psm_locate(wm, cfdpvdbName, &vdbAddress, &elt) < 0)
	{
		putErrmsg("Failed searching for vdb.", NULL);
		return;
	}

	if (elt)
	{
		dropVdb(wm, vdbAddress);	/*	Destroy Vdb.	*/
		psm_free(wm,vdbAddress);
		if (psm_uncatlg(wm, cfdpvdbName) < 0)
		{
			putErrmsg("Failed uncataloging vdb.",NULL);
		}
	}

	oK(_cfdpvdb(&stop));			/*	Forget old Vdb.	*/
}

void	cfdpRaiseVdb()
{
	char	*cfdpvdbName = _cfdpvdbName();

	if (_cfdpvdb(&cfdpvdbName) == NULL)	/*	Create new Vdb.	*/
	{
		putErrmsg("CFDP can't reinitialize vdb.", NULL);
	}
}

Object	getCfdpDbObject()
{
	return _cfdpdbObject(NULL);
}

CfdpDB	*getCfdpConstants()
{
	return _cfdpConstants();
}

CfdpVdb	*getCfdpVdb()
{
	return _cfdpvdb(NULL);
}

int	_cfdpStart(char *utaCmd)
{
	Sdr	sdr = getIonsdr();
	CfdpVdb	*cfdpvdb = _cfdpvdb(NULL);

	if (utaCmd == NULL)
	{
		putErrmsg("CFDP can't start: no UTA command.", NULL);
		return -1;
	}

	CHKERR(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/

	/*	Start the CFDP events clock if necessary.		*/

	if (cfdpvdb->clockPid == ERROR || sm_TaskExists(cfdpvdb->clockPid) == 0)
	{
		cfdpvdb->clockPid = pseudoshell("cfdpclock");
	}

	/*	Start UT adapter service if necessary.			*/

	if (cfdpvdb->utaPid == ERROR || sm_TaskExists(cfdpvdb->utaPid) == 0)
	{
		cfdpvdb->utaPid = pseudoshell(utaCmd);
	}

	sdr_exit_xn(sdr);	/*	Unlock memory.			*/
	return 0;
}

void	_cfdpStop()		/*	Reverses cfdpStart.		*/
{
	Sdr		sdr = getIonsdr();
	CfdpVdb		*cfdpvdb = _cfdpvdb(NULL);

	/*	Tell all CFDP processes to stop.			*/

	CHKVOID(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/

	/*	Disable blocking ZCO buffer space access.		*/

	ionPauseAttendant(&(cfdpvdb->attendant));

	/*	Stop user application input thread.			*/

	if (cfdpvdb->eventSemaphore != SM_SEM_NONE)
	{
		sm_SemEnd(cfdpvdb->eventSemaphore);
	}

	/*	Stop UTA task.						*/
	
	if (cfdpvdb->fduSemaphore != SM_SEM_NONE)
	{
		sm_SemEnd(cfdpvdb->fduSemaphore);
	}

	/*	Stop clock task.					*/

	if (cfdpvdb->clockPid != ERROR)
	{
		sm_TaskKill(cfdpvdb->clockPid, SIGTERM);
	}

	ionStopAttendant(&(cfdpvdb->attendant));
	sdr_exit_xn(sdr);	/*	Unlock memory.			*/

	/*	Wait until all CFDP processes have stopped.		*/

	if (cfdpvdb->utaPid != ERROR)
	{
		while (sm_TaskExists(cfdpvdb->utaPid))
		{
			microsnooze(100000);
		}
	}

	if (cfdpvdb->clockPid != ERROR)
	{
		while (sm_TaskExists(cfdpvdb->clockPid))
		{
			microsnooze(100000);
		}
	}

	/*	Now erase all the tasks and reset the semaphores.	*/

	CHKVOID(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	cfdpvdb->utaPid = ERROR;
	cfdpvdb->clockPid = ERROR;
	if (cfdpvdb->eventSemaphore == SM_SEM_NONE)
	{
		cfdpvdb->eventSemaphore = sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO);
	}
	else
	{
		sm_SemUnend(cfdpvdb->eventSemaphore);
		sm_SemGive(cfdpvdb->eventSemaphore);
	}

	sm_SemTake(cfdpvdb->eventSemaphore);		/*	Lock.	*/
	if (cfdpvdb->fduSemaphore == SM_SEM_NONE)
	{
		cfdpvdb->fduSemaphore = sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO);
	}
	else
	{
		sm_SemUnend(cfdpvdb->fduSemaphore);
		sm_SemGive(cfdpvdb->fduSemaphore);
	}

	sm_SemTake(cfdpvdb->fduSemaphore);		/*	Lock.	*/
	if (cfdpvdb->currentFile != -1)
	{
		close(cfdpvdb->currentFile);
		cfdpvdb->currentFile = -1;
	}

	cfdpvdb->currentFdu = 0;
	sdr_exit_xn(sdr);	/*	Unlock memory.			*/
}

int	cfdpAttach()
{
	Object		cfdpdbObject = _cfdpdbObject(NULL);
	CfdpVdb		*cfdpvdb = _cfdpvdb(NULL);
	Sdr		sdr;
	char		*cfdpvdbName = _cfdpvdbName();

	if (cfdpdbObject && cfdpvdb)
	{
		return 0;		/*	Already attached.	*/
	}

	if (ionAttach() < 0)
	{
		putErrmsg("CFDP can't attach to ION.", NULL);
		return -1;
	}

	sdr = getIonsdr();

	/*	Locate the CFDP database.				*/

	if (cfdpdbObject == 0)
	{
		CHKERR(sdr_begin_xn(sdr));	/*	Lock database.	*/
		cfdpdbObject = sdr_find(sdr, _cfdpdbName(), NULL);
		sdr_exit_xn(sdr);	/*	Unlock database.	*/
		if (cfdpdbObject == 0)
		{
			putErrmsg("Can't find CFDP database.", NULL);
			return -1;
		}

		oK(_cfdpdbObject(&cfdpdbObject));
	}

	oK(_cfdpConstants());

	/*	Locate the CFDP volatile database.			*/

	if (cfdpvdb == NULL)
	{
		if (_cfdpvdb(&cfdpvdbName) == NULL)
		{
			putErrmsg("CFDP volatile database not found.", NULL);
			return -1;
		}
	}

	return 0;		/*	CFDP service is available.	*/
}

void	cfdpDetach()
{
	char	*stop = NULL;

	oK(_cfdpvdb(&stop));
	return;
}

MetadataList	createMetadataList(Object log)
{
	Sdr	sdr = getIonsdr();
	Object	list;

	/*	Create new list of metadata objects, reference it at
	 *	the end of the database's list of metadata lists, and
	 *	store that reference in the new list's own user data.	*/

	CHKZERO(log);
	CHKZERO(sdr_begin_xn(sdr));
	list = sdr_list_create(sdr);
	if (list)
	{
		sdr_list_user_data_set(sdr, list,
				sdr_list_insert_last(sdr, log, list));
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't create metadata list.", NULL);
		return 0;
	}

	return list;
}

void	destroyUsrmsgList(MetadataList *list)
{
	Sdr		sdr = getIonsdr();
	Object		elt;
	Object		obj;
	MsgToUser	usrmsg;

	CHKVOID(list);
	elt = sdr_list_first(sdr, *list);
	while (elt)
	{
		obj = sdr_list_data(sdr, elt);
		sdr_list_delete(sdr, elt, NULL, NULL);
		sdr_read(sdr, (char *) &usrmsg, obj, sizeof(MsgToUser));
		if (usrmsg.text)
		{
			sdr_free(sdr, usrmsg.text);
		}

		sdr_free(sdr, obj);
		elt = sdr_list_first(sdr, *list);
	}

	elt = sdr_list_user_data(sdr, *list);
	if (elt)
	{
		sdr_list_delete(sdr, elt, NULL, NULL);
	}

	sdr_list_destroy(sdr, *list, NULL, NULL);
	*list = 0;
}

void	destroyFsreqList(MetadataList *list)
{
	Sdr			sdr = getIonsdr();
	Object			elt;
	Object			obj;
	FilestoreRequest	fsreq;

	CHKVOID(list);
	elt = sdr_list_first(sdr, *list);
	while (elt)
	{
		obj = sdr_list_data(sdr, elt);
		sdr_list_delete(sdr, elt, NULL, NULL);
		sdr_read(sdr, (char *) &fsreq, obj,
				sizeof(FilestoreRequest));
		if (fsreq.firstFileName)
		{
			sdr_free(sdr, fsreq.firstFileName);
		}

		if (fsreq.secondFileName)
		{
			sdr_free(sdr, fsreq.secondFileName);
		}

		sdr_free(sdr, obj);
		elt = sdr_list_first(sdr, *list);
	}

	elt = sdr_list_user_data(sdr, *list);
	if (elt)
	{
		sdr_list_delete(sdr, elt, NULL, NULL);
	}

	sdr_list_destroy(sdr, *list, NULL, NULL);
	*list = 0;
}

void	destroyFsrespList(Object *list)
{
	Sdr			sdr = getIonsdr();
	Object			elt;
	Object			obj;
	FilestoreResponse	fsresp;

	CHKVOID(list);
	elt = sdr_list_first(sdr, *list);
	while (elt)
	{
		obj = sdr_list_data(sdr, elt);
		sdr_list_delete(sdr, elt, NULL, NULL);
		sdr_read(sdr, (char *) &fsresp, obj,
				sizeof(FilestoreResponse));
		if (fsresp.firstFileName)
		{
			sdr_free(sdr, fsresp.firstFileName);
		}

		if (fsresp.secondFileName)
		{
			sdr_free(sdr, fsresp.secondFileName);
		}

		if (fsresp.message)
		{
			sdr_free(sdr, fsresp.message);
		}

		sdr_free(sdr, obj);
		elt = sdr_list_first(sdr, *list);
	}

	elt = sdr_list_user_data(sdr, *list);
	if (elt)
	{
		sdr_list_delete(sdr, elt, NULL, NULL);
	}

	sdr_list_destroy(sdr, *list, NULL, NULL);
	*list = 0;
}

void	cfdpScrub()
{
	Sdr		sdr = getIonsdr();
	CfdpDB		*cfdpConstants = _cfdpConstants();
	Object		elt;
	MetadataList	list;

	CHKVOID(ionLocked());
	elt = sdr_list_first(sdr, cfdpConstants->usrmsgLists);
	while (elt)
	{
		list = sdr_list_data(sdr, elt);
		destroyUsrmsgList(&list);
		elt = sdr_list_first(sdr, cfdpConstants->usrmsgLists);
	}

	elt = sdr_list_first(sdr, cfdpConstants->fsreqLists);
	while (elt)
	{
		list = sdr_list_data(sdr, elt);
		destroyFsreqList(&list);
		elt = sdr_list_first(sdr, cfdpConstants->fsreqLists);
	}

	elt = sdr_list_first(sdr, cfdpConstants->fsrespLists);
	while (elt)
	{
		list = sdr_list_data(sdr, elt);
		destroyFsrespList(&list);
		elt = sdr_list_first(sdr, cfdpConstants->fsrespLists);
	}
}

int	addFsResp(Object list, CfdpAction action, int status,
		char *firstFileName, char *secondFileName, char *message)
{
	Sdr			sdr = getIonsdr();
	CfdpDB			*cfdpConstants = _cfdpConstants();
	FilestoreResponse	fsresp;
	Object			addr;

	CHKERR(list);
	CHKERR(firstFileName == NULL || strlen(firstFileName) < 256);
	CHKERR(secondFileName == NULL || strlen(secondFileName) < 256);
	CHKERR(message == NULL || strlen(secondFileName) < 256);
	CHKERR(sdr_list_list(sdr, sdr_list_user_data(sdr, list))
			== cfdpConstants->fsreqLists);
	CHKERR(sdr_begin_xn(sdr));
	fsresp.action = action;
	fsresp.status = status;
	if (firstFileName)
	{
		fsresp.firstFileName = sdr_string_create(sdr,
				firstFileName);
	}

	if (secondFileName)
	{
		fsresp.secondFileName = sdr_string_create(sdr,
				secondFileName);
	}

	if (message)
	{
		fsresp.message = sdr_string_create(sdr, message);
	}

	addr = sdr_malloc(sdr, sizeof(FilestoreResponse));
	if (addr)
	{
		sdr_write(sdr, addr, (char *) &fsresp,
				sizeof(FilestoreResponse));
		oK(sdr_list_insert_last(sdr, list, addr));
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("CFDP: failed adding filestore response.", NULL);
		return -1;
	}

	return 0;
}

/*	*	CFDP entity management functions	*	*	*/

static Object	locateEntity(uvast entityId, Object *nextEntity)
{
	Sdr	sdr = getIonsdr();
	CfdpDB	*db = _cfdpConstants();
	Object	elt;
	Object	entityObj;
	Entity	entity;

	if (nextEntity) *nextEntity = 0;	/*	Default.	*/
	for (elt = sdr_list_first(sdr, db->entities); elt;
			elt = sdr_list_next(sdr, elt))
	{
		entityObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &entity, entityObj, sizeof(Entity));
		if (entity.entityId < entityId)
		{
			continue;
		}

		if (entity.entityId > entityId)
		{
			if (nextEntity) *nextEntity = elt;
			break;		/*	Same as end of list.	*/
		}

		return elt;
	}

	return 0;
}

Object	findEntity(uvast entityId, Entity *entity)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
	Object	entityObj;

	CHKZERO(entity);
	elt = locateEntity(entityId, NULL);
	if (elt == 0)
	{
		return 0;
	}

	entityObj = sdr_list_data(sdr, elt);
	sdr_read(sdr, (char *) entity, entityObj, sizeof(Entity));
	return entityObj;
}

int	ckTypeOkay(unsigned int ckType)
{
	switch (ckType)
	{
	case ModularChecksum:
	case CRC32CChecksum:
	case NullChecksum:
		return 1;

	default:
		return 0;
	}
}

Object	addEntity(uvast entityId, char *protocolName, char *endpointName,
		unsigned int rtt, unsigned int inCkType, unsigned int outCkType)
{
	Sdr	sdr = getIonsdr();
	CfdpDB	*db = _cfdpConstants();
	Object	elt;
	Object	nextElt;
	Object	entityObj;
	Entity	entity;

	elt = locateEntity(entityId, &nextElt);
	if (elt)
	{
		writeMemoNote("[?] CFDP entity already exists", itoa(entityId));
		return 0;
	}

	memset((char *) &entity, 0, sizeof(Entity));
	entity.entityId = entityId;
	istrcpy(entity.protocolName, protocolName, sizeof entity.protocolName);
	istrcpy(entity.endpointName, endpointName, sizeof entity.endpointName);
	if (strcmp(protocolName, "bp") == 0)
	{
		entity.utLayer = UtBp;
		entity.bpNodeNbr = entityId;
	}
	else if (strcmp(protocolName, "ltp") == 0)
	{
		entity.utLayer = UtLtp;
		entity.ltpEngineNbr = entityId;
	}
	else if (strcmp(protocolName, "tcp") == 0)
	{
		entity.utLayer = UtTcp;
		parseSocketSpec(endpointName, &entity.portNbr,
				&entity.ipAddress);
		if (entity.ipAddress == 0)
		{
			entity.ipAddress = getAddressOfHost();
			if (entity.ipAddress == 0)
			{
				putErrmsg("No own IP address for CFDP entity.",
						NULL);
				return 0;
			}
		}
	}
	else
	{
		writeMemoNote("[?] Invalid UT layer protocol", protocolName);
		return 0;
	}

	entity.ackTimerInterval = rtt;
	if (!ckTypeOkay(inCkType))
	{
		writeMemoNote("[?] Invalid inCkType", utoa(inCkType));
		return 0;
	}

	if (!ckTypeOkay(outCkType))
	{
		writeMemoNote("[?] Note invalid outCkType", utoa(outCkType));
	}

	entity.inCkType = inCkType;
	entity.outCkType = outCkType;
	entity.inboundFdus = sdr_list_create(sdr);
	entityObj = sdr_malloc(sdr, sizeof(Entity));
	if (entity.inboundFdus == 0 || entityObj == 0
	|| (nextElt == 0	?
		sdr_list_insert_last(sdr, db->entities, entityObj)
		: 
		sdr_list_insert_before(sdr, nextElt, entityObj)) == 0)
	{
		return 0;	/*	System failure.		*/
	}

	sdr_write(sdr, entityObj, (char *) &entity, sizeof(Entity));
	return entityObj;
}

int	changeEntity(uvast entityId, char *protocolName, char *endpointName,
		unsigned int rtt, unsigned int inCkType, unsigned int outCkType)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
	Object	entityObj;
	Entity	entity;

	elt = locateEntity(entityId, NULL);
	if (elt == 0)
	{
		writeMemoNote("[?] CFDP entity doesn't exist", itoa(entityId));
		return -1;
	}

	entityObj = sdr_list_data(sdr, elt);
	sdr_read(sdr, (char *) &entity, entityObj, sizeof(Entity));
	istrcpy(entity.protocolName, protocolName, sizeof entity.protocolName);
	istrcpy(entity.endpointName, endpointName, sizeof entity.endpointName);
	if (strcmp(protocolName, "bp") == 0)
	{
		entity.utLayer = UtBp;
		entity.bpNodeNbr = entityId;
	}
	else if (strcmp(protocolName, "ltp") == 0)
	{
		entity.utLayer = UtLtp;
		entity.ltpEngineNbr = entityId;
	}
	else if (strcmp(protocolName, "tcp") == 0)
	{
		entity.utLayer = UtTcp;
		parseSocketSpec(endpointName, &entity.portNbr,
				&entity.ipAddress);
		if (entity.ipAddress == 0)
		{
			entity.ipAddress = getAddressOfHost();
			if (entity.ipAddress == 0)
			{
				putErrmsg("No own IP address for CFDP entity.",
						NULL);
				return -1;
			}
		}
	}
	else
	{
		writeMemoNote("[?] Invalid UT layer protocol", protocolName);
		return -1;
	}

	if (!ckTypeOkay(inCkType))
	{
		writeMemoNote("[?] Invalid inCkType", utoa(inCkType));
		return 0;
	}

	if (!ckTypeOkay(outCkType))
	{
		writeMemoNote("[?] Note invalid outCkType", utoa(outCkType));
	}

	entity.ackTimerInterval = rtt;
	entity.inCkType = inCkType;
	entity.outCkType = outCkType;
	sdr_write(sdr, entityObj, (char *) &entity, sizeof(Entity));
	return 0;
}

int	removeEntity(uvast entityId)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
	Object	entityObj;
	Entity	entity;

	elt = locateEntity(entityId, NULL);
	if (elt == 0)
	{
		writeMemoNote("[?] CFDP entity doesn't exist", itoa(entityId));
		return -1;
	}

	entityObj = sdr_list_data(sdr, elt);
	sdr_read(sdr, (char *) &entity, entityObj, sizeof(Entity));
	if (sdr_list_length(sdr, entity.inboundFdus) > 0)
	{
		writeMemoNote("[?] Can't delete entity, FDUs pending",
				itoa(entityId));
		return -1;
	}

	sdr_list_destroy(sdr, entity.inboundFdus, NULL, NULL);
	sdr_free(sdr, entityObj);
	sdr_list_delete(sdr, elt, NULL, NULL);
	return 0;
}

/*	*	CFDP transaction mgt and access functions	*	*/

Object	findOutFdu(CfdpTransactionId *transactionId, OutFdu *fduBuf,
		Object *fduElt)
{
	Sdr	sdr = getIonsdr();
	CfdpDB	*cfdpConstants = _cfdpConstants();
	Object	elt;
	Object	fduObj;

	CHKZERO(transactionId);
	CHKZERO(fduBuf);
	CHKZERO(fduElt);
	*fduElt = 0;			/*	Default.		*/
	for (elt = sdr_list_first(sdr, cfdpConstants->outboundFdus); elt;
			elt = sdr_list_next(sdr, elt))
	{
		fduObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) fduBuf, fduObj, sizeof(OutFdu));
		if (memcmp(fduBuf->transactionId.transactionNbr.buffer,
				transactionId->transactionNbr.buffer, 8) == 0)
		{
			*fduElt = elt;
			return fduObj;
		}
	}

	return 0;
}

static Object	createInFdu(CfdpTransactionId *transactionId, Entity *entity,
			InFdu *fdubuf, Object *fduElt)
{
	Sdr		sdr = getIonsdr();
	Object		fduObj;
	CfdpDB		cfdpdb;

	memset((char *) fdubuf, 0, sizeof(InFdu));
	memcpy((char *) &fdubuf->transactionId, (char *) transactionId,
			sizeof(CfdpTransactionId));
	fdubuf->messagesToUser = sdr_list_create(sdr);
	fdubuf->filestoreRequests = sdr_list_create(sdr);
	fdubuf->extents = sdr_list_create(sdr);
	fdubuf->ckType = NullChecksum;		/*	Default		*/
	fdubuf->finishCondition = CfdpNoError;	/*	Default		*/
	fduObj = sdr_malloc(sdr, sizeof(InFdu));
	if (fduObj == 0 || fdubuf->messagesToUser == 0
	|| fdubuf->filestoreRequests == 0 || fdubuf->extents == 0
	|| (*fduElt = sdr_list_insert_last(sdr, entity->inboundFdus,
			fduObj)) == 0)
	{
		return 0;		/*	System failure.		*/
	}

	sdr_read(sdr, (char *) &cfdpdb, getCfdpDbObject(), sizeof(CfdpDB));
	fdubuf->inactivityDeadline = getCtime()
			+ cfdpdb.transactionInactivityLimit;
	sdr_write(sdr, fduObj, (char *) fdubuf, sizeof(InFdu));
	return fduObj;
}

Object	findInFdu(CfdpTransactionId *transactionId, InFdu *fduBuf,
		Object *fduElt, int createIfNotFound)
{
	uvast	sourceEntityId;
	Sdr	sdr = getIonsdr();
	Object	elt;
	Object	entityObj;
	Entity	entity;
	int	foundIt;
	Object	fduObj;

	CHKZERO(transactionId);
	CHKZERO(fduBuf);
	CHKZERO(fduElt);
	cfdp_decompress_number(&sourceEntityId,
			&transactionId->sourceEntityNbr);
	elt = locateEntity(sourceEntityId, NULL);
	if (elt)	/*	This is a known source entity.		*/
	{
		entityObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &entity, entityObj, sizeof(Entity));
		foundIt = 0;
		for (elt = sdr_list_first(sdr, entity.inboundFdus); elt;
				elt = sdr_list_next(sdr, elt))
		{
			fduObj = sdr_list_data(sdr, elt);
			sdr_read(sdr, (char *) fduBuf, fduObj,
					sizeof(InFdu));
			if (memcmp((char *) &fduBuf->transactionId,
					(char *) transactionId,
					sizeof(CfdpTransactionId)) == 0)
			{
				foundIt = 1;
				break;
			}
		}
	
		if (foundIt)	/*	FDU is already started.		*/
		{
			*fduElt = elt;
			return fduObj;
		}

		/*	No such FDU.  Create it?			*/

		if (createIfNotFound)
		{
			return createInFdu(transactionId, &entity, fduBuf,
					fduElt);
		}

		*fduElt = 0;
		return 0;
	}

	/*	No such entity.						*/

	if (!createIfNotFound)
	{
		*fduElt = 0;
		return 0;
	}

	/*	Must create Entity, then create FDU within new Entity.
	 *	Assume BP-reachable entity, modular checksum.		*/

	cfdp_decompress_number(&entity.entityId,
			&transactionId->sourceEntityNbr);
	entityObj = addEntity(sourceEntityId, "bp", "*", 1, 0, 0);
	if (entityObj == 0)
	{
		return 0;
	}

	sdr_read(sdr, (char *) &entity, entityObj, sizeof(Entity));
	return createInFdu(transactionId, &entity, fduBuf, fduElt);
}

int	suspendOutFdu(CfdpTransactionId *transactionId, CfdpCondition condition,
			int reqNbr)
{
	OutFdu		fduBuf;
	Object		fduObj;
	Object		fduElt;
	Sdr		sdr = getIonsdr();
	CfdpEvent	event;

	CHKZERO(transactionId);
	fduObj = findOutFdu(transactionId, &fduBuf, &fduElt);
	if (fduObj == 0 || fduBuf.state != FduActive)
	{
		return 0;
	}

	sdr_stage(sdr, NULL, fduObj, 0);
	fduBuf.state = FduSuspended;
	sdr_write(sdr, fduObj, (char *) &fduBuf, sizeof(OutFdu));
	memset((char *) &event, 0, sizeof(CfdpEvent));
	event.type = CfdpSuspendedInd;
	memcpy((char *) &event.transactionId, (char *) transactionId,
			sizeof(CfdpTransactionId));
	event.condition = condition;
	event.reqNbr = reqNbr;
	if (enqueueCfdpEvent(&event) < 0)
	{
		putErrmsg("CFDP can't suspend transaction.", NULL);
		return -1;
	}

	return event.reqNbr;
}

int	cancelOutFdu(CfdpTransactionId *transactionId, CfdpCondition condition,
			int reqNbr)
{
	OutFdu		fduBuf;
	Object		fduObj;
	Object		fduElt;
	Sdr		sdr = getIonsdr();
	CfdpEvent	event;

	CHKZERO(transactionId);
	fduObj = findOutFdu(transactionId, &fduBuf, &fduElt);
	if (fduObj == 0 || fduBuf.state == FduCanceled)
	{
		writeMemo("[?] CFDP unable to cancel outbound FDU.");
		return 0;
	}

	sdr_stage(sdr, NULL, fduObj, 0);
	fduBuf.state = FduCanceled;
	sdr_write(sdr, fduObj, (char *) &fduBuf, sizeof(OutFdu));
	memset((char *) &event, 0, sizeof(CfdpEvent));
	event.type = CfdpTransactionFinishedInd;
	memcpy((char *) &event.transactionId, (char *) transactionId,
			sizeof(CfdpTransactionId));
	event.condition = condition;
	event.fileStatus = CfdpFileStatusUnreported;
	if (fduBuf.metadataPdu == 0 && fduBuf.eofPdu == 0
	&& fduBuf.progress == fduBuf.fileSize)
	{
		event.deliveryCode = CfdpDataComplete;
	}
	else
	{
		event.deliveryCode = CfdpDataIncomplete;
	}

	event.reqNbr = reqNbr;
	if (enqueueCfdpEvent(&event) < 0)
	{
		putErrmsg("CFDP can't cancel transaction.", NULL);
		return -1;
	}

	return event.reqNbr;
}

void	destroyOutFdu(OutFdu *fdu, Object fduObj, Object fduElt)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
	Object	obj;
		OBJ_POINTER(FileDataPdu, pdu);

	CHKVOID(fdu);
	CHKVOID(fduObj);
	CHKVOID(fduElt);
	if (fdu->metadataPdu)
	{
		sdr_free(sdr, fdu->metadataPdu);
	}

	while (fdu->fileDataPdus)
	{
		elt = sdr_list_first(sdr, fdu->fileDataPdus);
		if (elt == 0)
		{
			sdr_list_destroy(sdr, fdu->fileDataPdus, NULL, NULL);
			break;
		}

		obj = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, FileDataPdu, pdu, obj);
		if (pdu->metadata)
		{
			sdr_free(sdr, pdu->metadata);
		}

		sdr_free(sdr, obj);
		sdr_list_delete(sdr, elt, NULL, NULL);
	}

	if (fdu->eofPdu)
	{
		sdr_free(sdr, fdu->eofPdu);
	}

	if (fdu->closureElt)
	{
		sdr_free(sdr,  sdr_list_data(sdr, fdu->closureElt));
		sdr_list_delete(sdr, fdu->closureElt, NULL, NULL);
	}

	if (fdu->fileRef)
	{
		zco_destroy_file_ref(sdr, fdu->fileRef);
	}

	sdr_free(sdr, fduObj);
	sdr_list_delete(sdr, fduElt, NULL, NULL);
}

static int	abandonOutFdu(CfdpTransactionId *transactionId,
			CfdpCondition condition)
{
	Sdr		sdr = getIonsdr();
	OutFdu		fduBuf;
	Object		fduObj;
	Object		fduElt;
	CfdpEvent	event;

	fduObj = findOutFdu(transactionId, &fduBuf, &fduElt);
	if (fduObj == 0)
	{
		return 0;
	}

	sdr_stage(sdr, NULL, fduObj, 0);
	memset((char *) &event, 0, sizeof(CfdpEvent));
	event.type = CfdpAbandonedInd;
	memcpy((char *) &event.transactionId, (char *) transactionId,
			sizeof(CfdpTransactionId));
	event.condition = condition;
	event.progress = fduBuf.progress;
	event.reqNbr = getReqNbr();
	if (enqueueCfdpEvent(&event) < 0)
	{
		putErrmsg("CFDP can't cancel transaction.", NULL);
		return -1;
	}

	destroyOutFdu(&fduBuf, fduObj, fduElt);
	return event.reqNbr;
}

void	destroyInFdu(InFdu *fdu, Object fduObj, Object fduElt)
{
	Sdr	sdr = getIonsdr();
	CfdpVdb	*cfdpvdb = _cfdpvdb(NULL);
	Object	elt;
	Object	obj;
		OBJ_POINTER(MsgToUser, msg);
		OBJ_POINTER(FilestoreRequest, req);

	CHKVOID(fdu);
	CHKVOID(fduObj);
	CHKVOID(fduElt);
	if (fdu->sourceFileName)
	{
		sdr_free(sdr, fdu->sourceFileName);
	}

	if (fdu->destFileName)
	{
		sdr_free(sdr, fdu->destFileName);
	}

	if (fdu->workingFileName && (fdu->workingFileName != fdu->destFileName))
	{
		sdr_free(sdr, fdu->workingFileName);
	}

	if (fdu->flowLabel)
	{
		sdr_free(sdr, fdu->flowLabel);
	}

	while (fdu->messagesToUser)
	{
		elt = sdr_list_first(sdr, fdu->messagesToUser);
		if (elt == 0)
		{
			sdr_list_destroy(sdr, fdu->messagesToUser, NULL,
					NULL);
			fdu->messagesToUser = 0;
			continue;
		}

		obj = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, MsgToUser, msg, obj);
		if (msg->text)
		{
			sdr_free(sdr, msg->text);
		}

		sdr_free(sdr, obj);
		sdr_list_delete(sdr, elt, NULL, NULL);
	}

	while (fdu->filestoreRequests)
	{
		elt = sdr_list_first(sdr, fdu->filestoreRequests);
		if (elt == 0)
		{
			sdr_list_destroy(sdr, fdu->filestoreRequests, NULL,
				       	NULL);
			fdu->filestoreRequests = 0;
			continue;
		}

		obj = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, FilestoreRequest, req, obj);
		if (req->firstFileName)
		{
			sdr_free(sdr, req->firstFileName);
		}

		if (req->secondFileName)
		{
			sdr_free(sdr, req->secondFileName);
		}

		sdr_free(sdr, obj);
		sdr_list_delete(sdr, elt, NULL, NULL);
	}

	while (fdu->extents)
	{
		elt = sdr_list_first(sdr, fdu->extents);
		if (elt == 0)
		{
			sdr_list_destroy(sdr, fdu->extents, NULL, NULL);
			fdu->extents = 0;
			continue;
		}

		obj = sdr_list_data(sdr, elt);
		sdr_free(sdr, obj);	/*	A CfdpExtent structure.	*/
		sdr_list_delete(sdr, elt, NULL, NULL);
	}

	sdr_free(sdr, fduObj);
	sdr_list_delete(sdr, fduElt, NULL, NULL);
	if (cfdpvdb->currentFdu == fduObj)
	{
		if (cfdpvdb->currentFile != -1)
		{
			close(cfdpvdb->currentFile);
			cfdpvdb->currentFile = -1;
		}

		cfdpvdb->currentFdu = 0;
	}
}

static int	abandonInFdu(CfdpTransactionId *transactionId,
			CfdpCondition condition)
{
	InFdu		fduBuf;
	Object		fduObj;
	Object		fduElt;
	Sdr		sdr = getIonsdr();
	CfdpEvent	event;
	char		fileName[256];

	fduObj = findInFdu(transactionId, &fduBuf, &fduElt, 0);
	if (fduObj == 0)
	{
		return 0;
	}

	sdr_stage(sdr, NULL, fduObj, 0);
	memset((char *) &event, 0, sizeof(CfdpEvent));
	event.type = CfdpAbandonedInd;
	memcpy((char *) &event.transactionId, (char *) transactionId,
			sizeof(CfdpTransactionId));
	event.condition = condition;
	event.progress = fduBuf.progress;
	event.reqNbr = getReqNbr();
	if (enqueueCfdpEvent(&event) < 0)
	{
		putErrmsg("CFDP can't abandon transaction.", NULL);
		return -1;
	}

	if (fduBuf.destFileName)
	{
		sdr_string_read(sdr, fileName, fduBuf.destFileName);
		unlink(fileName);
	}

	destroyInFdu(&fduBuf, fduObj, fduElt);
	return event.reqNbr;
}

static int	missingFileName(char *fileName, int parmNbr,
			FilestoreResponse *resp, char *msgBuf, int bufLen)
{
	if (fileName)
	{
		return 0;
	}
	
	resp->status = 1;
	isprintf(msgBuf, bufLen, "file name %d not provided", parmNbr);
	return 1;
}
			
static void	frCreateFile(char *firstFileName, char *secondFileName,
			FilestoreResponse *resp, char *msgBuf, int bufLen)
{
	int	fd;

	if (missingFileName(firstFileName, 1, resp, msgBuf, bufLen))
	{
		return;
	}

	fd = ifopen(firstFileName, O_CREAT, 0777);
	if (fd < 0)
	{
		resp->status = 1;
		isprintf(msgBuf, bufLen, "%.255s", system_error_msg());
	}
	else
	{
		close(fd);
	}
}

static void	frDeleteFile(char *firstFileName, char *secondFileName,
			FilestoreResponse *resp, char *msgBuf, int bufLen)
{
	if (missingFileName(firstFileName, 1, resp, msgBuf, bufLen))
	{
		return;
	}

	if (unlink(firstFileName) < 0)
	{
		resp->status = 1;
		isprintf(msgBuf, bufLen, "%.255s", system_error_msg());
	}
}

static void	frRenameFile(char *firstFileName, char *secondFileName,
			FilestoreResponse *resp, char *msgBuf, int bufLen)
{
	if (missingFileName(firstFileName, 1, resp, msgBuf, bufLen)
	|| missingFileName(secondFileName, 2, resp, msgBuf, bufLen))
	{
		return;
	}

	if (checkFile(firstFileName) != 1)
	{
		resp->status = 1;
		isprintf(msgBuf, bufLen, "%.255s", system_error_msg());
		return;
	}

	if (checkFile(secondFileName) == 1)
	{
		resp->status = 2;
		return;
	}

	if (rename(firstFileName, secondFileName) < 0)
	{
		resp->status = 3;
		isprintf(msgBuf, bufLen, "%.255s", system_error_msg());
	}
}

static void	frCopyFile(char *firstFileName, char *secondFileName,
			FilestoreResponse *resp, char *msgBuf, int bufLen,
			int flag)
{
	char	*buf;
	int	destFd;
	int	sourceFd;
	int	length;
	int	bytesWritten;
	int	writing = 1;

	if (missingFileName(firstFileName, 1, resp, msgBuf, bufLen)
	|| missingFileName(secondFileName, 2, resp, msgBuf, bufLen))
	{
		return;
	}

	if ((buf = MTAKE(10000)) == NULL)
	{
		resp->status = 3;
		istrcpy(msgBuf, "No space for buffer.", bufLen);
		return;
	}

	destFd = ifopen(firstFileName, O_WRONLY | flag, 0);
	if (destFd < 0)
	{
		MRELEASE(buf);
		resp->status = 3;
		isprintf(msgBuf, bufLen, "%.255s", system_error_msg());
		return;
	}

	sourceFd = ifopen(secondFileName, O_RDONLY, 0);
	if (sourceFd < 0)
	{
		close(destFd);
		MRELEASE(buf);
		resp->status = 3;
		isprintf(msgBuf, bufLen, "%.255s", system_error_msg());
		return;
	}

	while (writing)
	{
		length = read(sourceFd, buf, 10000);
		switch (length)
		{
		case -1:
			resp->status = 3;
			isprintf(msgBuf, bufLen, "%.255s", system_error_msg());
			writing = 0;
			break;			/*	Out of switch.	*/

		case 0:				/*	End of file.	*/
			writing = 0;
			break;			/*	Out of switch.	*/

		default:
			while (length > 0)
			{
				bytesWritten = write(destFd, buf, length);
				if (bytesWritten < 0)
				{
					resp->status = 3;
					isprintf(msgBuf, bufLen, "%.255s",
						system_error_msg());
					writing = 0;
					break;	/*	Out of loop.	*/
				}

				length -= bytesWritten;
			}
		}
	}

	close(sourceFd);
	close(destFd);
	MRELEASE(buf);
}

static void	frAppendFile(char *firstFileName, char *secondFileName,
			FilestoreResponse *resp, char *msgBuf, int bufLen)
{
	if (missingFileName(firstFileName, 1, resp, msgBuf, bufLen)
	|| missingFileName(secondFileName, 2, resp, msgBuf, bufLen))
	{
		return;
	}

	if (checkFile(firstFileName) != 1)
	{
		resp->status = 1;
		isprintf(msgBuf, bufLen, "%.255s", system_error_msg());
		return;
	}

	if (checkFile(secondFileName) != 1)
	{
		resp->status = 2;
		isprintf(msgBuf, bufLen, "%.255s", system_error_msg());
		return;
	}

	frCopyFile(firstFileName, secondFileName, resp, msgBuf, bufLen,
			O_APPEND);
}

static void	frReplaceFile(char *firstFileName, char *secondFileName,
			FilestoreResponse *resp, char *msgBuf, int bufLen)
{
	if (missingFileName(firstFileName, 1, resp, msgBuf, bufLen)
	|| missingFileName(secondFileName, 2, resp, msgBuf, bufLen))
	{
		return;
	}

	if (checkFile(firstFileName) != 1)
	{
		resp->status = 1;
		isprintf(msgBuf, bufLen, "%.255s", system_error_msg());
		return;
	}

	if (checkFile(secondFileName) != 1)
	{
		resp->status = 2;
		isprintf(msgBuf, bufLen, "%.255s", system_error_msg());
		return;
	}

	frCopyFile(firstFileName, secondFileName, resp, msgBuf, bufLen,
			O_TRUNC);
}

static void	frCreateDirectory(char *firstFileName, char *secondFileName,
			FilestoreResponse *resp, char *msgBuf, int bufLen)
{
	if (missingFileName(firstFileName, 1, resp, msgBuf, bufLen))
	{
		return;
	}

#if (defined(VXWORKS) || defined(mingw))
	if (mkdir(firstFileName) < 0)
#else
	if (mkdir(firstFileName, 0777) < 0)
#endif
	{
		resp->status = 1;
		isprintf(msgBuf, bufLen, "%.255s", system_error_msg());
	}
}

static void	frRemoveDirectory(char *firstFileName, char *secondFileName,
			FilestoreResponse *resp, char *msgBuf, int bufLen)
{
	if (missingFileName(firstFileName, 1, resp, msgBuf, bufLen))
	{
		return;
	}

	if (rmdir(firstFileName) < 0)
	{
		resp->status = 1;
		isprintf(msgBuf, bufLen, "%.255s", system_error_msg());
	}
}

static void	frDenyFile(char *firstFileName, char *secondFileName,
			FilestoreResponse *resp, char *msgBuf, int bufLen)
{
	if (missingFileName(firstFileName, 1, resp, msgBuf, bufLen))
	{
		return;
	}

	if (checkFile(firstFileName) != 1)
	{
		return;		/*	Nothing to delete.		*/
	}

	if (unlink(firstFileName) < 0)
	{
		resp->status = 1;
		isprintf(msgBuf, bufLen, "%.255s", system_error_msg());
	}
}

static void	frDenyDirectory(char *firstFileName, char *secondFileName,
			FilestoreResponse *resp, char *msgBuf, int bufLen)
{
	if (missingFileName(firstFileName, 1, resp, msgBuf, bufLen))
	{
		return;
	}

	if (checkFile(firstFileName) != 1)
	{
		return;		/*	Nothing to delete.		*/
	}

	if (rmdir(firstFileName) < 0)
	{
		resp->status = 1;
		isprintf(msgBuf, bufLen, "%.255s", system_error_msg());
	}
}

static int	executeFilestoreRequests(InFdu *fdu,
			MetadataList filestoreResponses)
{
	Sdr			sdr = getIonsdr();
	Object			elt;
				OBJ_POINTER(FilestoreRequest, req);
	char			firstFileNameBuf[256];
	char			*firstFileName;
	char			secondFileNameBuf[256];
	char			*secondFileName;
	Object			addr;
	FilestoreResponse	resp;
	char			msgBuf[256];
	int			reqAborted = 0;

	for (elt = sdr_list_first(sdr, fdu->filestoreRequests); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, FilestoreRequest, req,
				sdr_list_data(sdr, elt));
		if (req->firstFileName)
		{
			sdr_string_read(sdr, firstFileNameBuf,
					req->firstFileName);
			firstFileName = firstFileNameBuf;
		}
		else
		{
			firstFileName = NULL;
		}

		if (req->secondFileName)
		{
			sdr_string_read(sdr, secondFileNameBuf,
					req->secondFileName);
			secondFileName = secondFileNameBuf;
		}
		else
		{
			secondFileName = NULL;
		}

		addr = sdr_malloc(sdr, sizeof(FilestoreResponse));
		if (addr == 0
		|| sdr_list_insert_last(sdr, filestoreResponses, addr) == 0)
		{
			putErrmsg("Can't create filestore response.", NULL);
			return -1;
		}

		memset((char *) &resp, 0, sizeof(FilestoreResponse));
		resp.action = req->action;
		if (firstFileName)
		{
			resp.firstFileName = sdr_string_create(sdr,
					firstFileName);
			if (resp.firstFileName == 0)
			{
				putErrmsg("Can't write 1st file name.", NULL);
				return -1;
			}
		}

		if (secondFileName)
		{
			resp.secondFileName = sdr_string_create(sdr,
					secondFileName);
			if (resp.secondFileName == 0)
			{
				putErrmsg("Can't write 2nd file name.", NULL);
				return -1;
			}
		}

		if (reqAborted)	/*	All remaining requests fail.	*/
		{
			resp.status = 15;
			sdr_write(sdr, addr, (char *) &resp,
					sizeof(FilestoreResponse));
			continue;
		}

		msgBuf[0] = '\0';
		switch (req->action)
		{
		case CfdpCreateFile:
			frCreateFile(firstFileName, secondFileName, &resp,
					msgBuf, sizeof msgBuf);
			break;

		case CfdpDeleteFile:
			frDeleteFile(firstFileName, secondFileName, &resp,
					msgBuf, sizeof msgBuf);
			break;

		case CfdpRenameFile:
			frRenameFile(firstFileName, secondFileName, &resp,
					msgBuf, sizeof msgBuf);
			break;

		case CfdpAppendFile:
			frAppendFile(firstFileName, secondFileName, &resp,
					msgBuf, sizeof msgBuf);
			break;

		case CfdpReplaceFile:
			frReplaceFile(firstFileName, secondFileName, &resp,
					msgBuf, sizeof msgBuf);
			break;

		case CfdpCreateDirectory:
			frCreateDirectory(firstFileName, secondFileName, &resp,
					msgBuf, sizeof msgBuf);
			break;

		case CfdpRemoveDirectory:
			frRemoveDirectory(firstFileName, secondFileName, &resp,
					msgBuf, sizeof msgBuf);
			break;

		case CfdpDenyFile:
			frDenyFile(firstFileName, secondFileName, &resp,
					msgBuf, sizeof msgBuf);
			break;

		case CfdpDenyDirectory:
			frDenyDirectory(firstFileName, secondFileName, &resp,
					msgBuf, sizeof msgBuf);
			break;

		default:
			resp.status = 15;
			istrcpy(msgBuf, "Invalid action code.", sizeof msgBuf);
		}

		if (resp.status != 0)	/*	Request failed.		*/
		{
			reqAborted = 1;
			if (strlen(msgBuf) > 0)
			{
				resp.message = sdr_string_create(sdr,
						msgBuf);
				if (resp.message == 0)
				{
					putErrmsg("Can't write messge.", NULL);
					return -1;
				}
			}
		}

		sdr_write(sdr, addr, (char *) &resp,
				sizeof(FilestoreResponse));
	}

	return 0;
}

static int	getQualifiedFileName(char *pathNameBuf, int bufLen,
			char *fileName)
{
	char	*wdname;
	int	wdnameLen;
	int	filenameLen;

	if (fullyQualified(fileName))
	{
		istrcpy(pathNameBuf, fileName, bufLen);
	}
	else
	{
		/*	ION working directory is the location for all
		 *	received files for which destination path name
		 *	is not absolute.				*/

		wdname = getIonWorkingDirectory();
		istrcpy(pathNameBuf, wdname, bufLen);
		wdnameLen = strlen(pathNameBuf);
		filenameLen = strlen(fileName);
		if ((wdnameLen + 1 + filenameLen + 1) > bufLen)
		{
			*pathNameBuf = '\0';
			return 0;	/*	Too long.		*/
		}
 
		*(pathNameBuf + wdnameLen) = ION_PATH_DELIMITER;
		wdnameLen++;	/*	wdnamelen including delimiter	*/
		istrcpy(pathNameBuf + wdnameLen, fileName, bufLen - wdnameLen);
	}

#if (!(defined(VXWORKS) || defined(mingw)))
	/*	Create (as necessary) the directory in which the file
	 *	is to be created.
	 *
	 *	Per Josh Schoolcraft, 23 June 2011.
	 *
	 *	Given the destination file's qualified pathname, try
	 *	to make sure the directory in which the file is to be
	 *	created exists.
	 *
	 *	Each directory on the path that doesn't already exist
	 *	is created: at each level of the directory tree a
	 *	qualified name is used to create the directory that
	 *	is needed at that level.
	 *
	 *	Note that the length of the path name is known not
	 *	to be zero: if no Metadata yet, or if the destination
	 *	file name in the Metadata PDU was of length zero,
	 *	then getFileName() made up a name based on the FDU's
	 *	transaction ID because fdu->workingFileName was zero.	*/

	size_t	pathNameLen;
	char	*cursor;
	char	*lastPathSeparator = NULL;
 
	pathNameLen = istrlen(pathNameBuf, bufLen);
	if (pathNameLen > MAXPATHLEN)		/*	Too long.	*/
	{
		return 0;	/*	Can't create the directory.	*/
	}

	/*	Temporarily strip off the unqualified file name, i.e.,
	 *	everything after and including the last path separator
	 *	character.						*/

	cursor = pathNameBuf + pathNameLen;	/*	Terminal NULL.	*/
	while (cursor > pathNameBuf)
	{
		if (*cursor == ION_PATH_DELIMITER)
		{
			lastPathSeparator = cursor;
			*cursor = 0;	/*	Delimit at file name.	*/
			break;
		}

		cursor--;
	}

	/*	Now create directories along the path as necessary.
	 *	Wherever a path name separator is found, we change
	 *	it to NULL, create a directory using all qualification
	 *	to that point, and then restore the separator to
	 *	enable creation of the next directory in the path.
	 *
	 *	We skip over the first byte of the path name: if
	 *	it's not a path name separator then we wouldn't act
	 *	on it anyway, and if it is then we want to ignore
	 *	it since we'd never create a top-level directory
	 *	with a name of length zero.				*/

	for (cursor = pathNameBuf + 1; *cursor; cursor++)
	{
		if (*cursor == ION_PATH_DELIMITER)
		{
		        *cursor = 0;	/*	Momentarily...		*/
			if (mkdir(pathNameBuf, 0777) < 0)
			{
				if (errno != EEXIST)
				{
					putSysErrmsg("Can't create directory.",
							pathNameBuf);
                			*cursor = ION_PATH_DELIMITER;
					if (lastPathSeparator)
					{
						*lastPathSeparator
							= ION_PATH_DELIMITER;
					}

					return 0;
				}

				/*	If directory already exists,
				 *	no problem.			*/
			}

                	*cursor = ION_PATH_DELIMITER;
		}
	}

	/*	Now create the destination directory itself.		*/

	if (mkdir(pathNameBuf, 0777) < 0)
	{
		if (errno != EEXIST)
		{
			putSysErrmsg("Can't create directory.", pathNameBuf);
			if (lastPathSeparator)
			{
				*lastPathSeparator = ION_PATH_DELIMITER;
			}

			return 0;
		}

		/*	If directory already exists, no problem.	*/
	}

	/*	And restore the original qualified path name.		*/

	if (lastPathSeparator)
	{
		*lastPathSeparator = ION_PATH_DELIMITER;
	}
#endif
	return 1;
}

static int	relocateFile(char *fromPath, char *toPath)
{
	icopy(fromPath, toPath);
	if (checkFile(toPath) != 1)	/*	Copy failed.		*/
	{
		return -1;
	}

	unlink(fromPath);
	return 0;
}

static void	renameWorkingFile(InFdu *fduBuf)
{
	Sdr	sdr = getIonsdr();
	char	workingFileName[SDRSTRING_BUFSZ];
	char	destFileName[SDRSTRING_BUFSZ];
	char	renameErrBuffer[600];

	sdr_string_read(sdr, workingFileName, fduBuf->workingFileName);
	sdr_string_read(sdr, destFileName, fduBuf->destFileName);
	if (rename(workingFileName, destFileName) < 0)
	{
		if (relocateFile(workingFileName, destFileName) < 0)
		{
			isprintf(renameErrBuffer, sizeof renameErrBuffer,
					"CFDP can't relocate '%s' to '%s'",
					workingFileName, destFileName);
			putSysErrmsg(renameErrBuffer, NULL);
		}
	}
}

static int	constructFinishPdu(InFdu *fdu, CfdpEvent *event)
{
	static unsigned char	fpduBuf[CFDP_MAX_PDU_SIZE];
	Sdr			sdr = getIonsdr();
	CfdpDB			*db = getCfdpConstants();
	CfdpVdb			*vdb = _cfdpvdb(NULL);
	unsigned char		*cursor;
	unsigned int		fpduLength = 0;
	CfdpCondition		condition;
	size_t			length;
	Object			elt;
	Object			obj;
				OBJ_POINTER(FilestoreResponse, resp);
	int			firstFileNameLen;
	char			firstFileName[256];
	int			secondFileNameLen;
	char			secondFileName[256];
	int			filestoreMsgLen;
	char			filestoreMsg[256];
	FinishPdu		fpdu;

	cursor = fpduBuf;

	/*	Insert directive code.					*/

	*cursor = 5;		/*	Finish PDU.			*/
	cursor++;
	fpduLength++;

	/*	Note condition, delivery, file status.			*/

	condition = event->condition;
	if (condition == CfdpNoError && fdu->finishCondition != condition)
	{
		condition = fdu->finishCondition;
	}

	*cursor = ((condition & 0x0f) << 4) 
			+ ((event->deliveryCode & 0x01) << 2)
			+ (event->fileStatus & 0x03);
	cursor++;
	fpduLength++;

	/*	Note filestore response TLVs.				*/

	if (event->filestoreResponses != 0)
	{
		/*	Copy all responses in list.			*/

		for (elt = sdr_list_first(sdr, event->filestoreResponses); elt;
				elt = sdr_list_next(sdr, elt))
		{
			obj = sdr_list_data(sdr, elt);
			GET_OBJ_POINTER(sdr, FilestoreResponse, resp, obj);
			firstFileNameLen = 0;
			firstFileName[0] = '\0';
			if (resp->firstFileName)
			{
				firstFileNameLen = sdr_string_read(sdr,
					firstFileName, resp->firstFileName);
				if (firstFileNameLen < 0)
				{
					putErrmsg("Missing sdrstring.", NULL);
					break;
				}
			}

			secondFileNameLen = 0;
			secondFileName[0] = '\0';
			if (resp->secondFileName)
			{
				secondFileNameLen = sdr_string_read(sdr,
					secondFileName, resp->secondFileName);
				if (secondFileNameLen < 0)
				{
					putErrmsg("Missing sdrstring.", NULL);
					break;
				}
			}

			filestoreMsgLen = 0;
			filestoreMsg[0] = '\0';
			if (resp->message)
			{
				filestoreMsgLen = sdr_string_read(sdr,
					filestoreMsg, resp->message);
				if (filestoreMsgLen < 0)
				{
					putErrmsg("Missing sdrstring.", NULL);
					break;
				}
			}

			/*	Append to metadata if possible.		*/

			length = 1 + 1 + firstFileNameLen
					+ 1 + secondFileNameLen
					+ 1 + filestoreMsgLen;
			if (length > 255)
			{
				putErrmsg("Filestore request too long.",
						itoa(length));
				break;
			}

			if (fpduLength + 1 + 1 + length > CFDP_MAX_PDU_SIZE)
			{
				putErrmsg("Finish PDU too long.", itoa(length));
				break;
			}

			/*		Type.				*/

			*cursor = 0x01;
			cursor++;
			fpduLength++;

			/*		Length.				*/

			*cursor = length;
			cursor++;
			fpduLength++;

			/*		Value.				*/

			*cursor = (((int) (resp->action)) << 4)
					+ (resp->status & 0x0f);
			cursor++;
			fpduLength++;

			*cursor = firstFileNameLen;
			cursor++;
			fpduLength++;

			memcpy(cursor, firstFileName, firstFileNameLen);
			cursor += firstFileNameLen;
			fpduLength += firstFileNameLen;

			*cursor = secondFileNameLen;
			cursor++;
			fpduLength++;

			memcpy(cursor, secondFileName, secondFileNameLen);
			cursor += secondFileNameLen;
			fpduLength += secondFileNameLen;

			*cursor = filestoreMsgLen;
			cursor++;
			fpduLength++;

			memcpy(cursor, filestoreMsg, filestoreMsgLen);
			cursor += filestoreMsgLen;
			fpduLength += filestoreMsgLen;
		}
	}
#if 0
	/*	Note fault location TLV.				*/

	if (event->condition != CfdpNoError)
	{
		length = db->ownEntityNbr.length;
		if (fpduLength + 1 + 1 + length > CFDP_MAX_PDU_SIZE)
		{
			putErrmsg("Finish PDU too long.", itoa(length));
		}
		else
		{
			*cursor = 0x06;			/*	Type.	*/
			cursor++;
			fpduLength++;

			*cursor = length;
			cursor++;
			fpduLength++;

			memcpy(cursor, db->ownEntityNbr.buffer + (8 - length),
					length);
			cursor += length;
			fpduLength += length;
		}
	}
#endif
	/*	Record the Finish PDU to the database for transmission.	*/

	fpdu.pdu = sdr_malloc(sdr, fpduLength);
	if (fpdu.pdu == 0)
	{
		putErrmsg("Can't construct Finish PDU.", NULL);
		return -1;
	}

	sdr_write(sdr, fpdu.pdu, (char *) fpduBuf, fpduLength);
	fpdu.length = fpduLength;

	/*	Post the FinishPdu object for transmission.		*/

	fpdu.largeFile = 0;	/*	No offset or file size in fpdu.	*/
	fpdu.entityNbrLength = event->transactionId.sourceEntityNbr.length;
	fpdu.transactionNbrLength = event->transactionId.transactionNbr.length;
	memcpy((char *) &fpdu.transactionId, (char *) &(event->transactionId),
			sizeof(CfdpTransactionId));
	obj = sdr_malloc(sdr, sizeof(FinishPdu));
	if (obj == 0
	|| sdr_list_insert_last(sdr, db->finishPdus, obj) == 0)
	{
		putErrmsg("Can't post Finish PDU.", NULL);
		return -1;
	}

	sdr_write(sdr, obj, (char *) &fpdu, sizeof(FinishPdu));
	sm_SemGive(vdb->fduSemaphore);
	return 0;
}

int	completeInFdu(InFdu *fduBuf, Object fduObj, Object fduElt,
		CfdpCondition condition, int reqNbr)
{
	Sdr		sdr = getIonsdr();
	CfdpDB		*db = getCfdpConstants();
	CfdpEvent	event;
	char		workingFileName[256];
	char		reportBuffer[256];

	CHKERR(ionLocked());
	CHKERR(fduBuf);
	CHKERR(fduObj);
	CHKERR(fduElt);
	memset((char *) &event, 0, sizeof(CfdpEvent));
	event.type = CfdpTransactionFinishedInd;
	memcpy((char *) &event.transactionId, (char *) &fduBuf->transactionId,
			sizeof(CfdpTransactionId));
	event.condition = condition;
	event.reqNbr = reqNbr;
	switch (condition)
	{
	case CfdpNoError:
		if (fduBuf->destFileName == 0)
		{
			event.fileStatus = CfdpFileStatusUnreported;
		}
		else
		{
			event.fileStatus = CfdpFileRetained;
			if (fduBuf->workingFileName != fduBuf->destFileName)
			{
				renameWorkingFile(fduBuf);
			}
		}

		event.deliveryCode = CfdpDataComplete;
		event.filestoreResponses = createMetadataList(db->fsrespLists);
		if (event.filestoreResponses == 0)
		{
			putErrmsg("CFDP can't record filestore responses.",
					NULL);
			return -1;
		}

		if (executeFilestoreRequests(fduBuf,
				event.filestoreResponses) < 0)
		{
			putErrmsg("CFDP can't execute filestore requests.",
					NULL);
			return -1;
		}

		break;

	case CfdpFilestoreRejection:
		event.fileStatus = CfdpFileRejected;
		event.deliveryCode = CfdpDataIncomplete;
		break;

	default:
		if (fduBuf->destFileName == 0)
		{
			event.fileStatus = CfdpFileStatusUnreported;
			if (fduBuf->checksumVerified)
			{
				event.deliveryCode = CfdpDataComplete;
			}
			else
			{
				event.deliveryCode = CfdpDataIncomplete;
			}
		}
		else
		{
			if (db->discardIncompleteFile)
			{
				sdr_string_read(sdr, workingFileName,
						fduBuf->workingFileName);
				unlink(workingFileName);
				event.fileStatus = CfdpFileDiscarded;
				event.deliveryCode = CfdpDataIncomplete;
			}
			else
			{
				event.fileStatus = CfdpFileRetained;
				if (fduBuf->workingFileName !=
						fduBuf->destFileName)
				{
					renameWorkingFile(fduBuf);
				}

				if (fduBuf->checksumVerified)
				{
					event.deliveryCode = CfdpDataComplete;
				}
				else
				{
					event.deliveryCode =
							CfdpDataIncomplete;
				}
			}
		}
	}

	isprintf(reportBuffer, sizeof reportBuffer, "bytesReceived %u  size \
%u  progress %u", fduBuf->bytesReceived, fduBuf->fileSize, fduBuf->progress);
	event.statusReport = sdr_string_create(sdr, reportBuffer);
	event.reqNbr = getReqNbr();
	if (enqueueCfdpEvent(&event) < 0)
	{
		putErrmsg("CFDP can't complete inbound transaction.", NULL);
		return -1;
	}

	if (fduBuf->closureRequested)
	{
		if (constructFinishPdu(fduBuf, &event) < 0)
		{
			putErrmsg("CFDP can't close transaction.", NULL);
			return -1;
		}
	}

	destroyInFdu(fduBuf, fduObj, fduElt);
	return event.reqNbr;
}

/*	*	*	Service interface functions	*	*	*/

int	enqueueCfdpEvent(CfdpEvent *event)
{
	Sdr	sdr = getIonsdr();
	CfdpVdb	*cfdpvdb = _cfdpvdb(NULL);
	Object	eventObj;
	CfdpDB	cfdpdb;
	Object	elt;

	CHKERR(ionLocked());
	CHKERR(event);
	eventObj = sdr_malloc(sdr, sizeof(CfdpEvent));
	if (eventObj == 0)
	{
		putErrmsg("Can't create CFDP event.", NULL);
		return -1;
	}

	sdr_read(sdr, (char *) &cfdpdb, getCfdpDbObject(), sizeof(CfdpDB));
	if (sdr_list_insert_last(sdr, cfdpdb.events, eventObj) == 0)
	{
		putErrmsg("Can't enqueue CFDP event.", NULL);
		return -1;
	}

	sdr_write(sdr, eventObj, (char *) event, sizeof(CfdpEvent));

	/*	Discard unread events as necessary.			*/

	while (sdr_list_length(sdr, cfdpdb.events) > cfdpdb.maxQueuedEvents)
	{
		elt = sdr_list_first(sdr, cfdpdb.events);
		sdr_free(sdr, sdr_list_data(sdr, elt));
		sdr_list_delete(sdr, elt, NULL, NULL);
	}

	/*	Tell user application that an event is waiting.		*/

	sm_SemGive(cfdpvdb->eventSemaphore);
	return 0;
}

int	handleFault(CfdpTransactionId *transactionId, CfdpCondition fault,
		CfdpHandler *handler)
{
	Sdr		sdr = getIonsdr();
	CfdpDB		cfdpdb;
	Object		fduObj;
	InFdu		inFdu;
	OutFdu		outFdu;
	Object		fduElt;
	CfdpEvent	event;

	CHKERR(transactionId);
	CHKERR(handler);
	*handler = CfdpNoHandler;
	sdr_read(sdr, (char *) &cfdpdb, getCfdpDbObject(), sizeof(CfdpDB));
	if (memcmp(transactionId->sourceEntityNbr.buffer,
			cfdpdb.ownEntityNbr.buffer, 8) == 0)
	{
		memset((char *) &outFdu, 0, sizeof(OutFdu));
		fduObj = findOutFdu(transactionId, &outFdu, &fduElt);
		if (fduObj != 0)
		{
			*handler = outFdu.faultHandlers[fault];
		}
	}
	else
	{
		memset((char *) &inFdu, 0, sizeof(InFdu));
		fduObj = findInFdu(transactionId, &inFdu, &fduElt, 0);
		if (fduObj != 0)
		{
			*handler = inFdu.faultHandlers[fault];
		}
	}

	if (*handler == CfdpNoHandler)
	{
		*handler = cfdpdb.faultHandlers[fault];
	}

	switch (*handler)
	{
	case CfdpCancel:
		if (fduObj == 0)
		{
			return 0;
		}

		if (memcmp(transactionId->sourceEntityNbr.buffer,
				cfdpdb.ownEntityNbr.buffer, 8) == 0)
		{
			return cancelOutFdu(transactionId, fault, 0);
		}

		return completeInFdu(&inFdu, fduObj, fduElt, fault, 0);

	case CfdpSuspend:
		if (fduObj == 0)
		{
			return 0;
		}

		if (memcmp(transactionId->sourceEntityNbr.buffer,
				cfdpdb.ownEntityNbr.buffer, 8) == 0)
		{
			return suspendOutFdu(transactionId, fault, 0);
		}

		return 0;	/*	Per 4.1.11.3.1.2.		*/

	case CfdpIgnore:
		memset((char *) &event, 0, sizeof(CfdpEvent));
		event.type = CfdpFaultInd;
		memcpy((char *) &event.transactionId, (char *) transactionId,
				sizeof(CfdpTransactionId));
		event.condition = fault;
		if (memcmp(transactionId->sourceEntityNbr.buffer,
				cfdpdb.ownEntityNbr.buffer, 8) == 0)
		{
			event.progress = outFdu.progress;
		}
		else
		{
			event.progress = inFdu.progress;
		}

		event.reqNbr = 0;
		if (enqueueCfdpEvent(&event) < 0)
		{
			putErrmsg("Can't post Fault indication.", NULL);
			return -1;
		}

		return 0;

	case CfdpAbandon:
		if (fduObj == 0)
		{
			return 0;
		}

		if (memcmp(transactionId->sourceEntityNbr.buffer,
				cfdpdb.ownEntityNbr.buffer, 8) == 0)
		{
			return abandonOutFdu(transactionId, fault);
		}

		return abandonInFdu(transactionId, fault);

	default:
		return 0;
	}
}

/*	*	*	PDU issuance functions	*	*	*	*/

static Object	selectOutFdu(CfdpDB *cfdpdb, OutFdu *buffer)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
	Object	obj;

	for (elt = sdr_list_first(sdr, cfdpdb->outboundFdus); elt;
			elt = sdr_list_next(sdr, elt))
	{
		obj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) buffer, obj, sizeof(OutFdu));
		if (buffer->state != FduActive
		|| buffer->eofPdu == 0	/*	Nothing left to send.	*/)
		{
			continue;
		}

		return obj;
	}

	return 0;
}

static int	selectFduPdu(OutFdu *fdu, Object *pdu, int *pduIsFileData,
			int *haveMetadata)
{
	Sdr		sdr = getIonsdr();
	vast		length;
	Object		elt;
	Object		pduObj;
			OBJ_POINTER(FileDataPdu, seg);
	unsigned int	headerLength;
	unsigned char	headerBuf[72];
	unsigned char	*headerCursor;
	uvast		largeOffset;
	unsigned int	smallOffset;
	Object		header;

	if (fdu->metadataPdu)
	{
		length = fdu->mpduLength;

		/*	Pass additive inverse of length to zco_create
		 *	to note that space has already been awarded.	*/

		*pdu = zco_create(sdr, ZcoSdrSource, fdu->metadataPdu, 0,
				0 - length, ZcoOutbound);
		switch (*pdu)
		{
		case (Object) ERROR:
		case 0:
			putErrmsg("Can't create Metadata PDU ZCO.", NULL);
			return -1;

		default:
			break;
		}

		fdu->metadataPdu = 0;
		*pduIsFileData = 0;
		return 0;
	}

	if (fdu->fileSize > 0)
	{
		if (fdu->fileRef == 0)
		{
			fdu->fileRef = zco_create_file_ref(sdr,
					fdu->sourceFileName, NULL, ZcoOutbound);
			if (fdu->fileRef == 0)
			{
				putErrmsg("No space for file ZCO ref.", NULL);
				return -1;
			}
		}

		elt = sdr_list_first(sdr, fdu->fileDataPdus);
		if (elt)
		{
			pduObj = sdr_list_data(sdr, elt);
			GET_OBJ_POINTER(sdr, FileDataPdu, seg, pduObj);
			headerCursor = headerBuf;
			headerLength = 0;
			if (fdu->recordBoundsRespected
			|| seg->metadataLength > 0)
			{
				*haveMetadata = 1;
				*headerCursor =
					(((int) (seg->continuationState)) << 6)
					+ (seg->metadataLength & 0x3f);
				headerLength++;
				headerCursor++;
				if (seg->metadataLength > 0)
				{
					sdr_read(sdr, (char *) (headerCursor),
							seg->metadata,
							seg->metadataLength);
					headerLength += seg->metadataLength;
					headerCursor += seg->metadataLength;
				}
			}
			else
			{
				*haveMetadata = 0;
			}

			if (fdu->largeFile)
			{
				largeOffset = seg->offset;
				largeOffset = htonv(largeOffset);
				memcpy(headerCursor, (char *) &largeOffset, 8);
				headerLength += 8;
			}
			else
			{
				smallOffset = seg->offset;
				smallOffset = htonl(smallOffset);
				memcpy(headerCursor, (char *) &smallOffset, 4);
				headerLength += 4;
			}

			header = sdr_malloc(sdr, headerLength);
			if (header == 0)
			{
				putErrmsg("No space for file PDU hdr.", NULL);
				return -1;
			}

			sdr_write(sdr, header, (char *) headerBuf,
					headerLength);
			length = headerLength;

			/*	Pass additive inverse of length to
			 *	zco_create to note that space has
			 *	already been awarded.			*/

			*pdu = zco_create(sdr, ZcoSdrSource, header, 0,
					0 - length, ZcoOutbound);
			switch (*pdu)
			{
			case (Object) ERROR:
			case 0:
				putErrmsg("Can't create data PDU ZCO.", NULL);
				return -1;

			default:
				break;
			}

			length = seg->length;

			/*	Pass additive inverse of length to
			 *	zco_append_extent to note that space
			 *	has already been awarded.		*/

			if (zco_append_extent(sdr, *pdu, ZcoFileSource,
				fdu->fileRef, seg->offset, 0 - length) < 0)
			{
				putErrmsg("Can't append extent.", NULL);
				return -1;
			}

			fdu->progress += seg->length;
			sdr_free(sdr, pduObj);
			sdr_list_delete(sdr, elt, NULL, NULL);
			*pduIsFileData = 1;
			return 0;
		}
	}

	length = fdu->epduLength;

	/*	Pass additive inverse of length to zco_create to note
	 *	that space has already been awarded.			*/

	*pdu = zco_create(sdr, ZcoSdrSource, fdu->eofPdu, 0, 0 - length,
			ZcoOutbound);
	switch (*pdu)
	{
	case (Object) ERROR:
	case 0:
		putErrmsg("Can't create EOF PDU ZCO.", NULL);
		return -1;

	default:
		break;
	}

	fdu->eofPdu = 0;
	*pduIsFileData = 0;
	return 0;
}

static int	selectOutPdu(CfdpDB *db, Object *pdu, Object *fdu,
			OutFdu *fduBuffer, FinishPdu *fpdu, int *direction,
		       	int *pduIsFileData, int *haveMetadata)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
	Object	obj;
	vast	length;

	elt = sdr_list_first(sdr, db->finishPdus);
	if (elt)	/*	Have got a Finished PDU to send.	*/
	{
		obj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) fpdu, obj, sizeof(FinishPdu));
		length = fpdu->length;

		/*	Pass additive inverse of length to zco_create
		 *	to note that space has already been awarded.	*/

		*pdu = zco_create(sdr, ZcoSdrSource, fpdu->pdu, 0,
				0 - length, ZcoOutbound);
		switch (*pdu)
		{
		case (Object) ERROR:
		case 0:
			putErrmsg("Can't create Finish PDU ZCO.", NULL);
			return -1;

		default:
			break;
		}

		sdr_free(sdr, obj);
		sdr_list_delete(sdr, elt, NULL, NULL);
		*direction = 1;		/*	Toward source.		*/
		return 0;
	}

	/*	Look for a forward PDU.					*/

	*direction = 0;			/*	Toward destination.	*/
	*fdu = selectOutFdu(db, fduBuffer);
	if (*fdu)
	{
		sdr_stage(sdr, NULL, *fdu, 0);
		if (selectFduPdu(fduBuffer, pdu, pduIsFileData, haveMetadata))
		{
			putErrmsg("UTO failed getting getting outbound PDU.",
					NULL);
			return -1;
		}
	}

	return 0;
}

int	cfdpDequeueOutboundPdu(Object *pdu, OutFdu *fduBuffer, FinishPdu *fpdu,
		int *direction)
{
	Sdr			sdr = getIonsdr();
	CfdpVdb			*cfdpvdb = _cfdpvdb(NULL);
	CfdpDB			cfdpdb;
	ReqTicket		ticket;
	Object			fdu = 0;
	int			pduIsFileData = 0;	/*	Boolean.*/
	int			haveMetadata = 0;	/*	Boolean.*/
	int			recordStructure = 0;	/*	Boolean.*/
	int			crcRequired = 0;	/*	Boolean.*/
	CfdpTransactionId	transactionId;
	CfdpNumber		destinationEntity;
	int			largeFile;
	int			entityNbrLength;
	int			transactionNbrLength;
	unsigned int		octet;
	int			entityNbrPad;
	int			transactionNbrPad;
	unsigned int		dataFieldLength;
	int			pduSourceDataLength;
	unsigned char		pduHeader[28];
	unsigned int		pduHeaderLength = 4;
	unsigned int		proposedLength;
	unsigned char		*buf;
	ZcoReader		reader;
	unsigned short		crc;

	CHKERR(pdu);
	CHKERR(fduBuffer);
	*pdu = 0;
	sdr_read(sdr, (char *) &cfdpdb, getCfdpDbObject(), sizeof(CfdpDB));
	crcRequired = cfdpdb.crcRequired;

	/*	Reserve one PDU's worth of ZCO space.  Assume file
	 *	data segment size limit is maximum PDU size; reserve
	 *	it in both file and heap, because we don't know which
	 *	will be needed.  (Only one or the other will actually
	 *	be allocated, when the ZCO is finally created.)		*/

	if (ionRequestZcoSpace(ZcoOutbound, cfdpdb.maxFileDataLength, 0,
			cfdpdb.maxFileDataLength, 1, 0, &(cfdpvdb->attendant),
			&ticket) < 0)
	{
		putErrmsg("Failed trying to reserve ZCO space.", NULL);
		return -1;
	}

	if (!(ionSpaceAwarded(ticket)))
	{
		/*	Space is not currently available.		*/

		if (sm_SemTake(cfdpvdb->attendant.semaphore) < 0)
		{
			putErrmsg("Failed taking semaphore.", NULL);
			ionShred(ticket);	/*	Cancel request.	*/
			return -1;
		}

		if (sm_SemEnded(cfdpvdb->attendant.semaphore))
		{
			writeMemo("[i] CFDP UTO ZCO request interrupted.");
			ionShred(ticket);	/*	Cancel request.	*/
			return -1;
		}

		/*	ZCO space has now been reserved.		*/
	}

	/*	At this point it is known that there's sufficient
	 *	ZCO space for the next outbound PDU, so create it.	*/

	CHKERR(sdr_begin_xn(sdr));
	if (selectOutPdu(&cfdpdb, pdu, &fdu, fduBuffer, fpdu, direction,
			&pduIsFileData, &haveMetadata) < 0)
	{
		putErrmsg("UTO can't get outbound PDU.", NULL);
		sdr_cancel_xn(sdr);
		ionShred(ticket);		/*	Cancel request.	*/
		return -1;
	}

	ionShred(ticket);	/*	Dismiss reservation.		*/
	while (*pdu == 0)
	{
		sdr_exit_xn(sdr);

		/*	Wait until an FDU is resumed or a new one is
		 *	created or a Finished PDU is issued.		*/

		if (sm_SemTake(cfdpvdb->fduSemaphore) < 0)
		{
			putErrmsg("UTO can't take FDU semaphore.",
					NULL);
			return -1;
		}

		if (sm_SemEnded(cfdpvdb->fduSemaphore))
		{
			writeMemo("[i] UTO has been stopped.");
			return -1;
		}

		CHKERR(sdr_begin_xn(sdr));
		if (selectOutPdu(&cfdpdb, pdu, &fdu, fduBuffer, fpdu, direction,
				&pduIsFileData, &haveMetadata) < 0)
		{
			putErrmsg("UTO can't get outbound PDU.", NULL);
			sdr_cancel_xn(sdr);
			return -1;
		}
	}

	/*	Have now selected a PDU to transmit.			*/

	if (*direction == 1)	/*	Finished PDU.			*/
	{
		largeFile = fpdu->largeFile;
		entityNbrLength = fpdu->entityNbrLength;
		transactionNbrLength = fpdu->transactionNbrLength;
		memcpy((char *) &transactionId, (char *) &(fpdu->transactionId),
				sizeof(CfdpTransactionId));
		memcpy((char *) &destinationEntity,
				(char *) &cfdpdb.ownEntityNbr,
				sizeof(CfdpNumber));
	}
	else			/*	A forward PDU.			*/
	{
		sdr_write(sdr, fdu, (char *) fduBuffer, sizeof(OutFdu));
		largeFile = fduBuffer->largeFile;
		entityNbrLength = cfdpdb.ownEntityNbr.length;
		if (fduBuffer->destinationEntityNbr.length > entityNbrLength)
		{
			entityNbrLength =
				fduBuffer->destinationEntityNbr.length;
		}

		transactionNbrLength =
			fduBuffer->transactionId.transactionNbr.length;
		recordStructure = fduBuffer->recordBoundsRespected;
		memcpy((char *) &transactionId,
				(char *) &(fduBuffer->transactionId),
				sizeof(CfdpTransactionId));
		memcpy((char *) &destinationEntity,
				(char *) &(fduBuffer->destinationEntityNbr),
				sizeof(CfdpNumber));
	}

	octet = (1 << 5)			/*	Version 001.	*/
			+ (pduIsFileData << 4)	/*	bit 3: PDU type	*/
			+ ((*direction) << 3)	/*	bit 4: <-->	*/
			+ 4			/*	bit 5: unack	*/
			+ (crcRequired ? 2 : 0)	/*	6: CRC?		*/
			+ (largeFile & 0x01);	/*	7: 64-bit vals? */
	pduHeader[0] = octet;
	pduSourceDataLength = zco_length(sdr, *pdu);
	dataFieldLength = pduSourceDataLength + (crcRequired ? 2 : 0);

	/*	Note that length of CRC, if present, is included in
	 *	the data field length per 4.1.1.3.2.			*/

	pduHeader[1] = (dataFieldLength >> 8) & 0xff;
	pduHeader[2] = dataFieldLength & 0xff;

	/*	Compute the lengths byte value.				*/

	entityNbrPad = 8 - entityNbrLength;
	transactionNbrPad = 8 - transactionNbrLength;
	octet = (recordStructure ? 128 : 0)		/*	bit 0	*/
			+ ((entityNbrLength - 1) << 4)	/*	bit 1-3	*/
			+ (haveMetadata ? 8 : 0)	/*	bit 4	*/
			+ (transactionNbrLength - 1);	/*	bit 5-7	*/
	pduHeader[3] = octet;

	/*	Insert entity IDs and transaction number.		*/

	proposedLength = pduHeaderLength + entityNbrLength
			+ transactionNbrLength + entityNbrLength;
	if (proposedLength > sizeof pduHeader)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("pduHeaderLength too large.", utoa(proposedLength));
		return -1;
	}

	memcpy(pduHeader + pduHeaderLength, transactionId.sourceEntityNbr.buffer
			+ entityNbrPad, entityNbrLength);
	pduHeaderLength += entityNbrLength;
	memcpy(pduHeader + pduHeaderLength,
			transactionId.transactionNbr.buffer + transactionNbrPad,
			transactionNbrLength);
	pduHeaderLength += transactionNbrLength;
	memcpy(pduHeader + pduHeaderLength, destinationEntity.buffer
			+ entityNbrPad, entityNbrLength);
	pduHeaderLength += entityNbrLength;

	/*	Prepend header to pdu.					*/

	oK(zco_prepend_header(sdr, *pdu, (char *) pduHeader, pduHeaderLength));

	/*	If CRC required, compute CRC and append to pdu.		*/

	if (crcRequired)
	{
		buf = _crcComputationBuf();
		memcpy((char *) buf, pduHeader, pduHeaderLength);
		zco_start_receiving(*pdu, &reader);
		if (zco_receive_source(sdr, &reader, pduSourceDataLength,
				((char *) buf) + pduHeaderLength) < 0)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("Can't read ZCO.", NULL);
			return -1;
		}

		crc = computeCRC(buf, pduHeaderLength + pduSourceDataLength);
		crc = htons(crc);
		oK(zco_append_trailer(sdr, *pdu, (char *) &crc, 2));
	}

	if (sdr_end_xn(sdr))
	{
		putErrmsg("UTO can't dequeue outbound PDU.", NULL);
		return -1;
	}

	if (cfdpvdb->watching & WATCH_p)
	{
		iwatch('p');
	}

	return 0;
}

/*	*	*	PDU handling functions	*	*	*	*/
#if 0
static int	parseFaultLocationTLV(OutFdu *fdu, unsigned char **cursor,
			int length, int *bytesRemaining)
{
	int	padLength;

	if (length > 8)		/*	Invalid fault location.		*/
	{
		return 0;	/*	Malformed.			*/
	}

	fdu->eofFaultLocation.length = length;
	padLength = 8 - length;
	memset(fdu->eofFaultLocation.buffer, 0, padLength);
	memcpy(fdu->eofFaultLocation.buffer + padLength, *cursor, length);
	*cursor += length;
	*bytesRemaining -= length;
	return 0;
}
#endif
static int	parseFilestoreResponseTLV(CfdpEvent *event,
			unsigned char **cursor, int length, int *bytesRemaining)
{
	FilestoreResponse	resp;
	Sdr			sdr = getIonsdr();
	char			firstNameBuf[256];
	int			firstNameLength;
	char			secondNameBuf[256];
	int			secondNameLength;
	char			filestoreMsgBuf[256];
	int			filestoreMsgLength;
	Object			respObj;

	if (length < 1)				/*	Malformed.	*/
	{
		*bytesRemaining = 0;		/*	End TLV loop.	*/
		return 0;			/*	End TLV loop.	*/
	}

	resp.action = (**cursor >> 4) & 0x0f;
	resp.status = (**cursor) & 0x0f;
	(*cursor)++;
	(*bytesRemaining)--;
	length--;

	/*	First name cited in request.				*/

	if (length < 1)				/*	Malformed.	*/
	{
		*bytesRemaining = 0;		/*	End TLV loop.	*/
		return 0;			/*	End TLV loop.	*/
	}

	firstNameLength = **cursor;
	(*cursor)++;
	(*bytesRemaining)--;
	length--;
	if (firstNameLength == 0 || firstNameLength > length)
	{
		*bytesRemaining = 0;		/*	End TLV loop.	*/
		return 0;
	}

	memcpy(firstNameBuf, *cursor, firstNameLength);
	firstNameBuf[firstNameLength] = 0;
	*cursor += firstNameLength;
	*bytesRemaining -= firstNameLength;
	length -= firstNameLength;

	/*	Second name cited in request.				*/

	if (length < 1)				/*	Malformed.	*/
	{
		*bytesRemaining = 0;		/*	End TLV loop.	*/
		return 0;			/*	End TLV loop.	*/
	}

	secondNameLength = **cursor;
	(*cursor)++;
	(*bytesRemaining)--;
	length--;
	if (secondNameLength > length)		/*	Malformed.	*/
	{
		*bytesRemaining = 0;		/*	End TLV loop.	*/
		return 0;
	}

	if (secondNameLength > 0)
	{
		memcpy(secondNameBuf, *cursor, secondNameLength);
		secondNameBuf[secondNameLength] = 0;
		*cursor += secondNameLength;
		*bytesRemaining -= secondNameLength;
		length -= secondNameLength;
	}

	/*	Filestore message in response.				*/

	if (length < 1)				/*	Malformed.	*/
	{
		*bytesRemaining = 0;		/*	End TLV loop.	*/
		return 0;			/*	End TLV loop.	*/
	}

	filestoreMsgLength = **cursor;
	(*cursor)++;
	(*bytesRemaining)--;
	length--;
	if (filestoreMsgLength > length)	/*	Malformed.	*/
	{
		*bytesRemaining = 0;		/*	End TLV loop.	*/
		return 0;
	}

	if (filestoreMsgLength > 0)
	{
		memcpy(filestoreMsgBuf, *cursor, filestoreMsgLength);
		filestoreMsgBuf[filestoreMsgLength] = 0;
		*cursor += filestoreMsgLength;
		*bytesRemaining -= filestoreMsgLength;
		length -= filestoreMsgLength;
	}

	/*	Build filestore response object.			*/

	switch (resp.action)
	{
	case CfdpCreateFile:
	case CfdpDeleteFile:
	case CfdpCreateDirectory:
	case CfdpRemoveDirectory:
	case CfdpDenyFile:
	case CfdpDenyDirectory:
		if (secondNameLength > 0)	/*	Invalid.	*/
		{
			*bytesRemaining = 0;	/*	End TLV loop.	*/
			return 0;
		}

		break;

	case CfdpRenameFile:
	case CfdpAppendFile:
	case CfdpReplaceFile:
		if (secondNameLength == 0)	/*	Incomplete.	*/
		{
			*bytesRemaining = 0;	/*	End TLV loop.	*/
			return 0;
		}
	}

	resp.firstFileName = sdr_string_create(sdr, firstNameBuf);
	if (resp.firstFileName == 0)
	{
		putErrmsg("Can't retain first file name.", NULL);
		return -1;
	}

	if (secondNameLength == 0)
	{
		resp.secondFileName = 0;
	}
	else
	{
		resp.secondFileName = sdr_string_create(sdr, secondNameBuf);
		if (resp.secondFileName == 0)
		{
			putErrmsg("Can't retain second file name.", NULL);
			return -1;
		}
	}

	if (filestoreMsgLength == 0)
	{
		resp.message = 0;
	}
	else
	{
		resp.message = sdr_string_create(sdr, filestoreMsgBuf);
		if (resp.message == 0)
		{
			putErrmsg("Can't retain filestore message.", NULL);
			return -1;
		}
	}

	respObj = sdr_malloc(sdr, sizeof(FilestoreResponse));
	if (respObj == 0
	|| sdr_list_insert_last(sdr, event->filestoreResponses, respObj) == 0)
	{
		putErrmsg("Can't add filestore response.", NULL);
		return -1;
	}

	sdr_write(sdr, respObj, (char *) &resp, sizeof(FilestoreResponse));
	return 0;
}

static int	parseFinishPduTLV(CfdpEvent *event, unsigned char **cursor,
			int *bytesRemaining)
{
	int	type;
	int	length;

	if (*bytesRemaining < 2)		/*	Malformed.	*/
	{
		*bytesRemaining = 0;		/*	End TLV loop.	*/
		return 0;
	}

	type = **cursor;
	length = *(*cursor + 1);
	*cursor += 2;
	*bytesRemaining -= 2;
	if (*bytesRemaining < length)		/*	Malformed.	*/
	{
		*bytesRemaining = 0;		/*	End TLV loop.	*/
		return 0;
	}

	switch (type)
	{
#if 0
	case 6:
		return parseFaultLocationTLV(event, cursor, length,
				bytesRemaining);
#endif
	case 1:
		return parseFilestoreResponseTLV(event, cursor, length,
				bytesRemaining);

	default:				/*	Invalid.	*/
		*bytesRemaining = 0;		/*	End TLV loop.	*/
		return 0;
	}
}

static int	handleFinishPdu(unsigned char *cursor, int bytesRemaining,
			OutFdu *fdu, Object fduObj)
{
	Sdr		sdr = getIonsdr();
	CfdpEvent	event;

	if (fdu->finishReceived)
	{
		return 0;	/*	Ignore redundant Finish.	*/
	}

	if (bytesRemaining < 1)
	{
		return 0;			/*	Malformed.	*/
	}

	if (fdu->closureElt)
	{
		sdr_free(sdr, sdr_list_data(sdr, fdu->closureElt));
		sdr_list_delete(sdr, fdu->closureElt, NULL, NULL);
		fdu->closureElt = 0;
	}

	fdu->finishReceived = 1;
	memset((char *) &event, 0, sizeof(CfdpEvent));
	memcpy((char *) &event.transactionId, (char *) &fdu->transactionId,
			sizeof(CfdpTransactionId));
	event.reqNbr = fdu->reqNbr;
	event.type = CfdpTransactionFinishedInd;
	event.condition = ((*cursor) >> 4) & 0x0f;
	event.deliveryCode = ((*cursor) >> 2) & 0x01;
	event.fileStatus = (*cursor) & 0x03;
	cursor++;
	bytesRemaining--;
	event.filestoreResponses =
			createMetadataList((getCfdpConstants())->fsrespLists);
	while (bytesRemaining > 0)
	{
		if (parseFinishPduTLV(&event, &cursor, &bytesRemaining) < 0)
		{
			putErrmsg("Failed parsing Finish TLVs.", NULL);
			return -1;
		}
	}

	if (enqueueCfdpEvent(&event) < 0)
	{
		putErrmsg("CFDP can't report on Finish received.", NULL);
		sdr_cancel_xn(sdr);
		return -1;
	}

	sdr_write(sdr, fduObj, (char *) fdu, sizeof(OutFdu));
	return 0;
}

static int	checkInFduComplete(InFdu *fdu, Object fduObj, Object fduElt)
{
	CfdpHandler	handler;

	if (!fdu->metadataReceived)
	{
		return 0;
	}

	if (!fdu->eofReceived)
	{
		return 0;
	}

	if (fdu->bytesReceived < fdu->fileSize)	/*	Missing data.	*/
	{
		return 0;
	}

	if (fdu->computedChecksum == fdu->eofChecksum
	|| fdu->ckType == NullChecksum)
	{
		fdu->checksumVerified = 1;
	}
	else
	{
		if (handleFault(&fdu->transactionId, CfdpChecksumFailure,
					&handler) < 0)
		{
			putErrmsg("Can't check FDU completion.", NULL);
			return -1;
		}

		switch (handler)
		{
		case CfdpCancel:
		case CfdpAbandon:
			return 0;		/*	Nothing to do.	*/

		default:
			break;			/*	No problem.	*/
		}
	}

	return completeInFdu(fdu, fduObj, fduElt, CfdpNoError, 0);
}

static int	getFileName(InFdu *fdu, char *stringBuf, int bufLen)
{
	Sdr	sdr = getIonsdr();
	uvast	sourceEntityId;
	uvast	transactionNbr;

	if (fdu->workingFileName == 0)
	{
		cfdp_decompress_number(&sourceEntityId,
				&fdu->transactionId.sourceEntityNbr);
		cfdp_decompress_number(&transactionNbr,
				&fdu->transactionId.transactionNbr);
		isprintf(stringBuf, bufLen,
				"%s%ccfdp." UVAST_FIELDSPEC "." UVAST_FIELDSPEC,
				getIonWorkingDirectory(), ION_PATH_DELIMITER,
				sourceEntityId, transactionNbr);
		fdu->workingFileName = sdr_string_create(sdr, stringBuf);
		if (fdu->workingFileName == 0)
		{
			putErrmsg("Can't retain working file name.", stringBuf);
			return -1;
		}
	}
	else
	{
		sdr_string_read(sdr, stringBuf, fdu->workingFileName);
	}

	return 0;
}

static int	handleFilestoreRejection(InFdu *fdu, int returnCode,
			CfdpHandler *handler)
{
	if (handleFault(&fdu->transactionId, CfdpFilestoreRejection,
				handler) < 0)
	{
		putErrmsg("Can't handle filestore rejection.", NULL);
		returnCode = -1;
	}

	return returnCode;
}

static int	writeSegmentData(InFdu *fdu, unsigned char **cursor,
			int *bytesRemaining, vast *segmentOffset,
			int bytesToWrite)
{
	CfdpVdb		*cfdpvdb = _cfdpvdb(NULL);
	CfdpHandler	handler;
	int		remainder;

	if (cfdpvdb->corruptionModulus)
	{
		remainder = rand() % cfdpvdb->corruptionModulus;
		if (remainder == 0)
		{
			(**cursor)++;	/*	Introduce corruption.	*/
			writeMemo("CFDP corrupted a byte.");
		}
	}

	if (write(cfdpvdb->currentFile, *cursor, bytesToWrite) < 0)
	{
		putSysErrmsg("Can't write to file", itoa(bytesToWrite));
		return handleFilestoreRejection(fdu, -1, &handler);
	}

	fdu->bytesReceived += bytesToWrite;
	while (bytesToWrite > 0)
	{
		addToChecksum(**cursor, segmentOffset, &fdu->computedChecksum,
				fdu->ckType);
		(*cursor)++;
		(*bytesRemaining)--;
		bytesToWrite--;
	}

	return 0;
}

static int	handleFileDataPdu(unsigned char *cursor, int bytesRemaining,
			InFdu *fdu, Object fduObj, Object fduElt, int largeFile,
			int recordStructure, int haveMetadata)
{
	PsmPartition	wm = getIonwm();
	CfdpEvent	event;
	int		offsetLength;
	int		i;
	vast		segmentOffset;
	uvast		segmentEnd;
	CfdpHandler	handler;
	Sdr		sdr = getIonsdr();
	CfdpVdb		*cfdpvdb = _cfdpvdb(NULL);
	CfdpDB		cfdpdb;
	Object		elt;
	Object		addr;
	CfdpExtent	extent;
	uvast		extentEnd = 0;
	Object		nextElt = 0;
	uvast		bytesToSkip;
	char		stringBuf[256];
	char		workingNameBuffer[MAXPATHLEN + 1];
	vast		endOfFile;
	uvast		fileLength;
	uvast		fillBufSize;
	uvast		bufSizeLimit;
	PsmUsageSummary	usage;
	char		*fillBuf;
	uvast		fillSize;
	Object		nextAddr;
	CfdpExtent	nextExtent;
	uvast		bytesToWrite;
	uvast		nextExtentEnd;

	/*	Prepare to issue indication.				*/

	memset((char *) &event, 0, sizeof(CfdpEvent));
	event.type = CfdpFileSegmentRecvInd;
	memcpy((char *) &event.transactionId, (char *) &fdu->transactionId,
			sizeof(CfdpTransactionId));
	event.recordBoundsRespected = recordStructure;
	if (haveMetadata)
	{
		if (bytesRemaining < 1)
		{
			return 0;	/*	Malformed.	*/
		}

		event.continuationState = ((*cursor) >> 6) & 0x03;
		event.segMetadataLength = (*cursor) & 0x3f;
		cursor++;
		bytesRemaining--;
		if (event.segMetadataLength > 0)
		{
			if (event.segMetadataLength > bytesRemaining)
			{
				return 0;	/*	Malformed.	*/
			}

			memcpy(event.segMetadata, cursor,
					event.segMetadataLength);
			cursor += event.segMetadataLength;
			bytesRemaining -= event.segMetadataLength;
		}
	}

	offsetLength = largeFile ? 8 : 4;
	if (offsetLength > bytesRemaining)
	{
		return 0;			/*	Malformed.	*/
	}

	segmentOffset = 0;
	for (i = 0; i < offsetLength; i++)
	{
		segmentOffset = (segmentOffset << 8) + *cursor;
		cursor++;
		bytesRemaining--;
	}

	if (bytesRemaining == 0)		/*	No file data.	*/
	{
		return 0;			/*	Nothing to do.	*/
	}

	sdr_read(sdr, (char *) &cfdpdb, getCfdpDbObject(), sizeof(CfdpDB));
	fdu->inactivityDeadline = getCtime()
			+ cfdpdb.transactionInactivityLimit;

	event.offset = segmentOffset;
	event.length = bytesRemaining;

	/*	Update reception progress, check for fault.		*/

	segmentEnd = segmentOffset + bytesRemaining;
	if (segmentEnd > fdu->progress)
	{
		fdu->progress = segmentEnd;
		if (fdu->eofReceived && fdu->progress > fdu->fileSize)
		{
			if (handleFault(&fdu->transactionId, CfdpFileSizeError,
					&handler) < 0)
			{
				putErrmsg("Can't handle EOF PDU.", NULL);
				return -1;
			}

			switch (handler)
			{
			case CfdpCancel:
			case CfdpAbandon:
				return 0;	/*	Nothing to do.	*/

			default:
				break;		/*	No problem.	*/
			}
		}
	}

	/*	Figure out how much of the file data PDU is new data.	*/

	for (elt = sdr_list_first(sdr, fdu->extents); elt;
			elt = sdr_list_next(sdr, elt))
	{
		addr = sdr_list_data(sdr, elt);
		sdr_stage(sdr, (char *) &extent, addr, sizeof(CfdpExtent));
		extentEnd = extent.offset + extent.length;
#if CFDPDEBUG
printf("Viewing extent from " UVAST_FIELDSPEC " to " UVAST_FIELDSPEC ".\n",
extent.offset, extent.offset + extent.length);
#endif
		if (extentEnd < segmentOffset)	/*	No relation.	*/
		{
			continue;	/*	Look for later extent.	*/
		}

		/*	This extent ends at or after the start of
		 *	this segment.  Don't search any further in
		 *	the extents.					*/

		if (extent.offset <= segmentOffset)
		{
			/*	This extent starts before the start
			 *	of the segment, i.e., part or all of
			 *	this segment has already been received.	*/

			bytesToSkip = extentEnd - segmentOffset;
			if (bytesToSkip >= bytesRemaining)
			{
				return 0;	/*	Ignore.		*/
			}

			/*	This segment extends this extent.	*/

			extent.length = segmentEnd - extent.offset;
			sdr_write(sdr, addr, (char *) &extent,
					sizeof(CfdpExtent));
#if CFDPDEBUG
printf("Rewriting extent at " UVAST_FIELDSPEC ", to " UVAST_FIELDSPEC ".\n",
extent.offset, extent.offset + extent.length);
#endif
			extentEnd = extent.offset + extent.length;

			/*	Skip over any repeated data at the
			 *	start of the segment.			*/

			segmentOffset += bytesToSkip;
			cursor += bytesToSkip;
			bytesRemaining -= bytesToSkip;
#if CFDPDEBUG
printf("Skipping " UVAST_FIELDSPEC " bytes, segmentOffset changed to " UVAST_FIELDSPEC ".\n",
bytesToSkip, segmentOffset);
#endif
		}
		else	/*	This segment starts a new extent.	*/
		{
			nextElt = elt;
			elt = 0;	/*	New extent needed.	*/
		}

		break;
	}

	/*	Insert new extent if necessary.				*/

	if (elt == 0)	/*	Must write the new extent to database.	*/
	{
		extent.offset = segmentOffset;
		extent.length = bytesRemaining;
		addr = sdr_malloc(sdr, sizeof (CfdpExtent));
		if (addr == 0
		|| (elt = (nextElt == 0	?
			sdr_list_insert_last(sdr, fdu->extents, addr)
					:
			sdr_list_insert_before(sdr, nextElt, addr))) == 0)
		{
			putErrmsg("Can't insert extent.", NULL);
			return -1;
		}

		sdr_write(sdr, addr, (char *) &extent, sizeof(CfdpExtent));
#if CFDPDEBUG
printf("Writing extent from " UVAST_FIELDSPEC " to " UVAST_FIELDSPEC ".\n",
extent.offset, extent.offset + extent.length);
#endif
		extentEnd = extent.offset + extent.length;
	}

	nextElt = sdr_list_next(sdr, elt);

	/*	Open the file (possibly a temporary working file) if
	 *	it's not the currently open file.			*/

	if (getFileName(fdu, stringBuf, sizeof stringBuf) < 0)
	{
		putErrmsg("Can't get file name.", NULL);
		return -1;
	}

	istrcpy(workingNameBuffer, stringBuf, sizeof workingNameBuffer);

	/*	Now open the file, creating it if necessary.		*/

	if (cfdpvdb->currentFdu != fduObj)	/*	Switching FDU.	*/
	{
		if (cfdpvdb->currentFile != -1)
		{
			close(cfdpvdb->currentFile);
			cfdpvdb->currentFile = -1;
		}

		cfdpvdb->currentFdu = 0;
		cfdpvdb->currentFile = ifopen(workingNameBuffer,
				O_RDWR | O_CREAT, 0777);
		if (cfdpvdb->currentFile < 0)
		{
			putSysErrmsg("Can't open working file",
					workingNameBuffer);
			return handleFilestoreRejection(fdu, 0, &handler);
		}

		cfdpvdb->currentFdu = fduObj;
	}

	/*	Write leading fill characters as necessary.		*/

	endOfFile = ilseek(cfdpvdb->currentFile, 0, SEEK_END);
	if (endOfFile < 0)
	{
		putSysErrmsg("Can't lseek in file", workingNameBuffer);
		return handleFilestoreRejection(fdu, -1, &handler);
	}

	fileLength = endOfFile;
	if (fileLength < segmentOffset)
	{
		/*	Temporarily take large working memory
		 *	buffer for fill characters.  Try to use
		 *	all free large pool space; if sparse, try
		 *	for configured maximum.				*/

		fillBufSize = segmentOffset - fileLength;
		bufSizeLimit = CFDP_FILLBUF_LIMIT;
		psm_usage(wm, &usage);
		if (usage.largePoolFree > bufSizeLimit)
		{
			bufSizeLimit = usage.largePoolFree;
		}

		if (fillBufSize > bufSizeLimit)
		{
			fillBufSize = bufSizeLimit;
		}

		while (1)
		{
			fillBuf = MTAKE(fillBufSize);
			if (fillBuf)
			{
				break;	/*	Got large buffer.	*/
			}

			/*	Try to grab a smaller buffer.		*/

			fillBufSize /= 2;
			if (fillBufSize < 1)
			{
				putErrmsg("No working memory for fill buffer.",
						NULL);
				return handleFilestoreRejection(fdu, 0,
						&handler);
			}
		}

		memset(fillBuf, cfdpdb.fillCharacter, fillBufSize);
		while (fileLength < segmentOffset)
		{
			fillSize = segmentOffset - fileLength;
			if (fillSize > fillBufSize)
			{
				fillSize = fillBufSize;
			}

			if (write(cfdpvdb->currentFile, fillBuf, fillSize) < 0)
			{
				putSysErrmsg("Can't write to file",
						workingNameBuffer);
				MRELEASE(fillBuf);
				return handleFilestoreRejection(fdu, -1,
						&handler);
			}
	
			fileLength += fillSize;
		}

		MRELEASE(fillBuf);
	}

	/*	Reposition at offset of new file data bytes.		*/

	if (ilseek(cfdpvdb->currentFile, segmentOffset, SEEK_SET) < 0)
	{
		putSysErrmsg("Can't lseek in file", workingNameBuffer);
		return handleFilestoreRejection(fdu, -1, &handler);
	}

	/*	Now write new file data, updating checksum in the
	 *	process.  While doing this, collapse subsequent
	 *	extents into the current one until an unbridged gap
	 *	in continuity is reached.  This may entail filling
	 *	any number of inter-extent gaps.			*/

	while (nextElt)
	{
		nextAddr = sdr_list_data(sdr, nextElt);
		sdr_stage(sdr, (char *) &nextExtent, nextAddr,
				sizeof(CfdpExtent));
#if CFDPDEBUG
printf("Continuing to extent from " UVAST_FIELDSPEC " to " UVAST_FIELDSPEC "; \
segmentOffset is " UVAST_FIELDSPEC ".\n", nextExtent.offset,
nextExtent.offset + nextExtent.length, segmentOffset);
#endif
		if (nextExtent.offset > segmentEnd)
		{
			break;	/*	Reached an unbridged gap.	*/
		}

		/*	This extent will be subsumed into prior extent.
		 *	First, bridge gap to the start of this extent.	*/

		bytesToWrite = nextExtent.offset - segmentOffset;
		if (writeSegmentData(fdu, &cursor, &bytesRemaining,
				&segmentOffset, bytesToWrite) < 0)
		{
			putErrmsg("Can't write segment data.",
					workingNameBuffer);
			return -1;
		}

		/*	Now skip over all data that were written when
		 *	this extent was posted.				*/

		bytesToSkip = nextExtent.length;
		segmentOffset += bytesToSkip;
		cursor += bytesToSkip;
		bytesRemaining -= bytesToSkip;

		/*	Now subsume the extent into the prior extent.	*/

		nextExtentEnd = nextExtent.offset + nextExtent.length;
		if (nextExtentEnd > extentEnd)
		{
			/*	Extend the prior extent.		*/

			extent.length = nextExtentEnd - extent.offset;
			sdr_write(sdr, addr, (char *) &extent,
					sizeof(CfdpExtent));
			extentEnd = extent.offset + extent.length;
		}

		elt = sdr_list_next(sdr, nextElt);
		sdr_free(sdr, nextAddr);
		sdr_list_delete(sdr, nextElt, NULL, NULL);
		nextElt = elt;
	}

	/*	Write final hunk of segment data.			*/

	if (segmentEnd > segmentOffset)
	{
		bytesToWrite = segmentEnd - segmentOffset;
		if (writeSegmentData(fdu, &cursor, &bytesRemaining,
				&segmentOffset, bytesToWrite) < 0)
		{
			putErrmsg("Can't write segment data.",
					workingNameBuffer);
			return -1;
		}
	}

#ifdef TargetFFS
	close(cfdpvdb->currentFile);
	cfdpvdb->currentFile = -1;
	cfdpvdb->currentFdu = 0;
#endif
	/*	Deliver File-Segment-Recv indication.			*/

	if (enqueueCfdpEvent(&event) < 0)
	{
		putErrmsg("Can't post File-Segment-Recv indication.", NULL);
		return -1;
	}

	sdr_write(sdr, fduObj, (char *) fdu, sizeof(InFdu));
	return checkInFduComplete(fdu, fduObj, fduElt);
}

static int	parseFilestoreRequestTLV(InFdu *fdu, unsigned char **cursor,
			int length, int *bytesRemaining)
{
	FilestoreRequest	req;
	Sdr			sdr = getIonsdr();
	char			firstNameBuf[256];
	int			firstNameLength;
	char			secondNameBuf[256];
	int			secondNameLength;
	Object			reqObj;

	if (length < 2)				/*	Malformed.	*/
	{
		*bytesRemaining = 0;		/*	End TLV loop.	*/
		return 0;			/*	End TLV loop.	*/
	}

	req.action = (**cursor >> 4) & 0x0f;
	(*cursor)++;
	(*bytesRemaining)--;
	length--;
	firstNameLength = **cursor;
	(*cursor)++;
	(*bytesRemaining)--;
	length--;
	if (firstNameLength == 0 || firstNameLength > length)
	{
		*bytesRemaining = 0;		/*	End TLV loop.	*/
		return 0;
	}

	memcpy(firstNameBuf, *cursor, firstNameLength);
	firstNameBuf[firstNameLength] = 0;
	*cursor += firstNameLength;
	*bytesRemaining -= firstNameLength;
	length -= firstNameLength;
	if (length < 1)		/*	No length for 2nd file name.	*/
	{
		*bytesRemaining = 0;		/*	End TLV loop.	*/
		return 0;
	}

	secondNameLength = **cursor;
	(*cursor)++;
	(*bytesRemaining)--;
	length--;
	if (secondNameLength != length)		/*	Malformed.	*/
	{
		*bytesRemaining = 0;		/*	End TLV loop.	*/
		return 0;
	}

	if (secondNameLength > 0)
	{
		memcpy(secondNameBuf, *cursor, secondNameLength);
		secondNameBuf[secondNameLength] = 0;
		*cursor += secondNameLength;
		*bytesRemaining -= secondNameLength;
	}

	switch (req.action)
	{
	case CfdpCreateFile:
	case CfdpDeleteFile:
	case CfdpCreateDirectory:
	case CfdpRemoveDirectory:
	case CfdpDenyFile:
	case CfdpDenyDirectory:
		if (secondNameLength > 0)	/*	Invalid.	*/
		{
			*bytesRemaining = 0;	/*	End TLV loop.	*/
			return 0;
		}

		break;

	case CfdpRenameFile:
	case CfdpAppendFile:
	case CfdpReplaceFile:
		if (secondNameLength == 0)	/*	Incomplete.	*/
		{
			*bytesRemaining = 0;	/*	End TLV loop.	*/
			return 0;
		}
	}

	req.firstFileName = sdr_string_create(sdr, firstNameBuf);
	if (req.firstFileName == 0)
	{
		putErrmsg("Can't retain first file name.", NULL);
		return -1;
	}

	if (secondNameLength == 0)
	{
		req.secondFileName = 0;
	}
	else
	{
		req.secondFileName = sdr_string_create(sdr, secondNameBuf);
		if (req.secondFileName == 0)
		{
			putErrmsg("Can't retain second file name.", NULL);
			return -1;
		}
	}

	reqObj = sdr_malloc(sdr, sizeof(FilestoreRequest));
	if (reqObj == 0
	|| sdr_list_insert_last(sdr, fdu->filestoreRequests, reqObj) == 0)
	{
		putErrmsg("Can't add filestore request.", NULL);
		return -1;
	}

	sdr_write(sdr, reqObj, (char *) &req, sizeof(FilestoreRequest));
	return 0;
}

static int	parseMessageToUserTLV(InFdu *fdu, unsigned char **cursor,
			int length, int *bytesRemaining)
{
	MsgToUser	msg;
	Sdr		sdr = getIonsdr();
	Object		msgObj;

	if (length == 0)	/*	Null message.			*/
	{
		return 0;	/*	Nothing to do.			*/
	}

	msg.length = length;
	msg.text = sdr_malloc(sdr, msg.length);
	if (msg.text)
	{
		sdr_write(sdr, msg.text, (char *) *cursor, msg.length);
	}

	msgObj = sdr_malloc(sdr, sizeof(MsgToUser));
	if (msgObj == 0
	|| sdr_list_insert_last(sdr, fdu->messagesToUser, msgObj) == 0)
	{
		putErrmsg("Can't add message to user.", NULL);
		return -1;
	}

	sdr_write(sdr, msgObj, (char *) &msg, sizeof(MsgToUser));
	*cursor += msg.length;
	*bytesRemaining -= msg.length;
	return 0;
}

static int	parseFaultHandlerTLV(InFdu *fdu, unsigned char **cursor,
			int length, int *bytesRemaining)
{
	unsigned int	override;
	CfdpCondition	condition;
	CfdpHandler	handler;

	if (length != 1)			/*	Incomplete.	*/
	{
		*bytesRemaining = 0;		/*	End TLV loop.	*/
		return 0;
	}

	override = **cursor;
	condition = (override >> 4) & 0x0f;
	handler = override & 0x0f;
	switch (handler)
	{
	case CfdpNoHandler:
	case CfdpCancel:
	case CfdpSuspend:
	case CfdpIgnore:
	case CfdpAbandon:
		break;

	default:				/*	Invalid.	*/
		*bytesRemaining = 0;		/*	End TLV loop.	*/
		return 0;
	}

	fdu->faultHandlers[condition] = handler;
	*cursor += length;
	*bytesRemaining -= length;
	return 0;
}

static int	parseFlowLabelTLV(InFdu *fdu, unsigned char **cursor,
			int length, int *bytesRemaining)
{
	Sdr	sdr = getIonsdr();

	if (length == 0)	/*	Null flow label.		*/
	{
		return 0;	/*	Nothing to do.			*/
	}

	if (fdu->flowLabel)
	{
		sdr_free(sdr, fdu->flowLabel);
	}

	fdu->flowLabel = sdr_malloc(sdr, length);
	if (fdu->flowLabel == 0)
	{
		putErrmsg("Can't retain flow label.", NULL);
		return -1;
	}

	sdr_write(sdr, fdu->flowLabel, (char *) *cursor, length);
	fdu->flowLabelLength = length;
	*cursor += length;
	*bytesRemaining -= length;
	return 0;
}

static int	parseEntityIdTLV(InFdu *fdu, unsigned char **cursor,
			int length, int *bytesRemaining)
{
	int	padLength;

	if (length > 8)		/*	Invalid fault location.		*/
	{
		return 0;	/*	Malformed.			*/
	}

	fdu->eofFaultLocation.length = length;
	padLength = 8 - length;
	memset(fdu->eofFaultLocation.buffer, 0, padLength);
	memcpy(fdu->eofFaultLocation.buffer + padLength, *cursor, length);
	*cursor += length;
	*bytesRemaining -= length;
	return 0;
}

static int	parseTLV(InFdu *fdu, unsigned char **cursor,
			int *bytesRemaining, int directiveCode)
{
	int	type;
	int	length;

	if (*bytesRemaining < 2)		/*	Malformed.	*/
	{
		*bytesRemaining = 0;		/*	End TLV loop.	*/
		return 0;
	}

	type = **cursor;
	length = *(*cursor + 1);
	*cursor += 2;
	*bytesRemaining -= 2;
	if (*bytesRemaining < length)		/*	Malformed.	*/
	{
		*bytesRemaining = 0;		/*	End TLV loop.	*/
		return 0;
	}

	if (directiveCode == 4)			/*	EOF PDU.	*/
	{
		if (type == 6)
		{
			return parseEntityIdTLV(fdu, cursor, length,
					bytesRemaining);
		}

		/*	Invalid.  No other TLV is valid for EOF.	*/

		return 0;
	}


	/*	Directive code must be 7, Metadata PDU.			*/

	switch (type)
	{
	case 0:
		return parseFilestoreRequestTLV(fdu, cursor, length,
				bytesRemaining);

	case 2:
		return parseMessageToUserTLV(fdu, cursor, length,
				bytesRemaining);

	case 4:
		return parseFaultHandlerTLV(fdu, cursor, length,
				bytesRemaining);

	case 5:
		return parseFlowLabelTLV(fdu, cursor, length,
				bytesRemaining);

	default:				/*	Invalid.	*/
		*bytesRemaining = 0;		/*	End TLV loop.	*/
		return 0;
	}
}

static int	handleEofPdu(unsigned char *cursor, int bytesRemaining,
			InFdu *fdu, Object fduObj, Object fduElt, int largeFile)
{
	int		sizeFieldLength;
	int		minPduSize;
	int		i;
	CfdpHandler	handler;
	CfdpEvent	event;
	Sdr		sdr = getIonsdr();
	CfdpDB		cfdpdb;

	if (fdu->eofReceived)
	{
		return 0;		/*	Ignore redundant EOF.	*/
	}

	if (largeFile)
	{
		sizeFieldLength = 8;	/*	64-bit file size.	*/
	}
	else
	{
		sizeFieldLength = 4;	/*	32-bit file size.	*/
	}

	minPduSize = 5 + sizeFieldLength;
	if (bytesRemaining < minPduSize)
	{
		return 0;		/*	Malformed.		*/
	}

	sdr_read(sdr, (char *) &cfdpdb, getCfdpDbObject(), sizeof(CfdpDB));
	fdu->inactivityDeadline = getCtime()
			+ cfdpdb.transactionInactivityLimit;
	fdu->eofReceived = 1;
	fdu->eofCondition = ((*cursor) >> 4) & 0x0f;
	cursor++;
	bytesRemaining--;
	fdu->eofChecksum = 0;
	for (i = 0; i < 4; i++)
	{
		fdu->eofChecksum = (fdu->eofChecksum << 8) + *cursor;
		cursor++;
		bytesRemaining--;
	}

	fdu->fileSize = 0;
	for (i = 0; i < sizeFieldLength; i++)
	{
		fdu->fileSize = (fdu->fileSize << 8) + *cursor;
		cursor++;
		bytesRemaining--;
	}

	if (bytesRemaining > 0)
	{
		if (fdu->eofCondition == CfdpNoError)
		{
			return 0;		/*	Malformed.	*/
		}

		if (parseTLV(fdu, &cursor, &bytesRemaining, 4) < 0)
		{
			putErrmsg("Failed parsing TLV.", NULL);
			return -1;
		}

		if (bytesRemaining > 0)		/*	Extra bytes.	*/
		{
			return 0;		/*	Malformed.	*/
		}
	}

	/*	Check for file size error.				*/

	if (fdu->progress > fdu->fileSize)
	{
		if (handleFault(&fdu->transactionId, CfdpFileSizeError,
				&handler) < 0)
		{
			putErrmsg("Can't handle EOF PDU.", NULL);
			return -1;
		}

		switch (handler)
		{
		case CfdpCancel:
		case CfdpAbandon:
			return 0;		/*	Nothing to do.	*/

		default:
			break;			/*	No problem.	*/
		}
	}

	/*	Deliver EOF-Recv indication.				*/

	memset((char *) &event, 0, sizeof(CfdpEvent));
	event.type = CfdpEofRecvInd;
	event.fileSize = fdu->fileSize;
	memcpy((char *) &event.transactionId, (char *) &fdu->transactionId,
			sizeof(CfdpTransactionId));
	if (enqueueCfdpEvent(&event) < 0)
	{
		putErrmsg("Can't post EOF-Recv indication.", NULL);
		return -1;
	}

	fdu->checkTime = getCtime();
	fdu->checkTime += cfdpdb.checkTimerPeriod;
	sdr_write(sdr, fduObj, (char *) fdu, sizeof(InFdu));
	return checkInFduComplete(fdu, fduObj, fduElt);
}

static int	handleMetadataPdu(unsigned char *cursor, int bytesRemaining,
			InFdu *fdu, Object fduObj, Object fduElt, int largeFile)
{
	int		sizeFieldLength;
	int		minPduSize;
	CfdpDB		cfdpdb;
	CfdpCksumType	ckType;
	int		ckTypeMismatch = 0;
	unsigned int	fileSize;
	int		i;
	char		stringBuf[256];
	char		qualifiedFileName[MAXPATHLEN + 1];
	Sdr		sdr = getIonsdr();
	CfdpEvent	event;
	CfdpHandler	handler;

	if (fdu->metadataReceived)
	{
		return 0;	/*	Ignore redundant metadata.	*/
	}

	if (largeFile)
	{
		sizeFieldLength = 8;	/*	64-bit file size.	*/
	}
	else
	{
		sizeFieldLength = 4;	/*	32-bit file size.	*/
	}

	minPduSize = 1 + sizeFieldLength;
	if (bytesRemaining < minPduSize)
	{
		return 0;		/*	Malformed.		*/
	}

	sdr_read(sdr, (char *) &cfdpdb, getCfdpDbObject(), sizeof(CfdpDB));
	fdu->inactivityDeadline = getCtime()
			+ cfdpdb.transactionInactivityLimit;
	fdu->metadataReceived = 1;
	fdu->closureRequested = ((*cursor) >> 6) & 0x01;

	/*	Get checksum type from low-order 4 bits of first byte
	 *	of Metadata PDU parameter field contents, except that
	 *	if some file data segments of this file have already
	 *	been received there is no point in switching to a
	 *	different checksum type.				*/

	ckType = (*cursor) & 0x0f;
	if (fdu->progress == 0)
	{
		if (ckTypeOkay(ckType))
		{
			fdu->ckType = ckType;
		}
		else	/*	Leave default in place.			*/
		{
			if (handleFault(&fdu->transactionId,
				CfdpUnsupportedChecksumType, &handler) < 0)
			{
				putErrmsg("No fault handler.", NULL);
			}

			fdu->finishCondition = CfdpUnsupportedChecksumType;
		}
	}
	else
	{
		if (fdu->ckType != ckType)
		{
			ckTypeMismatch = 1;
		}
	}

	cursor++;
	bytesRemaining--;

	/*	Get projected file size.				*/

	fileSize = 0;
	for (i = 0; i < sizeFieldLength; i++)
	{
		fileSize = (fileSize << 8) + *cursor;
		cursor++;
		bytesRemaining--;
	}

	/*	Parse source file name LV.				*/

	if (bytesRemaining < 1) return 0;	/*	Malformed.	*/
	i = *cursor;
	cursor++;
	bytesRemaining--;
	if (bytesRemaining < i) return 0;	/*	Malformed.	*/
	if (i > 0)
	{
		memcpy(stringBuf, cursor, i);
		cursor += i;
		bytesRemaining -= i;
		stringBuf[i] = 0;
		fdu->sourceFileName = sdr_string_create(sdr, stringBuf);
		if (fdu->sourceFileName == 0)
		{
			putErrmsg("Can't retain source file name.", stringBuf);
			return -1;
		}
	}

	/*	Parse destination file name LV.				*/

	if (bytesRemaining < 1) return 0;	/*	Malformed.	*/
	i = *cursor;
	cursor++;
	bytesRemaining--;
	if (bytesRemaining < i) return 0;	/*	Malformed.	*/
	if (i > 0)
	{
		memcpy(stringBuf, cursor, i);
		cursor += i;
		bytesRemaining -= i;
		stringBuf[i] = 0;
		if (getQualifiedFileName(qualifiedFileName,
				sizeof qualifiedFileName, stringBuf) == 0
		|| strlen(qualifiedFileName) >= SDRSTRING_BUFSZ)
		{
			writeMemoNote("[?] CFDP can't qualify dest file name",
					stringBuf);
			istrcpy(qualifiedFileName, stringBuf,
					sizeof qualifiedFileName);
		}

		fdu->destFileName = sdr_string_create(sdr, qualifiedFileName);
		if (fdu->destFileName == 0)
		{
			putErrmsg("Can't retain dest file name.",
					qualifiedFileName);
			return -1;
		}

		if (fdu->workingFileName == 0)
		{
			fdu->workingFileName = fdu->destFileName;
		}

		if (fdu->sourceFileName == 0)
		{
			/*	Compressed: use destination file name.	*/

			fdu->sourceFileName = sdr_string_create(sdr,
					stringBuf);
			if (fdu->sourceFileName == 0)
			{
				putErrmsg("Can't retain source file name.",
						stringBuf);
				return -1;
			}
		}
	}

	if (ckTypeMismatch)
	{
		writeMemoNote("[?] Late arrival of Metadata, so checksums \
will not match", stringBuf);
	}

	/*	Parse TLVs.  If at any point a TLV is found to be
	 *	malformed, parsing of the Metadata PDU terminates
	 *	but processing continues.				*/

	while (bytesRemaining > 0)
	{
		if (parseTLV(fdu, &cursor, &bytesRemaining, 7) < 0)
		{
			putErrmsg("Failed parsing TLVs.", NULL);
			return -1;
		}
	}

	/*	Deliver Metadata-Recv indication.			*/

	memset((char *) &event, 0, sizeof(CfdpEvent));
	event.type = CfdpMetadataRecvInd;
	memcpy((char *) &event.transactionId, (char *) &fdu->transactionId,
			sizeof(CfdpTransactionId));
	if (fdu->sourceFileName)
	{
		sdr_string_read(sdr, stringBuf, fdu->sourceFileName);
		event.sourceFileName = sdr_string_create(sdr, stringBuf);
	}

	if (fdu->destFileName)
	{
		sdr_string_read(sdr, stringBuf, fdu->destFileName);
		event.destFileName = sdr_string_create(sdr, stringBuf);
	}

	event.fileSize = fileSize;	/*	Projected, not actual.	*/
	event.messagesToUser = fdu->messagesToUser;

	/*	Must transform the messagesToUser list into a
	 *	MetadataList for delivery to application.		*/

	sdr_list_user_data_set(sdr, event.messagesToUser,
		sdr_list_insert_last(sdr, (getCfdpConstants())->usrmsgLists,
			event.messagesToUser));

	/*	Detach messagesToUser list from FDU so it won't be
	 *	deleted twice.						*/

	fdu->messagesToUser = 0;
	if (enqueueCfdpEvent(&event) < 0)
	{
		putErrmsg("Can't post Metadata-Recv indication.", NULL);
		return -1;
	}

	sdr_write(sdr, fduObj, (char *) fdu, sizeof(InFdu));

	/*	Metadata PDU has been fully processed.  Now follow up.	*/

	if (fdu->destFileName)
	{
		sdr_string_read(sdr, stringBuf, fdu->destFileName);
		if (checkFile(stringBuf) == 1)
		{
			/*	This file already exists.		*/

			writeMemoNote("[?] File already exists.", stringBuf);
			if (handleFilestoreRejection(fdu, 0, &handler) < 0)
			{
				return -1;
			}

			switch (handler)
			{
			case CfdpCancel:
			case CfdpAbandon:
				return 0;	/*	Nothing to do.	*/

			default:
				break;		/*	No problem.	*/
			}
		}
	}

	return checkInFduComplete(fdu, fduObj, fduElt);
}

int	cfdpHandleInboundPdu(unsigned char *buf, int length)
{
	unsigned char		*cursor = buf;
	int			bytesRemaining = length;
	int			pduIsFileData;
	int			direction;
	int			modeIsUnacknowledged;
	int			crcIsPresent;
	int			largeFile;
	int			dataFieldLength;
	int			recordStructure;
	int			entityNbrLength;
	int			entityNbrPad;
	int			haveMetadata;
	int			transactionNbrLength;
	int			transactionNbrPad;
	CfdpNumber		sourceEntityNbr;
	CfdpNumber		transactionNbr;
	CfdpNumber		destinationEntityNbr;
	Sdr			sdr = getIonsdr();
	CfdpDB			*cfdpConstants = _cfdpConstants();
	CfdpVdb			*cfdpvdb = _cfdpvdb(NULL);
	unsigned short		deliveredCRC;
	unsigned short		computedCRC;
	CfdpTransactionId	transactionId;
	CfdpHandler		handler;
	Object			fduObj;
	InFdu			fduBuf;
	OutFdu			outFduBuf;
	Object			fduElt;
	int			directiveCode;
	int			result;

#if CFDPDEBUG
printf("...in cfdpHandleInboundPdu...\n"); 
#endif
	CHKERR(buf);
	memset((char *) &sourceEntityNbr, 0, sizeof(CfdpNumber));
	memset((char *) &transactionNbr, 0, sizeof(CfdpNumber));
	memset((char *) &destinationEntityNbr, 0, sizeof(CfdpNumber));

	/*	Parse PDU header.					*/

	if (bytesRemaining < 4)
	{
		return 0;		/*	Malformed PDU.		*/
	}

	pduIsFileData = ((*cursor) >> 4) & 0x01;
	direction = ((*cursor) >> 3) & 0x01;
	modeIsUnacknowledged = ((*cursor) >> 2) & 0x01;
	crcIsPresent = ((*cursor) >> 1) & 0x01;
	largeFile = (*cursor) & 0x01;
	cursor++;
	bytesRemaining--;
	dataFieldLength = *cursor << 8;
	cursor++;
	bytesRemaining--;
	dataFieldLength += *cursor;
	cursor++;
	bytesRemaining--;
	recordStructure = ((*cursor) >> 7) & 0x01;
	entityNbrLength = ((*cursor) >> 4) & 0x07;
	entityNbrLength += 1;		/*	De-adjust.		*/
	entityNbrPad = 8 - entityNbrLength;
	haveMetadata = ((*cursor) >> 3) & 0x01;
	transactionNbrLength = *cursor & 0x07;
	transactionNbrLength += 1;	/*	De-adjust.		*/
	transactionNbrPad = 8 - transactionNbrLength;
	cursor++;
	bytesRemaining--;
	if (bytesRemaining < (entityNbrLength << 1) + transactionNbrLength)
	{
#if CFDPDEBUG
printf("...malformed PDU (missing %d bytes)...\n",
((entityNbrLength << 1) + transactionNbrLength) - bytesRemaining); 
#endif
		return 0;		/*	Malformed PDU.		*/
	}

	sourceEntityNbr.length = entityNbrLength;
	memcpy(sourceEntityNbr.buffer + entityNbrPad, cursor, entityNbrLength);
	cursor += entityNbrLength;
	bytesRemaining -= entityNbrLength;
	transactionNbr.length = transactionNbrLength;
	memcpy(transactionNbr.buffer + transactionNbrPad, cursor,
			transactionNbrLength);
	cursor += transactionNbrLength;
	bytesRemaining -= transactionNbrLength;
	destinationEntityNbr.length = entityNbrLength;
	memcpy(destinationEntityNbr.buffer + entityNbrPad, cursor,
			entityNbrLength);
	cursor += entityNbrLength;
	bytesRemaining -= entityNbrLength;
#if CFDPDEBUG
printf("...parsed the PDU...\n"); 
#endif

	/*	Check CRC if necessary.					*/

	if (crcIsPresent)
	{
#if CFDPDEBUG
printf("...computing CRC...\n"); 
#endif
		if (bytesRemaining < 2)
		{
			return 0;	/*	Malformed PDU.		*/
		}

		/*	Omit CRC from PDU processing.			*/

		bytesRemaining -= 2;
		length -= 2;

		/*	Check the CRC.					*/

		memcpy((char *) &deliveredCRC, buf + length, 2);
		deliveredCRC = ntohs(deliveredCRC);
		computedCRC = computeCRC(buf, length);
		if (computedCRC != deliveredCRC)
		{
#if CFDPDEBUG
printf("...CRC validation failed...\n"); 
#endif
			return 0;	/*	Corrupted PDU.		*/
		}
	}

	/*	PDU is known not to be corrupt, so process it.		*/

#if CFDPDEBUG
printf("...PDU known not to be corrupt...\n"); 
#endif
	if (modeIsUnacknowledged == 0)	/*	Unusable PDU.		*/
	{
#if CFDPDEBUG
printf("...wrong CFDP transmission mode...\n"); 
#endif
		return handleFault(&transactionId,
				CfdpInvalidTransmissionMode, &handler);
	}

	if (cfdpvdb->watching & WATCH_q)
	{
		iwatch('q');
	}

	memcpy((char *) &transactionId.sourceEntityNbr,
			(char *) &sourceEntityNbr, sizeof(CfdpNumber));
	memcpy((char *) &transactionId.transactionNbr,
			(char *) &transactionNbr, sizeof(CfdpNumber));
	if (direction == 1)		/*	Return traffic.		*/
	{
		directiveCode = *cursor;
		cursor++;
		bytesRemaining--;
		if (directiveCode != 5)	/*	Must be Finish.		*/
		{
#if CFDPDEBUG
printf("...PDU type is invalid (must be Finish)...\n"); 
#endif
			return 0;
		}

		/*	Handle Finish PDU.				*/

		CHKERR(sdr_begin_xn(sdr));
		fduObj = findOutFdu(&transactionId, &outFduBuf, &fduElt);
		if (fduObj == 0
		|| outFduBuf.closureLatency == 0
		|| outFduBuf.eofPdu != 0)
		{
#if CFDPDEBUG
printf("...spurious Finish PDU...\n"); 
#endif
			sdr_exit_xn(sdr);
			return 0;
		}

		result = handleFinishPdu(cursor, bytesRemaining, &outFduBuf,
				fduObj);
		if (result < 0)
		{
			putErrmsg("UTI can't handle Finish PDU.", NULL);
			sdr_cancel_xn(sdr);
			return -1;
		}

		return sdr_end_xn(sdr);
	}

	/*	PDU is known to be forward traffic.			*/

	if (memcmp(destinationEntityNbr.buffer,
			cfdpConstants->ownEntityNbr.buffer, 8) != 0)
	{
#if CFDPDEBUG
printf("...PDU is misdirected...\n"); 
#endif
		return 0;		/*	Misdirected PDU.	*/
	}

	/*	Get InFdu, creating as necessary.			*/

	CHKERR(sdr_begin_xn(sdr));
	fduObj = findInFdu(&transactionId, &fduBuf, &fduElt, 1);
	if (fduObj == 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't create new inbound FDU.", NULL);
		return -1;
	}

	if (fduBuf.state == FduCanceled)
	{
		return sdr_end_xn(sdr); /*	Useless PDU.		*/
	}

	if (pduIsFileData)
	{
		result = handleFileDataPdu(cursor, bytesRemaining, &fduBuf,
				fduObj, fduElt, largeFile, recordStructure,
				haveMetadata);
		if (result < 0)
		{
			putErrmsg("UTI can't handle file data PDU.", NULL);
			sdr_cancel_xn(sdr);
			return -1;
		}

		return sdr_end_xn(sdr);
	}

	if (bytesRemaining < 1)
	{
		return sdr_end_xn(sdr); /*	Malformed PDU.		*/
	}

	directiveCode = *cursor;
	cursor++;
	bytesRemaining--;
	switch (directiveCode)
	{
	case 4:				/*	EOF PDU.		*/
		result = handleEofPdu(cursor, bytesRemaining, &fduBuf,
				fduObj, fduElt, largeFile);
		break;

	case 7:				/*	Metadata PDU.		*/
		result = handleMetadataPdu(cursor, bytesRemaining, &fduBuf,
				fduObj, fduElt, largeFile);
		break;

	default:			/*	Invalid PDU for unack.	*/
		return sdr_end_xn(sdr);
	}

	if (result < 0)
	{
		putErrmsg("UTI can't handle file directive PDU.", NULL);
		sdr_cancel_xn(sdr);
		return -1;
	}

	return sdr_end_xn(sdr);
}
