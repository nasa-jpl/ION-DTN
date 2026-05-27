/*

	ionwatch.c:	ION daemon status monitor.

	Author: Generated for ION daemon lifecycle management

									*/
/*	Copyright (c) 2007, California Institute of Technology.		*/
/*	All rights reserved.						*/

#include "ion.h"
#include "platform.h"
#include "ltpP.h"
#include "ltp.h"
#include <stdarg.h>

#ifdef BUILD_BPv7
#include "bpP.h"
#include "bp.h"
#endif

#ifdef BUILD_BPv6
#include "bpP.h"
#include "bp.h"
#endif

#ifdef ENABLE_CFDP
#include "cfdpP.h"
#include "cfdp.h"
#endif

#ifdef ENABLE_DTPC
#include "dtpc.h"
/* Forward declarations to avoid dtpcP.h conflicts with bpP.h */
typedef struct
{
	int		dtpcdPid;
	int		clockPid;
	int		watching;
	sm_SemId	aduSemaphore;
	PsmAddress	vsaps;
	PsmAddress	profiles;
} DtpcVdb;
extern DtpcVdb	*getDtpcVdb(void);
#endif

#ifdef ENABLE_BSSP
#include "bsspP.h"
#include "bssp.h"
#endif

typedef enum
{
	DAEMON_NOT_STARTED = 0,	/* PID is ERROR or <= 0 */
	DAEMON_RUNNING = 1,	/* PID exists and process is running */
	DAEMON_STALE = -1	/* PID set but process doesn't exist */
} DaemonStatus;

#define MAX_DAEMONS 256

typedef struct
{
	char protocol[16];
	char daemon[64];
	int pid;
	DaemonStatus status;
} DaemonRecord;

typedef struct
{
	DaemonRecord daemons[MAX_DAEMONS];
	int count;
	int initialized;
} DaemonHistory;

static DaemonHistory g_previous_history;
static DaemonHistory g_current_history;

static const char* getStatusString(DaemonStatus status)
{
	switch (status)
	{
	case DAEMON_NOT_STARTED:
		return "Not Started";
	case DAEMON_RUNNING:
		return "Running";
	case DAEMON_STALE:
		return "STALE";
	default:
		return "Unknown";
	}
}

static DaemonStatus checkDaemonStatus(int pid, const char *expectedName)
{
	(void)expectedName;  /* Not used in simplified version */

	if (pid == ERROR || pid <= 0)
	{
		return DAEMON_NOT_STARTED;
	}

	if (!sm_TaskExists(pid))
	{
		return DAEMON_STALE;
	}

	return DAEMON_RUNNING;
}

static int findDaemonInHistory(const char *protocol, const char *daemon, int pid)
{
	int i;

	(void)pid;  /* Not used - search by protocol/daemon name only */

	for (i = 0; i < g_previous_history.count; i++)
	{
		if (strcmp(g_previous_history.daemons[i].protocol, protocol) == 0 &&
			strcmp(g_previous_history.daemons[i].daemon, daemon) == 0)
		{
			return i;
		}
	}

	return -1;
}

static int hasStatusChanged(const char *protocol, const char *daemon, int pid,
                            DaemonStatus status)
{
	int idx;

	if (!g_previous_history.initialized)
	{
		return 1;  /* First iteration - everything is a "change" */
	}

	idx = findDaemonInHistory(protocol, daemon, pid);
	if (idx < 0)
	{
		return 1;  /* New daemon */
	}

	return (g_previous_history.daemons[idx].status != status);
}

static void recordDaemon(const char *protocol, const char *daemon, int pid,
                         DaemonStatus status)
{
	if (g_current_history.count < MAX_DAEMONS)
	{
		strncpy(g_current_history.daemons[g_current_history.count].protocol, protocol, 15);
		g_current_history.daemons[g_current_history.count].protocol[15] = '\0';
		strncpy(g_current_history.daemons[g_current_history.count].daemon, daemon, 63);
		g_current_history.daemons[g_current_history.count].daemon[63] = '\0';
		g_current_history.daemons[g_current_history.count].pid = pid;
		g_current_history.daemons[g_current_history.count].status = status;
		g_current_history.count++;
	}
}

static void resetDaemonRecording(void)
{
	/* Clear current history to prepare for new iteration */
	g_current_history.count = 0;
}

static void commitDaemonHistory(void)
{
	int i;

	/* Copy current history to previous history for next iteration */
	g_previous_history.count = g_current_history.count;
	for (i = 0; i < g_current_history.count; i++)
	{
		g_previous_history.daemons[i] = g_current_history.daemons[i];
	}
	g_previous_history.initialized = 1;
}

static void logMessage(int logToFile, const char *format, ...)
{
	char buffer[512];
	char logBuffer[768];
	va_list args;

	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);

	if (logToFile)
	{
		snprintf(logBuffer, sizeof(logBuffer), "[i] ionwatch: %s", buffer);
		writeMemo(logBuffer);
	}
	else
	{
		printf("%s\n", buffer);
	}
}

static void printDaemonEx(const char *protocol, const char *daemon, int pid,
                          DaemonStatus status, const char *notes,
                          int logToFile, int quietMode, int *anyChanged)
{
	int changed = 0;
	char changeMarker[32] = "";
	char outputLine[512];

	/* Check if status changed */
	if (quietMode)
	{
		changed = hasStatusChanged(protocol, daemon, pid, status);
		if (changed)
		{
			snprintf(changeMarker, sizeof(changeMarker), " [*CHANGED*]");
			if (anyChanged != NULL)
			{
				*anyChanged = 1;
			}
		}
	}

	/* Record this daemon for next iteration */
	if (quietMode)
	{
		recordDaemon(protocol, daemon, pid, status);
	}

	/* Build output line */
	if (logToFile)
	{
		/* Log format: Protocol/daemon status (PID) [CHANGED] */
		/* In quiet mode after first iteration, only log if changed */
		if (quietMode && g_previous_history.initialized && !changed)
		{
			return;  /* Silent - no output */
		}

		if (changed && g_previous_history.initialized)
		{
			int idx = findDaemonInHistory(protocol, daemon, pid);
			if (idx >= 0)
			{
				snprintf(outputLine, sizeof(outputLine),
					"%s/%s %s (PID %d) - status changed from %s",
					protocol, daemon, getStatusString(status), pid,
					getStatusString(g_previous_history.daemons[idx].status));
			}
			else
			{
				snprintf(outputLine, sizeof(outputLine),
					"%s/%s %s (PID %d) - status changed",
					protocol, daemon, getStatusString(status), pid);
			}
		}
		else
		{
			snprintf(outputLine, sizeof(outputLine),
				"%s/%s %s (PID %d)",
				protocol, daemon, getStatusString(status), pid);
		}
		logMessage(1, "%s", outputLine);
	}
	else
	{
		/* Stdout format: simple output with change marker */
		/* In quiet mode after first iteration, only print if changed */
		if (quietMode && g_previous_history.initialized && !changed)
		{
			return;  /* Silent - no output */
		}

		printf("%-8s %-20s %-10d %-12s%-12s %s\n",
			protocol, daemon, pid, getStatusString(status),
			changeMarker, notes);
	}
}

static void printHeader(void)
{
	printf("\n%-8s %-20s %-10s %-12s %s\n",
		"Protocol", "Daemon", "PID", "Status", "Notes");
	printf("--------------------------------------------------------------------------------\n");
}

static void printFooter(void)
{
	printf("--------------------------------------------------------------------------------\n\n");
}

static int displayDaemonStatus(int showOnlyRunning, int logToFile,
                                int quietMode, int *anyChanged)
{
	IonVdb *ionvdb = NULL;
	LtpVdb *ltpvdb = NULL;
	PsmPartition ltpwm;
	PsmAddress elt;
	PsmAddress addr;
	LtpVspan *vspan;
	LtpVseat *vseat;
	char daemonName[300];
	Sdr sdr;
	LtpSpan span;
	char lsoCmd[SDRSTRING_BUFSZ];
#if defined(BUILD_BPv7) || defined(BUILD_BPv6)
	BpVdb *bpvdb = NULL;
	PsmPartition bpwm;
	VPlan *vplan;
	VInduct *vinduct;
	VOutduct *voutduct;
#endif
#ifdef ENABLE_CFDP
	CfdpVdb *cfdpvdb = NULL;
#endif
#ifdef ENABLE_DTPC
	DtpcVdb *dtpcvdb = NULL;
#endif
#ifdef ENABLE_BSSP
	BsspVdb *bsspvdb = NULL;
#endif
	DaemonStatus status;

	/* Print header only for stdout (not for log file) */
	if (!logToFile)
	{
		/* In quiet mode, only print if this is the first iteration */
		if (!quietMode || !g_previous_history.initialized)
		{
			printHeader();
		}
	}

	/*	ICI daemons						*/

	ionvdb = getIonVdb();
	if (ionvdb != NULL)
	{
		status = checkDaemonStatus(ionvdb->clockPid, "rfxclock");
		if (!showOnlyRunning || status == DAEMON_RUNNING)
		{
			printDaemonEx("ICI", "rfxclock", ionvdb->clockPid,
					status, "Contact plan manager",
					logToFile, quietMode, anyChanged);
		}
	}

	/*	LTP daemons						*/

	ltpvdb = getLtpVdb();
	if (ltpvdb != NULL)
	{
		status = checkDaemonStatus(ltpvdb->clockPid, "ltpclock");
		if (!showOnlyRunning || status == DAEMON_RUNNING)
		{
			printDaemonEx("LTP", "ltpclock", ltpvdb->clockPid,
					status, "Event scheduler",
					logToFile, quietMode, anyChanged);
		}

		status = checkDaemonStatus(ltpvdb->delivPid, "ltpdeliv");
		if (!showOnlyRunning || status == DAEMON_RUNNING)
		{
			printDaemonEx("LTP", "ltpdeliv", ltpvdb->delivPid,
					status, "Delivery service",
					logToFile, quietMode, anyChanged);
		}

		/*	Per-span daemons (LSO, ltpmeter)		*/

		ltpwm = getIonwm();
		sdr = getIonsdr();

		for (elt = sm_list_first(ltpwm, ltpvdb->spans); elt;
				elt = sm_list_next(ltpwm, elt))
		{
			addr = sm_list_data(ltpwm, elt);
			vspan = (LtpVspan *) psp(ltpwm, addr);
			if (vspan == NULL)
			{
				continue;
			}

			/* Read the span structure from SDR to get lsoCmd */
			CHKZERO(sdr_begin_xn(sdr));
			sdr_read(sdr, (char *) &span,
					sdr_list_data(sdr, vspan->spanElt),
					sizeof(LtpSpan));
			sdr_string_read(sdr, lsoCmd, span.lsoCmd);
			sdr_exit_xn(sdr);

			/* Check LSO (Link Service Output) daemon */
			status = checkDaemonStatus(vspan->lsoPid, lsoCmd);
			if (!showOnlyRunning || status == DAEMON_RUNNING)
			{
				snprintf(daemonName, sizeof(daemonName),
						"%s [" UVAST_FIELDSPEC "]",
						lsoCmd, vspan->engineId);
				printDaemonEx("LTP", daemonName, vspan->lsoPid,
						status, "Link service output",
						logToFile, quietMode, anyChanged);
			}

			/* Check ltpmeter daemon */
			status = checkDaemonStatus(vspan->meterPid, "ltpmeter");
			if (!showOnlyRunning || status == DAEMON_RUNNING)
			{
				snprintf(daemonName, sizeof(daemonName),
						"ltpmeter [" UVAST_FIELDSPEC "]",
						vspan->engineId);
				printDaemonEx("LTP", daemonName, vspan->meterPid,
						status, "Meter",
						logToFile, quietMode, anyChanged);
			}
		}

		/*	Per-seat daemons (LSI)				*/

		for (elt = sm_list_first(ltpwm, ltpvdb->seats); elt;
				elt = sm_list_next(ltpwm, elt))
		{
			addr = sm_list_data(ltpwm, elt);
			vseat = (LtpVseat *) psp(ltpwm, addr);
			if (vseat == NULL)
			{
				continue;
			}

			/* Check LSI (Link Service Input) daemon */
			status = checkDaemonStatus(vseat->lsiPid, vseat->lsiCmd);
			if (!showOnlyRunning || status == DAEMON_RUNNING)
			{
				snprintf(daemonName, sizeof(daemonName),
						"%s", vseat->lsiCmd);
				printDaemonEx("LTP", daemonName, vseat->lsiPid,
						status, "Link service input",
						logToFile, quietMode, anyChanged);
			}
		}
	}

#if defined(BUILD_BPv7) || defined(BUILD_BPv6)
	/*	BP daemons						*/

	bpvdb = getBpVdb();
	if (bpvdb != NULL)
	{
		status = checkDaemonStatus(bpvdb->clockPid, "bpclock");
		if (!showOnlyRunning || status == DAEMON_RUNNING)
		{
			printDaemonEx("BP", "bpclock", bpvdb->clockPid,
					status, "Event scheduler",
					logToFile, quietMode, anyChanged);
		}

		status = checkDaemonStatus(bpvdb->cpsdPid, "cpsd");
		if (!showOnlyRunning || status == DAEMON_RUNNING)
		{
			printDaemonEx("BP", "cpsd", bpvdb->cpsdPid,
					status, "Contact plan sync",
					logToFile, quietMode, anyChanged);
		}

		status = checkDaemonStatus(bpvdb->transitPid, "bptransit");
		if (!showOnlyRunning || status == DAEMON_RUNNING)
		{
			printDaemonEx("BP", "bptransit", bpvdb->transitPid,
					status, "Transit processor",
					logToFile, quietMode, anyChanged);
		}

		/*	Per-plan daemons (bpclm)				*/

		bpwm = getIonwm();

		for (elt = sm_list_first(bpwm, bpvdb->plans); elt;
		     elt = sm_list_next(bpwm, elt))
		{
			addr = sm_list_data(bpwm, elt);
			vplan = (VPlan *) psp(bpwm, addr);
			if (vplan == NULL)
			{
				continue;
			}

			status = checkDaemonStatus(vplan->clmPid, "bpclm");
			if (!showOnlyRunning || status == DAEMON_RUNNING)
			{
				snprintf(daemonName, sizeof(daemonName),
					"bpclm [%s]", vplan->neighborEid);
				printDaemonEx("BP", daemonName, vplan->clmPid,
					status, "CL manager",
					logToFile, quietMode, anyChanged);
			}
		}

		/*	Per-induct daemons (CLIs)				*/

		for (elt = sm_list_first(bpwm, bpvdb->inducts); elt;
				elt = sm_list_next(bpwm, elt))
		{
			addr = sm_list_data(bpwm, elt);
			vinduct = (VInduct *) psp(bpwm, addr);
			if (vinduct == NULL)
			{
				continue;
			}

			snprintf(daemonName, sizeof(daemonName),
					"%scli", vinduct->protocolName);
			status = checkDaemonStatus(vinduct->cliPid, daemonName);
			if (!showOnlyRunning || status == DAEMON_RUNNING)
			{
				snprintf(daemonName, sizeof(daemonName),
						"%scli [%s]", vinduct->protocolName,
						vinduct->ductName);
				printDaemonEx("BP", daemonName, vinduct->cliPid,
						status, "CL input",
						logToFile, quietMode, anyChanged);
			}
		}

		/*	Per-outduct daemons (CLOs)				*/

		for (elt = sm_list_first(bpwm, bpvdb->outducts); elt;
				elt = sm_list_next(bpwm, elt))
		{
			addr = sm_list_data(bpwm, elt);
			voutduct = (VOutduct *) psp(bpwm, addr);
			if (voutduct == NULL)
			{
				continue;
			}

			snprintf(daemonName, sizeof(daemonName),
					"%sclo", voutduct->protocolName);
			status = checkDaemonStatus(voutduct->cloPid, daemonName);
			if (!showOnlyRunning || status == DAEMON_RUNNING)
			{
				snprintf(daemonName, sizeof(daemonName),
						"%sclo [%s]", voutduct->protocolName,
						voutduct->ductName);
				printDaemonEx("BP", daemonName, voutduct->cloPid,
						status, "CL output",
						logToFile, quietMode, anyChanged);
			}
		}
	}
#endif

#ifdef ENABLE_CFDP
	/*	CFDP daemons						*/

	cfdpvdb = getCfdpVdb();
	if (cfdpvdb != NULL)
	{
		status = checkDaemonStatus(cfdpvdb->clockPid, "cfdpclock");
		if (!showOnlyRunning || status == DAEMON_RUNNING)
		{
			printDaemonEx("CFDP", "cfdpclock", cfdpvdb->clockPid,
					status, "Event scheduler",
					logToFile, quietMode, anyChanged);
		}

		status = checkDaemonStatus(cfdpvdb->utaPid, "cfdp");
		if (!showOnlyRunning || status == DAEMON_RUNNING)
		{
			printDaemonEx("CFDP", "cfdp UT layer", cfdpvdb->utaPid,
					status, "UT adapter",
					logToFile, quietMode, anyChanged);
		}
	}
#endif

#ifdef ENABLE_DTPC
	/*	DTPC daemons						*/

	dtpcvdb = getDtpcVdb();
	if (dtpcvdb != NULL)
	{
		status = checkDaemonStatus(dtpcvdb->clockPid, "dtpcclock");
		if (!showOnlyRunning || status == DAEMON_RUNNING)
		{
			printDaemonEx("DTPC", "dtpcclock", dtpcvdb->clockPid,
					status, "Event scheduler",
					logToFile, quietMode, anyChanged);
		}

		status = checkDaemonStatus(dtpcvdb->dtpcdPid, "dtpcd");
		if (!showOnlyRunning || status == DAEMON_RUNNING)
		{
			printDaemonEx("DTPC", "dtpcd", dtpcvdb->dtpcdPid,
					status, "Main daemon",
					logToFile, quietMode, anyChanged);
		}
	}
#endif

#ifdef ENABLE_BSSP
	/*	BSSP daemons						*/

	bsspvdb = getBsspVdb();
	if (bsspvdb != NULL)
	{
		status = checkDaemonStatus(bsspvdb->clockPid, "bsspclock");
		if (!showOnlyRunning || status == DAEMON_RUNNING)
		{
			printDaemonEx("BSSP", "bsspclock", bsspvdb->clockPid,
					status, "Event scheduler",
					logToFile, quietMode, anyChanged);
		}
	}
#endif

	/* Print footer only for stdout (not for log file) */
	if (!logToFile)
	{
		/* In quiet mode, only print if first iteration or if there were changes */
		if (!quietMode || !g_previous_history.initialized || *anyChanged)
		{
			printFooter();
		}
	}

	return 0;
}

static unsigned int ionwatch_count(int *newValue)
{
	static unsigned int count = 1;

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

static void handleQuit(int signum)
{
	/* Tell the compiler that we are not using 'signum' */
	(void)signum;

	int newCount = 1;	/*	Advance to end of last cycle.	*/

	PUTS("\n[Terminated by user.]");
	oK(ionwatch_count(&newCount));
}

/*	Quick pre-check: verify that the SDR shared memory segment
 *	exists before calling ionAttach().  ionAttach() triggers a
 *	cascade of low-level error messages (putSysErrmsg, fprintf
 *	to stderr) when the shared memory is absent, which is
 *	confusing when the user simply wants to check if ION is
 *	running.  By probing for the well-known SDR working memory
 *	key first, we can print a clean, user-friendly message
 *	instead.
 *
 *	SDR_SM_KEY is 255 * 256 = 65280 (0xFF00), defined in
 *	sdrP.h.  We duplicate the value here to avoid pulling in
 *	private SDR headers.						*/

#ifndef SDR_SM_KEY
#define SDR_SM_KEY	(255 * 256)
#endif

static int	ionShmExists(void)
{
#ifdef SVR4_SHM
	/*	SVR4 / POSIX shared memory: probe with shmget.		*/

	return (shmget(SDR_SM_KEY, 0, 0) != -1);
#else
	/*	On platforms without SVR4 shared memory (e.g.,
	 *	VxWorks), skip the pre-check and let ionAttach()
	 *	report errors normally.					*/

	return 1;
#endif
}

static int run_daemonwatch(int interval, int showOnlyRunning,
		int logToFile, int quietMode)
{
	int secRemaining;
	int decrement = 0;
	int anyChanged = 0;

	if (!ionShmExists())
	{
		PUTS("ION is not running (shared memory not found).");
		return 1;
	}

	if (ionAttach() < 0)
	{
		putErrmsg("Can't attach to ION.", NULL);
		writeErrmsgMemos();
		return 1;
	}

	/*	Attach to protocol subsystems to access their Vdbs.	*/

	ltp_attach();

#if defined(BUILD_BPv7) || defined(BUILD_BPv6)
	bp_attach();
#endif

#ifdef ENABLE_CFDP
	cfdp_attach();
#endif

#ifdef ENABLE_DTPC
	dtpc_attach();
#endif

#ifdef ENABLE_BSSP
	bssp_attach();
#endif

	/*	Initial display.					*/

	resetDaemonRecording();
	anyChanged = 0;

	if (displayDaemonStatus(showOnlyRunning, logToFile,
			quietMode, &anyChanged) < 0)
	{
		putErrmsg("Can't display daemon status.", NULL);
		ionDetach();
		return 1;
	}

	/* Commit current iteration to history for next comparison */
	commitDaemonHistory();

	if (interval == 0)	/*	One-time poll.			*/
	{
		ionDetach();
		return 0;
	}

	/*	Start watching.						*/

	isignal(SIGTERM, handleQuit);
	isignal(SIGINT, handleQuit);

	while (ionwatch_count(NULL) > 0)
	{
		secRemaining = interval;
		while (secRemaining > 0)
		{
			snooze(1);
			secRemaining--;
		}

		resetDaemonRecording();
		anyChanged = 0;

		if (displayDaemonStatus(showOnlyRunning, logToFile,
				quietMode, &anyChanged) < 0)
		{
			putErrmsg("Can't display daemon status.", NULL);
			break;
		}

		/* Commit current history for comparison in next iteration */
		commitDaemonHistory();

		/* In quiet mode, only output if there were changes */
		if (quietMode && !anyChanged)
		{
			/* Silent - no output */
		}

		oK(ionwatch_count(&decrement));
	}

	writeErrmsgMemos();
	ionDetach();
	return 0;
}

#if defined(ION_LWT)
int ionwatch(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	int interval = a1 ? strtol((char *) a1, NULL, 0) : 0;
	int count = a2 ? strtol((char *) a2, NULL, 0) : 1;
	int showOnlyRunning = a3 ? 1 : 0;
	int logToFile = a4 ? 1 : 0;
	int quietMode = a5 ? 1 : 0;
#else
int main(int argc, char **argv)
{
	int interval = 0;
	int count = 1;
	int showOnlyRunning = 0;
	int logToFile = 0;
	int quietMode = 0;
	int i;

	for (i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--watch") == 0)
		{
			if (i + 1 < argc)
			{
				interval = strtol(argv[i + 1], NULL, 0);
				count = 0;  /* Infinite loop unless -c is specified */
				i++;
			}
			else
			{
				PUTS("Error: --watch requires an interval argument");
				return 1;
			}
		}
		else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--count") == 0)
		{
			if (i + 1 < argc)
			{
				count = strtol(argv[i + 1], NULL, 0);
				i++;
			}
			else
			{
				PUTS("Error: --count requires a count argument");
				return 1;
			}
		}
		else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--running-only") == 0)
		{
			showOnlyRunning = 1;
		}
		else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--log") == 0)
		{
			logToFile = 1;
		}
		else if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--quiet") == 0 ||
				strcmp(argv[i], "--changes-only") == 0)
		{
			quietMode = 1;
		}
		else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
		{
			PUTS("Usage: ionwatch [options]");
			PUTS("");
			PUTS("Options:");
			PUTS("  -w, --watch <interval>    Watch mode: refresh every <interval> seconds");
			PUTS("  -c, --count <count>       Number of refresh cycles (default: 1)");
			PUTS("  -r, --running-only        Show only running daemons");
			PUTS("  -l, --log                 Output to ion.log instead of stdout");
			PUTS("  -q, --quiet               Quiet mode: show full status on first iteration,");
			PUTS("                            then silent unless status changes");
			PUTS("  -h, --help                Show this help message");
			PUTS("");
			PUTS("Examples:");
			PUTS("  ionwatch               Display daemon status once");
			PUTS("  ionwatch -w 5          Refresh every 5 seconds indefinitely");
			PUTS("  ionwatch -w 2 -c 10    Refresh every 2 seconds, 10 times");
			PUTS("  ionwatch -r            Show only running daemons");
			PUTS("  ionwatch -l            Log output to ion.log");
			PUTS("  ionwatch -w 10 -q -l   Watchdog: log changes to ion.log every 10s");
			return 0;
		}
		else
		{
			printf("Unknown option: %s\n", argv[i]);
			PUTS("Use --help for usage information");
			return 1;
		}
	}

	if (count < 0)
	{
		count = 0;  /* Invalid negative count, set to infinite */
	}

	if (count == 0)
	{
		count = 2147483647;  /* Effectively infinite for watch mode */
	}
#endif

	oK(ionwatch_count(&count));
	return run_daemonwatch(interval, showOnlyRunning,
			logToFile, quietMode);
}
