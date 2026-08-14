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

#ifdef ION_HAVE_ROBUST_MUTEX
static int		createSdrMutex(SdrState *sdr);
static void		destroySdrMutex(SdrState *sdr);
static int		recoverOrphanedXn(Sdr sdrv, char *deadInfo);
static void		recordLockOwner(SdrState *sdr);
static void		formatLockOwner(const SdrLockOwner *owner,
				uint64_t nowUs, char *out, size_t outSize);
static uint64_t		monotonicMicroseconds(void);
#endif

#ifndef SDR_TRACE
char	*_noTraceMsg(void)
{
	return "Tracing disabled; recompile with -DSDR_TRACE.";
}
#endif

char	*_notInXnMsg(void)
{
	return "sdr_in_xn(sdrv)";
}

char	*_apiErrMsg(void)
{
	return "API values";
}

char	*_noMemoryMsg(void)
{
	return "Not enough available SDR working memory.";
}

char	*_violationMsg(void)
{
	return "SDR boundaries or integrity violation.";
}

static char	*_SdrSchName(void)
{
	return "sdrsch";
}

static char	*_SdrWmName(void)
{
	return "sdrwm";
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
			sm_SemEnd(lock);
			microsnooze(50000);
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

static size_t	_sdrwmSize(size_t *wmsize)
{
	static size_t	size = 0;

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
					microsnooze(50000);
					sm_SemDelete(sdr->sdrSemaphore);
					sdr->sdrSemaphore = SM_SEM_NONE;
				}
#ifdef ION_HAVE_ROBUST_MUTEX
				destroySdrMutex(sdr);
#endif
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

void	*allocFromSdrMemory(const char *fileName, int lineNbr, size_t length)
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

void	releaseToSdrMemory(const char *fileName, int lineNbr, void *block)
{
	PsmPartition	sdrwm = _sdrwm(NULL);

	Psm_free(fileName, lineNbr, sdrwm, psa(sdrwm, (char *) block));
}

void	*sdrMemAtoP(uaddr address)
{
	return (void *) psp(_sdrwm(NULL), address);
}

uaddr sdrMemPtoA(void *pointer)
{
	return (uaddr) psa(_sdrwm(NULL), pointer);
}

static PsmPartition	_sdrwm(sm_WmParms *parms)
{
	static uaddr		sdrwmId = 0;
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
		if (parms->wmKey == -11111 )
		{
			sdrwm = NULL;   /* reset database state */
			return sdrwm;
		}

		if (sdrwm == NULL)  /* re-initialize all static */
		{
			sdrwmId = 0;
			memmgrIdx = -1; /*	For local lists only.	*/
			sdrwmIsPrivate = 0;
			sdrmtake = allocFromSdrMemory;
			sdrmrlse = releaseToSdrMemory;
			sdrmatop = sdrMemAtoP;
			sdrmptoa = sdrMemPtoA;
			sch = (SdrControlHeader *) NULL;
		}

		if (parms->wmName == NULL)	/*	Shutdown.	*/
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

		if (parms->wmKey == SM_NO_KEY)
		{
			parms->wmKey = SDR_SM_KEY;
		}

		/*	The SDR working memory is owned by ION whenever
		 *	we open it through this path, regardless of
		 *	whether the key was the legacy default or a
		 *	per-node value supplied via ionconfig.  Marking
		 *	the partition private ensures sdr_shutdown()
		 *	destroys the underlying shared-memory segment
		 *	so that one node's ionexit cleans up its own
		 *	SDR working memory without leaking it.		*/

		sdrwmIsPrivate = 1;

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

/*	In SDR_IN_DRAM mode this returns a live pointer into the
 *	dataspace, so every patchMap() is visible on the next
 *	map->X read.  In file-backed mode it returns a pointer to
 *	a process-local static snapshot that is NOT refreshed when
 *	patchMap() writes to the dataspace.  Callers must therefore
 *	never re-read map->X after patching X in the same function:
 *	either compute the new value into a local and pass it to
 *	patchMap, or use sdrFetch(V, ADDRESS_OF(X)) for a live read.	*/

SdrMap	*_mapImage(Sdr sdrv)
{
	static SdrMap	map;

	if (sdrv->sdr->configFlags & SDR_IN_DRAM)
	{
		return (SdrMap *)(void *) (sdrv->dssm);
	}

	sdrFetch(map, 0);
	return &map;
}

/*	*	Mutual exclusion functions	*	*	*	*/

/*	monotonicMicroseconds: CLOCK_MONOTONIC in microseconds.  Used
 *	by SdrLockOwner and SdrDropStats diagnostics; available on
 *	all platforms regardless of ION_HAVE_ROBUST_MUTEX.		*/

static uint64_t	monotonicMicrosecondsCommon(void)
{
	struct timespec	ts;

	if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
	{
		return 0;
	}

	return ((uint64_t) ts.tv_sec) * 1000000ULL
			+ ((uint64_t) ts.tv_nsec) / 1000ULL;
}

#ifdef ION_HAVE_ROBUST_MUTEX

/*	Capture /proc/self/cmdline once per process and cache it in a
 *	process-local static.  The capture is best-effort: if /proc
 *	isn't available or the read fails, we fall back to "(unknown)"
 *	and the diagnostic message still carries the pid.		*/

static char		cmdlineBuf[SDR_LOCK_OWNER_CMDLINE_MAX];
static pthread_once_t	cmdlineOnce = PTHREAD_ONCE_INIT;

static void	captureCmdline(void)
{
	int	fd;
	ssize_t	n;
	ssize_t	i;

	fd = open("/proc/self/cmdline", O_RDONLY);
	if (fd < 0)
	{
		istrcpy(cmdlineBuf, "(unknown)", sizeof cmdlineBuf);
		return;
	}

	n = read(fd, cmdlineBuf, sizeof cmdlineBuf - 1);
	oK(close(fd));
	if (n <= 0)
	{
		istrcpy(cmdlineBuf, "(unknown)", sizeof cmdlineBuf);
		return;
	}

	/*	/proc/self/cmdline separates argv entries with NUL bytes.
	 *	Rewrite to spaces so the cached string is a single readable
	 *	token, then NUL-terminate and trim any trailing space.	*/

	for (i = 0; i < n; i++)
	{
		if (cmdlineBuf[i] == '\0')
		{
			cmdlineBuf[i] = ' ';
		}
	}

	cmdlineBuf[n] = '\0';
	if (n > 0 && cmdlineBuf[n - 1] == ' ')
	{
		cmdlineBuf[n - 1] = '\0';
	}
}

static const char *capturedCmdline(void)
{
	oK(pthread_once(&cmdlineOnce, captureCmdline));
	return cmdlineBuf;
}

static uint64_t	monotonicMicroseconds(void)
{
	return monotonicMicrosecondsCommon();
}

static void	recordLockOwner(SdrState *sdr)
{
	const char	*cmd = capturedCmdline();

	sdr->lastOwner.pid = (int) getpid();
	sdr->lastOwner.acquired_at_us = monotonicMicroseconds();
	istrcpy(sdr->lastOwner.cmdline, cmd, sizeof sdr->lastOwner.cmdline);
}

static void	formatLockOwner(const SdrLockOwner *owner, uint64_t nowUs,
			char *out, size_t outSize)
{
	uint64_t	heldUs;

	heldUs = (owner->acquired_at_us && nowUs >= owner->acquired_at_us)
			? nowUs - owner->acquired_at_us : 0;
	isprintf(out, (int) outSize,
			"pid=%d cmdline=\"%s\" held=%llu us",
			owner->pid,
			owner->cmdline[0] ? owner->cmdline : "(unknown)",
			(unsigned long long) heldUs);
}

#endif	/*	ION_HAVE_ROBUST_MUTEX					*/

static int	lockSdr(Sdr sdrv)
{
	SdrState	*sdr = sdrv->sdr;
#ifdef SDR_PERF_INSTRUMENTATION
	uint64_t	acqStart = monotonicMicrosecondsCommon();
#endif
#ifdef ION_HAVE_ROBUST_MUTEX
	int		result;

	result = pthread_mutex_lock(&sdr->sdrMutex);
	if (result == EOWNERDEAD)
	{
		/*	The previous owner of the transaction lock
		 *	died while holding it.  We now own the lock;
		 *	roll back the dead owner's partial transaction
		 *	before marking the mutex consistent again.	*/

		SdrLockOwner	dead;
		char		deadInfo[160];

		/*	Snapshot the dead-owner record before recovery
		 *	starts.  We pass it into recoverOrphanedXn so
		 *	the diagnostic survives even if recovery fails
		 *	and the mutex transitions to ENOTRECOVERABLE.	*/

		memcpy(&dead, &sdr->lastOwner, sizeof dead);
		dead.cmdline[sizeof dead.cmdline - 1] = '\0';
		formatLockOwner(&dead, monotonicMicroseconds(),
				deadInfo, sizeof deadInfo);

		if (recoverOrphanedXn(sdrv, deadInfo) < 0)
		{
			/*	Unlocking without pthread_mutex_consistent
			 *	transitions the mutex to ENOTRECOVERABLE,
			 *	so every future locker fails the same way
			 *	rather than building on a corrupt SDR.	*/

			oK(pthread_mutex_unlock(&sdr->sdrMutex));
			return -1;
		}

		oK(pthread_mutex_consistent(&sdr->sdrMutex));
	}
	else if (result == ENOTRECOVERABLE)
	{
		putErrmsg("SDR transaction lock is unrecoverable; the SDR \
profile must be reloaded.", NULL);
		return -1;
	}
	else if (result != 0)
	{
		putErrmsg("Can't lock SDR transaction mutex.", itoa(result));
		return -1;
	}

	/*	sdrXnEnded carries the graceful-shutdown signal that
	 *	sm_SemEnded() provided on the semaphore path.		*/

	if (ion_ipc_atomic_get(&sdr->sdrXnEnded))
	{
		oK(pthread_mutex_unlock(&sdr->sdrMutex));
		return -1;
	}
#else
	if (sm_SemTake(sdr->sdrSemaphore) < 0)
	{
		return -1;
	}

	/*	sm_SemTake returns 0 when the semaphore was ended
	 *	during the wait (graceful shutdown path). In that
	 *	case, we don't actually hold the semaphore and must
	 *	not proceed with transaction setup.			*/

	if (sm_SemEnded(sdr->sdrSemaphore))
	{
		return -1;
	}
#endif

	/*	Lock now held (or semaphore taken): fold this acquisition's
	 *	latency into the shared perf counters.  Safe to write the
	 *	counters here without a separate guard because we hold the
	 *	transaction lock, so acquisitions are serialized.	*/

#ifdef SDR_PERF_INSTRUMENTATION
	{
		unsigned long	acqUs = (unsigned long)
				(monotonicMicrosecondsCommon() - acqStart);

		sdr->perfCounters.totalLockAcquireUs += acqUs;
		sdr->perfCounters.lockAcquireCount++;
		if (acqUs > sdr->perfCounters.maxLockAcquireUs)
		{
			sdr->perfCounters.maxLockAcquireUs = acqUs;
		}
	}
#endif

	sdr->sdrOwnerThread = pthread_self();
	sdr->sdrOwnerTask = sm_TaskIdSelf();
	sdr->xnDepth = 1;
	sdr->xnCanceled = 0;
	sdr->modified = 0;
#ifdef ION_HAVE_ROBUST_MUTEX
	recordLockOwner(sdr);
#endif
	return 0;
}

int	takeSdr(Sdr sdrv)
{
	SdrState	*sdr;

	CHKERR(sdrv);
	sdr = sdrv->sdr;
	CHKERR(sdr);
#ifdef ION_HAVE_ROBUST_MUTEX
	if (!(sdr->sdrMutexCreated) || ion_ipc_atomic_get(&sdr->sdrXnEnded))
	{
		return -1;		/*	Can't be taken.		*/
	}
#else
	if (sdr->sdrSemaphore == -1 || sm_SemEnded(sdr->sdrSemaphore))
	{
		return -1;		/*	Can't be taken.		*/
	}
#endif

	if (sdr->sdrOwnerTask == sm_TaskIdSelf()
	&& pthread_equal(sdr->sdrOwnerThread, pthread_self()))
	{
		sdr->xnDepth++;
		return 0;		/*	Already taken.		*/
	}

	return lockSdr(sdrv);
}

static void	unlockSdr(SdrState *sdr)
{
	sdr->sdrOwnerTask = -1;
	sdr->sdrOwnerThread = 0;
#ifdef ION_HAVE_ROBUST_MUTEX
	oK(pthread_mutex_unlock(&sdr->sdrMutex));
#else
	if (sdr->sdrSemaphore != -1)
	{
		sm_SemGive(sdr->sdrSemaphore);
	}
#endif
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

int	Sdr_initialize(size_t wmSize, char *wmPtr, int wmKey, char *wmName)
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

	if (wmName == NULL)
	{
		wmName = _SdrWmName();
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

void	sdr_shutdown(void)	/*	Ends SDR service on machine.	*/
{
	sm_WmParms	wmparms;

	if (_sdrwm(NULL) != NULL)
	{
		wmparms.wmKey = 0;
		wmparms.wmSize = 0;
		wmparms.wmAddress = NULL;
		wmparms.wmName = NULL;
		oK(_sdrwm(&wmparms));
	}

	_sdrlock(1);		/*	Delete SDR system semaphore.	*/
}

int	_xniEnd(const char *fileName, int lineNbr, const char *arg, Sdr sdrv)
{
	_postErrmsg(fileName, lineNbr,
			"Assertion failed, SDR transaction canceled.", arg);
	writeErrmsgMemos();
	printStackTrace();
	crashXn(sdrv);
	if (_coreFileNeeded(NULL))
	{
		sm_Abort();
	}

	return 1;
}

/*	*	Transaction utility functions	*	*	*	*/

/*	Logging is the mechanism that enables SDR transactions to be
	reversible.  The log for an SDR is a file or an extent of
	shared memory in which are recorded all of the SDR dataspace
	updates in the scope of the current transaction; the last byte
	of the log is the last byte of the last log entry.

	When the log is written to shared memory it is volatile and
	is lost on a system reset.  But when the log is written to
	a file it is non-volatile and therefore can be used to protect
	dataspace integrity in the event of an unplanned power cycle.
	The log file is truncated to length zero at the termination of
	each transaction.  Therefore, if it is of non-zero length at
	startup then all complete log entries in the log file must be
	backed out of the SDR heap in shared memory (if applicable) and
	the SDR heap in the dataspace file (if applicable) before the
	dataspace is accessed for any purpose: since this transaction
	was not ended, the dataspace may be in an inconsistent state.

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

static int  readFromLog(int logfile, char *logsm, size_t offset,
			char *into, size_t length, SdrState *sdr)
{
	if (logsm == NULL)	/* Log is in file.	*/
	{
		ssize_t bytesRead;

		if (lseek(logfile, offset, SEEK_SET) < 0)
		{
			putSysErrmsg("Can't seek in log file", itoa(offset));
			return -1;
		}

		bytesRead = read(logfile, into, length);

		/* Check for read error (-1) first, then for a partial read. */
		if (bytesRead < 0 || (size_t)bytesRead < length)
		{
			putSysErrmsg("Can't read from log file", itoa(length));
			return -1;
		}
	}
	else		/* Log is in memory.	*/
	{
		if (offset + length > sdr->logSize)
		{
			putErrmsg("Log entry extends past end of log.",
					itoa(length));
			return -1;
		}

		memcpy(into, logsm + offset, length);
	}

	return 0;
	}

static int	reverseTransaction(SdrState *sdr, int logfile, char *logsm,
			int dsfile, char *dssm)
{
	PsmPartition	sdrwm = _sdrwm(NULL);
	PsmAddress	elt;
	size_t		length;
	uaddr		logEntryOffset;
	uaddr		logEntryControl[2];	/*	Offset, length.	*/
	char		*buf;
	ssize_t		bytesWritten; /* Added for safe write() check. */

	if ((logfile == -1 && logsm == NULL))
	{
		return 0;	/*	No reversal possible.		*/
	}

	for (elt = sm_list_last(sdrwm, sdr->logEntries); elt;
			elt = sm_list_prev(sdrwm, elt))
	{
		logEntryOffset = (uaddr) sm_list_data(sdrwm, elt);
		length = sizeof logEntryControl;
		if (readFromLog(logfile, logsm, logEntryOffset,
				(char *) logEntryControl, length, sdr) < 0)
		{
			putErrmsg("Can't locate log entry.", NULL);
			return -1;
		}

		/*	Recover original data from log file.		*/

		logEntryOffset += length;	/*	Point at data.	*/
		length = logEntryControl[1];	/*	Data length.	*/
		if (dssm)
		{
			if (readFromLog(logfile, logsm, logEntryOffset, dssm
					+ logEntryControl[0], length, sdr) < 0)
			{
				putErrmsg("Can't read log entry.", NULL);
				return -1;
			}

			/*	If dataspace is also on file, recover.	*/

			if (dsfile != -1)
			{
				if (lseek(dsfile, logEntryControl[0], SEEK_SET) < 0)
				{
					putSysErrmsg("Can't seek in dataspace file", NULL);
					return -1;
				}

				bytesWritten = write(dsfile, dssm + logEntryControl[0], length);
				if (bytesWritten < 0 || (size_t)bytesWritten < length)
				{
					putSysErrmsg("Can't reverse log entry", NULL);
					return -1;
				}
			}
		}
		else	/*	Must create buffer for recovered data.	*/
		{
			if (dsfile != -1)
			{
				buf = MTAKE(length);
				if (buf == NULL)
				{
					putErrmsg("Log entry too big.",
						itoa(length));
					return -1;
				}

				if (readFromLog(logfile, logsm, logEntryOffset,
						buf, length, sdr) < 0)
				{
					putErrmsg("Can't read log entry.",
							NULL);
					MRELEASE(buf);
					return -1;
				}

				if (lseek(dsfile, logEntryControl[0], SEEK_SET) < 0)
				{
					putSysErrmsg("Can't seek in dataspace file", NULL);
					MRELEASE(buf);
					return -1;
				}

				bytesWritten = write(dsfile, buf, length);
				if (bytesWritten < 0 || (size_t)bytesWritten < length)
				{
					putSysErrmsg("Can't reverse log entry", NULL);
					MRELEASE(buf);
					return -1;
				}

				MRELEASE(buf);
			}
		}

		sdr->logLength -= (sizeof(logEntryControl) + length);
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
		if (sdrv->sdr->logSize == 0)	/*	Log is in file.	*/
		{
			isprintf(logfilename, sizeof logfilename,
					"%s%c%s.sdrlog", sdrv->sdr->pathName,
					ION_PATH_DELIMITER, sdrv->sdr->name);
			sdrv->logfile = iopen(logfilename,
					O_RDWR | O_CREAT | O_TRUNC, 0777);
			if (sdrv->logfile == -1)
			{
				putSysErrmsg("Can't open log file",
						logfilename);
			}
		}
	}

	sdrv->knownObjects.count = 0;

	sdrv->sdr->logLength = 0;
	sm_list_clear(_sdrwm(NULL), sdrv->sdr->logEntries, NULL, NULL);
}

#ifdef ION_HAVE_ROBUST_MUTEX

/*	recoverOrphanedXn is invoked when pthread_mutex_lock returns
 *	EOWNERDEAD, i.e. the process that held the transaction lock
 *	died mid-transaction.  The recovering caller already owns the
 *	lock; this rolls back whatever the dead owner left behind so
 *	the caller can begin a fresh transaction on a consistent SDR.
 *	On success the caller marks the mutex consistent; on failure
 *	it deliberately does not, so the mutex becomes ENOTRECOVERABLE
 *	rather than handing out a corrupt SDR.				*/

static int	recoverOrphanedXn(Sdr sdrv, char *deadInfo)
{
	static char	noRecord[] = "(no owner record)";
	SdrState	*sdr = sdrv->sdr;

	writeMemoNote("[?] SDR transaction lock owner died mid-transaction; \
recovering.", deadInfo ? deadInfo : noRecord);

	/*	A read-only transaction leaves nothing to undo.  If the
	 *	dead owner modified the heap, roll the changes back from
	 *	the transaction log.					*/

	if (sdr->modified)
	{
		if (!(sdr->configFlags & SDR_REVERSIBLE))
		{
			putErrmsg("Orphaned transaction modified a \
non-reversible SDR; cannot recover.", NULL);
			return -1;
		}

		putErrmsg("Reversing orphaned transaction...", NULL);
		if (reverseTransaction(sdr, sdrv->logfile, sdrv->logsm,
				sdrv->dsfile, sdrv->dssm) < 0)
		{
			putErrmsg("Can't reverse orphaned transaction.", NULL);
			return -1;
		}
	}

	/*	Discard the dead owner's transaction bookkeeping so the
	 *	recovering caller starts from a clean slate.		*/

	clearTransaction(sdrv);
	sdr->xnDepth = 0;
	sdr->xnCanceled = 0;
	sdr->modified = 0;
	sdr->sdrOwnerTask = -1;
	sdr->sdrOwnerThread = 0;
	return 0;
}

#endif	/*	ION_HAVE_ROBUST_MUTEX					*/

static void	handleUnrecoverableError(Sdr sdrv)
{
#ifdef DEBUG_BUILD
	putErrmsg("Unrecoverable SDR error: sdr_exit_xn called when SDR \
modifications were made (modified=1), faulty implementation or bug.", NULL);
	printStackTrace();
#else
	putErrmsg("Unrecoverable SDR error.", NULL);
#endif
#ifdef IN_FLIGHT
	sdr_abort(sdrv);

#else
	/* sdrv is unused when not compiling with IN_FLIGHT */
	(void)sdrv;

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

		if (sdr->modified)
		{
			handleUnrecoverableError(sdrv);
		}

		clearTransaction(sdrv);
		unlockSdr(sdr);
		return;
	}

	/*  Check if need to reverse modification.		 */
	if (sdr->modified)
	{
		/* there are changes to reverse, call reverseTransaction */
		putErrmsg("Attempting transaction reversal...", NULL);

		/*	Transaction must be reversed as necessary.		*/

		if (reverseTransaction(sdr, sdrv->logfile, sdrv->logsm, sdrv->dsfile,
				sdrv->dssm) < 0)
		{
			handleUnrecoverableError(sdrv);

			/*	In case not aborted....				*/

			clearTransaction(sdrv);
			unlockSdr(sdr);
			return;
		}
	}
	else
	{
		/* no modifications made, no need to reverse */
		putErrmsg("Transaction reversal not necessary.", NULL);
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
#ifdef ION_HAVE_ROBUST_MUTEX
	/*	The legacy semaphore path can leave the transaction lock
	 *	held here for the restart utility to recover later (via
	 *	sm_SemUnwedge when the profile is reloaded).  A robust
	 *	mutex cannot be recovered that way: the kernel only marks
	 *	it owner-died if the owner's robust list and the mutex are
	 *	still mapped at the moment the owner exits, and the task
	 *	that cancelled this transaction is about to unmap the SDR
	 *	working memory in ionDetach.  So close the transaction and
	 *	release the mutex cleanly now; the restarted ION simply
	 *	re-acquires it.						*/

	clearTransaction(sdrv);
	unlockSdr(sdr);
#endif
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
		sdr->xnCanceled = 1; /*	 Force reversal, if applicable. */
		sdr->xnDepth = 0;	/*	Unlock is immediate.	*/
		terminateXn(sdrv);
	}
}

/*	*	SDR dataspace administration functions.		*	*/

static int	reloadLogEntries(SdrState *sdr, int logfile)
{
	PsmPartition	sdrwm = _sdrwm(NULL);
	size_t		logFileLength;
	uaddr		logEntryControl[2];	/*	Offset, length.	*/
	size_t		lengthRead;
	size_t		endOfEntry;

	/*	Note that when the transaction log is written to memory,
	 *	either it is volatile and is destroyed on any power
	 *	reset or else it is non-volatile (battery-backed, or
	 *	a memory-mapped file), in which case the list of log
	 *	entries in the SdrState object is assumed also to be
	 *	non-volatile; so either there is no possibility of
	 *	reloading log entries or else there is no need to
	 *	reload log entries.  So this function is executed only
	 *	when the transaction log is written to a file.		*/

	logFileLength = lseek(logfile, 0, SEEK_END);
	sdr->logLength = 0;
	sm_list_clear(sdrwm, sdr->logEntries, NULL, NULL);
	while (1)
	{
		if (sdr->logLength + sizeof logEntryControl > logFileLength)
		{
			/*	From this point in the file, there
			 *	are too few remaining bytes to contain
			 *	the control fields for another log
			 *	entry.  So all complete log entries
			 *	in the file have been reloaded.		*/

			 return 0;
		}

		if (lseek(logfile, sdr->logLength, SEEK_SET) < 0)
		{
			putSysErrmsg("Can't move to next log entry", NULL);
			return -1;
		}

		lengthRead = read(logfile, (char *) logEntryControl,
				sizeof logEntryControl);
		if (lengthRead < sizeof logEntryControl)
		{
			putSysErrmsg("Can't read log entry", NULL);
			return -1;
		}

		endOfEntry = sdr->logLength + sizeof logEntryControl
				+ logEntryControl[1];
		if (endOfEntry > logFileLength)
		{
			/*	The log entry control fields for this
			 *	log entry were written to the log
			 *	file, but the program was interrupted
			 *	before the old data bytes were written.
			 *	Since writing log entries always
			 *	precedes writing to the dataspace, we
			 *	know that this log entry not only
			 *	cannot be reversed (we can't retrieve
			 *	the old data) but need not be reversed
			 *	(the new data weren't written either).
			 *	So we ignore this final log entry, and
			 *	we know that there are no subsequent
			 *	log entries to reload.			*/

			return 0;
		}

		if (sm_list_insert_last(sdrwm, sdr->logEntries,
				(PsmAddress) (sdr->logLength)) == 0)
		{
			putErrmsg("Can't reload log entry.", NULL);
			return -1;
		}

		sdr->logLength = endOfEntry;
	}
}

static size_t	getBigBuffer(char **buffer)
{
	size_t	bufsize = _sdrwmSize(NULL);

	/*	Temporarily take large buffer from SDR working memory.	*/

	while (1)
	{
		bufsize = bufsize / 2;
		if (bufsize == 0)
		{
			return 0;
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
	map->dsSize = sdr->dsSize;
	map->heapSize = sdr->heapSize;
	map->startOfSmallPool = sizeof(SdrMap);
	map->endOfSmallPool = map->startOfSmallPool;
	memset(map->smallPoolFree, 0, sizeof map->smallPoolFree);
	map->endOfLargePool = sdr->dsSize;
	map->startOfLargePool = map->endOfLargePool;
	memset(map->largePoolFree, 0, sizeof map->largePoolFree);
	map->largePoolSearchLimit = 0;
	map->unassignedSpace = map->startOfLargePool - map->endOfSmallPool;
#if 0
	map->inUse = 0;
	map->maxInUse = 0;
	map->smallPoolFree = 0;
	map->largePoolFree = 0;
#endif
}

static int	createDsFile(SdrState *sdr, char *dsfilename)
{
	size_t	bufsize;
	char	*buffer;
	int	dsfile;
	size_t	lengthRemaining;
	size_t	lengthToWrite;
	SdrMap	map;
	ssize_t bytesWritten; /* Added for safe write() check. */

	bufsize = getBigBuffer(&buffer);
	if (bufsize < 1)
	{
		putErrmsg("Can't get buffer in sdrwm.", NULL);
		return -1;
	}

	memset(buffer, 0 , bufsize);
	dsfile = iopen(dsfilename, O_RDWR | O_CREAT, 0777);
	if (dsfile == -1)
	{
		MRELEASE(buffer);
		putSysErrmsg("Can't create dataspace file", dsfilename);
		return -1;
	}

	lengthRemaining = sdr->dsSize;
	while (lengthRemaining > 0)
	{
		lengthToWrite = lengthRemaining;
		if (lengthToWrite > bufsize)
		{
			lengthToWrite = bufsize;
		}

		bytesWritten = write(dsfile, buffer, lengthToWrite);
		if (bytesWritten < 0 || (size_t)bytesWritten < lengthToWrite)
		{
			close(dsfile);
			unlink(dsfilename);
			MRELEASE(buffer);
			putSysErrmsg("Can't extend dataspace file", dsfilename);
			return -1;
		}

		lengthRemaining -= lengthToWrite;
	}

	MRELEASE(buffer);
	initSdrMap(&map, sdr);
	if (lseek(dsfile, 0, SEEK_SET) < 0)
	{
		close(dsfile);
		unlink(dsfilename);
		putSysErrmsg("Can't seek to start of dataspace file", dsfilename);
		return -1;
	}

	bytesWritten = write(dsfile, (char *) &map, sizeof map);
	if (bytesWritten < 0 || (size_t)bytesWritten < sizeof map)
	{
		close(dsfile);
		unlink(dsfilename);
		putSysErrmsg("Can't initialize dataspace file", dsfilename);
		return -1;
	}

	if (lseek(dsfile, 0, SEEK_SET) < 0)
	{
		close(dsfile);
		unlink(dsfilename);
		putSysErrmsg("Can't rewind dataspace file", dsfilename);
		return -1;
	}

	return dsfile;
}

static int	restageDsFromFile(SdrState *sdr, int dsfile, char *dssm)
{
	size_t	bytesRemaining = sdr->dsSize;
	size_t	offset;
	vast	bytesRead;

	offset = 0;
	while (bytesRemaining > 0)
	{
		bytesRead = read(dsfile, dssm + offset, bytesRemaining);
		if (bytesRead < 0)
		{
			if (errno == EINTR)
			{
				continue;
			}

			putSysErrmsg("Can't read from ds file", NULL);
			return -1;
		}

		bytesRemaining -= bytesRead;
		offset += bytesRead;
	}

	return 0;
}

#ifdef ION_HAVE_ROBUST_MUTEX

/*	On platforms with robust-mutex support the SDR transaction
 *	lock is a process-shared robust pthread mutex.  createSdrMutex
 *	is called once, by the process that creates the SDR; every
 *	other process inherits the mutex through the shared sdrwm
 *	partition.  PTHREAD_MUTEX_ROBUST is what makes a subsequent
 *	pthread_mutex_lock return EOWNERDEAD when the owning process
 *	dies mid-transaction, so the lock can be recovered instead of
 *	being orphaned.							*/

static int	createSdrMutex(SdrState *sdr)
{
	pthread_mutexattr_t	attr;

	if (pthread_mutexattr_init(&attr) != 0)
	{
		return -1;
	}

	if (pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED) != 0
	|| pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST) != 0
	|| pthread_mutex_init(&sdr->sdrMutex, &attr) != 0)
	{
		oK(pthread_mutexattr_destroy(&attr));
		return -1;
	}

	oK(pthread_mutexattr_destroy(&attr));
	ion_ipc_atomic_init(&sdr->sdrXnEnded, 0);
	memset(&sdr->lastOwner, 0, sizeof sdr->lastOwner);
	sdr->sdrMutexCreated = 1;
	return 0;
}

static void	destroySdrMutex(SdrState *sdr)
{
	if (sdr->sdrMutexCreated)
	{
		/*	sdrXnEnded is the robust-mutex analogue of
		 *	sm_SemEnd: setting it makes takeSdr refuse new
		 *	transactions and makes any task that acquires
		 *	the mutex during shutdown release it and bail
		 *	out (see lockSdr).  The snooze gives in-flight
		 *	lockers a moment to drain before the mutex is
		 *	destroyed.					*/

		ion_ipc_atomic_set(&sdr->sdrXnEnded, 1);
		microsnooze(50000);
		oK(pthread_mutex_destroy(&sdr->sdrMutex));
		sdr->sdrMutexCreated = 0;
	}
}

#endif	/*	ION_HAVE_ROBUST_MUTEX					*/

static void	destroySdr(SdrState *sdr)
{
	sm_SemId	lock = _sdrlock(0);
	PsmPartition	sdrwm = _sdrwm(NULL);
	char		dsfilename[PATHLENMAX + 1 + 32 + 1 + 3 + 1];
	char		logfilename[PATHLENMAX + 1 + 32 + 1 + 6 + 1];
	char		*dssm = NULL;
	uaddr		dssmId = 0;
	char		*logsm = NULL;
	uaddr		logsmId = 0;

	/*	Destroy global persistent SDR state.			*/

	if (sdr->sdrSemaphore != SM_SEM_NONE)
	{
		sm_SemEnd(sdr->sdrSemaphore);
		microsnooze(50000);
		sm_SemDelete(sdr->sdrSemaphore);
	}

#ifdef ION_HAVE_ROBUST_MUTEX
	destroySdrMutex(sdr);
#endif

	/*	Destroy file copy of dataspace if any.			*/

	if (sdr->configFlags & SDR_IN_FILE)
	{
		isprintf(dsfilename, sizeof dsfilename, "%s%c%s.sdr",
				sdr->pathName, ION_PATH_DELIMITER, sdr->name);
		unlink(dsfilename);
	}

	/*	Destroy memory copy of dataspace if any.		*/

	if (sdr->configFlags & SDR_IN_DRAM)
	{
		switch (sm_ShmAttach(sdr->dsKey, sdr->dsSize, &dssm, &dssmId))
		{
		case -1:
			break;

		default:
			sm_ShmDestroy(dssmId);
		}
	}

	/*	Destroy transaction log if any.				*/

	if (sdr->configFlags & SDR_REVERSIBLE)
	{
		if (sdr->logSize == 0)	/*	Log is in file.		*/
		{
			isprintf(logfilename, sizeof logfilename,
					"%s%c%s.sdrlog", sdr->pathName,
					ION_PATH_DELIMITER, sdr->name);
			unlink(logfilename);
		}
		else			/*	Log is in memory.	*/
		{
			switch (sm_ShmAttach(sdr->logKey, sdr->logSize, &logsm,
					&logsmId))
			{
			case -1:
				break;

			default:
				sm_ShmDestroy(logsmId);
			}
		}
	}

	/*	Destroy list of log entries if any.			*/

	if (sdr->logEntries)
	{
		oK(sm_list_destroy(sdrwm, sdr->logEntries, NULL, NULL));
	}

	/*	Unload profile and destroy it.				*/

	if (sdr->sdrsElt)
	{
		oK(sm_list_delete(sdrwm, sdr->sdrsElt, NULL, NULL));
	}

	/*	Wipe clean, in case it gets reused.			*/

	memset((char *) sdr, 0, sizeof(SdrState));
	psm_free(sdrwm, psa(sdrwm, sdr));
	sm_SemGive(lock);
}

int	sdr_load_profile(char *name, int configFlags, size_t heapWords,
		int heapKey, size_t logSize, int logKey, char *pathName,
		char *restartCmd)
{
	sm_SemId		lock = _sdrlock(0);
	PsmPartition		sdrwm = _sdrwm(NULL);
	SdrControlHeader	*sch = _sch(NULL);
	PsmAddress		elt;
	SdrState		*sdr;
	PsmAddress		newSdrAddress;
	size_t			limit;
	struct stat		statbuf;
	char			logfilename[PATHLENMAX + 1 + 32 + 1 + 6 + 1];
	int			logfile = -1;
	char			*logsm = NULL;
	uaddr			logsmId;
	char			dsfilename[PATHLENMAX + 1 + 32 + 1 + 3 + 1];
	int			dsfile = -1;
	char			*dssm = NULL;
	uaddr			dssmId;

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
#if (HEAP_PTRS)
	if (!(configFlags & SDR_IN_DRAM))
	{
		putErrmsg("When ION is compiled to use pointers to retrieve \
SDR heap data, the heap MUST be resident in memory.", itoa(configFlags));
		return -1;
	}
#endif
	if (lock == SM_SEM_NONE || sm_SemTake(lock) < 0)
	{
		putErrmsg("Can't lock SDR control header.", NULL);
		return -1;
	}

	if (!(configFlags & SDR_REVERSIBLE))
	{
		logSize = 0;
		logKey = SM_NO_KEY;
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
			&& (sdr->dsKey == heapKey || heapKey == SM_NO_KEY)
			&& sdr->logSize == logSize
			&& (sdr->logKey == logKey || logKey == SM_NO_KEY)
			&& strcmp(sdr->pathName, pathName) == 0)
			{
#ifndef ION_HAVE_ROBUST_MUTEX
				/*	Recover the transaction semaphore in
				 *	case a previous user died holding it.
				 *	The robust mutex needs no such nudge:
				 *	it recovers itself via EOWNERDEAD on
				 *	the next lock attempt.			*/

				sm_SemUnwedge(sdr->sdrSemaphore, 3);
#endif
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
		putErrmsg("Can't allocate memory for SDR state.", NULL);
		sm_SemGive(lock);
		return -1;
	}

	sdr = (SdrState *) psp(sdrwm, newSdrAddress);
	memset(sdr, 0, sizeof(SdrState));
	limit = sizeof(sdr->name) - 1;
	istrcpy(sdr->name, name, limit);
	sdr->configFlags = configFlags;
	sdr->initHeapWords = heapWords;
	sdr->heapSize = heapWords * WORD_SIZE;
	sdr->dsSize = sdr->heapSize + sizeof(SdrMap);
	if (heapKey == SM_NO_KEY)
	{
		heapKey = sm_GetUniqueKey();
	}

	sdr->dsKey = heapKey;
	sdr->logSize = logSize;
	if (logKey == SM_NO_KEY)
	{
		logKey = sm_GetUniqueKey();
	}

	sdr->logKey = logKey;
#ifdef ION_HAVE_ROBUST_MUTEX
	sdr->sdrSemaphore = SM_SEM_NONE;	/*	Mutex is the lock.	*/
	if (createSdrMutex(sdr) < 0)
	{
		putErrmsg("Can't create transaction mutex for SDR.", NULL);
		destroySdr(sdr);		/*	Releases lock.	*/
		return -1;
	}
#else
	sdr->sdrSemaphore = sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO);
	if (sdr->sdrSemaphore == SM_SEM_NONE)
	{
		putErrmsg("Can't create transaction semaphore for SDR.", NULL);
		destroySdr(sdr);		/*	Releases lock.	*/
		return -1;
	}
#endif

	sdr->sdrOwnerTask = -1;
	/*	logEntries is only ever accessed while the SDR transaction
	 *	mutex (or, at load/destroy time, the control-header lock) is
	 *	held, so its own per-list semaphore is redundant.  Create it
	 *	unlocked: a locked logEntries would leave that inner, non-
	 *	robust semaphore orphaned if the owner died mid-transaction,
	 *	deadlocking robust-mutex recovery in wipeList (issue #1265).	*/

	sdr->logEntries = sm_list_create_unlocked(sdrwm);
	if (sdr->logEntries == 0)
	{
		putErrmsg("Can't create log entries list for SDR.", NULL);
		destroySdr(sdr);		/*	Releases lock.	*/
		return -1;
	}

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
		putErrmsg("Can't insert SDR state into list of SDRs.", NULL);
		destroySdr(sdr);		/*	Releases lock.	*/
		return -1;
	}

	/*	If SDR exists, back out any current transaction.  If
	 *	not, create the SDR and initialize it.			*/

	if (sdr->configFlags & SDR_REVERSIBLE)
	{
		if (logSize > 0)	/*	Log in memory only.	*/
		{
			logsm = NULL;
			if (sm_ShmAttach(sdr->logKey, sdr->logSize, &logsm,
					&logsmId) < 0)
			{
				putErrmsg("Can't attach to log memory.", NULL);
				destroySdr(sdr);/*	Releases lock.	*/
				return -1;
			}
		}
		else			/*	Log is in file.		*/
		{
			isprintf(logfilename, sizeof logfilename,
					"%s%c%s.sdrlog", sdr->pathName,
					ION_PATH_DELIMITER, name);
			logfile = iopen(logfilename,
					O_RDWR | O_CREAT | O_APPEND, 0777);
			if (logfile == -1)
			{
				putSysErrmsg("Can't open log file",
						logfilename);
				destroySdr(sdr);/*	Releases lock.	*/
				return -1;
			}

			/*	Reload transaction log entries to
			 *	enable reversal.			*/

			if (reloadLogEntries(sdr, logfile) < 0)
			{
				close(logfile);
				putErrmsg("Can't reload log entries.", NULL);
				destroySdr(sdr);/*	Releases lock.	*/
				return -1;
			}
		}
	}

	if (sdr->configFlags & SDR_IN_FILE)
	{
		isprintf(dsfilename, sizeof dsfilename, "%s%c%s.sdr",
				sdr->pathName, ION_PATH_DELIMITER, name);
		dsfile = iopen(dsfilename, O_RDWR, 0777);
		if (dsfile == -1)	/*	No dataspace file.	*/
		{
			dsfile = createDsFile(sdr, dsfilename);
			if (dsfile == -1)	/*	Can't create.	*/
			{
				if (logfile != -1) close(logfile);
				if (logsm) sm_ShmDetach(logsm);
				putErrmsg("Can't have file-based dataspace",
						NULL);
				destroySdr(sdr);/*	Releases lock.	*/
				return -1;
			}
		}

		if (reverseTransaction(sdr, logfile, logsm, dsfile, NULL) < 0)
		{
			close(dsfile);
			if (logfile != -1) close(logfile);
			if (logsm) sm_ShmDetach(logsm);
			putErrmsg("Can't reverse log entries.", NULL);
			destroySdr(sdr);	/*	Releases lock.	*/
			return -1;
		}
	}

	if (sdr->configFlags & SDR_IN_DRAM)
	{
		dssm = NULL;
		if (sm_ShmAttach(sdr->dsKey, sdr->dsSize, &dssm, &dssmId) < 0)
		{
			if (dsfile != -1) close(dsfile);
			if (logfile != -1) close(logfile);
			if (logsm) sm_ShmDetach(logsm);
			putErrmsg("Can't attach to dataspace memory.", NULL);
			destroySdr(sdr);	/*	Releases lock.	*/
			return -1;
		}

		if (dsfile == -1)	/*	No backup in file.	*/
		{
			/*	Just initialize the dataspace.		*/

			initSdrMap((SdrMap *)(void *) dssm, sdr);
		}
		else			/*	Heap is also in file.	*/
		{
			/*	File is authoritative.			*/

			if (restageDsFromFile(sdr, dsfile, dssm) < 0)
			{
				sm_ShmDetach(dssm);
				if (dsfile != -1) close(dsfile);
				if (logfile != -1) close(logfile);
				if (logsm) sm_ShmDetach(logsm);
				putErrmsg("Can't load ds from file.", NULL);
				destroySdr(sdr);
				return -1;
			}
		}
	}

	if (logsm)
	{
		sm_ShmDetach(logsm);
	}

	if (logfile != -1)
	{
		close(logfile);
	}

	if (dssm)
	{
		sm_ShmDetach(dssm);
	}

	if (dsfile != -1)
	{
		close(dsfile);
	}

	sm_SemGive(lock);
	return 0;
}

int	sdr_reload_profile(char *name, int configFlags, size_t heapWords,
		int heapKey, size_t logSize, int logKey, char *pathName,
		char *restartCmd)
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

	if (!(configFlags & SDR_REVERSIBLE))
	{
		logSize = 0;
		logKey = SM_NO_KEY;
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
		|| (sdr->dsKey != heapKey && heapKey != SM_NO_KEY)
		|| sdr->logSize != logSize
		|| (sdr->logKey != logKey && logKey != SM_NO_KEY)
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

#ifdef ION_HAVE_ROBUST_MUTEX
		destroySdrMutex(sdr);
#else
		sm_SemEnd(sdr->sdrSemaphore);
		microsnooze(50000);
		sm_SemDelete(sdr->sdrSemaphore);
#endif
		psm_free(sdrwm, sdrAddress);
		oK(sm_list_delete(sdrwm, elt, NULL, NULL));
	}

	/*	Profile for this SDR is now known to be unloaded.	*/

	sm_SemGive(lock);
	return sdr_load_profile(name, configFlags, heapWords, heapKey, logSize,
			logKey, pathName, restartCmd);
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
	char			dsfilename[PATHLENMAX + 1 + 32 + 1 + 3 + 1];
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
	memset((char *) sdrv, 0, sizeof(SdrView));
	sdrv->sdr = sdr;
	if (sdr->configFlags & SDR_IN_FILE)
	{
		isprintf(dsfilename, sizeof dsfilename, "%s%c%s.sdr",
				sdr->pathName, ION_PATH_DELIMITER, name);
		sdrv->dsfile = iopen(dsfilename, O_RDWR, 0777);
		if (sdrv->dsfile == -1)
		{
			sm_SemGive(lock);
			putSysErrmsg("Can't open dataspace file", dsfilename);
			return NULL;
		}
	}
	else
	{
		sdrv->dsfile = -1;
	}

	if (sdr->configFlags & SDR_IN_DRAM)
	{
		sdrv->dssm = NULL;
		if (sm_ShmAttach(sdr->dsKey, sdr->dsSize, &(sdrv->dssm),
					&(sdrv->dssmId)) < 0)
		{
			sm_SemGive(lock);
			putErrmsg("Can't attach to dataspace in memory.",
					itoa(sdr->dsKey));
			return NULL;
		}
	}

	if (sdr->configFlags & SDR_REVERSIBLE)
	{
		if (sdr->logSize == 0)	/*	Log is in a file.	*/
		{
			isprintf(logfilename, sizeof logfilename,
					"%s%c%s.sdrlog", sdr->pathName,
					ION_PATH_DELIMITER, name);
			sdrv->logfile = iopen(logfilename,
					O_RDWR | O_CREAT | O_APPEND, 0777);
			if (sdrv->logfile == -1)
			{
				sm_SemGive(lock);
				putSysErrmsg("Can't open log file",
						logfilename);
				return NULL;
			}
		}
		else			/*	Log is in memory.	*/
		{
			sdrv->logfile = -1;
			sdrv->logsm = NULL;
			if (sm_ShmAttach(sdr->logKey, sdr->logSize,
					&(sdrv->logsm), &(sdrv->logsmId)) < 0)
			{
				sm_SemGive(lock);
				putErrmsg("Can't attach to log in memory.",
						itoa(sdr->logKey));
				return NULL;
			}
		}
	}
	else
	{
		sdrv->logfile = -1;
	}

	/*	knownObjects is already zeroed by the memset above;
	 *	the backing array is MTAKE'd lazily on first insert.	*/

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

size_t	sdr_heap_size(Sdr sdrv)
{
	CHKZERO(sdrv);
	return sdrv->sdr->heapSize;
}

void	sdr_stop_using(Sdr sdrv, int shutdown)
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

	if (sdrv->dsfile != -1)
	{
		close(sdrv->dsfile);
	}

	if (sdrv->dssm)
	{
		sm_ShmDetach(sdrv->dssm);
	}

	if (sdrv->logfile != -1)
	{
		close(sdrv->logfile);
	}

	if (sdrv->logsm)
	{
		sm_ShmDetach(sdrv->logsm);
	}

	if (sdrv->knownObjects.items)
	{
		MRELEASE(sdrv->knownObjects.items);
		sdrv->knownObjects.items = NULL;
		sdrv->knownObjects.capacity = 0;
		sdrv->knownObjects.count = 0;
	}

	/*	Erase content of SdrView, in case space is re-used
	 *	for another SdrView; then delete it.			*/

	memset((char *) sdrv, 0, sizeof(SdrView));
	psm_free(sdrwm, psa(sdrwm, sdrv));

	/*	Shut down SDR system if specified, otherwise leave SDR
		system intact but detach this process from SDR system	*/
	if (shutdown)
	{
		sdr_shutdown();
	}
	else
	{
		/*  Detach from SDR Working Memory */
		sm_ShmDetach(sdrwm->space);

		/*  Reset sdrwm database */
		sm_WmParms reset;
		reset.wmKey = -11111; 	/* use key value of -11111 to reset database */
		oK(_sdrwm(&reset));
	}
}

void	sdr_abort(Sdr sdrv)
{
	CHKVOID(sdrv);
#ifdef ION_HAVE_ROBUST_MUTEX
	destroySdrMutex(sdrv->sdr);
#else
	sm_SemEnd(sdrv->sdr->sdrSemaphore);
	microsnooze(50000);
	sm_SemDelete(sdrv->sdr->sdrSemaphore);
	sdrv->sdr->sdrSemaphore = -1;
#endif
	sdr_shutdown();
}

void	sdr_destroy(Sdr sdrv, int shutdown)
{
	sm_SemId	lock = _sdrlock(0);
	SdrState	*sdr;

	CHKVOID(sdrv);

	/*	All tracing of this SDR must stop, as the SDR is
	 *	going to be destroyed.					*/

#ifdef SDR_TRACE
	if (sdrv->trace)
	{
		sptrace_stop(sdrv->trace);
	}
#endif

	sdr = sdrv->sdr;
#ifdef ION_HAVE_ROBUST_MUTEX
	destroySdrMutex(sdr);
#else
	sm_SemEnd(sdr->sdrSemaphore);		/*	Interrupt.	*/
	microsnooze(50000);
	sm_SemDelete(sdr->sdrSemaphore);
	sdr->sdrSemaphore = SM_SEM_NONE;
#endif

	/*	Now destroy the SDR itself.				*/

	if (lock == SM_SEM_NONE || sm_SemTake(lock) < 0)
	{
		putErrmsg("Can't lock SDR control header.", NULL);
		return;
	}

	destroySdr(sdr);			/*	Releases lock.	*/

	/*	Destroy local access handle to this SDR.		*/

	sdr_stop_using(sdrv, shutdown);
}

/*	*	Low-level transaction functions		*	*	*/

#ifdef SDR_PERF_INSTRUMENTATION
int	Sdr_begin_xn(const char *file, int line, Sdr sdrv)
#else
int	sdr_begin_xn(Sdr sdrv)
#endif
{
	CHKZERO(sdrv);
	if (takeSdr(sdrv) < 0)
	{
		return 0;	/*	Failed to begin transaction.	*/
	}

#ifdef SDR_PERF_INSTRUMENTATION
	SDR_PERF_XN_BEGIN(&sdrv->perfStats, file, line);
#endif
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
			if (sdr->modified)
			{
				/*	Can't simply exit from a
				 *	transaction during which
				 *	data were modified - must
				 *	either end or cancel.  This
				 *	may happen during crash recovery
				 *	when call stack is unwinding.
				 *	Instead of aborting, trigger
				 *	transaction cancellation which
				 *	will attempt reversal.		*/

				writeMemo("[?] sdr_exit_xn called with \
modifications; triggering cancellation.");
				sdr->xnCanceled = 1;
				terminateXn(sdrv);
				return;
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
#ifdef SDR_PERF_INSTRUMENTATION
			SDR_PERF_XN_END(&sdr->perfCounters, &sdrv->perfStats);
#endif
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

/*	*	sdr_drop_xn -- cascade-safe transaction close		*/

static const char	*baseName(const char *path)
{
	const char	*slash;

	if (path == NULL)
	{
		return "?";
	}

	slash = strrchr(path, '/');
	return slash ? slash + 1 : path;
}

void	_sdr_drop_xn(const char *file, int line, const char *func, Sdr sdrv,
			const char *ctx)
{
	SdrState	*sdr;
	uint64_t	now;
	char		logBuf[SDR_DROP_LAST_SITE_MAX + 96];

	CHKVOID(sdrv);
	sdr = sdrv->sdr;
	CHKVOID(sdr);

	if (!sdr_in_xn(sdrv))
	{
		/*	sdr_drop_xn is a transaction-close primitive;
		 *	calling it outside a transaction is a programming
		 *	error.  Log loudly and return without doing
		 *	anything else.					*/

		putErrmsg("sdr_drop_xn called outside a transaction.",
				ctx ? ctx : "(no context)");
		return;
	}

	/*	Accounting.  We hold the SDR transaction mutex at this
	 *	point (sdr_in_xn was true), so updates to dropStats
	 *	are serialized across this SDR.  Readers from outside
	 *	the lock (e.g. ionwarn snapshotting) may see a torn
	 *	lastDropSite buffer briefly; the data is diagnostic
	 *	only.							*/

	now = monotonicMicrosecondsCommon();
	if (sdr->dropStats.totalDrops == 0)
	{
		sdr->dropStats.firstDropAtUs = now;
	}

	sdr->dropStats.lastDropAtUs = now;
	sdr->dropStats.totalDrops += 1;

	isprintf(sdr->dropStats.lastDropSite,
			sizeof sdr->dropStats.lastDropSite,
			"%s:%d %s: \"%s\"",
			baseName(file), line,
			func ? func : "?",
			ctx ? ctx : "(no context)");

	isprintf(logBuf, sizeof logBuf,
			"[?] sdr_drop_xn at %s (drops_total=%llu, pid=%d)",
			sdr->dropStats.lastDropSite,
			(unsigned long long) sdr->dropStats.totalDrops,
			(int) getpid());
	writeMemo(logBuf);

	/*	Commit the partial modifications.  We deliberately do
	 *	NOT route through sdr_cancel_xn / terminateXn: on a
	 *	non-reversible SDR with sdr->modified set, that path
	 *	calls handleUnrecoverableError -> sm_Abort, which is
	 *	exactly the cascade trigger sdr_drop_xn exists to
	 *	avoid (see #983).					*/

	oK(sdr_end_xn(sdrv));
}

int	sdr_drop_stats(Sdr sdrv, SdrDropStats *out)
{
	SdrState	*sdr;

	CHKERR(sdrv);
	CHKERR(out);
	sdr = sdrv->sdr;
	CHKERR(sdr);

	memcpy(out, &sdr->dropStats, sizeof *out);
	out->lastDropSite[sizeof out->lastDropSite - 1] = '\0';
	return 0;
}

unsigned long long	sdr_drop_count(Sdr sdrv)
{
	if (sdrv == NULL || sdrv->sdr == NULL)
	{
		return 0;
	}

	return sdrv->sdr->dropStats.totalDrops;
}

void *sdr_pointer(Sdr sdrv, SdrAddress address)
{
	CHKNULL(sdrv);
	if ((sdrv->sdr->configFlags & SDR_IN_DRAM) == 0 || address <= 0)
	{
		return NULL;
	}

	return (void *) (sdrv->dssm + address);
}

SdrAddress sdr_address(Sdr sdrv, void *pointer)
{
	char	*ptr;

	CHKZERO(sdrv);
	ptr = (char *) pointer;
	if ((sdrv->sdr->configFlags & SDR_IN_DRAM) == 0 || ptr <= sdrv->dssm)
	{
		return 0;
	}

	return (SdrAddress) (ptr - sdrv->dssm);
}

/*	*	Low-level I/O functions		*	*	*	*/

#ifdef NO_SDRMGT

SdrObject _sdrzalloc(Sdr sdrv, size_t nbytes)
{
	return 0;
}

SdrObject _sdrmalloc(Sdr sdrv, size_t nbytes)
{
	return 0;
}

void _sdrfree(Sdr sdrv, SdrObject, PutSrc);
{
	return;
}

int sdrBoundaryViolated(SdrView *sdrv, SdrAddress offset, size_t length)
{
	return 0;
}

#endif

static int	writeToLog(const char *file, int line, Sdr sdrv, char *from,
			size_t length)
{
	SdrState	*sdr = sdrv->sdr;
	ssize_t		bytesWritten; /* Added for safe write() check. */

	if (sdr->logSize == 0)		/*	Log is in file.		*/
	{
		bytesWritten = write(sdrv->logfile, from, length);
		if (bytesWritten < 0 || (size_t)bytesWritten != length)
		{
			_putSysErrmsg(file, line, "Can't write log entry",
					itoa(length));
			return -1;
		}
	}
	else				/*	Log is in memory.	*/
	{
		if (sdr->logLength + length > sdr->logSize)
		{
			char buf[256];
			isprintf(buf, sizeof(buf), "Log max size exceeded. \
SDR: %s  logSize: %lu logLength: %lu length: %lu depth: %d",
					sdr->name, (unsigned long) sdr->logSize,
					(unsigned long) sdr->logLength,
					(unsigned long) length, sdr->xnDepth);
			_putErrmsg(file, line, buf, NULL);
			return -1;
		}

		memcpy(sdrv->logsm + sdr->logLength, (char *) from, length);
	}

	sdr->logLength += length;
	if ( sdr->logLength > sdr->maxLogLength )
	{
		sdr->maxLogLength = sdr->logLength;
	}

	return length;
}

void _sdrput(const char *file, int line, Sdr sdrv, SdrAddress into, char *from,
		size_t length, PutSrc src)
{
	SdrState	*sdr;
	uaddr		logEntryControl[2];
	char		*buffer;
	size_t		logOffset;
	/* --- Added for safe I/O checks. --- */
	ssize_t     bytesRead;
	ssize_t     bytesWritten;
#ifdef SDR_PERF_INSTRUMENTATION
	struct timeval	perfStart;
#endif

	if (length == 0)
	{
		return;
	}

	CHKVOID(length > 0);
	CHKVOID(sdrv);
	CHKVOID(from);
	sdr = sdrv->sdr;

	/*	Defensive check: verify current task still owns the
	 *	transaction.  After transaction crash/cancellation,
	 *	another task may have taken ownership while this
	 *	task's call stack is still unwinding.  If we don't
	 *	own the transaction, silently return to avoid
	 *	corrupting the new owner's transaction.		*/

	if (sdr->sdrOwnerTask != sm_TaskIdSelf())
	{
#ifdef DEBUG_BUILD
		char	buf[128];
		isprintf(buf, sizeof(buf),
			"[!] SDR write owner mismatch: sdrOwnerTask=%d, sm_TaskIdSelf()=%d",
			sdr->sdrOwnerTask, sm_TaskIdSelf());
		writeMemo(buf);
		printStackTrace();
#endif
		return;		/*	No longer transaction owner.	*/
	}
	if (length > sdr->dsSize || into > sdr->dsSize - length)
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

#ifdef SDR_PERF_INSTRUMENTATION
	SDR_PERF_WRITE_BEGIN(&sdrv->perfStats, &perfStart);
#endif
	if (sdr->configFlags & SDR_REVERSIBLE)
	{
		logOffset = sdr->logLength;	/*	Before writing.	*/
		logEntryControl[0] = into;
		logEntryControl[1] = length;
		if (writeToLog(file, line, sdrv, (char *) logEntryControl,
				sizeof logEntryControl) < 0)
		{
			_putSysErrmsg(file, line, "Can't write logEntryControl",
					NULL);
			crashXn(sdrv);
			return;
		}

		if (sdr->configFlags & SDR_IN_DRAM)
		{
			if (writeToLog(file, line, sdrv, sdrv->dssm + into,
					length) < 0)
			{
				_putSysErrmsg(file, line, "Can't write log \
entry", itoa(length));
				crashXn(sdrv);
				return;
			}
		}
		else	/*	Dataspace is only in file.		*/
		{
			buffer = MTAKE(length);
			if (buffer == NULL)
			{
				_putErrmsg(file, line, "Not enough memory for \
log entry.", itoa(length));
				crashXn(sdrv);
				return;
			}

			if (lseek(sdrv->dsfile, into, SEEK_SET) < 0)
			{
				MRELEASE(buffer);
				_putSysErrmsg(file, line, "Can't seek to read old data",
						itoa(length));
				crashXn(sdrv);
				return;
			}

			bytesRead = read(sdrv->dsfile, buffer, length);
			if (bytesRead < 0 || (size_t)bytesRead < length)
			{
				MRELEASE(buffer);
				_putSysErrmsg(file, line, "Can't read old data",
						itoa(length));
				crashXn(sdrv);
				return;
			}

			if (writeToLog(file, line, sdrv, buffer, length) < 0)
			{
				MRELEASE(buffer);
				_putSysErrmsg(file, line, "Can't write log \
entry", itoa(length));
				crashXn(sdrv);
				return;
			}

			MRELEASE(buffer);
		}

		/*	Store location of newly written log entry.	*/

		if (sm_list_insert_last(_sdrwm(NULL), sdr->logEntries,
				(PsmAddress) logOffset) == 0)
		{
			_putErrmsg(file, line, "Can't note transaction log \
entry.", NULL);
			crashXn(sdrv);
			return;
		}
	}

	if (sdr->configFlags & SDR_IN_FILE)
	{
		if (lseek(sdrv->dsfile, into, SEEK_SET) < 0)
		{
			_putSysErrmsg(file, line, "Can't seek to write to dataspace",
					itoa(length));
			crashXn(sdrv);
			return;
		}

		bytesWritten = write(sdrv->dsfile, from, length);
		if (bytesWritten < 0 || (size_t)bytesWritten < length)
		{
			_putSysErrmsg(file, line, "Can't write to dataspace",
					itoa(length));
			crashXn(sdrv);
			return;
		}
	}

	if (sdr->configFlags & SDR_IN_DRAM)
	{
		memcpy(sdrv->dssm + into, from, length);
	}

#ifdef SDR_PERF_INSTRUMENTATION
	SDR_PERF_WRITE_END(&sdrv->perfStats, &perfStart, length);
#endif
	sdr->modified = 1;
}

void Sdr_write(const char *file, int line, Sdr sdrv, SdrAddress into,
		char *from, size_t length)
{
	if (!(sdr_in_xn(sdrv)))
	{
		oK(_iEnd(file, line, _notInXnMsg()));
		return;
	}

	joinTrace(sdrv, file, line);
	_sdrput(file, line, sdrv, into, from, length, UserPut);
}

void _sdrfetch(Sdr sdrv, char *into, SdrAddress from, size_t length)
{
	SdrState	*sdr;
	ssize_t		bytesRead; /* Added for safe read() check. */
#ifdef SDR_PERF_INSTRUMENTATION
	struct timeval	perfStart;
#endif

	if (length == 0)			/*	Nothing to do.	*/
	{
		return;
	}

	CHKVOID(length > 0);
	CHKVOID(sdrv);
	CHKVOID(into);
	sdr = sdrv->sdr;
	CHKVOID(sdr);
	if (length > sdr->dsSize || from > sdr->dsSize - length)
	{
		memset(into, 0, length);	/*	Default value.	*/
		putErrmsg(_violationMsg(), "read");
		crashXn(sdrv);			/*	Releases SDR.	*/
		return;
	}

#ifdef SDR_PERF_INSTRUMENTATION
	SDR_PERF_READ_BEGIN(&sdrv->perfStats, &perfStart);
#endif
	if (sdr->configFlags & SDR_IN_DRAM)
	{
		memcpy(into, sdrv->dssm + from, length);
	}
	else
	{
		memset(into, 0, length);	/*	Default value.	*/
		if (sdr->configFlags & SDR_IN_FILE)
		{
			/* --- Start of corrected block --- */
			if (lseek(sdrv->dsfile, from, SEEK_SET) < 0)
			{
				putSysErrmsg("Dataspace seek failed", itoa(from));
				crashXn(sdrv);  /* Releases SDR.   */
				return;
			}

			bytesRead = read(sdrv->dsfile, into, length);
			if (bytesRead < 0 || (size_t)bytesRead < length)
			{
				putSysErrmsg("Dataspace read failed",
						itoa(length));
				crashXn(sdrv);  /* Releases SDR.   */
				return;
			}
		}
	}
#ifdef SDR_PERF_INSTRUMENTATION
	SDR_PERF_READ_END(&sdrv->perfStats, &perfStart, length);
#endif
}

void sdr_read(Sdr sdrv, char *into, SdrAddress from, size_t length)
{
	_sdrfetch(sdrv, into, from, length);
}
