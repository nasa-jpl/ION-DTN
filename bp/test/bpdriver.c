/*
	bpdriver.c:	sender for bundle benchmark test.
									*/
/*									*/
/*	Copyright (c) 2004, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*	Enhanced by Ryan Metzger (MITRE Corp.) August 2006.		*/
/*									*/

#include <bp.h>

#define	DEFAULT_ADU_LENGTH	(60000)

#if 0
#define	CYCLE_TRACE
#endif

/*	Indication marks:	"." for BpPayloadPresent (1),
				"*" for BpReceptionTimedOut (2).
				"!" for BpReceptionInterrupted (3).	*/

//static char	dlvmarks[] = "?.*!";

static int	running;
static BpSAP	sap;

static void	handleQuit()
{
	running = 0;
	bp_interrupt(sap);
}

static int	run_bpdriver(int cyclesRemaining, char *ownEid, char *destEid,
			int aduLength, int streaming)
{
	Sdr		sdr;
	BpCustodySwitch	custodySwitch;
	int		cycles;
	int		aduFile;
	int		randomAduLength = 0;
	int		bytesRemaining;
	int		bytesToWrite;
	char		buffer[DEFAULT_ADU_LENGTH] = "test...";
	Object		fileRef;
	Object		bundleZco;
	Object		newBundle;
	int		bytesSent = 0;
	time_t		startTime;
	BpDelivery	dlv;
	time_t		endTime;
	long		interval;

	if (cyclesRemaining == 0 || ownEid == NULL || destEid == NULL
	|| aduLength == 0)
	{
		puts("Usage: bpdriver <number of cycles> <own endpoint ID> \
<destination endpoint ID> [<payload size>]");
		puts("  Payload size defaults to 60000 bytes.");
		puts("");
		puts("  Normal operation of bpdriver is to wait for");
		puts("  acknowledgment after sending each bundle.  To run");
		puts("  in streaming mode instead, specify a negative");
		puts("  payload size; the absolute value of this parameter");
		puts("  will be used as the actual payload size.");
		puts("");
		puts("  To use payload sizes chosen at random from the");
	       	puts("	range 1024 to 62464, in multiples of 1024,");
	       	puts("	specify payload size 1 (or -1 for streaming mode).");
		puts("");
		puts("  bpdriver normally runs with custody transfer");
	       	puts("	disabled.  To request custody transfer for all");
	       	puts("	bundles sent, specify number of cycles as a");
	       	puts("	negative number; the absolute value of this");
	       	puts("	parameter will be used as the actual number of");
	       	puts("	cycles.");
		puts("");
		puts("  Destination (receiving) application must be bpecho");
		puts("  when bpdriver is run in stop-and-wait mode, should");
		puts("  be bpcounter in streaming mode.");
		return 0;
	}

	if (bp_attach() < 0)
	{
		putErrmsg("Can't attach to BP.", NULL);
		return 0;
	}

	if (bp_open(ownEid, &sap) < 0)
	{
		putErrmsg("Can't open own endpoint.", ownEid);
		return 0;
	}

	sdr = bp_get_sdr();
	if (cyclesRemaining < 0)
	{
		custodySwitch = SourceCustodyRequired;
		cyclesRemaining = 0 - cyclesRemaining;
	}
	else
	{
		custodySwitch = NoCustodyRequested;
	}

	cycles = cyclesRemaining;
	if (aduLength == 1)
	{
		randomAduLength = 1;
		srand((unsigned int) time(NULL));
	}

	aduFile = open("bpdriverAduFile", O_WRONLY | O_CREAT | O_TRUNC, 0777);
	if (aduFile < 0)
	{
		bp_close(sap);
		putSysErrmsg("can't create ADU file", NULL);
		return 0;
	}

	if (randomAduLength)
	{
		bytesRemaining = 65536;
	}
	else
	{
		bytesRemaining = aduLength;
	}

	while (bytesRemaining > 0)
	{
		if (bytesRemaining < DEFAULT_ADU_LENGTH)
		{
			bytesToWrite = bytesRemaining;
		}
		else
		{
			bytesToWrite = DEFAULT_ADU_LENGTH;
		}

		if (write(aduFile, buffer, bytesToWrite) < 0)
		{
			bp_close(sap);
			close(aduFile);
			putSysErrmsg("error writing to ADU file", NULL);
			return 0;
		}

		bytesRemaining -= bytesToWrite;
	}

	close(aduFile);
	sdr_begin_xn(sdr);
	fileRef = zco_create_file_ref(sdr, "bpdriverAduFile", NULL);
	if (sdr_end_xn(sdr) < 0 || fileRef == 0)
	{
		bp_close(sap);
		putSysErrmsg("bpdriver can't create file ref", NULL);
		return 0;
	}

	signal(SIGINT, handleQuit);
	running = 1;
	startTime = time(NULL);
	while (running && cyclesRemaining > 0)
	{
		if (randomAduLength)
		{
			aduLength = ((rand() % 60) + 1) * 1024;
		}

		sdr_begin_xn(sdr);
		bundleZco = zco_create(sdr, ZcoFileSource, fileRef, 0,
				aduLength);
		if (sdr_end_xn(sdr) < 0 || bundleZco == 0)
		{
			putSysErrmsg("bpdriver can't create ZCO", NULL);
			running = 0;
			continue;
		}

		if (bp_send(sap, BP_BLOCKING, destEid, NULL, 300,
				BP_STD_PRIORITY, custodySwitch, 0, 0, NULL,
				bundleZco, &newBundle) < 1)
		{
			putSysErrmsg("bpdriver can't send message",
					itoa(aduLength));
			running = 0;
			continue;
		}

		bytesSent += aduLength;
//putchar('^');
//fflush(stdout);
		cyclesRemaining--;
		if ((cyclesRemaining % 1000) == 0)
		{
			printf("%d\n", cyclesRemaining);
		}

		if (streaming)
		{
			continue;
		}

		/*	Now wait for acknowledgment before sending
			next bundle.					*/

		while (running)
		{
			if (bp_receive(sap, &dlv, BP_BLOCKING) < 0)
			{
				putSysErrmsg("bpdriver reception failed", NULL);
				running = 0;
				continue;
			}

			bp_release_delivery(&dlv, 1);
//putchar(dlvmarks[dlv.result]);
//fflush(stdout);
			if (dlv.result == BpPayloadPresent)
			{
				break;
			}
		}
	}

	bp_close(sap);
	endTime = time(NULL);
	writeErrmsgMemos();
	puts("Stopping bpdriver.");
	sdr_begin_xn(sdr);
	zco_destroy_file_ref(sdr, fileRef);
	if (sdr_end_xn(sdr) < 0)
	{
		putSysErrmsg("bpdriver can't destroy file reference", NULL);
	}

	interval = endTime - startTime;
	printf("Time: %ld seconds.\n", interval);
	printf("Data size: %d bytes in %d bundles.\n", bytesSent,
			cycles - cyclesRemaining);
	if (interval <= 0)
	{
		puts("Interval is too short to measure rate.");
	}
	else
	{
		printf("Throughput: %lu bytes per second.\n",
				bytesSent / interval);
	}

	bp_detach();
	return 0;
}

#if defined (VXWORKS) || defined (RTEMS)
int	bpdriver(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	int	cyclesRemaining = atoi((char *) a1);
	char	*ownEid = (char *) a2;
	char	*destEid = (char *) a3;
	int	aduLength = (a4 == 0 ? DEFAULT_ADU_LENGTH : atoi((char *) a4));
	int	streaming = 0;
#else
int	main(int argc, char **argv)
{
	int	cyclesRemaining = 0;
	char	*ownEid = NULL;
	char	*destEid = NULL;
	int	aduLength = DEFAULT_ADU_LENGTH;
	int	streaming = 0;

	if (argc > 5) argc = 5;
	switch (argc)
	{
	case 5:
	  	aduLength = atoi(argv[4]);
	case 4:
		destEid = argv[3];
	case 3:
		ownEid = argv[2];
	case 2:
		cyclesRemaining = atoi(argv[1]);
	default:
		break;
	}
#endif
	if (aduLength < 0)
	{
		streaming = 1;
		aduLength = 0 - aduLength;
	}

	return run_bpdriver(cyclesRemaining, ownEid, destEid, aduLength,
			streaming);
}
