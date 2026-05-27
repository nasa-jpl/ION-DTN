/*
 * ion_test_utils.h
 *
 * Shared test utilities and helper functions for ION API tests
 *
 * This header provides common test infrastructure including:
 * - Test result tracking
 * - Color output macros
 * - Process management utilities
 * - Common ION test functions
 */

#ifndef ION_TEST_UTILS_H
#define ION_TEST_UTILS_H

#include "ion.h"
#include "ion_admin.h"
#include "ionsec.h"
#include "bp.h"
#include "bp_admin.h"
#include "ltp.h"
#include "ltp_admin.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Test result tracking */
typedef struct
{
	int	passed;
	int	failed;
	int	total;
} TestResults;

/* Color codes for output */
#define COLOR_GREEN "\x1b[32m"
#define COLOR_RED "\x1b[31m"
#define COLOR_YELLOW "\x1b[33m"
#define COLOR_BLUE "\x1b[34m"
#define COLOR_RESET "\x1b[0m"

/* Utility macros */
#define TEST_START(name) \
	printf("\n" COLOR_BLUE "==> Testing: %s" COLOR_RESET "\n", name); \
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

/* Global test results - must be defined in the test program */
extern TestResults results;

/*
 * =============================================================================
 * Process Management Utilities
 * =============================================================================
 */

/**
 * Terminates a process by name, ensuring it has stopped.
 *
 * @param process_name - The name of the process to terminate.
 * @return 0 on success, -1 on timeout.
 */
int ensure_process_terminated(const char *process_name);

/**
 * Helper function to wait for BP agent to start.
 *
 * @param maxWaitSec - Maximum time to wait in seconds
 * @return 1 if started, 0 if timeout
 */
int waitForBpAgent(int maxWaitSec);

/**
 * Helper function to check if a process is running.
 *
 * @param processName - Name of the process to check
 * @return 1 if running, 0 otherwise
 */
int processIsRunning(const char *processName);

/**
 * Helper function to wait for scheme forwarder daemon.
 *
 * @param daemonName - Name of the daemon to wait for
 * @param maxWaitSec - Maximum time to wait in seconds
 * @return 1 if started, 0 if timeout
 */
int waitForSchemeDaemon(const char *daemonName, int maxWaitSec);

/*
 * =============================================================================
 * ION Initialization and Configuration Tests
 * =============================================================================
 */

/**
 * Test ION initialization and attach.
 *
 * @param nodeNbr - Node number to initialize
 * @return 0 on success, -1 on error
 */
int test_ion_initialization_and_attach(uvast nodeNbr);

/**
 * Test RFX start.
 *
 * @return 0 on success, -1 on error
 */
int test_rfx_start(void);

/**
 * Test add contact.
 *
 * @param from_time - Start time of contact
 * @param to_time - End time of contact
 * @param from_node - Source node
 * @param to_node - Destination node
 * @param xmit_rate - Transmission rate in bytes/sec
 * @param confidence - Confidence level (0.0-1.0)
 * @return 0 on success, -1 on error
 */
int test_add_contact(time_t from_time, time_t to_time,
		uvast from_node, uvast to_node,
		unsigned int xmit_rate, float confidence);

/**
 * Test add range.
 *
 * @param from_time - Start time of range
 * @param to_time - End time of range
 * @param from_node - Source node
 * @param to_node - Destination node
 * @param owlt - One-Way Light Time in seconds
 * @return 0 on success, -1 on error
 */
int test_add_range(time_t from_time, time_t to_time,
		uvast from_node, uvast to_node,
		unsigned int owlt);

/*
 * =============================================================================
 * ION Security (ionsec) Tests
 * =============================================================================
 */

/**
 * Test ION Security initialization and attach.
 *
 * @return 0 on success, -1 on error
 */
int test_ionsec_initialization_and_attach(void);

/*
 * =============================================================================
 * BP (Bundle Protocol) Tests
 * =============================================================================
 */

/**
 * Test BP initialization.
 *
 * @return 0 on success, -1 on error
 */
int test_bp_initialization(void);

/**
 * Test BP attach.
 *
 * @return 0 on success, -1 on error
 */
int test_bp_attach(void);

/**
 * Test IPN initialization (optional for routing).
 *
 * @return 0 on success, -1 on error
 */
int test_ipn_initialization(void);

/**
 * Test BP start.
 *
 * @return 0 on success, -1 on error
 */
int test_bp_start(void);

/**
 * Test add scheme.
 *
 * @param name - Scheme name (e.g., "ipn")
 * @param fwdCmd - Forwarder command
 * @param admAppCmd - Admin app command
 * @return 0 on success, -1 on error
 */
int test_add_scheme(const char *name, const char *fwdCmd, const char *admAppCmd);

/**
 * Test add endpoint.
 *
 * @param eid - Endpoint ID
 * @param recvRule - Receive rule (EnqueueBundle, DiscardBundle, etc.)
 * @param script - Optional script path (can be NULL)
 * @return 0 on success, -1 on error
 */
int test_add_endpoint(const char *eid, BpRecvRule recvRule, const char *script);

/**
 * Test add protocol.
 *
 * @param protocol_name - Protocol name (e.g., "tcp", "udp", "ltp")
 * @param protocol_class - Protocol class (0=Scheduled, 1=Continuous, etc.)
 * @return 0 on success, -1 on error
 */
int test_add_protocol(const char *protocol_name, int protocol_class);

/**
 * Test add induct.
 *
 * @param protocol_name - Protocol name
 * @param duct_name - Duct name (e.g., "localhost:4556")
 * @param cli_command - CLI command (e.g., "tcpcli")
 * @return 0 on success, -1 on error
 */
int test_add_induct(const char *protocol_name, const char *duct_name,
                    const char *cli_command);

/**
 * Test add outduct.
 *
 * @param protocol_name - Protocol name
 * @param duct_name - Duct name
 * @param clo_command - CLO command
 * @param max_payload_length - Maximum payload length (0 for default)
 * @return 0 on success, -1 on error
 */
int test_add_outduct(const char *protocol_name, const char *duct_name,
                     const char *clo_command, unsigned int max_payload_length);

/**
 * Test add plan.
 *
 * @param eid - Endpoint ID for the plan
 * @param nominal_rate - Nominal transmission rate
 * @return 0 on success, -1 on error
 */
int test_add_plan(const char *eid, unsigned int nominal_rate);

/**
 * Test add planduct.
 *
 * @param eid - Endpoint ID
 * @param protocol_name - Protocol name
 * @param duct_name - Duct name
 * @return 0 on success, -1 on error
 */
int test_add_planduct(const char *eid, const char *protocol_name,
		const char *duct_name);

/**
 * Test list protocols.
 *
 * @return 0 on success, -1 on error
 */
int test_list_protocols(void);

/**
 * Test cleanup - shutdown ION completely.
 *
 * @return 0 on success, -1 on error
 */
int test_cleanup(void);

#endif /* ION_TEST_UTILS_H */
