/*
 *	bpinspect.c - Bundle inspection and management utility
 *
 *	Copyright (c) 2025, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
 *
 *	Author: ION Development Team
 *
 *	This utility provides comprehensive bundle management capabilities:
 *	- List bundles with filtering and sorting
 *	- Display detailed bundle information
 *	- Cancel individual or batches of bundles
 *	- Export bundle details to files
 */

#include "bpinspect_data.h"
#include "bpinspect_filter.h"
#include "bpinspect_ops.h"
#include <signal.h>

/* Command-line options */
typedef struct
{
	int		list;			/* List bundles */
	int		detail;			/* Detail level (0-2) */
	int		cancel;			/* Cancel matching bundles */
	int		dryRun;			/* Dry-run mode (don't actually cancel) */
	int		noConfirm;		/* Skip confirmation prompts */
	char		exportFile[256];	/* Export to file */
	FilterCriteria	filter;			/* Filter criteria */
	SortField	sortField;		/* Sort field */
	int		sortAscending;		/* Sort direction */
	int		maxResults;		/* Maximum results to display */
} CmdOptions;

/* Global state for signal handling */
static int	g_interrupted = 0;

/*
 * Signal handler for clean shutdown
 */
static void handleQuit(int signum)
{
	(void)signum;  /* Suppress unused parameter warning */
	g_interrupted = 1;
	isignal(SIGINT, handleQuit);
}

/*
 * Print usage information
 */
static void printUsage(void)
{
	printf("Usage: bpinspect [options]\n\n");
	printf("Bundle inspection and management utility.\n\n");
	printf("Options:\n");
	printf("  -l                 List bundles (default action)\n");
	printf("  -d <level>         Detail level: 0=summary, 1=detailed, 2=full (default: 0)\n");
	printf("  -c                 Cancel matching bundles (requires confirmation)\n");
	printf("  -n                 No confirmation (use with -c)\n");
	printf("  -D                 Dry-run mode (show what would be done)\n");
	printf("  -e <file>          Export bundle details to file\n\n");
	printf("Filtering options:\n");
	printf("  -s <eid>           Filter by source EID (prefix match)\n");
	printf("  -S <eid>           Filter by source EID (exact match)\n");
	printf("  -t <eid>           Filter by destination EID (prefix match)\n");
	printf("  -T <eid>           Filter by destination EID (exact match)\n");
	printf("  -p <priority>      Filter by priority (0=bulk, 1=std, 2=urgent)\n");
	printf("  -m <bytes>         Filter by minimum payload size\n");
	printf("  -M <bytes>         Filter by maximum payload size\n");
	printf("  -x <seconds>       Filter bundles expiring within N seconds\n");
	printf("  -a                 Show admin bundles only\n");
	printf("  -A                 Show data bundles only (exclude admin)\n");
	printf("  -f                 Show fragments only\n");
	printf("  -q <state>         Filter by queue state (fwd,dlv,xmit,limbo)\n\n");
	printf("Sorting options:\n");
	printf("  -o <field>         Sort by field: time,exp,size,src,dst,pri\n");
	printf("  -r                 Reverse sort order (descending)\n");
	printf("  -L <count>         Limit results to N bundles (default: 1000)\n\n");
	printf("Examples:\n");
	printf("  bpinspect -l                          List all bundles\n");
	printf("  bpinspect -s ipn:1. -d 1              Show bundles from ipn:1.* with details\n");
	printf("  bpinspect -x 60 -o exp                Show bundles expiring within 60s, sorted\n");
	printf("  bpinspect -t ipn:2.1 -c               Cancel bundles to ipn:2.1\n");
	printf("  bpinspect -p 0 -D -c                  Dry-run cancel of bulk priority bundles\n");
	printf("  bpinspect -s ipn:1.1 -e bundles.txt   Export bundles from ipn:1.1\n\n");
}

/*
 * Parse command-line options
 */
static int parseOptions(int argc, char **argv, CmdOptions *opts)
{
	int	c;

	if (opts == NULL)
	{
		return -1;
	}

	/* Initialize defaults */
	memset(opts, 0, sizeof(CmdOptions));
	opts->list = 1;  /* Default action */
	opts->detail = 0;
	opts->cancel = 0;
	opts->dryRun = 0;
	opts->noConfirm = 0;
	opts->sortField = SORT_BY_CREATION_TIME;
	opts->sortAscending = 0;  /* Newest first */
	opts->maxResults = 1000;
	bpinspect_filter_init(&opts->filter);

	/* Parse options */
	while ((c = getopt(argc, argv, "hld:cnDe:s:S:t:T:p:m:M:x:aAfq:o:rL:")) != -1)
	{
		switch (c)
		{
		case 'h':
			printUsage();
			return 1;  /* Exit after showing help */

		case 'l':
			opts->list = 1;
			break;

		case 'd':
			opts->detail = atoi(optarg);
			if (opts->detail < 0 || opts->detail > 2)
			{
				printf("Error: detail level must be 0-2\n");
				return -1;
			}
			break;

		case 'c':
			opts->cancel = 1;
			break;

		case 'n':
			opts->noConfirm = 1;
			break;

		case 'D':
			opts->dryRun = 1;
			break;

		case 'e':
			istrcpy(opts->exportFile, optarg, sizeof(opts->exportFile));
			break;

		case 's':
			istrcpy(opts->filter.sourceFilter, optarg, MAX_EID_LEN);
			opts->filter.sourceMatchMode = MATCH_PREFIX;
			break;

		case 'S':
			istrcpy(opts->filter.sourceFilter, optarg, MAX_EID_LEN);
			opts->filter.sourceMatchMode = MATCH_EXACT;
			break;

		case 't':
			istrcpy(opts->filter.destFilter, optarg, MAX_EID_LEN);
			opts->filter.destMatchMode = MATCH_PREFIX;
			break;

		case 'T':
			istrcpy(opts->filter.destFilter, optarg, MAX_EID_LEN);
			opts->filter.destMatchMode = MATCH_EXACT;
			break;

		case 'p':
			opts->filter.priorityFilter = atoi(optarg);
			if (opts->filter.priorityFilter < 0 ||
			    opts->filter.priorityFilter > 2)
			{
				printf("Error: priority must be 0-2\n");
				return -1;
			}
			break;

		case 'm':
			opts->filter.minSize = strtoll(optarg, NULL, 10);
			break;

		case 'M':
			opts->filter.maxSize = strtoll(optarg, NULL, 10);
			break;

		case 'x':
			opts->filter.expiringWithinSecs = atoi(optarg);
			break;

		case 'a':
			opts->filter.adminOnly = 1;
			opts->filter.dataOnly = 0;
			break;

		case 'A':
			opts->filter.dataOnly = 1;
			opts->filter.adminOnly = 0;
			break;

		case 'f':
			opts->filter.fragmentsOnly = 1;
			break;

		case 'q':
			/* Parse queue state filter */
			opts->filter.showForwarding = 0;
			opts->filter.showDelivery = 0;
			opts->filter.showTransmission = 0;
			opts->filter.showLimbo = 0;

			if (strstr(optarg, "fwd"))
				opts->filter.showForwarding = 1;
			if (strstr(optarg, "dlv"))
				opts->filter.showDelivery = 1;
			if (strstr(optarg, "xmit"))
				opts->filter.showTransmission = 1;
			if (strstr(optarg, "limbo"))
				opts->filter.showLimbo = 1;
			break;

		case 'o':
			/* Parse sort field */
			if (strcmp(optarg, "time") == 0)
				opts->sortField = SORT_BY_CREATION_TIME;
			else if (strcmp(optarg, "exp") == 0)
				opts->sortField = SORT_BY_EXPIRATION_TIME;
			else if (strcmp(optarg, "size") == 0)
				opts->sortField = SORT_BY_SIZE;
			else if (strcmp(optarg, "src") == 0)
				opts->sortField = SORT_BY_SOURCE;
			else if (strcmp(optarg, "dst") == 0)
				opts->sortField = SORT_BY_DEST;
			else if (strcmp(optarg, "pri") == 0)
				opts->sortField = SORT_BY_PRIORITY;
			else
			{
				printf("Error: invalid sort field '%s'\n", optarg);
				return -1;
			}
			break;

		case 'r':
			opts->sortAscending = 1;
			break;

		case 'L':
			opts->maxResults = atoi(optarg);
			if (opts->maxResults <= 0)
			{
				printf("Error: max results must be > 0\n");
				return -1;
			}
			break;

		default:
			printUsage();
			return -1;
		}
	}

	return 0;
}

/*
 * Format bundle for list display (one line)
 */
static void format_bundle_line(const BundleCacheEntry *entry, char *buf, int len)
{
	char	sizeBuf[32];  /* Increased from 16 to avoid truncation warning */
	char	ttlBuf[16];
	char	priChar;

	/* Format size */
	if (entry->payloadLength < 1024)
		snprintf(sizeBuf, sizeof(sizeBuf), "%lldB",
			 (long long) entry->payloadLength);
	else if (entry->payloadLength < 1024 * 1024)
		snprintf(sizeBuf, sizeof(sizeBuf), "%.1fK",
			 (double) entry->payloadLength / 1024.0);
	else
		snprintf(sizeBuf, sizeof(sizeBuf), "%.1fM",
			 (double) entry->payloadLength / (1024.0 * 1024.0));

	/* Format TTL */
	if (entry->timeRemaining < 0)
		snprintf(ttlBuf, sizeof(ttlBuf), "EXP");
	else if (entry->timeRemaining < 60)
		snprintf(ttlBuf, sizeof(ttlBuf), "%ds", entry->timeRemaining);
	else if (entry->timeRemaining < 3600)
		snprintf(ttlBuf, sizeof(ttlBuf), "%dm",
			 entry->timeRemaining / 60);
	else
		snprintf(ttlBuf, sizeof(ttlBuf), "%dh",
			 entry->timeRemaining / 3600);

	/* Priority character */
	switch (entry->priority)
	{
	case 0: priChar = 'B'; break;
	case 1: priChar = 'S'; break;
	case 2: priChar = 'U'; break;
	default: priChar = '?'; break;
	}

	/* Format line */
	snprintf(buf, len, "%-24s %-24s %8s %6s %c %-10s",
		 entry->source,
		 entry->dest,
		 sizeBuf,
		 ttlBuf,
		 priChar,
		 entry->queueState);
}

/*
 * List bundles in table format
 */
static void list_bundles(const BundleCacheEntry *entries, int count, int detail)
{
	int	i;
	char	lineBuf[256];

	if (count == 0)
	{
		printf("No bundles found matching criteria.\n");
		return;
	}

	printf("\n");
	printf("Total bundles: %d\n\n", count);

	if (detail == 0)
	{
		/* Table header */
		printf("%-24s %-24s %8s %6s %s %-10s\n",
		       "Source", "Destination", "Size", "TTL", "P", "Queue");
		printf("%-24s %-24s %8s %6s %s %-10s\n",
		       "------------------------",
		       "------------------------",
		       "--------",
		       "------",
		       "-",
		       "----------");

		/* List each bundle */
		for (i = 0; i < count; i++)
		{
			format_bundle_line(&entries[i], lineBuf, sizeof(lineBuf));
			printf("%s\n", lineBuf);
		}
	}
	else
	{
		/* Detailed listing */
		for (i = 0; i < count; i++)
		{
			printf("Bundle %d of %d:\n", i + 1, count);
			bpinspect_ops_print_bundle(&entries[i], detail);

			if (g_interrupted)
			{
				printf("\nInterrupted by user.\n");
				break;
			}
		}
	}

	printf("\n");
}

/*
 * Confirm action with user
 */
static int confirm_action(const char *message, int count)
{
	char	response[16];

	printf("\n%s (%d bundles)\n", message, count);
	printf("Are you sure? (yes/no): ");
	fflush(stdout);

	if (fgets(response, sizeof(response), stdin) == NULL)
	{
		return 0;
	}

	/* Check for affirmative response */
	if (strcmp(response, "yes\n") == 0 || strcmp(response, "y\n") == 0)
	{
		return 1;
	}

	return 0;
}

/*
 * Cancel bundles
 */
static int cancel_bundles(BundleCacheEntry *entries, int count,
			  int dryRun, int noConfirm)
{
	int	i;
	int	canceledCount;

	if (count == 0)
	{
		printf("No bundles to cancel.\n");
		return 0;
	}

	/* Show what will be canceled */
	printf("\nBundles to cancel:\n\n");
	for (i = 0; i < count && i < 20; i++)  /* Show first 20 */
	{
		char	lineBuf[256];
		format_bundle_line(&entries[i], lineBuf, sizeof(lineBuf));
		printf("  %s\n", lineBuf);
	}

	if (count > 20)
	{
		printf("  ... and %d more\n", count - 20);
	}

	/* Dry-run mode - just show what would be done */
	if (dryRun)
	{
		printf("\nDry-run mode: would cancel %d bundles\n", count);
		return 0;
	}

	/* Confirm action */
	if (!noConfirm)
	{
		if (!confirm_action("Cancel bundles?", count))
		{
			printf("Canceled.\n");
			return 0;
		}
	}

	/* Cancel bundles */
	printf("\nCanceling bundles...\n");
	canceledCount = bpinspect_ops_cancel_bundles(entries, count);

	if (canceledCount < 0)
	{
		printf("Error canceling bundles.\n");
		return -1;
	}

	printf("Canceled %d of %d bundles.\n", canceledCount, count);
	return 0;
}

/*
 * Main function
 */
int main(int argc, char **argv)
{
	CmdOptions		opts;
	BundleListState		state;
	BundleCacheEntry	*filtered = NULL;
	int			filteredCount;
	int			result;

	/* Parse command-line options */
	result = parseOptions(argc, argv, &opts);
	if (result != 0)
	{
		return (result < 0) ? 1 : 0;
	}

	/* Set up signal handler */
	isignal(SIGINT, handleQuit);

	/* Attach to BP */
	if (bp_attach() < 0)
	{
		putErrmsg("Can't attach to BP.", NULL);
		writeErrmsgMemos();
		return 1;
	}

	/* Initialize bundle list state */
	if (bpinspect_data_init(&state) < 0)
	{
		putErrmsg("Can't initialize bundle list.", NULL);
		writeErrmsgMemos();
		bp_detach();
		return 1;
	}

	/* Enumerate bundles */
	printf("Enumerating bundles...\n");
	result = bpinspect_data_refresh(&state);
	if (result < 0)
	{
		putErrmsg("Can't enumerate bundles.", NULL);
		writeErrmsgMemos();
		bpinspect_data_cleanup(&state);
		bp_detach();
		return 1;
	}

	printf("Found %d total bundles.\n", result);

	/* Sort bundles */
	if (bpinspect_data_sort(&state, opts.sortField, opts.sortAscending) < 0)
	{
		putErrmsg("Can't sort bundles.", NULL);
		writeErrmsgMemos();
		bpinspect_data_cleanup(&state);
		bp_detach();
		return 1;
	}

	/* Apply filter */
	filteredCount = bpinspect_filter_apply(&state, &opts.filter,
					       &filtered, opts.maxResults);
	if (filteredCount < 0)
	{
		putErrmsg("Can't filter bundles.", NULL);
		writeErrmsgMemos();
		bpinspect_data_cleanup(&state);
		bp_detach();
		return 1;
	}

	printf("Matched %d bundles after filtering.\n", filteredCount);

	/* Perform requested action */
	if (opts.cancel)
	{
		/* Cancel bundles */
		result = cancel_bundles(filtered, filteredCount,
					opts.dryRun, opts.noConfirm);
	}
	else if (opts.exportFile[0] != '\0')
	{
		/* Export to file */
		int	i;

		printf("Exporting %d bundles to %s...\n",
		       filteredCount, opts.exportFile);

		for (i = 0; i < filteredCount; i++)
		{
			if (bpinspect_ops_export_bundle(&filtered[i],
							opts.exportFile,
							opts.detail) < 0)
			{
				printf("Error exporting bundle %d\n", i + 1);
			}

			if (g_interrupted)
			{
				printf("\nInterrupted by user.\n");
				break;
			}
		}

		printf("Export complete.\n");
	}
	else
	{
		/* List bundles */
		list_bundles(filtered, filteredCount, opts.detail);
	}

	/* Cleanup */
	if (filtered != NULL)
	{
		MRELEASE(filtered);
	}

	bpinspect_data_cleanup(&state);
	bp_detach();

	return 0;
}
