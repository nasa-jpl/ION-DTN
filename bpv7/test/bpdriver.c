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
#define	DEFAULT_TTL		300

/*	Indication marks:	"." for BpPayloadPresent (1),
				"*" for BpReceptionTimedOut (2).
				"!" for BpReceptionInterrupted (3).	*/

//static char	dlvmarks[] = "?.*!";

static BpSAP	_bpsap(BpSAP *newSap)
{
	void	*value;
	BpSAP	sap;

	if (newSap)			/*	Add task variable.	*/
	{
		value = (void *) (*newSap);
		sap = (BpSAP) sm_TaskVar(&value);
	}
	else				/*	Retrieve task variable.	*/
	{
		sap = (BpSAP) sm_TaskVar(NULL);
	}

	return sap;
}

static ReqAttendant	*_attendant(ReqAttendant *newAttendant)
{
	static ReqAttendant	*attendant = NULL;

	if (newAttendant)
	{
		attendant = newAttendant;
	}

	return attendant;
}

static void	handleQuit(int signum)
{
	isignal(SIGINT, handleQuit);
	bp_interrupt(_bpsap(NULL));
	ionPauseAttendant(_attendant(NULL));
}

static int	run_bpdriver(int cyclesRemaining, char *ownEid, char *destEid,
			int aduLength, int streaming, int ttl)
{
	static char	buffer[DEFAULT_ADU_LENGTH] = "test...";
	BpSAP		sap;
	Sdr		sdr;
	int		running = 1;
	BpCustodySwitch	custodySwitch;
	int		cycles;
	int		aduFile;
	int		randomAduLength = 0;
	int		bytesRemaining;
	int		bytesToWrite;
	Object		pilotAduString;
	Object		fileRef;
	ReqAttendant	attendant;
	Object		bundleZco;
	Object		newBundle;
	double		bytesSent = 0.0;
	struct timeval	startTime;
	BpDelivery	dlv;
	struct timeval	endTime;
	double		interval;
	char		textBuf[256];

	if (cyclesRemaining == 0 || ownEid == NULL || destEid == NULL
	|| aduLength == 0)
	{
		PUTS("Usage: bpdriver <number of cycles> <own endpoint ID> \
<destination endpoint ID> [<payload size>] [t<Bundle TTL>]");
		PUTS("  Payload size defaults to 60000 bytes.");
		PUTS("  Bundle TTL defaults to 300 seconds.");
		PUTS("");
		PUTS("  Normal operation of bpdriver is to wait for");
		PUTS("  acknowledgment after sending each bundle.  To run");
		PUTS("  in streaming mode instead, specify a negative");
		PUTS("  payload size; the absolute value of this parameter");
		PUTS("  will be used as the actual payload size.");
		PUTS("");
		PUTS("  To use payload sizes chosen at random from the");
	       	PUTS("	range 1024 to 62464, in multiples of 1024,");
	       	PUTS("	specify payload size 1 (or -1 for streaming mode).");
		PUTS("");
		PUTS("  bpdriver normally runs with custody transfer");
	       	PUTS("	disabled.  To request custody transfer for all");
	       	PUTS("	bundles sent, specify number of cycles as a");
	       	PUTS("	negative number; the absolute value of this");
	       	PUTS("	parameter will be used as the actual number of");
	       	PUTS("	cycles.");
		PUTS("");
		PUTS("  Destination (receiving) application must be bpecho");
		PUTS("  when bpdriver is run in stop-and-wait mode, should");
		PUTS("  be bpcounter in streaming mode.");
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

	oK(_bpsap(&sap));
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

	aduFile = iopen("bpdriverAduFile", O_WRONLY | O_CREAT | O_TRUNC, 0777);
	if (ttl == 0)
	{
		ttl = DEFAULT_TTL;
	}

	if (aduFile < 0)
	{
		putSysErrmsg("can't create ADU file", NULL);
		bp_close(sap);
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
			putSysErrmsg("error writing to ADU file", NULL);
			bp_close(sap);
			close(aduFile);
			return 0;
		}

		bytesRemaining -= bytesToWrite;
	}

	close(aduFile);
	CHKZERO(sdr_begin_xn(sdr));
	fileRef = zco_create_file_ref(sdr, "bpdriverAduFile", NULL,
			ZcoOutbound);
	if (sdr_end_xn(sdr) < 0 || fileRef == 0)
	{
		putErrmsg("bpdriver can't create file ref.", NULL);
		bp_close(sap);
		return 0;
	}

	if (ionStartAttendant(&attendant))
	{
		putErrmsg("Can't initialize blocking transmission.", NULL);
		bp_close(sap);
		return 0;
	}

	_attendant(&attendant);

	/*	Send pilot bundle to start bpcounter's timer.		*/
		
	CHKZERO(sdr_begin_xn(sdr));
	pilotAduString = sdr_string_create(sdr, "Go.");
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("bpdriver can't create pilot ADU string.", NULL);
		bp_close(sap);
		return 0;
	}

	bundleZco = ionCreateZco(ZcoSdrSource, pilotAduString, 0, 
			sdr_string_length(sdr, pilotAduString),
			BP_STD_PRIORITY, 0, ZcoOutbound, &attendant);
	if (bundleZco == 0 || bundleZco == (Object) ERROR)
	{
		putErrmsg("bpdriver can't create pilot ADU.", NULL);
		bp_close(sap);
		return 0;
	}

	if (bp_send(sap, destEid, NULL, ttl, BP_STD_PRIORITY, custodySwitch, 0,
			0, NULL, bundleZco, &newBundle) < 1)
	{
		putErrmsg("bpdriver can't send pilot bundle.",
				itoa(aduLength));
		bp_close(sap);
		return 0;
	}

	if (!streaming)
	{
		/*	Must wait for acknowledgment before sending
			first bundle.					*/

		while (running)
		{
			if (bp_receive(sap, &dlv, BP_BLOCKING) < 0)
			{
				putErrmsg("bpdriver reception failed.", NULL);
				running = 0;
				continue;
			}

			bp_release_delivery(&dlv, 1);
//putchar(dlvmarks[dlv.result]);
//fflush(stdout);
			if (dlv.result == BpReceptionInterrupted
			|| dlv.result == BpEndpointStopped)
			{
				running = 0;
				continue;
			}

			if (dlv.result == BpPayloadPresent)
			{
				break;	/*	Out of reception loop.	*/
			}
		}
	}

	snooze(1);	/*	Make sure pilot bundle has been sent.	*/

	/*	Begin timed bundle transmission.			*/

	isignal(SIGINT, handleQuit);
	getCurrentTime(&startTime);
	while (running && cyclesRemaining > 0)
	{
		if (randomAduLength)
		{
			aduLength = ((rand() % 60) + 1) * 1024;
		}

		bundleZco = ionCreateZco(ZcoFileSource, fileRef, 0, aduLength,
				BP_STD_PRIORITY, 0, ZcoOutbound, &attendant);
		if (bundleZco == 0 || bundleZco == (Object) ERROR)
		{
			putErrmsg("bpdriver can't create ZCO.", NULL);
			running = 0;
			continue;
		}

		if (bp_send(sap, destEid, NULL, ttl, BP_STD_PRIORITY,
			custodySwitch, 0, 0, NULL, bundleZco, &newBundle) < 1)
		{
			putErrmsg("bpdriver can't send message.",
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
			PUTS(itoa(cyclesRemaining));
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
				putErrmsg("bpdriver reception failed.", NULL);
				running = 0;
				continue;
			}

			bp_release_delivery(&dlv, 1);
//putchar(dlvmarks[dlv.result]);
//fflush(stdout);
			if (dlv.result == BpReceptionInterrupted)
			{
				continue;
			}

			if (dlv.result == BpEndpointStopped)
			{
				running = 0;
				continue;
			}

			if (dlv.result == BpPayloadPresent)
			{
				break;	/*	Out of reception loop.	*/
			}
		}
	}

	bp_close(sap);
	getCurrentTime(&endTime);
	writeErrmsgMemos();
	PUTS("Stopping bpdriver.");
	CHKZERO(sdr_begin_xn(sdr));
	zco_destroy_file_ref(sdr, fileRef);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("bpdriver can't destroy file reference.", NULL);
	}

	if (endTime.tv_usec < startTime.tv_usec)
	{
		endTime.tv_usec += 1000000;
		endTime.tv_sec -= 1;
	}

	PUTMEMO("Total bundles", itoa(cycles - cyclesRemaining));
	interval = (endTime.tv_usec - startTime.tv_usec)
			+ (1000000 * (endTime.tv_sec - startTime.tv_sec));
	isprintf(textBuf, sizeof textBuf, "%.3f", interval / 1000000);
	PUTMEMO("Time (seconds)", textBuf);
	isprintf(textBuf, sizeof textBuf, "%.0f", bytesSent);
	PUTMEMO("Total bytes", textBuf);
	if (interval > 0.0)
	{
		isprintf(textBuf, sizeof textBuf, "%.3f",
			((bytesSent * 8) / (interval / 1000000)) / 1000000);
		PUTMEMO("Throughput (Mbps)", textBuf);
	}
	else
	{
		PUTS("Interval is too short to measure rate.");
	}

	ionStopAttendant(&attendant);
	bp_detach();
	return 0;
}

#if defined (ION_LWT)
int	bpdriver(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	int	cyclesRemaining = atoi((char *) a1);
	char	*ownEid = (char *) a2;
	char	*destEid = (char *) a3;
	int	aduLength = (a4 == 0 ? DEFAULT_ADU_LENGTH : atoi((char *) a4));
	int	streaming = 0;
	int 	ttl = (a5 == 0 ? DEFAULT_TTL : atoi((char *) a5));
#else
int	main(int argc, char **argv)
{
	int	cyclesRemaining = 0;
	char	*ownEid = NULL;
	char	*destEid = NULL;
	int	aduLength = DEFAULT_ADU_LENGTH;
	int	streaming = 0;
	int ttl=0;

	if (argc > 6) argc = 6;
	switch (argc)
	{
	case 6:
		if(argv[5][0] == 't')
		{
				ttl = atoi(&argv[5][1]);
		}
		else
		{
			aduLength = atoi(argv[5]);
		}
	case 5:
		if(argv[4][0] == 't')
		{
				ttl = atoi(&argv[4][1]);
		}
		else
		{
			aduLength = atoi(argv[4]);
		}
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
			streaming, ttl);
}
