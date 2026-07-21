/*

	psmwatch.c:	PSM memory activity monitor.

									*/
/*									*/
/*	Copyright (c) 2007, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include "memmgr.h"
#include "platform.h"
#include "psm.h"
#include "ion.h"

#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

/*	Logger function to restore stdout logging after ionAttach()
 *	redirects memos to ion.log.					*/

static void	logToStdoutLocal(char *text)
{
	if (text)
	{
		fprintf(stdout, "%s\n", text);
		fflush(stdout);
	}
}

#define DAEMON_POLL_INTERVAL_SEC	10
#define DEFAULT_REPORT_INTERVAL_MIN	10
#define DEFAULT_PERCENT_THRESHOLD	5
#define DEFAULT_WARN_PERCENT		90.0
#define DEFAULT_WARN_CADENCE_SEC	60

static unsigned int psmwatch_count(int *newValue)
{
	static unsigned int count = 1;

	if (newValue)
	{
		if (*newValue == 0) /*	Decrement.		*/
		{
			count--;
		}
		else /*	Initialize.		*/
		{
			count = *newValue;
		}
	}

	return count;
}

static void handleQuit(int signum)
{
	/* Tell the compiler that we are not using 'signum' */
	(void)signum;

	int newCount = 1; /*	Advance to end of last cycle.	*/

	PUTS("[Terminated by user.]");
	oK(psmwatch_count(&newCount));
}

/*	Daemon mode state variables.					*/

static volatile int	daemonRunning = 1;
static double		lastReportedPct = -1.0;
static time_t		lastReportTime = 0;

/*	High-water warning state variables.				*/

static int		psmWarnActive = 0;
static time_t		lastPsmWarnTime = 0;

static void	handleDaemonQuit(int signum)
{
	(void)signum;
	daemonRunning = 0;
}

static int	daemonize(void)
{
	pid_t	pid;

	pid = fork();
	if (pid < 0)
	{
		return -1;
	}

	if (pid > 0)
	{
		exit(0);	/*	Parent exits.			*/
	}

	if (setsid() < 0)
	{
		return -1;
	}

	/*	Second fork to prevent acquiring a controlling terminal.*/

	pid = fork();
	if (pid < 0)
	{
		return -1;
	}

	if (pid > 0)
	{
		exit(0);
	}

	/*	Close standard file descriptors and redirect to /dev/null.*/

	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
	oK(open("/dev/null", O_RDONLY));	/*	stdin		*/
	oK(open("/dev/null", O_WRONLY));	/*	stdout		*/
	oK(open("/dev/null", O_WRONLY));	/*	stderr		*/

	return 0;
}

static double	calculatePsmUsagePct(PsmPartition psm)
{
	PsmUsageSummary	summary;
	double		unavailableBytes;
	double		totalBytes;

	psm_usage(psm, &summary);
	unavailableBytes = (double)(summary.smallPoolAllocated
			+ summary.largePoolAllocated);
	totalBytes = (double)(summary.partitionSize);
	if (totalBytes <= 0)
	{
		return 0.0;
	}

	return (unavailableBytes / totalBytes) * 100.0;
}

static void	reportPsmUsage(PsmPartition psm, char *partitionName, double pct)
{
	PsmUsageSummary	summary;
	char		buf[256];
	unsigned long	unavailableBytes;

	psm_usage(psm, &summary);
	unavailableBytes = (unsigned long)(summary.smallPoolAllocated
			+ summary.largePoolAllocated);
	isprintf(buf, sizeof buf,
		"[i] psmwatch: %s usage: %.3f%% (%lu/%lu bytes)",
		partitionName, pct, unavailableBytes,
		(unsigned long) summary.partitionSize);
	writeMemo(buf);
}

static void	checkWarnThreshold(double currentPct, double warnPercent,
			int *warnActive, time_t *lastWarnTime,
			int warnCadenceSec, time_t now,
			const char *poolLabel, const char *daemonLabel,
			unsigned long usedBytes, unsigned long totalBytes)
{
	char	buf[384];

	if (warnPercent <= 0)
	{
		return;		/*	Warnings disabled.		*/
	}

	if (currentPct >= warnPercent)
	{
		/*	Above threshold: warn on first crossing or
		 *	when cadence interval has elapsed.		*/

		if (!(*warnActive)
				|| (now - *lastWarnTime) >= warnCadenceSec)
		{
			isprintf(buf, sizeof buf,
				"[!] %s: WARNING - %s usage %.1f%% "
				"exceeds %.0f%% threshold "
				"(%lu/%lu bytes)",
				daemonLabel, poolLabel,
				currentPct, warnPercent,
				usedBytes, totalBytes);
			writeMemo(buf);
			*warnActive = 1;
			*lastWarnTime = now;
		}
	}
	else if (*warnActive)
	{
		/*	Recovered: was above, now below.		*/

		isprintf(buf, sizeof buf,
			"[i] %s: %s usage recovered to %.1f%% "
			"(below %.0f%% threshold)",
			daemonLabel, poolLabel,
			currentPct, warnPercent);
		writeMemo(buf);
		*warnActive = 0;
	}
}

static int	run_psmwatch_daemon(int memKey, long memSize,
			char *partitionName, int intervalMinutes,
			double percentThreshold,
			double warnPercent, int warnCadenceSec)
{
	char		*memory = NULL;
	uaddr		smId = 0;
	PsmView		memView;
	PsmPartition	psm = &memView;
	int		memmgrIdx;
	double		currentPct;
	time_t		now;
	int		thresholdCrossed;
	int		intervalElapsed;
	PsmUsageSummary	warnSummary;
	unsigned long	usedBytes;
	char		startBuf[256];

	/*	Daemonize first, before attaching to PSM.		*/

	if (daemonize() < 0)
	{
		return -1;
	}

	/*	Register signal handlers for graceful shutdown.		*/

	isignal(SIGTERM, handleDaemonQuit);
	isignal(SIGINT, handleDaemonQuit);

	/*	Attach to ION in the daemon process.  This sets up
	 *	the logger to write to ion.log and initializes IPC.	*/

	if (ionAttach() < 0)
	{
		return -1;
	}

	/*	Register our actual PID with ION so rfx_stop() can
	 *	find us.  The PID from pseudoshell() was the
	 *	intermediate process that exited during daemonize().	*/

	ionRegisterPsmwatchPid(getpid());

	if (memmgr_open(memKey, memSize, &memory, &smId, partitionName, &psm,
			&memmgrIdx, NULL, NULL, NULL, NULL) < 0)
	{
		putErrmsg("Can't attach to psm.", NULL);
		writeErrmsgMemos();
		ionDetach();
		return -1;
	}

	/*	Log startup message.					*/

	if (warnPercent > 0)
	{
		isprintf(startBuf, sizeof startBuf,
			"[i] psmwatch daemon started (warn at %.0f%% "
			"used, cadence %ds).", warnPercent, warnCadenceSec);
		writeMemo(startBuf);
	}
	else
	{
		writeMemo("[i] psmwatch daemon started.");
	}

	/*	Initial report.						*/

	lastReportedPct = calculatePsmUsagePct(psm);
	lastReportTime = getCtime();
	reportPsmUsage(psm, partitionName, lastReportedPct);

	/*	Main daemon loop: poll every DAEMON_POLL_INTERVAL_SEC.	*/

	while (daemonRunning)
	{
		snooze(DAEMON_POLL_INTERVAL_SEC);
		if (!daemonRunning)
		{
			break;
		}

		currentPct = calculatePsmUsagePct(psm);
		now = getCtime();

		/*	Check if threshold crossed or interval elapsed.	*/

		thresholdCrossed = (int)(currentPct / percentThreshold)
				!= (int)(lastReportedPct / percentThreshold);
		intervalElapsed = (now - lastReportTime)
				>= (intervalMinutes * 60);

		if (thresholdCrossed || intervalElapsed)
		{
			reportPsmUsage(psm, partitionName, currentPct);
			lastReportedPct = currentPct;
			lastReportTime = now;
		}

		/*	Check high-water warning threshold.		*/

		if (warnPercent > 0)
		{
			psm_usage(psm, &warnSummary);
			usedBytes = (unsigned long)
				(warnSummary.smallPoolAllocated
				+ warnSummary.largePoolAllocated);
			checkWarnThreshold(currentPct, warnPercent,
				&psmWarnActive, &lastPsmWarnTime,
				warnCadenceSec, now,
				"PSM", "psmwatch", usedBytes,
				(unsigned long)
				warnSummary.partitionSize);
		}
	}

	/*	Log shutdown message.					*/

	writeMemo("[i] psmwatch daemon stopped.");
	writeErrmsgMemos();
	return 0;
}

static int run_psmwatch(int memKey, long memSize, char *partitionName,
		int interval, int verbose, int noTrace, size_t traceShmSize)
{
	char           *memory = NULL;
	uaddr           smId = 0;
	PsmView         memView;
	PsmPartition    psm = &memView;
	int             memmgrIdx;
	PsmUsageSummary psmsummary;
	int             secRemaining;
	int             decrement = 0;

	if (sm_ipc_init() < 0)
	{
		writeErrmsgMemos();
		PUTS("IPC initialization failed.");
		return 0;
	}

	if (memmgr_open(memKey, memSize, &memory, &smId, partitionName, &psm,
			    &memmgrIdx, NULL, NULL, NULL, NULL)
			< 0)
	{
		putErrmsg("Can't attach to psm.", NULL);
		writeErrmsgMemos();
		return 0;
	}

	/*	Initial state.						*/

	psm_usage(psm, &psmsummary);
	psm_report(&psmsummary);
	if (interval == 0) /*	One-time poll.			*/
	{
		return 0;
	}

	/*	Start watching trace.					*/

	if (!noTrace)
	{
		if (traceShmSize == 0)
		{
			traceShmSize = 20000000;
		}

		if (psm_start_trace(psm, traceShmSize, NULL) < 0)
		{
			putErrmsg("Can't start trace.", NULL);
			writeErrmsgMemos();
			return 0;
		}
	}

	isignal(SIGTERM, handleQuit);
	isignal(SIGINT, handleQuit);

	while (psmwatch_count(NULL) > 0)
	{
		secRemaining = interval;
		while (secRemaining > 0)
		{
			snooze(1);
			secRemaining--;
			if (!verbose && !noTrace)
			{
				psm_clear_trace(psm);
			}
		}

		if (noTrace) /* Polling without memory allocation tracing */
		{
			psm_usage(psm, &psmsummary);
			psm_report(&psmsummary);
		}
		else /* Existing trace mode */
		{
			psm_print_trace(psm, verbose);
		}
		oK(psmwatch_count(&decrement));
	}

	if (!noTrace) /*	Stop watching trace.   */
	{
		psm_stop_trace(psm);
	}

	writeErrmsgMemos();
	return 0;
}

#if !defined(ION_LWT)
static void	printUsage(void)
{
	PUTS("Usage: psmwatch [<interval> [<count> [verbose]]]");
	PUTS("   psmwatch <shared_memory_key> <memory_size> <partition_name> \
<interval> <count> [verbose]");
	PUTS("");
	PUTS("   If only interval and count are provided, memory parameters are \
auto-detected from ION configuration.");
	PUTS("");
	PUTS("   shared_memory_key: integer key, HEX or decimal");
	PUTS("   memory_size: size of shared memory segment");
	PUTS("   partition_name: name of the memory partition, 'ionwm' or 'sdrwm'");
	PUTS("   interval: polling interval in seconds; 0 or negative \
		values to poll without memory allocation tracing.");
	PUTS("   count: number of polls");
	PUTS("   verbose: enable verbose output, tracing all allocations, \
		does not remove log entries for freed blocks.");
	PUTS("");
	PUTS("Daemon mode:");
	PUTS("   psmwatch -d [<interval_minutes> [<percent_threshold> \
[<warn_pct> [<warn_cadence_sec>]]]] \
[<shared_memory_key> <memory_size> <partition_name>]");
	PUTS("   -d: run as daemon, reporting usage to ion.log");
	PUTS("   interval_minutes: reporting interval (default: 10)");
	PUTS("   percent_threshold: report when usage crosses this threshold \
(default: 5)");
	PUTS("   warn_pct: log WARNING when usage exceeds this %% \
(default: 90, 0=disable)");
	PUTS("   warn_cadence_sec: seconds between repeated warnings while \
above threshold (default: 60)");
	PUTS("   If key/size/name omitted, uses ION defaults.");
}
#endif

#if defined(ION_LWT)
int psmwatch(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5, saddr a6,
		saddr a7, saddr a8, saddr a9, saddr a10)
{
	int   memKey = 0;
	int   memSize = 0;
	char *partitionName = (char *) a3;
	int   interval = 0;
	int   count = 0;
	int   verbose = a6 ? 1 : 0;
	int   noTrace = 0;
	int   parsed_int;

	if (a1)
	{
		if (platform_parse_int((char *) a1, &parsed_int) < 0)
		{
			PUTS("[?] Invalid memKey.");
			return 1;
		}
		memKey = parsed_int;
	}

	if (a2)
	{
		if (platform_parse_int((char *) a2, &parsed_int) < 0)
		{
			PUTS("[?] Invalid memSize.");
			return 1;
		}
		memSize = parsed_int;
	}

	if (a4)
	{
		if (platform_parse_int((char *) a4, &parsed_int) < 0)
		{
			PUTS("[?] Invalid interval.");
			return 1;
		}
		interval = parsed_int;
	}

	if (a5)
	{
		if (platform_parse_int((char *) a5, &parsed_int) < 0)
		{
			PUTS("[?] Invalid count.");
			return 1;
		}
		count = parsed_int;
	}

	if (interval <= 0)
	{
		noTrace = 1;
		interval = -interval;
	}

	oK(psmwatch_count(&count));
	return run_psmwatch(memKey, memSize, partitionName, interval, verbose,
			noTrace, 0);
}
#else
int main(int argc, char **argv)
{
	int	memKey;
	long	memSize;
	char	*partitionName;
	int	interval;
	int	count;
	int	verbose = 0;
	int	noTrace = 0;
	int	intervalMinutes;
	double	percentThreshold;
	int	argIdx;
	Sdr	sdr;
	SdrObject iondbObj;
	IonDB	iondb;
	size_t	traceShmSize = 0;
	int	parsed_int;
	uvast	parsed_uvast;
	char	errMsg[256];

	/*	Check for help request.					*/

	if (argc > 1 && (strcmp(argv[1], "-h") == 0
			|| strcmp(argv[1], "--help") == 0))
	{
		printUsage();
		return 0;
	}

	/*	Check for daemon mode.					*/

	if (argc > 1 && strcmp(argv[1], "-d") == 0)
	{
		/*	Daemon mode: psmwatch -d [interval] [threshold]
		 *	[warn_pct] [warn_cadence] [key size name]	*/

		double	warnPercent = DEFAULT_WARN_PERCENT;
		int	warnCadenceSec = DEFAULT_WARN_CADENCE_SEC;

		intervalMinutes = DEFAULT_REPORT_INTERVAL_MIN;
		percentThreshold = DEFAULT_PERCENT_THRESHOLD;
		memKey = 0;
		memSize = 0;
		partitionName = "ionwm";
		argIdx = 2;

		if (argIdx < argc && argv[argIdx][0] != '-')
		{
			if (platform_parse_int(argv[argIdx], &parsed_int) < 0)
			{
				isprintf(errMsg, sizeof(errMsg), "[?] Invalid interval_minutes: %s", argv[argIdx]);
				PUTS(errMsg);
				return 1;
			}
			intervalMinutes = parsed_int;

			if (intervalMinutes < 1)
			{
				intervalMinutes = DEFAULT_REPORT_INTERVAL_MIN;
			}

			argIdx++;
		}

		if (argIdx < argc && argv[argIdx][0] != '-')
		{
			percentThreshold = strtod(argv[argIdx], NULL);
			if (percentThreshold < 0.001 || percentThreshold > 100)
			{
				percentThreshold = DEFAULT_PERCENT_THRESHOLD;
			}

			argIdx++;
		}

		if (argIdx < argc && argv[argIdx][0] != '-')
		{
			warnPercent = strtod(argv[argIdx], NULL);
			if (warnPercent < 0 || warnPercent > 100)
			{
				warnPercent = DEFAULT_WARN_PERCENT;
			}

			argIdx++;
		}

		if (argIdx < argc && argv[argIdx][0] != '-')
		{
			if (platform_parse_int(argv[argIdx], &parsed_int) < 0)
			{
				isprintf(errMsg, sizeof(errMsg), "[?] Invalid warn_cadence_sec: %s", argv[argIdx]);
				PUTS(errMsg);
				return 1;
			}
			warnCadenceSec = parsed_int;

			if (warnCadenceSec < 1)
			{
				warnCadenceSec = DEFAULT_WARN_CADENCE_SEC;
			}

			argIdx++;
		}

		/*	Check if key, size, name are provided.		*/

		if (argIdx + 2 < argc)
		{
			if (platform_parse_int(argv[argIdx], &parsed_int) < 0)
			{
				isprintf(errMsg, sizeof(errMsg), "[?] Invalid memKey: %s", argv[argIdx]);
				PUTS(errMsg);
				return 1;
			}
			memKey = parsed_int;

			if (platform_parse_uvast(argv[argIdx + 1], &parsed_uvast) < 0)
			{
				isprintf(errMsg, sizeof(errMsg), "[?] Invalid memSize. Must be >= 0: %s", argv[argIdx + 1]);
				PUTS(errMsg);
				return 1;
			}
			memSize = (long) parsed_uvast;

			partitionName = argv[argIdx + 2];
		}
		else
		{
			/*	Use ION defaults.			*/

			if (ionAttach() < 0)
			{
				putErrmsg("psmwatch daemon: can't attach \
to ION.", NULL);
				writeErrmsgMemos();
				return 1;
			}

			sdr = getIonsdr();
			iondbObj = getIonDbObject();
			CHKZERO(sdr_begin_xn(sdr));
			sdr_read(sdr, (char *) &iondb, iondbObj,
					sizeof(IonDB));
			sdr_exit_xn(sdr);

			memKey = iondb.parmcopy.wmKey;
			memSize = iondb.parmcopy.wmSize;
			partitionName = "ionwm";

			ionDetach();
		}

		return run_psmwatch_daemon(memKey, memSize, partitionName,
				intervalMinutes, percentThreshold,
				warnPercent, warnCadenceSec);
	}

	/*	Standard mode.  Determine if we need to auto-detect
	 *	memory parameters.
	 *
	 *	Legacy: psmwatch <key> <size> <name> <interval> <count> [verbose]
	 *	        (6+ args, argv[3] is non-numeric partition name)
	 *
	 *	Auto:   psmwatch [<interval> [<count> [verbose]]]
	 *	        (1-3 args, or argc>=4 but argv[3] looks numeric)	*/

	if (argc >= 6)
	{
		/*	Check if argv[3] looks like a partition name
		 *	(not a number).  If it starts with a letter,
		 *	assume legacy mode with explicit parameters.	*/

		if ((argv[3][0] >= 'a' && argv[3][0] <= 'z')
				|| (argv[3][0] >= 'A' && argv[3][0] <= 'Z'))
		{
			/*	Legacy mode with explicit parameters.	*/

			if (platform_parse_int(argv[1], &parsed_int) < 0)
			{
				isprintf(errMsg, sizeof(errMsg), "[?] Invalid memKey: %s", argv[1]);
				PUTS(errMsg);
				return 1;
			}
			memKey = parsed_int;

			if (platform_parse_uvast(argv[2], &parsed_uvast) < 0)
			{
				isprintf(errMsg, sizeof(errMsg), "[?] Invalid memSize. Must be >= 0: %s", argv[2]);
				PUTS(errMsg);
				return 1;
			}
			memSize = (long) parsed_uvast;

			partitionName = argv[3];

			if (platform_parse_int(argv[4], &parsed_int) < 0)
			{
				isprintf(errMsg, sizeof(errMsg), "[?] Invalid interval: %s", argv[4]);
				PUTS(errMsg);
				return 1;
			}
			interval = parsed_int;

			if (interval <= 0)
			{
				noTrace = 1;
				interval = -interval;
			}

			if (platform_parse_int(argv[5], &parsed_int) < 0)
			{
				isprintf(errMsg, sizeof(errMsg), "[?] Invalid count: %s", argv[5]);
				PUTS(errMsg);
				return 1;
			}
			count = parsed_int;

			if (count < 1)
			{
				count = 1;
			}

			if (argc > 6)
			{
				verbose = 1;
			}

			oK(psmwatch_count(&count));
			return run_psmwatch(memKey, memSize, partitionName,
					interval, verbose, noTrace, 0);
		}
	}

	/*	Auto-detect mode: get memory parameters from ION.	*/

	if (ionAttach() < 0)
	{
		putErrmsg("psmwatch: can't attach to ION.", NULL);
		writeErrmsgMemos();
		return 1;
	}

	sdr = getIonsdr();
	iondbObj = getIonDbObject();
	CHKZERO(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
	sdr_exit_xn(sdr);

	memKey = iondb.parmcopy.wmKey;
	memSize = iondb.parmcopy.wmSize;
	traceShmSize = iondb.parmcopy.traceShmSize;
	partitionName = "ionwm";

	ionDetach();

	/*	Restore stdout logging for standard mode.
	 *	ionAttach() redirects memos to ion.log.		*/

	setLogger(logToStdoutLocal);

	/*	Parse remaining arguments: [interval [count [verbose]]]	*/

	interval = 0;
	count = 1;

	if (argc > 1)
	{
		if (platform_parse_int(argv[1], &parsed_int) < 0)
		{
			isprintf(errMsg, sizeof(errMsg), "[?] Invalid interval: %s", argv[1]);
			PUTS(errMsg);
			return 1;
		}
		interval = parsed_int;

		if (interval <= 0)
		{
			noTrace = 1;
			interval = -interval;
		}
	}

	if (argc > 2)
	{
		if (platform_parse_int(argv[2], &parsed_int) < 0)
		{
			isprintf(errMsg, sizeof(errMsg), "[?] Invalid count: %s", argv[2]);
			PUTS(errMsg);
			return 1;
		}
		count = parsed_int;

		if (count < 1)
		{
			count = 1;
		}
	}

	if (argc > 3)
	{
		verbose = 1;
	}

	oK(psmwatch_count(&count));
	return run_psmwatch(memKey, memSize, partitionName, interval, verbose,
			noTrace, traceShmSize);
}
#endif
