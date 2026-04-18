/*
 * ion_test_utils.c
 *
 * Implementation of shared test utilities and helper functions for ION API tests
 */

#include "ion_test_utils.h"
#include "rfx.h"

/*
 * =============================================================================
 * Process Management Utilities
 * =============================================================================
 */

/**
 * Terminates a process by name, ensuring it has stopped.
 * return 0 on success, -1 on timeout.
 */
int ensure_process_terminated(const char *process_name)
{
	char command[256];
	int status;

	printf("Attempting to gracefully terminate '%s' with SIGINT...\n", process_name);

	/* 1. Signal Gracefully (using SIGINT) */
	sprintf(command, "pkill -INT %s 2>/dev/null", process_name);
	if(system(command)) { /* ignored */ }

	/* 2. Poll for termination with a 5-second timeout */
	int max_retries = 50; /* 50 * 100ms = 5 seconds */
	for (int i = 0; i < max_retries; i++)
	{
		sprintf(command, "pgrep %s >/dev/null 2>&1", process_name);
		status = system(command);

		/* Check if pgrep returned 1 (no process found)
		 * system() returns exit_code << 8, so 256 means exit code 1 */
		if (status == 256 || status == -1)
		{
			printf("'%s' has terminated gracefully.\n", process_name);
			return 0; /* Success! Process is gone. */
		}

		/* Wait for 1 second before checking again */
		sleep(1);
	}

	/* 3. If the loop finishes, the process is still alive. Escalate. */
	printf("'%s' did not terminate gracefully. Forcing termination with SIGKILL...\n", process_name);
	sprintf(command, "pkill -KILL %s 2>/dev/null", process_name);
	if (system(command)) { /* ignored */ }

	/* Give it a moment to die */
	sleep(1);

	/* Final check */
	sprintf(command, "pgrep %s >/dev/null 2>&1", process_name);
	status = system(command);
	if (status == 256 || status == -1)
	{
		printf("'%s' has been successfully terminated.\n", process_name);
		return 0; /* Success! */
	}
	else
	{
		fprintf(stderr, "Error: Failed to terminate '%s'.\n", process_name);
		return -1; /* Failure */
	}
}

int waitForBpAgent(int maxWaitSec)
{
	int	waited = 0;

	LOG_INFO("Waiting for BP agent to start (max %d seconds)...", maxWaitSec);
	while (waited < maxWaitSec)
	{
		if (bp_agent_is_started())
		{
			LOG_INFO("BP agent started after %d seconds", waited);
			return 1;
		}

		sleep(1);
		waited++;
	}

	return 0;
}

int processIsRunning(const char *processName)
{
	char	cmd[256];
	FILE	*fp;
	char	line[256];
	int	found = 0;

	snprintf(cmd, sizeof(cmd), "ps aux | grep '%s' | grep -v grep",
			processName);
	fp = popen(cmd, "r");
	if (fp != NULL)
	{
		if (fgets(line, sizeof(line), fp) != NULL)
		{
			found = 1;
		}

		pclose(fp);
	}

	return found;
}

int waitForSchemeDaemon(const char *daemonName, int maxWaitSec)
{
	int	waited = 0;

	LOG_INFO("Waiting for %s daemon to initialize (max %d seconds)...",
			daemonName, maxWaitSec);
	while (waited < maxWaitSec)
	{
		if (processIsRunning(daemonName))
		{
			/* Found the process, give it a bit more time to initialize */
			sleep(1);
			LOG_INFO("%s daemon ready after %d seconds", daemonName,
					waited + 1);
			return 1;
		}

		sleep(1);
		waited++;
	}

	LOG_INFO("Warning: %s daemon not detected after %d seconds", daemonName,
			maxWaitSec);
	return 0;
}

/*
 * =============================================================================
 * ION Initialization and Configuration Tests
 * =============================================================================
 */

int test_ion_initialization_and_attach(uvast nodeNbr)
{
	IonParms	parms;
	int		rc;

	TEST_START("ION Initialization");
	memset(&parms, 0, sizeof(IonParms));
	parms.wmKey = 0;
	parms.wmSize = 100000000;	/*	100MB for UDP_MULTISEND span recycling.	*/
	parms.wmAddress = NULL;
	istrcpy(parms.sdrName, "ion", sizeof(parms.sdrName));
	parms.sdrWmSize = 1000000;
	parms.configFlags = SDR_IN_DRAM | SDR_BOUNDED;  /* No SDR_REVERSIBLE - avoid ionrestart */
	parms.heapWords = 250000;
	parms.heapKey = SM_NO_KEY;
	parms.logSize = 0;
	parms.logKey = SM_NO_KEY;
	istrcpy(parms.pathName, "/tmp", sizeof(parms.pathName));
	LOG_INFO("Calling ionInitialize() with node " UVAST_FIELDSPEC " (without SDR_REVERSIBLE)...", nodeNbr);
	rc = ionInitialize(&parms, nodeNbr);
	if (rc < 0)
	{
		TEST_FAIL("ionInitialize", "Failed to initialize ION");
		LOG_INFO("Hint: Run 'make ionclean' to clean up any existing ION data");
		return -1;
	}

	LOG_INFO("ionInitialize() succeeded");
	LOG_INFO("Calling ionAttach()...");
	rc = ionAttach();
	if (rc < 0)
	{
		TEST_FAIL("ionAttach", "Failed to attach to ION");
		return -1;
	}

	TEST_PASS("ION Initialization and Attach");
	return 0;
}

int test_rfx_start(void)
{
	int loopcount;

	TEST_START("Start RFX");
	LOG_INFO("Calling rfx_start()...");
	rfx_start();
	for (loopcount = 5; !rfx_system_is_started() && loopcount; loopcount--)
	{
		snooze(1);
	}
	if (!rfx_system_is_started())
	{
		TEST_FAIL("rfx_start", "RFX did not start");
		return -1;
	}
	TEST_PASS("rfx_start()");
	return 0;
}

int test_add_contact(time_t from_time, time_t to_time,
		uvast from_node, uvast to_node,
		unsigned int xmit_rate, float confidence)
{
	TEST_START("Add Contact");
	LOG_INFO("Adding contact: from=" UVAST_FIELDSPEC " to=" UVAST_FIELDSPEC
			", rate=%u bytes/sec, confidence=%.1f",
			from_node, to_node, xmit_rate, confidence);
	if (ion_add_contact(from_time, to_time, from_node, to_node,
			xmit_rate, confidence) < 0)
	{
		TEST_FAIL("ion_add_contact", "Failed to add contact");
		return -1;
	}
	TEST_PASS("ion_add_contact");

	LOG_INFO("Listing all contacts...");
	ion_list_contacts();

	return 0;
}

int test_add_range(time_t from_time, time_t to_time,
		uvast from_node, uvast to_node,
		unsigned int owlt)
{
	TEST_START("Add Range");
	LOG_INFO("Adding range: from=" UVAST_FIELDSPEC " to=" UVAST_FIELDSPEC
			", OWLT=%u seconds",
			from_node, to_node, owlt);
	if (ion_add_range(from_time, to_time, from_node, to_node, owlt) < 0)
	{
		TEST_FAIL("ion_add_range", "Failed to add range");
		return -1;
	}
	TEST_PASS("ion_add_range");

	LOG_INFO("Listing all ranges...");
	ion_list_ranges();

	return 0;
}

/*
 * =============================================================================
 * ION Security (ionsec) Tests
 * =============================================================================
 */

int test_ionsec_initialization_and_attach(void)
{
	int	rc;

	TEST_START("ION Security Initialization");
	LOG_INFO("Calling secInitialize()...");
	rc = secInitialize();
	if (rc < 0)
	{
		TEST_FAIL("secInitialize", "Failed to initialize ION security database");
		return -1;
	}

	LOG_INFO("secInitialize() succeeded");
	LOG_INFO("Calling secAttach()...");
	rc = secAttach();
	if (rc < 0)
	{
		TEST_FAIL("secAttach", "Failed to attach to ION security database");
		return -1;
	}

	TEST_PASS("ION Security Initialization and Attach (empty database)");
	return 0;
}

/*
 * =============================================================================
 * BP (Bundle Protocol) Tests
 * =============================================================================
 */

int test_bp_initialization(void)
{
	int	rc;

	TEST_START("BP Initialization");
	LOG_INFO("Calling bp_init()...");
	rc = bp_init();
	if (rc < 0)
	{
		TEST_FAIL("bp_init", "Failed to initialize BP");
		return -1;
	}

	TEST_PASS("bp_init");
	return 0;
}

int test_bp_attach(void)
{
	int	rc;

	TEST_START("BP Attach");
	LOG_INFO("Calling bp_attach()...");
	rc = bp_attach();
	if (rc < 0)
	{
		TEST_FAIL("bp_attach", "Failed to attach to BP");
		return -1;
	}

	TEST_PASS("bp_attach");
	return 0;
}

int test_ipn_initialization(void)
{
	int	rc;

	TEST_START("IPN Initialization");
	LOG_INFO("Calling ipn_init()...");
	LOG_INFO("NOTE: ipn_init() is OPTIONAL - only needed for IPN routing config");
	LOG_INFO("      Endpoints can be added BEFORE ipn_init() is called");
	rc = ipn_init();
	if (rc < 0)
	{
		TEST_FAIL("ipn_init", "Failed to initialize IPN routing database");
		return -1;
	}

	TEST_PASS("ipn_init");
	LOG_INFO("IPN routing database successfully initialized");
	return 0;
}

int test_bp_start(void)
{
	int	rc;

	TEST_START("BP Start");
	LOG_INFO("Calling bp_start()...");
	LOG_INFO("This will start: bpclock, bptransit, and ALL scheme forwarders");
	rc = bp_start();
	if (rc < 0)
	{
		TEST_FAIL("bp_start", "Failed to start BP");
		return -1;
	}

	LOG_INFO("bp_start() succeeded");
	if (!waitForBpAgent(10))
	{
		TEST_FAIL("bp_agent_is_started",
				"BP agent did not start in time");
		return -1;
	}

	LOG_INFO("BP agent is started");
	LOG_INFO("Waiting for scheme forwarders to initialize...");
	sleep(2);

	if (waitForSchemeDaemon("ipnfw", 5))
	{
		LOG_INFO("ipnfw forwarder started");
	}
	else
	{
		LOG_INFO("Warning: ipnfw daemon not detected");
	}

	TEST_PASS("BP Start and Agent Ready");
	return 0;
}

int test_add_scheme(const char *name, const char *fwdCmd, const char *admAppCmd)
{
	int	rc;
	char	testName[256];
	char	logMsg[256];

	snprintf(testName, sizeof(testName), "Add Scheme ('%s')", name);
	TEST_START(testName);
	LOG_INFO("NOTE: Schemes are configured BEFORE bp_start()");
	LOG_INFO("      bp_start() will automatically start all schemes");
	snprintf(logMsg, sizeof(logMsg),
			"Calling add_scheme('%s', '%s', '%s')...", name, fwdCmd,
			admAppCmd);
	LOG_INFO("%s", logMsg);
	rc = add_scheme((char *)(uintptr_t) name, (char *)(uintptr_t) fwdCmd,
			(char *)(uintptr_t) admAppCmd);
	if (rc <= 0)
	{
		snprintf(logMsg, sizeof(logMsg), "Failed to add '%s' scheme", name);
		TEST_FAIL("add_scheme", logMsg);
		return -1;
	}

	snprintf(logMsg, sizeof(logMsg), "add_scheme('%s')", name);
	TEST_PASS(logMsg);
	LOG_INFO("Schemes configured - forwarders will start when bp_start() is called");
	return 0;
}

int test_add_endpoint(const char *eid, BpRecvRule recvRule, const char *script)
{
	int	rc;
	char	testName[256];
	char	logMsg[256];
	char	actionName[20];

	switch (recvRule)
	{
	case EnqueueBundle:
		strcpy(actionName, "EnqueueBundle");
		break;
	case DiscardBundle:
		strcpy(actionName, "DiscardBundle");
		break;
	default:
		strcpy(actionName, "UnknownAction");
		break;
	}

	snprintf(testName, sizeof(testName), "Add Endpoint ('%s')", eid);
	TEST_START(testName);
	snprintf(logMsg, sizeof(logMsg),
			"Calling add_endpoint('%s', %s, %s)...", eid, actionName,
			(script == NULL) ? "NULL" : script);
	LOG_INFO("%s", logMsg);
	rc = add_endpoint((char *)(uintptr_t) eid, recvRule, (char *)(uintptr_t) script);
	if (rc <= 0)
	{
		snprintf(logMsg, sizeof(logMsg), "Failed to add '%s' endpoint", eid);
		TEST_FAIL("add_endpoint", logMsg);
		return -1;
	}

	snprintf(logMsg, sizeof(logMsg), "add_endpoint('%s')", eid);
	TEST_PASS(logMsg);
	return 0;
}

int test_add_protocol(const char *protocol_name, int protocol_class)
{
	int	rc;
	char	testName[256];
	char	logMsg[256];
	const char	*class_name;

	switch (protocol_class)
	{
	case 0:
		class_name = "Scheduled";
		break;
	case 1:
		class_name = "Continuous";
		break;
	case 2:
		class_name = "Best-effort";
		break;
	case 10:
		class_name = "Reliable";
		break;
	case 26:
		class_name = "Any";
		break;
	default:
		class_name = "Unknown";
		break;
	}

	snprintf(testName, sizeof(testName), "Add Protocol ('%s', class=%s)",
			protocol_name, class_name);
	TEST_START(testName);
	snprintf(logMsg, sizeof(logMsg),
			"Calling add_protocol('%s', %d)...", protocol_name,
			protocol_class);
	LOG_INFO("%s", logMsg);
	rc = add_protocol((char *)(uintptr_t) protocol_name, protocol_class);
	if (rc <= 0)
	{
		snprintf(logMsg, sizeof(logMsg), "Failed to add '%s' protocol",
				protocol_name);
		TEST_FAIL("add_protocol", logMsg);
		return -1;
	}

	snprintf(logMsg, sizeof(logMsg), "add_protocol('%s')", protocol_name);
	TEST_PASS(logMsg);
	return 0;
}

int test_add_induct(const char *protocol_name, const char *duct_name,
		const char *cli_command)
{
	int	rc;
	char	testName[256];
	char	logMsg[256];

	snprintf(testName, sizeof(testName), "Add Induct ('%s:%s')",
			protocol_name, duct_name);
	TEST_START(testName);
	snprintf(logMsg, sizeof(logMsg),
			"Calling add_induct('%s', '%s', '%s')...", protocol_name,
			duct_name, cli_command);
	LOG_INFO("%s", logMsg);
	rc = add_induct((char *)(uintptr_t) protocol_name, (char *)(uintptr_t) duct_name,
			(char *)(uintptr_t) cli_command);
	if (rc <= 0)
	{
		snprintf(logMsg, sizeof(logMsg), "Failed to add induct '%s:%s'",
				protocol_name, duct_name);
		TEST_FAIL("add_induct", logMsg);
		return -1;
	}

	snprintf(logMsg, sizeof(logMsg), "add_induct('%s:%s')", protocol_name,
			duct_name);
	TEST_PASS(logMsg);
	return 0;
}

int test_add_outduct(const char *protocol_name, const char *duct_name,
		const char *clo_command, unsigned int max_payload_length)
{
	int	rc;
	char	testName[256];
	char	logMsg[256];

	snprintf(testName, sizeof(testName), "Add Outduct ('%s:%s')",
			protocol_name, duct_name);
	TEST_START(testName);
	snprintf(logMsg, sizeof(logMsg),
			"Calling add_outduct('%s', '%s', '%s', %u)...",
			protocol_name, duct_name, clo_command, max_payload_length);
	LOG_INFO("%s", logMsg);
	rc = add_outduct((char *)(uintptr_t) protocol_name, (char *)(uintptr_t) duct_name,
			(char *)(uintptr_t) clo_command, max_payload_length);
	if (rc <= 0)
	{
		snprintf(logMsg, sizeof(logMsg), "Failed to add outduct '%s:%s'",
				protocol_name, duct_name);
		TEST_FAIL("add_outduct", logMsg);
		return -1;
	}

	snprintf(logMsg, sizeof(logMsg), "add_outduct('%s:%s')", protocol_name,
			duct_name);
	TEST_PASS(logMsg);
	return 0;
}

int test_add_plan(const char *eid, unsigned int nominal_rate)
{
	int	rc;
	char	testName[256];
	char	logMsg[256];

	snprintf(testName, sizeof(testName), "Add Plan ('%s')", eid);
	TEST_START(testName);
	snprintf(logMsg, sizeof(logMsg), "Calling add_plan('%s', %u)...",
			eid, nominal_rate);
	LOG_INFO("%s", logMsg);
	rc = add_plan((char *)(uintptr_t) eid, nominal_rate);
	if (rc <= 0)
	{
		snprintf(logMsg, sizeof(logMsg), "Failed to add plan '%s'", eid);
		TEST_FAIL("add_plan", logMsg);
		return -1;
	}

	snprintf(logMsg, sizeof(logMsg), "add_plan('%s')", eid);
	TEST_PASS(logMsg);
	return 0;
}

int test_add_planduct(const char *eid, const char *protocol_name,
                      const char *duct_name)
{
	int	rc;
	char	testName[256];
	char	logMsg[256];

	snprintf(testName, sizeof(testName), "Add Planduct ('%s' -> %s:%s)",
			eid, protocol_name, duct_name);
	TEST_START(testName);
	snprintf(logMsg, sizeof(logMsg),
			"Calling add_planduct('%s', '%s', '%s')...", eid,
			protocol_name, duct_name);
	LOG_INFO("%s", logMsg);
	rc = add_planduct((char *)(uintptr_t) eid, (char *)(uintptr_t) protocol_name,
			(char *)(uintptr_t) duct_name);
	if (rc <= 0)
	{
		snprintf(logMsg, sizeof(logMsg),
				"Failed to add planduct '%s' -> %s:%s", eid,
				protocol_name, duct_name);
		TEST_FAIL("add_planduct", logMsg);
		return -1;
	}

	snprintf(logMsg, sizeof(logMsg), "add_planduct('%s' -> %s:%s)",
			eid, protocol_name, duct_name);
	TEST_PASS(logMsg);
	return 0;
}

int test_list_protocols(void)
{
	TEST_START("List Protocols");
	LOG_INFO("Calling bp_list_protocols()...");
	bp_list_protocols();
	TEST_PASS("bp_list_protocols()");
	sleep(2);
	return 0;
}

int test_cleanup(void)
{
	int loopcount;

	TEST_START("Cleanup - ION fully shutdown and clean up");

#if defined(__sun)
	/* Skip killall on Solaris - system() hangs with active child processes */
	LOG_INFO("Skipping killall on Solaris - relying on bp_stop() and rfx_stop()");
#else
	/* Kill ionrestart FIRST - it auto-restarts components */
	LOG_INFO("Killing ionrestart daemon...");
	if (system("killall -9 ionrestart 2>/dev/null")) { /* ignored */ }
	snooze(1);
#endif

	/* bp_stop() all daemons */
	bp_stop();
	LOG_INFO("Calling bp_stop()...");
	/* Wait up to 5 seconds for shutdown */
	for (loopcount = 5; bp_agent_is_started() && loopcount; loopcount--)
	{
		snooze(1);
	}
	LOG_INFO("BP agent stopped");

	/* Stop LTP daemons (ltpclock, ltpdeliv, ltpmeter, LSO/LSI) before
	 * tearing down IPC/SDR — otherwise they keep running and crash on
	 * the next tick when sdr_begin_xn() hits a destroyed semaphore.
	 * Only call ltp_stop() if LTP was actually initialized — tests that
	 * use only BP/TCP (e.g. bp_plan_crash_recovery) never start LTP, and
	 * ltpStop() would dereference a NULL ltpvdb and segfault. */
	if (ltp_engine_is_started())
	{
		LOG_INFO("Calling ltp_stop()...");
		ltp_stop();
		snooze(2);
		LOG_INFO("LTP stopped");
	}
	else
	{
		LOG_INFO("LTP not started; skipping ltp_stop()");
	}

	/* Stop ION daemon RFX */
	rfx_stop();
	LOG_INFO("Calling rfx_stop()...");
	/* Wait up to 5 seconds for shutdown */
	for (loopcount = 5; rfx_system_is_started() && loopcount; loopcount--)
	{
		snooze(1);
	}
	LOG_INFO("RFX stopped");

	/* Give all processes extra time to fully terminate */
	LOG_INFO("Waiting for all processes to fully terminate...");
	snooze(2);

	/* Delete SDR - only after all daemons have stopped */
	LOG_INFO("Calling ionTerminate(1) to delete SDR...");
	ionTerminate(1);

	/* Shutdown IPC system */
	LOG_INFO("Calling sm_ipc_stop() to remove IPC system...");
	sm_ipc_stop();

#if defined(__sun)
	/* Skip final killall on Solaris - system() hangs with active child processes */
	LOG_INFO("Skipping final killall on Solaris");
#else
	/* Kill any remaining processes */
	LOG_INFO("Ensuring all ION processes are terminated...");
	if (system("killall -9 rfxclock bpclock ipnfw tcpcli tcpclo udpcli udpclo ltpcli ltpclo ionrestart 2>/dev/null")) { /* ignored */ }
	snooze(1);
#endif

	TEST_PASS("Cleanup ION, SDR, and IPC complete");
	return 0;
}
