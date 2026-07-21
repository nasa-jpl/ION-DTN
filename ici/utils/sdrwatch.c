/*

	sdrwatch.c:	SDR activity monitor.

									*/
/*									*/
/*	Copyright (c) 2005, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include "platform.h"
#include "sdr.h"
#include "ion.h"
#include "zco.h"
#include "sdrxn.h"
#include "sdrmgt.h"

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

static unsigned int	sdrwatch_count(int *newValue)
{
	static unsigned int	count = 1;

	if (newValue)
	{
		if (*newValue == 0)	/*	Decrement.		*/
		{
			count--;
		}
		else			/*	Initialize.		*/
		{
			count = *newValue;
		}
	}

	return count;
}

static void	handleQuit(int signum)
{
	/* Tell the compiler that we are not using 'signum' */
	(void)signum;

	int	newCount = 1;	/*	Advance to end of last cycle.	*/

	PUTS("[Terminated by user.]");
	oK(sdrwatch_count(&newCount));
}

/*	Daemon mode state variables.					*/

static volatile int	daemonRunning = 1;
static double		lastReportedHeapPct = -1.0;
static double		lastReportedInboundPct = -1.0;
static double		lastReportedOutboundPct = -1.0;
static time_t		lastReportTime = 0;

/*	High-water warning state variables.				*/

static int		heapWarnActive = 0;
static int		inboundWarnActive = 0;
static int		outboundWarnActive = 0;
static time_t		lastHeapWarnTime = 0;
static time_t		lastInboundWarnTime = 0;
static time_t		lastOutboundWarnTime = 0;

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

static double	calculateSdrHeapPct(Sdr sdr)
{
	SdrUsageSummary	summary;
	double		unavailableBytes;
	double		totalBytes;

	CHKZERO(sdr_begin_xn(sdr));
	sdr_usage(sdr, &summary);
	sdr_exit_xn(sdr);

	unavailableBytes = (double)(summary.smallPoolAllocated
			+ summary.largePoolAllocated);
	totalBytes = (double)(summary.heapSize);
	if (totalBytes <= 0)
	{
		return 0.0;
	}

	return (unavailableBytes / totalBytes) * 100.0;
}

static double	calculateZcoInboundPct(Sdr sdr)
{
	double	occupancy;
	double	maxOccupancy;

	CHKZERO(sdr_begin_xn(sdr));
	occupancy = zco_get_heap_occupancy(sdr, ZcoInbound);
	maxOccupancy = zco_get_max_heap_occupancy(sdr, ZcoInbound);
	sdr_exit_xn(sdr);

	if (maxOccupancy <= 0)
	{
		return 0.0;
	}

	return (occupancy / maxOccupancy) * 100.0;
}

static double	calculateZcoOutboundPct(Sdr sdr)
{
	double	occupancy;
	double	maxOccupancy;

	CHKZERO(sdr_begin_xn(sdr));
	occupancy = zco_get_heap_occupancy(sdr, ZcoOutbound);
	maxOccupancy = zco_get_max_heap_occupancy(sdr, ZcoOutbound);
	sdr_exit_xn(sdr);

	if (maxOccupancy <= 0)
	{
		return 0.0;
	}

	return (occupancy / maxOccupancy) * 100.0;
}

static void	reportSdrHeapUsage(Sdr sdr, char *sdrName, double pct)
{
	SdrUsageSummary	summary;
	char		buf[256];
	unsigned long	unavailableBytes;

	CHKVOID(sdr_begin_xn(sdr));
	sdr_usage(sdr, &summary);
	sdr_exit_xn(sdr);

	unavailableBytes = (unsigned long)(summary.smallPoolAllocated
			+ summary.largePoolAllocated);
	isprintf(buf, sizeof buf,
		"[i] sdrwatch: %s SDR heap usage: %.3f%% (%lu/%lu bytes)",
		sdrName, pct, unavailableBytes,
		(unsigned long) summary.heapSize);
	writeMemo(buf);
}

static void	reportZcoUsage(Sdr sdr, char *sdrName, double inboundPct,
			double outboundPct)
{
	char		buf[256];
	vast		inboundOccupancy;
	vast		inboundMax;
	vast		outboundOccupancy;
	vast		outboundMax;

	CHKVOID(sdr_begin_xn(sdr));
	inboundOccupancy = zco_get_heap_occupancy(sdr, ZcoInbound);
	inboundMax = zco_get_max_heap_occupancy(sdr, ZcoInbound);
	outboundOccupancy = zco_get_heap_occupancy(sdr, ZcoOutbound);
	outboundMax = zco_get_max_heap_occupancy(sdr, ZcoOutbound);
	sdr_exit_xn(sdr);

	isprintf(buf, sizeof buf,
		"[i] sdrwatch: %s ZCO heap inbound: %.3f%% (%ld/%ld), "
		"outbound: %.3f%% (%ld/%ld)",
		sdrName, inboundPct, (long) inboundOccupancy, (long) inboundMax,
		outboundPct, (long) outboundOccupancy, (long) outboundMax);
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

static int	run_sdrwatch_daemon(char *sdrName, int intervalMinutes,
			double percentThreshold,
			double warnPercent, int warnCadenceSec)
{
	Sdr		sdr;
	double		currentHeapPct;
	double		currentInboundPct;
	double		currentOutboundPct;
	time_t		now;
	int		heapThresholdCrossed;
	int		inboundThresholdCrossed;
	int		outboundThresholdCrossed;
	int		intervalElapsed;
	SdrUsageSummary	warnSummary;
	unsigned long	usedBytes;
	vast		zcoOccupancy;
	vast		zcoMax;
	char		startBuf[256];

	/*	Daemonize first, before attaching to SDR.		*/

	if (daemonize() < 0)
	{
		return -1;
	}

	/*	Register signal handlers for graceful shutdown.		*/

	isignal(SIGTERM, handleDaemonQuit);
	isignal(SIGINT, handleDaemonQuit);

	/*	Attach to ION in the daemon process.  This sets up
	 *	the logger to write to ion.log and initializes SDR.	*/

	if (ionAttach() < 0)
	{
		return -1;
	}

	/*	Register our actual PID with ION so rfx_stop() can
	 *	find us.  The PID from pseudoshell() was the
	 *	intermediate process that exited during daemonize().	*/

	ionRegisterSdrwatchPid(getpid());

	sdr = sdr_start_using(sdrName);
	if (sdr == NULL)
	{
		putErrmsg("Can't attach to sdr.", sdrName);
		writeErrmsgMemos();
		ionDetach();
		return -1;
	}

	/*	Log startup message.					*/

	if (warnPercent > 0)
	{
		isprintf(startBuf, sizeof startBuf,
			"[i] sdrwatch daemon started (warn at %.0f%% "
			"used, cadence %ds).", warnPercent, warnCadenceSec);
		writeMemo(startBuf);
	}
	else
	{
		writeMemo("[i] sdrwatch daemon started.");
	}

	/*	Initial report.						*/

	lastReportedHeapPct = calculateSdrHeapPct(sdr);
	lastReportedInboundPct = calculateZcoInboundPct(sdr);
	lastReportedOutboundPct = calculateZcoOutboundPct(sdr);
	lastReportTime = getCtime();
	reportSdrHeapUsage(sdr, sdrName, lastReportedHeapPct);
	reportZcoUsage(sdr, sdrName, lastReportedInboundPct,
			lastReportedOutboundPct);

	/*	Main daemon loop: poll every DAEMON_POLL_INTERVAL_SEC.	*/

	while (daemonRunning)
	{
		snooze(DAEMON_POLL_INTERVAL_SEC);
		if (!daemonRunning)
		{
			break;
		}

		currentHeapPct = calculateSdrHeapPct(sdr);
		currentInboundPct = calculateZcoInboundPct(sdr);
		currentOutboundPct = calculateZcoOutboundPct(sdr);
		now = getCtime();

		/*	Check if any threshold crossed or interval elapsed.*/

		heapThresholdCrossed = (int)(currentHeapPct / percentThreshold)
				!= (int)(lastReportedHeapPct / percentThreshold);
		inboundThresholdCrossed = (int)(currentInboundPct / percentThreshold)
				!= (int)(lastReportedInboundPct / percentThreshold);
		outboundThresholdCrossed = (int)(currentOutboundPct / percentThreshold)
				!= (int)(lastReportedOutboundPct / percentThreshold);
		intervalElapsed = (now - lastReportTime)
				>= (intervalMinutes * 60);

		if (heapThresholdCrossed || inboundThresholdCrossed
				|| outboundThresholdCrossed || intervalElapsed)
		{
			reportSdrHeapUsage(sdr, sdrName, currentHeapPct);
			reportZcoUsage(sdr, sdrName, currentInboundPct,
					currentOutboundPct);
			lastReportedHeapPct = currentHeapPct;
			lastReportedInboundPct = currentInboundPct;
			lastReportedOutboundPct = currentOutboundPct;
			lastReportTime = now;
		}

		/*	Check high-water warning thresholds.		*/

		if (warnPercent > 0)
		{
			CHKERR(sdr_begin_xn(sdr));
			sdr_usage(sdr, &warnSummary);
			sdr_exit_xn(sdr);

			usedBytes = (unsigned long)
				(warnSummary.smallPoolAllocated
				+ warnSummary.largePoolAllocated);
			checkWarnThreshold(currentHeapPct,
				warnPercent, &heapWarnActive,
				&lastHeapWarnTime, warnCadenceSec,
				now, "SDR heap", "sdrwatch",
				usedBytes,
				(unsigned long) warnSummary.heapSize);

			CHKERR(sdr_begin_xn(sdr));
			zcoOccupancy = zco_get_heap_occupancy(sdr,
					ZcoInbound);
			zcoMax = zco_get_max_heap_occupancy(sdr,
					ZcoInbound);
			sdr_exit_xn(sdr);
			checkWarnThreshold(currentInboundPct,
				warnPercent, &inboundWarnActive,
				&lastInboundWarnTime,
				warnCadenceSec, now,
				"ZCO inbound", "sdrwatch",
				(unsigned long) zcoOccupancy,
				(unsigned long) zcoMax);

			CHKERR(sdr_begin_xn(sdr));
			zcoOccupancy = zco_get_heap_occupancy(sdr,
					ZcoOutbound);
			zcoMax = zco_get_max_heap_occupancy(sdr,
					ZcoOutbound);
			sdr_exit_xn(sdr);
			checkWarnThreshold(currentOutboundPct,
				warnPercent, &outboundWarnActive,
				&lastOutboundWarnTime,
				warnCadenceSec, now,
				"ZCO outbound", "sdrwatch",
				(unsigned long) zcoOccupancy,
				(unsigned long) zcoMax);
		}
	}

	/*	Log shutdown message.					*/

	writeMemo("[i] sdrwatch daemon stopped.");
	sdr_stop_using(sdr, 0);
	writeErrmsgMemos();
	return 0;
}

static int	run_sdrwatch(char *sdrName, char *mode, int interval,
			int verbose)
{
	Sdr		sdr;
	SdrUsageSummary	sdrsummary;
	int		secRemaining;
	int		decrement = 0;

	sdr_initialize(0, NULL, SM_NO_KEY, NULL);
	sdr = sdr_start_using(sdrName);
	if (sdr == NULL)
	{
		putErrmsg("Can't attach to sdr.", NULL);
		writeErrmsgMemos();
		return 0;
	}

	/*	Initial state.						*/

	CHKERR(sdr_begin_xn(sdr));
	switch (*mode)
	{
	case 's':
		sdr_stats(sdr);
		break;

	case 'r':
		sdr_reset_stats(sdr);
		break;

	case 'z':
		sdr_stats(sdr);
		printf("\n");
		zco_status(sdr);
		break;

	case 't':
		sdr_usage(sdr, &sdrsummary);
		sdr_report(&sdrsummary);
		break;

	default:
		putErrmsg("Invalid sdrwatch mode.", mode);
		interval = 0;	/*	Force immediate return.		*/
	}

	sdr_exit_xn(sdr);
	if (interval == 0)	/*	One-time poll.			*/
	{
		sdr_stop_using(sdr, 0);
		return 0;
	}

	/*	Start watching trace.					*/

	if (*mode == 't')
	{
		if (sdr_start_trace(sdr, 20000000, NULL) < 0)
		{
			putErrmsg("Can't start trace.", NULL);
			sdr_stop_using(sdr, 0);
			writeErrmsgMemos();
			return 0;
		}
	}

	isignal(SIGTERM, handleQuit);
	isignal(SIGINT, handleQuit);

	while (sdrwatch_count(NULL) > 0)
	{
		secRemaining = interval;
		while (secRemaining > 0)
		{
			snooze(1);
			secRemaining--;
		}

		CHKERR(sdr_begin_xn(sdr));
		switch (*mode)
		{
		case 's':
			sdr_stats(sdr);
			break;

		case 'r':
			sdr_reset_stats(sdr);
			break;

		case 'z':
			sdr_stats(sdr);
			printf("\n");
			zco_status(sdr);
			break;

		default:
			if (!verbose)
			{
				sdr_clear_trace(sdr);
			}

			sdr_print_trace(sdr, verbose);
		}

		sdr_exit_xn(sdr);
		oK(sdrwatch_count(&decrement));
	}

	PUTS("Stopping sdrwatch.");
	if (*mode == 't')
	{
		sdr_stop_trace(sdr);
	}

	sdr_stop_using(sdr, 0);
	writeErrmsgMemos();
	return 0;
}

#if !defined (ION_LWT)
static void	printUsage(void)
{
	PUTS("Usage: sdrwatch [<sdr_name>] [ -t | -s | -r | -z ] [<interval> \
[<count> [verbose]]]");
	PUTS("\t-t to print a trace of space allocation and release (default)");
	PUTS("\t-s to print stats for current transaction, no tracing");
	PUTS("\t-r to reset log length high-water mark and then print stats \
for current transaction, no tracing");
	PUTS("\t-z to print stats for current transaction and print ZCO \
status after that transaction ends, no tracing");
	PUTS("   sdr_name: name of the SDR to monitor; if omitted, auto-detected \
from ION configuration");
	PUTS("   interval: polling interval in seconds");
	PUTS("   count: number of polls");
	PUTS("   verbose: enable verbose output, tracing all allocations, \
		does not remove log entries for freed blocks.");
	PUTS("");
	PUTS("Daemon mode:");
	PUTS("   sdrwatch [<sdr_name>] -d [<interval_minutes> \
[<percent_threshold> [<warn_pct> [<warn_cadence_sec>]]]]");
	PUTS("   -d: run as daemon, reporting usage to ion.log");
	PUTS("   interval_minutes: reporting interval (default: 10)");
	PUTS("   percent_threshold: report when usage crosses this threshold \
(default: 5)");
	PUTS("   warn_pct: log WARNING when usage exceeds this %% \
(default: 90, 0=disable)");
	PUTS("   warn_cadence_sec: seconds between repeated warnings while \
above threshold (default: 60)");
	PUTS("   sdr_name: if omitted, auto-detected from ION configuration");
}
#endif

#if defined (ION_LWT)
int	sdrwatch(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char	*sdrName = (char *) a1;
	char	*modeToken = (char *) a2;
	int	interval = 0;
	int	count = 0;
	int	verbose = 0;
	char	*mode = "t";

	if (*modeToken == '-')
	{
		mode = modeToken + 1;
		interval = strtol((char *) a3, NULL, 0);
		count = strtol((char *) a4, NULL, 0);
		verbose = a5;
	}
	else
	{
		interval = strtol((char *) a2, NULL, 0);
		count = strtol((char *) a3, NULL, 0);
		verbose = a4;
	}

	if (interval < 0)
	{
		interval = 0;
	}

	if (count < 1)
	{
		count = 1;
	}

	oK(sdrwatch_count(&count));
	return run_sdrwatch(sdrName, mode, interval, verbose);
}
#else
int	main(int argc, char **argv)
{
	char	*sdrName = "ion";
	char	sdrNameBuf[MAX_SDR_NAME + 1];
	int	interval = 0;
	int	count = 0;
	int	verbose = 0;
	char	*mode = "t";
	int	intervalMinutes;
	double	percentThreshold;
	int	argIdx;
	Sdr	sdr;
	SdrObject iondbObj;
	IonDB	iondb;

	/*	Check for help request.					*/

	if (argc > 1 && (strcmp(argv[1], "-h") == 0
			|| strcmp(argv[1], "--help") == 0))
	{
		printUsage();
		return 0;
	}

	/*	Helper function: Check if argument is a mode flag.	*/
	#define IS_MODE_FLAG(arg)	(strcmp(arg, "-t") == 0 \
					|| strcmp(arg, "-s") == 0 \
					|| strcmp(arg, "-r") == 0 \
					|| strcmp(arg, "-z") == 0 \
					|| strcmp(arg, "-d") == 0)

	/*	Determine if we need to auto-detect SDR name.
	 *	Auto-detect if:
	 *	  - No arguments (argc == 1)
	 *	  - First argument is a mode flag (-t, -s, -r, -z, -d)
	 *	  - First argument looks like a number (interval)	*/

	if (argc < 2 || IS_MODE_FLAG(argv[1])
			|| (argv[1][0] >= '0' && argv[1][0] <= '9'))
	{
		/*	Auto-detect SDR name from ION configuration.	*/

		if (ionAttach() < 0)
		{
			putErrmsg("sdrwatch: can't attach to ION.", NULL);
			writeErrmsgMemos();
			return 1;
		}

		sdr = getIonsdr();
		iondbObj = getIonDbObject();
		CHKZERO(sdr_begin_xn(sdr));
		sdr_read(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
		sdr_exit_xn(sdr);

		istrcpy(sdrNameBuf, iondb.parmcopy.sdrName,
				sizeof sdrNameBuf);
		sdrName = sdrNameBuf;

		ionDetach();

		/*	Restore stdout logging for standard mode.
		 *	ionAttach() redirects memos to ion.log.	*/

		setLogger(logToStdoutLocal);

		argIdx = 1;	/*	Start parsing from argv[1].	*/
	}
	else
	{
		/*	First argument is SDR name.			*/

		sdrName = argv[1];
		argIdx = 2;	/*	Start parsing from argv[2].	*/
	}

	/*	Check for daemon mode (-d flag).			*/

	if (argIdx < argc && strcmp(argv[argIdx], "-d") == 0)
	{
		double	warnPercent = DEFAULT_WARN_PERCENT;
		int	warnCadenceSec = DEFAULT_WARN_CADENCE_SEC;

		intervalMinutes = DEFAULT_REPORT_INTERVAL_MIN;
		percentThreshold = DEFAULT_PERCENT_THRESHOLD;
		argIdx++;

		if (argIdx < argc)
		{
			intervalMinutes = strtol(argv[argIdx], NULL, 0);
			if (intervalMinutes < 1)
			{
				intervalMinutes = DEFAULT_REPORT_INTERVAL_MIN;
			}

			argIdx++;
		}

		if (argIdx < argc)
		{
			percentThreshold = strtod(argv[argIdx], NULL);
			if (percentThreshold < 0.001 || percentThreshold > 100)
			{
				percentThreshold = DEFAULT_PERCENT_THRESHOLD;
			}

			argIdx++;
		}

		if (argIdx < argc)
		{
			warnPercent = strtod(argv[argIdx], NULL);
			if (warnPercent < 0 || warnPercent > 100)
			{
				warnPercent = DEFAULT_WARN_PERCENT;
			}

			argIdx++;
		}

		if (argIdx < argc)
		{
			warnCadenceSec = strtol(argv[argIdx], NULL, 0);
			if (warnCadenceSec < 1)
			{
				warnCadenceSec = DEFAULT_WARN_CADENCE_SEC;
			}
		}

		return run_sdrwatch_daemon(sdrName, intervalMinutes,
				percentThreshold, warnPercent,
				warnCadenceSec);
	}

	/*	Standard mode: sdrwatch [sdr_name] [-t|-s|-r|-z] [interval]
	 *	[count] [verbose]					*/

	if (argIdx < argc && *(argv[argIdx]) == '-')
	{
		mode = argv[argIdx] + 1;
		argIdx++;
	}

	if (argIdx < argc)
	{
		interval = strtol(argv[argIdx], NULL, 0);
		argIdx++;
	}

	if (argIdx < argc)
	{
		count = strtol(argv[argIdx], NULL, 0);
		argIdx++;
	}

	if (argIdx < argc)
	{
		verbose = 1;
	}

	if (interval < 0)
	{
		interval = 0;
	}

	if (count < 1)
	{
		count = 1;
	}

	oK(sdrwatch_count(&count));
	return run_sdrwatch(sdrName, mode, interval, verbose);
}
#endif
