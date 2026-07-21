/*
 *	sdrP.h:	private definitions for spacecraft data recorder
 *		management library.
 *
 *	    Modification history:
 *		01-02-01  SCB	Revised for Solaris shm, multiple logs.
 *		03-08-96  APS	Abstracted the IPC services.
 *
 *	Copyright (c) 2001, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 */

#ifndef SDRP_H
#define SDRP_H

#ifndef NO_SDR_TRACE
#define	SDR_TRACE
#endif

#include "platform.h"
#include "memmgr.h"
#include "psm.h"
#include "lyst.h"
#include "smlist.h"
#include "sdrxn.h"
#include "ion_atomic.h"

#ifdef SDR_TRACE
#include "sptrace.h"
#endif

#ifdef SDR_PERF_INSTRUMENTATION
#include "sdr_perf.h"
#endif

#define SDR_SM_KEY	(255 * 256)
#define SDR_SM_NAME	"sdrwm"

/*	The structure of an SDR dataspace (DS) is as follows, where:
		M = sizeof(SdrMap)
		D = total size of SDR DS (includes map and heap)

	From offset	Until offset		Content is:

	0		M			map of sdr heap
	M		D			sdr heap (objects)	*/


typedef struct
{
	SdrAddress from; /* 1st byte of object */
	SdrAddress to;	 /* 1st byte beyond scope of object */
} ObjectExtent;

/*	Sorted (by 'from') dynamic array of ObjectExtents.  Backs the
 *	SDR_BOUNDED write-validation set per transaction.  Extents are
 *	non-overlapping, so a predecessor binary search suffices to
 *	answer "is [from, from+length) contained in any known extent?".	*/

typedef struct
{
	ObjectExtent	*items;
	size_t		count;
	size_t		capacity;
} ExtentArray;

#ifdef ION_HAVE_ROBUST_MUTEX

/*	SdrLockOwner records who acquired the SDR transaction lock,
 *	captured at acquire-time and read by the next acquirer if the
 *	previous owner died mid-transaction (EOWNERDEAD).  It lives
 *	in shared memory next to sdrMutex so the recovery log can
 *	identify the orphan by pid + cmdline without external tooling.
 *	Sized to fit close to one cache line — 64 bytes covers every
 *	realistic ION daemon invocation (bpadmin '.', tcc 203,
 *	udplsi [::1]:1113, etc.).					*/

#define SDR_LOCK_OWNER_CMDLINE_MAX	64

typedef struct
{
	uint64_t	acquired_at_us;	/*	CLOCK_MONOTONIC	*/
	int		pid;
	char		cmdline[SDR_LOCK_OWNER_CMDLINE_MAX];
} SdrLockOwner;

#endif	/*	ION_HAVE_ROBUST_MUTEX					*/

#define	INITIALIZED	(0x99999999)

/*	Memory management abstraction.					*/
#define MTAKE(size)	allocFromSdrMemory(__FILE__, __LINE__, size)
#define MRELEASE(addr)	releaseToSdrMemory(__FILE__, __LINE__, addr)
extern void		*allocFromSdrMemory(const char *, int, size_t);
extern void		releaseToSdrMemory(const char *, int, void *);
extern void		*sdrMemAtoP(uaddr);
extern uaddr		sdrMemPtoA(void *);

/*	SdrControlHeader is the object in the root of the SDR working
 *	memory (a shared memory partition) that enables multiple
 *	applications to use the SDR system on a single computer
 *	concurrently.							*/

typedef struct
{
	PsmAddress	sdrs;	/*	An SmList of (SdrState *).	*/
} SdrControlHeader;

/*	SdrState is an object that encapsulates the volatile state of
 *	a single SDR.  It resides in SDR working memory (a shared
 *	memory partition), in the control header's list of sdrs.	*/

typedef struct sdr_str
{
		/*	General SDR operational parameters.	*/

	char		name[32];
	PsmAddress	sdrsElt;		/*	In sch->sdrs.	*/
	int		configFlags;
	size_t		initHeapWords;		/*	In FULL WORDS.	*/
	size_t		heapSize;		/*	dsSize - map	*/
	size_t		dsSize;			/*	heap + map	*/
	int		dsKey;			/*	RAM DS shmKey	*/
	size_t		logSize;		/*	(if in memory)	*/
	int		logKey;			/*	RAM log shmKey	*/

		/*	Parameters of current transaction.	*/

	sm_SemId	sdrSemaphore;
#ifdef ION_HAVE_ROBUST_MUTEX
		/*	On platforms with robust-mutex support the
		 *	transaction lock is a process-shared robust
		 *	pthread mutex instead of sdrSemaphore, so that
		 *	a process dying mid-transaction is recovered
		 *	via EOWNERDEAD rather than orphaning the lock.
		 *	sdrXnEnded carries the shutdown signal that
		 *	sm_SemEnded() provided on the semaphore path;
		 *	it is in shared memory, so it must be a
		 *	lock-free IPC atomic, not a process-local one.	*/

	pthread_mutex_t	 sdrMutex;		/*	Robust xn lock.	*/
	ion_ipc_atomic_t sdrXnEnded;		/*	Boolean.	*/
	int		 sdrMutexCreated;	/*	Boolean.	*/
	SdrLockOwner	 lastOwner;	/*	For EOWNERDEAD diag.	*/
#endif
	int		sdrOwnerTask;		/*	Task ID.	*/
	pthread_t	sdrOwnerThread;		/*	Thread ID.	*/
	int		xnDepth;
	int		modified;	/*	Boolean.		*/
	int		xnCanceled;		/*	Boolean.	*/
	int		logLength;		/*	All entries.	*/
	int		maxLogLength;		/*	Max Log Length  */
	PsmAddress	logEntries;		/*	Offsets in log.	*/

	SdrDropStats	dropStats;	/*	sdr_drop_xn accounting.	*/

		/*	SDR trace data access.			*/

	int		traceKey;		/*	trace shmKey	*/
	size_t		traceSize;		/*	0 = disabled	*/
	int		traceCount;		/*	episode counter	*/

		/*	Path to directory for files (log, ds).	*/

	char		pathName[MAXPATHLEN];

		/*	Parameters for restart.				*/

	int		halted;			/*	boolean		*/
	char		restartCmd[32];
	time_t		restartTime;

#ifdef SDR_PERF_INSTRUMENTATION
		/*	Performance instrumentation counters.		*/

	SdrPerfCounters	perfCounters;
#endif
} SdrState;

typedef struct
{
	SdrAddress	firstFreeBlock;
	size_t		freeBlocks;
} SmallFreeBucket;

typedef struct
{
	SdrAddress	firstFreeBlock;
	size_t		freeBlocks;
	size_t		freeBytes;
} LargeFreeBucket;

/*	SdrMap is an object that encapsulates the potentially non-
 *	volatile space management state of a single SDR.  It resides
 *	at the front of the SDR DS itself, preceding the SDR's heap.
 *	Since an SDR may be written to a file in addition to (or
 *	even rather than) occupying a shared-memory partition, the
 *	SDR's map can persist after reboot of the computer in which
 *	the SDR resides.  When an SDR is to be added to the sdrs
 *	list of the SDR control header, we can to some small extent
 *	assure that the SDR being added is the one we think we're
 *	adding; to do this, we check the dataspace size declared in
 *	sdr_load_profile() against the dsSize in the SDR's map.		*/

typedef struct	/*	Non-volatile state at front of SDR.		*/
{
	SdrObject	catalogue;		/*	partition root	*/
	unsigned int	status;			/*	INITIALIZED?	*/
	size_t		dsSize;			/*	Map + heap.	*/
	size_t		heapSize;

		/*	For dynamic management of heap space.	*/

	SdrAddress	startOfSmallPool;
	SdrAddress	endOfSmallPool;
	SmallFreeBucket	smallPoolFree[SMALL_SIZES];
	SdrAddress	startOfLargePool;
	SdrAddress	endOfLargePool;
	LargeFreeBucket	largePoolFree[LARGE_ORDERS];
	unsigned int	largePoolSearchLimit;
	size_t		unassignedSpace;
} SdrMap;

/*	SdrView is an object that encapsulates a single process's
 *	transient private access to a single SDR.  It resides in
 *	SDR working memory (a shared memory partition) but is private
 *	to the process.  This is the structure that is returned by
 *	the sdr_start_using() function.					*/

typedef struct sdrv_str
{
	SdrState	*sdr;		/*	Local SDR state access.	*/

	int		dsfile;		/*	DS in file (fd).	*/
	char		*dssm;		/*	DS in shared memory.	*/
	uaddr		dssmId;		/*	DS shmId if applicable.	*/

	int		logfile;	/*	Xn log file (fd).	*/
	char		*logsm;		/*	Log in shared memory.	*/
	uaddr		logsmId;	/*	Log shmId if applicable.*/

	ExtentArray	knownObjects;	/*	SDR_BOUNDED bookkeeping	*/

	PsmView		traceArea;	/*	local access to trace	*/
	PsmView		*trace;		/*	local access to trace	*/
	const char	*currentSourceFileName;	/*	for tracing	*/
	int		currentSourceFileLine;	/*	for tracing	*/

#ifdef SDR_PERF_INSTRUMENTATION
		/*	Per-transaction performance stats.		*/

	SdrPerfStats	perfStats;
#endif
} SdrView;

typedef enum { UserPut = 0, SystemPut } PutSrc;

extern int		takeSdr(Sdr sdrv);
extern void		releaseSdr(SdrState *sdr);

extern void		joinTrace(Sdr, const char *, int);

extern SdrMap		*_mapImage(Sdr sdrv);

#ifndef SDR_TRACE
extern char		*_noTraceMsg(void);
#endif

extern char		*_notInXnMsg(void);
extern char		*_apiErrMsg(void);
extern char		*_noMemoryMsg(void);
extern char		*_violationMsg(void);

#define ADDRESS_OF(X)	(((char *) &(map->X)) - ((char *) map))

extern void _sdrput(const char *, int, Sdr, SdrAddress, char *, size_t, PutSrc);
#define	sdrPatch(A,V)	_sdrput(__FILE__, __LINE__, sdrv, (A), (char *) &(V), \
sizeof (V), SystemPut)
#define	patchMap(X,V)	_sdrput(__FILE__, __LINE__, sdrv, ADDRESS_OF(X), \
(char *) &(V), sizeof map->X, SystemPut)
#define	sdrPut(A,V)	_sdrput(file, line, sdrv, (A), (char *) &(V), \
sizeof (V), SystemPut)

extern void _sdrfetch(Sdr, char *, SdrAddress, size_t);
#define sdrFetch(V,A)	_sdrfetch(sdrv, (char *) &(V), (A), sizeof (V))

extern SdrObject _sdrzalloc(Sdr, size_t);
extern SdrObject _sdrmalloc(Sdr, size_t);
extern void	 _sdrfree(Sdr, SdrObject, PutSrc);
#define sdrFree(Obj)	_sdrfree(sdrv, Obj, SystemPut)

extern int		sdrBoundaryViolated(Sdr, SdrAddress, size_t);
extern int		sdrFetchSafe(Sdr);

extern void		crashXn(Sdr);

#endif /* SDRP_H */
