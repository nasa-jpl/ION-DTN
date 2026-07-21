/*
	ltpstats.c:	print snapshot of LTP statistics.

	Author: Ion Development Team

	Copyright (c) 2024, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
*/

#include "ltpP.h"
#include "platform.h"

#define DEFAULT_MODE	MODE_KEY

typedef enum
{
	MODE_ALL = 0,
	MODE_KEY,
	MODE_GROUPED
} DisplayMode;

static char	*statNames[LTP_SPAN_STATS] = {
	"out_seg_queued",
	"out_seg_popped",
	"ckpt_xmit",
	"pos_rpt_recv",
	"neg_rpt_recv",
	"export_cancel_recv",
	"ckpt_re_xmit",
	"seg_re_xmit",
	"export_cancel_xmit",
	"export_complete",
	"ckpt_recv",
	"pos_rpt_xmit",
	"neg_rpt_xmit",
	"import_cancel_recv",
	"rpt_re_xmit",
	"import_cancel_xmit",
	"import_complete",
	"in_seg_recv_red",
	"in_seg_recv_green",
	"in_seg_redundant",
	"in_seg_malformed",
	"in_seg_unk_sender",
	"in_seg_unk_client",
	"in_seg_screened",
	"in_seg_miscolored",
	"in_seg_ses_closed",
	"in_seg_too_fast"
};

/* Key statistics indices */
static int keyStats[] = {
	OUT_SEG_QUEUED, OUT_SEG_POPPED,
	CKPT_XMIT, CKPT_RE_XMIT,
	SEG_RE_XMIT,
	EXPORT_COMPLETE,
	IN_SEG_RECV_RED, IN_SEG_RECV_GREEN,
	CKPT_RECV,
	IMPORT_COMPLETE,
	-1  /* Terminator */
};

/* Grouped statistics */
typedef struct
{
	char	*groupName;
	int	indices[15];  /* Max stats per group, -1 terminated */
} StatGroup;

static StatGroup statGroups[] = {
	{
		"Export (Outbound)",
		{ OUT_SEG_QUEUED, OUT_SEG_POPPED, CKPT_XMIT,
		  SEG_RE_XMIT, CKPT_RE_XMIT, EXPORT_CANCEL_XMIT,
		  EXPORT_COMPLETE, -1 }
	},
	{
		"Import (Inbound)",
		{ IN_SEG_RECV_RED, IN_SEG_RECV_GREEN, CKPT_RECV,
		  IMPORT_CANCEL_XMIT, IMPORT_COMPLETE, -1 }
	},
	{
		"Acknowledgments",
		{ POS_RPT_RECV, NEG_RPT_RECV, POS_RPT_XMIT,
		  NEG_RPT_XMIT, RPT_RE_XMIT, -1 }
	},
	{
		"Errors/Discards",
		{ IN_SEG_REDUNDANT, IN_SEG_MALFORMED, IN_SEG_UNK_SENDER,
		  IN_SEG_UNK_CLIENT, IN_SEG_SCREENED, IN_SEG_MISCOLORED,
		  IN_SEG_SES_CLOSED, IN_SEG_TOO_FAST,
		  EXPORT_CANCEL_RECV, IMPORT_CANCEL_RECV, -1 }
	},
	{ NULL, { -1 } }  /* Terminator */
};

static void	printUsage(void)
{
	PUTS("Usage: ltpstats [-a | -k | -g] [-r]");
	PUTS("  -a : Report all 27 statistics");
	PUTS("  -k : Report key statistics only (default)");
	PUTS("  -g : Report grouped statistics");
	PUTS("  -r : Reset current counts/bytes after reporting");
}

static void	reportStat(uvast engineId, char *fromTimestamp,
			char *toTimestamp, char *statName,
			unsigned int count, uvast bytes)
{
	char	buffer[512];

	isprintf(buffer, sizeof buffer,
		"[x] span " UVAST_FIELDSPEC " %s from %s to %s: %u "
		UVAST_FIELDSPEC,
		engineId, statName, fromTimestamp, toTimestamp,
		count, bytes);
	writeMemo(buffer);
}

static void	reportAllStats(LtpSpanStats *stats, uvast engineId,
			char *fromTimestamp, char *toTimestamp)
{
	int	i;

	for (i = 0; i < LTP_SPAN_STATS; i++)
	{
		reportStat(engineId, fromTimestamp, toTimestamp,
			statNames[i], stats->tallies[i].currentCount,
			stats->tallies[i].currentBytes);
	}
}

static void	reportKeyStats(LtpSpanStats *stats, uvast engineId,
			char *fromTimestamp, char *toTimestamp)
{
	int	i;
	int	idx;

	for (i = 0; keyStats[i] >= 0; i++)
	{
		idx = keyStats[i];
		reportStat(engineId, fromTimestamp, toTimestamp,
			statNames[idx], stats->tallies[idx].currentCount,
			stats->tallies[idx].currentBytes);
	}
}

static void	reportGroupedStats(LtpSpanStats *stats, uvast engineId,
			char *fromTimestamp, char *toTimestamp)
{
	int	i, j, idx;
	char	buffer[256];

	for (i = 0; statGroups[i].groupName != NULL; i++)
	{
		isprintf(buffer, sizeof buffer,
			"[i] === %s ===", statGroups[i].groupName);
		writeMemo(buffer);

		for (j = 0; statGroups[i].indices[j] >= 0; j++)
		{
			idx = statGroups[i].indices[j];
			reportStat(engineId, fromTimestamp, toTimestamp,
				statNames[idx],
				stats->tallies[idx].currentCount,
				stats->tallies[idx].currentBytes);
		}
	}
}

static void resetStats(Sdr sdr, SdrObject statsObj)
{
	LtpSpanStats	stats;
	int		i;
	time_t		currentTime;

	sdr_stage(sdr, (char *) &stats, statsObj, sizeof(LtpSpanStats));

	currentTime = getCtime();
	stats.resetTime = currentTime;

	for (i = 0; i < LTP_SPAN_STATS; i++)
	{
		stats.tallies[i].currentCount = 0;
		stats.tallies[i].currentBytes = 0;
	}

	sdr_write(sdr, statsObj, (char *) &stats, sizeof(LtpSpanStats));
}

static void	processSpanStats(DisplayMode mode, int doReset)
{
	Sdr		sdr;
	SdrObject	sdrElt;
	SdrObject	spanObj;
	LtpSpan		span;
	LtpSpanStats	stats;
	char		fromTimestamp[TIMESTAMPBUFSZ];
	char		toTimestamp[TIMESTAMPBUFSZ];
	time_t		currentTime;

	sdr = getIonsdr();
	CHKVOID(sdr_begin_xn(sdr));

	if (getLtpConstants() == NULL)
	{
		sdr_exit_xn(sdr);
		writeMemo("[!] LTP is not initialized.");
		return;
	}

	currentTime = getCtime();
	writeTimestampLocal(currentTime, toTimestamp);

	for (sdrElt = sdr_list_first(sdr, (getLtpConstants())->spans);
			sdrElt;
			sdrElt = sdr_list_next(sdr, sdrElt))
	{
		spanObj = sdr_list_data(sdr, sdrElt);
		sdr_read(sdr, (char *) &span, spanObj, sizeof(LtpSpan));
		sdr_read(sdr, (char *) &stats, span.stats,
			sizeof(LtpSpanStats));

		writeTimestampLocal(stats.resetTime, fromTimestamp);

		/* Report statistics based on mode */
		switch (mode)
		{
		case MODE_ALL:
			reportAllStats(&stats, span.engineId,
				fromTimestamp, toTimestamp);
			break;

		case MODE_KEY:
			reportKeyStats(&stats, span.engineId,
				fromTimestamp, toTimestamp);
			break;

		case MODE_GROUPED:
			reportGroupedStats(&stats, span.engineId,
				fromTimestamp, toTimestamp);
			break;
		}

		/* Reset if requested */
		if (doReset)
		{
			resetStats(sdr, span.stats);
		}
	}

	if (doReset)
	{
		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Failed to commit statistics reset.", NULL);
		}
	}
	else
	{
		sdr_exit_xn(sdr);
	}
}

#if defined (ION_LWT)
int	ltpstats(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	int		argc = (int) a1;
	char		**argv = (char **) a2;
#else
int	main(int argc, char **argv)
{
#endif
	DisplayMode	mode = DEFAULT_MODE;
	int		doReset = 0;
	int		i;

	/* Parse command-line arguments */
	for (i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-a") == 0)
		{
			mode = MODE_ALL;
		}
		else if (strcmp(argv[i], "-k") == 0)
		{
			mode = MODE_KEY;
		}
		else if (strcmp(argv[i], "-g") == 0)
		{
			mode = MODE_GROUPED;
		}
		else if (strcmp(argv[i], "-r") == 0)
		{
			doReset = 1;
		}
		else if (strcmp(argv[i], "-h") == 0
			|| strcmp(argv[i], "--help") == 0)
		{
			printUsage();
			return 0;
		}
		else
		{
			PUTS("Invalid argument.");
			printUsage();
			return 1;
		}
	}

	if (ltp_attach() < 0)
	{
		putErrmsg("ltpstats can't attach to LTP.", NULL);
		return 1;
	}

	writeMemo("[i] Start of LTP statistics snapshot...");
	processSpanStats(mode, doReset);
	writeMemo("[i] ...end of LTP statistics snapshot.");

	ltp_detach();
	return 0;
}
