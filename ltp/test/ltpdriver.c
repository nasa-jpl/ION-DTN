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

static int	run_ltpdriver(uvast destEngineId, int clientId,
			int cyclesRemaining, int greenLength, int sduLength)
{
	static char	buffer[DEFAULT_ADU_LENGTH] = "test...";
	Sdr		sdr;
	int		running = 1;
	int		aduFile;
	int		randomSduLength = 0;
	int		bytesRemaining;
	int		bytesToWrite;
	Object		fileRef;
	Object		zco;
	LtpSessionId	sessionId;
	int		bytesSent = 0;
	time_t		startTime;
	int		redLength;
	time_t		endTime;
	long		interval;
	int		cycles = cyclesRemaining;

	if (destEngineId == 0 || clientId < 1
	|| cyclesRemaining < 1 || greenLength < 0 || sduLength < 1)
	{
		PUTS("Usage: ltpdriver <destination engine ID> <client ID> \
<number of cycles> <'green' length> [<payload size>]");
		PUTS("  Payload size defaults to 60000 bytes.");
		PUTS("");
		PUTS("  To use payload sizes chosen at random from the");
	       	PUTS("	range 1024 to 62464, in multiples of 1024,");
	       	PUTS("	specify payload size 1.");
		PUTS("");
		PUTS("  Expected destination (receiving) application is");
		PUTS("  ltpcounter.");
		return 0;
	}

	if (ltp_attach() < 0)
	{
		putErrmsg("ltpdriver can't initialize LTP.", NULL);
		return 1;
	}

	sdr = getIonsdr();
	if (sduLength == 1)
	{
		randomSduLength = 1;
	}

	aduFile = iopen("ltpdriverSduFile", O_WRONLY | O_CREAT, 0666);
	if (aduFile < 0)
	{
		putSysErrmsg("Can't create ADU file", NULL);
		return 0;
	}

	if (randomSduLength)
	{
		bytesRemaining = 65536;
	}
	else
	{
		bytesRemaining = sduLength;
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
	CHKZERO(sdr_begin_xn(sdr));
	fileRef = zco_create_file_ref(sdr, "ltpdriverSduFile", NULL,
			ZcoOutbound);
	if (sdr_end_xn(sdr) < 0 || fileRef == 0)
	{
		putErrmsg("ltpdriver can't create file ref.", NULL);
		return 0;
	}
	
	startTime = time(NULL);
	while (running && cyclesRemaining > 0)
	{
		if (randomSduLength)
		{
			sduLength = ((rand() % 60) + 1) * 1024;
		}

		redLength = sduLength - greenLength;
		if (redLength < 0)
		{
			redLength = 0;
		}

		zco = ionCreateZco(ZcoFileSource, fileRef, 0, sduLength, 0,
				0, ZcoOutbound, NULL);
		if (zco == 0 || zco == (Object) ERROR)
		{
			putErrmsg("ltpdriver can't create ZCO.", NULL);
			running = 0;
			continue;
		}

		switch (ltp_send(destEngineId, clientId, zco, redLength,
				&sessionId))
		{
		case 0:
			putErrmsg("ltpdriver can't send SDU.",
					itoa(sduLength));
			break;		/*	Out of switch.		*/

		case -1:
			putErrmsg("ltp_send failed.", NULL);
			running = 0;
			continue;
		}

		bytesSent += sduLength;
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
	if (randomSduLength)
	{
		PUTS("Data size random.");
	}
	else
	{
		PUTMEMO("Data size (bytes)", itoa(sduLength));
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
	CHKZERO(sdr_begin_xn(sdr));
	zco_destroy_file_ref(sdr, fileRef);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("ltpdriver can't destroy file reference.", NULL);
	}

	ltp_detach();
	return 0;
}

#if defined (ION_LWT)
int	ltpdriver(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	uvast		destEngineId = (uvast) a1;
	int		clientId = a2;
	int		cycles = a3;
	int		greenLen = a4;
	int		aduLen = (a5 == 0 ? DEFAULT_ADU_LENGTH : a5);
#else
int	main(int argc, char **argv)
{
	uvast		destEngineId = 0;
	int		clientId = 0;
	int		cycles = 0;
	int		greenLen = 0;
	int		aduLen = DEFAULT_ADU_LENGTH;

	if (argc > 6) argc = 6;
	switch (argc)
	{
	case 6:
	  	aduLen = strtol(argv[5], NULL, 0);

	case 5:
	  	greenLen = strtol(argv[4], NULL, 0);

	case 4:
	  	cycles = strtol(argv[3], NULL, 0);

	case 3:
		clientId = strtol(argv[2], NULL, 0);

	case 2:
		destEngineId = strtouvast(argv[1]);

	default:
		break;
	}
#endif
	return run_ltpdriver(destEngineId, clientId, cycles, greenLen, aduLen);
}
