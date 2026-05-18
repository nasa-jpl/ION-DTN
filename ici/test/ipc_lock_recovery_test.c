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
	sees EOWNERDEAD inside takeIpcLock(), logs a diagnostic with a
	stack trace, and aborts deterministically -- the same policy as
	the PSM partition lock.  Subsequent callers see ENOTRECOVERABLE
	and abort the same way.

	Two cases:

	  Case 1  EOWNERDEAD path        -> a victim takes the IPC lock
	                                    and dies by SIGKILL; the next
	                                    caller aborts on EOWNERDEAD.
	  Case 2  ENOTRECOVERABLE path   -> the next caller aborts via
	                                    ENOTRECOVERABLE.

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
#define	CHILD_UNEXPECTED_OK	13
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
 *	function that needs the global IPC lock.  sm_SemCreate is a
 *	natural choice: every daemon startup and every ionAttach calls
 *	it.  The robust-mutex path should detect the orphaned holder
 *	(EOWNERDEAD on Case 1, ENOTRECOVERABLE on Case 2), print a
 *	diagnostic with a stack trace, and call sm_Abort().		*/

static int	runLockUser(void)
{
	sm_SemId	sem;

	if (sm_ipc_init() < 0)
	{
		_exit(CHILD_INIT_FAILED);
	}

	/*	Should abort inside takeIpcLock().			*/

	sem = sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO);
	(void) sem;

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

static int	expectAbort(char *argv0, const char *header)
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

	if (!expectAbort(argv0,
			"Case 1: EOWNERDEAD -- first caller after IPC-lock "
			"orphan must abort"))
	{
		goto cleanup;
	}

	if (!expectAbort(argv0,
			"Case 2: ENOTRECOVERABLE -- subsequent callers also "
			"abort"))
	{
		goto cleanup;
	}

	ok = 1;

cleanup:
	/*	The IPC subsystem is in an unrecoverable state; force a
	 *	clean teardown so the next test run can start fresh.	*/

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
