/*
 * bp_plan_crash_recovery_test.c
 *
 * Regression test for BP plan crash recovery.
 *
 * This test verifies that bpStartPlan() can successfully restart a BP plan's
 * bpclm daemon after it has crashed or been forcibly terminated. The test
 * ensures that the semaphore reset bug fix (calling resetPlan() before
 * startPlan()) works correctly.
 *
 * Test Sequence:
 * 1. Initialize ION and BP with a TCP plan
 * 2. Start the plan normally (spawns bpclm daemon)
 * 3. Verify bpclm is running
 * 4. Forcibly kill bpclm daemon (simulates crash)
 * 5. Call bpStartPlan() to restart the plan
 * 6. Verify bpclm successfully restarted
 * 7. Clean shutdown
 *
 * Expected Result:
 * - bpclm should successfully restart after crash
 * - No "semaphore ended" errors
 * - Plan remains functional after restart
 */

#include "../ltp_loopback/ion_test_utils.h"
#include <signal.h>
#include <sys/types.h>

TestResults results = {0, 0, 0};

/*
 * Check if a specific PID is still running
 */
static int pid_exists(int pid)
{
	char cmd[128];
	FILE *fp;
	char line[128];

	snprintf(cmd, sizeof(cmd), "ps -p %d -o pid= 2>/dev/null", pid);
	fp = popen(cmd, "r");
	if (fp == NULL)
	{
		return 0;
	}

	int exists = (fgets(line, sizeof(line), fp) != NULL);
	pclose(fp);
	return exists;
}

/*
 * Get the PID of the bpclm daemon for ipn:1.0
 */
static int get_bpclm_pid(void)
{
	FILE *fp;
	char cmd[256];
	char line[128];
	int pid = -1;

	/* Use ps to find bpclm process */
	snprintf(cmd, sizeof(cmd), "ps aux | grep 'bpclm ipn:1.0' | grep -v grep | awk '{print $2}'");
	fp = popen(cmd, "r");
	if (fp == NULL)
	{
		return -1;
	}

	if (fgets(line, sizeof(line), fp) != NULL)
	{
		pid = atoi(line);
	}

	pclose(fp);
	return pid;
}

/*
 * Main test function
 */
int main(int argc, char *argv[])
{
	int rc;
	int bpclm_pid;
	time_t current_time;

	/* Suppress unused parameter warnings */
	(void)argc;
	(void)argv;

	printf("\n");
	printf(COLOR_BLUE "========================================\n" COLOR_RESET);
	printf(COLOR_BLUE "BP Plan Crash Recovery Regression Test\n" COLOR_RESET);
	printf(COLOR_BLUE "========================================\n" COLOR_RESET);

	/*
	 * Test 1: Initialize ION
	 */
	TEST_START("Initialize ION node 1");
	rc = test_ion_initialization_and_attach(1);
	if (rc < 0)
	{
		TEST_FAIL("Initialize ION", "ion_initialize_node or ion_attach failed");
		goto cleanup;
	}
	TEST_PASS("Initialize ION");

	/*
	 * Test 2: Register node in region 1
	 */
	TEST_START("Register node in region 1");
	rc = ion_register_node(1);
	if (rc < 0)
	{
		TEST_FAIL("Register node", "ion_register_node failed");
		goto cleanup;
	}
	TEST_PASS("Register node in region 1");

	/*
	 * Test 4: Start RFX
	 */
	TEST_START("Start RFX");
	rc = test_rfx_start();
	if (rc < 0)
	{
		TEST_FAIL("Start RFX", "rfx_start failed");
		goto cleanup;
	}
	TEST_PASS("Start RFX");

	/*
	 * Test 5: Add contact (needed for routing)
	 */
	TEST_START("Add contact");
	current_time = time(NULL);
	rc = test_add_contact(current_time, current_time + 3600, 1, 1, 100000, 1.0);
	if (rc < 0)
	{
		TEST_FAIL("Add contact", "add_contact failed");
		goto cleanup;
	}
	TEST_PASS("Add contact");

	/*
	 * Test 6: Add range
	 */
	TEST_START("Add range");
	rc = test_add_range(current_time, current_time + 3600, 1, 1, 1);
	if (rc < 0)
	{
		TEST_FAIL("Add range", "add_range failed");
		goto cleanup;
	}
	TEST_PASS("Add range");

	/*
	 * Test 7: Initialize ionsec
	 */
	TEST_START("Initialize ionsec");
	rc = test_ionsec_initialization_and_attach();
	if (rc < 0)
	{
		TEST_FAIL("Initialize ionsec", "ionsec_attach failed");
		goto cleanup;
	}
	TEST_PASS("Initialize ionsec");

	/*
	 * Test 8: Initialize BP
	 */
	TEST_START("Initialize BP");
	rc = test_bp_initialization();
	if (rc < 0)
	{
		TEST_FAIL("Initialize BP", "bp_initialize failed");
		goto cleanup;
	}
	TEST_PASS("Initialize BP");

	/*
	 * Test 9: Attach to BP
	 */
	TEST_START("Attach to BP");
	rc = test_bp_attach();
	if (rc < 0)
	{
		TEST_FAIL("Attach to BP", "bp_attach failed");
		goto cleanup;
	}
	TEST_PASS("Attach to BP");

	/*
	 * Test 10: Initialize IPN
	 */
	TEST_START("Initialize IPN");
	rc = test_ipn_initialization();
	if (rc < 0)
	{
		TEST_FAIL("Initialize IPN", "ipn_init failed");
		goto cleanup;
	}
	TEST_PASS("Initialize IPN");

	/*
	 * Test 11: Add ipn scheme
	 */
	TEST_START("Add ipn scheme");
	rc = test_add_scheme("ipn", "ipnfw", "ipnadminep");
	if (rc < 0)
	{
		TEST_FAIL("Add ipn scheme", "add_scheme failed");
		goto cleanup;
	}
	TEST_PASS("Add ipn scheme");

	/*
	 * Test 12: Add endpoint
	 */
	TEST_START("Add endpoint ipn:1.1");
	rc = test_add_endpoint("ipn:1.1", EnqueueBundle, NULL);
	if (rc < 0)
	{
		TEST_FAIL("Add endpoint", "add_endpoint failed");
		goto cleanup;
	}
	TEST_PASS("Add endpoint ipn:1.1");

	/*
	 * Test 13: Add tcp protocol
	 */
	TEST_START("Add tcp protocol");
	rc = test_add_protocol("tcp", 0);
	if (rc < 0)
	{
		TEST_FAIL("Add tcp protocol", "add_protocol failed");
		goto cleanup;
	}
	TEST_PASS("Add tcp protocol");

	/*
	 * Test 14: Add tcp induct
	 */
	TEST_START("Add tcp induct");
	rc = test_add_induct("tcp", "localhost:4556", "tcpcli localhost:4556");
	if (rc < 0)
	{
		TEST_FAIL("Add tcp induct", "add_induct failed");
		goto cleanup;
	}
	TEST_PASS("Add tcp induct");

	/*
	 * Test 15: Add tcp outduct
	 */
	TEST_START("Add tcp outduct");
	rc = test_add_outduct("tcp", "localhost:4556", "tcpclo localhost:4556", 0);
	if (rc < 0)
	{
		TEST_FAIL("Add tcp outduct", "add_outduct failed");
		goto cleanup;
	}
	TEST_PASS("Add tcp outduct");

	/*
	 * Test 16: Add plan for ipn:1.0
	 */
	TEST_START("Add plan ipn:1.0");
	rc = test_add_plan("ipn:1.0", 100000);
	if (rc < 0)
	{
		TEST_FAIL("Add plan", "add_plan failed");
		goto cleanup;
	}
	TEST_PASS("Add plan ipn:1.0");

	/*
	 * Test 17: Add planduct
	 */
	TEST_START("Add planduct");
	rc = test_add_planduct("ipn:1.0", "tcp", "localhost:4556");
	if (rc < 0)
	{
		TEST_FAIL("Add planduct", "add_planduct failed");
		goto cleanup;
	}
	TEST_PASS("Add planduct");

	/*
	 * Test 18: Start BP (starts all scheme forwarders)
	 */
	TEST_START("Start BP");
	rc = test_bp_start();
	if (rc < 0)
	{
		TEST_FAIL("Start BP", "bp_start failed");
		goto cleanup;
	}
	TEST_PASS("Start BP");

	/* Wait for ipnfw to start */
	if (!waitForSchemeDaemon("ipnfw", 5))
	{
		TEST_FAIL("Start BP", "ipnfw daemon did not start");
		goto cleanup;
	}

	/*
	 * Test 19: Start the plan (spawns bpclm)
	 */
	TEST_START("Start plan ipn:1.0 (initial)");
	rc = bp_start_plan("ipn:1.0");
	if (rc < 0)
	{
		TEST_FAIL("Start plan", "bp_start_plan failed");
		goto cleanup;
	}
	TEST_PASS("Start plan ipn:1.0 (initial)");

	/* Give bpclm time to start */
	sleep(2);

	/*
	 * Test 20: Verify bpclm is running
	 */
	TEST_START("Verify bpclm daemon is running");
	bpclm_pid = get_bpclm_pid();
	if (bpclm_pid <= 0)
	{
		TEST_FAIL("Verify bpclm running", "bpclm daemon not found");
		goto cleanup;
	}
	LOG_INFO("bpclm PID: %d", bpclm_pid);
	TEST_PASS("Verify bpclm daemon is running");

	/*
	 * Test 21: Simulate crash by killing bpclm
	 */
	TEST_START("Simulate bpclm crash (SIGKILL)");
	if (kill(bpclm_pid, SIGKILL) < 0)
	{
		TEST_FAIL("Simulate crash", "Failed to kill bpclm");
		goto cleanup;
	}
	sleep(1);  /* Give process time to die */

	/* Verify it's dead */
	if (pid_exists(bpclm_pid))
	{
		TEST_FAIL("Simulate crash", "bpclm PID still exists after SIGKILL");
		goto cleanup;
	}
	LOG_INFO("bpclm daemon killed successfully (PID %d)", bpclm_pid);
	TEST_PASS("Simulate bpclm crash (SIGKILL)");

	/*
	 * Test 22: Restart the plan (THIS IS THE CRITICAL TEST)
	 */
	TEST_START("Restart plan ipn:1.0 after crash");
	rc = bp_start_plan("ipn:1.0");
	if (rc < 0)
	{
		TEST_FAIL("Restart plan", "bp_start_plan failed after crash - semaphore not reset!");
		goto cleanup;
	}
	TEST_PASS("Restart plan ipn:1.0 after crash");

	/* Give new bpclm time to start */
	sleep(2);

	/*
	 * Test 23: Verify bpclm restarted successfully
	 */
	TEST_START("Verify bpclm daemon restarted");
	bpclm_pid = get_bpclm_pid();
	if (bpclm_pid <= 0)
	{
		TEST_FAIL("Verify bpclm restarted", "bpclm daemon not running after restart");
		goto cleanup;
	}
	LOG_INFO("New bpclm PID: %d", bpclm_pid);
	TEST_PASS("Verify bpclm daemon restarted");

cleanup:
	/*
	 * Cleanup
	 */
	LOG_INFO("Performing cleanup...");
	test_cleanup();

	/*
	 * Print summary
	 */
	printf("\n");
	printf(COLOR_BLUE "========================================\n" COLOR_RESET);
	printf(COLOR_BLUE "Test Summary\n" COLOR_RESET);
	printf(COLOR_BLUE "========================================\n" COLOR_RESET);
	printf("Total tests:  %d\n", results.total);
	printf(COLOR_GREEN "Passed:       %d\n" COLOR_RESET, results.passed);
	if (results.failed > 0)
	{
		printf(COLOR_RED "Failed:       %d\n" COLOR_RESET, results.failed);
	}
	else
	{
		printf("Failed:       %d\n", results.failed);
	}

	if (results.failed == 0 && results.passed > 0)
	{
		printf("\n" COLOR_GREEN "ALL TESTS PASSED!\n" COLOR_RESET);
		printf(COLOR_GREEN "BP plan crash recovery is working correctly.\n" COLOR_RESET);
		return 0;
	}
	else
	{
		printf("\n" COLOR_RED "SOME TESTS FAILED!\n" COLOR_RESET);
		return 1;
	}
}
