/*
 *	ltpwatch.c:	LTP session status monitor.
 *
 *	Author: ION Development Team
 *
 *	Copyright (c) 2024, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 */

#include "ltpP.h"
#include "ion.h"

typedef enum
{
	FILTER_ALL = 0,
	FILTER_EXPORT,
	FILTER_IMPORT
} TypeFilter;

#define NUM_STATUSES	15

typedef struct
{
	const char	*name;
	int		count;
} StatusCounter;

static const char	*statusNames[NUM_STATUSES] =
{
	"buffering",
	"transmitting",
	"awaiting-cleanup",
	"green-only",
	"receiving",
	"complete-pending",
	"awaiting-delivery",
	"delivered",
	"completed",
	"canceled(user)",
	"canceled(unreachable)",
	"canceled(rexmit)",
	"canceled(miscolor)",
	"canceled(engine)",
	"canceled(unknown)"
};

static void	tallyStatus(StatusCounter *counters, const char *status)
{
	int	i;

	for (i = 0; i < NUM_STATUSES; i++)
	{
		if (strcmp(counters[i].name, status) == 0)
		{
			counters[i].count++;
			return;
		}
	}
}

static void	initCounters(StatusCounter *counters)
{
	int	i;

	for (i = 0; i < NUM_STATUSES; i++)
	{
		counters[i].name = statusNames[i];
		counters[i].count = 0;
	}
}

static void	printStatusSummary(StatusCounter *counters)
{
	char	buffer[256];
	int	i;
	int	printed = 0;

	for (i = 0; i < NUM_STATUSES; i++)
	{
		if (counters[i].count > 0)
		{
			isprintf(buffer, sizeof buffer,
				"  %-24s %d",
				counters[i].name, counters[i].count);
			PUTS(buffer);
			printed++;
		}
	}

	if (printed == 0)
	{
		PUTS("  (no sessions)");
	}
}

static unsigned int	ltpwatch_count(int *newValue)
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
	int	newCount = 1;

	(void) signum;
	PUTS("[Terminated by user.]");
	oK(ltpwatch_count(&newCount));
}

static void	printUsage(void)
{
	PUTS("Usage: ltpwatch [-a] [-c] [-t <type>] [-s <session>] \
[-e <engineId>] [<interval> [<count>]]");
	PUTS("");
	PUTS("Options:");
	PUTS("  -a            Show only active sessions (omit dead/completed)");
	PUTS("  -c            Summary count mode (counts by status)");
	PUTS("  -t <type>     Filter by session type: 'import' or 'export' \
(default: both)");
	PUTS("  -s <number>   Filter by specific session number");
	PUTS("  -e <engineId> Filter by span engine ID (default: all spans)");
	PUTS("  -h            Show this help message");
	PUTS("");
	PUTS("Arguments:");
	PUTS("  interval      Refresh interval in seconds (default: 0 = \
one-shot)");
	PUTS("  count         Number of iterations (default: infinite if \
interval>0, 1 if interval=0)");
	PUTS("");
	PUTS("Examples:");
	PUTS("  ltpwatch                   One-shot, all sessions, all spans");
	PUTS("  ltpwatch 5                 Refresh every 5 seconds indefinitely");
	PUTS("  ltpwatch 2 10              Refresh every 2 seconds, 10 times");
	PUTS("  ltpwatch -t export         One-shot, export sessions only");
	PUTS("  ltpwatch -e 2 -t import 3  Engine 2, imports only, every 3 sec");
	PUTS("  ltpwatch -s 12345          Find session 12345 across all spans");
	PUTS("  ltpwatch -a                Active sessions only");
	PUTS("  ltpwatch -c                Summary counts by status");
	PUTS("  ltpwatch -a -c -t import   Active import summary counts");
}

static const char	*getCancelReasonStr(int reasonCode)
{
	switch (reasonCode)
	{
	case LtpCancelByUser:
		return "canceled(user)";

	case LtpClientSvcUnreachable:
		return "canceled(unreachable)";

	case LtpRetransmitLimitExceeded:
		return "canceled(rexmit)";

	case LtpMiscoloredSegment:
		return "canceled(miscolor)";

	case LtpCancelByEngine:
		return "canceled(engine)";

	default:
		return "canceled(unknown)";
	}
}

static const char	*getExportSessionStatus(LtpExportSession *session,
				int isDead)
{
	if (isDead)
	{
		if (session->reasonCode == 0 || session->reasonCode == -1)
		{
			return "completed";
		}

		return getCancelReasonStr(session->reasonCode);
	}

	/*	Active session.						*/

	if (session->stateFlags & LTP_FINAL_ACK)
	{
		return "awaiting-cleanup";
	}

	if (session->stateFlags & LTP_EOB_SENT)
	{
		return "transmitting";
	}

	if (session->redPartLength == 0)
	{
		return "green-only";
	}

	return "buffering";
}

static const char	*getImportSessionStatus(LtpImportSession *session,
				int isDead)
{
	if (isDead)
	{
		if (session->reasonCode == 0 || session->reasonCode == -1)
		{
			return "completed";
		}

		return getCancelReasonStr(session->reasonCode);
	}

	/*	Active session.						*/

	if (session->delivered)
	{
		return "delivered";
	}

	if (session->finalRptAcked)
	{
		return "awaiting-delivery";
	}

	if (session->endOfBlockRecd
	&& session->redPartReceived >= session->redPartLength)
	{
		return "complete-pending";
	}

	return "receiving";
}

static void	displayExportSession(LtpExportSession *session, int isDead,
			unsigned int filterSession, int *count)
{
	char		buffer[256];
	const char	*status;

	if (filterSession != 0 && session->sessionNbr != filterSession)
	{
		return;
	}

	status = getExportSessionStatus(session, isDead);

	isprintf(buffer, sizeof buffer,
		"  %-10u %-18s %-6u %-10d %-10d %-6d",
		session->sessionNbr,
		status,
		session->clientSvcId,
		session->totalLength,
		session->redPartLength,
		session->timer.expirationCount);
	PUTS(buffer);

	(*count)++;
}

static void	displayImportSession(LtpImportSession *session, int isDead,
			unsigned int filterSession, int *count)
{
	char		buffer[256];
	const char	*status;

	if (filterSession != 0 && session->sessionNbr != filterSession)
	{
		return;
	}

	status = getImportSessionStatus(session, isDead);

	isprintf(buffer, sizeof buffer,
		"  %-10u %-18s %-6u %-10d %-10d %-6d",
		session->sessionNbr,
		status,
		session->clientSvcId,
		session->redPartLength,
		session->redPartReceived,
		session->timer.expirationCount);
	PUTS(buffer);

	(*count)++;
}

static void processSpan(Sdr sdr, SdrObject spanObj, LtpSpan *span,
			TypeFilter typeFilter, unsigned int filterSession,
			int activeOnly, int countMode)
{
	char			buffer[256];
	SdrObject		elt;
	SdrObject		sessionObj;
	LtpExportSession	exportSession;
	LtpImportSession	importSession;
	int			activeExports = 0;
	int			deadExports = 0;
	int			activeImports = 0;
	int			deadImports = 0;
	int			displayCount;
	int			totalCount;
	LtpDB			*ltpConstants;
	StatusCounter		counters[NUM_STATUSES];
	const char		*status;

	isprintf(buffer, sizeof buffer,
		"\n--- Span " UVAST_FIELDSPEC " ---", span->engineId);
	PUTS(buffer);

	/*	Display export sessions if not filtered out.		*/

	if (typeFilter == FILTER_ALL || typeFilter == FILTER_EXPORT)
	{
		/*	Count sessions.					*/

		activeExports = sdr_list_length(sdr, span->exportSessions);

		ltpConstants = getLtpConstants();
		if (!activeOnly && ltpConstants != NULL)
		{
			/*	Count dead exports for this span.	*/

			for (elt = sdr_list_first(sdr, ltpConstants->deadExports);
				elt; elt = sdr_list_next(sdr, elt))
			{
				sessionObj = sdr_list_data(sdr, elt);
				sdr_read(sdr, (char *) &exportSession,
					sessionObj, sizeof(LtpExportSession));

				/*	Check if this dead export belongs
				 *	to this span.			*/

				if (exportSession.span == spanObj)
				{
					deadExports++;
				}
			}
		}

		if (activeOnly)
		{
			isprintf(buffer, sizeof buffer,
				"\nEXPORT SESSIONS (Active: %d)",
				activeExports);
		}
		else
		{
			isprintf(buffer, sizeof buffer,
				"\nEXPORT SESSIONS (Active: %d, Dead: %d)",
				activeExports, deadExports);
		}

		PUTS(buffer);

		if (countMode)
		{
			initCounters(counters);
			totalCount = 0;

			/*	Tally active export sessions.		*/

			for (elt = sdr_list_first(sdr,
				span->exportSessions); elt;
				elt = sdr_list_next(sdr, elt))
			{
				sessionObj = sdr_list_data(sdr, elt);
				sdr_read(sdr, (char *) &exportSession,
					sessionObj,
					sizeof(LtpExportSession));
				if (filterSession != 0
				&& exportSession.sessionNbr
					!= filterSession)
				{
					continue;
				}

				status = getExportSessionStatus(
					&exportSession, 0);
				tallyStatus(counters, status);
				totalCount++;
			}

			/*	Tally dead export sessions.		*/

			if (!activeOnly && ltpConstants != NULL)
			{
				for (elt = sdr_list_first(sdr,
					ltpConstants->deadExports);
					elt;
					elt = sdr_list_next(sdr, elt))
				{
					sessionObj = sdr_list_data(sdr,
						elt);
					sdr_read(sdr,
						(char *) &exportSession,
						sessionObj,
						sizeof(LtpExportSession));
					if (exportSession.span != spanObj)
					{
						continue;
					}

					if (filterSession != 0
					&& exportSession.sessionNbr
						!= filterSession)
					{
						continue;
					}

					status = getExportSessionStatus(
						&exportSession, 1);
					tallyStatus(counters, status);
					totalCount++;
				}
			}

			printStatusSummary(counters);
			isprintf(buffer, sizeof buffer,
				"  Total: %d", totalCount);
			PUTS(buffer);
		}
		else
		{
			PUTS("  Session    Status             Client \
TotalLen   RedLen     Rexmit");
			PUTS("  -------    ------             ------ \
---------  ---------  ------");

			displayCount = 0;

			/*	Display active export sessions.		*/

			for (elt = sdr_list_first(sdr,
				span->exportSessions); elt;
				elt = sdr_list_next(sdr, elt))
			{
				sessionObj = sdr_list_data(sdr, elt);
				sdr_read(sdr, (char *) &exportSession,
					sessionObj,
					sizeof(LtpExportSession));
				displayExportSession(&exportSession, 0,
					filterSession, &displayCount);
			}

			/*	Display dead export sessions.		*/

			if (!activeOnly && ltpConstants != NULL)
			{
				for (elt = sdr_list_first(sdr,
					ltpConstants->deadExports);
					elt;
					elt = sdr_list_next(sdr, elt))
				{
					sessionObj = sdr_list_data(sdr,
						elt);
					sdr_read(sdr,
						(char *) &exportSession,
						sessionObj,
						sizeof(LtpExportSession));
					if (exportSession.span == spanObj)
					{
						displayExportSession(
							&exportSession,
							1, filterSession,
							&displayCount);
					}
				}
			}

			if (displayCount == 0)
			{
				PUTS("  (no sessions)");
			}
		}
	}

	/*	Display import sessions if not filtered out.		*/

	if (typeFilter == FILTER_ALL || typeFilter == FILTER_IMPORT)
	{
		activeImports = sdr_list_length(sdr, span->importSessions);

		if (!activeOnly)
		{
			deadImports = sdr_list_length(sdr,
				span->deadImports);
		}

		if (activeOnly)
		{
			isprintf(buffer, sizeof buffer,
				"\nIMPORT SESSIONS (Active: %d)",
				activeImports);
		}
		else
		{
			isprintf(buffer, sizeof buffer,
				"\nIMPORT SESSIONS (Active: %d, Dead: %d)",
				activeImports, deadImports);
		}

		PUTS(buffer);

		if (countMode)
		{
			initCounters(counters);
			totalCount = 0;

			/*	Tally active import sessions.		*/

			for (elt = sdr_list_first(sdr,
				span->importSessions); elt;
				elt = sdr_list_next(sdr, elt))
			{
				sessionObj = sdr_list_data(sdr, elt);
				sdr_read(sdr, (char *) &importSession,
					sessionObj,
					sizeof(LtpImportSession));
				if (filterSession != 0
				&& importSession.sessionNbr
					!= filterSession)
				{
					continue;
				}

				status = getImportSessionStatus(
					&importSession, 0);
				tallyStatus(counters, status);
				totalCount++;
			}

			/*	Tally dead import sessions.		*/

			if (!activeOnly)
			{
				for (elt = sdr_list_first(sdr,
					span->deadImports); elt;
					elt = sdr_list_next(sdr, elt))
				{
					sessionObj = sdr_list_data(sdr,
						elt);
					sdr_read(sdr,
						(char *) &importSession,
						sessionObj,
						sizeof(LtpImportSession));
					if (filterSession != 0
					&& importSession.sessionNbr
						!= filterSession)
					{
						continue;
					}

					status = getImportSessionStatus(
						&importSession, 1);
					tallyStatus(counters, status);
					totalCount++;
				}
			}

			printStatusSummary(counters);
			isprintf(buffer, sizeof buffer,
				"  Total: %d", totalCount);
			PUTS(buffer);
		}
		else
		{
			PUTS("  Session    Status             Client \
RedLen     Received   Rexmit");
			PUTS("  -------    ------             ------ \
---------  ---------  ------");

			displayCount = 0;

			/*	Display active import sessions.		*/

			for (elt = sdr_list_first(sdr,
				span->importSessions); elt;
				elt = sdr_list_next(sdr, elt))
			{
				sessionObj = sdr_list_data(sdr, elt);
				sdr_read(sdr, (char *) &importSession,
					sessionObj,
					sizeof(LtpImportSession));
				displayImportSession(&importSession, 0,
					filterSession, &displayCount);
			}

			/*	Display dead import sessions.		*/

			if (!activeOnly)
			{
				for (elt = sdr_list_first(sdr,
					span->deadImports); elt;
					elt = sdr_list_next(sdr, elt))
				{
					sessionObj = sdr_list_data(sdr,
						elt);
					sdr_read(sdr,
						(char *) &importSession,
						sessionObj,
						sizeof(LtpImportSession));
					displayImportSession(
						&importSession, 1,
						filterSession,
						&displayCount);
				}
			}

			if (displayCount == 0)
			{
				PUTS("  (no sessions)");
			}
		}
	}
}

static int	runLtpWatch(uvast filterEngineId, int hasEngineFilter,
			TypeFilter typeFilter, unsigned int filterSession,
			int interval, int activeOnly, int countMode)
{
	Sdr		sdr;
	LtpDB		*ltpConstants;
	SdrObject	sdrElt;
	SdrObject	spanObj;
	LtpSpan		span;
	char		buffer[256];
	char		timestamp[TIMESTAMPBUFSZ];
	time_t		currentTime;
	int		secRemaining;
	int		decrement = 0;
	int		spanCount;

	sdr = getIonsdr();

	isignal(SIGTERM, handleQuit);
	isignal(SIGINT, handleQuit);

	while (ltpwatch_count(NULL) > 0)
	{
		CHKERR(sdr_begin_xn(sdr));

		ltpConstants = getLtpConstants();
		if (ltpConstants == NULL)
		{
			sdr_exit_xn(sdr);
			putErrmsg("LTP is not initialized.", NULL);
			return 1;
		}

		/*	Print header with timestamp.			*/

		currentTime = getCtime();
		writeTimestampLocal(currentTime, timestamp);
		isprintf(buffer, sizeof buffer,
			"\n=== LTP Session Watch [%s] ===", timestamp);
		PUTS(buffer);

		/*	Process each span.				*/

		spanCount = 0;
		for (sdrElt = sdr_list_first(sdr, ltpConstants->spans);
				sdrElt; sdrElt = sdr_list_next(sdr, sdrElt))
		{
			spanObj = sdr_list_data(sdr, sdrElt);
			sdr_read(sdr, (char *) &span, spanObj,
				sizeof(LtpSpan));

			/*	Apply engine ID filter if specified.	*/

			if (hasEngineFilter
			&& span.engineId != filterEngineId)
			{
				continue;
			}

			processSpan(sdr, spanObj, &span, typeFilter,
				filterSession, activeOnly, countMode);
			spanCount++;
		}

		if (spanCount == 0)
		{
			if (hasEngineFilter)
			{
				isprintf(buffer, sizeof buffer,
					"\nNo span found for engine ID "
					UVAST_FIELDSPEC, filterEngineId);
				PUTS(buffer);
			}
			else
			{
				PUTS("\nNo spans configured.");
			}
		}

		PUTS("");
		sdr_exit_xn(sdr);

		/*	Handle interval and count.			*/

		if (interval == 0)
		{
			break;	/*	One-shot mode.			*/
		}

		oK(ltpwatch_count(&decrement));

		if (ltpwatch_count(NULL) > 0)
		{
			secRemaining = interval;
			while (secRemaining > 0 && ltpwatch_count(NULL) > 0)
			{
				snooze(1);
				secRemaining--;
			}
		}
	}

	return 0;
}

#if defined (ION_LWT)
int	ltpwatch(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	int		argc = (int) a1;
	char		**argv = (char **) a2;
#else
int	main(int argc, char **argv)
{
#endif
	TypeFilter	typeFilter = FILTER_ALL;
	uvast		filterEngineId = 0;
	int		hasEngineFilter = 0;
	unsigned int	filterSession = 0;
	int		activeOnly = 0;
	int		countMode = 0;
	int		interval = 0;
	int		count = 0;
	int		i;
	int		result;

	/*	Parse command-line arguments.				*/

	for (i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-h") == 0
		|| strcmp(argv[i], "--help") == 0)
		{
			printUsage();
			return 0;
		}
		else if (strcmp(argv[i], "-a") == 0)
		{
			activeOnly = 1;
		}
		else if (strcmp(argv[i], "-c") == 0)
		{
			countMode = 1;
		}
		else if (strcmp(argv[i], "-t") == 0)
		{
			if (i + 1 >= argc)
			{
				PUTS("Missing argument for -t");
				printUsage();
				return 1;
			}

			i++;
			if (strcmp(argv[i], "export") == 0)
			{
				typeFilter = FILTER_EXPORT;
			}
			else if (strcmp(argv[i], "import") == 0)
			{
				typeFilter = FILTER_IMPORT;
			}
			else
			{
				PUTS("Invalid type. Use 'import' or 'export'.");
				printUsage();
				return 1;
			}
		}
		else if (strcmp(argv[i], "-e") == 0)
		{
			if (i + 1 >= argc)
			{
				PUTS("Missing argument for -e");
				printUsage();
				return 1;
			}

			i++;
			filterEngineId = getFqn(argv[i]);
			hasEngineFilter = 1;
		}
		else if (strcmp(argv[i], "-s") == 0)
		{
			if (i + 1 >= argc)
			{
				PUTS("Missing argument for -s");
				printUsage();
				return 1;
			}

			i++;
			filterSession = strtoul(argv[i], NULL, 0);
		}
		else if (argv[i][0] == '-')
		{
			char	errBuf[128];

			isprintf(errBuf, sizeof errBuf,
				"Unknown option: %s", argv[i]);
			PUTS(errBuf);
			printUsage();
			return 1;
		}
		else
		{
			/*	Positional argument: interval or count.	*/

			if (interval == 0)
			{
				interval = strtol(argv[i], NULL, 0);
				if (interval < 0)
				{
					interval = 0;
				}
			}
			else if (count == 0)
			{
				count = strtol(argv[i], NULL, 0);
				if (count < 1)
				{
					count = 1;
				}
			}
		}
	}

	/*	Set default count.					*/

	if (interval == 0)
	{
		count = 1;	/*	One-shot mode.			*/
	}
	else if (count == 0)
	{
		count = -1;	/*	Infinite iterations.		*/
	}

	if (count > 0)
	{
		oK(ltpwatch_count(&count));
	}
	else
	{
		/*	For infinite mode, set a very large count.	*/

		count = 2147483647;
		oK(ltpwatch_count(&count));
	}

	/*	Attach to LTP.						*/

	if (ltp_attach() < 0)
	{
		putErrmsg("ltpwatch can't attach to LTP.", NULL);
		return 1;
	}

	/*	Run the watch loop.					*/

	result = runLtpWatch(filterEngineId, hasEngineFilter, typeFilter,
		filterSession, interval, activeOnly, countMode);

	ltp_detach();
	return result;
}
