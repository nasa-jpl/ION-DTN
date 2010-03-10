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

#define	SMALL_BLOCK_OHD	(WORD_SIZE)
#define	SMALL_BLK_LIMIT	(SMALL_SIZES * WORD_SIZE)
#define SMALL_IN_USE	((PsmAddress) 0xffffff00)

#define	BLK_IN_USE	((PsmAddress) 0xffffffff)

/*
 * The overhead on a small block is WORD_SIZE bytes.  When the block is
 * free, these bytes contain a pointer to the next free block, which must be
 * an integral multiple of WORD_SIZE.  When the block is in use, the low-order
 * byte is set to the size of the block's user data (expressed as an integer
 * 1 through SMALL_SIZES: the total block size minus overhead, divided by
 * WORD_SIZE) and the three high-order bytes are all set to 0xff.
 *
 * NOTE: since any address in excess of 0xffffff00 will be interpreted as
 * indicating that the block is in use, the maximum size of the small pool
 * is 0xffffff00; the address of any free block at a higher address would
 * be unrecognizable as such.
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
	u_int		userDataSize;	/*	in bytes, not words	*/
	PsmAddress	next;
};

struct big_ohd2				/*	Trailing overhead.	*/
{
	PsmAddress	start;
	PsmAddress	prev;
};

#define SMALL(x)	((struct small_ohd *)(((char *) map) + x))
#define BIG1(x)		((struct big_ohd1 *)(((char *) map) + x))
#define BIG2(x)		((struct big_ohd2 *)(((char *) map) + x))
#define PTR(x)		(((char *) map) + x)

#define	LG_OHD_SIZE	(1 << LARGE_ORDER1)   /* double word	*/
#define	LARGE_BLOCK_OHD	(2 * LG_OHD_SIZE)
#define	MIN_LARGE_BLOCK	(3 * LG_OHD_SIZE)
#define	LARGE_BLK_LIMIT	(1UL << LARGE_ORDERn)

#define	INITIALIZED	(0x99999999)
#define	MANAGED		(0xbbbbbbbb)

typedef struct			/*	Global view in shared memory.	*/
{
	PsmAddress	directory;
	u_int		status;
	sm_SemId	semaphore;
	int		owner;		/*	Last took the semaphore.*/
	int		depth;		/*	Count of ungiven takes.	*/
	int		desperate;
	u_long		partitionSize;
	char		name[32];
	int		traceKey;	/*	For sptrace.		*/
	long		traceSize;	/*	0 = trace disabled.	*/
	PsmAddress	startOfSmallPool;
	PsmAddress	endOfSmallPool;
	PsmAddress	firstSmallFree[SMALL_SIZES];
	PsmAddress	startOfLargePool;
	PsmAddress	endOfLargePool;
	PsmAddress	firstLargeFree[LARGE_ORDERS];
	u_long		unassignedSpace;
} PartitionMap;

typedef struct
{
	char		name[33];
	PsmAddress	address;
} PsmCatlgEntry;

static const char	*outOfSpaceMsg = "not enough available memory";
static const char	*cannotAllocateMsg = "can't allocate memory";
static const char	*badBlockSizeMsg = "can't allocate, illegal block size";
#ifndef PSM_TRACE
static const char	*noTraceMsg = "tracing unavailable; recompile with \
-DPSM_TRACE";
#endif

/*	*	Non-platform-specific implementation	*	*	*/

static int	lockPartition(PartitionMap *map)
{
	int	self;

	if (map->status != MANAGED)
	{
		errno = EINVAL;
		putSysErrmsg("partition not managed", NULL);
		return 0;
	}

	self = sm_TaskIdSelf();
	if (map->owner != self)
	{
		if (sm_SemTake(map->semaphore))
		{
			putErrmsg("Can't lock partition.", NULL);
			return 0;
		}

		map->owner = self;
	}

	map->depth++;
	return 1;
}

static void	unlockPartition(PartitionMap *map)
{
	if (map->status == MANAGED)
	{
		map->depth--;
		if (map->depth == 0)
		{
			map->owner = -1;
			sm_SemGive(map->semaphore);
		}
	}
}

PsmMgtOutcome	psm_manage(char *start, u_long length, char *name,
			PsmPartition *psmp)
{
	PsmPartition	partition;
	PartitionMap	*map;

	if (start == NULL)
	{
		errno = EINVAL;
		putSysErrmsg("starting address is NULL", NULL);
		return Refused;
	}

	if ((((unsigned long) start) % LG_OHD_SIZE) != 0)
	{
		errno = EINVAL;
		putSysErrmsg("starting address not double-word-aligned", NULL);
		return Refused;	/*	Start address misaligned.	*/
	}

	/*	Acquire handle to space management structure.		*/

	partition = *psmp;

	/*	Dynamically allocate space management structure as
	 *	necessary.						*/

	if (partition == NULL)
	{
		partition = (PsmPartition) acquireSystemMemory(sizeof(PsmView));
		if (partition == NULL)
		{
			putSysErrmsg("can't allocate space for partition \
management structure", NULL);
			return Refused;
		}

		partition->freeNeeded = 1;
	}
	else
	{
		partition->freeNeeded = 0;
	}

	partition->space = start;
	partition->trace = NULL;
	map = (PartitionMap *) (partition->space);
	if (map->status == MANAGED)
	{
		*psmp = partition;
		sm_SemUnwedge(map->semaphore, 3);
		return Redundant;
	}

	/*	Need to manage and possibly initialize the partition.	*/

	if (length % LG_OHD_SIZE)
	{
		if (partition->freeNeeded) free(partition);
		errno = EINVAL;
		putSysErrmsg("partition length is not an integral number of \
double words", itoa(length));
		return Refused;
	}

	if (length < sizeof(PartitionMap))
	{
		if (partition->freeNeeded) free(partition);
		errno = EINVAL;
		putSysErrmsg("partition length is less than partition map size",
itoa(length));
		return Refused;	/*	Partition can't contain map.	*/
	}

	if (name == NULL)
	{
		if (partition->freeNeeded) free(partition);
		errno = EINVAL;
		putSysErrmsg("partition name is NULL", NULL);
		return Refused;
	}

	if (strlen(name) > 31)
	{
		if (partition->freeNeeded) free(partition);
		errno = EINVAL;
		putSysErrmsg("partition name length exceeds 31",
				itoa(strlen(name)));
		return Refused;
	}

	switch (map->status)
	{
	case INITIALIZED:
		if (map->partitionSize != length)
		{
			if (partition->freeNeeded) free(partition);
			errno = EINVAL;
			putSysErrmsg("asserted partition length doesn't match \
actual length", itoa(map->partitionSize));
			return Refused;	/*	Size mismatch.		*/
		}

		if (strcmp(map->name, name) != 0)
		{
			if (partition->freeNeeded) free(partition);
			errno = EINVAL;
			putSysErrmsg("asserted partition name doesn't match \
actual name", map->name);
			return Refused;	/*	Name mismatch.		*/
		}

		break;	/*	Proceed with managing the partition.	*/

	default:	/*	Must initialize the partition.		*/
		map->directory = 0;
		map->desperate = 0;
		map->partitionSize = length;
		strcpy(map->name, name);
		map->startOfSmallPool = sizeof(PartitionMap);
		map->endOfSmallPool = map->startOfSmallPool;
		memset((char *) (map->firstSmallFree), 0,
				sizeof map->firstSmallFree);
		map->endOfLargePool = length;
		map->startOfLargePool = map->endOfLargePool;
		memset((char *) (map->firstLargeFree), 0,
				sizeof map->firstLargeFree);
		map->unassignedSpace = map->startOfLargePool -
				map->endOfSmallPool;
		map->traceKey = sm_GetUniqueKey();
		map->traceSize = 0;
	}

	map->semaphore = sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO);
	if (map->semaphore < 0)
	{
		if (partition->freeNeeded) free(partition);
		putErrmsg("Can't create semaphore for partition map.", NULL);
		return Refused;
	}

	map->owner = -1;
	map->depth = 0;
	map->status = MANAGED;
	*psmp = partition;
	return Okay;
}

char	*psm_name(PsmPartition partition)
{
	PartitionMap	*map;

	REQUIRE(partition);
	map = (PartitionMap *) (partition->space);
	return map->name;
}

char	*psm_space(PsmPartition partition)
{
	REQUIRE(partition);
	return partition->space;
}

void	psm_unmanage(PsmPartition partition)
{
	PartitionMap	*map;

	REQUIRE(partition);
	map = (PartitionMap *) (partition->space);
	if (map->status == MANAGED)
	{
	/*	Wait for partition to be no longer in use; unmanage.	*/

		if (sm_SemTake(map->semaphore) == 0)
		{
			sm_SemDelete(map->semaphore);
			map->status = INITIALIZED;
		}
	}

	/*	Destroy space management structure if necessary.	*/

	if (partition->freeNeeded)
	{
		free(partition);
	}
}

void	psm_erase(PsmPartition partition)
{
	PartitionMap	*map;

	REQUIRE(partition);
	map = (PartitionMap *) (partition->space);
	psm_unmanage(partition);	/*	Locks partition.	*/
	map->status = 0;
}

void    *psp(PsmPartition partition, PsmAddress address)
{
	REQUIRE(partition);
	return (address < sizeof(PartitionMap) ? NULL
			: (partition->space) + address);
}

PsmAddress	psa(PsmPartition partition, void *pointer)
{
	REQUIRE(partition);
	return (((char *) pointer) - (partition->space));
}

void	psm_panic(PsmPartition partition)
{
	PartitionMap	*map;

	REQUIRE(partition);
	map = (PartitionMap *) (partition->space);
	if (lockPartition(map))
	{
		map->desperate = 1;
		unlockPartition(map);
	}
}

void	psm_relax(PsmPartition partition)
{
	PartitionMap	*map;

	REQUIRE(partition);
	map = (PartitionMap *) (partition->space);
	if (lockPartition(map))
	{
		map->desperate = 0;
		unlockPartition(map);
	}
}

int	psm_set_root(PsmPartition partition, PsmAddress root)
{
	PartitionMap	*map;
	int		result = 1;	/*	Success.		*/

	REQUIRE(partition);
	map = (PartitionMap *) (partition->space);
	if (lockPartition(map) == 0)
	{
		putErrmsg("Can't set partition root.", NULL);
		return -1;
	}

	if (root == 0)
	{
		errno = EINVAL;
		putSysErrmsg("new partition root value is zero", NULL);
		result = 0;
	}
	else if (map->directory != 0)
	{
		errno = EINVAL;
		putSysErrmsg("partition already has root value; erase it first",
				NULL);
		result = 0;
	}
	else
	{
		map->directory = root;
	}

	unlockPartition(map);
	return result;
}

void	psm_erase_root(PsmPartition partition)
{
	PartitionMap	*map;

	REQUIRE(partition);
	map = (PartitionMap *) (partition->space);
	if (lockPartition(map))
	{
		map->directory = 0;
		unlockPartition(map);
	}
}

PsmAddress	psm_get_root(PsmPartition partition)
{
	PartitionMap	*map;
	PsmAddress	root;

	REQUIRE(partition);
	map = (PartitionMap *) (partition->space);
	if (lockPartition(map) == 0)
	{
		putErrmsg("Can't get partition root.", NULL);
		return 0;
	}

	root = map->directory;
	unlockPartition(map);
	return root;
}

int	Psm_add_catlg(char *file, int line, PsmPartition partition)
{
	PartitionMap	*map;
	PsmAddress	catlg;

	REQUIRE(partition);
	map = (PartitionMap *) (partition->space);
	if (lockPartition(map) == 0)
	{
		putErrmsg("Can't add catalog to partition.", NULL);
		return -1;
	}

	if (map->directory != 0)
	{
		unlockPartition(map);
		errno = EINVAL;
		putSysErrmsg("partition already has root value; erase it first",
				NULL);
		return 0;
	}

	catlg = Sm_list_create(file, line, partition);
	if (catlg == 0)
	{
		unlockPartition(map);
		putErrmsg("Can't add catalog to partition.", NULL);
		return -1;
	}

	map->directory = catlg;
	unlockPartition(map);
	return 1;
}

int	Psm_catlg(char *file, int line, PsmPartition partition, char *name,
		PsmAddress address)
{
	PartitionMap	*map;
	PsmCatlgEntry	entry;
	PsmAddress	entryObj;
	PsmAddress	elt;

	REQUIRE(partition);
	REQUIRE(name);
	REQUIRE(address);
	map = (PartitionMap *) (partition->space);
	if (lockPartition(map) == 0)
	{
		putErrmsg("Can't catalog item.", NULL);
		return -1;
	}

	if (map->directory == 0)
	{
		unlockPartition(map);
		errno = EINVAL;
		putSysErrmsg("partition has no catalog", NULL);
		return 0;
	}
	else if (*name == '\0')
	{
		unlockPartition(map);
		errno = EINVAL;
		putSysErrmsg("item name length is zero", NULL);
		return 0;
	}
	else if (strlen(name) > 32)
	{
		unlockPartition(map);
		errno = EINVAL;
		putSysErrmsg("item name length exceeds 32", itoa(strlen(name)));
		return 0;
	}

	entryObj = psm_locate(partition, name);
	if (entryObj)
	{
		unlockPartition(map);	/*	Already in catalog.	*/
		errno = EINVAL;
		putSysErrmsg("item is already in catalog", name);
		return 0;
	}

	strcpy(entry.name, name);
	entry.address = address;
	entryObj = Psm_malloc(file, line, partition, sizeof(PsmCatlgEntry));
	if (entryObj == 0)
	{
		unlockPartition(map);
		putErrmsg("Can't create catalog entry.", NULL);
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
		putErrmsg("Can't append entry to catalog.", NULL);
		return -1;
	}

	unlockPartition(map);
	return 1;
}

void	Psm_uncatlg(char *file, int line, PsmPartition partition, char *name)
{
	PartitionMap	*map;
	PsmAddress	elt;
	PsmAddress	entryObj;
	PsmCatlgEntry	entry;

	REQUIRE(partition);
	REQUIRE(name);
	map = (PartitionMap *) (partition->space);
	if (lockPartition(map) == 0)
	{
		return;
	}

	if (map->directory == 0)
	{
		unlockPartition(map);
		return;
	}

	for (elt = sm_list_first(partition, map->directory); elt;
			elt = sm_list_next(partition, elt))
	{
		entryObj = sm_list_data(partition, elt);
		memcpy((char *) &entry, (char *) psp(partition, entryObj),
				sizeof(PsmCatlgEntry));
		if (strcmp(entry.name, name) == 0)
		{
			break;
		}
	}

	if (elt == 0)		/*	Not found in catalog.		*/
	{
		unlockPartition(map);
		return;
	}

	Sm_list_delete(file, line, partition, elt, (SmListDeleteFn) NULL, NULL);
	Psm_free(file, line, partition, entryObj);
	unlockPartition(map);
}

PsmAddress	psm_locate(PsmPartition partition, char *name)
{
	PartitionMap	*map;
	PsmAddress	elt;
	PsmAddress	entryObj;
	PsmCatlgEntry	entry;

	REQUIRE(partition);
	REQUIRE(name);
	map = (PartitionMap *) (partition->space);
	if (lockPartition(map) == 0)
	{
		putErrmsg("Can't search for item.", NULL);
		return 0;
	}

	if (map->directory == 0)
	{
		unlockPartition(map);
		errno = EINVAL;
		putSysErrmsg("partition has no catalog", NULL);
		return 0;
	}

	for (elt = sm_list_first(partition, map->directory); elt;
			elt = sm_list_next(partition, elt))
	{
		entryObj = sm_list_data(partition, elt);
		memcpy((char *) &entry, (char *) psp(partition, entryObj),
				sizeof(PsmCatlgEntry));
		if (strcmp(entry.name, name) == 0)
		{
			break;
		}
	}

	if (elt == 0)		/*	Not found in catalog.		*/
	{
		unlockPartition(map);
		return 0;
	}

	unlockPartition(map);
	return entry.address;
}

static void	removeFromBucket(PartitionMap *map, int bucket,
			struct big_ohd1 *blk, struct big_ohd2 *trailer)
{
	struct big_ohd1	*next;
	struct big_ohd2	*trailerOfNext;
	struct big_ohd1	*prev;

	if (blk->next != 0)
	{
		next = BIG1(blk->next);
		trailerOfNext = BIG2(blk->next + LG_OHD_SIZE
				+ next->userDataSize);
		trailerOfNext->prev = trailer->prev;
	}

	if (trailer->prev == 0)		/*	Removing 1st in bucket.	*/
	{
		map->firstLargeFree[bucket] = blk->next;
	}
	else
	{
		prev = BIG1(trailer->prev);
		prev->next = blk->next;
	}

	blk->next = BLK_IN_USE;
	trailer->prev = BLK_IN_USE;
}

static int	computeBucket(u_int userDataSize)
{
	u_int	highOrderBits;
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
	firstBlock = map->firstLargeFree[bucket];
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
	map->firstLargeFree[bucket] = block;
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
	PartitionMap	*map = (PartitionMap *) (partition->space);

	if (partition->trace == NULL)
	{
		if (map->traceSize < 1)	/*	Trace is disabled.	*/
		{
			return 0;	/*	Don't trace.		*/
		}

		if (psm_start_trace(partition, map->traceSize, NULL) < 1)
		{
			return 0;	/*	Fail silently.		*/
		}
	}
	else	/*	Still valid?					*/
	{
		if (map->traceSize < 1)	/*	Trace is now disabled.	*/
		{
			partition->trace = NULL;
			return 0;	/*	Don't trace.		*/
		}
	}

	return 1;			/*	Trace the event.	*/
}

static void	traceAlloc(char *file, int line, PsmPartition partition,
			PsmAddress address, int size)
{
	if (traceInProgress(partition))
	{
		sptrace_log_alloc(partition->trace, address, size, file, line);
	}
}

static void	traceFree(char *file, int line, PsmPartition partition,
		PsmAddress address)
{
	if (traceInProgress(partition))
	{
		sptrace_log_free(partition->trace, address, file, line);
	}
}

static void	traceMemo(char *file, int line, PsmPartition partition,
			PsmAddress address, char *msg)
{
	if (traceInProgress(partition))
	{
		sptrace_log_memo(partition->trace, address, msg, file, line);
	}
}
#endif

void	Psm_free(char *file, int line, PsmPartition partition,
		PsmAddress address)
{
	PartitionMap		*map;
	PsmAddress		block;
	struct small_ohd	*smallBlk;
	int			userDataWords;
	int			i;
	struct big_ohd1		*largeBlk;
#ifdef PSM_TRACE
	char			textbuf[100];
#endif

	REQUIRE(partition);
	map = (PartitionMap *) (partition->space);
	if (lockPartition(map) == 0)
	{
		return;
	}

	if (address >= map->startOfSmallPool
	&& address < map->endOfSmallPool)
	{
		block = address - SMALL_BLOCK_OHD;
		smallBlk = SMALL(block);
		if ((smallBlk->next) > SMALL_IN_USE)
		{
			userDataWords = (int) smallBlk->next - SMALL_IN_USE;
			i = userDataWords - 1;
			smallBlk->next = map->firstSmallFree[i];
			map->firstSmallFree[i] = block;
#ifdef PSM_TRACE
			traceFree(file, line, partition, address);
#endif
		}
		else
		{
#ifdef PSM_TRACE
			sprintf(textbuf, "psm_free failed: block unallocated");
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
			sprintf(textbuf, "psm_free failed: block unallocated");
			traceMemo(file, line, partition, address, textbuf);
#endif
		}
	}
	else
	{
#ifdef PSM_TRACE
		sprintf(textbuf, "psm_free failed: block not allocated from \
this partition");
		traceMemo(file, line, partition, address, textbuf);
#endif
	}

	unlockPartition(map);
}

static PsmAddress	mallocLarge(PartitionMap *map, register u_int nbytes)
{
	int		bucket;
	int		primeBucket;
	int		desperationBucket;
	PsmAddress	block = 0;
	struct big_ohd1	*blk;
	u_int		increment;
	struct big_ohd2	*trailer;
	u_int		surplus;
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
	if (nbytes != (1 << (bucket + LARGE_ORDER1)))
	{
		bucket++;
	}

	primeBucket = bucket;		/*	Save in case desperate.	*/
	while (bucket < LARGE_ORDERS
	&& (block = map->firstLargeFree[bucket]) == 0)
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
			errno = ENOMEM;
			putSysErrmsg(outOfSpaceMsg, utoa(nbytes));
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
			&& (block = map->firstLargeFree[bucket]) == 0)
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
			block = map->firstLargeFree[bucket];
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
				errno = ENOMEM;
				putSysErrmsg(outOfSpaceMsg, utoa(nbytes));
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

PsmAddress	Psm_malloc(char *file, int line, PsmPartition partition,
			register u_long nbytes)
{
	PartitionMap	*map;
#ifdef PSM_TRACE
	char		textbuf[100];
#endif
	PsmAddress	block;

	REQUIRE(partition);
	if (nbytes == 0 || nbytes > LARGE_BLK_LIMIT)
	{
#ifdef PSM_TRACE
		sprintf(textbuf, "psm_malloc failed: illegal block size %lu",
				nbytes);
		traceMemo(file, line, partition, 0, textbuf);
#endif
		errno = EINVAL;
		putSysErrmsg(badBlockSizeMsg, utoa(nbytes));
		return 0;
	}

	map = (PartitionMap *) (partition->space);
	if (lockPartition(map) == 0)
	{
		putErrmsg(cannotAllocateMsg, NULL);
		return 0;
	}

	block = mallocLarge(map, nbytes);
#ifdef PSM_TRACE
	if (block) traceAlloc(file, line, partition, block, nbytes);
#endif
	unlockPartition(map);
	return block;
}

PsmAddress	Psm_zalloc(char *file, int line, PsmPartition partition,
			register u_long nbytes)
{
	PartitionMap		*map;
#ifdef PSM_TRACE
	char			textbuf[100];
#endif
	PsmAddress		block;
	int			i;
	int			increment;
	struct small_ohd	*blk;

	REQUIRE(partition);
	if (nbytes == 0 || nbytes > LARGE_BLK_LIMIT)
	{
#ifdef PSM_TRACE
		sprintf(textbuf, "psm_zalloc failed: illegal block size %lu",
				nbytes);
		traceMemo(file, line, partition, 0, textbuf);
#endif
		errno = EINVAL;
		putSysErrmsg(badBlockSizeMsg, utoa(nbytes));
		return 0;
	}

	map = (PartitionMap *) (partition->space);
	if (lockPartition(map) == 0)
	{
		putErrmsg(cannotAllocateMsg, NULL);
		return 0;
	}

	if (nbytes > SMALL_BLK_LIMIT)
	{
		block = mallocLarge(map, nbytes);
	}
	else
	{
		/*	Increase nbytes to align it properly: must be
			an integral multiple of SMALL_BLOCK_OHD.	*/

		nbytes += (SMALL_BLOCK_OHD - 1);
		nbytes >>= SPACE_ORDER;	/*	Truncate.		*/
		i = nbytes - 1;		/*	(gives bucket #)	*/
		nbytes <<= SPACE_ORDER;	/*	Restore size.		*/
		block = map->firstSmallFree[i];
		if (block == 0)
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
		else	/*	Found a free block.			*/
		{
			blk = SMALL(block);
			map->firstSmallFree[i] = blk->next;
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

void	psm_usage(PsmPartition partition, PsmUsageSummary *usage)
{
	PartitionMap	*map;
	int		i;
	u_int		size;
	PsmAddress	block;
	PsmAddress	nextBlock;
	u_int		freeTotal;
	int		count;

	REQUIRE(partition);
	REQUIRE(usage);
	map = (PartitionMap *) (partition->space);
	if (lockPartition(map) == 0)
	{
		return;
	}

	strcpy(usage->partitionName, map->name);
	usage->partitionSize = map->partitionSize;
	usage->smallPoolSize = map->endOfSmallPool - map->startOfSmallPool;
	freeTotal = 0;
	size = 0;
	for (i = 0; i < SMALL_SIZES; i++)
	{
		size += WORD_SIZE;
		count = 0;
		for (block = map->firstSmallFree[i]; block; block = nextBlock)
		{
			count++;
			nextBlock = (SMALL(block))->next;
		}

		freeTotal += (count * size);
		usage->smallPoolFreeBlockCount[i] = count;
	}

	usage->smallPoolFree = freeTotal;
	usage->smallPoolAllocated = usage->smallPoolSize - freeTotal;
	usage->largePoolSize = map->endOfLargePool - map->startOfLargePool;
	freeTotal = 0;
	size = WORD_SIZE;
	for (i = 0; i < LARGE_ORDERS; i++)
	{
		size *= 2;
		count = 0;
		for (block = map->firstLargeFree[i]; block; block = nextBlock)
		{
			count++;
			freeTotal += (BIG1(block))->userDataSize;
			nextBlock = (BIG1(block))->next;
		}

		usage->largePoolFreeBlockCount[i] = count;
	}

	usage->largePoolFree = freeTotal;
	usage->largePoolAllocated = usage->largePoolSize - freeTotal;
	usage->unusedSize = usage->partitionSize -
			(sizeof(PartitionMap) +
			 usage->smallPoolSize +
			 usage->largePoolSize);
	unlockPartition(map);
}

void	psm_report(PsmUsageSummary *usage)
{
	int	i;
	u_long	size;
	int	count;
	char	textbuf[100];

	REQUIRE(usage);
	sprintf(textbuf, "-- partition '%s' usage report --",
			usage->partitionName);
	writeMemo(textbuf);
	size = 0;
	writeMemo("small pool free blocks:");
	for (i = 0; i < SMALL_SIZES; i++)
	{
		size += WORD_SIZE;
		count = usage->smallPoolFreeBlockCount[i];
		if (count > 0)
		{
			sprintf(textbuf, "    %10d of size %10ld", count, size);
			writeMemo(textbuf);
		}
	}

	sprintf(textbuf, "       total avbl: %10ld", usage->smallPoolFree);
	writeMemo(textbuf);
	sprintf(textbuf, "     total unavbl: %10ld", usage->smallPoolAllocated);
	writeMemo(textbuf);
	sprintf(textbuf, "       total size: %10ld", usage->smallPoolSize);
	writeMemo(textbuf);
	size = WORD_SIZE;
	sprintf(textbuf, "large pool free blocks:");
	writeMemo(textbuf);
	for (i = 0; i < LARGE_ORDERS; i++)
	{
		size *= 2;
		count = usage->largePoolFreeBlockCount[i];
		if (count > 0)
		{
			sprintf(textbuf, "    %10d of order %10ld", count,
					size);
			writeMemo(textbuf);
		}
	}

	sprintf(textbuf, "       total avbl: %10ld", usage->largePoolFree);
	writeMemo(textbuf);
	sprintf(textbuf, "     total unavbl: %10ld", usage->largePoolAllocated);
	writeMemo(textbuf);
	sprintf(textbuf, "       total size: %10ld", usage->largePoolSize);
	writeMemo(textbuf);
	sprintf(textbuf, "total partition:   %10ld", usage->partitionSize);
	writeMemo(textbuf);
	sprintf(textbuf, "total unused:      %10ld", usage->unusedSize);
	writeMemo(textbuf);
}

int	psm_start_trace(PsmPartition partition, long shmSize, char *shm)
{
#ifndef PSM_TRACE
	errno = EINVAL;
	putSysErrmsg(noTraceMsg, NULL);
	return 0;
#else
	PartitionMap	*map;

	REQUIRE(partition);
	map = (PartitionMap *) (partition->space);
	if (lockPartition(map) == 0)
	{
		putErrmsg("Can't start psm trace.", NULL);
		return 0;
	}

	if (map->traceSize > 0)	/*	Trace is already enabled.	*/
	{
		if (map->traceSize != shmSize)
        	{
			unlockPartition(map);
			errno = EINVAL;
			putSysErrmsg("asserted trace memory size doesn't match \
actual", itoa(map->traceSize));
			return 0;	/*	Size mismatch.		*/
		}
	}
	else			/*	Trace is not currently enabled.	*/
	{
		map->traceSize = shmSize;	/*	Enable trace.	*/
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

	unlockPartition(map);
	return 1;
#endif
}

void	psm_print_trace(PsmPartition partition, int verbose)
{
#ifndef PSM_TRACE
	return;
#else
	PartitionMap	*map;
	PsmUsageSummary	summary;

	REQUIRE(partition);
	map = (PartitionMap *) (partition->space);
	sptrace_report(partition->trace, verbose);
	if (lockPartition(map) == 0)
	{
		return;
	}

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

	REQUIRE(partition);
	map = (PartitionMap *) (partition->space);
	if (lockPartition(map) == 0)
	{
		return;
	}

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

	REQUIRE(partition);
	map = (PartitionMap *) (partition->space);
	if (lockPartition(map) == 0)
	{
		return;
	}

	sptrace_stop(partition->trace);
	partition->trace = NULL;
	map->traceSize = 0;			/*	Disable trace.	*/
	unlockPartition(map);
#endif
}
