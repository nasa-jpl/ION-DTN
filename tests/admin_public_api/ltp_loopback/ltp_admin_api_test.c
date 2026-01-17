/*
 * ltp_admin_api_test.c
 *
 * Test program for ION LTP Administrative APIs
 * Tests LTP-specific public functions with bundle transfer
 *
 * Compile:
 * gcc -o dotest ltp_admin_api_test.c ion_test_utils.c -lici -lbp -lltp -lpthread -lm -lrt
 *
 * Run:
 * ./dotest
 */

#include "ion_test_utils.h"
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
		const char *lso_command, int queuing_latency, int purge_enabled)
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
			(char *)(uintptr_t) lso_command,
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
static int test_add_ltp_seat(const char *lsi_command)
{
	TEST_START("Add LTP Seat");
	LOG_INFO("Adding LTP seat: %s", lsi_command);
	if (add_seat((char *)(uintptr_t)lsi_command) <= 0)
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

/* Test: List LTP Configuration */
static int test_list_ltp_config(void)
{
	TEST_START("List LTP Configuration");
	LOG_INFO("Listing LTP spans...");
	ltp_list_spans();
	printf("\n");
	LOG_INFO("Listing LTP seats...");
	ltp_list_seats();
	printf("\n");
	TEST_PASS("List LTP configuration");
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
	printf("  ION LTP Admin API Test Suite\n");
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

	/* Step 7: List LTP configuration */
	printf("\n--- Step 7: List LTP Configuration ---\n");
	if (test_list_ltp_config() < 0)
	{
		goto cleanup;
	}

	/* Step 8: Initialize BP */
	printf("\n--- Step 8: Initialize Bundle Protocol ---\n");
	if (test_bp_initialization() < 0)
	{
		goto cleanup;
	}

	if (test_bp_attach() < 0)
	{
		goto cleanup;
	}

	/* Step 9: Configure BP scheme and endpoints */
	printf("\n--- Step 9: Configure BP Scheme and Endpoints ---\n");
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

	/* Step 10: Add LTP protocol and configure ducts */
	printf("\n--- Step 10: Add LTP Protocol Configuration ---\n");
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

	if (test_list_protocols() < 0)
	{
		goto cleanup;
	}

	/* Step 11: Start BP */
	printf("\n--- Step 11: Start Bundle Protocol ---\n");
	if (test_bp_start() < 0)
	{
		goto cleanup;
	}

	/* Step 12: Bundle send/receive test */
	printf("\n--- Step 12: Bundle Send/Receive Test (LTP) ---\n");
	printf(" * Sending bundle to ipn:1.2 using bpsource and bpsink\n");

	//printf("\n" COLOR_YELLOW "Press ENTER to continue with LTP bundle test..." COLOR_RESET);
	//getchar();

	/* Clean up temporary file */
	if (system("rm -f ./bpsink_ltp_output.txt")) { /* ignored */ }

	/* Start bpsink in background to receive bundles at ipn:1.2 */
	LOG_INFO("Starting bpsink to receive at ipn:1.2...");
	int bpsink_result = system("bpsink ipn:1.2 > ./bpsink_ltp_output.txt 2>&1 &");
	if (bpsink_result != 0)
	{
		LOG_INFO("Warning: bpsink command returned non-zero status: %d", bpsink_result);
	}
	else
	{
		LOG_INFO("bpsink command executed");
	}
	sleep(2);  /* Give bpsink time to attach to endpoint */

	/* Send a bundle using bpsource */
	LOG_INFO("Sending bundle to ipn:1.2 using bpsource...");
	int bpsource_result = system("echo 'Hello from ION LTP test!' | bpsource ipn:1.2");
	if (bpsource_result != 0)
	{
		LOG_INFO("Warning: bpsource command returned non-zero status: %d", bpsource_result);
	}
	else
	{
		LOG_INFO("bpsource command executed");
	}

	/* Wait for bundle delivery */
	sleep(5);

	/* Stop bpsink */
	LOG_INFO("Stopping bpsink...");
	ensure_process_terminated("bpsink");
	sleep(2);  /* Give system time to cleanup */

	/* Check if bundle was received - Direct file parsing method */
	TEST_START("LTP Bundle Transfer Test");
	LOG_INFO("Checking bpsink output...");

	FILE *fp = fopen("./bpsink_ltp_output.txt", "r");
	if (fp == NULL) {
		TEST_FAIL("LTP Bundle Transfer", "Cannot open bpsink output file");
		LOG_INFO("Error: %s", strerror(errno));
	} else {
		char line[256];
		int found = 0;

		while (fgets(line, sizeof(line), fp) != NULL) {
			if (strstr(line, "Payload delivered") != NULL) {
				found = 1;
				break;
			}
		}
		fclose(fp);

		if (found) {
			TEST_PASS("LTP Bundle send/receive test");
			LOG_INFO("Bundle successfully delivered via LTP!");
			/* Display the actual output for verification */
			LOG_INFO("bpsink output:");
			if (system("cat ./bpsink_ltp_output.txt 2>/dev/null"))
			{
				/* ignored */
			}
		} else {
			TEST_FAIL("LTP Bundle Transfer", "Bundle not received");
			LOG_INFO("bpsink output (for debugging):");
			if (system("cat ./bpsink_ltp_output.txt 2>/dev/null"))
			{
				/* ignored */
			}
		}
	}

	printf("\n========================================\n");
	printf("  LTP Tests Complete\n");
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
