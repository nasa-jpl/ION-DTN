/*
	ltpdriver.c:	sender for LTP benchmark test.
									*/
/*									*/
/*	Copyright (c) 2007, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include "platform.h"
#include "zco.h"
#include "ltpP.h"

#define	DEFAULT_ADU_LENGTH	(60000)

static int	run_ltpdriver(int cyclesRemaining, unsigned long destEngineId,
			int aduLength)
{
	static char	buffer[DEFAULT_ADU_LENGTH] = "test...";
	Sdr		sdr;
	int		running = 1;
	int		aduFile;
	int		randomAduLength = 0;
	int		bytesRemaining;
	int		bytesToWrite;
	Object		fileRef;
	Object		zcoRef;
	LtpSessionId	sessionId;
	int		bytesSent = 0;
	time_t		startTime;
	time_t		endTime;
	long		interval;
	int		cycles = cyclesRemaining;

	if (cyclesRemaining == 0 || destEngineId == 0 || aduLength == 0)
	{
		PUTS("Usage: ltpdriver <number of cycles> \
<destination engine ID> [<payload size>]");
		PUTS("  Payload size defaults to 60000 bytes.");
		PUTS("");
		PUTS("  To use payload sizes chosen at random from the");
	       	PUTS("	range 1024 to 62464, in multiples of 1024,");
	       	PUTS("	specify payload size 1.");
		PUTS("");
		PUTS("  Destination (receiving) application must be");
		PUTS("  ltpcounter.");
		return 0;
	}

	if (ltp_attach() < 0)
	{
		putErrmsg("ltpdriver can't initialize LTP.", NULL);
		return 1;
	}

	sdr = getIonsdr();
	if (aduLength == 1)
	{
		randomAduLength = 1;
	}

	aduFile = iopen("ltpdriverAduFile", O_WRONLY | O_CREAT, 0666);
	if (aduFile < 0)
	{
		putSysErrmsg("Can't create ADU file", NULL);
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
			close(aduFile);
			putSysErrmsg("Error writing to ADU file", NULL);
			return 0;
		}

		bytesRemaining -= bytesToWrite;
	}

//sdr_start_trace(sdr, 10000000, NULL);
	close(aduFile);
	sdr_begin_xn(sdr);
	fileRef = zco_create_file_ref(sdr, "ltpdriverAduFile", NULL);
	if (sdr_end_xn(sdr) < 0 || fileRef == 0)
	{
		putErrmsg("ltpdriver can't create file ref.", NULL);
		return 0;
	}
	
	startTime = time(NULL);
	while (running && cyclesRemaining > 0)
	{
		if (randomAduLength)
		{
			aduLength = ((rand() % 60) + 1) * 1024;
		}

		sdr_begin_xn(sdr);
		zcoRef = zco_create(sdr, ZcoFileSource, fileRef, 0,
				aduLength);
		if (sdr_end_xn(sdr) < 0 || zcoRef == 0)
		{
			putErrmsg("ltpdriver can't create ZCO.", NULL);
			return 0;
		}

		switch (ltp_send(destEngineId, 1, zcoRef, aduLength,
				&sessionId))
		{
		case 0:
			putErrmsg("ltpdriver can't send message.",
					itoa(aduLength));
			break;		/*	Out of switch.		*/

		case -1:
			putErrmsg("ltp_send failed.", NULL);
			running = 0;
			continue;
		}

		bytesSent += aduLength;
//putchar('^');
//fflush(stdout);
		cyclesRemaining--;
		if ((cyclesRemaining % 100) == 0)
		{
//sdr_clear_trace(sdr);
//sdr_print_trace(sdr, 0);
			PUTS(itoa(cyclesRemaining));
		}
	}

	endTime = time(NULL);
	writeErrmsgMemos();
	interval = endTime - startTime;
	PUTMEMO("Time (seconds)", itoa(interval));
	if (randomAduLength)
	{
		PUTS("Data size random.");
	}
	else
	{
		PUTMEMO("Data size (bytes)", itoa(aduLength));
	}

	PUTMEMO("Cycles", itoa(cycles));
	PUTMEMO("Bytes", itoa(bytesSent));
	if (interval <= 0)
	{
		PUTS("Interval is too short to measure rate.");
	}
	else
	{
		PUTMEMO("Throughput (bytes per second)",
				itoa(bytesSent / interval));
	}

//sdr_stop_trace(sdr);
	sdr_begin_xn(sdr);
	zco_destroy_file_ref(sdr, fileRef);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("ltpdriver can't destroy file reference.", NULL);
	}

	ltp_detach();
	return 0;
}

#if defined (VXWORKS) || defined (RTEMS)
int	ltpdriver(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	int		cyclesRemaining = a1;
	unsigned long	destEngineId = (unsigned long) a2;
	int		aduLength = (a3 == 0 ? DEFAULT_ADU_LENGTH : a3);
#else
int	main(int argc, char **argv)
{
	int		cyclesRemaining = 0;
	unsigned long	destEngineId = 0;
	int		aduLength = DEFAULT_ADU_LENGTH;

	if (argc > 4) argc = 4;
	switch (argc)
	{
	case 4:
	  	aduLength = atoi(argv[3]);
	case 3:
		destEngineId = strtoul(argv[2], NULL, 0);
	case 2:
		cyclesRemaining = atoi(argv[1]);
	default:
		break;
	}
#endif
	return run_ltpdriver(cyclesRemaining, destEngineId, aduLength);
}
