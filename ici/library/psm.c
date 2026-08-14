/*
	psm.c:	personal space management library implementation.

	Originally designed for managing a spacecraft solid-state recorder
	that takes the form of a fixed-size pool of static RAM with a flat
	address space.
									*/
/*	Copyright (c) 1997, California Institute of Technology.		*/
/*	ALL RIGHTS RESERVED.  U.S. Government Sponsorship		*/
/*	acknowledged.							*/
/*									*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#ifndef NO_PSM_TRACE
#define	PSM_TRACE
#endif

#include "psm.h"
#include "smlist.h"

#ifdef PSM_TRACE
#include "sptrace.h"
#endif

#include <stdint.h>

/*
 * PSM_BLK_ALIGN is the alignment grain (and overhead size) of small-pool
 * blocks.  It must be >= any basic C type's alignment requirement: on RTEMS 6
 * sparc/leon3 `time_t` (and `long long`) require 8-byte alignment, which the
 * historical WORD_SIZE=4 grain on 32-bit targets did not satisfy.  Set it to
 * the larger of WORD_SIZE and 8 so user data is always suitably aligned for
 * any type; on LP64 targets this is identical to WORD_SIZE.
 */
#if WORD_SIZE >= 8
#define PSM_BLK_ALIGN_ORDER SPACE_ORDER
#else
#define PSM_BLK_ALIGN_ORDER 3
#endif
#define PSM_BLK_ALIGN	(1UL << PSM_BLK_ALIGN_ORDER)

#define SMALL_BLOCK_OHD (PSM_BLK_ALIGN)
#define SMALL_BLK_LIMIT (SMALL_SIZES * PSM_BLK_ALIGN)

#if SPACE_ORDER ==3	/* 64-bit machine	*/
#define	SMALL_IN_USE	((PsmAddress) 0xffffffffffffff00)
#define	BLK_IN_USE	((PsmAddress) 0xffffffffffffffff)
#else
#define	SMALL_IN_USE	((PsmAddress) 0xffffff00)
#define	BLK_IN_USE	((PsmAddress) 0xffffffff)
#endif

/*----------------------------------------------------------------------------
 *  A local mutex that only protects ownerTask, ownerThread, depth from TSan
 *  warnings in *this process*.
 *----------------------------------------------------------------------------*/
static pthread_mutex_t  _psmLocalMutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * The overhead on a small block is PSM_BLK_ALIGN bytes (= WORD_SIZE on LP64;
 * widened to 8 on 32-bit targets so the user data area carries 8-byte
 * alignment for any basic C type).  When the block is free, the first
 * WORD_SIZE bytes contain the PsmAddress (i.e., offset from start of
 * partition) of the next free block, which must be an integral multiple of
 * PSM_BLK_ALIGN.  When the block is in use, the low-order byte is set to the
 * size of the block's user data (expressed as an integer 1 through
 * SMALL_SIZES: the total block size minus overhead, divided by PSM_BLK_ALIGN)
 * and all higher-order bytes are set to 0xff.
 *
 * NOTE: since any PsmAddress in excess of SMALL_IN_USE will be
 * interpreted as indicating that the block is in use, the maximum
 * size of the small pool is SMALL_IN_USE; the address of any free
 * block at a higher address would be unrecognizable as such.
 */
struct small_ohd
{
	PsmAddress	next;
};

/*
 * The overhead on a large block is in two parts that MUST BE THE SAME SIZE,
 * a leading overhead area and a trailing overhead area.  The first word of
 * the leading overhead contains the size of the block's user data.
 * The first word of the trailing overhead contains a pointer to
 * the start of the leading overhead.  When the block is in use, the second
 * word of the leading overhead and the second word of the trailing overhead
 * both contain BLK_IN_USE; when the block is free, the second word of the
 * leading overhead points to the next free block and the second word of
 * the trailing overhead points to the preceding free block.
 *
 * To ensure correct alignment of the trailing overhead area, the user
 * data area in a large block must be an integral multiple of the leading
 * and trailing overhead areas' size.
 */

struct big_ohd1				/*	Leading overhead.	*/
{
	size_t		userDataSize;	/*	in bytes, not words	*/
	PsmAddress	next;
};

struct big_ohd2				/*	Trailing overhead.	*/
{
	PsmAddress	start;
	PsmAddress	prev;
};

#define SMALL(x)	((struct small_ohd *)(void *)(((char *) map) + x))
#define BIG1(x)		((struct big_ohd1 *)(void *)(((char *) map) + x))
#define BIG2(x)		((struct big_ohd2 *)(void *)(((char *) map) + x))
#define PTR(x)		(((char *) map) + x)

#define	LG_OHD_SIZE	(1 << LARGE_ORDER1)	/*	double word	*/
#define	LARGE_BLOCK_OHD	(2 * LG_OHD_SIZE)
#define	MIN_LARGE_BLOCK	(3 * LG_OHD_SIZE)
#define	LARGE_BLK_LIMIT	(LARGE1 << LARGE_ORDERn)

#define	INITIALIZED	(0x99999999)
#define	MANAGED		(0xbbbbbbbb)

typedef struct
{
	PsmAddress	firstFreeBlock;
	size_t		freeBlocks;
} SmallFreeBucket;

typedef struct
{
	PsmAddress	firstFreeBlock;
	size_t		freeBlocks;
	size_t		freeBytes;
} LargeFreeBucket;

/*
 * PartitionMap precedes the small pool inside the partition, so its size
 * determines where the first small block lands. Force 8-byte alignment (and
 * therefore size, since C requires the structure's total size is rounded up to
 * a multiple of its strictest alignment) so the small pool starts on a
 * PSM_BLK_ALIGN boundary even on 32-bit targets where every field is only
 * WORD_SIZE (=4) aligned.
 *
 * _Alignas(8) on the structure definition is how we'd ideally enforce this,
 * but since ION still needs to support C99, we append an 8-byte wide member
 * (align8).
 */
typedef struct			/*	Global view in shared memory.	*/
{
	PsmAddress	directory;
	unsigned int	status;
	sm_SemId	semaphore;	/*	Legacy non-robust path.	*/
#ifdef ION_HAVE_ROBUST_MUTEX
	/*	On platforms with robust-mutex support the partition
	 *	lock is a process-shared robust pthread mutex instead of
	 *	the legacy POSIX semaphore.  PTHREAD_MUTEX_ROBUST makes
	 *	pthread_mutex_lock return EOWNERDEAD when the holder
	 *	died mid-operation, so the lock can be recovered (the
	 *	allocator state cannot be rolled back -- no log -- but
	 *	the partition becomes usable again instead of every
	 *	subsequent caller wedging on an orphaned semaphore).	*/
	pthread_mutex_t	partLock;
	int		partLockCreated;	/*	Boolean.	*/
#endif
	int		ownerTask;	/*	Last took the lock.	*/
	pthread_t	ownerThread;	/*	Last took the lock.	*/
	int		depth;		/*	Count of nested takes.	*/
	int		desperate;
	size_t		partitionSize;
	char		name[32];
	int		traceKey;	/*	For sptrace.		*/
	size_t		traceSize;	/*	0 = trace disabled.	*/
	int		traceCount;	/*	Trace episode counter.	*/
	PsmAddress	startOfSmallPool;
	PsmAddress	endOfSmallPool;
	SmallFreeBucket	smallPoolFree[SMALL_SIZES];
	PsmAddress	startOfLargePool;
	PsmAddress	endOfLargePool;
	LargeFreeBucket	largePoolFree[LARGE_ORDERS];
	size_t		unassignedSpace;
	uint64_t	align8;
} PartitionMap;

typedef struct
{
	char		name[33];
	PsmAddress	address;
} PsmCatlgEntry;

static char	*_outOfSpaceMsg(void)
{
	return "Not enough available memory.";
}

static char	*_badBlockSizeMsg(void)
{
	return "Can't allocate, illegal block size.";
}

#ifndef PSM_TRACE
static char	*_noTraceMsg()
{
	return "Tracing unavailable; recompile with -DPSM_TRACE.";
}
#endif

/*	*	Non-platform-specific implementation	*	*	*/

/******************************************************************************
 *  lockPartition
 *
 *  Acquires the partition for the calling thread.  If this thread already owns
 *  the partition, simply increments the re-entrant depth. Otherwise, takes
 *  cross-process ownership (via sm_SemTake()) and marks the partition as owned.
 *
 *  A process-local mutex (_psmLocalMutex) is used to protect reads/writes to
 *  ownerTask, ownerThread, and depth against concurrent threads in the same
 *  process.
 ******************************************************************************/
static void lockPartition(PartitionMap *map)
{
	int		selfTask;
	pthread_t	selfThread;

	CHKVOID(map->status == MANAGED);
	selfTask = sm_TaskIdSelf();
	selfThread = pthread_self();

	/*
	 * Acquire local mutex to safely read the ownership fields;
	 * prevents data races within this process.
	 */
	pthread_mutex_lock(&_psmLocalMutex);

	/* Re-entrant call by this thread; just increment depth. */
	if (map->ownerTask == selfTask
	&& pthread_equal(map->ownerThread, selfThread))
	{
		map->depth++;
		pthread_mutex_unlock(&_psmLocalMutex);
		return;
	}

	/*  If we get here, we do NOT own the partition, so we must release
	 *  the local mutex before blocking on the cross-process lock.
	 *  (Otherwise we can deadlock ourselves.)
	 */
	pthread_mutex_unlock(&_psmLocalMutex);

#ifdef ION_HAVE_ROBUST_MUTEX
	{
		int	rc;

		CHKVOID(map->partLockCreated);
		rc = pthread_mutex_lock(&map->partLock);
		if (rc == EOWNERDEAD)
		{
			/*	The previous holder died mid-operation
			 *	while holding the lock.  PSM has no
			 *	transaction log; the partition allocator
			 *	(small/large pool free lists, in-use bits,
			 *	the catalog) may be in any intermediate
			 *	state the dead writer left behind.
			 *	Continuing would risk silent corruption --
			 *	a misthreaded free list or use-after-free
			 *	is far worse than the wedge we set out to
			 *	fix.  Abort deterministically with a
			 *	diagnostic and a stack trace so the
			 *	operator can restart the node from a clean
			 *	state rather than building on a corrupt
			 *	partition.				*/

			putErrmsg("PSM partition lock holder died \
mid-operation; allocator state may be inconsistent. Aborting to avoid silent \
heap corruption.", map->name);
			printStackTrace();

			/*	Unlock WITHOUT pthread_mutex_consistent
			 *	to transition the mutex to ENOTRECOVERABLE
			 *	so every subsequent locker fails the same
			 *	deterministic way (loud, not silent).
			 *	Flush stdio before sm_Abort() because
			 *	abort() does not flush libc buffers and the
			 *	diagnostic would otherwise be lost.	*/

			oK(pthread_mutex_unlock(&map->partLock));
			fflush(NULL);
			sm_Abort();
		}
		else if (rc == ENOTRECOVERABLE)
		{
			/*	A previous caller saw EOWNERDEAD and
			 *	aborted; the partition is permanently
			 *	unusable until re-managed.		*/

			putErrmsg("PSM partition lock is unrecoverable (a \
previous holder died mid-operation). Aborting.", map->name);
			printStackTrace();
			fflush(NULL);
			sm_Abort();
		}
		else if (rc != 0)
		{
			putErrmsg("Can't lock PSM partition mutex.",
					itoa(rc));
			printStackTrace();
			fflush(NULL);
			sm_Abort();
		}
	}
#else
	CHKVOID(map->semaphore != -1);
	oK(sm_SemTake(map->semaphore));
#endif

	/*
	 * Now that we hold the partition across processes, lock the local mutex
	 * again to safely update the ownership fields in this process.
	 */
	pthread_mutex_lock(&_psmLocalMutex);
	map->ownerThread = selfThread;
	map->ownerTask   = selfTask;
	map->depth       = 1;
	pthread_mutex_unlock(&_psmLocalMutex);
}


/******************************************************************************
 *  unlockPartition
 *
 *  If the calling thread owns the partition, decrements the re-entrant depth.
 *  Once depth is zero, releases cross-process ownership (via sm_SemGive()).
 *
 *  Uses the same local mutex (_psmLocalMutex) to protect reads/writes of
 *  ownerTask, ownerThread, and depth in this process.
 ******************************************************************************/
static void unlockPartition(PartitionMap *map)
{
	pthread_mutex_lock(&_psmLocalMutex);

	if (map->status == MANAGED
	&& map->ownerTask == sm_TaskIdSelf()
	&& pthread_equal(map->ownerThread, pthread_self()))
	{
		map->depth--;
		if (map->depth == 0)
		{
			/* No more re-entrant locks held by this thread. */
			map->ownerTask = -1;

#ifdef ION_HAVE_ROBUST_MUTEX
			if (map->partLockCreated)
			{
				/*
				 * Release local mutex before releasing the
				 * cross-process lock to avoid holding two
				 * locks simultaneously.
				 */
				pthread_mutex_unlock(&_psmLocalMutex);
				oK(pthread_mutex_unlock(&map->partLock));
				return;
			}
#else
			if (map->semaphore != -1)
			{
				/*
				 * Release local mutex before calling sm_SemGive()
				 * to avoid holding two locks simultaneously.
				 */
				sm_SemId semId = map->semaphore;
				pthread_mutex_unlock(&_psmLocalMutex);

				sm_SemGive(semId);
				return;
			}
#endif
		}
	}

	/*  If we don't actually own the partition, do nothing. */
	pthread_mutex_unlock(&_psmLocalMutex);
}


static void	discard(PsmPartition partition)
{
	if (partition->freeNeeded)
	{
		TRACK_FREE(partition);
		free(partition);
	}
}

int	psm_manage(char *start, size_t length, char *name, PsmPartition *psmp,
		PsmMgtOutcome *outcome)
{
	PsmPartition	partition;
	PartitionMap	*map;

	CHKERR(outcome);
	*outcome = Refused;
	CHKERR(start != NULL);
	if ((((uaddr) start) % LG_OHD_SIZE) != 0)
	{
		putErrmsg("Starting address not double-word-aligned.",
				utoa((uaddr) start));
		return -1;	/*	Start address misaligned.	*/
	}

	/*	Acquire handle to space management structure.		*/

	partition = *psmp;

	/*	Dynamically allocate space management structure as
	 *	necessary.						*/

	if (partition == NULL)
	{
		partition = (PsmPartition) acquireSystemMemory(sizeof(PsmView));
		CHKERR(partition != NULL);
		partition->freeNeeded = 1;
	}
	else
	{
		partition->freeNeeded = 0;
	}

	partition->space = start;
	partition->trace = NULL;
	map = (PartitionMap *)(void *) (partition->space);
	if (map->status == MANAGED)
	{
		*psmp = partition;
#ifndef ION_HAVE_ROBUST_MUTEX
		/*	On the legacy semaphore path, defensively try to
		 *	clear a possibly-orphaned partition lock; on the
		 *	robust-mutex path, EOWNERDEAD recovery in
		 *	lockPartition() handles this automatically.	*/

		sm_SemUnwedge(map->semaphore, 3);
#endif
		*outcome = Redundant;
		return 0;
	}

	/*	Need to manage and possibly initialize the partition.	*/

	if (length % LG_OHD_SIZE)
	{
		discard(partition);
		putErrmsg("Partition length is not an integral number of \
double words.", utoa(length));
		return -1;
	}

	if (length < sizeof(PartitionMap))
	{
		discard(partition);
		putErrmsg("Partition length is less than partition map size.",
utoa(length));
		return -1;	/*	Partition can't contain map.	*/
	}

	if (name == NULL)
	{
		discard(partition);
		putErrmsg("Partition name is NULL.", NULL);
		return -1;
	}

	if (strlen(name) > 31)
	{
		discard(partition);
		putErrmsg("Partition name length exceeds 31.", name);
		return -1;
	}

	switch (map->status)
	{
	case INITIALIZED:
		if (map->partitionSize != length)
		{
			discard(partition);
			putErrmsg("Asserted partition length doesn't match \
actual length.", itoa(map->partitionSize));
			return -1;	/*	Size mismatch.		*/
		}

		if (strcmp(map->name, name) != 0)
		{
			discard(partition);
			putErrmsg("Asserted partition name doesn't match \
actual name.", map->name);
			return -1;	/*	Name mismatch.		*/
		}

		break;	/*	Proceed with managing the partition.	*/

	default:	/*	Must initialize the partition.		*/
		memset((char *) map, 0, sizeof(PartitionMap));
		map->partitionSize = length;
		istrcpy(map->name, name, sizeof map->name);
		map->startOfSmallPool = sizeof(PartitionMap);
		map->endOfSmallPool = map->startOfSmallPool;
		map->endOfLargePool = length;
		map->startOfLargePool = map->endOfLargePool;
		map->unassignedSpace = map->startOfLargePool -
				map->endOfSmallPool;
		map->traceKey = sm_GetUniqueKey();
	}

#ifdef ION_HAVE_ROBUST_MUTEX
	{
		pthread_mutexattr_t	attr;

		if (pthread_mutexattr_init(&attr) != 0)
		{
			discard(partition);
			putErrmsg("Can't init partition mutex attr.", NULL);
			return -1;
		}

		if (pthread_mutexattr_setpshared(&attr,
				PTHREAD_PROCESS_SHARED) != 0
		|| pthread_mutexattr_setrobust(&attr,
				PTHREAD_MUTEX_ROBUST) != 0
		|| pthread_mutex_init(&map->partLock, &attr) != 0)
		{
			oK(pthread_mutexattr_destroy(&attr));
			discard(partition);
			putErrmsg("Can't create robust mutex for partition.",
					NULL);
			return -1;
		}

		oK(pthread_mutexattr_destroy(&attr));
		map->partLockCreated = 1;
		map->semaphore = SM_SEM_NONE;	/*	Legacy unused.	*/
	}
#else
	map->semaphore = sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO);
	if (map->semaphore < 0)
	{
		discard(partition);
		putErrmsg("Can't create semaphore for partition map.", NULL);
		return -1;
	}
#endif

	map->ownerTask = -1;
	map->depth = 0;
	map->status = MANAGED;
	*psmp = partition;
	*outcome = Okay;
	return 0;
}

char	*psm_name(PsmPartition partition)
{
	PartitionMap	*map;

	CHKNULL(partition);
	map = (PartitionMap *)(void *) (partition->space);
	return map->name;
}

char	*psm_space(PsmPartition partition)
{
	CHKNULL(partition);
	return partition->space;
}

void	psm_unmanage(PsmPartition partition)
{
	PartitionMap	*map;

	CHKVOID(partition);
	map = (PartitionMap *)(void *) (partition->space);
	if (map->status == MANAGED)
	{
	/*	Wait for partition to be no longer in use; unmanage.	*/

#ifdef ION_HAVE_ROBUST_MUTEX
		if (map->partLockCreated)
		{
			/*	We are destroying the partition entirely,
			 *	so allocator consistency no longer matters.
			 *	Drain any straggling lock state -- including
			 *	an EOWNERDEAD orphan -- by marking the lock
			 *	consistent if needed so pthread_mutex_destroy
			 *	will accept it.  An ENOTRECOVERABLE lock
			 *	can be destroyed without first locking.	*/

			int	rc = pthread_mutex_lock(&map->partLock);

			if (rc == EOWNERDEAD)
			{
				oK(pthread_mutex_consistent(&map->partLock));
			}

			if (rc == 0 || rc == EOWNERDEAD)
			{
				oK(pthread_mutex_unlock(&map->partLock));
			}

			microsnooze(50000);
			oK(pthread_mutex_destroy(&map->partLock));
			map->partLockCreated = 0;
		}
#else
		sm_SemTake(map->semaphore);
		sm_SemEnd(map->semaphore);
		microsnooze(50000);
		sm_SemDelete(map->semaphore);
#endif
		map->status = INITIALIZED;
	}

	/*	Destroy space management structure if necessary.	*/

	discard(partition);
}

void	psm_erase(PsmPartition partition)
{
	PartitionMap	*map;

	CHKVOID(partition);
	map = (PartitionMap *)(void *) (partition->space);
	psm_unmanage(partition);	/*	Locks partition.	*/
	map->status = 0;
}

#ifdef ION_HAVE_ROBUST_MUTEX
/*	Test-only helper: take the partition's robust mutex and leave
 *	it held by the caller.  Used by ici/test/psm_robust_recovery_test
 *	to set up an orphaned-lock scenario.  Not declared in psm.h;
 *	the test source declares it manually as extern.		*/

void	_psm_test_take_lock_unreleased(PsmPartition partition)
{
	PartitionMap	*map;

	CHKVOID(partition);
	map = (PartitionMap *)(void *) (partition->space);
	lockPartition(map);
	/*	Intentionally NOT unlocked; caller is expected to die
	 *	with the lock held to orphan it.			*/
}
#endif	/*	ION_HAVE_ROBUST_MUTEX					*/

void    *psp(PsmPartition partition, PsmAddress address)
{
	CHKNULL(partition);
	return (address < sizeof(PartitionMap) ? NULL
			: (partition->space) + address);
}

PsmAddress	psa(PsmPartition partition, void *pointer)
{
	PartitionMap	*map;
	char		*ptr = (char *) pointer;

	CHKZERO(partition);
	map = (PartitionMap *)(void *) (partition->space);
	if (ptr < partition->space + sizeof(PartitionMap)
	|| ptr > partition->space + map->partitionSize)
	{
		return 0;
	}

	return (ptr - partition->space);
}

void	psm_panic(PsmPartition partition)
{
	PartitionMap	*map;

	CHKVOID(partition);
	map = (PartitionMap *)(void *) (partition->space);
	lockPartition(map);
	map->desperate = 1;
	unlockPartition(map);
}

void	psm_relax(PsmPartition partition)
{
	PartitionMap	*map;

	CHKVOID(partition);
	map = (PartitionMap *)(void *) (partition->space);
	lockPartition(map);
	map->desperate = 0;
	unlockPartition(map);
}

int	psm_set_root(PsmPartition partition, PsmAddress root)
{
	PartitionMap	*map;
	int		err = 0;

	CHKERR(partition);
	map = (PartitionMap *)(void *) (partition->space);
	lockPartition(map);
	if (map->directory != 0)
	{
		putErrmsg("Partition already has root value; erase it first.",
				NULL);
		err = -1;
	}
	else
	{
		if (root == 0)
		{
			writeMemo("[i] New partition root value is zero.");
		}

		map->directory = root;
	}

	unlockPartition(map);
	return err;
}

void	psm_erase_root(PsmPartition partition)
{
	PartitionMap	*map;

	CHKVOID(partition);
	map = (PartitionMap *)(void *) (partition->space);
	lockPartition(map);
	map->directory = 0;
	unlockPartition(map);
}

PsmAddress	psm_get_root(PsmPartition partition)
{
	PartitionMap	*map;
	PsmAddress	root;

	CHKZERO(partition);
	map = (PartitionMap *)(void *) (partition->space);
	if (map->status == 0)
	{
		/*	Catch before assert in lockPartition().		*/

		writeMemo("[i] psm_get_root(): map->status == NULL, returns 0");
		return 0; //sky resolves "Assertion failed" issue (psmwatch)
	}

	lockPartition(map);
	root = map->directory;
	unlockPartition(map);
	return root;
}

int	Psm_add_catlg(const char *file, int line, PsmPartition partition)
{
	PartitionMap	*map;
	PsmAddress	catlg;

	if (!partition)
	{
		oK(_iEnd(file, line, "partition"));
		return -1;
	}

	map = (PartitionMap *)(void *) (partition->space);
	lockPartition(map);
	if (map->directory != 0)
	{
		unlockPartition(map);
		_putErrmsg(file, line, "Partition already has root value; \
erase it first.", NULL);
		return -1;
	}

	catlg = Sm_list_create(file, line, partition);
	if (catlg == 0)
	{
		unlockPartition(map);
		_putErrmsg(file, line, "Can't add catalog to partition.", NULL);
		return -1;
	}

	map->directory = catlg;
	unlockPartition(map);
	return 0;
}

int	Psm_catlg(const char *file, int line, PsmPartition partition,
		char *name, PsmAddress address)
{
	PartitionMap	*map;
	PsmAddress	objAddress;
	PsmAddress	elt;
	PsmCatlgEntry	entry;
	PsmAddress	entryObj;

	if (!(partition && name && address))
	{
		oK(_iEnd(file, line, "partition && name && address"));
		return -1;
	}

	map = (PartitionMap *)(void *) (partition->space);
	lockPartition(map);
	if (map->directory == 0)
	{
		unlockPartition(map);
		_putErrmsg(file, line, "Partition has no catalog.", NULL);
		return -1;
	}
	else if (*name == '\0')
	{
		unlockPartition(map);
		_putErrmsg(file, line, "Item name length is zero.", NULL);
		return -1;
	}
	else if (strlen(name) > 32)
	{
		unlockPartition(map);
		_putErrmsg(file, line, "Item name length exceeds 32.", name);
		return -1;
	}

	if (psm_locate(partition, name, &objAddress, &elt) < 0)
	{
		unlockPartition(map);
		_putErrmsg(file, line, "Failed searching for item in catalog.",
				name);
		return -1;
	}

	if (elt)			 /*	Already in catalog.	*/
	{
		unlockPartition(map);
		if (objAddress == address)
		{
			return 0;	/*	Redundant.		*/
		}

		_putErrmsg(file, line, "Item is already in catalog.", name);
		return -1;
	}

	istrcpy(entry.name, name, sizeof entry.name);
	entry.address = address;
	entryObj = Psm_malloc(file, line, partition, sizeof(PsmCatlgEntry));
	if (entryObj == 0)
	{
		unlockPartition(map);
		_putErrmsg(file, line, "Can't create catalog entry.", NULL);
		return -1;
	}

	memcpy((char *) psp(partition, entryObj), (char *) &entry,
			sizeof(PsmCatlgEntry));
	elt = Sm_list_insert_last(file, line, partition, map->directory,
			entryObj);
	if (elt == 0)
	{
		Psm_free(file, line, partition, entryObj);
		unlockPartition(map);
		_putErrmsg(file, line, "Can't append entry to catalog.", NULL);
		return -1;
	}

	unlockPartition(map);
	return 0;
}

int	Psm_uncatlg(const char *file, int line, PsmPartition partition,
		char *name)
{
	PartitionMap	*map;
	PsmAddress	objAddress;
	PsmAddress	elt;
	PsmAddress	entryObj;

	if (!(partition && name))
	{
		oK(_iEnd(file, line, "partition && name"));
		return -1;
	}

	map = (PartitionMap *)(void *) (partition->space);
	lockPartition(map);
	if (psm_locate(partition, name, &objAddress, &elt) < 0)
	{
		unlockPartition(map);
		_putErrmsg(file, line, "Failed searching for item in catalog.",
				name);
		return -1;
	}

	if (elt)
	{
		entryObj = sm_list_data(partition, elt);
		Sm_list_delete(file, line, partition, elt, NULL, NULL);
		Psm_free(file, line, partition, entryObj);
	}

	unlockPartition(map);
	return 0;
}

int	psm_locate(PsmPartition partition, char *name, PsmAddress *address,
		PsmAddress *entryElt)
{
	PartitionMap	*map;
	PsmAddress	elt;
	PsmAddress	entryObj;
	PsmCatlgEntry	entry;

	CHKERR(partition);
	CHKERR(name);
	CHKERR(address);
	CHKERR(entryElt);
	*address = 0;		/*	Default is "not found".		*/
	*entryElt = 0;		/*	Default is "not found".		*/
	map = (PartitionMap *)(void *) (partition->space);
	lockPartition(map);
	if (map->directory == 0)
	{
		unlockPartition(map);
		putErrmsg("Partition has no catalog.", NULL);
		return -1;
	}

	for (elt = sm_list_first(partition, map->directory); elt;
			elt = sm_list_next(partition, elt))
	{
		entryObj = sm_list_data(partition, elt);
		memcpy((char *) &entry, (char *) psp(partition, entryObj),
				sizeof(PsmCatlgEntry));
		if (strcmp(entry.name, name) == 0)
		{
			*address = entry.address;
			break;
		}
	}

	*entryElt = elt;
	unlockPartition(map);
	return 0;
}

static void	removeFromBucket(PartitionMap *map, int bucket,
			struct big_ohd1 *blk, struct big_ohd2 *trailer)
{
	struct big_ohd1	*next;
	struct big_ohd2	*trailerOfNext;
	struct big_ohd1	*prev;

	map->largePoolFree[bucket].freeBytes -= blk->userDataSize;
	map->largePoolFree[bucket].freeBlocks--;
	if (blk->next != 0)
	{
		next = BIG1(blk->next);
		trailerOfNext = BIG2(blk->next + LG_OHD_SIZE
				+ next->userDataSize);
		trailerOfNext->prev = trailer->prev;
	}

	if (trailer->prev == 0)		/*	Removing 1st in bucket.	*/
	{
		map->largePoolFree[bucket].firstFreeBlock = blk->next;
	}
	else
	{
		prev = BIG1(trailer->prev);
		prev->next = blk->next;
	}

	blk->next = BLK_IN_USE;
	trailer->prev = BLK_IN_USE;
}

static int	computeBucket(size_t userDataSize)
{
	size_t	highOrderBits;
	int	bucket;

	highOrderBits = userDataSize >> (LARGE_ORDER1 + 1);
	bucket = 0;
	while (highOrderBits)
	{
		bucket++;
		highOrderBits >>= 1;
	}

	return bucket;
}

static void	insertFreeBlock(PartitionMap *map, struct big_ohd1 *blk,
			struct big_ohd2 *trailer, PsmAddress block)
{
	int		bucket;
	PsmAddress	firstBlock;
	struct big_ohd1	*next;
	struct big_ohd2	*trailer2;

	bucket = computeBucket(blk->userDataSize);
	map->largePoolFree[bucket].freeBytes += blk->userDataSize;
	map->largePoolFree[bucket].freeBlocks++;
	firstBlock = map->largePoolFree[bucket].firstFreeBlock;
	if (firstBlock == 0)
	{
		blk->next = 0;
	}
	else
	{
		blk->next = firstBlock;
		next = BIG1(firstBlock);
		trailer2 = BIG2(firstBlock + LG_OHD_SIZE
				+ next->userDataSize);
		trailer2->prev = block;
	}

	trailer->prev = 0;
	map->largePoolFree[bucket].firstFreeBlock = block;
}

static void	freeLarge(PartitionMap *map, PsmAddress block)
{
	struct big_ohd1	*blk;
	struct big_ohd2	*trailer;
	PsmAddress	nextBlock;
	struct big_ohd1	*next;
	struct big_ohd2	*trailer2;
	int		bucket;
	PsmAddress	prevBlock;
	struct big_ohd1	*prev;

	blk = BIG1(block);
	trailer = BIG2(block + LG_OHD_SIZE + blk->userDataSize);

	/*	Consolidate with physically subsequent block if free.	*/

	nextBlock = block + LARGE_BLOCK_OHD + blk->userDataSize;
	if (nextBlock < map->endOfLargePool)
	{
		next = BIG1(nextBlock);
		if (next->next != BLK_IN_USE)	/*	Block is free.	*/
		{
			trailer2 = BIG2(nextBlock + LG_OHD_SIZE
					+ next->userDataSize);
			bucket = computeBucket(next->userDataSize);
			removeFromBucket(map, bucket, next, trailer2);

			/*	Concatenate with blk.			*/

			blk->userDataSize += (next->userDataSize
					+ LARGE_BLOCK_OHD);
			trailer2->start = block;
			trailer = trailer2;
		}
	}

	/*	Consolidate with physically prior block if free.	*/

	if (block > map->startOfLargePool)
	{
		trailer2 = BIG2(block - LG_OHD_SIZE);
		if (trailer2->prev != BLK_IN_USE)	/*	Free.	*/
		{
			prevBlock = trailer2->start;
			prev = BIG1(prevBlock);
			bucket = computeBucket(prev->userDataSize);
			removeFromBucket(map, bucket, prev, trailer2);

			/*	Concatenate with blk.			*/

			prev->userDataSize += (blk->userDataSize
					+ LARGE_BLOCK_OHD);
			trailer->start = prevBlock;
			blk = prev;
			block = prevBlock;
		}
	}

	/*	Insert the (possibly consolidated) free block.		*/

	insertFreeBlock(map, blk, trailer, block);
}

#ifdef PSM_TRACE
static int	traceInProgress(PsmPartition partition)
{
	PartitionMap	*map = (PartitionMap *)(void *) (partition->space);

	if (partition->trace == NULL)
	{
		/*	Task not yet attached to any enabled trace.	*/

		if (map->traceSize < 1)	/*	Trace is disabled.	*/
		{
			return 0;	/*	Don't trace.		*/
		}

		/*	Attach this task to the enabled trace.		*/

		if (psm_start_trace(partition, map->traceSize, NULL) < 0)
		{
			return 0;	/*	Fail silently.		*/
		}
	}
	else	/*	Still valid?					*/
	{
		if (map->traceSize < 1)	/*	Trace is now disabled.	*/
		{
			sptrace_stop(partition->trace);
			partition->trace = NULL;
			return 0;	/*	Don't trace.		*/
		}

		if (partition->traceCount != map->traceCount)
		{
			/*	New trace episode; reattach.		*/

			sptrace_stop(partition->trace);
			partition->trace = NULL;
			if (psm_start_trace(partition, map->traceSize,
					NULL) < 0)
			{
				return 0;
			}
		}
	}

	return 1;			/*	Trace the event.	*/
}

static void	traceAlloc(const char *file, int line, PsmPartition partition,
			PsmAddress address, int size)
{
	if (traceInProgress(partition))
	{
		sptrace_log_alloc(partition->trace, address, size, file, line);
	}
}

static void	traceFree(const char *file, int line, PsmPartition partition,
		PsmAddress address)
{
	if (traceInProgress(partition))
	{
		sptrace_log_free(partition->trace, address, file, line);
	}
}

static void	traceMemo(const char *file, int line, PsmPartition partition,
			PsmAddress address, char *msg)
{
	if (traceInProgress(partition))
	{
		sptrace_log_memo(partition->trace, address, msg, file, line);
	}
}
#endif

void	Psm_free(const char *file, int line, PsmPartition partition,
		PsmAddress address)
{
	PartitionMap		*map;
	PsmAddress		block;
	struct small_ohd	*smallBlk;
	size_t			userDataWords;	/*	Block size.	*/
	int			i;		/*	Bucket index #.	*/
	struct big_ohd1		*largeBlk;
#ifdef PSM_TRACE
	char			textbuf[100];
#endif

	if (!(partition))
	{
		oK(_iEnd(file, line, "partition"));
		return;
	}

	map = (PartitionMap *)(void *) (partition->space);
	if (map->status != MANAGED)
	{
		writeMemo("[?] psm_free on unmanaged partition (may be normal during shutdown/reversal).");
		return;
	}

	if (address >= map->partitionSize
	|| ((address / WORD_SIZE) * WORD_SIZE) != address)
	{
		writeMemo("Illegal psm_free.");
		oK(_iEnd(file, line, "partition"));
		return;
	}

	lockPartition(map);
	if (address >= map->startOfSmallPool
	&& address < map->endOfSmallPool)
	{
		block = address - SMALL_BLOCK_OHD;
		smallBlk = SMALL(block);
		if ((smallBlk->next) > SMALL_IN_USE)
		{
			userDataWords = (size_t) smallBlk->next - SMALL_IN_USE;
			i = userDataWords - 1;
			map->smallPoolFree[i].freeBlocks++;
			smallBlk->next = map->smallPoolFree[i].firstFreeBlock;
			map->smallPoolFree[i].firstFreeBlock = block;
#ifdef PSM_TRACE
			traceFree(file, line, partition, address);
#endif
		}
		else
		{
#ifdef PSM_TRACE
			istrcpy(textbuf, "psm_free failed: block unallocated",
					sizeof textbuf);
			traceMemo(file, line, partition, address, textbuf);
#endif
		}
	}
	else if (address >= map->startOfLargePool
	&& address < map->endOfLargePool)
	{
		block = address - LG_OHD_SIZE;
		largeBlk = BIG1(block);
		if (largeBlk->next == BLK_IN_USE)
		{
			freeLarge(map, block);
#ifdef PSM_TRACE
			traceFree(file, line, partition, address);
#endif
		}
		else
		{
#ifdef PSM_TRACE
			istrcpy(textbuf, "psm_free failed: block unallocated",
					sizeof textbuf);
			traceMemo(file, line, partition, address, textbuf);
#endif
		}
	}
	else
	{
#ifdef PSM_TRACE
		istrcpy(textbuf, "psm_free failed: block not allocated from \
this partition", sizeof textbuf);
		traceMemo(file, line, partition, address, textbuf);
#endif
	}

	unlockPartition(map);
}

static PsmAddress	mallocLarge(PartitionMap *map, register size_t nbytes)
{
	int		bucket;
	int		primeBucket;
	int		desperationBucket;
	PsmAddress	block = 0;
	struct big_ohd1	*blk;
	size_t		increment;
	struct big_ohd2	*trailer;
	size_t		surplus;
	struct big_ohd2	*newTrailer;
	PsmAddress	newBlock;
	struct big_ohd1	*newBlk;

	/*	Increase nbytes to align it properly: must be an
		integral multiple of LG_OHD_SIZE (which is equal to
		2^LARGE_ORDER1, i.e., 1 << LARGE_ORDER1).		*/

	nbytes += (LG_OHD_SIZE - 1);	/*	to force carry if nec.	*/
	nbytes >>= LARGE_ORDER1;	/*	truncate		*/
	nbytes <<= LARGE_ORDER1;	/*	restore size		*/

	/*	Determine which bucket a block of this size would be
		stored in if free, but then (unless nbytes is the
		minimum block size for that bucket) allocate from the
		NEXT bucket because every free block in that bucket is
		certain to be at least of size nbytes -- so you can
		take the first one instead of searching for a match.	*/

	bucket = computeBucket(nbytes);
	desperationBucket = bucket;	/*	Save in case desperate.	*/
	if (nbytes != (size_t)(1 << (bucket + LARGE_ORDER1)))
	{
		bucket++;
	}

	primeBucket = bucket;		/*	Save in case desperate.	*/
	while (bucket < LARGE_ORDERS
	&& (block = map->largePoolFree[bucket].firstFreeBlock) == 0)
	{
		bucket++;
	}

	if (block)
	{
		blk = BIG1(block);
	}
	else			/*	Need to increase pool size.	*/
	{
		increment = nbytes + LARGE_BLOCK_OHD;
		if (map->unassignedSpace >= increment)
		{
			map->startOfLargePool -= increment;
			map->unassignedSpace -= increment;
			block = map->startOfLargePool;
			blk = BIG1(block);
			blk->userDataSize = nbytes;
			blk->next = BLK_IN_USE;
			trailer = BIG2(block + LG_OHD_SIZE + blk->userDataSize);
			trailer->start = block;
			trailer->prev = BLK_IN_USE;
			return block + LG_OHD_SIZE;
		}

		/*	Can't allocate block from unassigned space.	*/

		if (!(map->desperate))
		{
			putErrmsg(_outOfSpaceMsg(), utoa(nbytes));
			return 0;
		}

		/*	Last-ditch efforts to dig up a block.  First,
			assign all remaining unassigned space to the
			large pool, in hopes that it will consolidate
			with a free block to produce a free block of
			sufficient size.				*/

		if (map->unassignedSpace > 0)
		{
			increment = map->unassignedSpace;
			map->startOfLargePool -= increment;
			map->unassignedSpace -= increment;
			block = map->startOfLargePool;
			blk = BIG1(block);
			blk->userDataSize = increment - LARGE_BLOCK_OHD;
			blk->next = BLK_IN_USE;
			trailer = BIG2(block + LG_OHD_SIZE + blk->userDataSize);
			trailer->start = block;
			trailer->prev = BLK_IN_USE;
			freeLarge(map, block);	/*	Consolidate.	*/

			/*	Now hunt again.				*/

			bucket = primeBucket;
			while (bucket < LARGE_ORDERS
			&& (block = map->largePoolFree[bucket].firstFreeBlock)
					== 0)
			{
				bucket++;
			}
		}

		if (block)
		{
			blk = BIG1(block);
		}
		else
		{
		/*	Finally, hunt sequentially through all free
			blocks in the desperation bucket, many of
			which may be too small but some of which
			might be big enough.				*/

			bucket = desperationBucket;
			block = map->largePoolFree[bucket].firstFreeBlock;
			while (block)
			{
				blk = BIG1(block);
				if (blk->userDataSize >= nbytes)
				{
					break;
				}

				block = blk->next;
			}

			if (block == 0)
			{
				putErrmsg(_outOfSpaceMsg(), utoa(nbytes));
				return 0;
			}
		}
	}

	/*	Free block found.  Must remove from bucket.		*/

	trailer = BIG2(block + LG_OHD_SIZE + blk->userDataSize);
	removeFromBucket(map, bucket, blk, trailer);

	/*	Split off surplus, if any, as separate free block.	*/

	surplus = blk->userDataSize - nbytes;
	if (surplus >= MIN_LARGE_BLOCK)	/*	Must bisect block.	*/
	{
		/*	Shorten original block.				*/

		blk->userDataSize = nbytes;
		newTrailer = BIG2(block + LG_OHD_SIZE + blk->userDataSize);
		newTrailer->start = block;
		newTrailer->prev = BLK_IN_USE;

		/*	Make new block out of surplus.			*/

		newBlock = block + LARGE_BLOCK_OHD + blk->userDataSize;
		newBlk = BIG1(newBlock);
		newBlk->userDataSize = surplus - LARGE_BLOCK_OHD;
		trailer->start = newBlock;
		insertFreeBlock(map, newBlk, trailer, newBlock);
	}

	return block + LG_OHD_SIZE;
}

PsmAddress	Psm_malloc(const char *file, int line, PsmPartition partition,
			register size_t nbytes)
{
	PartitionMap	*map;
#ifdef PSM_TRACE
	char		textbuf[100];
#endif
	PsmAddress	block;

	if (!(partition))
	{
		oK(_iEnd(file, line, "partition"));
		return 0;
	}

	if (nbytes == 0 || nbytes > LARGE_BLK_LIMIT)
	{
#ifdef PSM_TRACE
		isprintf(textbuf, sizeof textbuf, "psm_malloc failed: illegal \
block size %lu", nbytes);
		traceMemo(file, line, partition, 0, textbuf);
#endif
		_putErrmsg(file, line, _badBlockSizeMsg(), utoa(nbytes));
		return 0;
	}

	map = (PartitionMap *)(void *) (partition->space);
	lockPartition(map);
	block = mallocLarge(map, nbytes);
#ifdef PSM_TRACE
	if (block) traceAlloc(file, line, partition, block, nbytes);
#endif
	unlockPartition(map);
	return block;
}

PsmAddress	Psm_zalloc(const char *file, int line, PsmPartition partition,
			register size_t nbytes)
{
	PartitionMap		*map;
#ifdef PSM_TRACE
	char			textbuf[100];
#endif
	PsmAddress		block;
	int			i;
	size_t			increment;
	struct small_ohd	*blk;

	if (!(partition))
	{
		oK(_iEnd(file, line, "partition"));
		return 0;
	}

	if (nbytes == 0 || nbytes > LARGE_BLK_LIMIT)
	{
#ifdef PSM_TRACE
		isprintf(textbuf, sizeof textbuf, "psm_zalloc failed: illegal \
block size %lu", nbytes);
		traceMemo(file, line, partition, 0, textbuf);
#endif
		_putErrmsg(file, line, _badBlockSizeMsg(), utoa(nbytes));
		return 0;
	}

	map = (PartitionMap *)(void *) (partition->space);
	lockPartition(map);
	if (nbytes > SMALL_BLK_LIMIT)
	{
		block = mallocLarge(map, nbytes);
	}
	else
	{
		/*
		 * Increase nbytes to align it properly: must be an integral
		 * multiple of PSM_BLK_ALIGN.
		 */
		nbytes += (PSM_BLK_ALIGN - 1);
		nbytes >>= PSM_BLK_ALIGN_ORDER; /* Truncate. */
		i = nbytes - 1;			/* (gives bucket #) */
		nbytes <<= PSM_BLK_ALIGN_ORDER; /* Restore size. */
		if (map->smallPoolFree[i].freeBlocks == 0)
		{
			increment = nbytes + SMALL_BLOCK_OHD;
			if (map->unassignedSpace < increment)
			{
				block = mallocLarge(map, nbytes);
			}
			else
			{
				block = map->endOfSmallPool;
				blk = SMALL(block);
				map->endOfSmallPool += increment;
				map->unassignedSpace -= increment;
				blk->next = SMALL_IN_USE + i + 1;
				block += SMALL_BLOCK_OHD;
			}
		}
		else	/*	There is at least one free block.	*/
		{
			block = map->smallPoolFree[i].firstFreeBlock;
			map->smallPoolFree[i].freeBlocks--;
			blk = SMALL(block);
			map->smallPoolFree[i].firstFreeBlock = blk->next;
			blk->next = SMALL_IN_USE + i + 1;
			block += SMALL_BLOCK_OHD;
		}
	}

#ifdef PSM_TRACE
	if (block) traceAlloc(file, line, partition, block, nbytes);
#endif
	unlockPartition(map);
	return block;
}

void	psm_audit(PsmPartition partition)
{
	PartitionMap		*map;
	int			i;
	int			blockCount;
	PsmAddress		block;
	struct small_ohd	*blk;

	CHKVOID(partition);
	map = (PartitionMap *)(void *) (partition->space);
	lockPartition(map);
	for (i = 0; i < SMALL_SIZES; i++)
	{
		blockCount = 0;
		block = map->smallPoolFree[i].firstFreeBlock;
		while (block)
		{
			blockCount++;
			blk = SMALL(block);
			block = blk->next;
			if (block == 0)		/*	End of list.	*/
			{
				continue;
			}

			if (block < sizeof(PartitionMap)
			|| block > map->partitionSize)
			{
				writeMemo("Small pool audit failed: next.");
				abort();
			}
		}

		if ((size_t)blockCount != map->smallPoolFree[i].freeBlocks)
		{
			writeMemo("Small pool audit failed: count.");
			abort();
		}
	}

	unlockPartition(map);
}

void	psm_usage(PsmPartition partition, PsmUsageSummary *usage)
{
	PartitionMap	*map;
	int		i;
	size_t		size;
	size_t		freeTotal;

	CHKVOID(partition);
	CHKVOID(usage);
	map = (PartitionMap *)(void *) (partition->space);
	lockPartition(map);
	istrcpy(usage->partitionName, map->name, sizeof usage->partitionName);
	usage->partitionSize = map->partitionSize;
	usage->smallPoolSize = map->endOfSmallPool - map->startOfSmallPool;
	freeTotal = 0;
	size = 0;
	for (i = 0; i < SMALL_SIZES; i++)
	{
		size += PSM_BLK_ALIGN;
		usage->smallPoolFreeBlockCount[i] =
				map->smallPoolFree[i].freeBlocks;
		freeTotal += (map->smallPoolFree[i].freeBlocks * size);
	}

	usage->smallPoolFree = freeTotal;
	usage->smallPoolAllocated = usage->smallPoolSize - freeTotal;
	usage->largePoolSize = map->endOfLargePool - map->startOfLargePool;
	freeTotal = 0;
	for (i = 0; i < LARGE_ORDERS; i++)
	{
		usage->largePoolFreeBlockCount[i] =
				map->largePoolFree[i].freeBlocks;
		freeTotal += map->largePoolFree[i].freeBytes;
	}

	usage->largePoolFree = freeTotal;
	usage->largePoolAllocated = usage->largePoolSize - freeTotal;
	usage->unusedSize = map->unassignedSpace;
	unlockPartition(map);
}

void	psm_report(PsmUsageSummary *usage)
{
	int	i;
	size_t	size;
	size_t	count;
	char	textbuf[100];

	CHKVOID(usage);
	isprintf(textbuf, sizeof textbuf, "-- partition '%s' usage report --",
			usage->partitionName);
	writeMemo(textbuf);
	size = 0;
	writeMemo("small pool free blocks:");
	for (i = 0; i < SMALL_SIZES; i++)
	{
		size += PSM_BLK_ALIGN;
		count = usage->smallPoolFreeBlockCount[i];
		if (count > 0)
		{
			isprintf(textbuf, sizeof textbuf,
					"    " UVAST_FIELDSPEC
					" of size " UVAST_FIELDSPEC,
					(uvast) count, (uvast) size);
			writeMemo(textbuf);
		}
	}

	isprintf(textbuf, sizeof textbuf,
			"       total avbl: %10ld", usage->smallPoolFree);
	writeMemo(textbuf);
	isprintf(textbuf, sizeof textbuf,
			"     total unavbl: %10ld", usage->smallPoolAllocated);
	writeMemo(textbuf);
	isprintf(textbuf, sizeof textbuf,
			"       total size: %10ld", usage->smallPoolSize);
	writeMemo(textbuf);
	size = WORD_SIZE;
	istrcpy(textbuf, "large pool free blocks:", sizeof textbuf);
	writeMemo(textbuf);
	for (i = 0; i < LARGE_ORDERS; i++)
	{
		size *= 2;
		count = usage->largePoolFreeBlockCount[i];
		if (count > 0)
		{
			isprintf(textbuf, sizeof textbuf,
					"    " UVAST_FIELDSPEC
					" of order " UVAST_FIELDSPEC,
					(uvast) count, (uvast) size);
			writeMemo(textbuf);
		}
	}

	isprintf(textbuf, sizeof textbuf,
			"       total avbl: %10ld", usage->largePoolFree);
	writeMemo(textbuf);
	isprintf(textbuf, sizeof textbuf,
			"     total unavbl: %10ld", usage->largePoolAllocated);
	writeMemo(textbuf);
	isprintf(textbuf, sizeof textbuf,
			"       total size: %10ld", usage->largePoolSize);
	writeMemo(textbuf);
	isprintf(textbuf, sizeof textbuf,
			"total partition:   %10ld", usage->partitionSize);
	writeMemo(textbuf);
	isprintf(textbuf, sizeof textbuf,
			"total unused:      %10ld", usage->unusedSize);
	writeMemo(textbuf);
}

int	psm_start_trace(PsmPartition partition, size_t shmSize, char *shm)
{
#ifndef PSM_TRACE
	putErrmsg(_noTraceMsg(), NULL);
	return -1;
#else
	PartitionMap	*map;

	CHKERR(partition);
	map = (PartitionMap *)(void *) (partition->space);
	lockPartition(map);
	if (map->traceSize > 0)	/*	Trace is already enabled.	*/
	{
		if (map->traceSize != shmSize)
		{
			unlockPartition(map);
			putErrmsg("Asserted trace memory size doesn't match \
actual.", itoa(map->traceSize));
			return -1;	/*	Size mismatch.		*/
		}
	}
	else			/*	Trace is not currently enabled.	*/
	{
		map->traceSize = shmSize;	/*	Enable trace.	*/
		map->traceCount++;		/*	New episode.	*/
	}

	partition->trace = (PsmView *) (partition->traceArea);

	/*	(To prevent dynamic allocation of the trace episode's
	 *	space management structure.)				*/

	partition->trace = sptrace_start(map->traceKey, map->traceSize, shm,
			partition->trace, map->name);
	if (partition->trace == NULL)
	{
		unlockPartition(map);
		putErrmsg("Can't start psm trace.", NULL);
		return -1;
	}

	partition->traceCount = map->traceCount;
	sptrace_set_episode_id(partition->trace, map->traceCount);
	unlockPartition(map);
	return 0;
#endif
}

void	psm_print_trace(PsmPartition partition, int verbose)
{
#ifndef PSM_TRACE
	return;
#else
	PartitionMap	*map;
	PsmUsageSummary	summary;

	CHKVOID(partition);
	map = (PartitionMap *)(void *) (partition->space);
	sptrace_report(partition->trace, verbose);
	lockPartition(map);
	psm_usage(partition, &summary);
	unlockPartition(map);
	psm_report(&summary);
#endif
}

void	psm_clear_trace(PsmPartition partition)
{
#ifndef PSM_TRACE
	return;
#else
	PartitionMap	*map;

	CHKVOID(partition);
	map = (PartitionMap *)(void *) (partition->space);
	lockPartition(map);
	sptrace_clear(partition->trace);
	unlockPartition(map);
#endif
}

void	psm_stop_trace(PsmPartition partition)
{
#ifndef PSM_TRACE
	return;
#else
	PartitionMap	*map;

	CHKVOID(partition);
	map = (PartitionMap *)(void *) (partition->space);
	lockPartition(map);
	map->traceSize = 0;			/*	Disable trace.	*/
	snooze(1);   /* allow sufficient time for existing trace to end */
	sptrace_stop(partition->trace);
	partition->trace = NULL;
	unlockPartition(map);
#endif
}
