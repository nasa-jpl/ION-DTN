/*
	psm_robust_recovery_test.c:	standalone correctness test for the
	robust-mutex protection of the PSM partition lock.

	On a build with ION_HAVE_ROBUST_MUTEX, this verifies that when a
	process dies while holding the PSM partition lock, the next process
	to attempt a PSM operation **aborts deterministically with
	diagnostics** rather than wedging the node forever.

	Unlike the SDR transaction lock (which has a transaction log and
	can roll back), PSM has no log; the allocator state left by a
	dead writer may be inconsistent in any way.  The deliberate policy
	is to abort loudly so the operator restarts the node from a clean
	state rather than building on a corrupt partition.

	Two cases:

	  Case 1  EOWNERDEAD path        -> a victim takes the lock and
	                                    dies by SIGKILL; the next caller
	                                    aborts on EOWNERDEAD.
	  Case 2  ENOTRECOVERABLE path   -> after the first abort the mutex
	                                    transitions to ENOTRECOVERABLE;
	                                    the next caller aborts again,
	                                    this time on ENOTRECOVERABLE.

	The coordinator never calls a PSM operation that would itself abort;
	it always spawns a child for that.  Children are exec'd (not just
	forked) so they have a fresh sm_TaskIdSelf() identity.

	Prints "PASS" and exits 0 on success; prints "FAIL: <reason>" and
	exits 1 on first failure.  On a build without ION_HAVE_ROBUST_MUTEX
	the test is not applicable and exits 2 (skip).

	Copyright (c) 2026, California Institute of Technology.
	All rights reserved.  U.S. Government Sponsorship acknowledged.
									*/

#include "platform.h"
#include "psm.h"
#include "platform_sm.h"

#ifndef ION_HAVE_ROBUST_MUTEX

#if defined (ION_LWT)
int	psm_robust_recovery_test(saddr a1, saddr a2, saddr a3, saddr a4,
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
#include <signal.h>

extern void	_psm_test_take_lock_unreleased(PsmPartition partition);

#define	TEST_WM_KEY	(0x70736D72)	/*	"psmr"			*/
#define	TEST_WM_SIZE	(1 * 1024 * 1024)
#define	TEST_PART_NAME	"psmrtest"

/*	Exit codes used by exec'd child processes so the coordinator can
 *	distinguish "child failed to set up" from "child reached the
 *	intended state".  A child that successfully reaches a PSM op on
 *	the orphaned partition never returns; it dies by SIGABRT.	*/

#define	CHILD_ATTACH_FAILED	11
#define	CHILD_MANAGE_FAILED	12
#define	CHILD_UNEXPECTED_OK	13
#define	CHILD_UNREACHABLE	14

static int	attachAndManage(PsmPartition *psmpOut, uaddr *shmIdOut)
{
	char		*base = NULL;	/*	Let kernel choose address. */
	uaddr		shmId = 0;
	PsmPartition	psmp = NULL;
	PsmMgtOutcome	outcome;

	if (sm_ShmAttach(TEST_WM_KEY, TEST_WM_SIZE, &base, &shmId) < 0)
	{
		return CHILD_ATTACH_FAILED;
	}

	if (psm_manage(base, TEST_WM_SIZE, TEST_PART_NAME, &psmp, &outcome)
			< 0 || outcome == Refused)
	{
		return CHILD_MANAGE_FAILED;
	}

	*psmpOut = psmp;
	*shmIdOut = shmId;
	return 0;
}

/*	*	*	Victim	*	*	*	*	*	*/

/*	Victim attaches to the existing partition, takes the partition
 *	lock without releasing it, and dies by SIGKILL.  The lock is
 *	left orphaned for the coordinator to observe.			*/

static int	runVictim(void)
{
	PsmPartition	psmp = NULL;
	uaddr		shmId;
	int		rc;

	rc = attachAndManage(&psmp, &shmId);
	if (rc != 0)
	{
		_exit(rc);
	}

	_psm_test_take_lock_unreleased(psmp);

	/*	Die while still holding the partition lock.		*/

	kill(getpid(), SIGKILL);
	_exit(CHILD_UNREACHABLE);
}

/*	*	*	Lock user	*	*	*	*	*	*/

/*	Lock user attaches to the orphaned partition and calls a PSM
 *	operation that needs the partition lock.  The expected behavior
 *	is that the operation never returns: the robust-mutex path in
 *	lockPartition() detects the orphan (EOWNERDEAD or
 *	ENOTRECOVERABLE) and calls sm_Abort().  We try psm_zalloc as a
 *	low-cost operation that hits the lock.				*/

static int	runLockUser(void)
{
	PsmPartition	psmp = NULL;
	uaddr		shmId;
	int		rc;
	PsmAddress	addr;

	rc = attachAndManage(&psmp, &shmId);
	if (rc != 0)
	{
		_exit(rc);
	}

	/*	Should abort inside lockPartition.  If it returns, the
	 *	robust-mutex protection did not kick in -- this is a
	 *	failure of the test invariant, reported via exit code.	*/

	addr = psm_zalloc(psmp, 16);
	(void) addr;

	_exit(CHILD_UNEXPECTED_OK);
}

/*	*	*	Coordinator	*	*	*	*	*/

static int	orphanLock(char *argv0)
{
	pid_t	pid;
	int	status;

	fflush(stdout);
	fflush(stderr);
	pid = fork();
	if (pid < 0)
	{
		fprintf(stderr, "FAIL: fork (victim) failed\n");
		return -1;
	}

	if (pid == 0)
	{
		execlp(argv0, argv0, "victim", (char *) NULL);
		_exit(127);
	}

	if (waitpid(pid, &status, 0) != pid)
	{
		fprintf(stderr, "FAIL: waitpid (victim) failed\n");
		return -1;
	}

	if (!WIFSIGNALED(status) || WTERMSIG(status) != SIGKILL)
	{
		fprintf(stderr, "FAIL: victim did not die by SIGKILL "
				"(status 0x%x)\n", (unsigned) status);
		return -1;
	}

	return 0;
}

/*	Spawn a lock-user child and verify it terminates by SIGABRT.
 *	Returns 1 on PASS, 0 on FAIL.					*/

static int	expectAbort(char *argv0, const char *header)
{
	pid_t	pid;
	int	status;

	printf("%s\n", header);
	fflush(stdout);

	fflush(stdout);
	fflush(stderr);
	pid = fork();
	if (pid < 0)
	{
		fprintf(stderr, "FAIL: fork (lockuser) failed\n");
		return 0;
	}

	if (pid == 0)
	{
		execlp(argv0, argv0, "lockuser", (char *) NULL);
		_exit(127);
	}

	if (waitpid(pid, &status, 0) != pid)
	{
		fprintf(stderr, "FAIL: waitpid (lockuser) failed\n");
		return 0;
	}

	if (WIFEXITED(status))
	{
		fprintf(stderr, "FAIL: lockuser exited normally with code "
				"%d; expected SIGABRT\n",
				WEXITSTATUS(status));
		return 0;
	}

	if (!WIFSIGNALED(status))
	{
		fprintf(stderr, "FAIL: lockuser terminated abnormally "
				"(status 0x%x); expected SIGABRT\n",
				(unsigned) status);
		return 0;
	}

	if (WTERMSIG(status) != SIGABRT)
	{
		fprintf(stderr, "FAIL: lockuser died by signal %d; "
				"expected SIGABRT (%d)\n",
				WTERMSIG(status), SIGABRT);
		return 0;
	}

	printf("  PASS (child aborted by SIGABRT as expected)\n");
	fflush(stdout);
	return 1;
}

static int	runCoordinator(char *argv0)
{
	char		*base = NULL;	/*	Let kernel choose address. */
	uaddr		shmId = 0;
	PsmPartition	psmp = NULL;
	PsmMgtOutcome	outcome;
	int		ok = 0;

	/*	Make sure no stale partition is left from a previous
	 *	run.  Ignore failures: it may not exist yet.		*/

	if (sm_ShmAttach(TEST_WM_KEY, TEST_WM_SIZE, &base, &shmId) >= 0)
	{
		sm_ShmDetach(base);
		sm_ShmDestroy(shmId);
		base = NULL;
		shmId = 0;
	}

	if (sm_ShmAttach(TEST_WM_KEY, TEST_WM_SIZE, &base, &shmId) < 0)
	{
		fprintf(stderr, "FAIL: coordinator sm_ShmAttach\n");
		return 0;
	}

	memset(base, 0, TEST_WM_SIZE);	/*	Clean slate.		*/

	if (psm_manage(base, TEST_WM_SIZE, TEST_PART_NAME, &psmp, &outcome)
			< 0 || outcome != Okay)
	{
		fprintf(stderr, "FAIL: coordinator psm_manage (outcome=%d)\n",
				(int) outcome);
		goto cleanup;
	}

	/*	Step 1: orphan the partition lock with a victim.	*/

	if (orphanLock(argv0) < 0)
	{
		goto cleanup;
	}

	/*	Step 2: first PSM op on the orphaned partition must abort
	 *	via the EOWNERDEAD path.				*/

	if (!expectAbort(argv0,
			"Case 1: EOWNERDEAD -- first caller after orphan "
			"must abort"))
	{
		goto cleanup;
	}

	/*	Step 3: the first abort left the mutex in
	 *	ENOTRECOVERABLE; a second caller must also abort, this
	 *	time via the ENOTRECOVERABLE path.			*/

	if (!expectAbort(argv0,
			"Case 2: ENOTRECOVERABLE -- subsequent callers also "
			"abort"))
	{
		goto cleanup;
	}

	ok = 1;

cleanup:
	if (psmp != NULL)
	{
		/*	psm_unmanage handles an EOWNERDEAD lock by
		 *	marking consistent before destroy.  Wrap in a
		 *	fork so a possible abort in there doesn't kill
		 *	the coordinator.				*/

		pid_t	pid = fork();
		if (pid == 0)
		{
			psm_unmanage(psmp);
			_exit(0);
		}
		else if (pid > 0)
		{
			int	st;
			(void) waitpid(pid, &st, 0);
		}
	}

	if (shmId != 0)
	{
		sm_ShmDestroy(shmId);
	}

	return ok;
}

int	main(int argc, char **argv)
{
	if (argc >= 2 && strcmp(argv[1], "victim") == 0)
	{
		return runVictim();		/*	Never returns.	*/
	}

	if (argc >= 2 && strcmp(argv[1], "lockuser") == 0)
	{
		return runLockUser();		/*	Aborts.		*/
	}

	if (!runCoordinator(argv[0]))
	{
		return 1;
	}

	printf("PASS\n");
	return 0;
}

#endif	/*	ION_HAVE_ROBUST_MUTEX					*/
