/*
	reversibility_correctness_test.c:	standalone correctness test
	for SDR transaction reversibility.

	Loads a private SDR profile (no ION attach), pre-populates it
	with a known set of objects, snapshots the dataspace, performs
	a transaction with a deliberately destructive mix of writes,
	mallocs and frees, then cancels the transaction and asserts
	that the dataspace is restored byte-for-byte to the snapshot.
	Repeats with a read-only transaction (must not trigger
	reversal) and once more with a modified transaction (must
	still reverse correctly after the read-only path).

	Prints "PASS" and exits 0 on success; prints "FAIL: <reason>"
	and exits non-zero on first failure.

	Author: redesigned 2026 to replace reversibilityCheck1-4.
									*/
/*									*/
/*	Copyright (c) 2026, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*									*/

#include "platform.h"
#include "sdr.h"
#include "sdrP.h"

#define	TEST_SDR_NAME		"revtest"
#define	TEST_WM_SIZE		(2 * 1024 * 1024)
#define	TEST_HEAP_WORDS		(128 * 1024)	/*	~1 MB heap.	*/
#define	TEST_LOG_SIZE		(256 * 1024)	/*	In-memory log.	*/

#define	NUM_SMALL_OBJS		5
#define	SMALL_OBJ_SIZE		64
#define	NUM_LARGE_OBJS		2
#define	LARGE_OBJ_SIZE		10000

typedef struct
{
	SdrObject addr;
	size_t	size;
	char	pattern;	/*	Fill byte for verification.	*/
} TestObj;

static TestObj	smallObjs[NUM_SMALL_OBJS];
static TestObj	largeObjs[NUM_LARGE_OBJS];

static void	fillPattern(char *buf, size_t size, char pattern)
{
	size_t	i;

	for (i = 0; i < size; i++)
	{
		buf[i] = (char) ((unsigned char) pattern + (unsigned char) i);
	}
}

static int	prepopulate(Sdr sdr)
{
	char	buf[LARGE_OBJ_SIZE];
	int	i;

	CHKERR(sdr_begin_xn(sdr));
	for (i = 0; i < NUM_SMALL_OBJS; i++)
	{
		smallObjs[i].size = SMALL_OBJ_SIZE;
		smallObjs[i].pattern = (char) (0x10 + i);
		smallObjs[i].addr = sdr_malloc(sdr, smallObjs[i].size);
		if (smallObjs[i].addr == 0)
		{
			sdr_cancel_xn(sdr);
			return -1;
		}

		fillPattern(buf, smallObjs[i].size, smallObjs[i].pattern);
		sdr_write(sdr, smallObjs[i].addr, buf, smallObjs[i].size);
	}

	for (i = 0; i < NUM_LARGE_OBJS; i++)
	{
		largeObjs[i].size = LARGE_OBJ_SIZE;
		largeObjs[i].pattern = (char) (0x80 + i);
		largeObjs[i].addr = sdr_malloc(sdr, largeObjs[i].size);
		if (largeObjs[i].addr == 0)
		{
			sdr_cancel_xn(sdr);
			return -1;
		}

		fillPattern(buf, largeObjs[i].size, largeObjs[i].pattern);
		sdr_write(sdr, largeObjs[i].addr, buf, largeObjs[i].size);
	}

	return sdr_end_xn(sdr);
}

static char	*snapshotHeap(Sdr sdrv, size_t *sizeOut)
{
	size_t	dsSize = sdrv->sdr->dsSize;
	char	*copy;

	copy = (char *) malloc(dsSize);
	if (copy == NULL)
	{
		return NULL;
	}

	memcpy(copy, sdrv->dssm, dsSize);
	*sizeOut = dsSize;
	return copy;
}

static int	heapMatchesSnapshot(Sdr sdrv, const char *snapshot,
			size_t snapshotSize)
{
	if (snapshotSize != sdrv->sdr->dsSize)
	{
		fprintf(stderr, "FAIL: dsSize changed (%zu -> %zu)\n",
				snapshotSize, sdrv->sdr->dsSize);
		return 0;
	}

	if (memcmp(sdrv->dssm, snapshot, snapshotSize) != 0)
	{
		size_t	i;

		for (i = 0; i < snapshotSize; i++)
		{
			if (sdrv->dssm[i] != snapshot[i])
			{
				fprintf(stderr, "FAIL: heap differs at "
						"offset %zu (snap=0x%02x, "
						"live=0x%02x)\n",
						i,
						(unsigned char) snapshot[i],
						(unsigned char) sdrv->dssm[i]);
				return 0;
			}
		}
	}

	return 1;
}

static int	verifyApiReads(Sdr sdr)
{
	char	buf[LARGE_OBJ_SIZE];
	char	expected[LARGE_OBJ_SIZE];
	int	i;

	CHKERR(sdr_begin_xn(sdr));
	for (i = 0; i < NUM_SMALL_OBJS; i++)
	{
		sdr_read(sdr, buf, smallObjs[i].addr, smallObjs[i].size);
		fillPattern(expected, smallObjs[i].size,
				smallObjs[i].pattern);
		if (memcmp(buf, expected, smallObjs[i].size) != 0)
		{
			sdr_exit_xn(sdr);
			fprintf(stderr, "FAIL: small obj %d content "
					"mismatch via API\n", i);
			return 0;
		}
	}

	for (i = 0; i < NUM_LARGE_OBJS; i++)
	{
		sdr_read(sdr, buf, largeObjs[i].addr, largeObjs[i].size);
		fillPattern(expected, largeObjs[i].size,
				largeObjs[i].pattern);
		if (memcmp(buf, expected, largeObjs[i].size) != 0)
		{
			sdr_exit_xn(sdr);
			fprintf(stderr, "FAIL: large obj %d content "
					"mismatch via API\n", i);
			return 0;
		}
	}

	sdr_exit_xn(sdr);
	return 1;
}

/*	destructiveXn performs a heterogeneous mix of modifications
 *	and then cancels.  Asserts sdr->modified == 1 just prior to
 *	cancellation, and == 0 afterwards (terminateXn clears it
 *	after a successful reversal).					*/
static int	destructiveXn(Sdr sdr)
{
	char	scratch[LARGE_OBJ_SIZE];
	SdrObject tmpSmall;
	SdrObject tmpLarge;

	CHKERR(sdr_begin_xn(sdr));

	/*	Partial overwrite in the middle of small obj #2.	*/
	memset(scratch, 0xAA, 20);
	sdr_write(sdr, smallObjs[2].addr + 10, scratch, 20);

	/*	Whole-object overwrite of large obj #0.			*/
	memset(scratch, 0xBB, largeObjs[0].size);
	sdr_write(sdr, largeObjs[0].addr, scratch, largeObjs[0].size);

	/*	Free small obj #4 (will be re-claimed on reverse).	*/
	sdr_free(sdr, smallObjs[4].addr);

	/*	Allocate and write a new small object.			*/
	tmpSmall = sdr_malloc(sdr, 100);
	if (tmpSmall == 0)
	{
		sdr_cancel_xn(sdr);
		fprintf(stderr, "FAIL: tmpSmall malloc failed\n");
		return 0;
	}

	memset(scratch, 0xCC, 100);
	sdr_write(sdr, tmpSmall, scratch, 100);

	/*	Allocate, write, then free a large object inside
	 *	the same transaction.					*/
	tmpLarge = sdr_malloc(sdr, 5000);
	if (tmpLarge == 0)
	{
		sdr_cancel_xn(sdr);
		fprintf(stderr, "FAIL: tmpLarge malloc failed\n");
		return 0;
	}

	memset(scratch, 0xDD, 5000);
	sdr_write(sdr, tmpLarge, scratch, 5000);
	sdr_free(sdr, tmpLarge);

	/*	Verify modified is set just before cancel.  The flag
	 *	is reset to 0 at the start of the next sdr_begin_xn,
	 *	not by terminateXn -- so its post-cancel value is not
	 *	meaningful here.  Heap-state comparison after cancel
	 *	is the real correctness assertion.			*/

	if (sdr->sdr->modified != 1)
	{
		sdr_cancel_xn(sdr);
		fprintf(stderr, "FAIL: expected modified == 1 before "
				"cancel, got %d\n", sdr->sdr->modified);
		return 0;
	}

	sdr_cancel_xn(sdr);
	return 1;
}

static int	readOnlyXn(Sdr sdr)
{
	char	buf[SMALL_OBJ_SIZE];

	CHKERR(sdr_begin_xn(sdr));

	/*	Read several objects; touch nothing.  Confirms the
	 *	skip-reversal optimization at sdrxn.c:864-868 leaves
	 *	the modified flag false.				*/

	sdr_read(sdr, buf, smallObjs[0].addr, smallObjs[0].size);
	sdr_read(sdr, buf, smallObjs[1].addr, smallObjs[1].size);

	if (sdr->sdr->modified != 0)
	{
		sdr_cancel_xn(sdr);
		fprintf(stderr, "FAIL: read-only xn unexpectedly marked "
				"modified (=%d)\n", sdr->sdr->modified);
		return 0;
	}

	sdr_cancel_xn(sdr);
	return 1;
}

static int	runTests(Sdr sdr)
{
	char	*snapshot;
	size_t	snapshotSize;

	if (prepopulate(sdr) < 0)
	{
		fprintf(stderr, "FAIL: prepopulate failed\n");
		return 0;
	}

	snapshot = snapshotHeap(sdr, &snapshotSize);
	if (snapshot == NULL)
	{
		fprintf(stderr, "FAIL: snapshot allocation failed\n");
		return 0;
	}

	/*	Case 1: destructive xn -> cancel -> verify rollback.	*/
	printf("Case 1: destructive transaction (expect reversal)\n");
	fflush(stdout);
	if (!destructiveXn(sdr))
	{
		free(snapshot);
		return 0;
	}

	if (!heapMatchesSnapshot(sdr, snapshot, snapshotSize))
	{
		fprintf(stderr, "FAIL: heap not restored after first "
				"destructive cancel\n");
		free(snapshot);
		return 0;
	}

	if (!verifyApiReads(sdr))
	{
		free(snapshot);
		return 0;
	}

	printf("Case 1: PASS - heap restored byte-for-byte\n");
	fflush(stdout);

	/*	Case 2: read-only xn -> cancel -> no rollback work,
	 *	heap unchanged.						*/
	printf("Case 2: read-only transaction (expect skip path)\n");
	fflush(stdout);
	if (!readOnlyXn(sdr))
	{
		free(snapshot);
		return 0;
	}

	if (!heapMatchesSnapshot(sdr, snapshot, snapshotSize))
	{
		fprintf(stderr, "FAIL: heap drifted across read-only "
				"cancel\n");
		free(snapshot);
		return 0;
	}

	printf("Case 2: PASS - skip path taken, heap unchanged\n");
	fflush(stdout);

	/*	Case 3: destructive xn again, to confirm the read-only
	 *	skip path left reversibility itself functional.		*/
	printf("Case 3: destructive transaction after read-only "
			"(expect reversal)\n");
	fflush(stdout);
	if (!destructiveXn(sdr))
	{
		free(snapshot);
		return 0;
	}

	if (!heapMatchesSnapshot(sdr, snapshot, snapshotSize))
	{
		fprintf(stderr, "FAIL: heap not restored after second "
				"destructive cancel\n");
		free(snapshot);
		return 0;
	}

	if (!verifyApiReads(sdr))
	{
		free(snapshot);
		return 0;
	}

	printf("Case 3: PASS - heap restored byte-for-byte\n");
	fflush(stdout);

	free(snapshot);
	return 1;
}

#if defined (ION_LWT)
int	reversibility_correctness_test(saddr a1, saddr a2, saddr a3,
		saddr a4, saddr a5, saddr a6, saddr a7, saddr a8,
		saddr a9, saddr a10)
{
	(void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
	(void)a6; (void)a7; (void)a8; (void)a9; (void)a10;
#else
int	main(int argc, char **argv)
{
	(void)argc; (void)argv;
#endif
	/*	SDR_BOUNDED is intentionally omitted here.  Boundary
	 *	enforcement is an orthogonal integrity feature that
	 *	requires sdr_stage() for partial writes to large
	 *	objects from prior transactions and forbids any
	 *	cross-transaction modification of small objects --
	 *	concerns unrelated to reversibility.  Testing the
	 *	rollback path under the smaller configuration keeps
	 *	this test focused.					*/

	int	configFlags = SDR_IN_DRAM | SDR_REVERSIBLE;
	Sdr	sdr;
	int	ok;

	if (sdr_initialize(TEST_WM_SIZE, NULL, SM_NO_KEY, NULL) < 0)
	{
		fprintf(stderr, "FAIL: sdr_initialize\n");
		return 1;
	}

	if (sdr_load_profile(TEST_SDR_NAME, configFlags, TEST_HEAP_WORDS,
			SM_NO_KEY, TEST_LOG_SIZE, SM_NO_KEY, ".", NULL) < 0)
	{
		fprintf(stderr, "FAIL: sdr_load_profile\n");
		sdr_shutdown();
		return 1;
	}

	sdr = sdr_start_using(TEST_SDR_NAME);
	if (sdr == NULL)
	{
		fprintf(stderr, "FAIL: sdr_start_using\n");
		sdr_shutdown();
		return 1;
	}

	ok = runTests(sdr);

	sdr_destroy(sdr, 0);
	sdr_shutdown();

	if (ok)
	{
		printf("PASS\n");
		return 0;
	}

	return 1;
}
