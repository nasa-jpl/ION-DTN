/*
 *  ION runtime image initialization and driver.
 *  RTEMS 6.1 ARM64 port - uses ION Admin Public API
 */

#include <bsp.h>
#include <rtems.h>
#include <rtems/error.h>
#include <rtems/shell.h>
#include <assert.h>
#include <inttypes.h>
#include <sys/time.h>
#include <rtems/bsd/bsd.h>
#include "platform.h"
#include "ion.h"
#include "ionsec.h"
#include "rfx.h"
#include "ltp.h"
#include "bp.h"
#include "ion_admin.h"
#include "ltp_admin.h"
#include "bp_admin.h"
#include "ltpnm.h"
#ifndef NASA_PROTECTED_FLIGHT_CODE
#include "cfdp.h"
#endif

#define ION_NODE_NBR	  ((uvast) 19)
#define LTP_ENGINE_STR	  "19"		   /* must match ION_NODE_NBR */
#define TEST_ENDPOINT_LTP 1
#define TEST_ENDPOINT_TCP 2
#define TEST_LTP_DUCT	  "127.0.0.1:1113" /* udplsi / udplso */
#define TEST_TCP_DUCT	  "127.0.0.1:4556" /* tcpcli / tcpclo */

/*
 * Note: EnqueueBundle is an enum value in BpRecvRule, not a function.
 * We use it directly in add_endpoint() calls below.
 */

static void	initNetwork()
{
	rtems_status_code sc;
	int exit_code;

	puts("Initializing RTEMS BSD networking stack...");

	/* Initialize the BSD network stack */
	sc = rtems_bsd_initialize();
	assert(sc == RTEMS_SUCCESSFUL);

	/* Configure loopback interface (lo0) */
	exit_code = rtems_bsd_ifconfig_lo0();
	assert(exit_code == 0);

	puts("Network initialization complete (loopback interface ready).");
}

static void	initClock()
{
	rtems_time_of_day tod;
	rtems_status_code sc;

	/*
	 * Set system clock to November 7, 2025, 00:00:00 UTC
	 * This provides a realistic timestamp for bundle creation
	 * and prevents timestamp corruption that blocks bundle forwarding.
	 */
	tod.year   = 2025;
	tod.month  = 11;
	tod.day    = 7;
	tod.hour   = 0;
	tod.minute = 0;
	tod.second = 0;
	tod.ticks  = 0;

	sc = rtems_clock_set(&tod);
	if (sc != RTEMS_SUCCESSFUL)
	{
		printf("rtems_clock_set failed: %s\n", rtems_status_text(sc));
		assert(sc == RTEMS_SUCCESSFUL);
	}

	puts("System clock initialized to November 7, 2025.");
}

static int	startDTN()
{
	uvast		nodenbr = ION_NODE_NBR;
	int		count;
	time_t		now;
	IonParms	parms;

	/*	Initialize ION with public API - no config files needed	*/

	puts("Initializing ION...");
	sm_ipc_init();

	/*	Set up ION parameters for RTEMS 6.1 64-bit ARM		*/
	memset(&parms, 0, sizeof(IonParms));
	parms.wmKey = 0;			/* Auto-allocate private memory */
	parms.wmSize = 2000000;			/* 2 MB for UDP/TCP buffers */
	parms.wmAddress = NULL;
	istrcpy(parms.sdrName, "ion", sizeof(parms.sdrName));
	parms.sdrWmSize = 2000000;		/* 2 MB for UDP/TCP buffers */
	parms.configFlags = SDR_IN_DRAM | SDR_BOUNDED;
	parms.heapWords = 2000000;		/* 2 MB for UDP/TCP buffers */
	parms.heapKey = SM_NO_KEY;		/* Auto-allocate */
	parms.logSize = 0;
	parms.logKey = SM_NO_KEY;
	istrcpy(parms.pathName, "/ion", sizeof(parms.pathName));

	if (ionInitialize(&parms, nodenbr) < 0)
	{
		writeMemo("[?] ION initialization failed.");
		return -1;
	}

	if (ionAttach() < 0)
	{
		writeMemo("[?] ION attach failed.");
		return -1;
	}

	/*	Register node in region	 - REQUIRED before adding contacts	*/
	if (ion_register_node(1) < 0)
	{
		writeMemo("[?] Node registration failed.");
		return -1;
	}

	/*	Initialize ION Security						*/
	if (secInitialize() < 0)
	{
		writeMemo("[?] Security initialization failed.");
		return -1;
	}

	if (secAttach() < 0)
	{
		writeMemo("[?] Security attach failed.");
		return -1;
	}

	/*	Start RFX (ION clock daemon)					*/
	puts("Starting RFX clock...");
	if (rfx_start() < 0)
	{
		writeMemo("[?] RFX start failed.");
		return -1;
	}

	count = 5;
	while (rfx_system_is_started() == 0)
	{
		snooze(1);
		count--;
		if (count == 0)
		{
			writeMemo("[?] RFX start hung up, abandoned.");
			return -1;
		}
	}

	/*	Add contacts and ranges						*/
	now = time(NULL);
	if (ion_add_contact(now + 1, now + 7200, nodenbr, nodenbr, 100000, 1.0) < 0)
	{
		writeMemo("[?] Failed to add contact.");
		return -1;
	}

	if (ion_add_range(now + 1, now + 7200, nodenbr, nodenbr, 1) < 0)
	{
		writeMemo("[?] Failed to add range.");
		return -1;
	}

	/*	Initialize and configure LTP					*/
	puts("Initializing LTP...");
	if (ltp_init(100) < 0)  /* Estimated max sessions */
	{
		writeMemo("[?] LTP initialization failed.");
		return -1;
	}

	/*	Add LTP span using UDP for loopback		*/
	/* add_span(engine_id, max_export_sessions, max_import_sessions, max_segment_size,
	            aggr_size_limit, aggr_time_limit, lso_command, queuing_latency, purge_enabled) */
	if (add_span(nodenbr, 100, 100, 1400, 10000, 1, "udplso " TEST_LTP_DUCT,
			    1, 0) < 0)
	{
		writeMemo("[?] Failed to add LTP span.");
		return -1;
	}

	if (add_seat("udplsi " TEST_LTP_DUCT) < 0)
	{
		writeMemo("[?] Failed to add LTP seat.");
		return -1;
	}

	if (ltp_start() < 0)
	{
		writeMemo("[?] LTP start failed.");
		return -1;
	}

	count = 5;
	while (ltp_engine_is_started() == 0)
	{
		snooze(1);
		count--;
		if (count == 0)
		{
			writeMemo("[?] LTP start hung up, abandoned.");
			return -1;
		}
	}

	/*	Initialize and configure BP					*/
	puts("Initializing BP...");
	if (bp_init() < 0)
	{
		writeMemo("[?] BP initialization failed.");
		return -1;
	}

	if (bp_attach() < 0)
	{
		writeMemo("[?] BP attach failed.");
		return -1;
	}

	/*	Add IPN scheme							*/
	if (add_scheme("ipn", "ipnfw", "ipnadminep") < 0)
	{
		writeMemo("[?] Failed to add IPN scheme.");
		return -1;
	}

	/*	Add endpoints - using NULL for callback since test only	*/
	if (add_endpoint("ipn:19.0", EnqueueBundle, NULL) < 0)
	{
		writeMemo("[?] Failed to add endpoint ipn:19.0.");
		return -1;
	}

	if (add_endpoint("ipn:19.1", EnqueueBundle, NULL) < 0)
	{
		writeMemo("[?] Failed to add endpoint ipn:19.1.");
		return -1;
	}

	if (add_endpoint("ipn:19.2", EnqueueBundle, NULL) < 0)
	{
		writeMemo("[?] Failed to add endpoint ipn:19.2.");
		return -1;
	}

	if (add_endpoint("ipn:19.64", EnqueueBundle, NULL) < 0)
	{
		writeMemo("[?] Failed to add endpoint ipn:19.64.");
		return -1;
	}

	if (add_endpoint("ipn:19.65", EnqueueBundle, NULL) < 0)
	{
		writeMemo("[?] Failed to add endpoint ipn:19.65.");
		return -1;
	}

	if (add_endpoint("ipn:19.126", EnqueueBundle, NULL) < 0)
	{
		writeMemo("[?] Failed to add endpoint ipn:19.126.");
		return -1;
	}

	if (add_endpoint("ipn:19.127", EnqueueBundle, NULL) < 0)
	{
		writeMemo("[?] Failed to add endpoint ipn:19.127.");
		return -1;
	}

	/*	Add LTP protocol						*/
	if (add_protocol("ltp", 0) < 0)	/* 0 = Scheduled */
	{
		writeMemo("[?] Failed to add LTP protocol.");
		return -1;
	}

	/*	Add LTP convergence layer adapters				*/
	if (add_induct("ltp", LTP_ENGINE_STR, "ltpcli") < 0)
	{
		writeMemo("[?] Failed to add LTP induct.");
		return -1;
	}

	if (add_outduct("ltp", LTP_ENGINE_STR, "ltpclo", 0) < 0)
	{
		writeMemo("[?] Failed to add LTP outduct.");
		return -1;
	}

#ifdef ENABLE_TCPCL
	/* Add TCPCL protocol and convergence layer adapters */
	if (add_protocol("tcp", 8) < 0)
	{
		writeMemo("[?] Failed to add TCP protocol.");
		return -1;
	}

	if (add_induct("tcp", TEST_TCP_DUCT, "tcpcli") < 0)
	{
		writeMemo("[?] Failed to add TCP induct.");
		return -1;
	}

	/*
	 * tcpclo is deprecated; tcpcl outducts are drained by the
	 * tcpcli threads, so no CLO command is needed (NULL).
	 */
	if (add_outduct("tcp", TEST_TCP_DUCT, NULL, 0) < 0)
	{
		writeMemo("[?] Failed to add TCP outduct.");
		return -1;
	}
#endif

	/*	Add routing plan						*/
	if (add_plan("ipn:19.0", 0) < 0)
	{
		writeMemo("[?] Failed to add plan.");
		return -1;
	}

	if (add_planduct("ipn:19.0", "ltp", LTP_ENGINE_STR) < 0)
	{
		writeMemo("[?] Failed to add planduct.");
		return -1;
	}

	/*	Start BP							*/
	puts("Starting BP...");
	if (bp_start() < 0)
	{
		writeMemo("[?] BP start failed.");
		return -1;
	}

	count = 5;
	while (bp_agent_is_started() == 0)
	{
		snooze(1);
		count--;
		if (count == 0)
		{
			writeMemo("[?] BP start hung up, abandoned.");
			return -1;
		}
	}

	/*	Start lgagent for diagnostics					*/
	pseudoshell("lgagent ipn:19.127");
	snooze(1);

#ifndef NASA_PROTECTED_FLIGHT_CODE
	/*	CFDP is excluded in this minimal BP/LTP port			*/
#endif

	puts("ION startup complete.");
	return 0;
}

static void	printLtpSpanStats()
{
	NmltpSpan	spanStats;
	int		success = 0;
	char		buffer[256];

	ltpnm_span_get(ION_NODE_NBR, &spanStats, &success);
	if (success == 0)
	{
		writeMemo("[?] Failed to retrieve LTP span statistics.");
		return;
	}

	isprintf(buffer, sizeof buffer, "LTP Span Statistics for engine " UVAST_FIELDSPEC ":",
		ION_NODE_NBR);
	puts(buffer);
	isprintf(buffer, sizeof buffer, "  Output segments: popped=%lu bytes=%lu",
		spanStats.outputSegPoppedCount, spanStats.outputSegPoppedBytes);
	puts(buffer);
	isprintf(buffer, sizeof buffer, "  Input segments (red): count=%lu bytes=%lu",
		spanStats.inputSegRecvRedCount, spanStats.inputSegRecvRedBytes);
	puts(buffer);
	isprintf(buffer, sizeof buffer, "  Input segments (green): count=%lu bytes=%lu",
		spanStats.inputSegRecvGreenCount, spanStats.inputSegRecvGreenBytes);
	puts(buffer);
	isprintf(buffer, sizeof buffer, "  Checkpoints transmitted: %lu",
		spanStats.outputCkptXmitCount);
	puts(buffer);
	isprintf(buffer, sizeof buffer, "  Checkpoints received: %lu",
		spanStats.inputCkptRecvCount);
	puts(buffer);
	isprintf(buffer, sizeof buffer, "  Sessions: export=%lu import=%lu completed=%lu",
		spanStats.currentExportSessions, spanStats.currentImportSessions,
		spanStats.outputCompleteCount);
	puts(buffer);
}

static void testLoopback(const char *label, const char *payload,
		unsigned int sinkEp)
{
	char buf[120];

	isprintf(buf, sizeof buf, "Starting %s loopback test.", label);
	puts(buf);
	isprintf(buf, sizeof buf, "bpsink ipn:" UVAST_FIELDSPEC ".%u",
			ION_NODE_NBR, sinkEp);
	pseudoshell(buf);
	snooze(2);
	isprintf(buf, sizeof buf, "bpsource ipn:" UVAST_FIELDSPEC ".%u '%s'",
			ION_NODE_NBR, sinkEp, payload);
	pseudoshell(buf);
	snooze(5);

	/* Verify transmission success with bundle statistics */
	isprintf(buf, sizeof buf, "Verifying %s bundle transmission:", label);
	puts(buf);
	pseudoshell("bpstats");
	puts("\nBundle Protocol outduct status:");
	pseudoshell("bplist");
	snooze(1);

	isprintf(buf, sizeof buf, "%s loopback test ended.", label);
	puts(buf);
}


	puts("Loopback test ended.");
}

static int	stopDTN(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
			saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
#ifndef NASA_PROTECTED_FLIGHT_CODE
	/*	CFDP is excluded in this minimal BP/LTP port			*/
#endif

	/*	Stop BP (void function, no return value to check)		*/

	puts("Stopping BP...");
	bp_stop();

	while (bp_agent_is_started())
	{
		snooze(1);
	}

	/*	Stop LTP (void function, no return value to check)		*/

	puts("Stopping LTP...");
	ltp_stop();

	while (ltp_engine_is_started())
	{
		snooze(1);
	}

	/*	Stop rfxclock (void function, no return value to check)	*/

	puts("Stopping RFX...");
	rfx_stop();

	while (rfx_system_is_started())
	{
		snooze(1);
	}

	/*	Erase all ION data in DRAM					*/

	puts("Terminating ION...");
	ionTerminate(1);	/* 1 = delete SDR */
	sm_ipc_stop();

	return 0;
}

rtems_task	Init(rtems_task_argument ignored)
{
	puts("=== ION RTEMS 6.1 ARM64 Port - Minimal BP/LTP ===");

	/* Initialize system clock to prevent timestamp corruption */
	initClock();

	/* Initialize BSD networking stack for UDP support */
	initNetwork();

	puts("Starting ION with public API (no config files)...");

	if (startDTN() < 0)
	{
		writeMemo("[?] Can't start ION.");
		exit(1);
	}

	testLoopback("UDP/LTP", "Hello, world via LTP.", TEST_ENDPOINT_LTP);
	snooze(3);

	puts("\nLTP Protocol Layer Statistics:");
	printLtpSpanStats();
	snooze(1);

#ifdef ENABLE_TCPCL
	/* Swap egress planduct from LTP to TCP for the second test. */
	puts("\nSwitching egress planduct from ltp/" LTP_ENGINE_STR
	     " to tcp/" TEST_TCP_DUCT "...");
	if (remove_planduct("ltp", LTP_ENGINE_STR) < 0)
	{
		writeMemo("[?] Failed to remove LTP planduct.");
	}
	if (add_planduct("ipn:19.0", "tcp", TEST_TCP_DUCT) < 0)
	{
		writeMemo("[?] Failed to add TCP planduct.");
	}
	snooze(2);

	testLoopback("TCP", "Hello, world via TCPCL.", TEST_ENDPOINT_TCP);
	snooze(3);
#endif

	/*	Check statistics one more time after longer delay		*/
	puts("Final statistics check:");
	pseudoshell("bpstats");
	puts("\nFinal LTP Protocol Layer Statistics:");
	printLtpSpanStats();
	snooze(1);

	puts("Stopping ION...");
	oK(stopDTN(0, 0, 0, 0, 0, 0, 0, 0, 0, 0));
	puts("ION stopped successfully.");
	exit(0);
}

/*
 * inferUtcDelta() - Helper function for UTC time synchronization
 * Note: Currently unused in this port
 */
void	inferUtcDelta(char *correctUtcTimeStamp)
{
	IonVdb	*ionvdb = getIonVdb();
	time_t	correctUtcTime = readTimestampUTC(correctUtcTimeStamp, 0);
	time_t	clocktime = getCtime();
	int	delta = clocktime - correctUtcTime;
	char	buffer[80];

	CHKVOID(setDeltaFromUTC(delta) == 0);
	sprintf(buffer, "[i] Delta from UTC revised, is now %d.", delta);
	writeMemo(buffer);
}

void	showUtcDelta()
{
	IonVdb	*ionvdb = getIonVdb();
	char	buffer[80];

	sprintf(buffer, "[i] Delta from UTC is %d.", ionvdb->deltaFromUTC);
	writeMemo(buffer);
}

/*	*	*	RTEMS 6.1 Configuration	*	*	*	*/

#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER

#define	CONFIGURE_RTEMS_INIT_TASKS_TABLE

/*
 * SPARC (LEON3) has a separate FPU register file; tasks that touch the
 * FPU without the RTEMS_FLOATING_POINT attribute trap with
 * INTERNAL_ERROR_ILLEGAL_USE_OF_FLOATING_POINT_UNIT.
 */
#define CONFIGURE_INIT_TASK_ATTRIBUTES \
	(RTEMS_DEFAULT_ATTRIBUTES | RTEMS_FLOATING_POINT)

/*	Resource limits - adjusted for minimal BP/LTP port	*/
/*	Use unlimited objects for libbsd compatibility		*/
#define	CONFIGURE_UNLIMITED_OBJECTS
#define	CONFIGURE_UNLIMITED_ALLOCATION_SIZE			32
#define	CONFIGURE_UNIFIED_WORK_AREAS
#define	CONFIGURE_MAXIMUM_USER_EXTENSIONS			5

#ifndef CONFIGURE_MICROSECONDS_PER_TICK
#define	CONFIGURE_MICROSECONDS_PER_TICK				10000
#endif
#ifndef CONFIGURE_TICKS_PER_TIMESLICE
#define	CONFIGURE_TICKS_PER_TIMESLICE				10
#endif
#define CONFIGURE_MAXIMUM_FILE_DESCRIPTORS			40
#define CONFIGURE_USE_IMFS_AS_BASE_FILESYSTEM

#define CONFIGURE_MAXIMUM_POSIX_THREADS				40
/*	POSIX mutexes and condition variables config removed in RTEMS 6	*/
#define CONFIGURE_MAXIMUM_POSIX_SEMAPHORES			100
#define CONFIGURE_MAXIMUM_POSIX_MESSAGE_QUEUES			10

#define	CONFIGURE_STACK_CHECKER_ON
#define	CONFIGURE_ZERO_WORKSPACE_AUTOMATICALLY			TRUE

#define	CONFIGURE_DISABLE_CLASSIC_NOTEPADS

#define	CONFIGURE_INIT

#undef Object
#include <rtems/confdefs.h>

/*
 * Loopback Network Configuration
 * NOTE: Disabled for this minimal port - using POSIX message queues only
 * If actual networking is needed, enable rtems-libbsd instead of legacy bsdnet
 */
#if 0
extern int rtems_bsdnet_loopattach(struct rtems_bsdnet_ifconfig *, int);

static struct rtems_bsdnet_ifconfig	loopback_config =
{
	"lo0",				/* name */
	rtems_bsdnet_loopattach,	/* attach function */
	NULL,				/* link to next interface */
	"127.0.0.1",			/* IP address */
	"255.0.0.0",			/* IP net mask */
};

struct rtems_bsdnet_config		rtems_bsdnet_config =
{
	&loopback_config,		/* Network interface */
	NULL,				/* Use fixed network configuration */
	0,				/* Default network task priority */
	0,				/* Default mbuf capacity */
	0,				/* Default mbuf cluster capacity */
	"127.0.0.1",			/* Host name */
	"localdomain",			/* Domain name */
	"127.0.0.1",			/* Gateway */
	"127.0.0.1",			/* Log host */
	{"127.0.0.1" },			/* Name server(s) */
	{"127.0.0.1" },			/* NTP server(s) */
	2,				/* sb_efficiency */
	8192,				/* udp_tx_buf_size */
	8192,				/* udp_rx_buf_size */
	8192,				/* tcp_tx_buf_size */
	8192				/* tcp_rx_buf_size */
};
#endif
