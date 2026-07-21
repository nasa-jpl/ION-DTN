/*
 *	bpwatch.c:	BP statistics monitor.
 *
 *	Author: ION Development Team
 *
 *	Copyright (c) 2026, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 */

#include "bpP.h"

#define MODE_SUMMARY	0
#define MODE_PLANS	1
#define MODE_ENDPOINT	2

static unsigned int	bpwatch_count(int *newValue)
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
	oK(bpwatch_count(&newCount));
}

static void	formatBytes(uvast bytes, char *buf, size_t buflen)
{
	if (bytes == 0)
	{
		isprintf(buf, buflen, "%s", "0");
	}
	else if (bytes < 1024)
	{
		isprintf(buf, buflen, UVAST_FIELDSPEC, bytes);
	}
	else if (bytes < 1048576)
	{
		isprintf(buf, buflen, UVAST_FIELDSPEC "K",
				bytes / 1024);
	}
	else if (bytes < (uvast) 1073741824)
	{
		isprintf(buf, buflen, UVAST_FIELDSPEC "M",
				bytes / 1048576);
	}
	else
	{
		isprintf(buf, buflen, UVAST_FIELDSPEC "G",
				bytes / (uvast) 1073741824);
	}
}

static void	formatScalar(Scalar *s, char *buf, size_t buflen)
{
	uvast	total;

	total = ((uvast) s->gigs * 1000000000) + (uvast) s->units;
	formatBytes(total, buf, buflen);
}

static void	printCosRow(const char *label, BpCosStats *stats)
{
	char	buffer[256];
	char	b0[16], b1[16], b2[16], bt[16];
	uvast	totalCount;
	uvast	totalBytes;

	totalCount = (uvast) stats->tallies[0].currentCount
			+ (uvast) stats->tallies[1].currentCount
			+ (uvast) stats->tallies[2].currentCount;
	totalBytes = stats->tallies[0].currentBytes
			+ stats->tallies[1].currentBytes
			+ stats->tallies[2].currentBytes;

	formatBytes(stats->tallies[0].currentBytes, b0, sizeof b0);
	formatBytes(stats->tallies[1].currentBytes, b1, sizeof b1);
	formatBytes(stats->tallies[2].currentBytes, b2, sizeof b2);
	formatBytes(totalBytes, bt, sizeof bt);

	isprintf(buffer, sizeof buffer,
		"  %-13s %6u/%-6s %6u/%-6s %6u/%-6s %6u/%-6s",
		label,
		stats->tallies[0].currentCount, b0,
		stats->tallies[1].currentCount, b1,
		stats->tallies[2].currentCount, b2,
		(unsigned int) totalCount, bt);
	PUTS(buffer);
}

static void	printDbRow(const char *label, Tally *tally)
{
	char	buffer[256];
	char	b[16];

	formatBytes(tally->currentBytes, b, sizeof b);
	isprintf(buffer, sizeof buffer, "  %-13s %6u     %s",
		label, tally->currentCount, b);
	PUTS(buffer);
}

static void	displaySummary(Sdr sdr, BpDB *bpdb)
{
	BpCosStats	sourceStats;
	BpCosStats	xmitStats;
	BpCosStats	recvStats;
	BpDbStats	dbStats;
	int		transitLen;
	int		limboLen;
	char		buffer[256];
	char		fromTimestamp[TIMESTAMPBUFSZ];

	/*	Queue depths.						*/

	transitLen = sdr_list_length(sdr, bpdb->transit);
	limboLen = sdr_list_length(sdr, bpdb->limboQueue);

	writeTimestampLocal(bpdb->resetTime, fromTimestamp);
	isprintf(buffer, sizeof buffer,
		"\nStats since %s    Transit: %d    Limbo: %d",
		fromTimestamp, transitLen, limboLen);
	PUTS(buffer);

	/*	Per-priority stats header.				*/

	PUTS("");
	PUTS("  Category      Bulk           Std            Exp"
		"            Total");
	PUTS("                cnt/bytes      cnt/bytes      cnt/bytes"
		"      cnt/bytes");
	PUTS("  ------------- -------------- -------------- --------"
		"------ --------------");

	/*	CosStats rows: Sourced, Transmitted, Received.		*/

	sdr_read(sdr, (char *) &sourceStats, bpdb->sourceStats,
			sizeof(BpCosStats));
	printCosRow("Sourced", &sourceStats);

	sdr_read(sdr, (char *) &xmitStats, bpdb->xmitStats,
			sizeof(BpCosStats));
	printCosRow("Transmitted", &xmitStats);

	sdr_read(sdr, (char *) &recvStats, bpdb->recvStats,
			sizeof(BpCosStats));
	printCosRow("Received", &recvStats);

	/*	DbStats rows.						*/

	sdr_read(sdr, (char *) &dbStats, bpdb->dbStats,
			sizeof(BpDbStats));

	PUTS("");
	PUTS("  Category      Count      Bytes");
	PUTS("  ------------- ---------- ----------");
	printDbRow("Forwarded",
			&dbStats.tallies[BP_DB_FWD_OKAY]);
	printDbRow("Reforwarded",
			&dbStats.tallies[BP_DB_REQUEUED_FOR_FWD]);
	printDbRow("Expired",
			&dbStats.tallies[BP_DB_EXPIRED]);
	printDbRow("Abandoned",
			&dbStats.tallies[BP_DB_ABANDON]);

	/*	Delivery: aggregate from all endpoints.			*/

	{
		SdrObject	schemeElt;
		SdrObject	endpointElt;
		Scheme		scheme;
		Endpoint	endpoint;
		EndpointStats	epStats;
		unsigned int	dlvCount = 0;
		uvast		dlvBytes = 0;
		char		b[16];

		for (schemeElt = sdr_list_first(sdr, bpdb->schemes);
				schemeElt;
				schemeElt = sdr_list_next(sdr, schemeElt))
		{
			sdr_read(sdr, (char *) &scheme,
				sdr_list_data(sdr, schemeElt),
				sizeof(Scheme));
			for (endpointElt = sdr_list_first(sdr,
					scheme.endpoints);
					endpointElt;
					endpointElt = sdr_list_next(sdr,
					endpointElt))
			{
				sdr_read(sdr, (char *) &endpoint,
					sdr_list_data(sdr, endpointElt),
					sizeof(Endpoint));
				if (endpoint.stats == 0)
				{
					continue;
				}

				sdr_read(sdr, (char *) &epStats,
					endpoint.stats,
					sizeof(EndpointStats));
				dlvCount +=
					epStats.tallies[BP_ENDPOINT_DELIVERED]
					.currentCount;
				dlvBytes +=
					epStats.tallies[BP_ENDPOINT_DELIVERED]
					.currentBytes;
			}
		}

		formatBytes(dlvBytes, b, sizeof b);
		isprintf(buffer, sizeof buffer,
			"  %-13s %6u     %s", "Delivered",
			dlvCount, b);
		PUTS(buffer);
	}

	/*	Fragmentation summary.					*/

	isprintf(buffer, sizeof buffer,
		"\n  Fragmentation: " VAST_FIELDSPEC " bundles, "
		VAST_FIELDSPEC " fragments produced",
		bpdb->currentBundlesFragmented,
		bpdb->currentFragmentsProduced);
	PUTS(buffer);
}

static void	displayPlanStats(Sdr sdr, BpDB *bpdb)
{
	SdrObject planElt;
	SdrObject planObj;
	BpPlan	plan;
	char	buffer[256];
	int	bulkLen;
	int	stdLen;
	int	urgLen;
	char	bBulk[16], bStd[16], bUrg[16];

	for (planElt = sdr_list_first(sdr, bpdb->plans);
			planElt;
			planElt = sdr_list_next(sdr, planElt))
	{
		planObj = sdr_list_data(sdr, planElt);
		sdr_read(sdr, (char *) &plan, planObj, sizeof(BpPlan));

		isprintf(buffer, sizeof buffer,
			"\n--- Plan %s (rate: %u B/s%s) ---",
			plan.neighborEid, plan.nominalRate,
			plan.blocked ? ", BLOCKED" : "");
		PUTS(buffer);

		bulkLen = sdr_list_length(sdr, plan.bulkQueue);
		stdLen = sdr_list_length(sdr, plan.stdQueue);
		urgLen = sdr_list_length(sdr, plan.urgentQueue);

		formatScalar(&plan.bulkBacklog, bBulk, sizeof bBulk);
		formatScalar(&plan.stdBacklog, bStd, sizeof bStd);
		formatScalar(&plan.urgentBacklog, bUrg, sizeof bUrg);

		PUTS("");
		PUTS("  Queue       Bundles    Backlog");
		PUTS("  ---------   --------   ----------");
		isprintf(buffer, sizeof buffer,
			"  Bulk        %6d     %s", bulkLen, bBulk);
		PUTS(buffer);
		isprintf(buffer, sizeof buffer,
			"  Standard    %6d     %s", stdLen, bStd);
		PUTS(buffer);
		isprintf(buffer, sizeof buffer,
			"  Urgent      %6d     %s", urgLen, bUrg);
		PUTS(buffer);

		/*	Plan enqueue/dequeue stats if available.	*/

		if (plan.stats != 0)
		{
			PlanStats	planStats;
			char		be[16], bd[16];

			sdr_read(sdr, (char *) &planStats, plan.stats,
					sizeof(PlanStats));
			formatBytes(
				planStats.tallies[BP_PLAN_ENQUEUED]
				.currentBytes, be, sizeof be);
			formatBytes(
				planStats.tallies[BP_PLAN_DEQUEUED]
				.currentBytes, bd, sizeof bd);
			isprintf(buffer, sizeof buffer,
				"\n  Enqueued: %u (%s)    Dequeued: %u (%s)",
				planStats.tallies[BP_PLAN_ENQUEUED]
				.currentCount, be,
				planStats.tallies[BP_PLAN_DEQUEUED]
				.currentCount, bd);
			PUTS(buffer);
		}
	}
}

static void	displayEndpointStats(Sdr sdr, BpDB *bpdb,
			const char *filterNss)
{
	SdrObject	schemeElt;
	SdrObject	endpointElt;
	Scheme		scheme;
	Endpoint	endpoint;
	EndpointStats	epStats;
	int		found = 0;
	char		buffer[256];
	char		bs[16], bd[16];
	int		dlvQueueLen;

	for (schemeElt = sdr_list_first(sdr, bpdb->schemes);
			schemeElt;
			schemeElt = sdr_list_next(sdr, schemeElt))
	{
		sdr_read(sdr, (char *) &scheme,
			sdr_list_data(sdr, schemeElt),
			sizeof(Scheme));

		for (endpointElt = sdr_list_first(sdr,
				scheme.endpoints);
				endpointElt;
				endpointElt = sdr_list_next(sdr,
				endpointElt))
		{
			sdr_read(sdr, (char *) &endpoint,
				sdr_list_data(sdr, endpointElt),
				sizeof(Endpoint));

			if (filterNss != NULL
			&& strcmp(endpoint.nss, filterNss) != 0)
			{
				continue;
			}

			found = 1;

			isprintf(buffer, sizeof buffer,
				"\n--- Endpoint %s:%s ---",
				scheme.name, endpoint.nss);
			PUTS(buffer);

			dlvQueueLen = sdr_list_length(sdr,
					endpoint.deliveryQueue);

			if (endpoint.stats == 0)
			{
				isprintf(buffer, sizeof buffer,
					"  Delivery queue: %d"
					"    (stats not enabled)",
					dlvQueueLen);
				PUTS(buffer);
				continue;
			}

			sdr_read(sdr, (char *) &epStats,
				endpoint.stats,
				sizeof(EndpointStats));

			formatBytes(
				epStats.tallies[BP_ENDPOINT_SOURCED]
				.currentBytes, bs, sizeof bs);
			formatBytes(
				epStats.tallies[BP_ENDPOINT_DELIVERED]
				.currentBytes, bd, sizeof bd);

			isprintf(buffer, sizeof buffer,
				"  Sourced: %u (%s)    "
				"Queued: %d    "
				"Abandoned: %u    "
				"Delivered: %u (%s)",
				epStats.tallies[BP_ENDPOINT_SOURCED]
				.currentCount, bs,
				dlvQueueLen,
				epStats.tallies[BP_ENDPOINT_ABANDONED]
				.currentCount,
				epStats.tallies[BP_ENDPOINT_DELIVERED]
				.currentCount, bd);
			PUTS(buffer);
		}
	}

	if (!found)
	{
		if (filterNss != NULL)
		{
			isprintf(buffer, sizeof buffer,
				"\nNo endpoint found matching '%s'.",
				filterNss);
			PUTS(buffer);
		}
		else
		{
			PUTS("\nNo endpoints registered.");
		}
	}
}

static int	runBpWatch(int mode, const char *filter, int interval)
{
	Sdr		sdr;
	SdrObject	bpDbObject;
	BpDB		bpdb;
	char		buffer[256];
	char		timestamp[TIMESTAMPBUFSZ];
	time_t		currentTime;
	int		secRemaining;
	int		decrement = 0;

	sdr = getIonsdr();
	bpDbObject = getBpDbObject();

	isignal(SIGTERM, handleQuit);
	isignal(SIGINT, handleQuit);

	while (bpwatch_count(NULL) > 0)
	{
		CHKERR(sdr_begin_xn(sdr));

		sdr_read(sdr, (char *) &bpdb, bpDbObject, sizeof(BpDB));

		/*	Print header with timestamp.			*/

		currentTime = getCtime();
		writeTimestampLocal(currentTime, timestamp);
		isprintf(buffer, sizeof buffer,
			"\n=== BP Statistics Watch [%s] ===", timestamp);
		PUTS(buffer);

		switch (mode)
		{
		case MODE_PLANS:
			displayPlanStats(sdr, &bpdb);
			break;

		case MODE_ENDPOINT:
			displayEndpointStats(sdr, &bpdb, filter);
			break;

		default:
			displaySummary(sdr, &bpdb);
		}

		PUTS("");
		sdr_exit_xn(sdr);

		/*	Handle interval and count.			*/

		if (interval == 0)
		{
			break;	/*	One-shot mode.			*/
		}

		oK(bpwatch_count(&decrement));

		if (bpwatch_count(NULL) > 0)
		{
			secRemaining = interval;
			while (secRemaining > 0 && bpwatch_count(NULL) > 0)
			{
				snooze(1);
				secRemaining--;
			}
		}
	}

	return 0;
}

static void	printUsage(void)
{
	PUTS("Usage: bpwatch [-p] [-e <nss>] [-h] [<interval> [<count>]]");
	PUTS("");
	PUTS("Options:");
	PUTS("  -p            Per-plan queue and backlog view");
	PUTS("  -e <nss>      Per-endpoint stats (e.g. -e 1.1)");
	PUTS("  -h            Show this help message");
	PUTS("");
	PUTS("Arguments:");
	PUTS("  interval      Refresh interval in seconds \
(default: 0 = one-shot)");
	PUTS("  count         Number of iterations (default: infinite \
if interval>0, 1 if interval=0)");
	PUTS("");
	PUTS("Examples:");
	PUTS("  bpwatch                One-shot summary");
	PUTS("  bpwatch 5              Refresh every 5 seconds");
	PUTS("  bpwatch 2 10           Refresh 10 times, every 2 sec");
	PUTS("  bpwatch -p             Per-plan queue depths");
	PUTS("  bpwatch -e 1.1 3       Endpoint ipn:*.1.1, every 3 sec");
}

#if defined (ION_LWT)
int	bpwatch(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	int		argc = (int) a1;
	char		**argv = (char **) a2;
#else
int	main(int argc, char **argv)
{
#endif
	int		mode = MODE_SUMMARY;
	const char	*filterNss = NULL;
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
		else if (strcmp(argv[i], "-p") == 0)
		{
			mode = MODE_PLANS;
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
			mode = MODE_ENDPOINT;
			filterNss = argv[i];
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
		oK(bpwatch_count(&count));
	}
	else
	{
		/*	For infinite mode, set a very large count.	*/

		count = 2147483647;
		oK(bpwatch_count(&count));
	}

	/*	Attach to BP.						*/

	if (bp_attach() < 0)
	{
		putErrmsg("bpwatch can't attach to BP.", NULL);
		return 1;
	}

	/*	Run the watch loop.					*/

	result = runBpWatch(mode, filterNss, interval);

	bp_detach();
	return result;
}
