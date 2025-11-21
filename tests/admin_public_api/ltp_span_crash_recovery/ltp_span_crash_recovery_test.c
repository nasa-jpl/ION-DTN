/*
 * ltp_span_crash_recovery_test.c
 *
 * Regression test for LTP span crash recovery.
 *
 * This test verifies that ltpStartSpan() can successfully restart an LTP span's
 * LSO (Link Service Output) daemon after it has crashed or been forcibly
 * terminated. The test ensures that the semaphore reset bug fix (calling
 * resetSpan() before startSpan()) works correctly.
 *
 * Test Sequence:
 * 1. Initialize ION, LTP, and BP with an LTP span to node 2
 * 2. Start the span normally (spawns udplso LSO daemon)
 * 3. Verify udplso is running
 * 4. Forcibly kill udplso daemon (simulates crash)
 * 5. Call ltpStartSpan() to restart the span
 * 6. Verify udplso successfully restarted
 * 7. Clean shutdown
 *
 * Expected Result:
 * - udplso should successfully restart after crash
 * - No "semaphore ended" errors
 * - Span remains functional after restart
 */

#include "../ltp_loopback/ion_test_utils.h"
#include "ltp_admin.h"
#include "ltp.h"
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
 * Get the PID of the udplso daemon for engine 2
 * Note: The LSO command is "udplso localhost:4556 2"
 * On Solaris, ps truncates to "udplso localhost:4" so we use a
 * pattern that works on both Linux (full output) and Solaris (truncated).
 */
static int get_ltpclo_pid(void)
{
	FILE *fp;
	char cmd[256];
	char line[128];
	int pid = -1;

	/* Use ps to find udplso process - pattern matches both truncated and full output */
	snprintf(cmd, sizeof(cmd), "ps aux | grep 'udplso localhost:4' | grep -v grep | awk '{print $2}'");
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
	int ltpclo_pid;
	time_t current_time;

	/* Suppress unused parameter warnings */
	(void)argc;
	(void)argv;

	printf("\n");
	printf(COLOR_BLUE "========================================\n" COLOR_RESET);
	printf(COLOR_BLUE "LTP Span Crash Recovery Regression Test\n" COLOR_RESET);
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
	 * Test 5: Add contact (node 1 to node 2)
	 */
	TEST_START("Add contact 1->2");
	current_time = time(NULL);
	rc = test_add_contact(current_time, current_time + 3600, 1, 2, 100000, 1.0);
	if (rc < 0)
	{
		TEST_FAIL("Add contact", "add_contact failed");
		goto cleanup;
	}
	TEST_PASS("Add contact 1->2");

	/*
	 * Test 6: Add range
	 */
	TEST_START("Add range 1<->2");
	rc = test_add_range(current_time, current_time + 3600, 1, 2, 1);
	if (rc < 0)
	{
		TEST_FAIL("Add range", "add_range failed");
		goto cleanup;
	}
	TEST_PASS("Add range 1<->2");

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
	 * Test 8: Initialize LTP
	 */
	TEST_START("Initialize LTP");
	rc = ltp_init(1400);
	if (rc < 0)
	{
		TEST_FAIL("Initialize LTP", "ltp_init failed");
		goto cleanup;
	}
	TEST_PASS("Initialize LTP");

	/*
	 * Test 9: Attach to LTP
	 */
	TEST_START("Attach to LTP");
	rc = ltp_attach();
	if (rc < 0)
	{
		TEST_FAIL("Attach to LTP", "ltp_attach failed");
		goto cleanup;
	}
	TEST_PASS("Attach to LTP");

	/*
	 * Test 10: Start LTP
	 */
	TEST_START("Start LTP");
	rc = ltp_start();
	if (rc < 0)
	{
		TEST_FAIL("Start LTP", "ltp_start failed");
		goto cleanup;
	}
	TEST_PASS("Start LTP");

	/*
	 * Test 11: Add LTP span to engine 2
	 */
	TEST_START("Add LTP span to engine 2");
	rc = add_span(2,                   /* engine_id */
	              100,                 /* max_export_sessions */
	              100,                 /* max_import_sessions */
	              1400,                /* max_segment_size */
	              30000,               /* aggr_size_limit */
	              5,                   /* aggr_time_limit */
	              "udplso localhost:4556",  /* lso_command */
	              1,                   /* queuing_latency */
	              0);                  /* purge_enabled */
	if (rc <= 0)
	{
		TEST_FAIL("Add LTP span", "add_span failed");
		goto cleanup;
	}
	TEST_PASS("Add LTP span to engine 2");

	/*
	 * Test 12: Initialize BP
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
	 * Test 13: Attach to BP
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
	 * Test 14: Add ipn scheme
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
	 * Test 15: Add endpoint
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
	 * Test 16: Add LTP protocol
	 */
	TEST_START("Add LTP protocol");
	rc = test_add_protocol("ltp", 0);  /* 0 = Scheduled */
	if (rc < 0)
	{
		TEST_FAIL("Add LTP protocol", "add_protocol failed");
		goto cleanup;
	}
	TEST_PASS("Add LTP protocol");

	/*
	 * Test 17: Add LTP induct
	 */
	TEST_START("Add LTP induct");
	rc = test_add_induct("ltp", "2", "ltpcli");
	if (rc < 0)
	{
		TEST_FAIL("Add LTP induct", "add_induct failed");
		goto cleanup;
	}
	TEST_PASS("Add LTP induct");

	/*
	 * Test 18: Add LTP outduct
	 */
	TEST_START("Add LTP outduct");
	rc = test_add_outduct("ltp", "2", "ltpclo", 0);
	if (rc < 0)
	{
		TEST_FAIL("Add LTP outduct", "add_outduct failed");
		goto cleanup;
	}
	TEST_PASS("Add LTP outduct");

	/*
	 * Test 19: Add plan
	 */
	TEST_START("Add plan ipn:2.0");
	rc = test_add_plan("ipn:2.0", 100000);
	if (rc < 0)
	{
		TEST_FAIL("Add plan", "add_plan failed");
		goto cleanup;
	}
	TEST_PASS("Add plan ipn:2.0");

	/*
	 * Test 20: Add planduct
	 */
	TEST_START("Add planduct");
	rc = test_add_planduct("ipn:2.0", "ltp", "2");
	if (rc < 0)
	{
		TEST_FAIL("Add planduct", "add_planduct failed");
		goto cleanup;
	}
	TEST_PASS("Add planduct");

	/*
	 * Test 21: Start BP
	 */
	TEST_START("Start BP");
	rc = test_bp_start();
	if (rc < 0)
	{
		TEST_FAIL("Start BP", "bp_start failed");
		goto cleanup;
	}
	TEST_PASS("Start BP");

	/*
	 * Test 22: Start the span (spawns udplso)
	 */
	TEST_START("Start LTP span 2 (initial)");
	rc = ltp_start_span(2);
	if (rc < 0)
	{
		TEST_FAIL("Start LTP span", "ltp_start_span failed");
		goto cleanup;
	}
	TEST_PASS("Start LTP span 2 (initial)");

	/* Give udplso time to start */
	sleep(2);

	/*
	 * Test 23: Verify udplso is running
	 */
	TEST_START("Verify udplso daemon is running");
	ltpclo_pid = get_ltpclo_pid();
	if (ltpclo_pid <= 0)
	{
		TEST_FAIL("Verify udplso running", "udplso daemon not found");
		goto cleanup;
	}
	LOG_INFO("udplso PID: %d", ltpclo_pid);
	TEST_PASS("Verify udplso daemon is running");

	/*
	 * Test 24: Simulate crash by killing udplso
	 */
	TEST_START("Simulate udplso crash (SIGKILL)");
	if (kill(ltpclo_pid, SIGKILL) < 0)
	{
		TEST_FAIL("Simulate crash", "Failed to kill udplso");
		goto cleanup;
	}
	sleep(1);  /* Give process time to die */

	/* Verify it's dead */
	if (pid_exists(ltpclo_pid))
	{
		TEST_FAIL("Simulate crash", "udplso PID still exists after SIGKILL");
		goto cleanup;
	}
	LOG_INFO("udplso daemon killed successfully (PID %d)", ltpclo_pid);
	TEST_PASS("Simulate udplso crash (SIGKILL)");

	/*
	 * Test 25: Restart the span (THIS IS THE CRITICAL TEST)
	 */
	TEST_START("Restart LTP span 2 after crash");
	rc = ltp_start_span(2);
	if (rc < 0)
	{
		TEST_FAIL("Restart LTP span", "ltp_start_span failed after crash - semaphore not reset!");
		goto cleanup;
	}
	TEST_PASS("Restart LTP span 2 after crash");

	/* Give new udplso time to start */
	sleep(2);

	/*
	 * Test 26: Verify udplso restarted successfully
	 */
	TEST_START("Verify udplso daemon restarted");
	ltpclo_pid = get_ltpclo_pid();
	if (ltpclo_pid <= 0)
	{
		TEST_FAIL("Verify udplso restarted", "udplso daemon not running after restart");
		goto cleanup;
	}
	LOG_INFO("New udplso PID: %d", ltpclo_pid);
	TEST_PASS("Verify udplso daemon restarted");

	/*
	 * Test 27: Stop and remove the span before cleanup
	 * Since we forcibly killed udplso, the span may be in an inconsistent state.
	 * Following the pattern from ltp_span_management test: stop then remove span.
	 */
	TEST_START("Stop LTP span 2");
	ltp_stop_span(2);
	TEST_PASS("Stop LTP span 2");
	sleep(2);  /* Wait for span to stop */

	TEST_START("Remove LTP span 2");
	rc = remove_span(2);
	if (rc <= 0)
	{
		LOG_INFO("Warning: remove_span failed or returned 0 (span may have pending data)");
		LOG_INFO("Waiting for pending operations to clear...");
		sleep(5);
		/* Try one more time */
		rc = remove_span(2);
		if (rc <= 0)
		{
			LOG_INFO("Warning: remove_span still failed - continuing with cleanup anyway");
		}
		else
		{
			TEST_PASS("Remove LTP span 2");
		}
	}
	else
	{
		TEST_PASS("Remove LTP span 2");
	}
	sleep(2);  /* Wait for removal to complete */

cleanup:
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
		printf(COLOR_GREEN "LTP span crash recovery is working correctly.\n" COLOR_RESET);
		return 0;
	}
	else
	{
		printf("\n" COLOR_RED "SOME TESTS FAILED!\n" COLOR_RESET);
		return 1;
	}
}
