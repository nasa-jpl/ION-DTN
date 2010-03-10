/*
 *	sdrxn.c:	simple data recorder transaction system library.
 *
 *	Copyright (c) 2001-2007, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 *
 *	This library implements the transaction mechanism underlying the
 *	Simple Data Recorder system.
 *
 *	Modification History:
 *	Date	  Who	What
 *	4-3-96	  APS	Abstracted IPC services and task control.
 *	5-1-96	  APS	Ported to sparc-sunos4.
 *	12-20-00  SCB	Revised for sparc-sunos5.
 *	6-8-07    SCB	Divided sdr.c library into separable components.	
 */

#include "sdrP.h"
#include "lyst.h"
#include "sdrxn.h"

#ifndef SDR_TRACE
const char	*noTraceMsg = "Tracing disabled; recompile with -DSDR_TRACE.";
#endif
const char	*notInitMsg = "SDR system is not initialized.";
const char	*apiErrMsg = "NULL and/or invalid parameter(s).";
const char	*notFoundMsg = "Item not found";
const char	*noSemaphoreMsg = "Failed trying to create semaphore.";
const char	*noMemoryMsg = "Not enough available SDR working memory.";
const char	*violationMsg = "SDR boundaries or integrity violation.";
const char	*notInXnMsg = "Not in a transaction.";

#define SdrSchName	"sdrsch"

/*		SDR working memory area.				*/

static PsmView		sdrWorkingMemory;
static PsmPartition	sdrwm = &sdrWorkingMemory;
static int		sdrwmId = -1;
static long		sdrwmSize = 0;
static int		sdrMemory;	/*	For local lists only.	*/
static SdrControlHeader	*sch = NULL;
SdrMap			mapImage;

/*	*	Mutual exclusion functions	*	*	*	*/

static void	lockSdr(SdrState *sdr)
{
	if (sdr->sdrSemaphore != -1)
	{
		if (sm_SemTake(sdr->sdrSemaphore) < 0)
		{
			sm_Abort();
		}

		sdr->sdrOwnerTask = sm_TaskIdSelf();
		sdr->sdrOwnerThread = pthread_self();
		sdr->xnDepth = 1;
	}
}

void	takeSdr(SdrState *sdr)
{
	REQUIRE(sdr);
	if (sdr->sdrOwnerTask == sm_TaskIdSelf()
	&& pthread_equal(sdr->sdrOwnerThread, pthread_self()))
	{
		sdr->xnDepth++;
	}
	else
	{
		lockSdr(sdr);
	}
}

static void	unlockSdr(SdrState *sdr)
{
	sdr->sdrOwnerTask = -1;
	sdr->sdrOwnerThread = 0;
	sm_SemGive(sdr->sdrSemaphore);
}

void	releaseSdr(SdrState *sdr)
{
	REQUIRE(sdr);
	if (sdr->sdrOwnerTask == sm_TaskIdSelf()
	&& pthread_equal(sdr->sdrOwnerThread, pthread_self()))
	{
		sdr->xnDepth--;
		if (sdr->xnDepth == 0)
		{
			unlockSdr(sdr);
		}
	}
}

/*	*	SDR system administration functions	*	*	*/

MemAllocator		sdrmtake;
MemDeallocator		sdrmrlse;
MemAtoPConverter	sdrmatop;
MemPtoAConverter	sdrmptoa;

static void	*allocFromSdrMemory(char *fileName, int lineNbr, size_t length)
{
	PsmAddress	address;
	void		*block;

	address = Psm_zalloc(fileName, lineNbr, sdrwm, length);
	if (address == 0)
	{
		_putErrmsg(fileName, lineNbr, noMemoryMsg, NULL);
		return NULL;
	}

	block = psp(sdrwm, address);
	memset(block, 0, length);
	return block;
}

static void	releaseToSdrMemory(char *fileName, int lineNbr, void *block)
{
	Psm_free(fileName, lineNbr, sdrwm, psa(sdrwm, (char *) block));
}

static void	*sdrMemAtoP(unsigned long address)
{
	return (void *) psp(sdrwm, address);
}

static unsigned long sdrMemPtoA(void *pointer)
{
	return (unsigned long) psa(sdrwm, pointer);
}

int	Sdr_initialize(long wmSize, char *wmPtr, int wmKey, char *wmName)
{
	PsmAddress	controlHeaderAddress;
	int		wmIsPrivate = 0;

	if (sdrwmId != -1)	/*	Already initialized.		*/
        {
		return 0;
	}

	/*	Initialize the shared-memory libraries as necessary.	*/

	if (sm_ipc_init())
	{
		putErrmsg("Can't initialize IPC system.", NULL);
		return -1;	/*	Crash if can't initialize sm.	*/
	}

	/*	Attach to SDR's own shared working memory partition.	*/
   
	if (wmSize == 0)	/*	Use built-in default.		*/
	{
		wmSize = 1000000;
	}

	if (wmKey == SM_NO_KEY)
	{
		wmIsPrivate = 1;
		wmKey = SDR_SM_KEY;
	}

	if (wmName == NULL)
	{
		wmName = SDR_SM_NAME;
	}

	sdrmtake = allocFromSdrMemory;
	sdrmrlse = releaseToSdrMemory;
	sdrmatop = sdrMemAtoP;
	sdrmptoa = sdrMemPtoA;
	if (memmgr_open(wmKey, wmSize, &wmPtr, &sdrwmId, wmName, &sdrwm,
		&sdrMemory, sdrmtake, sdrmrlse, sdrmatop, sdrmptoa) < 0)
	{
		putErrmsg("Can't open SDR working memory.", NULL);
		return -1;
	}

	/*	Initialize SDR global control header if necessary.	*/

	sdrwmSize = wmSize;
	controlHeaderAddress = psm_locate(sdrwm, SdrSchName);
	if (controlHeaderAddress)	/*	Header already exists.	*/
	{
		sch = (SdrControlHeader *) psp(sdrwm, controlHeaderAddress);
		REQUIRE(sch);
		sm_SemUnwedge(sch->lock, 3);
		sm_list_unwedge(sdrwm, sch->sdrs, 3);
	}
	else
	{
		controlHeaderAddress = psm_zalloc(sdrwm,
				sizeof(SdrControlHeader));
		if (controlHeaderAddress == 0)
		{
			putErrmsg(noMemoryMsg, NULL);
			return -1;	/*	Not enough memory.	*/
		}

		/*	Initialize the control header.			*/

		sch = (SdrControlHeader *) psp(sdrwm, controlHeaderAddress);
		REQUIRE(sch);
		sch->lock = sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO);
		if (sch->lock == SM_SEM_NONE)
		{
			putErrmsg(noSemaphoreMsg, NULL);
			return -1;	/*	Can't create semaphore.	*/
		}

		sch->sdrs = sm_list_create(sdrwm);
		if (sch->sdrs == 0)
		{
			putErrmsg(noMemoryMsg, NULL);
			return -1;	/*	Not enough memory.	*/
		}

		sch->sdrwmIsPrivate = wmIsPrivate;
		sch->sdrwmKey = wmKey;

		/*	NOTE: psm_manage() succeeded, so wmName must
		 *	fit into 32-byte buffer.			*/

		strcpy(sch->sdrwmName, wmName);

		/*	Store location of control header in catalog
			of the shared SDR working memory, for access
			by subsequently started processes.		*/

		if (psm_catlg(sdrwm, SdrSchName, controlHeaderAddress) < 1)
		{
			putErrmsg("Can't catalog SDR control header.", NULL);
			return -1;	/*	Can't catalog.		*/
		}
	}

	return 0;
}

void	sdr_wm_usage(PsmUsageSummary *usage)
{
	REQUIRE(usage);
	psm_usage(sdrwm, usage);
}

void	sdr_shutdown()		/*	Ends SDR service on machine.	*/
{
	int		wmIsPrivate = 0;
	PsmAddress	elt;
	SdrState	*sdr;

	if (sdrwm == NULL)	/*	Nothing to do.			*/
	{
		return;
	}

	/*	SDR working memory has been allocated.			*/

	if (sch)		/*	Control header exists.		*/
	{
		wmIsPrivate = sch->sdrwmIsPrivate;

		/*	Deactivate all sdrs in list.			*/

		if (sch->lock != -1)
		{
			/*	Note: wait until sdrs list is no
			 *	longer in use before ending access
			 *	to the SDRs and destroying the sch
			 *	semaphore.				*/

			sm_SemTake(sch->lock);
			for (elt = sm_list_first(sdrwm, sch->sdrs); elt;
					elt = sm_list_next(sdrwm, elt))
			{
				sdr = (SdrState *)
					psp(sdrwm, sm_list_data(sdrwm, elt));
				REQUIRE(sdr);
				if (sdr->sdrSemaphore != -1)
				{
					sm_SemTake(sdr->sdrSemaphore);
					sm_SemEnd(sdr->sdrSemaphore);
					microsnooze(500000);
					sm_SemDelete(sdr->sdrSemaphore);
					sdr->sdrSemaphore = -1;
				}
			}

			sm_SemDelete(sch->lock);
		}

		psm_uncatlg(sdrwm, SdrSchName);
		psm_free(sdrwm, psa(sdrwm, sch));
		sch = NULL;
	}

	/*	Wipe out the working memory used for SDR operations
	 *	if necessary.						*/

	if (wmIsPrivate)
	{
		/*	Working memory is private, owned by SDR system.	*/

		memmgr_destroy(sdrwmId, &sdrwm);
	}
}

/*	*	Transaction utility functions	*	*	*	*/

/*	Logging is the mechanism that enables SDR transactions to be
	reversible.  The log for an SDR is a file in which are recorded
	all of the SDR data space updates in the scope of the current
	transaction; the last byte of the log file is the last byte
	of the last log entry.

	The log file is truncated to length zero at the termination of
	each transaction.  Therefore, if it is of non-zero length at
	startup and the SDR database is configured for SDR_IN_FILE then
	all complete log entries in the log file must be backed out of
	the db file before the database is restored to shared memory
	(assuming the database is also configured for SDR_IN_DRAM) and
	before it is accessed for any purpose: since this transaction
	was not ended, the database may be in an inconsistent state.

	An entry in the write-ahead log is an array of unsigned bytes.
	Its format is as follows, where W is WORD_SIZE (from platform.h;
	4 on a 32-bit machine, 8 on a 64-bit machine) and L is the number
	of bytes of data written.

	From offet	Until offset	Content is:

	0		W		start address within SDR
	W		(2*W)		value of L
	2*W		(2*W) + L	original data at this address

	When an SDR is configured to be reversible, the transaction
	owner's SdrView contains a list of the offsets (within the
	log file) of all log entries in the current transaction.
	This enables the transaction to be backed out quickly in
	the event that it is canceled: the log entries in the list
	are processed in reverse order, with the original data of
	each log entry being written back into the indicated start
	address.							*/

static int	reverseTransaction(Lyst logEntries, int logfile, int dbfile,
			char *dbsm)
{
	LystElt		elt;
	unsigned long	logEntryOffset;
	unsigned long	logEntryControl[2];	/*	Offset, length.	*/
	size_t		length;
	char		*buf;

	if (logfile == -1 || logEntries == NULL)
	{
		return 0;	/*	No reversal possible.		*/
	}

	for (elt = lyst_last(logEntries); elt; elt = lyst_prev(elt))
	{
		length = sizeof logEntryControl;
		logEntryOffset = (unsigned long) lyst_data(elt);
		if (lseek(logfile, logEntryOffset, SEEK_SET) < 0
		|| read(logfile, (char *) logEntryControl, length) < length)
		{
			putSysErrmsg("Can't locate log entry", NULL);
			return -1;
		}

		/*	Recover original data from log file.		*/

		length = logEntryControl[1];
		if (dbsm)
		{
			if (read(logfile, dbsm + logEntryControl[0], length)
					< length)
			{
				putSysErrmsg("Can't read log entry", NULL);
				return -1;
			}

			/*	If database is also on file, recover.	*/

			if (dbfile != -1)
			{
				/*	Use the sm database as buffer.	*/

				if (lseek(dbfile, logEntryControl[0], SEEK_SET)
						< 0
				|| write(dbfile, dbsm + logEntryControl[0],
						length) < length)
				{
					putSysErrmsg("Can't reverse log entry",
							NULL);
					return -1;
				}
			}
		}
		else	/*	Must create buffer for recovered data.	*/
		{
			if (dbfile != -1)
			{
				buf = MTAKE(length);
				if (buf == NULL)
				{
					putErrmsg("Log entry too big.",
						itoa(length));
					return -1;
				}

				if (read(logfile, buf, length) < length)
				{
					putSysErrmsg("Can't read log entry",
						NULL);
					MRELEASE(buf);
					return -1;
				}

				if (lseek(dbfile, logEntryControl[0], SEEK_SET)
						< 0
				|| write(dbfile, buf, length) < length)
				{
					putSysErrmsg("Can't reverse log entry",
							NULL);
					MRELEASE(buf);
					return -1;
				}

				MRELEASE(buf);
			}
		}
	}

	return 0;
}

static void	clearTransaction(Sdr sdrv)
{
	char	logfilename[PATHLENMAX + 1 + 32 + 1 + 6 + 1];

	if (sdrv->logfile != -1)
	{
		close(sdrv->logfile);
		sdrv->logfile = -1;
	}

	if (sdrv->sdr->configFlags & SDR_REVERSIBLE)
	{
		sprintf(logfilename, "%s%c%s.sdrlog", sdrv->sdr->pathName,
				ION_PATH_DELIMITER, sdrv->sdr->name);
		sdrv->logfile = open(logfilename, O_RDWR | O_CREAT | O_TRUNC,
				0777);
		if (sdrv->logfile == -1)
		{
			putSysErrmsg("Can't open log file", logfilename);
		}
	}

	sdrv->logfileLength = 0;
	if (sdrv->logEntries)
	{
		lyst_clear(sdrv->logEntries);
	}

	if (sdrv->knownObjects)
	{
		lyst_clear(sdrv->knownObjects);
	}
}

static void	handleUnrecoverableError(Sdr sdrv)
{
	putErrmsg("Unrecoverable SDR error.", NULL);
#ifdef IN_FLIGHT
	sdr_abort(sdrv);
#endif
	sm_Abort();
}

static void	terminateXn(Sdr sdrv)
{
	SdrState	*sdr = sdrv->sdr;

	if (sdr->xnCanceled)
	{
		sdr->xnCanceled = 0;
		if (sdr->configFlags & SDR_REVERSIBLE)
		{
			if (reverseTransaction(sdrv->logEntries, sdrv->logfile,
					sdrv->dbfile, sdrv->dbsm) < 0)
			{
				handleUnrecoverableError(sdrv);
			}
		}
		else	/*	Can't back out; if data modified, bail.	*/
		{
			if (sdrv->modified)
			{
				handleUnrecoverableError(sdrv);
			}
		}
	}

	/*	Database is in a consistent state, one way or another.	*/

	clearTransaction(sdrv);
	unlockSdr(sdr);
}

void	crashXn(Sdr sdrv)
{
	SdrState	*sdr;

	if (sdr_in_xn(sdrv))
	{
		sdr = sdrv->sdr;
#ifdef SDR_TRACE
		sptrace_log_memo(sdrv->trace, 0, "transaction aborted",
				sdrv->currentSourceFileName,
				sdrv->currentSourceFileLine);
#endif
		putErrmsg("Transaction aborted.", NULL);
		sdr->xnCanceled = 1;	/*	Force reversal.		*/
		sdr->xnDepth = 0;	/*	Unlock is immediate.	*/
		terminateXn(sdrv);
	}
}

/*	*	SDR database administration functions.	*	*	*/

static int	reloadLogEntries(Lyst logEntries, int logfile)
{
	int		logFileLength;
	unsigned long	logEntryOffset = 0;
	unsigned long	logEntryControl[2];	/*	Offset, length.	*/
	size_t		length = sizeof logEntryControl;
	size_t		lengthRead;
	size_t		endOfEntry;

	logFileLength = lseek(logfile, 0, SEEK_END);
	while (1)
	{
		if (logEntryOffset + length > logFileLength)
		{
			/*	From this point in the file, there
			 *	are too few remaining bytes to contain
			 *	the control fields for another log
			 *	entry.  So all complete log entries
			 *	in the file have been reloaded.		*/

			 return 0;
		}

		if (lseek(logfile, logEntryOffset, SEEK_SET) < 0)
		{
			putSysErrmsg("Can't move to next log entry", NULL);
			return -1;
		}

		lengthRead = read(logfile, (char *) logEntryControl, length);
		if (lengthRead < length)
		{
			putSysErrmsg("Can't read log entry", NULL);
			return -1;
		}

		endOfEntry = logEntryOffset + length + logEntryControl[1];
		if (endOfEntry > logFileLength)
		{
			/*	The log entry control fields for this
			 *	log entry were written to the log
			 *	file, but the program was interrupted
			 *	before the old data bytes were written.
			 *	Since writing log entries always
			 *	precedes writing to the database, we
			 *	know that this log entry not only
			 *	cannot be reversed (we can't retrieve
			 *	the old data) but need not be reversed
			 *	(the new data weren't written either).
			 *	So we ignore this final log entry, and
			 *	we know that there are no subsequent
			 *	log entries to reload.			*/

			return 0;
		}

		if (lyst_insert_last(logEntries, (void *) logEntryOffset)
				== NULL)
		{
			putErrmsg("Can't reload log entry.", NULL);
			return -1;
		}

		logEntryOffset = endOfEntry;
	}
}

static long	getBigBuffer(char **buffer)
{
	long	bufsize = sdrwmSize;

	/*	Temporarily take large buffer from SDR working memory.	*/

	while (1)
	{
		bufsize = bufsize / 2;
		if (bufsize == 0)
		{
			return -1;
		}

		*buffer = MTAKE(bufsize);
		if (*buffer)
		{
			return bufsize;
		}
	}
}

static void	initSdrMap(SdrMap *map, SdrState *sdr)
{
	map->catalogue = 0;
	map->status = INITIALIZED;
	map->sdrSize = sdr->sdrSize;
	map->heapSize = sdr->heapSize;
	map->startOfSmallPool = sizeof(SdrMap);
	map->endOfSmallPool = map->startOfSmallPool;
	memset(map->firstSmallFree, 0, sizeof map->firstSmallFree);
	map->endOfLargePool = sdr->sdrSize;
	map->startOfLargePool = map->endOfLargePool;
	memset(map->firstLargeFree, 0, sizeof map->firstLargeFree);
	map->unassignedSpace = map->startOfLargePool - map->endOfSmallPool;
}

static int	createDbFile(SdrState *sdr, char *dbfilename)
{
	long	bufsize;
	char	*buffer;
	int	dbfile;
	long	lengthRemaining;
	size_t	lengthToWrite;

	bufsize = getBigBuffer(&buffer);
	if (bufsize < 0)
	{
		putErrmsg("Can't get buffer in sdrwm.", NULL);
		return -1;
	}

	memset(buffer, 0 , sizeof buffer);
	dbfile = open(dbfilename, O_RDWR | O_CREAT, 0777);
	if (dbfile == -1)
	{
		MRELEASE(buffer);
		putSysErrmsg("Can't create database file", dbfilename);
		return -1;
	}

	lengthRemaining = sdr->sdrSize;
	while (lengthRemaining > 0)
	{
		lengthToWrite = lengthRemaining;
		if (lengthToWrite > bufsize)
		{
			lengthToWrite = bufsize;
		}

		if (write(dbfile, buffer, lengthToWrite) < lengthToWrite)
		{
			close(dbfile);
			unlink(dbfilename);
			MRELEASE(buffer);
			putSysErrmsg("Can't extend database file", dbfilename);
			return -1;
		}
	
		lengthRemaining -= lengthToWrite;
	}

	MRELEASE(buffer);
	initSdrMap(&mapImage, sdr);
	if (lseek(dbfile, 0, SEEK_SET) < 0
	|| write(dbfile, (char *) &mapImage, sizeof mapImage) < sizeof mapImage
	|| lseek(dbfile, 0, SEEK_SET) < 0)
	{
		close(dbfile);
		unlink(dbfilename);
		putSysErrmsg("Can't initialize database file", dbfilename);
		return -1;
	}

	return dbfile;
}

int	sdr_load_profile(char *name, int configFlags, long heapWords,
		int memKey, char *pathName)
{
	PsmAddress	elt;
	SdrState	*sdr;
	PsmAddress	newSdrAddress;
	long		limit;
	char		logfilename[PATHLENMAX + 1 + 32 + 1 + 6 + 1];
	int		logfile = -1;
	Lyst		logEntries = NULL;
	char		dbfilename[PATHLENMAX + 1 + 32 + 1 + 3 + 1];
	int		dbfile = -1;
	char		*dbsm;
	int		dbsmId;

	REQUIRE(sdrwm);
	REQUIRE(sch);
	REQUIRE(name);
	REQUIRE(pathName);
	sm_SemTake(sch->lock);
	for (elt = sm_list_first(sdrwm, sch->sdrs); elt;
			elt = sm_list_next(sdrwm, elt))
	{
		sdr = (SdrState *) psp(sdrwm, sm_list_data(sdrwm, elt));
		REQUIRE(sdr);
		if (strcmp(sdr->name, name) == 0)
		{
			sm_SemGive(sch->lock);
			if (sdr->configFlags == configFlags
			&& sdr->initHeapWords == heapWords
			&& (sdr->sdrKey == memKey || memKey == SM_NO_KEY)
			&& strcmp(sdr->pathName, pathName) == 0)
			{
				sm_SemUnwedge(sdr->sdrSemaphore, 3);
				return 1;	/*	Profile loaded.	*/
			}

			errno = EINVAL;
			putSysErrmsg("Wrong profile for this SDR", name);
			return -1;
		}
	}

	/*	This is an SDR profile that's not currently loaded.	*/

	newSdrAddress = psm_zalloc(sdrwm, sizeof(SdrState));
	if (newSdrAddress == 0)
	{
		sm_SemGive(sch->lock);
		putErrmsg("Can't allocate memory for SDR state.", NULL);
		return -1;
	}

	sdr = (SdrState *) psp(sdrwm, newSdrAddress);
	memset(sdr, 0, sizeof(SdrState));
	limit = sizeof(sdr->name) - 1;
	strncpy(sdr->name, name, limit);
	sdr->configFlags = configFlags;
	sdr->initHeapWords = heapWords;
	sdr->heapSize = heapWords * WORD_SIZE;
	sdr->sdrSize = sdr->heapSize + sizeof(SdrMap);
	if (memKey == SM_NO_KEY)
	{
		memKey = sm_GetUniqueKey();
	}

	sdr->sdrKey = memKey;
	sdr->sdrSemaphore = sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO);
	if (sdr->sdrSemaphore == SM_SEM_NONE)
	{
		psm_free(sdrwm, newSdrAddress);
		sm_SemGive(sch->lock);
		putErrmsg("Can't create transaction semaphore for SDR.", NULL);
		return -1;
	}

	sdr->sdrOwnerTask = -1;
	sdr->sdrOwnerThread = 0;
	sdr->traceKey = sm_GetUniqueKey();
	sdr->traceSize = 0;
	limit = sizeof(sdr->pathName) - 1;
	strncpy(sdr->pathName, pathName, limit);

	/*	Add SDR to linked list of defined SDRs.			*/

	sdr->sdrsElt = sm_list_insert_last(sdrwm, sch->sdrs, newSdrAddress);
	if (sdr->sdrsElt == 0)
	{
		psm_free(sdrwm, newSdrAddress);
		sm_SemGive(sch->lock);
		putErrmsg("Can't insert SDR state into list of SDRs.", NULL);
		return -1;
	}

	/*	If database exists, back out any current transaction.
		If not, create the database and initialize it.		*/

	if (configFlags & SDR_REVERSIBLE)
	{
		sprintf(logfilename, "%s%c%s.sdrlog", sdr->pathName,
				ION_PATH_DELIMITER, name);
		logfile = open(logfilename, O_RDWR | O_CREAT | O_APPEND, 0777);
		if (logfile == -1)
		{
			putSysErrmsg("Can't open log file", logfilename);
			return -1;
		}

		logEntries = lyst_create_using(sdrMemory);
		if (logEntries == 0)
		{
			close(logfile);
			putErrmsg(noMemoryMsg, NULL);
			return -1;
		}

		if (reloadLogEntries(logEntries, logfile) < 0)
		{
			lyst_destroy(logEntries);
			close(logfile);
			putErrmsg("Can't reload log entries.", NULL);
			return -1;
		}
	}

	if (configFlags & SDR_IN_FILE)
	{
		sprintf(dbfilename, "%s%c%s.sdr", sdr->pathName,
			       ION_PATH_DELIMITER, name);
		dbfile = open(dbfilename, O_RDWR, 0777);
		if (dbfile == -1)
		{
			dbfile = createDbFile(sdr, dbfilename);
			if (dbfile == -1)
			{
				if (logfile != -1) close(logfile);
				if (logEntries) lyst_destroy(logEntries);
				putErrmsg("Can't have file-based database",
						NULL);
				return -1;
			}
		}
		else	/*	Database file exists.			*/
		{
			if (reverseTransaction(logEntries, logfile, dbfile,
					NULL) < 0)
			{
				close(dbfile);
				if (logfile != -1) close(logfile);
				if (logEntries) lyst_destroy(logEntries);
				putErrmsg("Can't reverse log entries.", NULL);
				return -1;
			}
		}
	}

	if (configFlags & SDR_IN_DRAM)
	{
		dbsm = NULL;
		switch (sm_ShmAttach(sdr->sdrKey, sdr->sdrSize, &dbsm, &dbsmId))
		{
		case -1:	/*	Error.				*/
			if (dbfile != -1) close(dbfile);
			if (logfile != -1) close(logfile);
			if (logEntries) lyst_destroy(logEntries);
			putErrmsg("Can't attach to database partition.", NULL);
			return -1;
	
		case 0:		/*	Reattaching to existing SDR.	*/
			if (dbfile != -1)
			{
				/*	File is authoritative.		*/

				if (read(dbfile, dbsm, sdr->sdrSize)
						< sdr->sdrSize)
				{
					close(dbfile);
					if (logfile != -1) close(logfile);
					if (logEntries)
						lyst_destroy(logEntries);
					putErrmsg("Can't load db from file.",
							NULL);
					return -1;
				}

				break;
			}
	
			/*	Back transaction out of memory if nec.	*/
	
			if (reverseTransaction(logEntries, logfile, -1, dbsm)
					< 0)
			{
				if (dbfile != -1) close(dbfile);
				if (logfile != -1) close(logfile);
				if (logEntries) lyst_destroy(logEntries);
				putErrmsg("Can't reverse log entries.", NULL);
				return -1;
			}

			break;

		default:	/*	Newly allocated partition.	*/
			if (dbfile != -1)
			{
				/*	File is authoritative.		*/

				if (read(dbfile, dbsm, sdr->sdrSize)
						< sdr->sdrSize)
				{
					close(dbfile);
					if (logfile != -1) close(logfile);
					if (logEntries)
						lyst_destroy(logEntries);
					putErrmsg("Can't load db from file.",
							NULL);
					return -1;
				}

				break;
			}

			/*	Just initialize the database.		*/

			initSdrMap((SdrMap *) dbsm, sdr);
		}
	}

	if (logfile != -1)
	{
		close(logfile);
	}

	if (logEntries)
	{
		lyst_destroy(logEntries);
	}

	if (dbfile != -1)
	{
		close(dbfile);
	}

	sm_SemGive(sch->lock);
	return 0;
}

int	sdr_reload_profile(char *name, int configFlags, long heapWords,
		int memKey, char *pathName)
{
	PsmAddress	elt;
	PsmAddress	sdrAddress;
	SdrState	*sdr;

	REQUIRE(sdrwm);
	REQUIRE(sch);
	REQUIRE(name);
	REQUIRE(pathName);
	sm_SemTake(sch->lock);
	for (elt = sm_list_first(sdrwm, sch->sdrs); elt;
			elt = sm_list_next(sdrwm, elt))
	{
		sdrAddress = sm_list_data(sdrwm, elt);
		sdr = (SdrState *) psp(sdrwm, sdrAddress);
		REQUIRE(sdr);
		if (strcmp(sdr->name, name) == 0)
		{
			break;	/*	Out of sdrs loop.		*/
		}
	}

	if (elt)		/*	SDR was found in list.		*/
	{
		if (sdr->configFlags != configFlags
		|| sdr->initHeapWords != heapWords
		|| (sdr->sdrKey != memKey && memKey != SM_NO_KEY)
		|| strcmp(sdr->pathName, pathName) != 0)
		{
			sm_SemGive(sch->lock);
			errno = EINVAL;
			putSysErrmsg("Can't unload SDR: profile conflict",
					name);
			return -1;
		}

		/*	Profile for this SDR is currently loaded, so
		 *	we have to unload that profile in order to
		 *	force reversal of any incomplete transaction
		 *	that is currently in progress.			*/

		sm_SemDelete(sdr->sdrSemaphore);
		psm_free(sdrwm, sdrAddress);
		sm_list_delete(sdrwm, elt, NULL, NULL);
	}

	/*	Profile for this SDR is now known to be unloaded.	*/

	sm_SemGive(sch->lock);
	return sdr_load_profile(name, configFlags, heapWords, memKey, pathName);
}

static void	deleteObjectExtent(LystElt elt, void *userData)
{
	MRELEASE(lyst_data(elt));
}

Sdr	Sdr_start_using(char *name)
{
	PsmAddress	elt;
	SdrState	*sdr;
	PsmAddress	sdrViewAddress;
	SdrView		*sdrv;
	char		dbfilename[PATHLENMAX + 1 + 32 + 1 + 3 + 1];
	char		logfilename[PATHLENMAX + 1 + 32 + 1 + 6 + 1];

	REQUIRE(sdrwm);
	REQUIRE(sch);
	REQUIRE(name);
	sm_SemTake(sch->lock);
	for (elt = sm_list_first(sdrwm, sch->sdrs); elt;
			elt = sm_list_next(sdrwm, elt))
	{
		sdr = (SdrState *) psp(sdrwm, sm_list_data(sdrwm, elt));
		REQUIRE(sdr);
		if (strcmp(sdr->name, name) == 0)
		{
			break;
		}
	}

	if (elt == 0)	/*	Reached the end of the list, no match.	*/
	{
		sm_SemGive(sch->lock);
		errno = EINVAL;
		putSysErrmsg(notFoundMsg, name);
		return NULL;
	}

	/*	SDR is defined;	create view for access.			*/

	sdrViewAddress = psm_zalloc(sdrwm, sizeof(SdrView));
	if (sdrViewAddress == 0)
	{
		sm_SemGive(sch->lock);
		putErrmsg(noMemoryMsg, NULL);
		return NULL;
	}

	sdrv = (SdrView *) psp(sdrwm, sdrViewAddress);
	sdrv->sdr = sdr;
	if (sdr->configFlags & SDR_IN_FILE)
	{
		sprintf(dbfilename, "%s%c%s.sdr", sdr->pathName,
				ION_PATH_DELIMITER, name);
		sdrv->dbfile = open(dbfilename, O_RDWR, 0777);
		if (sdrv->dbfile == -1)
		{
			sm_SemGive(sch->lock);
			putSysErrmsg("Can't open database file", dbfilename);
			return NULL;
		}
	}
	else
	{
		sdrv->dbfile = -1;
	}

	if (sdr->configFlags & SDR_IN_DRAM)
	{
		sdrv->dbsm = NULL;
		if (sm_ShmAttach(sdr->sdrKey, sdr->sdrSize, &(sdrv->dbsm),
					&(sdrv->dbsmId)) < 0)
		{
			sm_SemGive(sch->lock);
			putErrmsg("Can't attach to database in memory.",
					itoa(sdr->sdrKey));
			return NULL;
		}
	}

	if (sdr->configFlags & SDR_REVERSIBLE)
	{
		sprintf(logfilename, "%s%c%s.sdrlog", sdr->pathName,
				ION_PATH_DELIMITER, name);
		sdrv->logfile = open(logfilename, O_RDWR | O_CREAT | O_APPEND,
				0777);
		if (sdrv->logfile == -1)
		{
			sm_SemGive(sch->lock);
			putSysErrmsg("Can't open log file", logfilename);
			return NULL;
		}

		sdrv->logEntries = lyst_create_using(sdrMemory);
		if (sdrv->logEntries == 0)
		{
			sm_SemGive(sch->lock);
			putErrmsg(noMemoryMsg, NULL);
			return NULL;
		}
	}
	else
	{
		sdrv->logfile = -1;
	}

	if (sdr->configFlags & SDR_BOUNDED)
	{
		sdrv->knownObjects = lyst_create_using(sdrMemory);
		if (sdrv->knownObjects == 0)
		{
			sm_SemGive(sch->lock);
			putErrmsg(noMemoryMsg, NULL);
			return NULL;
		}

		lyst_delete_set(sdrv->knownObjects, deleteObjectExtent, NULL);
	}

	sdrv->trace = NULL;
	sdrv->currentSourceFileName = NULL;
	sdrv->currentSourceFileLine = 0;
	sm_SemGive(sch->lock);
	return sdrv;
}

char	*sdr_name(Sdr sdrv)
{
	REQUIRE(sdrv);
	return sdrv->sdr->name;
}

long	sdr_heap_size(Sdr sdrv)
{
	REQUIRE(sdrv);
	return sdrv->sdr->heapSize;
}

void	sdr_stop_using(Sdr sdrv)
{
	REQUIRE(sdrv);

	/*	Should only stop using SDR when not currently using it,
	 *	i.e., when not currently in the midst of a transaction.	*/

	if (sdr_in_xn(sdrv))
	{
		crashXn(sdrv);
	}

	/*	Terminate all local SDR state and destroy the Sdr.	*/

	if (sdrv->dbfile != -1)
	{
		close(sdrv->dbfile);
	}

	if (sdrv->dbsm)
	{
		sm_ShmDetach(sdrv->dbsm);
	}

	if (sdrv->logfile != -1)
	{
		close(sdrv->logfile);
	}

	if (sdrv->logEntries)
	{
		lyst_destroy(sdrv->logEntries);
	}

	if (sdrv->knownObjects)
	{
		lyst_destroy(sdrv->knownObjects);
	}

	psm_free(sdrwm, psa(sdrwm, sdrv));
}

void	sdr_abort(Sdr sdrv)
{
	REQUIRE(sdrv);
	sm_SemEnd(sdrv->sdr->sdrSemaphore);
	microsnooze(500000);
	sm_SemDelete(sdrv->sdr->sdrSemaphore);
	sdrv->sdr->sdrSemaphore = -1;
	sdr_shutdown();
}

void	sdr_destroy(Sdr sdrv)
{
	SdrState	*sdr;
	char		dbfilename[PATHLENMAX + 1 + 32 + 1 + 3 + 1];
	char		logfilename[PATHLENMAX + 1 + 32 + 1 + 6 + 1];
	char		*dbsm;
	int		dbsmId;

	REQUIRE(sdrv);

	/*	All tracing of this SDR must stop, as the SDR is
	 *	going to be destroyed.					*/

#ifdef SDR_TRACE
	if (sdrv->trace)
	{
		sptrace_stop(sdrv->trace);
	}
#endif

	/*	Destroy local access handle to this SDR.		*/

	sdr = sdrv->sdr;
	sm_SemDelete(sdr->sdrSemaphore);	/*	Interrupt.	*/
	sdr_stop_using(sdrv);
	sm_SemTake(sch->lock);

	/*	Destroy file copy of heap if any.			*/

	if (sdr->configFlags & SDR_IN_FILE)
	{
		sprintf(dbfilename, "%s%c%s.sdr", sdr->pathName,
				ION_PATH_DELIMITER, sdr->name);
		unlink(dbfilename);
	}

	/*	Destroy memory copy of heap if any.			*/

	if (sdr->configFlags & SDR_IN_DRAM)
	{
		switch (sm_ShmAttach(sdr->sdrKey, sdr->sdrSize, &dbsm, &dbsmId))
		{
		case -1:
			break;

		default:
			sm_ShmDestroy(dbsmId);
		}
	}

	/*	Destroy transaction log file if any.			*/

	if (sdr->configFlags & SDR_REVERSIBLE)
	{
		sprintf(logfilename, "%s%c%s.sdrlog", sdr->pathName,
				ION_PATH_DELIMITER, sdr->name);
		unlink(logfilename);
	}

	/*	Unload profile and destroy it.				*/

	sm_list_delete(sdrwm, sdr->sdrsElt, NULL, NULL);
	psm_free(sdrwm, psa(sdrwm, sdr));
	sm_SemGive(sch->lock);
}

/*	*	Low-level transaction functions		*	*	*/

void	sdr_begin_xn(Sdr sdrv)
{
	REQUIRE(sdrv);
	takeSdr(sdrv->sdr);
	sdrv->modified = 0;
}

int	sdr_in_xn(Sdr sdrv)
{
	return (sdrv != NULL && sdrv->sdr != NULL
		&& sdrv->sdr->sdrOwnerTask == sm_TaskIdSelf()
		&& pthread_equal(sdrv->sdr->sdrOwnerThread, pthread_self()));
}

void	sdr_exit_xn(Sdr sdrv)
{
	SdrState	*sdr;

	REQUIRE(sdrv);
	sdr = sdrv->sdr;
	if (sdr_in_xn(sdrv))
	{
		sdr->xnDepth--;
		if (sdr->xnDepth == 0)
		{
			if (sdrv->modified)
			{
				/*	Can't simply exit from a
				 *	transaction during which
				 *	data were modified - must
				 *	either end or cancel.  This
				 *	is an implementation error.	*/

				handleUnrecoverableError(sdrv);
			}

			clearTransaction(sdrv);
			unlockSdr(sdr);
		}
	}
}

void	sdr_cancel_xn(Sdr sdrv)
{
	SdrState	*sdr = sdrv->sdr;

	if (sdr_in_xn(sdrv))
	{
		sdr->xnCanceled = 1;
		sdr->xnDepth--;
		if (sdr->xnDepth == 0)
		{
			terminateXn(sdrv);
		}
	}
}

int	sdr_end_xn(Sdr sdrv)
{
	SdrState	*sdr = sdrv->sdr;

	if (sdr_in_xn(sdrv))
	{
		sdr->xnDepth--;
		if (sdr->xnDepth == 0)
		{
			terminateXn(sdrv);
		}

		return 0;
	}

	return -1;
}

void	sdr_eject_xn(Sdr sdrv)
{
	SdrState	*sdr;

	/*	Emergency function to be used ONLY for system recovery
	 *	when you are *certain* that the owner of the SDR's
	 *	current transaction has crashed or is permanently
	 *	blocked without any chance of continuing.		*/

	if (sdrv != NULL)
	{
		sdr = sdrv->sdr;
		if (sdr != NULL)
		{
			sdr->xnCanceled = 1;
			sdr->xnDepth = 0;
			terminateXn(sdrv);
		}
	}
}

void	*sdr_pointer(Sdr sdrv, Address address)
{
	REQUIRE(sdrv);
	if ((sdrv->sdr->configFlags & SDR_IN_DRAM) == 0 || address <= 0)
	{
		return NULL;
	}

	return (void *) (sdrv->dbsm + address);
}

Address	sdr_address(Sdr sdrv, void *pointer)
{
	char	*ptr;

	REQUIRE(sdrv);
	ptr = (char *) pointer;
	if ((sdrv->sdr->configFlags & SDR_IN_DRAM) == 0 || ptr <= sdrv->dbsm)
	{
		return 0;
	}

	return (Address) (ptr - sdrv->dbsm);
}

/*	*	Low-level I/O functions		*	*	*	*/

#ifdef NO_SDRMGT

Object	_sdrzalloc(Sdr sdrv, size_t nbytes)
{
	return 0;
}

Object	_sdrmalloc(Sdr sdrv, size_t nbytes)
{
	return 0;
}

void	_sdrfree(Sdr sdrv, Object, PutSrc);
{
	return;
}

int	sdrBoundaryViolated(SdrView *sdrv, Address offset, long length)
{
	return 0;
}

#endif

void	_sdrput(char *file, int line, Sdr sdrv, Address into, char *from,
		long length, PutSrc src)
{
	SdrState	*sdr;
	Address		to;
	unsigned long	logEntryControl[2];
	char		*buffer;
	long		logOffset;

	if (length == 0)
	{
		return;
	}

	REQUIRE(length > 0);
	REQUIRE(sdrv);
	REQUIRE(from);
	REQUIRE(into >= 0);
	sdr = sdrv->sdr;
	to = into + length;
	if (to > sdr->sdrSize)
	{
		_putErrmsg(file, line, violationMsg, "write");
		crashXn(sdrv);
		return;
	}

	if (sdr->configFlags & SDR_BOUNDED && src == UserPut)
	{
		if (sdrBoundaryViolated(sdrv, into, length))
		{
			_putErrmsg(file, line, violationMsg, "write");
			crashXn(sdrv);
			return;
		}
	}

	if (sdr->configFlags & SDR_REVERSIBLE)
	{
		logEntryControl[0] = into;
		logEntryControl[1] = length;
		if (write(sdrv->logfile, (char *) logEntryControl,
			sizeof logEntryControl) < sizeof logEntryControl)
		{
			_putSysErrmsg(file, line, "Can't write logEntryControl",
					NULL);
			crashXn(sdrv);
			return;
		}

		if (sdr->configFlags & SDR_IN_DRAM)
		{
			if (write(sdrv->logfile, sdrv->dbsm + into, length)
					< length)
			{
				_putSysErrmsg(file, line, "Can't write log \
entry", itoa(length));
				crashXn(sdrv);
				return;
			}
		}
		else
		{
			buffer = MTAKE(length);
			if (buffer == NULL)
			{
				_putErrmsg(file, line, "Not enough memory for \
log entry.", itoa(length));
				crashXn(sdrv);
				return;
			}

			if (sdr->configFlags & SDR_IN_FILE)
			{
				if (lseek(sdrv->dbfile, into, SEEK_SET) < 0
				|| read(sdrv->dbfile, buffer, length) < length)
				{
					MRELEASE(buffer);
					_putSysErrmsg(file, line, "Can't read \
old data", itoa(length));
					crashXn(sdrv);
					return;
				}
			}

			if (write(sdrv->logfile, buffer, length) < length)
			{
				MRELEASE(buffer);
				_putSysErrmsg(file, line, "Can't write log \
entry", itoa(length));
				crashXn(sdrv);
				return;
			}

			MRELEASE(buffer);
		}

		logOffset = sdrv->logfileLength;
		if (lyst_insert_last(sdrv->logEntries, (void *) logOffset)
				== NULL)
		{
			_putErrmsg(file, line, "Can't note transaction log \
entry.", NULL); crashXn(sdrv);
			return;
		}

		sdrv->logfileLength += (length + sizeof logEntryControl);
	}

	if (sdr->configFlags & SDR_IN_FILE)
	{
		if (lseek(sdrv->dbfile, into, SEEK_SET) < 0
		|| write(sdrv->dbfile, from, length) < length)
		{
			_putSysErrmsg(file, line, "Can't write to database",
					itoa(length));
			crashXn(sdrv);
			return;
		}
	}

	if (sdr->configFlags & SDR_IN_DRAM)
	{
		memcpy(sdrv->dbsm + into, from, length);
	}

	sdrv->modified = 1;
}

void	Sdr_write(char *file, int line, Sdr sdrv, Address into, char *from,
		long length)
{
	if (sdr_in_xn(sdrv) == 0)
	{
		_putErrmsg(file, line, notInXnMsg, NULL);
	}
	else
	{
		joinTrace(sdrv, file, line);
		_sdrput(file, line, sdrv, into, from, length, UserPut);
	}
}

void	_sdrfetch(Sdr sdrv, char *into, Address from, long length)
{
	SdrState	*sdr;
	Address		to;

	if (length == 0)			/*	Nothing to do.	*/
	{
		return;
	}

	REQUIRE(length > 0);
	REQUIRE(sdrv);
	REQUIRE(into);
	REQUIRE(from >= 0);
	sdr = sdrv->sdr;
	to = from + length;
	if (to > sdr->sdrSize)
	{
		putErrmsg(violationMsg, "read");
		crashXn(sdrv);			/*	Releases SDR.	*/
		return;
	}

	if (sdr->configFlags & SDR_IN_DRAM)
	{
		memcpy(into, sdrv->dbsm + from, length);
	}
	else
	{
		if (sdr->configFlags & SDR_IN_FILE)
		{
			if (lseek(sdrv->dbfile, from, SEEK_SET) < 0
			|| read(sdrv->dbfile, into, length) < length)
			{
				putSysErrmsg("Database read failed",
						itoa(length));
				crashXn(sdrv);	/*	Releases SDR.	*/
				return;
			}
		}
	}
}

void	sdr_read(Sdr sdrv, char *into, Address from, long length)
{
	SdrState	*sdr;

	REQUIRE(sdrv);
	sdr = sdrv->sdr;
	takeSdr(sdr);
	_sdrfetch(sdrv, into, from, length);
	releaseSdr(sdr);
}
