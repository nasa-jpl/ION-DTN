/*
	bpdriver.c:	sender for bundle benchmark test.
									*/
/*									*/
/*	Copyright (c) 2004, California Institute of Technology.		*/
/*	All rights reserved.									*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*	Enhanced by Ryan Metzger (MITRE Corp.) August 2006.		*/
/*  Injection rate control added by Jay L. Gao, JPL, 2021 	*/
/*															*/

#include <bp.h>

#define	DEFAULT_ADU_LENGTH	(60000)
#define	DEFAULT_TTL		300

/*	Indication marks:	"." for BpPayloadPresent (1),
				"*" for BpReceptionTimedOut (2).
				"!" for BpReceptionInterrupted (3).	*/

//static char	dlvmarks[] = "?.*!";


int running = 1;		/* initialize bpdriver state */

static unsigned long	getUsecTimestamp()
{
	struct timeval	tv;

	getCurrentTime(&tv);
	return ((tv.tv_sec * 1000000) + tv.tv_usec);
}

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
	//isignal(SIGINT, handleQuit);
	printf("Catched Ctrl-C!!!\n");
    running = 0;
	bp_interrupt(_bpsap(NULL));
	ionPauseAttendant(_attendant(NULL));
}

static int	run_bpdriver(int cyclesRemaining, char *ownEid, char *destEid,
			int aduLength, int streaming, int ttl, int injectRate)
{
	static char	buffer[DEFAULT_ADU_LENGTH] = "test...";
	BpSAP		sap;
	Sdr		sdr;
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
	unsigned long		startTimestamp; /* cycle start time in usec */
	unsigned long		endTimestamp; 	/* cycle end time in usec */
	float				cycleTime;		/* desired cycle time in usec */
	float				delayTime;		/* delay added in usec */

	if (cyclesRemaining == 0 || ownEid == NULL || destEid == NULL
	|| aduLength == 0)
	{
		PUTS("Usage: bpdriver <number of cycles> <own endpoint ID> \
<destination endpoint ID> [<payload size>] [t<Bundle TTL>] \
[i<inject data rate>]");
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
	    PUTS("  range 1024 to 62464, in multiples of 1024,");
	    PUTS("  specify payload size 1 (or -1 for streaming mode).");
		PUTS("");
		PUTS("  bpdriver normally runs with custody transfer");
	    PUTS("  disabled.  To request custody transfer for all");
	    PUTS("  bundles sent, specify number of cycles as a");
	    PUTS("  negative number; the absolute value of this");
	    PUTS("  parameter will be used as the actual number of");
	    PUTS("  cycles.");
		PUTS("");
		PUTS("  Inject data rate specifies in bits-per-second");
		PUTS("  the average rate at which bpdriver will");
		PUTS("  send bundles into the network. A negative or");
		PUTS("  0 rate value will turn off injection rate control.");
		PUTS("  By default, bpdriver will inject bundle as fast");
		PUTS("  as it can.");
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

	microsnooze(100000);	/*	Make sure pilot bundle has been sent.	*/

	/*	Begin timed bundle transmission.			*/

	isignal(SIGINT, handleQuit);
	getCurrentTime(&startTime);
	int bSize = 1;		/* burst size for each burst */
	int bCount = 0; 	/* counter for each burst */
	cycleTime = (float) aduLength * 8 * 1000000 / (float) injectRate * bSize;
	startTimestamp = getUsecTimestamp();

	while (running && cyclesRemaining > 0)
	{
		/* measure cycle starting time */

		if ( bCount == 0 ) 
		{
			startTimestamp = getUsecTimestamp();
		}

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
			if (injectRate <= 0)
			{
				continue;
			}
			else
			{
				/*  rate control is on  */

				bCount++;

				/*   check end of a cycle  */

				if ( bCount == bSize )
				{
					endTimestamp = getUsecTimestamp();
					delayTime = cycleTime - (float)(endTimestamp - startTimestamp);

					if ( delayTime < 1 )
					{
						/*  increase burst size  */

						bSize += 1;
						cycleTime = (float) aduLength * 8 * 1000000 / (float) injectRate * bSize;
					
						if ( bSize > 10000 )
						{
							putErrmsg("bpdriver cannot keep up injection rate with burst size < 10,000.", itoa(injectRate));
							bp_close(sap);
							return 0;
						}
					}
					else
					{
						microsnooze((unsigned int)delayTime);
						bCount = 0;
					}
				}
			}

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
	int injectRate = (a6 == 0 ? 0 : atoi((char *) a6));
#else
int	main(int argc, char **argv)
{
	int	cyclesRemaining = 0;
	char	*ownEid = NULL;
	char	*destEid = NULL;
	int	aduLength = DEFAULT_ADU_LENGTH;
	int	streaming = 0;
	int ttl=0;
	int injectRate = 0;
	running = 1;

	if (argc > 7) argc = 7;
	switch (argc)
	{
	case 7:
		if(argv[6][0] == 't')
		{
				ttl = atoi(&argv[6][1]);
		}
		else if (argv[6][0] == 'i')
		{
				injectRate = atoi(&argv[6][1]);
		}
		else
		{
			aduLength = atoi(argv[6]);
		}
	case 6:
		if(argv[5][0] == 't')
		{
				ttl = atoi(&argv[5][1]);
		}
		else if (argv[5][0] == 'i')
		{
				injectRate = atoi(&argv[5][1]);
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
		else if (argv[4][0] == 'i')
		{
				injectRate = atoi(&argv[4][1]);
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
			streaming, ttl, injectRate);
}
