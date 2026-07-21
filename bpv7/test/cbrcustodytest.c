/*
	cbrcustodytest.c:	Test application for CBR custody transfer APIs.

	This application verifies that the custody transfer mechanism
	works correctly by:
	1. Sending a bundle with custody transfer requested
	2. Verifying the bundle appears in local custody tracking
	3. Polling for custody status changes
	4. Optionally triggering manual retransmission

	Usage: cbrcustodytest <destEid> [-r] [-t<timeout_sec>]
	       cbrcustodytest -l
	  -l: List-only mode (just list bundles in custody, don't send)
	  -r: Also test retransmission
	  -t<N>: Timeout in seconds (default 30)

	Copyright (c) 2026, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.

	Author: ION Development Team
									*/

#include <bp.h>
#include "bpP.h"
#include "cbr.h"

#define	DEFAULT_TTL		300
#define	DEFAULT_TIMEOUT		30
#define	POLL_INTERVAL_USEC	500000	/* 0.5 seconds */

static int	_running(int *newState)
{
	static int	state = 1;

	if (newState)
	{
		state = *newState;
	}

	return state;
}

static void	handleQuit(int signum)
{
	int	stop = 0;

	(void)signum;
	oK(_running(&stop));
}

/*	Callback for cbr_listCustodyBundles to print custody info.	*/
static void	printCustodyInfo(CbrCustodyInfo *info, void *userData)
{
	int	*count = (int *)userData;

	printf("  [%d] destEid=%s seqId=" UVAST_FIELDSPEC " seqNum="
			UVAST_FIELDSPEC "\n",
			*count, info->destEid, info->seqId, info->seqNum);
	printf("       custodyAccepted=%ld lastTransmit=%ld retransmitCount=%d\n",
			(long)info->custodyAccepted,
			(long)info->lastTransmit,
			info->retransmitCount);
	(*count)++;
}

static void	printUsage(void)
{
	PUTS("Usage: cbrcustodytest <destination EID> [-r] [-t<timeout_sec>]");
	PUTS("       cbrcustodytest -l");
	PUTS("  -l: List-only mode (just list bundles in custody)");
	PUTS("  -r: Also test retransmission");
	PUTS("  -t<N>: Timeout in seconds (default 30)");
}

#if defined (ION_LWT)
int	cbrcustodytest(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char		*destEid = (char *)a1;
	char		*arg2 = (char *)a2;
	char		*arg3 = (char *)a3;
#else
int	main(int argc, char **argv)
{
	char		*destEid = NULL;
	char		*arg2 = NULL;
	char		*arg3 = NULL;
	int		i;

	for (i = 1; i < argc; i++)
	{
		if (argv[i][0] != '-')
		{
			if (destEid == NULL)
			{
				destEid = argv[i];
			}
		}
		else if (arg2 == NULL)
		{
			arg2 = argv[i];
		}
		else if (arg3 == NULL)
		{
			arg3 = argv[i];
		}
	}
#endif
	int		listOnly = 0;
	int		testRetransmit = 0;
	int		timeoutSec = DEFAULT_TIMEOUT;
	Sdr		sdr;
	char		line[64];
	int		lineLength;
	SdrObject	extent;
	SdrObject	bundleZco;
	ReqAttendant	attendant;
	SdrObject	newBundle;
	int		bundleCount;
	int		elapsed;
	int		status;
	int		result = 0;

	/*	Parse optional arguments.				*/
	if (arg2 != NULL)
	{
		if (strcmp(arg2, "-l") == 0)
		{
			listOnly = 1;
		}
		else if (strcmp(arg2, "-r") == 0)
		{
			testRetransmit = 1;
		}
		else if (arg2[0] == '-' && arg2[1] == 't')
		{
			timeoutSec = atoi(arg2 + 2);
			if (timeoutSec <= 0)
			{
				timeoutSec = DEFAULT_TIMEOUT;
			}
		}
	}

	if (arg3 != NULL)
	{
		if (strcmp(arg3, "-l") == 0)
		{
			listOnly = 1;
		}
		else if (strcmp(arg3, "-r") == 0)
		{
			testRetransmit = 1;
		}
		else if (arg3[0] == '-' && arg3[1] == 't')
		{
			timeoutSec = atoi(arg3 + 2);
			if (timeoutSec <= 0)
			{
				timeoutSec = DEFAULT_TIMEOUT;
			}
		}
	}

	/*	List-only mode doesn't require destEid.			*/
	if (destEid == NULL && !listOnly)
	{
		printUsage();
		return 0;
	}

	/*	Attach to ION.						*/
	if (bp_attach() < 0)
	{
		putErrmsg("cbrcustodytest: Can't attach to BP.", NULL);
		return 1;
	}

	if (cbr_attach() < 0)
	{
		putErrmsg("cbrcustodytest: Can't attach to CBR.", NULL);
		bp_detach();
		return 1;
	}

	if (ionStartAttendant(&attendant) < 0)
	{
		putErrmsg("cbrcustodytest: Can't initialize attendant.", NULL);
		bp_detach();
		return 1;
	}

	sdr = bp_get_sdr();
	isignal(SIGINT, handleQuit);

	/*	Handle list-only mode.					*/
	if (listOnly)
	{
		CbrStatistics	stats;

		printf("cbrcustodytest: Listing bundles in custody...\n");
		bundleCount = 0;
		if (cbr_listCustodyBundles(sdr, printCustodyInfo, &bundleCount)
				< 0)
		{
			putErrmsg("cbrcustodytest: Can't list custody bundles.",
					NULL);
			bp_detach();
			return 1;
		}

		printf("cbrcustodytest: %d bundle(s) in custody.\n",
				bundleCount);

		/*	Display CBR/CT statistics.			*/
		if (cbr_getStatistics(sdr, &stats) == 0)
		{
			printf("\nCBR/CT Statistics:\n");
			printf("  CCS Accept Sent:     %u\n", stats.ccsAcceptSent);
			printf("  CCS Refuse Sent:     %u\n", stats.ccsRefuseSent);
			printf("  CCS Accept Received: %u\n", stats.ccsAcceptRecv);
			printf("  CCS Refuse Received: %u\n", stats.ccsRefuseRecv);
			printf("  Custody Originated:  %u\n", stats.custodyOriginated);
			printf("  Custody Accepted:    %u\n", stats.custodyAccepted);
			printf("  Custody Released:    %u\n", stats.custodyReleased);
			printf("  CRS Signals Sent:    %u\n", stats.crsSignalsSent);
			printf("  CRS Signals Recv:    %u\n", stats.crsSignalsRecv);
		}
		else
		{
			printf("cbrcustodytest: Can't get CBR statistics.\n");
		}

		bp_detach();
		return 0;
	}

	printf("cbrcustodytest: Destination=%s timeout=%d sec\n",
			destEid, timeoutSec);

	/*	Create and send a test bundle with custody transfer.	*/
	istrcpy(line, "CBR Custody Test Bundle", sizeof(line));
	lineLength = strlen(line);

	CHKZERO(sdr_begin_xn(sdr));
	extent = sdr_malloc(sdr, lineLength);
	if (extent)
	{
		sdr_write(sdr, extent, line, lineLength);
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("cbrcustodytest: No space for ZCO extent.", NULL);
		ionStopAttendant(&attendant);
		bp_detach();
		return 1;
	}

	bundleZco = ionCreateZco(ZcoSdrSource, extent, 0, lineLength,
			BP_STD_PRIORITY, 0, ZcoOutbound, &attendant);
	if (bundleZco == 0 || bundleZco == (SdrObject) ERROR)
	{
		putErrmsg("cbrcustodytest: Can't create ZCO.", NULL);
		ionStopAttendant(&attendant);
		bp_detach();
		return 1;
	}

	printf("cbrcustodytest: Sending bundle with SourceCustodyRequired...\n");

	/*	Send bundle with custody transfer requested.
	 *	SourceCustodyRequired indicates we want custody transfer.	*/
	if (bp_send(NULL, destEid, NULL, DEFAULT_TTL, BP_STD_PRIORITY,
			SourceCustodyRequired, 0, 0, NULL, bundleZco,
			&newBundle) < 1)
	{
		putErrmsg("cbrcustodytest: Can't send bundle.", NULL);
		printf("cbrcustodytest: bundle send failed.\n");
		CHKZERO(sdr_begin_xn(sdr));
		zco_destroy(sdr, bundleZco);
		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Can't destroy ZCO.", NULL);
		}

		ionStopAttendant(&attendant);
		bp_detach();
		return 1;
	}

	printf("cbrcustodytest: Bundle sent successfully.\n");

	/*	Wait briefly for bundle to be processed.		*/
	microsnooze(500000);	/* 0.5 sec */

	/*	List bundles currently in custody.			*/
	printf("\ncbrcustodytest: Listing bundles in custody...\n");
	bundleCount = 0;
	if (cbr_listCustodyBundles(sdr, printCustodyInfo, &bundleCount) < 0)
	{
		putErrmsg("cbrcustodytest: Can't list custody bundles.", NULL);
		result = 1;
	}
	else
	{
		printf("cbrcustodytest: %d bundle(s) in custody.\n",
				bundleCount);
		if (bundleCount == 0)
		{
			printf("cbrcustodytest: WARNING - Expected bundle "
					"to be in custody tracking.\n");
			printf("  (This may be OK if the next hop already "
					"accepted custody.)\n");
		}
	}

	/*	Poll for custody status changes.			*/
	printf("\ncbrcustodytest: Polling for custody release "
			"(timeout=%d sec)...\n", timeoutSec);
	elapsed = 0;
	while (_running(NULL) && elapsed < timeoutSec)
	{
		microsnooze(POLL_INTERVAL_USEC);
		elapsed++;

		/*	Check custody bundle count.			*/
		bundleCount = 0;
		cbr_listCustodyBundles(sdr, printCustodyInfo, &bundleCount);

		if (bundleCount == 0)
		{
			printf("cbrcustodytest: Custody released after "
					"~%d seconds.\n", elapsed);
			break;
		}

		if ((elapsed % 5) == 0)
		{
			printf("  ... still waiting (%d sec), "
					"%d bundle(s) in custody\n",
					elapsed, bundleCount);
		}
	}

	if (bundleCount > 0)
	{
		printf("cbrcustodytest: Timeout - %d bundle(s) still "
				"in custody.\n", bundleCount);

		if (testRetransmit)
		{
			printf("\ncbrcustodytest: Testing manual "
					"retransmission...\n");
			status = cbr_retransmitAllCustody(sdr, NULL);
			if (status < 0)
			{
				putErrmsg("cbrcustodytest: Retransmit failed.",
						NULL);
				result = 1;
			}
			else
			{
				printf("cbrcustodytest: Retransmitted %d "
						"bundle(s).\n", status);
			}
		}
	}
	else
	{
		printf("cbrcustodytest: SUCCESS - Custody transfer completed.\n");
	}

	/*	Clean up.						*/
	ionStopAttendant(&attendant);
	bp_detach();
	return result;
}
