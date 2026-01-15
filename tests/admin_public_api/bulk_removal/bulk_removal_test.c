/*
 * bulk_removal_test.c
 *
 * Test program for ION Bulk Removal APIs
 * Tests the following functions for runtime reconfiguration:
 * - ltp_remove_all_spans()
 * - ltp_remove_all_seats()
 * - bp_remove_all_endpoints()
 * - bp_remove_all_inducts()
 * - bp_remove_all_outducts()
 * - ipn_remove_all_plans()
 *
 * Test Sequence:
 * 1. Full ION/LTP/BP setup with multiple items of each type
 * 2. Test each bulk removal function
 * 3. Verify items were removed using list functions
 *
 * Compile:
 * gcc -o dotest bulk_removal_test.c ../ltp_loopback/ion_test_utils.c \
 *     -lici -lbp -lltp -lpthread -lm -lrt
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
	sleep(3);
	return 0;
}

/* Test: ltp_remove_all_spans() */
static int test_ltp_remove_all_spans(int expected_count)
{
	int	result;
	char	msg[128];

	TEST_START("ltp_remove_all_spans()");
	LOG_INFO("Removing all LTP spans...");
	result = ltp_remove_all_spans();
	if (result < 0)
	{
		TEST_FAIL("ltp_remove_all_spans", "Fatal error");
		return -1;
	}
	snprintf(msg, sizeof(msg), "Removed %d spans (expected %d)",
			result, expected_count);
	LOG_INFO("%s", msg);
	if (result == expected_count)
	{
		TEST_PASS("ltp_remove_all_spans");
		return 0;
	}
	else
	{
		snprintf(msg, sizeof(msg), "Removed %d, expected %d",
				result, expected_count);
		TEST_FAIL("ltp_remove_all_spans", msg);
		return -1;
	}
}

/* Test: ltp_remove_all_seats() */
static int test_ltp_remove_all_seats(int expected_count)
{
	int	result;
	char	msg[128];

	TEST_START("ltp_remove_all_seats()");
	LOG_INFO("Removing all LTP seats...");
	result = ltp_remove_all_seats();
	if (result < 0)
	{
		TEST_FAIL("ltp_remove_all_seats", "Fatal error");
		return -1;
	}
	snprintf(msg, sizeof(msg), "Removed %d seats (expected %d)",
			result, expected_count);
	LOG_INFO("%s", msg);
	if (result == expected_count)
	{
		TEST_PASS("ltp_remove_all_seats");
		return 0;
	}
	else
	{
		snprintf(msg, sizeof(msg), "Removed %d, expected %d",
				result, expected_count);
		TEST_FAIL("ltp_remove_all_seats", msg);
		return -1;
	}
}

/* Test: bp_remove_all_endpoints() */
static int test_bp_remove_all_endpoints(char *scheme, int expected_count)
{
	int	result;
	char	msg[128];

	TEST_START("bp_remove_all_endpoints()");
	LOG_INFO("Removing all %s endpoints...", scheme);
	result = bp_remove_all_endpoints(scheme);
	if (result < 0)
	{
		TEST_FAIL("bp_remove_all_endpoints", "Fatal error");
		return -1;
	}
	snprintf(msg, sizeof(msg), "Removed %d endpoints (expected %d)",
			result, expected_count);
	LOG_INFO("%s", msg);
	if (result == expected_count)
	{
		TEST_PASS("bp_remove_all_endpoints");
		return 0;
	}
	else
	{
		snprintf(msg, sizeof(msg), "Removed %d, expected %d",
				result, expected_count);
		TEST_FAIL("bp_remove_all_endpoints", msg);
		return -1;
	}
}

/* Test: bp_remove_all_inducts() */
static int test_bp_remove_all_inducts(char *protocol, int expected_count)
{
	int	result;
	char	msg[128];

	TEST_START("bp_remove_all_inducts()");
	LOG_INFO("Removing all %s inducts...", protocol);
	result = bp_remove_all_inducts(protocol);
	if (result < 0)
	{
		TEST_FAIL("bp_remove_all_inducts", "Fatal error");
		return -1;
	}
	snprintf(msg, sizeof(msg), "Removed %d inducts (expected %d)",
			result, expected_count);
	LOG_INFO("%s", msg);
	if (result == expected_count)
	{
		TEST_PASS("bp_remove_all_inducts");
		return 0;
	}
	else
	{
		snprintf(msg, sizeof(msg), "Removed %d, expected %d",
				result, expected_count);
		TEST_FAIL("bp_remove_all_inducts", msg);
		return -1;
	}
}

/* Test: bp_remove_all_outducts() */
static int test_bp_remove_all_outducts(char *protocol, int expected_count)
{
	int	result;
	char	msg[128];

	TEST_START("bp_remove_all_outducts()");
	LOG_INFO("Removing all %s outducts...", protocol);
	result = bp_remove_all_outducts(protocol);
	if (result < 0)
	{
		TEST_FAIL("bp_remove_all_outducts", "Fatal error");
		return -1;
	}
	snprintf(msg, sizeof(msg), "Removed %d outducts (expected %d)",
			result, expected_count);
	LOG_INFO("%s", msg);
	if (result == expected_count)
	{
		TEST_PASS("bp_remove_all_outducts");
		return 0;
	}
	else
	{
		snprintf(msg, sizeof(msg), "Removed %d, expected %d",
				result, expected_count);
		TEST_FAIL("bp_remove_all_outducts", msg);
		return -1;
	}
}

/* Test: ipn_remove_all_plans() */
static int test_ipn_remove_all_plans(int expected_count)
{
	int	result;
	char	msg[128];

	TEST_START("ipn_remove_all_plans()");
	LOG_INFO("Removing all IPN plans...");
	result = ipn_remove_all_plans();
	if (result < 0)
	{
		TEST_FAIL("ipn_remove_all_plans", "Fatal error");
		return -1;
	}
	snprintf(msg, sizeof(msg), "Removed %d plans (expected %d)",
			result, expected_count);
	LOG_INFO("%s", msg);
	if (result == expected_count)
	{
		TEST_PASS("ipn_remove_all_plans");
		return 0;
	}
	else
	{
		snprintf(msg, sizeof(msg), "Removed %d, expected %d",
				result, expected_count);
		TEST_FAIL("ipn_remove_all_plans", msg);
		return -1;
	}
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
	printf("  ION Bulk Removal API Test Suite\n");
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

	/* Step 5: Configure LTP engine with MULTIPLE spans and seats */
	printf("\n--- Step 5: Configure LTP Engine (Multiple Spans/Seats) ---\n");

	/* Add 2 spans */
	if (test_add_ltp_span(1, 100, 100, 1400, 10000, 1,
			"udplso localhost:1113", 1, 0) < 0)
	{
		goto cleanup;
	}

	if (test_add_ltp_span(2, 100, 100, 1400, 10000, 1,
			"udplso localhost:1114", 1, 0) < 0)
	{
		goto cleanup;
	}

	/* Add 2 seats */
	if (test_add_ltp_seat("udplsi localhost:1113") < 0)
	{
		goto cleanup;
	}

	if (test_add_ltp_seat("udplsi localhost:1114") < 0)
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

	/* Step 8: Configure BP scheme and MULTIPLE endpoints */
	printf("\n--- Step 8: Configure BP Scheme and Endpoints ---\n");
	if (test_add_scheme("ipn", "ipnfw", "ipnadminep") < 0)
	{
		goto cleanup;
	}

	/* Add 3 endpoints */
	if (test_add_endpoint("ipn:1.1", EnqueueBundle, NULL) < 0)
	{
		goto cleanup;
	}

	if (test_add_endpoint("ipn:1.2", EnqueueBundle, NULL) < 0)
	{
		goto cleanup;
	}

	if (test_add_endpoint("ipn:1.3", EnqueueBundle, NULL) < 0)
	{
		goto cleanup;
	}

	/* Step 9: Add LTP protocol and MULTIPLE ducts */
	printf("\n--- Step 9: Add LTP Protocol Configuration ---\n");
	if (test_add_protocol("ltp", 0) < 0)  /* 0 = Scheduled */
	{
		goto cleanup;
	}

	/* Add 2 inducts */
	if (test_add_induct("ltp", "1", "ltpcli") < 0)
	{
		goto cleanup;
	}

	if (test_add_induct("ltp", "2", "ltpcli") < 0)
	{
		goto cleanup;
	}

	/* Add 2 outducts */
	if (test_add_outduct("ltp", "1", "ltpclo", 0) < 0)
	{
		goto cleanup;
	}

	if (test_add_outduct("ltp", "2", "ltpclo", 0) < 0)
	{
		goto cleanup;
	}

	/* Add 2 plans */
	if (test_add_plan("ipn:1.0", 0) < 0)
	{
		goto cleanup;
	}

	if (test_add_plan("ipn:2.0", 0) < 0)
	{
		goto cleanup;
	}

	/* Attach outducts to plans */
	if (test_add_planduct("ipn:1.0", "ltp", "1") < 0)
	{
		goto cleanup;
	}

	if (test_add_planduct("ipn:2.0", "ltp", "2") < 0)
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
	 * Bulk Removal Tests
	 * =================================================================
	 */

	printf("\n========================================\n");
	printf("  Testing Bulk Removal Functions\n");
	printf("========================================\n");

	/* Display current configuration before removal */
	printf("\n--- Current Configuration Before Removal ---\n");
	LOG_INFO("LTP Spans:");
	ltp_list_spans();
	printf("\n");
	LOG_INFO("LTP Seats:");
	ltp_list_seats();
	printf("\n");
	LOG_INFO("BP Endpoints:");
	bp_list_endpoints();
	printf("\n");

	/* Stop BP first to allow safe removal */
	printf("\n--- Stopping BP for safe removal ---\n");
	LOG_INFO("Calling bp_stop()...");
	bp_stop();
	sleep(2);

	/* Test 1: Remove all LTP seats */
	printf("\n--- Test 1: ltp_remove_all_seats() ---\n");
	if (test_ltp_remove_all_seats(2) < 0)
	{
		LOG_INFO("Note: Seats may still be in use. Continuing...");
	}

	/* Test 2: Remove all LTP spans */
	printf("\n--- Test 2: ltp_remove_all_spans() ---\n");
	if (test_ltp_remove_all_spans(2) < 0)
	{
		LOG_INFO("Note: Spans may have pending data. Continuing...");
	}

	/* Test 3: Remove all IPN plans
	 * Note: Must remove before outducts since outducts are attached to plans */
	printf("\n--- Test 3: ipn_remove_all_plans() ---\n");
	if (test_ipn_remove_all_plans(2) < 0)
	{
		LOG_INFO("Note: Plans may have pending bundles. Continuing...");
	}

	/* Test 4: Remove all LTP outducts */
	printf("\n--- Test 4: bp_remove_all_outducts() ---\n");
	if (test_bp_remove_all_outducts("ltp", 2) < 0)
	{
		LOG_INFO("Note: Outducts may still be attached. Continuing...");
	}

	/* Test 5: Remove all LTP inducts */
	printf("\n--- Test 5: bp_remove_all_inducts() ---\n");
	if (test_bp_remove_all_inducts("ltp", 2) < 0)
	{
		LOG_INFO("Note: Inducts may still be in use. Continuing...");
	}

	/* Test 6: Remove all IPN endpoints
	 * Note: Expected count is 4, not 3, because add_scheme() automatically
	 * creates an administrative endpoint (ipn:1.0) in addition to the 3
	 * endpoints we explicitly added (ipn:1.1, ipn:1.2, ipn:1.3) */
	printf("\n--- Test 6: bp_remove_all_endpoints() ---\n");
	if (test_bp_remove_all_endpoints("ipn", 4) < 0)
	{
		LOG_INFO("Note: Endpoints may have pending bundles. Continuing...");
	}

	/* Display configuration after removal */
	printf("\n--- Configuration After Removal ---\n");
	LOG_INFO("LTP Spans:");
	ltp_list_spans();
	printf("\n");
	LOG_INFO("LTP Seats:");
	ltp_list_seats();
	printf("\n");
	LOG_INFO("BP Endpoints:");
	bp_list_endpoints();
	printf("\n");

	printf("\n========================================\n");
	printf("  Bulk Removal Tests Complete\n");
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
