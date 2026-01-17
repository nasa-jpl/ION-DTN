/*
 * test_deferred_deletion.c
 *
 * Test program to demonstrate semaphore deferred deletion with reference counting.
 *
 * This test creates scenarios where semaphore deletion is attempted while the
 * semaphore is still in use, demonstrating that:
 * 1. Deletion is deferred when refCount > 0
 * 2. Deletion completes when the last sm_SemGive() brings refCount to 0
 * 3. Concurrent access is properly protected
 *
 * Build:
 *   make tests/semaphore-deferred-deletion/dotest
 *
 * Or manually:
 *   gcc -o dotest test_deferred_deletion.c -lici -lpthread
 *
 * Run:
 *   ./dotest
 */

#include <platform.h>
#include <memmgr.h>
#include <sdr.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

/* Color codes for output */
#define COLOR_GREEN "\x1b[32m"
#define COLOR_RED "\x1b[31m"
#define COLOR_YELLOW "\x1b[33m"
#define COLOR_BLUE "\x1b[34m"
#define COLOR_CYAN "\x1b[36m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_RESET "\x1b[0m"

/* Test result tracking */
typedef struct
{
	int passed;
	int failed;
	int total;
} TestResults;

static TestResults results = {0, 0, 0};

/* Test macros */
#define TEST_START(name) \
	printf("\n" COLOR_BLUE "==> Test: %s" COLOR_RESET "\n", name); \
	results.total++;

#define TEST_PASS(name) \
	printf(COLOR_GREEN "    [PASS] %s" COLOR_RESET "\n", name); \
	results.passed++;

#define TEST_FAIL(name, reason) \
	printf(COLOR_RED "    [FAIL] %s: %s" COLOR_RESET "\n", name, reason); \
	results.failed++;

/* Helper for LOG_INFO with just a message */
#define LOG_INFO__1(msg) \
	printf(COLOR_YELLOW "    [INFO] " COLOR_RESET msg "\n")

/* Helper for LOG_INFO with a format string + args */
#define LOG_INFO__N(msg, ...) \
	printf(COLOR_YELLOW "    [INFO] " COLOR_RESET msg "\n", __VA_ARGS__)

/* Argument-counting dispatcher macro */
#define LOG_INFO__GET_MACRO(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, NAME, ...) NAME

/* The main C99-compliant LOG_INFO macro */
#define LOG_INFO(...) \
	LOG_INFO__GET_MACRO(__VA_ARGS__, \
			LOG_INFO__N, LOG_INFO__N, LOG_INFO__N, LOG_INFO__N, \
			LOG_INFO__N, LOG_INFO__N, LOG_INFO__N, LOG_INFO__N, \
			LOG_INFO__N, LOG_INFO__1, 0) \
	(__VA_ARGS__)

/* Thread argument structure */
typedef struct
{
	sm_SemId sem_id;
	int thread_num;
	int sleep_seconds;
} ThreadArg;

/*
 * Thread worker function that takes and holds a semaphore
 */
void *semaphore_holder_thread(void *arg)
{
	ThreadArg *targ = (ThreadArg *)arg;

	LOG_INFO("Thread %d: Taking semaphore %d", targ->thread_num, targ->sem_id);

	if (sm_SemTake(targ->sem_id) < 0)
	{
		LOG_INFO("Thread %d: Failed to take semaphore", targ->thread_num);
		return NULL;
	}

	LOG_INFO("Thread %d: Successfully took semaphore, holding for %d seconds",
			targ->thread_num, targ->sleep_seconds);

	/* Hold the semaphore for specified duration */
	sleep(targ->sleep_seconds);

	LOG_INFO("Thread %d: Releasing semaphore", targ->thread_num);
	sm_SemGive(targ->sem_id);

	LOG_INFO("Thread %d: Released semaphore", targ->thread_num);

	return NULL;
}

/*
 * Test 1: Normal deletion (no active users)
 */
int test_normal_deletion(void)
{
	sm_SemId sem;

	TEST_START("Normal Deletion (no active users)");

	/* Create semaphore */
	LOG_INFO("Creating semaphore...");
	sem = sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO);
	if (sem == SM_SEM_NONE)
	{
		TEST_FAIL("Normal Deletion", "Failed to create semaphore");
		return -1;
	}
	LOG_INFO("Created semaphore %d", sem);

	/* Delete immediately (refCount = 0) */
	LOG_INFO("Deleting semaphore (should succeed immediately)...");
	sm_SemDelete(sem);
	LOG_INFO("Deletion completed");

	/* Try to use deleted semaphore (should fail) */
	LOG_INFO("Attempting to take deleted semaphore (should fail)...");
	if (sm_SemTake(sem) < 0)
	{
		LOG_INFO("Correctly rejected take on deleted semaphore");
		TEST_PASS("Normal deletion with immediate cleanup");
		return 0;
	}
	else
	{
		sm_SemGive(sem);
		TEST_FAIL("Normal Deletion", "Should not be able to take deleted semaphore");
		return -1;
	}
}

/*
 * Test 2: Deferred deletion (single user)
 */
int test_deferred_deletion_single_user(void)
{
	sm_SemId sem;

	TEST_START("Deferred Deletion (single user)");

	/* Create semaphore */
	LOG_INFO("Creating semaphore...");
	sem = sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO);
	if (sem == SM_SEM_NONE)
	{
		TEST_FAIL("Deferred Deletion", "Failed to create semaphore");
		return -1;
	}
	LOG_INFO("Created semaphore %d", sem);

	/* Take semaphore (refCount = 1) */
	LOG_INFO("Taking semaphore (refCount should become 1)...");
	if (sm_SemTake(sem) < 0)
	{
		TEST_FAIL("Deferred Deletion", "Failed to take semaphore");
		return -1;
	}
	LOG_INFO("Successfully took semaphore");

	/* Try to delete while in use (should be deferred) */
	LOG_INFO("Attempting delete while in use (should be deferred)...");
	sm_SemDelete(sem);
	LOG_INFO("Delete returned (deletion should be pending)");

	/* Semaphore should still be usable before Give */
	/* Note: We can't take it again since we already hold it */

	/* Release semaphore (refCount -> 0, should trigger actual deletion) */
	LOG_INFO("Releasing semaphore (should complete deferred deletion)...");
	sm_SemGive(sem);
	LOG_INFO("Released semaphore, deletion should now be complete");

	/* Try to use semaphore after deferred deletion completed */
	sleep(1);  /* Give system time to complete deletion */
	LOG_INFO("Attempting to take semaphore after deferred deletion...");
	if (sm_SemTake(sem) < 0)
	{
		LOG_INFO("Correctly rejected take after deferred deletion");
		TEST_PASS("Deferred deletion completed successfully");
		return 0;
	}
	else
	{
		sm_SemGive(sem);
		TEST_FAIL("Deferred Deletion", "Should not be able to take after deferred deletion");
		return -1;
	}
}

/*
 * Test 3: Deferred deletion with multiple concurrent users
 */
int test_deferred_deletion_multi_thread(void)
{
	sm_SemId sem;
	pthread_t threads[3];
	ThreadArg args[3];
	int i;

	TEST_START("Deferred Deletion (multiple concurrent users)");

	/* Create semaphore */
	LOG_INFO("Creating semaphore...");
	sem = sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO);
	if (sem == SM_SEM_NONE)
	{
		TEST_FAIL("Multi-thread Deletion", "Failed to create semaphore");
		return -1;
	}
	LOG_INFO("Created semaphore %d", sem);

	/* Start 3 threads that will take and hold the semaphore */
	LOG_INFO("Starting 3 threads to use semaphore...");
	for (i = 0; i < 3; i++)
	{
		args[i].sem_id = sem;
		args[i].thread_num = i + 1;
		args[i].sleep_seconds = 2;

		if (pthread_create(&threads[i], NULL, semaphore_holder_thread, &args[i]) != 0)
		{
			TEST_FAIL("Multi-thread Deletion", "Failed to create thread");
			return -1;
		}
	}

	/* Give threads time to take the semaphore */
	sleep(1);

	/* Try to delete while threads are using it (should be deferred) */
	LOG_INFO("Attempting delete while threads are using semaphore...");
	sm_SemDelete(sem);
	LOG_INFO("Delete returned (deletion should be deferred)");

	/* Wait for all threads to complete */
	LOG_INFO("Waiting for all threads to complete...");
	for (i = 0; i < 3; i++)
	{
		pthread_join(threads[i], NULL);
	}
	LOG_INFO("All threads completed");

	/* Give system time to complete deferred deletion */
	sleep(1);

	/* Try to use semaphore after all users released it */
	LOG_INFO("Attempting to take semaphore after deferred deletion...");
	if (sm_SemTake(sem) < 0)
	{
		LOG_INFO("Correctly rejected take after deferred deletion");
		TEST_PASS("Multi-thread deferred deletion");
		return 0;
	}
	else
	{
		sm_SemGive(sem);
		TEST_FAIL("Multi-thread Deletion", "Should not be able to take after deletion");
		return -1;
	}
}

/*
 * Test 4: Race condition protection
 */
int test_race_condition_protection(void)
{
	sm_SemId sem;
	pthread_t thread;
	ThreadArg arg;

	TEST_START("Race Condition Protection");

	/* Create semaphore */
	LOG_INFO("Creating semaphore...");
	sem = sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO);
	if (sem == SM_SEM_NONE)
	{
		TEST_FAIL("Race Protection", "Failed to create semaphore");
		return -1;
	}
	LOG_INFO("Created semaphore %d", sem);

	/* Start thread that will hold semaphore */
	LOG_INFO("Starting thread to hold semaphore...");
	arg.sem_id = sem;
	arg.thread_num = 1;
	arg.sleep_seconds = 3;

	if (pthread_create(&thread, NULL, semaphore_holder_thread, &arg) != 0)
	{
		TEST_FAIL("Race Protection", "Failed to create thread");
		return -1;
	}

	/* Give thread time to take semaphore */
	sleep(1);

	/* Try to delete (should be deferred) */
	LOG_INFO("Main thread: Attempting delete while thread holds semaphore...");
	sm_SemDelete(sem);

	/* Try to take semaphore (should fail - pendingDelete is set) */
	LOG_INFO("Main thread: Attempting to take pending-delete semaphore (should fail)...");
	if (sm_SemTake(sem) < 0)
	{
		LOG_INFO("Correctly rejected take on pending-delete semaphore");
	}
	else
	{
		sm_SemGive(sem);
		TEST_FAIL("Race Protection", "Should not take pending-delete semaphore");
		pthread_join(thread, NULL);
		return -1;
	}

	/* Wait for thread to complete */
	pthread_join(thread, NULL);
	LOG_INFO("Thread completed and deletion should be finalized");

	TEST_PASS("Race condition protection");
	return 0;
}

/*
 * Main test runner
 */
int main(void)
{
	printf("\n");
	printf(COLOR_CYAN "========================================\n");
	printf("  ION Semaphore Deferred Deletion Test\n");
	printf("========================================\n" COLOR_RESET);
	printf("\n");

	LOG_INFO("Initializing IPC system...");
	if (sm_ipc_init() < 0)
	{
		printf(COLOR_RED "ERROR: Failed to initialize IPC system\n" COLOR_RESET);
		return 1;
	}
	LOG_INFO("IPC system initialized");

	/* Run all tests */
	test_normal_deletion();
	test_deferred_deletion_single_user();
	test_deferred_deletion_multi_thread();
	test_race_condition_protection();

	/* Print summary */
	printf("\n");
	printf(COLOR_CYAN "========================================\n");
	printf("  Test Summary\n");
	printf("========================================\n" COLOR_RESET);
	printf("  Total:  %d\n", results.total);
	printf(COLOR_GREEN "  Passed: %d\n" COLOR_RESET, results.passed);
	if (results.failed > 0)
	{
		printf(COLOR_RED "  Failed: %d\n" COLOR_RESET, results.failed);
	}
	else
	{
		printf("  Failed: %d\n", results.failed);
	}
	printf(COLOR_CYAN "========================================\n" COLOR_RESET);

	/* Cleanup */
	LOG_INFO("Cleaning up IPC system...");
	sm_ipc_stop();

	if (results.failed == 0 && results.passed > 0)
	{
		printf(COLOR_GREEN "\n✓ All tests passed!\n" COLOR_RESET);
		return 0;
	}
	else
	{
		printf(COLOR_RED "\n✗ Some tests failed!\n" COLOR_RESET);
		return 1;
	}
}
