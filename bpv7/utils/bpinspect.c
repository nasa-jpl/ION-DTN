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

/* ANSI color codes */
#define ANSI_RESET		"\033[0m"
#define ANSI_BOLD		"\033[1m"
#define ANSI_RED		"\033[31m"
#define ANSI_GREEN		"\033[32m"
#define ANSI_YELLOW		"\033[33m"
#define ANSI_BLUE		"\033[34m"
#define ANSI_CYAN		"\033[36m"
#define ANSI_WHITE		"\033[37m"
#define ANSI_BOLD_RED		"\033[1;31m"
#define ANSI_BOLD_YELLOW	"\033[1;33m"

/* Box-drawing characters (UTF-8) */
#define BOX_H			"\xE2\x94\x80"	/* ─ horizontal */
#define BOX_V			"\xE2\x94\x82"	/* │ vertical */
#define BOX_TL			"\xE2\x94\x8C"	/* ┌ top-left */
#define BOX_TR			"\xE2\x94\x90"	/* ┐ top-right */
#define BOX_BL			"\xE2\x94\x94"	/* └ bottom-left */
#define BOX_BR			"\xE2\x94\x98"	/* ┘ bottom-right */
#define BOX_VR			"\xE2\x94\x9C"	/* ├ vertical-right */
#define BOX_VL			"\xE2\x94\x24"	/* ┤ vertical-left */
#define BOX_HU			"\xE2\x94\xB4"	/* ┴ horizontal-up */
#define BOX_HD			"\xE2\x94\xAC"	/* ┬ horizontal-down */
#define BOX_CROSS		"\xE2\x94\xBC"	/* ┼ cross */

/* Color support state */
static int	g_useColor = 0;

/* Command-line options */
typedef struct
{
	int		list;			/* List bundles */
	int		detail;			/* Detail level (0-2) */
	int		cancel;			/* Cancel matching bundles */
	int		suspend;		/* Suspend matching bundles */
	int		resume;			/* Resume matching bundles */
	int		dryRun;			/* Dry-run mode (don't actually cancel) */
	int		noConfirm;		/* Skip confirmation prompts */
	int		useColor;		/* Use ANSI colors (0=auto, 1=on, -1=off) */
	char		exportFile[256];	/* Export to file */
	FilterCriteria	filter;			/* Filter criteria */
	SortField	sortField;		/* Sort field */
	int		sortAscending;		/* Sort direction */
	int		maxResults;		/* Maximum results to display */
} CmdOptions;

/* Global state for signal handling */
static int	g_interrupted = 0;

/*
 * Detect if terminal supports color
 */
static int detect_color_support(void)
{
	const char	*term;
	const char	*noColor;

	/* Check NO_COLOR environment variable */
	noColor = getenv("NO_COLOR");
	if (noColor != NULL && noColor[0] != '\0')
	{
		return 0;
	}

	/* Check if stdout is a terminal */
	if (!isatty(fileno(stdout)))
	{
		return 0;
	}

	/* Check TERM environment variable */
	term = getenv("TERM");
	if (term == NULL)
	{
		return 0;
	}

	/* Common terminals that support color */
	if (strstr(term, "color") != NULL ||
	    strstr(term, "xterm") != NULL ||
	    strstr(term, "screen") != NULL ||
	    strstr(term, "tmux") != NULL ||
	    strstr(term, "linux") != NULL)
	{
		return 1;
	}

	return 0;
}

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
	printf("  -u                 Suspend matching bundles (requires confirmation)\n");
	printf("  -R                 Resume matching bundles (requires confirmation)\n");
	printf("  -n                 No confirmation (use with -c, -u, or -R)\n");
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
	printf("Display options:\n");
	printf("  --color            Force color output\n");
	printf("  --no-color         Disable color output\n\n");
	printf("Examples:\n");
	printf("  bpinspect -l                          List all bundles\n");
	printf("  bpinspect -s ipn:1. -d 1              Show bundles from ipn:1.* with details\n");
	printf("  bpinspect -x 60 -o exp                Show bundles expiring within 60s, sorted\n");
	printf("  bpinspect -t ipn:2.1 -c               Cancel bundles to ipn:2.1\n");
	printf("  bpinspect -t ipn:2.1 -u               Suspend bundles to ipn:2.1\n");
	printf("  bpinspect -q limbo -R                 Resume all bundles in limbo\n");
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
	opts->suspend = 0;
	opts->resume = 0;
	opts->dryRun = 0;
	opts->noConfirm = 0;
	opts->useColor = 0;  /* Auto-detect */
	opts->sortField = SORT_BY_CREATION_TIME;
	opts->sortAscending = 0;  /* Newest first */
	opts->maxResults = 1000;
	bpinspect_filter_init(&opts->filter);

	/* Check for --color and --no-color before getopt */
	for (c = 1; c < argc; c++)
	{
		if (strcmp(argv[c], "--color") == 0)
		{
			opts->useColor = 1;
		}
		else if (strcmp(argv[c], "--no-color") == 0)
		{
			opts->useColor = -1;
		}
	}

	/* Parse options */
	while ((c = getopt(argc, argv, "hld:cnDe:s:S:t:T:p:m:M:x:aAfq:o:rL:uR")) != -1)
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

		case 'u':
			opts->suspend = 1;
			break;

		case 'R':
			opts->resume = 1;
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
 * Get color for priority
 */
static const char *get_priority_color(int priority)
{
	if (!g_useColor)
		return "";

	switch (priority)
	{
	case 0:  return ANSI_BLUE;		/* Bulk */
	case 1:  return ANSI_WHITE;		/* Standard */
	case 2:  return ANSI_BOLD_YELLOW;	/* Urgent */
	default: return ANSI_WHITE;
	}
}

/*
 * Get color for queue state
 */
static const char *get_queue_color(const char *queueState)
{
	if (!g_useColor)
		return "";

	if (strstr(queueState, "Forward"))
		return ANSI_GREEN;
	else if (strstr(queueState, "Deliver"))
		return ANSI_CYAN;
	else if (strstr(queueState, "Transmit"))
		return ANSI_YELLOW;
	else if (strstr(queueState, "Limbo"))
		return ANSI_RED;
	else if (strstr(queueState, "Planned"))
		return ANSI_YELLOW;

	return ANSI_WHITE;
}

/*
 * Get color for TTL warning
 */
static const char *get_ttl_color(int timeRemaining)
{
	if (!g_useColor)
		return "";

	if (timeRemaining < 0)
		return ANSI_BOLD_RED;	/* Expired */
	else if (timeRemaining < 300)
		return ANSI_BOLD_RED;	/* < 5 minutes */
	else if (timeRemaining < 3600)
		return ANSI_YELLOW;	/* < 1 hour */

	return "";
}

/*
 * Format bundle for list display (one line)
 */
static void format_bundle_line(const BundleCacheEntry *entry, char *buf, int len)
{
	char		sizeBuf[32];
	char		ttlBuf[16];
	char		priChar;
	const char	*priColor;
	const char	*queueColor;
	const char	*ttlColor;
	const char	*reset;

	/* Get colors */
	priColor = get_priority_color(entry->priority);
	queueColor = get_queue_color(entry->queueState);
	ttlColor = get_ttl_color(entry->timeRemaining);
	reset = g_useColor ? ANSI_RESET : "";

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

	/* Format line with colors */
	if (g_useColor)
	{
		snprintf(buf, len, "%-24s %-24s %8s %s%6s%s %s%c%s %s%-10s%s",
			 entry->source,
			 entry->dest,
			 sizeBuf,
			 ttlColor, ttlBuf, reset,
			 priColor, priChar, reset,
			 queueColor, entry->queueState, reset);
	}
	else
	{
		snprintf(buf, len, "%-24s %-24s %8s %6s %c %-10s",
			 entry->source,
			 entry->dest,
			 sizeBuf,
			 ttlBuf,
			 priChar,
			 entry->queueState);
	}
}

/*
 * List bundles in table format
 */
static void list_bundles(const BundleCacheEntry *entries, int count, int detail)
{
	int		i;
	char		lineBuf[512];
	const char	*bold;
	const char	*reset;

	if (count == 0)
	{
		printf("No bundles found matching criteria.\n");
		return;
	}

	bold = g_useColor ? ANSI_BOLD : "";
	reset = g_useColor ? ANSI_RESET : "";

	printf("\n");
	printf("Total bundles: %d\n\n", count);

	if (detail == 0)
	{
		/* Top border */
		if (g_useColor)
		{
			printf("%s", BOX_TL);
			for (i = 0; i < 24; i++) printf("%s", BOX_H);
			printf("%s", BOX_HD);
			for (i = 0; i < 24; i++) printf("%s", BOX_H);
			printf("%s", BOX_HD);
			for (i = 0; i < 8; i++) printf("%s", BOX_H);
			printf("%s", BOX_HD);
			for (i = 0; i < 6; i++) printf("%s", BOX_H);
			printf("%s", BOX_HD);
			printf("%s", BOX_H);
			printf("%s", BOX_HD);
			for (i = 0; i < 10; i++) printf("%s", BOX_H);
			printf("%s\n", BOX_TR);
		}
		else
		{
			printf("+------------------------+------------------------+--------+------+-+----------+\n");
		}

		/* Table header */
		if (g_useColor)
		{
			printf("%s%s%-24s%s%s%s%-24s%s%s%8s%s%6s%s%s%s%s%-10s%s%s\n",
				BOX_V, bold, "Source", reset, BOX_V,
				bold, "Destination", reset, BOX_V,
				"Size", BOX_V,
				"TTL", BOX_V,
				"P", BOX_V,
				bold, "Queue", reset, BOX_V);
		}
		else
		{
			printf("|%-24s|%-24s|%8s|%6s|%s|%-10s|\n",
				"Source", "Destination", "Size", "TTL", "P", "Queue");
		}

		/* Header separator */
		if (g_useColor)
		{
			printf("%s", BOX_VR);
			for (i = 0; i < 24; i++) printf("%s", BOX_H);
			printf("%s", BOX_CROSS);
			for (i = 0; i < 24; i++) printf("%s", BOX_H);
			printf("%s", BOX_CROSS);
			for (i = 0; i < 8; i++) printf("%s", BOX_H);
			printf("%s", BOX_CROSS);
			for (i = 0; i < 6; i++) printf("%s", BOX_H);
			printf("%s", BOX_CROSS);
			printf("%s", BOX_H);
			printf("%s", BOX_CROSS);
			for (i = 0; i < 10; i++) printf("%s", BOX_H);
			printf("%s\n", BOX_VL);
		}
		else
		{
			printf("+------------------------+------------------------+--------+------+-+----------+\n");
		}

		/* List each bundle */
		for (i = 0; i < count; i++)
		{
			format_bundle_line(&entries[i], lineBuf, sizeof(lineBuf));
			if (g_useColor)
			{
				printf("%s%s%s\n", BOX_V, lineBuf, BOX_V);
			}
			else
			{
				printf("|%s|\n", lineBuf);
			}
		}

		/* Bottom border */
		if (g_useColor)
		{
			printf("%s", BOX_BL);
			for (i = 0; i < 24; i++) printf("%s", BOX_H);
			printf("%s", BOX_HU);
			for (i = 0; i < 24; i++) printf("%s", BOX_H);
			printf("%s", BOX_HU);
			for (i = 0; i < 8; i++) printf("%s", BOX_H);
			printf("%s", BOX_HU);
			for (i = 0; i < 6; i++) printf("%s", BOX_H);
			printf("%s", BOX_HU);
			printf("%s", BOX_H);
			printf("%s", BOX_HU);
			for (i = 0; i < 10; i++) printf("%s", BOX_H);
			printf("%s\n", BOX_BR);
		}
		else
		{
			printf("+------------------------+------------------------+--------+------+-+----------+\n");
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
 * Suspend bundles
 */
static int suspend_bundles(BundleCacheEntry *entries, int count,
		int dryRun, int noConfirm)
{
	int	i;
	int	suspendedCount;

	if (count == 0)
	{
		printf("No bundles to suspend.\n");
		return 0;
	}

	/* Show what will be suspended */
	printf("\nBundles to suspend:\n\n");
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
		printf("\nDry-run mode: would suspend %d bundles\n", count);
		return 0;
	}

	/* Confirm action */
	if (!noConfirm)
	{
		if (!confirm_action("Suspend bundles?", count))
		{
			printf("Canceled.\n");
			return 0;
		}
	}

	/* Suspend bundles */
	printf("\nSuspending bundles...\n");
	suspendedCount = bpinspect_ops_suspend_bundles(entries, count);

	if (suspendedCount < 0)
	{
		printf("Error suspending bundles.\n");
		return -1;
	}

	printf("Suspended %d of %d bundles.\n", suspendedCount, count);
	return 0;
}

/*
 * Resume bundles
 */
static int resume_bundles(BundleCacheEntry *entries, int count,
		int dryRun, int noConfirm)
{
	int	i;
	int	resumedCount;

	if (count == 0)
	{
		printf("No bundles to resume.\n");
		return 0;
	}

	/* Show what will be resumed */
	printf("\nBundles to resume:\n\n");
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
		printf("\nDry-run mode: would resume %d bundles\n", count);
		return 0;
	}

	/* Confirm action */
	if (!noConfirm)
	{
		if (!confirm_action("Resume bundles?", count))
		{
			printf("Canceled.\n");
			return 0;
		}
	}

	/* Resume bundles */
	printf("\nResuming bundles...\n");
	resumedCount = bpinspect_ops_resume_bundles(entries, count);

	if (resumedCount < 0)
	{
		printf("Error resuming bundles.\n");
		return -1;
	}

	printf("Resumed %d of %d bundles.\n", resumedCount, count);
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

	/* Determine color support */
	if (opts.useColor == 1)
	{
		g_useColor = 1;  /* Force on */
	}
	else if (opts.useColor == -1)
	{
		g_useColor = 0;  /* Force off */
	}
	else
	{
		g_useColor = detect_color_support();  /* Auto-detect */
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

		/* Re-enumerate to verify cancellation (only if bundles were canceled) */
		if (!opts.dryRun && result == 0 && filteredCount > 0)
		{
			BundleListState		verifyState;
			int			remainingCount;

			printf("\nVerifying cancellation...\n");

			/* Re-enumerate bundles */
			if (bpinspect_data_init(&verifyState) < 0)
			{
				printf("Warning: Can't re-enumerate for verification.\n");
			}
			else
			{
				remainingCount = bpinspect_data_refresh(&verifyState);
				if (remainingCount >= 0)
				{
					printf("Before cancellation: %d bundles\n", filteredCount);
					printf("After cancellation:  %d total bundles\n", remainingCount);
					printf("Expected reduction:  %d bundles\n\n", filteredCount);

					if (remainingCount < state.count)
					{
						printf("Successfully removed %d bundles from system\n",
							state.count - remainingCount);
					}
					else
					{
						printf("WARNING: Bundle count did not decrease as expected\n");
						printf("  Check ion.log for detailed cancellation diagnostics\n");
					}
				}

				bpinspect_data_cleanup(&verifyState);
			}
		}
	}
	else if (opts.suspend)
	{
		/* Suspend bundles */
		result = suspend_bundles(filtered, filteredCount,
					 opts.dryRun, opts.noConfirm);
	}
	else if (opts.resume)
	{
		/* Resume bundles */
		result = resume_bundles(filtered, filteredCount,
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
