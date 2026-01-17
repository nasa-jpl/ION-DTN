/*
 * ltp_span_management_test.c
 *
 * Test program for ION LTP Span Management APIs
 * Tests dynamic span lifecycle: remove, add, start
 * Validates bundle buffering during span removal and delivery after restart
 *
 * Test Sequence:
 * 1. Full ION/LTP/BP setup
 * 2. Test 1: Send bundle with span running (should deliver)
 * 3. Remove LTP span (note: remove_span internally stops the span)
 * 4. Stop ltpclo outduct (recommended when removing last span)
 * 5. Test 2: Send bundle while span/outduct stopped (buffered in BP queue)
 * 6. Wait 10 seconds
 * 7. Re-add and start LTP span
 * 8. Restart ltpclo outduct
 * 9. Test 3: Verify buffered bundle from Test 2 is delivered
 *
 * Note: remove_span() internally calls stopSpan + waitForSpan (without resetSpan),
 * since resetSpan is only needed for restart scenarios, not removal.
 *
 * Compile:
 * gcc -o dotest ltp_span_management_test.c ../ltp_loopback/ion_test_utils.c -lici -lbp -lltp -lpthread -lm -lrt
 *
 * Run:
 * ./dotest
 */

#include "../ltp_loopback/ion_test_utils.h"
#include "ltp_admin.h"
#include "ltp.h"
#include "rfx.h"

/* Global test results */
TestResults results = { 0, 0, 0 };

/*
 * =============================================================================
 * LTP-Specific Test Functions
 * =============================================================================
 */

/* Test: LTP Initialization */
static int test_ltp_init(unsigned int estMaxExportSessions)
{
	TEST_START("LTP Initialization");
	LOG_INFO("Calling ltp_init(%u)...", estMaxExportSessions);
	if (ltp_init(estMaxExportSessions) < 0)
	{
		TEST_FAIL("ltp_init", "Failed to initialize LTP");
		return -1;
	}
	TEST_PASS("ltp_init");
	return 0;
}

/* Test: Add LTP Span */
static int test_add_ltp_span(uvast engine_id, unsigned int max_export_sessions,
		unsigned int max_import_sessions, unsigned int max_segment_size,
		unsigned int aggr_size_limit, unsigned int aggr_time_limit,
		char *lso_command, int queuing_latency, int purge_enabled)
{
	char	passMsg[128];

	TEST_START("Add LTP Span");
	LOG_INFO("Adding LTP span to engine " UVAST_FIELDSPEC "...", engine_id);
	if (add_span(engine_id,
			max_export_sessions,
			max_import_sessions,
			max_segment_size,
			aggr_size_limit,
			aggr_time_limit,
			lso_command,
			queuing_latency,
			purge_enabled) <= 0)
	{
		TEST_FAIL("add_span", "Failed to add LTP span");
		return -1;
	}
	snprintf(passMsg, sizeof(passMsg), "add_span to engine " UVAST_FIELDSPEC, engine_id);
	TEST_PASS(passMsg);
	return 0;
}

/* Test: Add LTP Seat */
static int test_add_ltp_seat(char *lsi_command)
{
	TEST_START("Add LTP Seat");
	LOG_INFO("Adding LTP seat: %s", lsi_command);
	if (add_seat(lsi_command) <= 0)
	{
		TEST_FAIL("add_seat", "Failed to add LTP seat");
		return -1;
	}
	TEST_PASS("add_seat");
	return 0;
}

/* Test: Start LTP Engine */
static int test_ltp_start(void)
{
	TEST_START("Start LTP Engine");
	LOG_INFO("Calling ltp_start()...");
	if (ltp_start() < 0)
	{
		TEST_FAIL("ltp_start", "Failed to start LTP engine");
		return -1;
	}
	TEST_PASS("ltp_start");
	LOG_INFO("Waiting for LTP to fully start...");
	sleep(5);  /* Give LTP time to fully start */
	return 0;
}

/* Test: Remove LTP Span */
static int test_remove_ltp_span(uvast engine_id)
{
	int result;

	LOG_INFO("Calling remove_span(" UVAST_FIELDSPEC ")...", engine_id);
	result = remove_span(engine_id);
	if (result <= 0)
	{
		if (result == 0)
		{
			LOG_INFO("remove_span returned 0: Span not found or has pending data");
			LOG_INFO("  This means one of the following:");
			LOG_INFO("  - Span has pending segments in queue (not yet transmitted)");
			LOG_INFO("  - Span has open export sessions (outbound, not fully acknowledged)");
			LOG_INFO("  - Span has open import sessions (inbound, still receiving)");
			LOG_INFO("  - Span has dead/canceled sessions (waiting for cleanup)");
		}
		else
		{
			LOG_INFO("remove_span returned -1: Failed to remove span (error condition)");
		}
		return -1;
	}
	LOG_INFO("remove_span succeeded!");
	LOG_INFO("Waiting for span removal to complete...");
	sleep(2);
	return 0;
}

/* Test: Start LTP Span */
static int test_ltp_start_span(uvast engine_id)
{
	int result;

	TEST_START("Start LTP Span");
	LOG_INFO("Calling ltp_start_span(" UVAST_FIELDSPEC ")...", engine_id);
	result = ltp_start_span(engine_id);
	if (result <= 0)
	{
		if (result == 0)
		{
			TEST_FAIL("ltp_start_span", "Span not found");
		}
		else
		{
			TEST_FAIL("ltp_start_span", "Failed to start span");
		}
		return -1;
	}
	TEST_PASS("ltp_start_span");
	LOG_INFO("Waiting for span to fully start...");
	sleep(5);
	return 0;
}

/* Helper: Send bundle test */
static int send_bundle_test(const char *test_name, const char *message,
		const char *output_file, int expect_delivery)
{
	FILE *fp;
	char line[256];
	int found = 0;
	char echo_cmd[512];
	char cat_cmd[256];

	TEST_START(test_name);

	/* Clean up previous output file */
	unlink(output_file);

	/* Start bpsink in background if not already running */
	LOG_INFO("Starting bpsink to receive at ipn:1.2...");
	snprintf(echo_cmd, sizeof(echo_cmd), "bpsink ipn:1.2 >> %s 2>&1 &", output_file);
	if (system(echo_cmd) != 0)
	{
		LOG_INFO("Warning: bpsink command returned non-zero status");
	}
	sleep(2);  /* Give bpsink time to attach */

	/* Send bundle */
	LOG_INFO("Sending bundle: '%s'", message);
	snprintf(echo_cmd, sizeof(echo_cmd), "echo '%s' | bpsource ipn:1.2", message);
	if (system(echo_cmd) != 0)
	{
		LOG_INFO("Warning: bpsource command returned non-zero status");
	}

	/* Wait for potential delivery */
	sleep(5);

	/* Check delivery status */
	LOG_INFO("Checking delivery status in %s...", output_file);
	fp = fopen(output_file, "r");
	if (fp == NULL)
	{
		if (expect_delivery)
		{
			TEST_FAIL(test_name, "Cannot open output file");
			LOG_INFO("Error: %s", strerror(errno));
			return -1;
		}
		else
		{
			/* No file means no delivery - expected for buffered case */
			TEST_PASS(test_name);
			LOG_INFO("Bundle buffered as expected (no delivery yet)");
			return 0;
		}
	}

	/* Parse output file */
	while (fgets(line, sizeof(line), fp) != NULL)
	{
		if (strstr(line, "Payload delivered") != NULL)
		{
			found = 1;
			break;
		}
	}
	fclose(fp);

	/* Verify expectation */
	if (expect_delivery)
	{
		if (found)
		{
			TEST_PASS(test_name);
			LOG_INFO("Bundle delivered as expected!");
			LOG_INFO("Output: cat %s", output_file);
			snprintf(cat_cmd, sizeof(cat_cmd), "cat %s 2>/dev/null", output_file);
			if (system(cat_cmd)) { /* ignored */ }
		}
		else
		{
			TEST_FAIL(test_name, "Bundle not delivered");
			LOG_INFO("Output (for debugging):");
			snprintf(cat_cmd, sizeof(cat_cmd), "cat %s 2>/dev/null", output_file);
			if (system(cat_cmd)) { /* ignored */ }
			return -1;
		}
	}
	else
	{
		if (!found)
		{
			TEST_PASS(test_name);
			LOG_INFO("Bundle buffered as expected (no delivery yet)");
		}
		else
		{
			TEST_FAIL(test_name, "Bundle was delivered but should be buffered");
			LOG_INFO("Output (for debugging):");
			snprintf(cat_cmd, sizeof(cat_cmd), "cat %s 2>/dev/null", output_file);
			if (system(cat_cmd)) { /* ignored */ }
			return -1;
		}
	}

	return 0;
}

/*
 * =============================================================================
 * Main Test Runner
 * =============================================================================
 */

int main(void)
{
	time_t	now;

	printf("\n========================================\n");
	printf("  ION LTP Span Management Test Suite\n");
	printf("========================================\n");

	/* Step 1: ION initialization */
	printf("\n--- Step 1: ION Initialization ---\n");
	if (test_ion_initialization_and_attach(1) < 0)
	{
		goto cleanup;
	}

	/* Step 1.5: Register node in region */
	printf("\n--- Step 1.5: Register Node in Region ---\n");
	printf(COLOR_BLUE "==> Testing: Register Node in Region" COLOR_RESET "\n");
	results.total++;
	LOG_INFO("Calling ion_register_node(1)...");
	if (ion_register_node(1) < 0)
	{
		TEST_FAIL("ion_register_node", "Failed to register node in region");
		goto cleanup;
	}
	TEST_PASS("ion_register_node(1)");

	/* Step 1.6: Initialize ION Security (empty database) */
	printf("\n--- Step 1.6: Initialize ION Security ---\n");
	if (test_ionsec_initialization_and_attach() < 0)
	{
		goto cleanup;
	}

	/* Step 2: Start RFX */
	printf("\n--- Step 2: Start RFX ---\n");
	if (test_rfx_start() < 0)
	{
		goto cleanup;
	}

	/* Step 3: Add Contact */
	printf("\n--- Step 3: Add Contact ---\n");
	now = time(NULL);
	if (test_add_contact(now + 1, now + 3600, 1, 1, 100000, 1.0) < 0)
	{
		goto cleanup;
	}

	/* Step 3.5: Add Range */
	printf("\n--- Step 3.5: Add Range ---\n");
	if (test_add_range(now + 1, now + 3600, 1, 1, 1) < 0)
	{
		goto cleanup;
	}

	/* Step 4: Initialize LTP */
	printf("\n--- Step 4: Initialize LTP ---\n");
	if (test_ltp_init(1000) < 0)
	{
		goto cleanup;
	}

	/* Step 5: Configure LTP engine (spans and seats) */
	printf("\n--- Step 5: Configure LTP Engine ---\n");
	if (test_add_ltp_span(1, 100, 100, 1400, 10000, 1,
			"udplso localhost:1113", 1, 0) < 0)
	{
		goto cleanup;
	}

	if (test_add_ltp_seat("udplsi localhost:1113") < 0)
	{
		goto cleanup;
	}

	/* Step 6: Start LTP engine */
	printf("\n--- Step 6: Start LTP Engine ---\n");
	if (test_ltp_start() < 0)
	{
		goto cleanup;
	}

	/* Step 7: Initialize BP */
	printf("\n--- Step 7: Initialize Bundle Protocol ---\n");
	if (test_bp_initialization() < 0)
	{
		goto cleanup;
	}

	if (test_bp_attach() < 0)
	{
		goto cleanup;
	}

	/* Step 8: Configure BP scheme and endpoints */
	printf("\n--- Step 8: Configure BP Scheme and Endpoints ---\n");
	if (test_add_scheme("ipn", "ipnfw", "ipnadminep") < 0)
	{
		goto cleanup;
	}

	if (test_add_endpoint("ipn:1.1", EnqueueBundle, NULL) < 0)
	{
		goto cleanup;
	}

	if (test_add_endpoint("ipn:1.2", EnqueueBundle, NULL) < 0)
	{
		goto cleanup;
	}

	/* Step 9: Add LTP protocol and configure ducts */
	printf("\n--- Step 9: Add LTP Protocol Configuration ---\n");
	if (test_add_protocol("ltp", 0) < 0)  /* 0 = Scheduled */
	{
		goto cleanup;
	}

	if (test_add_induct("ltp", "1", "ltpcli") < 0)
	{
		goto cleanup;
	}

	if (test_add_outduct("ltp", "1", "ltpclo", 0) < 0)
	{
		goto cleanup;
	}

	if (test_add_plan("ipn:1.0", 0) < 0)
	{
		goto cleanup;
	}

	if (test_add_planduct("ipn:1.0", "ltp", "1") < 0)
	{
		goto cleanup;
	}

	/* Step 10: Start BP */
	printf("\n--- Step 10: Start Bundle Protocol ---\n");
	if (test_bp_start() < 0)
	{
		goto cleanup;
	}

	/*
	 * =================================================================
	 * Span Management Tests
	 * =================================================================
	 */

	/* Test 1: Initial bundle transmission with span running */
	printf("\n--- Step 11: Test 1 - Initial Bundle Transmission ---\n");
	LOG_INFO("Testing bundle delivery with span running...");
	time_t bundle_sent_time = time(NULL);  /* Record time for diagnostics */
	if (send_bundle_test("Test 1: Initial Transmission",
			"Test 1: Initial transmission",
			"./bpsink_test1.txt",
			1) < 0)  /* expect_delivery = 1 */
	{
		goto cleanup;
	}

	/* Stop bpsink to prepare for next test */
	LOG_INFO("Stopping bpsink before span management...");
	ensure_process_terminated("bpsink");
	sleep(2);

	/* Wait for all LTP sessions to complete and segments to be acknowledged.
	 * This includes waiting for LtpForgetImportSession timeline events to
	 * fire and clean up closed sessions. */
	LOG_INFO("Waiting for LTP sessions to complete and be forgotten...");
	LOG_INFO("This may take 20+ seconds depending on configured timeouts...");
	sleep(20);

	/* Show current span state before removal attempt */
	now = time(NULL);
	int elapsed = (int)(now - bundle_sent_time);
	LOG_INFO("Current LTP span state before removal:");
	LOG_INFO("Time since bundle sent: %d seconds", elapsed);
	ltp_list_spans();
	printf("\n");
	LOG_INFO("Detailed session breakdown:");
	ltp_print_span_sessions(1);
	printf("\n");

	/* Test 2: Remove LTP Span (this internally stops the span) */
	printf("\n--- Step 12: Remove LTP Span ---\n");
	TEST_START("Remove LTP Span");
	LOG_INFO("Note: remove_span() internally calls stopSpan + waitForSpan");
	LOG_INFO("Waiting for pending LTP operations to clear...");
	sleep(5);

	/* Try to remove span with retries */
	int remove_attempts = 0;
	int remove_success = 0;
	while (remove_attempts < 5)
	{
		remove_attempts++;
		LOG_INFO("Removal attempt %d/5...", remove_attempts);

		/* Show span state before removal attempt */
		if (remove_attempts > 1)
		{
			LOG_INFO("Current span state (attempt %d):", remove_attempts);
			LOG_INFO("Detailed session counts:");
			ltp_print_span_sessions(1);
			printf("\n");
			LOG_INFO("Span configuration:");
			ltp_list_spans();
			printf("\n");
		}

		if (test_remove_ltp_span(1) == 0)
		{
			remove_success = 1;
			break;
		}

		if (remove_attempts < 5)
		{
			LOG_INFO("Waiting 10 more seconds for closed sessions to be forgotten...");
			sleep(10);
		}
	}

	if (!remove_success)
	{
		LOG_INFO("Final diagnostic - showing span state after all removal attempts:");
		LOG_INFO("Detailed session breakdown:");
		ltp_print_span_sessions(1);
		printf("\n");
		LOG_INFO("Span configuration:");
		ltp_list_spans();
		printf("\n");
		LOG_INFO("Possible causes for removal failure:");
		LOG_INFO("  1. Pending export sessions (outbound data not yet acknowledged)");
		LOG_INFO("  2. Pending import sessions (inbound data being received)");
		LOG_INFO("  3. Pending segments in queue (not yet transmitted)");
		LOG_INFO("  4. Dead/canceled sessions (waiting for cleanup)");
		LOG_INFO("Suggestion: Check ion.log for LTP session/segment details");
		LOG_INFO("Total time waited: %d seconds", (int)(time(NULL) - bundle_sent_time));
		TEST_FAIL("remove_span", "Failed to remove span after 5 attempts - span may have persistent pending data");
		goto cleanup;
	}

	TEST_PASS("remove_span");

	/* Confirm span was removed */
	LOG_INFO("Span removed successfully. Verifying removal:");
	ltp_list_spans();
	printf("\n");

	/*
	 * Stop ltpclo after removing the last span to prevent it from crashing
	 * when trying to transmit to a non-existent destination.
	 *
	 * This is the recommended practice: when removing the last LTP span,
	 * stop the LTP convergence layer outduct. Bundles will be safely
	 * queued in BP's outduct queue until ltpclo is restarted.
	 *
	 * Note: Outduct name is the destination engine ID (as a string).
	 */
	printf("\n--- Step 12b: Stop LTP Outduct ---\n");
	LOG_INFO("Stopping ltpclo outduct for engine 1 (no spans remain)");
	bp_stop_outduct("ltp", "1");  /* "1" is the destination engine ID */
	sleep(2);  /* Wait for ltpclo to fully stop */
	LOG_INFO("ltpclo stopped - bundles will be buffered in BP outduct queue");

	/* Test 3: Send bundle while span is removed (should buffer) */
	printf("\n--- Step 13: Test 2 - Bundle During Span Removal ---\n");
	LOG_INFO("Testing bundle buffering with span removed...");
	if (send_bundle_test("Test 2: Buffered During Removal",
			"Test 2: Buffered during span removal",
			"./bpsink_test2.txt",
			0) < 0)  /* expect_delivery = 0 (buffered) */
	{
		goto cleanup;
	}

	/* Test 4: Wait period */
	printf("\n--- Step 14: Wait Period (10 seconds) ---\n");
	LOG_INFO("Waiting 10 seconds before re-adding span...");
	sleep(10);

	/* Test 5: Re-add LTP Span */
	printf("\n--- Step 15: Re-add LTP Span ---\n");
	if (test_add_ltp_span(1, 100, 100, 1400, 10000, 1,
			"udplso localhost:1113", 1, 0) < 0)
	{
		goto cleanup;
	}

	/* Test 6: Start LTP Span */
	printf("\n--- Step 16: Start LTP Span ---\n");
	if (test_ltp_start_span(1) < 0)
	{
		goto cleanup;
	}

	/*
	 * Restart ltpclo now that we have an active span again.
	 *
	 * We stopped ltpclo in Step 12b after removing the last span to
	 * prevent transmission errors. Now that we've added a span back,
	 * we restart ltpclo to process the buffered bundle.
	 *
	 * Note: Bundles sent while ltpclo was stopped (like Test 2) are
	 * safely queued in BP's outduct queue and will now be transmitted.
	 * Outduct name is the destination engine ID (as a string).
	 */
	printf("\n--- Step 16b: Restart LTP Outduct ---\n");
	LOG_INFO("Restarting ltpclo outduct for engine 1");
	if (bp_start_outduct("ltp", "1") < 0)  /* "1" is the destination engine ID */
	{
		TEST_FAIL("bp_start_outduct", "Failed to restart ltpclo");
		goto cleanup;
	}
	LOG_INFO("ltpclo outduct restarted - will process buffered bundles");
	sleep(2);  /* Give ltpclo time to initialize */

	/* Test 7: Verify buffered bundle is delivered */
	printf("\n--- Step 17: Test 3 - Verify Buffered Bundle Delivery ---\n");
	LOG_INFO("Verifying buffered bundle is now delivered...");
	LOG_INFO("Waiting for buffered bundle delivery...");
	sleep(10);  /* Give more time for buffered bundle to be processed */

	/* Check if the buffered bundle was delivered */
	TEST_START("Test 3: Buffered Bundle Delivery");
	FILE *fp = fopen("./bpsink_test2.txt", "r");
	if (fp == NULL)
	{
		TEST_FAIL("Test 3", "Cannot open output file for buffered bundle");
		LOG_INFO("Error: %s", strerror(errno));
	}
	else
	{
		char line[256];
		int found = 0;

		while (fgets(line, sizeof(line), fp) != NULL)
		{
			if (strstr(line, "Payload delivered") != NULL)
			{
				found = 1;
				break;
			}
		}
		fclose(fp);

		if (found)
		{
			TEST_PASS("Test 3: Buffered Bundle Delivery");
			LOG_INFO("Buffered bundle delivered successfully after span restart!");
			LOG_INFO("Output:");
			if (system("cat ./bpsink_test2.txt")) { /* ignored */ }
		}
		else
		{
			TEST_FAIL("Test 3", "Buffered bundle not delivered after span restart");
			LOG_INFO("Output (for debugging):");
			if (system("cat ./bpsink_test2.txt")) { /* ignored */ }
		}
	}

	/* Stop bpsink */
	LOG_INFO("Stopping bpsink...");
	ensure_process_terminated("bpsink");
	sleep(2);

	printf("\n========================================\n");
	printf("  Span Management Tests Complete\n");
	printf("========================================\n");

cleanup:
	test_cleanup();

	printf("\n========================================\n");
	printf("  Test Summary\n");
	printf("========================================\n");
	printf("  Total:  %d\n", results.total);
	printf(COLOR_GREEN "  Passed: %d" COLOR_RESET "\n", results.passed);
	if (results.failed > 0)
	{
		printf(COLOR_RED "  Failed: %d" COLOR_RESET "\n", results.failed);
	}
	else
	{
		printf("  Failed: %d\n", results.failed);
	}

	printf("========================================\n");
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
