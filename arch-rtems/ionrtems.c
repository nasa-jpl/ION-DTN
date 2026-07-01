/*
 *  ION runtime image initialization and driver.
 *  RTEMS 6.1 ARM64 port - uses ION Admin Public API
 */

#include <bsp.h>
#include <rtems.h>
#include <rtems/version.h>
#include <rtems/error.h>
#include <rtems/shell.h>
#include <rtems/libio.h>
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
#include "cfdp.h"
#include "cfdpP.h"
#include "bputa.h"
#include <fcntl.h>
#include <unistd.h>
#ifdef ENABLE_AMS
#include <pthread.h>
#include "ams.h"
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

/*
 * Memory / footprint diagnostics
 *
 * Section sizes come from linker-emitted symbols, which differ by BSP family:
 * the modern aarch64 / riscv RTEMS linker scripts expose ready-made
 * bsp_section_*_size symbols, while the classic SPARC (leon3) linkcmds instead
 * exposes section begin/end addresses, from which printBinaryFootprint()
 * derives the sizes.  Architectures without these symbols fall back to zero
 * via the #else branch in printBinaryFootprint().
 */
#if defined(__sparc__) || defined(__x86_64__)
extern char text_start[];    /* .text begin */
extern char _rodata_start[]; /* .rodata begin */
extern char _etext[];	     /* .rodata end */
extern char _data_start[];   /* .data begin */
extern char _edata[];	     /* .data end */
extern char __bss_start[];   /* .bss begin */
extern char _end[];	     /* .bss end */
#elif defined(__aarch64__) || defined(__riscv)
extern char bsp_section_text_size[];
extern char bsp_section_rodata_size[];
extern char bsp_section_data_size[];
extern char bsp_section_bss_size[];
#endif

/*
 * IS_ENABLED(flag) -> 1 if 'flag' is #defined (the build passes
 * -Dflag for enabled options and omits it otherwise), 0 if not.
 * Lets printBuildInfo() report each compile flag on a single line.
 * (Same technique as the Linux kernel's IS_ENABLED.)
 */
#define ARG_PLACEHOLDER_1	       0,
#define TAKE_SECOND(ignored, val, ...) val
#define IS_ENABLED__(arg1_or_junk)     TAKE_SECOND(arg1_or_junk 1, 0)
#define IS_ENABLED_(flag)	       IS_ENABLED__(ARG_PLACEHOLDER_##flag)
#define IS_ENABLED(flag)	       IS_ENABLED_(flag)

/*
 * CPU architecture string, taken from the compiler's own target predefined
 * macros (so it always matches what we are built for).
 */
#if defined(__aarch64__)
#define ION_CPU_ARCH "aarch64"
#elif defined(__sparc__)
#define ION_CPU_ARCH "sparc"
#elif defined(__riscv) && (__riscv_xlen == 64)
#define ION_CPU_ARCH "riscv64"
#elif defined(__riscv)
#define ION_CPU_ARCH "riscv32"
#elif defined(__x86_64__)
#define ION_CPU_ARCH "x86_64"
#else
#define ION_CPU_ARCH "unknown-arch"
#endif

/*
 * RTEMS BSP name, from <bsp.h>'s RTEMS_BSP token (e.g. a53_lp64_qemu, leon3).
 * VNSTRING() (from ion.h) turns the token into a string.
 */
#ifdef RTEMS_BSP
#define ION_BSP_NAME VNSTRING(RTEMS_BSP)
#else
#define ION_BSP_NAME "unknown-bsp"
#endif

static void printBuildInfo(void)
{
	printf("=== ION on RTEMS %s -- BSP %s (%s) ===\n", rtems_version(),
			ION_BSP_NAME, ION_CPU_ARCH);
	printf("  Built                      : %s %s\n", __DATE__, __TIME__);
	printf("  Compiler                   : %s\n", __VERSION__);
	puts("=== Active compile flags ===");
	printf("  ENABLE_BSSP                : %d\n", IS_ENABLED(ENABLE_BSSP));
	printf("  ENABLE_DGR                 : %d\n", IS_ENABLED(ENABLE_DGR));
	printf("  ENABLE_TCPCL               : %d\n", IS_ENABLED(ENABLE_TCPCL));
	printf("  ENABLE_CFDP                : %d\n", IS_ENABLED(ENABLE_CFDP));
	printf("  ENABLE_AMS                 : %d\n", IS_ENABLED(ENABLE_AMS));
	printf("  ION_NODE_NBR               : " UVAST_FIELDSPEC "\n",
			ION_NODE_NBR);
}

static void printBinaryFootprint(void)
{
#if defined(__sparc__) || defined(__x86_64__)
	uintptr_t text = (uintptr_t) _rodata_start - (uintptr_t) text_start;
	uintptr_t rodata = (uintptr_t) _etext - (uintptr_t) _rodata_start;
	uintptr_t data = (uintptr_t) _edata - (uintptr_t) _data_start;
	uintptr_t bss = (uintptr_t) _end - (uintptr_t) __bss_start;
#elif defined(__aarch64__) || defined(__riscv)
	uintptr_t text = (uintptr_t) bsp_section_text_size;
	uintptr_t rodata = (uintptr_t) bsp_section_rodata_size;
	uintptr_t data = (uintptr_t) bsp_section_data_size;
	uintptr_t bss = (uintptr_t) bsp_section_bss_size;
#else
	uintptr_t text = 0;
	uintptr_t rodata = 0;
	uintptr_t data = 0;
	uintptr_t bss = 0;
#endif
	uintptr_t rom = text + rodata + data;
	uintptr_t sram = data + bss;

	puts("=== ION binary footprint (link-time) ===");
	puts("Note: libbsd takes ~1.6MB / ION minimal ~1.0MB / Kernel etc take 0.3MB ROM");
	puts("");
	printf("  .text   (code)         : %10" PRIuPTR " B  (%7.2f KiB)\n",
			text, text / 1024.0);
	printf("  .rodata (const data)   : %10" PRIuPTR " B  (%7.2f KiB)\n",
			rodata, rodata / 1024.0);
	printf("  .data   (init RAM)     : %10" PRIuPTR " B  (%7.2f KiB)\n",
			data, data / 1024.0);
	printf("  .bss    (uninit RAM)   : %10" PRIuPTR " B  (%7.2f KiB)\n",
			bss, bss / 1024.0);
	printf("  ROM footprint (t+r+d)  : %10" PRIuPTR " B  (%7.2f KiB)\n",
			rom, rom / 1024.0);
	printf("  Static RAM    (d+b)    : %10" PRIuPTR " B  (%7.2f KiB)\n",
			sram, sram / 1024.0);
	if (text == 0 && rodata == 0 && data == 0 && bss == 0)
	{
		puts("  (BSP did not provide bsp_section_*_size symbols)");
	}
}

static void printRuntimeMemory(const char *checkpoint)
{
	Heap_Information_block wksp;

	printf("=== Runtime memory [%s] ===\n", checkpoint);

	/*
	 * RTEMS workspace is the relevant allocation pool for comparing flag
	 * combinations - it holds tasks, semaphores, message queues, and other
	 * kernel objects that scale with the protocols compiled in.  The
	 * newlib C heap (mallinfo / malloc_info) is not provided by RTEMS 6's
	 * libc on the aarch64/a53_lp64_qemu BSP, so we omit it.
	 */
	if (rtems_workspace_get_information(&wksp))
	{
		uintptr_t used = (uintptr_t) wksp.Used.total;
		uintptr_t freeb = (uintptr_t) wksp.Free.total;
		uintptr_t largest = (uintptr_t) wksp.Free.largest;
		uintptr_t total = used + freeb;

		printf("  Workspace used         : %10" PRIuPTR
		       " B  (%7.2f KiB)\n",
				used, used / 1024.0);
		printf("  Workspace free         : %10" PRIuPTR
		       " B  (%7.2f KiB)\n",
				freeb, freeb / 1024.0);
		printf("  Workspace largest free : %10" PRIuPTR
		       " B  (%7.2f KiB)\n",
				largest, largest / 1024.0);
		printf("  Workspace total        : %10" PRIuPTR
		       " B  (%7.2f KiB)\n",
				total, total / 1024.0);
	}
	else
	{
		puts("  RTEMS workspace : <unavailable>");
	}
}

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

	/*
	 * Set the host name to the loopback address so that AMS host
	 * resolution (getNameOfHost / getAddressOfHost, used by the test MIB's
	 * config-server endpoint and the DGR transport service) resolves
	 * numerically to 127.0.0.1 -- there is no DNS resolver on this BSP.
	 */
	if (sethostname("127.0.0.1", strlen("127.0.0.1")) != 0)
	{
		puts("[?] Warning: sethostname(127.0.0.1) failed.");
	}

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
	/*
	 * 4 MB: AMS over DGR opens ~8 SAPs at ~222 KB each (dests[256]
	 * + two 64 KB buffers); the 1 MB BP/LTP/CFDP base is too small.
	 * Without AMS compiled in, the rest runs fine with <1MB
	 */
	parms.wmSize = 4000000;
	parms.wmAddress = NULL;
	istrcpy(parms.sdrName, "ion", sizeof(parms.sdrName));
	parms.sdrWmSize = 1000000;
	parms.configFlags = SDR_IN_DRAM | SDR_BOUNDED;
	parms.heapWords = 1000000;
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

#define CFDP_SRC_PATH "/cfdp_src.dat"
#define CFDP_DST_PATH "/cfdp_dst.dat"
#define CFDP_PAYLOAD  "Hello, world via CFDP."

/*
 * Bring CFDP up: cfdpInit creates SDR state, cfdpAttach wires this process to
 * it, cfdpStart spawns cfdpclock and the BP UT-adapter (bputa) via
 * pseudoshell.  bputa ships CFDP PDUs as bundles to endpoint ipn:N.64 (already
 * provisioned in startDTN).
 */
static int startCfdp(void)
{
	int count;

	if (cfdpInit() < 0)
	{
		writeMemo("[?] CFDP init failed.");
		return -1;
	}

	if (cfdpAttach() < 0)
	{
		writeMemo("[?] CFDP attach failed.");
		return -1;
	}

	if (cfdpStart("bputa") < 0)
	{
		writeMemo("[?] CFDP start failed.");
		return -1;
	}

	for (count = 5; cfdp_entity_is_started() == 0; count--)
	{
		if (count == 0)
		{
			writeMemo("[?] CFDP entity did not come up.");
			return -1;
		}
		snooze(1);
	}

	puts("CFDP entity is running (bputa UTA active).");
	return 0;
}

static int writeSourceFile(void)
{
	int fd;
	int len = (int) strlen(CFDP_PAYLOAD);

	puts("Opening CFDP source file...");
	fflush(stdout);
	fd = iopen(CFDP_SRC_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0)
	{
		putSysErrmsg("Can't create CFDP source file", CFDP_SRC_PATH);
		return -1;
	}

	puts("Writing CFDP source file...");
	fflush(stdout);
	if (write(fd, CFDP_PAYLOAD, len) != len)
	{
		close(fd);
		writeMemo("[?] Short write to CFDP source file.");
		return -1;
	}

	close(fd);
	puts("CFDP source file written.");
	return 0;
}

static int verifyDestFile(void)
{
	int  fd;
	int  len = (int) strlen(CFDP_PAYLOAD);
	char buf[64];
	int  got;

	fd = iopen(CFDP_DST_PATH, O_RDONLY, 0);
	if (fd < 0)
	{
		return -1; /* Not delivered yet. */
	}

	got = read(fd, buf, sizeof buf - 1);
	close(fd);
	if (got != len)
	{
		return -1;
	}

	buf[got] = '\0';
	return memcmp(buf, CFDP_PAYLOAD, len) == 0 ? 0 : -1;
}

static void testCfdp(void)
{
	BpUtParms	  utParms;
	CfdpNumber	  destEntity;
	CfdpTransactionId tid;
	int		  count;

	puts("Starting CFDP loopback test.");
	if (startCfdp() < 0)
	{
		return;
	}

	/* Create the CFDP source file on the IMFS root filesystem. */
	if (writeSourceFile() < 0)
	{
		return;
	}

	memset(&utParms, 0, sizeof utParms);
	utParms.lifespan = 300; /* 300 sec TTL. */
	utParms.classOfService = BP_STD_PRIORITY;
	utParms.custodySwitch = NoCustodyRequested;
	cfdp_compress_number(&destEntity, ION_NODE_NBR);

	puts("Issuing cfdp_put...");
	fflush(stdout);
	if (cfdp_put(&destEntity, sizeof utParms, (unsigned char *) &utParms,
			    CFDP_SRC_PATH, CFDP_DST_PATH, NULL, NULL, NULL, 0,
			    NULL, 0, 0, 0, &tid)
			< 0)
	{
		writeMemo("[?] cfdp_put failed.");
		return;
	}
	puts("cfdp_put accepted.");
	fflush(stdout);

	/*
	 * Poll the destination path; loopback over LTP/UDP on a single QEMU
	 * CPU finishes well under a second, but allow up to 10 s for slow
	 * simulator builds.
	 */
	for (count = 0; count < 10; count++)
	{
		snooze(1);
		if (verifyDestFile() == 0)
		{
			puts("CFDP delivered: '" CFDP_PAYLOAD "'");
			puts("CFDP loopback test ended.");
			return;
		}
	}

	writeMemo("[?] CFDP destination file did not arrive.");
	puts("CFDP loopback test ended.");
}

#ifdef ENABLE_AMS

#define AMS_SUBJECT_TEXT 1
#define AMS_PAYLOAD	 "Hello, world via AMS."

/*
 * Single-process AMS loopback demo (the multi-process amshello.c adapted to
 * threads).  amsd provides the config server + registrar; a catcher module
 * invites subject 1 ("text") and a pitcher module publishes one message to it
 * over the DGR transport service.
 */

static volatile int amsCatchDone;
static volatile int amsCatchOk;

static void *amsCatcher(void *arg)
{
	AmsModule     me;
	AmsEvent      evt;
	short	      cn, sn;
	int	      zn, nn, len, ct, pr;
	unsigned char fl;
	AmsMsgType    mt;
	char	     *txt;

	(void) arg;
	if (ams_register("@", NULL, "amsdemo", "test", "", "catch", &me) < 0)
	{
		writeMemo("[?] AMS catcher can't register.");
		amsCatchDone = 1;
		return NULL;
	}

	/* Invite subject 1 from any role/unit/continuum. */
	if (ams_invite(me, 0, 0, 0, AMS_SUBJECT_TEXT, 8, 0, AmsArrivalOrder,
			    AmsAssured)
			< 0)
	{
		writeMemo("[?] AMS catcher can't invite subject 1.");
		ams_unregister(me);
		amsCatchDone = 1;
		return NULL;
	}

	puts("AMS catcher registered; waiting for message...");
	fflush(stdout);

	while (1)
	{
		if (ams_get_event(me, AMS_BLOCKING, &evt) < 0)
		{
			break;
		}

		if (ams_get_event_type(evt) == AMS_MSG_EVT)
		{
			ams_parse_msg(evt, &cn, &zn, &nn, &sn, &len, &txt, &ct,
					&mt, &pr, &fl);
			printf("AMS catcher received: '%s'\n", txt);
			fflush(stdout);
			if (strcmp(txt, AMS_PAYLOAD) == 0)
			{
				amsCatchOk = 1;
			}

			ams_recycle_event(evt);
			break;
		}

		ams_recycle_event(evt);
	}

	ams_unregister(me);
	amsCatchDone = 1;
	return NULL;
}

static void *amsPitcher(void *arg)
{
	AmsModule     me;
	AmsEvent      evt;
	AmsStateType  state;
	AmsChangeType change;
	short	      sn, dcn;
	int	      zn, nn, rn, dzn, pr;
	unsigned char fl;
	AmsSequence   sequence;
	AmsDiligence  diligence;
	int	      textlen = strlen(AMS_PAYLOAD) + 1;

	(void) arg;
	if (ams_register("@", NULL, "amsdemo", "test", "", "pitch", &me) < 0)
	{
		writeMemo("[?] AMS pitcher can't register.");
		return NULL;
	}

	while (1)
	{
		if (ams_get_event(me, AMS_BLOCKING, &evt) < 0)
		{
			break;
		}

		ams_parse_notice(evt, &state, &change, &zn, &nn, &rn, &dcn,
				&dzn, &sn, &pr, &fl, &sequence, &diligence);
		ams_recycle_event(evt);

		/*
		 * Send once the catcher's invitation on subject 1 has
		 * propagated to us.
		 */
		if (state == AmsInvitationState && sn == AMS_SUBJECT_TEXT)
		{
			printf("AMS pitcher sending:  '%s'\n", AMS_PAYLOAD);
			fflush(stdout);
			ams_send(me, -1, zn, nn, AMS_SUBJECT_TEXT, 0, 0,
					textlen, AMS_PAYLOAD, 0);
			break;
		}
	}

	snooze(1); /* Let the message drain. */
	ams_unregister(me);
	return NULL;
}

static int spawnAmsThread(pthread_t *thread, void *(*fn)(void *) )
{
	pthread_attr_t attr;
	int	       result;

	/*
	 * AMS registration walks a deep call chain; give each module thread a
	 * generous stack to avoid overflow.
	 */
	if (pthread_attr_init(&attr) != 0)
	{
		return -1;
	}

	oK(pthread_attr_setstacksize(&attr, 64 * 1024));
	result = pthread_create(thread, &attr, fn, NULL);
	oK(pthread_attr_destroy(&attr));
	return result == 0 ? 0 : -1;
}

static void testAms(void)
{
	pthread_t catchThread;
	pthread_t pitchThread;
	int	  count;

	puts("Starting AMS pitch/catch loopback test.");

	/*
	 * Start the config server + registrar using the built-in test MIB
	 * ("@", DGR primary transport).  The '' argument is the (empty) root
	 * unit name, which makes the registrar required.
	 */
	pseudoshell("amsd @ @ amsdemo test ''");
	snooze(3); /* Allow CS + RS to come up. */

	amsCatchDone = 0;
	amsCatchOk = 0;

	if (spawnAmsThread(&catchThread, amsCatcher) < 0)
	{
		writeMemo("[?] Can't spawn AMS catcher thread.");
		puts("AMS loopback test ended.");
		return;
	}

	snooze(2); /* Let the catcher invite first. */

	if (spawnAmsThread(&pitchThread, amsPitcher) < 0)
	{
		writeMemo("[?] Can't spawn AMS pitcher thread.");
		oK(pthread_join(catchThread, NULL));
		puts("AMS loopback test ended.");
		return;
	}

	oK(pthread_join(pitchThread, NULL));

	for (count = 0; count < 10 && amsCatchDone == 0; count++)
	{
		snooze(1);
	}

	oK(pthread_join(catchThread, NULL));

	if (amsCatchOk)
	{
		puts("AMS delivered: '" AMS_PAYLOAD "'");
	}
	else
	{
		writeMemo("[?] AMS message not delivered.");
	}

	puts("AMS loopback test ended.");
}
#endif /* ENABLE_AMS */

static int	stopDTN(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
			saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	if (cfdp_entity_is_started())
	{
		puts("Stopping CFDP...");
		cfdpStop();
	}

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
	printBuildInfo();
	printBinaryFootprint();

	printRuntimeMemory("after boot, before init");

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

	testCfdp();
	snooze(2);

#ifdef ENABLE_AMS
	testAms();
	snooze(2);
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
	isprintf(buffer, sizeof buffer, "[i] Delta from UTC revised, is now %d.", delta);
	writeMemo(buffer);
}

void	showUtcDelta()
{
	IonVdb	*ionvdb = getIonVdb();
	char	buffer[80];

	isprintf(buffer, sizeof buffer, "[i] Delta from UTC is %d.", ionvdb->deltaFromUTC);
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

/*
 * The init task runs the entire ION bring-up (startDTN) synchronously on its
 * own stack, through a deep ION-admin call chain with large stack locals.
 * This overflows the RTEMS default minimum init task stack and silently
 * clobbers adjacent memory -- in particular the task's libio root location,
 * after which every filesystem op (stat/open/...) returns ENXIO.
 */
#define CONFIGURE_INIT_TASK_STACK_SIZE (128 * 1024)

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

/*
 * AMS with the DGR adds a lot of threads to the build.
 * These caps must stay >= the platform_sm.c MAX_POSIX_TASKS (100) set in the
 * wscript. Without AMS/DGR the values of 40/100 are sufficient for the test
 */
#define CONFIGURE_MAXIMUM_POSIX_THREADS				128
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
