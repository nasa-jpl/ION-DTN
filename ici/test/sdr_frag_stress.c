/*
 *	sdr_frag_stress.c:	micro-benchmark for the SDR large-pool
 *				allocator's fragmentation behavior.
 *
 *	Drives sdr_malloc and sdr_free in a configurable random-size
 *	mixed workload, snapshots sdr_usage periodically, and emits
 *	CSV to stdout: one header row, one row per snapshot, and a
 *	final REPORT line.  Knobs are passed via environment vars so
 *	the script driver (sdr_frag_compare.sh) can sweep them.
 *
 *	Defaults match a workload that fragments the heap noticeably
 *	in a few thousand iterations on a small (~40MB on 64-bit)
 *	heap, so it is fast to run interactively.
 */

#include "platform.h"
#include "sdr.h"
#include "sdrmgt.h"

#define DEFAULT_WM_SIZE		(2 * 1024 * 1024)
#define DEFAULT_HEAP_WORDS	(5000000)	/*	~40MB on 64-bit	*/
#define DEFAULT_LOG_SIZE	0		/*	no reversibility */
#define SDR_NAME		"sfrag"

typedef struct
{
	SdrObject	obj;
	size_t		size;
} LiveAlloc;

typedef struct
{
	unsigned long	iters;
	size_t		sizeMin;
	size_t		sizeMax;
	int		freeProbPct;
	int		searchLimit;
	unsigned long	snapshotEvery;
	size_t		heapWords;
	unsigned	seed;
	unsigned long	maxLive;
} Knobs;

static size_t	envSize(const char *name, size_t fallback)
{
	const char	*v = getenv(name);
	return v ? (size_t) strtoull(v, NULL, 10) : fallback;
}

static int	envInt(const char *name, int fallback)
{
	const char	*v = getenv(name);
	return v ? (int) strtol(v, NULL, 10) : fallback;
}

static void	readKnobs(Knobs *k)
{
	k->iters		= envSize("SDR_STRESS_ITERS", 5000);
	k->sizeMin		= envSize("SDR_STRESS_SIZE_MIN", 64);
	k->sizeMax		= envSize("SDR_STRESS_SIZE_MAX", 4096);
	k->freeProbPct		= envInt("SDR_STRESS_FREE_PROB_PCT", 45);
	k->searchLimit		= envInt("SDR_STRESS_SEARCH_LIMIT", 0);
	k->snapshotEvery	= envSize("SDR_STRESS_SNAPSHOT_EVERY", 100);
	k->heapWords		= envSize("SDR_STRESS_HEAP_WORDS",
					DEFAULT_HEAP_WORDS);
	k->seed			= (unsigned) envInt("SDR_STRESS_SEED", 42);
	k->maxLive		= envSize("SDR_STRESS_MAX_LIVE", 100000);
}

static size_t	pickSize(const Knobs *k)
{
	size_t	span = k->sizeMax - k->sizeMin + 1;
	size_t	r = ((size_t) rand()) % span;
	size_t	s = k->sizeMin + r;

	/*	8-byte align to keep large-pool allocator on its
	 *	natural granularity.					*/

	return (s + 7) & ~((size_t) 7);
}

static void	emitHeader(void)
{
	fprintf(stdout,
		"iter,allocs,frees,alloc_fails,in_use,small_free,"
		"large_free,unused,largest_free,frag_pct,large_frag_pct\n");
	fflush(stdout);
}

/*	Overall heap fragmentation: fraction of free space that
 *	can't be served from a single contiguous span, where the
 *	unassigned gap counts as one span.				*/

static int	fragPercent(const SdrUsageSummary *u)
{
	size_t	totalFree = u->smallPoolFree + u->largePoolFree + u->unusedSize;
	size_t	largestAvail = u->largestFreeBlock > u->unusedSize
				? u->largestFreeBlock : u->unusedSize;

	if (totalFree == 0)
	{
		return 0;
	}

	return (int) (100 - ((largestAvail * 100) / totalFree));
}

/*	Large-pool-only fragmentation: ignores the unassigned gap,
 *	so it isolates the allocator's internal fragmentation from
 *	the gap's contribution to overall capacity.  This is the
 *	metric Issues 2/3 ultimately care about.			*/

static int	largeFragPercent(const SdrUsageSummary *u)
{
	if (u->largePoolFree == 0)
	{
		return 0;
	}

	return (int) (100 - ((u->largestFreeBlock * 100) / u->largePoolFree));
}

static void	snapshot(Sdr sdr, unsigned long iter,
			unsigned long allocs, unsigned long frees,
			unsigned long allocFails)
{
	SdrUsageSummary	usage;
	size_t		inUse;

	CHKVOID(sdr_begin_xn(sdr));
	sdr_usage(sdr, &usage);
	sdr_exit_xn(sdr);

	inUse = usage.heapSize
		- (usage.smallPoolFree + usage.largePoolFree + usage.unusedSize);

	fprintf(stdout, "%lu,%lu,%lu,%lu,%zu,%zu,%zu,%zu,%zu,%d,%d\n",
			iter, allocs, frees, allocFails,
			inUse, usage.smallPoolFree, usage.largePoolFree,
			usage.unusedSize, usage.largestFreeBlock,
			fragPercent(&usage), largeFragPercent(&usage));
	fflush(stdout);
}

static int	runStress(Sdr sdr, const Knobs *k)
{
	LiveAlloc	*live;
	unsigned long	liveCount = 0;
	unsigned long	allocs = 0;
	unsigned long	frees = 0;
	unsigned long	allocFails = 0;
	unsigned long	i;

	live = (LiveAlloc *) calloc(k->maxLive, sizeof(LiveAlloc));
	if (live == NULL)
	{
		fprintf(stderr, "FAIL: out of memory for tracking array\n");
		return 1;
	}

	srand(k->seed);

	/*	One-time search-limit override if requested.		*/

	if (k->searchLimit > 0)
	{
		if (sdr_begin_xn(sdr) < 0)
		{
			free(live);
			return -1;
		}
		sdr_set_search_limit(sdr, (unsigned) k->searchLimit);
		sdr_end_xn(sdr);
	}

	emitHeader();

	for (i = 1; i <= k->iters; i++)
	{
		int	doFree = (liveCount > 0)
				&& (((rand() % 100) < k->freeProbPct)
					|| liveCount >= k->maxLive);

		if (doFree)
		{
			unsigned long	idx = ((unsigned long) rand())
					% liveCount;

			if (sdr_begin_xn(sdr) < 0)
			{
				free(live);
				return -1;
			}
			sdr_free(sdr, live[idx].obj);
			sdr_end_xn(sdr);

			/*	Swap-and-pop.				*/

			live[idx] = live[liveCount - 1];
			liveCount--;
			frees++;
		}
		else
		{
			size_t	  sz = pickSize(k);
			SdrObject obj;

			if (sdr_begin_xn(sdr) < 0)
			{
				free(live);
				return -1;
			}
			obj = sdr_malloc(sdr, sz);
			sdr_end_xn(sdr);

			if (obj == 0)
			{
				allocFails++;
				/*	Recover by freeing something
				 *	so the workload can continue.	*/

				if (liveCount > 0)
				{
					unsigned long	idx;

					idx = ((unsigned long) rand())
						% liveCount;
					if (sdr_begin_xn(sdr) < 0)
					{
						free(live);
						return -1;
					}
					sdr_free(sdr, live[idx].obj);
					sdr_end_xn(sdr);
					live[idx] = live[liveCount - 1];
					liveCount--;
					frees++;
				}

				continue;
			}

			live[liveCount].obj = obj;
			live[liveCount].size = sz;
			liveCount++;
			allocs++;
		}

		if ((i % k->snapshotEvery) == 0)
		{
			snapshot(sdr, i, allocs, frees, allocFails);
		}
	}

	/*	Final snapshot + REPORT line.				*/

	{
		SdrUsageSummary	usage;
		int		frag;
		snapshot(sdr, i - 1, allocs, frees, allocFails);

		if (sdr_begin_xn(sdr) < 0)
		{
			free(live);
			return -1;
		}
		sdr_usage(sdr, &usage);
		sdr_exit_xn(sdr);

		frag = fragPercent(&usage);
		fprintf(stdout,
			"REPORT,searchLimit=%d,iters=%lu,allocs=%lu,"
			"frees=%lu,fails=%lu,largest_free=%zu,"
			"large_free=%zu,unused=%zu,frag_pct=%d,"
			"large_frag_pct=%d\n",
			k->searchLimit, k->iters, allocs, frees,
			allocFails, usage.largestFreeBlock,
			usage.largePoolFree, usage.unusedSize, frag,
			largeFragPercent(&usage));
	}

	/*	Free any live objects so destroy is clean.		*/

	while (liveCount > 0)
	{
		liveCount--;
		if (sdr_begin_xn(sdr) < 0)
		{
			free(live);
			return -1;
		}
		sdr_free(sdr, live[liveCount].obj);
		sdr_end_xn(sdr);
	}

	free(live);
	return 0;
}

#if defined (ION_LWT)
int	sdr_frag_stress(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	(void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
	(void)a6; (void)a7; (void)a8; (void)a9; (void)a10;
#else
int	main(int argc, char **argv)
{
	(void)argc; (void)argv;
#endif
	Knobs	k;
	Sdr	sdr;
	int	rc;

	readKnobs(&k);

	if (sdr_initialize(DEFAULT_WM_SIZE, NULL, SM_NO_KEY, NULL) < 0)
	{
		fprintf(stderr, "FAIL: sdr_initialize\n");
		return 1;
	}

	if (sdr_load_profile(SDR_NAME, SDR_IN_DRAM, k.heapWords,
			SM_NO_KEY, DEFAULT_LOG_SIZE, SM_NO_KEY, ".", NULL) < 0)
	{
		fprintf(stderr, "FAIL: sdr_load_profile\n");
		sdr_shutdown();
		return 1;
	}

	sdr = sdr_start_using(SDR_NAME);
	if (sdr == NULL)
	{
		fprintf(stderr, "FAIL: sdr_start_using\n");
		sdr_shutdown();
		return 1;
	}

	rc = runStress(sdr, &k);

	sdr_destroy(sdr, 0);
	sdr_shutdown();
	return rc;
}
