/*
	robust_lock_recovery_test.c:	standalone correctness test for
	robust-mutex recovery of the SDR transaction lock.

	On a build with ION_HAVE_ROBUST_MUTEX, this verifies that when a
	process dies while holding the SDR transaction lock, the next
	process to begin a transaction recovers the lock deterministically
	(via EOWNERDEAD) instead of wedging the node forever.

	The program runs in two modes.  With no arguments it is the
	coordinator; it runs three cases in sequence:

	  Case 1  read-only orphan        -> recovery succeeds; there is
	                                     nothing to reverse.
	  Case 2  modified orphan         -> recovery reverses the dead
	                                     owner's partial transaction
	                                     and the heap is restored.
	  Case 3  modified orphan on a    -> recovery cannot roll back, so
	          non-reversible SDR         the next sdr_begin_xn fails
	                                     loudly rather than wedging.

	Each case is fully self-contained: the coordinator creates a
	private SDR (its own working memory), spawns a victim that
	orphans the lock, performs the recovery, then tears the SDR down
	completely before the next case.  No two SDRs share a working
	memory.

	The victim is a separate exec'd process, not a forked child:
	sm_TaskIdSelf() caches getpid() in a static, so a forked child
	would report the coordinator's task identity and the coordinator
	would mistake the orphaned lock for its own recursive re-entry.
	The victim is invoked as "robust_lock_recovery_test victim <case>".

	Prints "PASS" and exits 0 on success; prints "FAIL: <reason>" and
	exits 1 on first failure.  On a build without ION_HAVE_ROBUST_MUTEX
	the test is not applicable and exits 2 (skip).

	Copyright (c) 2026, California Institute of Technology.
	All rights reserved.  U.S. Government Sponsorship acknowledged.
									*/

#include "platform.h"
#include "sdr.h"
#include "sdrP.h"

#ifndef ION_HAVE_ROBUST_MUTEX

#if defined (ION_LWT)
int	robust_lock_recovery_test(saddr a1, saddr a2, saddr a3, saddr a4,
		saddr a5, saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	(void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
	(void)a6; (void)a7; (void)a8; (void)a9; (void)a10;
#else
int	main(int argc, char **argv)
{
	(void)argc; (void)argv;
#endif
	printf("SKIP: built without ION_HAVE_ROBUST_MUTEX; robust-mutex "
			"recovery is not applicable on this platform.\n");
	return 2;
}

#else	/*	ION_HAVE_ROBUST_MUTEX					*/

#include <sys/wait.h>

#define	TEST_WM_SIZE	(2 * 1024 * 1024)
#define	TEST_HEAP_WORDS	(128 * 1024)	/*	~1 MB heap.		*/
#define	TEST_LOG_SIZE	(256 * 1024)	/*	In-memory log.		*/
#define	TEST_SDR_NAME	"rlrtest"
#define	NUM_OBJS	8
#define	OBJ_SIZE	256

#define	REV_FLAGS	(SDR_IN_DRAM | SDR_REVERSIBLE)
#define	NOREV_FLAGS	(SDR_IN_DRAM)

/*	A victim that orphans the lock as intended never returns; it dies
 *	by SIGKILL.  These exit codes report a victim that failed to set
 *	the orphan up, so the coordinator does not mistake the absence of
 *	an orphan for a recovery.					*/
#define	VICTIM_EXEC_FAILED	11
#define	VICTIM_INIT_FAILED	12
#define	VICTIM_ATTACH_FAILED	13
#define	VICTIM_BEGIN_FAILED	14
#define	VICTIM_MALLOC_FAILED	15
#define	VICTIM_BAD_CASE		16
#define	VICTIM_UNREACHABLE	17

typedef struct
{
	int	flags;		/*	SDR configuration flags.	*/
	size_t	logSize;	/*	0 for a non-reversible SDR.	*/
	int	doWrite;	/*	Victim modifies the heap.	*/
} CaseParams;

static SdrObject testObjs[NUM_OBJS];

static int	caseParams(char *tag, CaseParams *p)
{
	if (strcmp(tag, "ro") == 0)
	{
		p->flags = REV_FLAGS;
		p->logSize = TEST_LOG_SIZE;
		p->doWrite = 0;
		return 0;
	}

	if (strcmp(tag, "mod") == 0)
	{
		p->flags = REV_FLAGS;
		p->logSize = TEST_LOG_SIZE;
		p->doWrite = 1;
		return 0;
	}

	if (strcmp(tag, "norev") == 0)
	{
		p->flags = NOREV_FLAGS;
		p->logSize = 0;
		p->doWrite = 1;
		return 0;
	}

	return -1;
}

/*	*	*	Victim	*	*	*	*	*	*/

static int	runVictim(char *tag)
{
	CaseParams	p;
	Sdr		sdr;

	if (caseParams(tag, &p) < 0)
	{
		_exit(VICTIM_BAD_CASE);
	}

	/*	Attach to the SDR the coordinator already created.	*/

	if (sdr_initialize(TEST_WM_SIZE, NULL, SM_NO_KEY, NULL) < 0)
	{
		_exit(VICTIM_INIT_FAILED);
	}

	if (sdr_load_profile(TEST_SDR_NAME, p.flags, TEST_HEAP_WORDS,
			SM_NO_KEY, p.logSize, SM_NO_KEY, ".", NULL) < 0)
	{
		_exit(VICTIM_ATTACH_FAILED);
	}

	sdr = sdr_start_using(TEST_SDR_NAME);
	if (sdr == NULL)
	{
		_exit(VICTIM_ATTACH_FAILED);
	}

	if (sdr_begin_xn(sdr) == 0)
	{
		_exit(VICTIM_BEGIN_FAILED);
	}

	if (p.doWrite)
	{
		char	  buf[OBJ_SIZE];
		SdrObject obj;

		obj = sdr_malloc(sdr, OBJ_SIZE);
		if (obj == 0)
		{
			_exit(VICTIM_MALLOC_FAILED);
		}

		memset(buf, 0xA5, sizeof buf);
		sdr_write(sdr, obj, buf, sizeof buf);
	}

	/*	Die while still holding the transaction lock.		*/

	kill(getpid(), SIGKILL);
	_exit(VICTIM_UNREACHABLE);
}

/*	*	*	Coordinator	*	*	*	*	*/

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
	return (snapshotSize == sdrv->sdr->dsSize
		&& memcmp(sdrv->dssm, snapshot, snapshotSize) == 0);
}

static int	prepopulate(Sdr sdr)
{
	char	buf[OBJ_SIZE];
	int	i;

	CHKERR(sdr_begin_xn(sdr));
	for (i = 0; i < NUM_OBJS; i++)
	{
		testObjs[i] = sdr_malloc(sdr, OBJ_SIZE);
		if (testObjs[i] == 0)
		{
			sdr_cancel_xn(sdr);
			return -1;
		}

		memset(buf, 0x20 + i, sizeof buf);
		sdr_write(sdr, testObjs[i], buf, sizeof buf);
	}

	return sdr_end_xn(sdr);
}

/*	orphanLock spawns a victim -- a separate exec'd process -- that
 *	opens a transaction on the case's SDR and then kills itself with
 *	SIGKILL while still holding the lock.  Returns 0 if the victim
 *	orphaned the lock as intended, -1 if it failed to set the orphan
 *	up.								*/

static int	orphanLock(char *argv0, char *tag)
{
	pid_t	pid;
	int	status;

	fflush(stdout);
	fflush(stderr);
	pid = fork();
	if (pid < 0)
	{
		fprintf(stderr, "FAIL: fork failed\n");
		return -1;
	}

	if (pid == 0)			/*	Child: become the victim. */
	{
		execlp(argv0, argv0, "victim", tag, (char *) NULL);
		_exit(VICTIM_EXEC_FAILED);
	}

	/*	Coordinator: wait for the victim to die.  Because the
	 *	victim kills itself only after sdr_begin_xn has returned,
	 *	the lock is guaranteed to have been held at the moment of
	 *	death once waitpid reports the victim gone.		*/

	if (waitpid(pid, &status, 0) != pid)
	{
		fprintf(stderr, "FAIL: waitpid failed\n");
		return -1;
	}

	if (!(WIFSIGNALED(status)) || WTERMSIG(status) != SIGKILL)
	{
		fprintf(stderr, "FAIL: victim did not die by SIGKILL while "
				"holding the lock (status 0x%x)\n",
				(unsigned) status);
		return -1;
	}

	return 0;
}

/*	runCase creates a private SDR, has a victim orphan its
 *	transaction lock, then performs and checks the recovery.  When
 *	expectRecover is non-zero the coordinator must be able to begin a
 *	transaction afterward and the heap must equal the pre-orphan
 *	snapshot; when it is zero (a modified orphan on a non-reversible
 *	SDR) the coordinator's sdr_begin_xn must instead fail.  The SDR
 *	is fully torn down before returning.				*/

static int	runCase(char *argv0, char *tag, char *header, int expectRecover)
{
	CaseParams	p;
	Sdr		sdr;
	char		*snapshot;
	size_t		snapshotSize = 0;
	int		began;
	int		result = 0;

	printf("%s\n", header);
	fflush(stdout);

	if (caseParams(tag, &p) < 0)
	{
		fprintf(stderr, "FAIL: %s: unknown case\n", tag);
		return 0;
	}

	if (sdr_initialize(TEST_WM_SIZE, NULL, SM_NO_KEY, NULL) < 0)
	{
		fprintf(stderr, "FAIL: %s: sdr_initialize\n", tag);
		return 0;
	}

	if (sdr_load_profile(TEST_SDR_NAME, p.flags, TEST_HEAP_WORDS,
			SM_NO_KEY, p.logSize, SM_NO_KEY, ".", NULL) < 0)
	{
		fprintf(stderr, "FAIL: %s: sdr_load_profile\n", tag);
		sdr_shutdown();
		return 0;
	}

	sdr = sdr_start_using(TEST_SDR_NAME);
	if (sdr == NULL)
	{
		fprintf(stderr, "FAIL: %s: sdr_start_using\n", tag);
		sdr_shutdown();
		return 0;
	}

	if (prepopulate(sdr) < 0)
	{
		fprintf(stderr, "FAIL: %s: prepopulate\n", tag);
		sdr_destroy(sdr, 1);
		return 0;
	}

	snapshot = snapshotHeap(sdr, &snapshotSize);
	if (snapshot == NULL)
	{
		fprintf(stderr, "FAIL: %s: snapshot allocation\n", tag);
		sdr_destroy(sdr, 1);
		return 0;
	}

	if (orphanLock(argv0, tag) == 0)
	{
		/*	The lock is now orphaned by the dead victim.	*/

		began = sdr_begin_xn(sdr);
		if (expectRecover)
		{
			if (began == 0)
			{
				fprintf(stderr, "FAIL: %s: could not begin a "
						"transaction; lock not "
						"recovered\n", tag);
			}
			else
			{
				sdr_exit_xn(sdr);
				if (heapMatchesSnapshot(sdr, snapshot,
						snapshotSize))
				{
					result = 1;
				}
				else
				{
					fprintf(stderr, "FAIL: %s: heap not "
							"restored after "
							"recovery\n", tag);
				}
			}
		}
		else
		{
			if (began != 0)
			{
				fprintf(stderr, "FAIL: %s: began a transaction "
						"on an unrecoverable SDR\n",
						tag);
				sdr_exit_xn(sdr);
			}
			else
			{
				result = 1;
			}
		}
	}

	free(snapshot);
	sdr_destroy(sdr, 1);

	if (result)
	{
		printf("  PASS\n");
		fflush(stdout);
	}

	return result;
}

int	main(int argc, char **argv)
{
	if (argc >= 3 && strcmp(argv[1], "victim") == 0)
	{
		return runVictim(argv[2]);	/*	Never returns.	*/
	}

	if (!runCase(argv[0], "ro",
			"Case 1: read-only transaction orphaned by SIGKILL", 1)
	|| !runCase(argv[0], "mod",
			"Case 2: modified transaction orphaned by SIGKILL", 1)
	|| !runCase(argv[0], "norev",
			"Case 3: modified orphan on a non-reversible SDR", 0))
	{
		return 1;
	}

	printf("PASS\n");
	return 0;
}

#endif	/*	ION_HAVE_ROBUST_MUTEX					*/
