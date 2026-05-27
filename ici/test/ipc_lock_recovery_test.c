/*
	ipc_lock_recovery_test.c:	standalone correctness test for the
	robust-mutex protection of the global IPC lock.

	The global IPC lock serializes access to the node-wide semaphore
	registry (sm_SemCreate / sm_SemDelete / sm_SemTake-table updates).
	On the legacy POSIX-named-semaphore path, a process killed while
	holding this lock (e.g. SIGKILL during shutdown teardown) orphans
	it; every subsequent sm_SemCreate then wedges, freezing ionAttach
	and every daemon startup on the node.

	On a build with ION_HAVE_ROBUST_MUTEX the lock is a process-shared
	robust pthread_mutex_t.  When the holder dies, the next caller
	sees EOWNERDEAD inside takeIpcLock(), logs a warning, marks the
	mutex consistent via pthread_mutex_consistent(), and proceeds.
	This matches the SDR-lock recovery policy and is what makes the
	deliberate `ionexit k n` restart flow (used by tests like
	relay-restart-mixed-cla) keep working: a daemon that dies while
	holding the IPC lock must not deadlock the post-restart node.

	The recovery is safe because the global sem-table's failure
	surface is bounded -- refCounts, in-use flags, pendingDelete
	bits, and slot-allocation cursor.  The worst-case outcome is an
	orphaned sem-table slot that gets reclaimed at next sm_ipc_stop.
	(Contrast: PSM partition-lock's allocator metadata is unbounded
	in its corruption potential, so PSM aborts rather than recovers.)

	Case 1  EOWNERDEAD recovery    -> a victim takes the IPC lock
	                                  and dies by SIGKILL; the next
	                                  sm_SemCreate must succeed.

	Prints "PASS" and exits 0 on success; prints "FAIL: <reason>" and
	exits 1 on first failure.  On a build without ION_HAVE_ROBUST_MUTEX
	the test is not applicable and exits 2 (skip).

	Copyright (c) 2026, California Institute of Technology.
	All rights reserved.  U.S. Government Sponsorship acknowledged.
									*/

#include "platform.h"
#include "platform_sm.h"

#ifndef ION_HAVE_ROBUST_MUTEX

#if defined (ION_LWT)
int	ipc_lock_recovery_test(saddr a1, saddr a2, saddr a3, saddr a4,
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

extern void	_sm_test_take_ipc_lock_unreleased(void);

#define	CHILD_INIT_FAILED	11
#define	CHILD_SEMCREATE_FAILED	12
#define	CHILD_OK		0
#define	CHILD_UNREACHABLE	14

/*	*	*	Victim	*	*	*	*	*	*/

static int	runVictim(void)
{
	if (sm_ipc_init() < 0)
	{
		_exit(CHILD_INIT_FAILED);
	}

	_sm_test_take_ipc_lock_unreleased();

	/*	Die while still holding the global IPC lock.		*/

	kill(getpid(), SIGKILL);
	_exit(CHILD_UNREACHABLE);
}

/*	*	*	Lock user	*	*	*	*	*	*/

/*	Lock user attaches to the existing IPC subsystem and calls a
 *	function that needs the global IPC lock.  sm_SemCreate is the
 *	natural choice: every daemon startup and every ionAttach calls
 *	it.  Under the recover-and-continue policy, takeIpcLock() must
 *	transparently mark the orphaned lock consistent and proceed;
 *	sm_SemCreate must succeed.					*/

static int	runLockUser(void)
{
	sm_SemId	sem;

	if (sm_ipc_init() < 0)
	{
		_exit(CHILD_INIT_FAILED);
	}

	sem = sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO);
	if (sem < 0)
	{
		_exit(CHILD_SEMCREATE_FAILED);
	}

	sm_SemDelete(sem);
	_exit(CHILD_OK);
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

static int	expectRecover(char *argv0, const char *header)
{
	pid_t	pid;
	int	status;

	printf("%s\n", header);
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

	if (WIFSIGNALED(status))
	{
		fprintf(stderr, "FAIL: lockuser died by signal %d; "
				"expected clean recovery\n",
				WTERMSIG(status));
		return 0;
	}

	if (!WIFEXITED(status))
	{
		fprintf(stderr, "FAIL: lockuser terminated abnormally "
				"(status 0x%x); expected exit 0\n",
				(unsigned) status);
		return 0;
	}

	if (WEXITSTATUS(status) != CHILD_OK)
	{
		fprintf(stderr, "FAIL: lockuser exited with code %d; "
				"expected %d (CHILD_OK)\n",
				WEXITSTATUS(status), CHILD_OK);
		return 0;
	}

	printf("  PASS (orphan recovered transparently; "
			"sm_SemCreate succeeded)\n");
	fflush(stdout);
	return 1;
}

static int	runCoordinator(char *argv0)
{
	int	ok = 0;

	if (sm_ipc_init() < 0)
	{
		fprintf(stderr, "FAIL: coordinator sm_ipc_init\n");
		return 0;
	}

	if (orphanLock(argv0) < 0)
	{
		goto cleanup;
	}

	/*	First caller after the orphan triggers EOWNERDEAD and
	 *	recovers the lock.					*/

	if (!expectRecover(argv0,
			"Case 1: EOWNERDEAD -- first caller after IPC-lock "
			"orphan must recover and succeed"))
	{
		goto cleanup;
	}

	/*	Second caller hits a now-consistent lock and proceeds
	 *	normally.  Confirming this catches any leftover state
	 *	that the recovery path failed to clean up.		*/

	if (!expectRecover(argv0,
			"Case 2: post-recovery -- subsequent callers see a "
			"healthy lock and continue normally"))
	{
		goto cleanup;
	}

	ok = 1;

cleanup:
	sm_ipc_stop();
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
