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

#ifndef SDR_SEMKEY
#define SDR_SEMKEY	(0xeee0)
#endif

static PsmPartition	_sdrwm(sm_WmParms *parms);

#ifndef SDR_TRACE
char	*_noTraceMsg()
{
	return "Tracing disabled; recompile with -DSDR_TRACE.";
}
#endif

char	*_notInXnMsg()
{
	return "sdr_in_xn(sdrv)";
}

char	*_apiErrMsg()
{
	return "API values";
}

char	*_noMemoryMsg()
{
	return "Not enough available SDR working memory.";
}

char	*_violationMsg()
{
	return "SDR boundaries or integrity violation.";
}

static char	*_SdrSchName()
{
	return "sdrsch";
}

/*	*	*	SDR global system variables	*	*	*/

static sm_SemId	_sdrlock(int delete)
{
	/*	This is the mutex for the SDR system as a whole; it's
	 *	global to all SDR operations on any single computer.	*/

	static sm_SemId	lock = SM_SEM_NONE;
	int		n;

	if (delete)
	{
		if (lock != SM_SEM_NONE)
		{
			sm_SemDelete(lock);
			lock = SM_SEM_NONE;
		}
	}
	else
	{
		if (lock == SM_SEM_NONE)
		{
			lock = sm_SemCreate(SDR_SEMKEY, SM_SEM_FIFO);
			if (lock != SM_SEM_NONE)
			{
				/*	Try unwedge twice, in case
				 *	there's a conflict with another
				 *	initializing node.		*/

				for (n = 2; n > 0; n--)
				{
					if (sm_SemUnwedge(lock, 3) < 0)
					{
						microsnooze(100000);
					}
					else
					{
						break;
					}
				}

				if (n == 0)
				{
					lock = SM_SEM_NONE;
				}
			}
		}
	}

	return lock;
}

static int	_sdrMemory(int *memmgrIdx)
{
	static int	idx = -1;

	if (memmgrIdx)
	{
		idx = *memmgrIdx;
	}

	return idx;
}

static long	_sdrwmSize(long *wmsize)
{
	static long	size = 0;

	if (wmsize)
	{
		size = *wmsize;
	}

	return size;
}

static SdrControlHeader	*_sch(SdrControlHeader **schp)
{
	static SdrControlHeader	*sch = NULL;
	sm_SemId		lock;
	PsmPartition		sdrwm;
	PsmAddress		elt;
	SdrState		*sdr;
	PsmAddress		controlHeaderAddress;

	if (schp == NULL)		/*	Just retrieving.	*/
	{
		return sch;
	}

	/*	Managing the SdrControlHeader.				*/

	if (*schp)			/*	Shutdown.		*/
	{
		/*	Deactivate all sdrs in list.			*/

		if (sch == NULL)
		{
			return NULL;	/*	Nothing to do.		*/
		}

		sdrwm = _sdrwm(NULL);
		lock = _sdrlock(0);
		if (lock != SM_SEM_NONE)
		{
			/*	Note: wait until sdrs list is no
			 *	longer in use before ending access
			 *	to the SDRs.				*/

			sm_SemTake(lock);
			for (elt = sm_list_first(sdrwm, sch->sdrs); elt;
					elt = sm_list_next(sdrwm, elt))
			{
				sdr = (SdrState *)
					psp(sdrwm, sm_list_data(sdrwm, elt));
				CHKNULL(sdr);
				if (sdr->sdrSemaphore != SM_SEM_NONE)
				{
					sm_SemTake(sdr->sdrSemaphore);
					sm_SemEnd(sdr->sdrSemaphore);
					microsnooze(500000);
					sm_SemDelete(sdr->sdrSemaphore);
					sdr->sdrSemaphore = SM_SEM_NONE;
				}
			}

			sm_SemGive(lock);
		}

		oK(psm_uncatlg(sdrwm, _SdrSchName()));
		psm_free(sdrwm, psa(sdrwm, sch));
		sch = NULL;
		return NULL;
	}

	/*	Creating the SdrControlHeader.				*/

	sdrwm = _sdrwm(NULL);
	if (psm_locate(sdrwm, _SdrSchName(), &controlHeaderAddress, &elt) < 0)
	{
		putErrmsg("Can't search for SDR control header.", NULL);
		return NULL;
	}

	if (elt)			/*	Header already exists.	*/
	{
		sch = (SdrControlHeader *) psp(sdrwm, controlHeaderAddress);
		CHKNULL(sch);
		sm_list_unwedge(sdrwm, sch->sdrs, 3);
		return sch;
	}

	/*	Header needs to be created.				*/

	controlHeaderAddress = psm_zalloc(sdrwm, sizeof(SdrControlHeader));
	if (controlHeaderAddress == 0)
	{
		putErrmsg(_noMemoryMsg(), NULL);
		return NULL;		/*	Not enough memory.	*/
	}

	/*	Initialize the control header.				*/

	sch = (SdrControlHeader *) psp(sdrwm, controlHeaderAddress);
	CHKNULL(sch);
	sch->sdrs = sm_list_create(sdrwm);
	if (sch->sdrs == 0)
	{
		putErrmsg(_noMemoryMsg(), NULL);
		return NULL;		/*	Not enough memory.	*/
	}

	/*	Store location of control header in catalog
		of the shared SDR working memory, for access
		by subsequently started processes.		*/

	if (psm_catlg(sdrwm, _SdrSchName(), controlHeaderAddress) < 0)
	{
		putErrmsg("Can't catalog SDR control header.", NULL);
		return NULL;		/*	Can't catalog.		*/
	}

	return sch;
}

void	*allocFromSdrMemory(char *fileName, int lineNbr, size_t length)
{
	PsmPartition	sdrwm = _sdrwm(NULL);
	PsmAddress	address;
	void		*block;

	address = Psm_zalloc(fileName, lineNbr, sdrwm, length);
	if (address == 0)
	{
		_putErrmsg(fileName, lineNbr, _noMemoryMsg(), NULL);
		return NULL;
	}

	block = psp(sdrwm, address);
	memset(block, 0, length);
	return block;
}

void	releaseToSdrMemory(char *fileName, int lineNbr, void *block)
{
	PsmPartition	sdrwm = _sdrwm(NULL);

	Psm_free(fileName, lineNbr, sdrwm, psa(sdrwm, (char *) block));
}

void	*sdrMemAtoP(unsigned long address)
{
	return (void *) psp(_sdrwm(NULL), address);
}

unsigned long sdrMemPtoA(void *pointer)
{
	return (unsigned long) psa(_sdrwm(NULL), pointer);
}

static PsmPartition	_sdrwm(sm_WmParms *parms)
{
	static int		sdrwmId = 0;
	static PsmView		sdrWorkingMemory;
	static PsmPartition	sdrwm = NULL;
	static int		memmgrIdx; /*	For local lists only.	*/
	static int		sdrwmIsPrivate = 0;
	static MemAllocator	sdrmtake = allocFromSdrMemory;
	static MemDeallocator	sdrmrlse = releaseToSdrMemory;
	static MemAtoPConverter	sdrmatop = sdrMemAtoP;
	static MemPtoAConverter	sdrmptoa = sdrMemPtoA;
	static SdrControlHeader	*sch = NULL;

	if (parms)
	{
		if (parms->wmSize == -1)	/*	Shutdown.	*/
		{
			if (sdrwm)
			{
				oK(_sch(&sch));	/*	Destroy.	*/
				sch = NULL;
				if (sdrwmIsPrivate)
				{
					memmgr_destroy(sdrwmId, &sdrwm);
				}
			}

			sdrwmId = 0;
			sdrwm = NULL;
			memmgrIdx = -1;
			oK(_sdrMemory(&memmgrIdx));
			return NULL;
		}

		/*	Opening SDR working memory.			*/

		if (sdrwm)			/*	Redundant.	*/
		{
			return sdrwm;
		}

		/*	Use built-in defaults as needed.		*/

		if (parms->wmSize == 0)
		{
			parms->wmSize = 1000000;
		}

		if (parms->wmKey == SM_NO_KEY)
		{
			sdrwmIsPrivate = 1;
			parms->wmKey = SDR_SM_KEY;
		}

		if (parms->wmName == NULL)
		{
			parms->wmName = SDR_SM_NAME;
		}

		sdrwm = &sdrWorkingMemory;
		if (memmgr_open(parms->wmKey, parms->wmSize, &parms->wmAddress,
				&sdrwmId, parms->wmName, &sdrwm, &memmgrIdx,
				sdrmtake, sdrmrlse, sdrmatop, sdrmptoa) < 0)
		{
			putErrmsg("Can't open SDR working memory.", NULL);
			if (sdrwmIsPrivate)
			{
				memmgr_destroy(sdrwmId, &sdrwm);
			}

			sdrwmId = 0;
			sdrwm = NULL;
			memmgrIdx = -1;
			return NULL;
		}

		oK(_sdrMemory(&memmgrIdx));
		oK(_sdrwmSize(&parms->wmSize));

		/*	Initialize SDR control header.			*/

		sch = NULL;
		sch = _sch(&sch);
		if (sch == NULL)
		{
			putErrmsg("Can't create SDR control header.", NULL);
			if (sdrwmIsPrivate)
			{
				memmgr_destroy(sdrwmId, &sdrwm);
			}

			sdrwmId = 0;
			sdrwm = NULL;
			memmgrIdx = -1;
			oK(_sdrMemory(&memmgrIdx));
			return NULL;
		}
	}

	return sdrwm;
}

SdrMap	*_mapImage(Sdr sdrv)
{
	static SdrMap	map;

	if (sdrv->sdr->configFlags & SDR_IN_DRAM)
	{
		return (SdrMap *) (sdrv->dbsm);
	}

	sdrFetch(map, 0);
	return &map;
}

/*	*	Mutual exclusion functions	*	*	*	*/

static int	lockSdr(SdrState *sdr)
{
	if (sm_SemTake(sdr->sdrSemaphore) < 0)
	{
		return -1;
	}

	sdr->sdrOwnerThread = pthread_self();
	sdr->sdrOwnerTask = sm_TaskIdSelf();
	sdr->xnDepth = 1;
	return 0;
}

int	takeSdr(SdrState *sdr)
{
	CHKERR(sdr);
	if (sdr->sdrSemaphore == -1 || sm_SemEnded(sdr->sdrSemaphore))
	{
		return -1;		/*	Can't be taken.		*/
	}

	if (sdr->sdrOwnerTask == sm_TaskIdSelf()
	&& pthread_equal(sdr->sdrOwnerThread, pthread_self()))
	{
		sdr->xnDepth++;
		return 0;		/*	Already taken.		*/
	}

	return lockSdr(sdr);
}

static void	unlockSdr(SdrState *sdr)
{
	sdr->sdrOwnerTask = -1;
	if (sdr->sdrSemaphore != -1)
	{
		sm_SemGive(sdr->sdrSemaphore);
	}
}

void	releaseSdr(SdrState *sdr)
{
	CHKVOID(sdr);
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

int	Sdr_initialize(long wmSize, char *wmPtr, int wmKey, char *wmName)
{
	sm_SemId	lock;
	sm_WmParms	wmparms;
	PsmPartition	sdrwm;

	/*	Initialize the shared-memory libraries as necessary.	*/

	if (sm_ipc_init())
	{
		putErrmsg("Can't initialize IPC system.", NULL);
		return -1;	/*	Crash if can't initialize sm.	*/
	}

	lock = _sdrlock(0);
	if (lock == SM_SEM_NONE || sm_SemTake(lock) < 0)
	{
		putErrmsg("Can't initialize SDR system.", NULL);
		return -1;	/*	Crash if can't lock sdr.	*/
	}

	/*	Attach to SDR's own shared working memory partition.	*/

	if (_sdrwm(NULL) != NULL)	/*	Already initialized.	*/
        {
		sm_SemGive(lock);
		return 0;
	}
   
	wmparms.wmKey = wmKey;
	wmparms.wmSize = wmSize;
	wmparms.wmAddress = wmPtr;
	wmparms.wmName = wmName;
	sdrwm = _sdrwm(&wmparms);
	sm_SemGive(lock);
	if (sdrwm == NULL)
	{
		putErrmsg("Can't open SDR working memory.", NULL);
		return -1;
	}

	return 0;
}

void	sdr_wm_usage(PsmUsageSummary *usage)
{
	CHKVOID(usage);
	psm_usage(_sdrwm(NULL), usage);
}

void	sdr_shutdown()		/*	Ends SDR service on machine.	*/
{
	sm_SemId	lock;
	sm_WmParms	wmparms;

	lock = _sdrlock(0);
	if (lock == SM_SEM_NONE || sm_SemTake(lock) < 0)
	{
		putErrmsg("SDR system not initialized, can't shut down.", NULL);
		return;	/*	Crash if can't lock sdr.	*/
	}

	if (_sdrwm(NULL) != NULL)
	{
		wmparms.wmSize = -1;
		oK(_sdrwm(&wmparms));
	}

	_sdrlock(1);		/*	Delete SDR system semaphore.	*/
}

int	_xniEnd(const char *fileName, int lineNbr, const char *arg, Sdr sdrv)
{
	_postErrmsg(fileName, lineNbr,
			"Assertion failed, SDR transaction canceled.", arg);
	writeErrmsgMemos();
	crashXn(sdrv);
	if (_coreFileNeeded(NULL))
	{
		sm_Abort();
	}

	return 1;
}

/*	*	Transaction utility functions	*	*	*	*/

/*	Logging is the mechanism that enables SDR transactions to be
	reversible.  The log for an SDR is a file in which are recorded
	all of the SDR data space updates in the scope of the current
	transaction; the last byte of the log file is the last byte
	of the last log entry.

	The log file is truncated to length zero at the termination of
	each transaction.  Therefore, if it is of non-zero length at
	startup then all complete log entries in the log file must be
	backed out of the SDR heap in shared memory (if applicable) and
	the SDR heap in the databse file (if applicable) before the
	database is accessed for any purpose: since this transaction
	was not ended, the database may be in an inconsistent state.

	An entry in the write-ahead log is an array of unsigned bytes.
	Its format is as follows, where W is WORD_SIZE (from platform.h;
	4 on a 32-bit machine, 8 on a 64-bit machine) and L is the number
	of bytes of data written.

	From offet	Until offset	Content is:

	0		W		start address within SDR heap
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
			char *dbsm, SdrState *sdr)
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
		isprintf(logfilename, sizeof logfilename, "%s%c%s.sdrlog",
				sdrv->sdr->pathName, ION_PATH_DELIMITER,
				sdrv->sdr->name);
		sdrv->logfile = iopen(logfilename, O_RDWR | O_CREAT | O_TRUNC,
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

	if (sdr->xnCanceled == 0)
	{
		clearTransaction(sdrv);
		unlockSdr(sdr);
		return;
	}

	/*	Transaction was canceled.  If cancellation has already
	 *	triggered restart, nothing more to do; just get out.	*/

	if (!(sdr_in_xn(sdrv)))
	{
		return;
	}

	/*	Initiate cancellation procedure.			*/

	sdr->xnCanceled = 0;
	if (!(sdr->configFlags & SDR_REVERSIBLE))
	{
		/*	Can't back out; if data modified, bail.	*/

		if (sdrv->modified)
		{
			handleUnrecoverableError(sdrv);
		}

		clearTransaction(sdrv);
		unlockSdr(sdr);
		return;
	}

	/*	Transaction must be reversed as necessary.		*/

	if (reverseTransaction(sdrv->logEntries, sdrv->logfile, sdrv->dbfile,
			sdrv->dbsm, sdr) < 0)
	{
		handleUnrecoverableError(sdrv);

		/*	In case not aborted....				*/

		clearTransaction(sdrv);
		unlockSdr(sdr);
		return;
	}

	/*	Reversal succeeded, so try to reboot volatiles.		*/

	if (sdr->restartCmd[0] == '\0')
	{
		/*	No restart utility, so can't do any more.	*/

		clearTransaction(sdrv);
		unlockSdr(sdr);
		return;
	}

	/*	Restart utility provided.				*/

	sdr->halted = 1;	/*	Enable restart to ionAttach.	*/
	if (pseudoshell(sdr->restartCmd) < 0)
	{
		writeMemoNote("[!] Can't execute restart command",
				sdr->restartCmd);
		sdr->halted = 0;
		clearTransaction(sdrv);
		unlockSdr(sdr);
		return;
	}

	/*	Restart utility is running; give it time to hijack the
	 *	transaction.						*/

	snooze(2);

	/*	Transaction still exists, but restart utility is now
	 *	its owner.  From the perspective of the current task,
	 *	the transaction is finished; nothing more to do.  The
	 *	restart utility will clear the hijacked transaction.	*/

	sdr->halted = 0;
	return;
}

void	crashXn(Sdr sdrv)
{
	SdrState	*sdr;

	CHKVOID(sdrv);
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
	long	bufsize = _sdrwmSize(NULL);

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
	SdrMap	map;

	bufsize = getBigBuffer(&buffer);
	if (bufsize < 0)
	{
		putErrmsg("Can't get buffer in sdrwm.", NULL);
		return -1;
	}

	memset(buffer, 0 , sizeof buffer);
	dbfile = iopen(dbfilename, O_RDWR | O_CREAT, 0777);
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
	initSdrMap(&map, sdr);
	if (lseek(dbfile, 0, SEEK_SET) < 0
	|| write(dbfile, (char *) &map, sizeof map) < sizeof map
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
		int memKey, char *pathName, char *restartCmd)
{
	sm_SemId		lock = _sdrlock(0);
	PsmPartition		sdrwm = _sdrwm(NULL);
	SdrControlHeader	*sch = _sch(NULL);
	PsmAddress		elt;
	SdrState		*sdr;
	PsmAddress		newSdrAddress;
	long			limit;
	struct stat		statbuf;
	char			logfilename[PATHLENMAX + 1 + 32 + 1 + 6 + 1];
	int			logfile = -1;
	Lyst			logEntries = NULL;
	char			dbfilename[PATHLENMAX + 1 + 32 + 1 + 3 + 1];
	int			dbfile = -1;
	char			*dbsm;
	int			dbsmId;

	CHKERR(sdrwm);
	CHKERR(sch);
	CHKERR(name);
	CHKERR(pathName);
	if (!(configFlags & SDR_IN_DRAM || configFlags & SDR_IN_FILE))
	{
		putErrmsg("No SDR heap site specified in configFlags.",
				itoa(configFlags));
		return -1;
	}

	if (lock == SM_SEM_NONE || sm_SemTake(lock) < 0)
	{
		putErrmsg("Can't lock SDR control header.", NULL);
		return -1;
	}

	for (elt = sm_list_first(sdrwm, sch->sdrs); elt;
			elt = sm_list_next(sdrwm, elt))
	{
		sdr = (SdrState *) psp(sdrwm, sm_list_data(sdrwm, elt));
		CHKERR(sdr);
		if (strcmp(sdr->name, name) == 0)
		{
			sm_SemGive(lock);
			if (sdr->configFlags == configFlags
			&& sdr->initHeapWords == heapWords
			&& (sdr->sdrKey == memKey || memKey == SM_NO_KEY)
			&& strcmp(sdr->pathName, pathName) == 0)
			{
				sm_SemUnwedge(sdr->sdrSemaphore, 3);
				return 0;	/*	Profile loaded.	*/
			}

			putErrmsg("Wrong profile for this SDR.", name);
			return -1;
		}
	}

	/*	This is an SDR profile that's not currently loaded.	*/

	newSdrAddress = psm_zalloc(sdrwm, sizeof(SdrState));
	if (newSdrAddress == 0)
	{
		sm_SemGive(lock);
		putErrmsg("Can't allocate memory for SDR state.", NULL);
		return -1;
	}

	sdr = (SdrState *) psp(sdrwm, newSdrAddress);
	memset(sdr, 0, sizeof(SdrState));
	limit = sizeof(sdr->name) - 1;
	istrcpy(sdr->name, name, limit);
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
		sm_SemGive(lock);
		putErrmsg("Can't create transaction semaphore for SDR.", NULL);
		return -1;
	}

	sdr->sdrOwnerTask = -1;
	sdr->traceKey = sm_GetUniqueKey();
	sdr->traceSize = 0;
	limit = sizeof(sdr->pathName) - 1;
	istrcpy(sdr->pathName, pathName, limit);
	if (stat(sdr->pathName, &statbuf) < 0
	|| (statbuf.st_mode & S_IFDIR) == 0)
	{
		writeMemoNote("[?] No such directory; disabling heap residence \
in file and transaction reversibility", sdr->pathName);
		sdr->configFlags &= (~SDR_IN_FILE); 
		sdr->configFlags &= (~SDR_REVERSIBLE); 
	}

	if (restartCmd == NULL)
	{
		sdr->restartCmd[0] = '\0';
	}
	else
	{
		limit = sizeof(sdr->restartCmd) - 1;
		istrcpy(sdr->restartCmd, restartCmd, limit);
	}

	/*	Add SDR to linked list of defined SDRs.			*/

	sdr->sdrsElt = sm_list_insert_last(sdrwm, sch->sdrs, newSdrAddress);
	if (sdr->sdrsElt == 0)
	{
		psm_free(sdrwm, newSdrAddress);
		sm_SemGive(lock);
		putErrmsg("Can't insert SDR state into list of SDRs.", NULL);
		return -1;
	}

	/*	If database exists, back out any current transaction.
		If not, create the database and initialize it.		*/

	if (sdr->configFlags & SDR_REVERSIBLE)
	{
		isprintf(logfilename, sizeof logfilename, "%s%c%s.sdrlog",
				sdr->pathName, ION_PATH_DELIMITER, name);
		logfile = iopen(logfilename, O_RDWR | O_CREAT | O_APPEND, 0777);
		if (logfile == -1)
		{
			psm_free(sdrwm, newSdrAddress);
			sm_SemGive(lock);
			putSysErrmsg("Can't open log file", logfilename);
			return -1;
		}

		logEntries = lyst_create_using(_sdrMemory(NULL));
		if (logEntries == 0)
		{
			close(logfile);
			psm_free(sdrwm, newSdrAddress);
			sm_SemGive(lock);
			putErrmsg(_noMemoryMsg(), NULL);
			return -1;
		}

		if (reloadLogEntries(logEntries, logfile) < 0)
		{
			lyst_destroy(logEntries);
			close(logfile);
			psm_free(sdrwm, newSdrAddress);
			sm_SemGive(lock);
			putErrmsg("Can't reload log entries.", NULL);
			return -1;
		}
	}

	if (sdr->configFlags & SDR_IN_FILE)
	{
		isprintf(dbfilename, sizeof dbfilename, "%s%c%s.sdr",
				sdr->pathName, ION_PATH_DELIMITER, name);
		dbfile = iopen(dbfilename, O_RDWR, 0777);
		if (dbfile == -1)
		{
			dbfile = createDbFile(sdr, dbfilename);
			if (dbfile == -1)
			{
				if (logfile != -1) close(logfile);
				if (logEntries) lyst_destroy(logEntries);
				psm_free(sdrwm, newSdrAddress);
				sm_SemGive(lock);
				putErrmsg("Can't have file-based database",
						NULL);
				return -1;
			}
		}
		else	/*	Database file exists.			*/
		{
			if (reverseTransaction(logEntries, logfile, dbfile,
					NULL, sdr) < 0)
			{
				close(dbfile);
				if (logfile != -1) close(logfile);
				if (logEntries) lyst_destroy(logEntries);
				psm_free(sdrwm, newSdrAddress);
				sm_SemGive(lock);
				putErrmsg("Can't reverse log entries.", NULL);
				return -1;
			}
		}
	}

	if (sdr->configFlags & SDR_IN_DRAM)
	{
		dbsm = NULL;
		switch (sm_ShmAttach(sdr->sdrKey, sdr->sdrSize, &dbsm, &dbsmId))
		{
		case -1:	/*	Error.				*/
			if (dbfile != -1) close(dbfile);
			if (logfile != -1) close(logfile);
			if (logEntries) lyst_destroy(logEntries);
			psm_free(sdrwm, newSdrAddress);
			sm_SemGive(lock);
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
					psm_free(sdrwm, newSdrAddress);
					sm_SemGive(lock);
					putErrmsg("Can't load db from file.",
							NULL);
					return -1;
				}

				break;
			}
	
			/*	Back transaction out of memory if nec.	*/
	
			if (reverseTransaction(logEntries, logfile, -1,
					dbsm, sdr) < 0)
			{
				if (logfile != -1) close(logfile);
				if (logEntries) lyst_destroy(logEntries);
				psm_free(sdrwm, newSdrAddress);
				sm_SemGive(lock);
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
					psm_free(sdrwm, newSdrAddress);
					sm_SemGive(lock);
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

	sm_SemGive(lock);
	return 0;
}

int	sdr_reload_profile(char *name, int configFlags, long heapWords,
		int memKey, char *pathName, char *restartCmd)
{
	sm_SemId		lock = _sdrlock(0);
	PsmPartition		sdrwm = _sdrwm(NULL);
	SdrControlHeader	*sch = _sch(NULL);
	PsmAddress		elt;
	PsmAddress		sdrAddress;
	SdrState		*sdr;

	CHKERR(sdrwm);
	CHKERR(sch);
	CHKERR(name);
	CHKERR(pathName);
	if (lock == SM_SEM_NONE || sm_SemTake(lock) < 0)
	{
		putErrmsg("Can't lock SDR control header.", NULL);
		return -1;
	}

	for (elt = sm_list_first(sdrwm, sch->sdrs); elt;
			elt = sm_list_next(sdrwm, elt))
	{
		sdrAddress = sm_list_data(sdrwm, elt);
		sdr = (SdrState *) psp(sdrwm, sdrAddress);
		CHKERR(sdr);
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
			sm_SemGive(lock);
			putErrmsg("Can't unload SDR: profile conflict.", name);
			return -1;
		}

		/*	Profile for this SDR is currently loaded, so
		 *	we have to unload that profile in order to
		 *	force reversal of any incomplete transaction
		 *	that is currently in progress.			*/

		sm_SemDelete(sdr->sdrSemaphore);
		psm_free(sdrwm, sdrAddress);
		oK(sm_list_delete(sdrwm, elt, NULL, NULL));
	}

	/*	Profile for this SDR is now known to be unloaded.	*/

	sm_SemGive(lock);
	return sdr_load_profile(name, configFlags, heapWords, memKey, pathName,
			restartCmd);
}

static void	deleteObjectExtent(LystElt elt, void *userData)
{
	MRELEASE(lyst_data(elt));
}

Sdr	Sdr_start_using(char *name)
{
	sm_SemId		lock = _sdrlock(0);
	PsmPartition		sdrwm = _sdrwm(NULL);
	SdrControlHeader	*sch = _sch(NULL);
	PsmAddress		elt;
	SdrState		*sdr;
	PsmAddress		sdrViewAddress;
	SdrView			*sdrv;
	char			dbfilename[PATHLENMAX + 1 + 32 + 1 + 3 + 1];
	char			logfilename[PATHLENMAX + 1 + 32 + 1 + 6 + 1];

	CHKNULL(sdrwm);
	CHKNULL(sch);
	CHKNULL(name);
	if (lock == SM_SEM_NONE || sm_SemTake(lock) < 0)
	{
		putErrmsg("Can't lock SDR control header.", NULL);
		return NULL;
	}

	for (elt = sm_list_first(sdrwm, sch->sdrs); elt;
			elt = sm_list_next(sdrwm, elt))
	{
		sdr = (SdrState *) psp(sdrwm, sm_list_data(sdrwm, elt));
		CHKNULL(sdr);
		if (strcmp(sdr->name, name) == 0)
		{
			break;
		}
	}

	if (elt == 0)	/*	Reached the end of the list, no match.	*/
	{
		sm_SemGive(lock);
		putErrmsg("SDR profile not found.", name);
		return NULL;
	}

	/*	SDR is defined;	create view for access.			*/

	sdrViewAddress = psm_zalloc(sdrwm, sizeof(SdrView));
	if (sdrViewAddress == 0)
	{
		sm_SemGive(lock);
		putErrmsg(_noMemoryMsg(), NULL);
		return NULL;
	}

	sdrv = (SdrView *) psp(sdrwm, sdrViewAddress);
	sdrv->sdr = sdr;
	if (sdr->configFlags & SDR_IN_FILE)
	{
		isprintf(dbfilename, sizeof dbfilename, "%s%c%s.sdr",
				sdr->pathName, ION_PATH_DELIMITER, name);
		sdrv->dbfile = iopen(dbfilename, O_RDWR, 0777);
		if (sdrv->dbfile == -1)
		{
			sm_SemGive(lock);
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
			sm_SemGive(lock);
			putErrmsg("Can't attach to database in memory.",
					itoa(sdr->sdrKey));
			return NULL;
		}
	}

	if (sdr->configFlags & SDR_REVERSIBLE)
	{
		isprintf(logfilename, sizeof logfilename, "%s%c%s.sdrlog",
				sdr->pathName, ION_PATH_DELIMITER, name);
		sdrv->logfile = iopen(logfilename, O_RDWR | O_CREAT | O_APPEND,
				0777);
		if (sdrv->logfile == -1)
		{
			sm_SemGive(lock);
			putSysErrmsg("Can't open log file", logfilename);
			return NULL;
		}

		sdrv->logEntries = lyst_create_using(_sdrMemory(NULL));
		if (sdrv->logEntries == 0)
		{
			sm_SemGive(lock);
			putErrmsg(_noMemoryMsg(), NULL);
			return NULL;
		}
	}
	else
	{
		sdrv->logfile = -1;
	}

	if (sdr->configFlags & SDR_BOUNDED)
	{
		sdrv->knownObjects = lyst_create_using(_sdrMemory(NULL));
		if (sdrv->knownObjects == 0)
		{
			sm_SemGive(lock);
			putErrmsg(_noMemoryMsg(), NULL);
			return NULL;
		}

		lyst_delete_set(sdrv->knownObjects, deleteObjectExtent, NULL);
	}

	sdrv->trace = NULL;
	sdrv->currentSourceFileName = NULL;
	sdrv->currentSourceFileLine = 0;
	sm_SemGive(lock);
	return sdrv;
}

char	*sdr_name(Sdr sdrv)
{
	CHKNULL(sdrv);
	return sdrv->sdr->name;
}

long	sdr_heap_size(Sdr sdrv)
{
	CHKZERO(sdrv);
	return sdrv->sdr->heapSize;
}

void	sdr_stop_using(Sdr sdrv)
{
	PsmPartition	sdrwm = _sdrwm(NULL);

	CHKVOID(sdrv);

	/*	Should only stop using SDR when not currently using it,
	 *	i.e., when not currently in the midst of a transaction.	*/

	if (sdr_in_xn(sdrv))
	{
		putErrmsg("Terminating transaction to stop using SDR.", NULL);
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

	/*	Erase content of SdrView, in case space is re-used
	 *	for another SdrView; then delete it.			*/

	memset((char *) sdrv, 0, sizeof(SdrView));
	psm_free(sdrwm, psa(sdrwm, sdrv));
}

void	sdr_abort(Sdr sdrv)
{
	CHKVOID(sdrv);
	sm_SemEnd(sdrv->sdr->sdrSemaphore);
	microsnooze(500000);
	sm_SemDelete(sdrv->sdr->sdrSemaphore);
	sdrv->sdr->sdrSemaphore = -1;
	sdr_shutdown();
}

void	sdr_destroy(Sdr sdrv)
{
	sm_SemId		lock = _sdrlock(0);
	PsmPartition		sdrwm = _sdrwm(NULL);
	SdrState		*sdr;
	char			dbfilename[PATHLENMAX + 1 + 32 + 1 + 3 + 1];
	char			logfilename[PATHLENMAX + 1 + 32 + 1 + 6 + 1];
	char			*dbsm = NULL;
	int			dbsmId = 0;

	CHKVOID(sdrv);

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

	/*	Destroy global persistent SDR state.			*/

	if (lock == SM_SEM_NONE || sm_SemTake(lock) < 0)
	{
		putErrmsg("Can't lock SDR control header.", NULL);
		return;
	}

	/*	Destroy file copy of heap if any.			*/

	if (sdr->configFlags & SDR_IN_FILE)
	{
		isprintf(dbfilename, sizeof dbfilename, "%s%c%s.sdr",
				sdr->pathName, ION_PATH_DELIMITER, sdr->name);
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
		isprintf(logfilename, sizeof logfilename, "%s%c%s.sdrlog",
				sdr->pathName, ION_PATH_DELIMITER, sdr->name);
		unlink(logfilename);
	}

	/*	Unload profile and destroy it.				*/

	oK(sm_list_delete(sdrwm, sdr->sdrsElt, NULL, NULL));
	psm_free(sdrwm, psa(sdrwm, sdr));
	sm_SemGive(lock);
}

/*	*	Low-level transaction functions		*	*	*/

int	sdr_begin_xn(Sdr sdrv)
{
	CHKZERO(sdrv);
	if (takeSdr(sdrv->sdr) < 0)
	{
		return 0;	/*	Failed to begin transaction.	*/
	}

	sdrv->modified = 0;
	return 1;		/*	Began transaction.		*/
}

int	sdr_in_xn(Sdr sdrv)
{
	CHKZERO(sdrv);
	return (sdrv->sdr != NULL
		&& sdrv->sdr->sdrOwnerTask == sm_TaskIdSelf()
		&& pthread_equal(sdrv->sdr->sdrOwnerThread, pthread_self()));
}

int	sdr_heap_is_halted(Sdr sdrv)
{
	CHKZERO(sdrv);
	return (sdrv->sdr != NULL && sdrv->sdr->halted);
}

int	sdrFetchSafe(Sdr sdrv)
{
	return (sdr_in_xn(sdrv) || sdr_heap_is_halted(sdrv));
}

void	sdr_exit_xn(Sdr sdrv)
{
	SdrState	*sdr;

	CHKVOID(sdrv);
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
	SdrState	*sdr;

	CHKVOID(sdrv);
	sdr = sdrv->sdr;
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
	SdrState	*sdr;

	CHKERR(sdrv);
	sdr = sdrv->sdr;
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

	CHKVOID(sdrv);
	sdr = sdrv->sdr;
	if (sdr != NULL)
	{
		sdr->xnCanceled = 1;
		sdr->xnDepth = 0;
		terminateXn(sdrv);
	}
}

void	*sdr_pointer(Sdr sdrv, Address address)
{
	CHKNULL(sdrv);
	if ((sdrv->sdr->configFlags & SDR_IN_DRAM) == 0 || address <= 0)
	{
		return NULL;
	}

	return (void *) (sdrv->dbsm + address);
}

Address	sdr_address(Sdr sdrv, void *pointer)
{
	char	*ptr;

	CHKZERO(sdrv);
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

	CHKVOID(length > 0);
	CHKVOID(sdrv);
	CHKVOID(from);
	CHKVOID(into >= 0);
	sdr = sdrv->sdr;
	to = into + length;
	if (to > sdr->sdrSize)
	{
		_putErrmsg(file, line, _violationMsg(), "write");
		crashXn(sdrv);
		return;
	}

	if (sdr->configFlags & SDR_BOUNDED && src == UserPut)
	{
		if (sdrBoundaryViolated(sdrv, into, length))
		{
			_putErrmsg(file, line, _violationMsg(), "write");
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
entry.", NULL);
			crashXn(sdrv);
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
	if (!(sdr_in_xn(sdrv)))
	{
		oK(_iEnd(file, line, _notInXnMsg()));
		return;
	}

	joinTrace(sdrv, file, line);
	_sdrput(file, line, sdrv, into, from, length, UserPut);
}

void	_sdrfetch(Sdr sdrv, char *into, Address from, long length)
{
	SdrState	*sdr;
	Address		to;

	if (length == 0)			/*	Nothing to do.	*/
	{
		return;
	}

	CHKVOID(length > 0);
	CHKVOID(sdrv);
	CHKVOID(into);
	CHKVOID(from >= 0);
	memset(into, 0, length);		/*	Default value.	*/
	sdr = sdrv->sdr;
	CHKVOID(sdr);
	to = from + length;
	if (to > sdr->sdrSize)
	{
		putErrmsg(_violationMsg(), "read");
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
	_sdrfetch(sdrv, into, from, length);
}
