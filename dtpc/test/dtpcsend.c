/*
	dtpcsend.c:	Sender for DTPC benchmark test.

	Authors: Giorgos Papastergiou, SPICE
		 Ioannis Alexiadis, SPICE

	Copyright (c) 2011, Space Internetworking Center,
	Democritus University of Thrace. ALL RIGHTS RESERVED.
									*/

#include "dtpc.h"

#define BUF_SIZE	(50000)
#define MIN_RATE	(1000)
#define MAX_RATE	(200000000)
#define MIN_PAYLOADSIZE	(2)
#define MAX_PAYLOADSIZE	(1000000)


static DtpcSAP	_dtpcsap(DtpcSAP *newSAP)
{
	static DtpcSAP	sap = NULL;

	if (newSAP)
	{
		sap = *newSAP;
		sm_TaskVar((void **) &sap);
	}

	return sap;
}

static int	checkElision(Object recordsList)
{
	Sdr		sdr = getIonsdr();
	Object		elt;
	Object		nextElt;
	Object		obj;
	PayloadRecord	item;
	uvast		firstLength;
	uvast		length;

	CHKZERO(sdr_begin_xn(sdr));
	firstLength = 0;
	for (elt = sdr_list_first(sdr, recordsList); elt; elt = nextElt)
	{
		nextElt = sdr_list_next(sdr, elt);
		obj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &item, obj, sizeof(PayloadRecord));
		oK(decodeSdnv(&length, item.length.text));
		if (firstLength == 0)
		{
			firstLength = length;
			continue;
		}

		if (length == firstLength)	/*	Duplicate.	*/
		{
			sdr_list_delete(sdr, elt, NULL, NULL);
			sdr_free(sdr, item.payload);
			sdr_free(sdr, obj);
		}
	}

	return sdr_end_xn(sdr);
}

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

	oK(_running(&stop));
}

static int	run_dtpcsend(int cycles, int rate, int recordLength,
			int topicID, int profileID, char *dstEid)
{
	static char	buffer[BUF_SIZE] = "this is a testfile created by \
dtpcsend";
	int		stop = 0;
	int		cyclesRemaining;
	char		randomLength = 0;	/*	Boolean		*/
	int		bytesRemaining;
	int		bytesToWrite;
	unsigned int	usecs;
	float		sleepSecs;
	time_t		startTime;
	time_t		endTime;
	int		interval;
	int		bytesSent = 0;
	char		totalbytes[21];
	DtpcSAP		sap;
	Sdr		sdr;
	Object		aduObj;
	Address		addr;
	DtpcElisionFn	elisionFn;

	elisionFn = checkElision;
	if (cycles < 1 || rate < MIN_RATE || rate > MAX_RATE ||
		(recordLength < MIN_PAYLOADSIZE && recordLength != 1) ||
		recordLength > MAX_PAYLOADSIZE || topicID < 1 ||
		profileID < 1 || dstEid == NULL)
	{
		PUTS("Usage: dtpcsend <number of cycles> <rate(bits/sec)> \
<payload size(bytes)> <topic ID> <profile ID> <destination endpoint>");
		PUTS("");
		PUTS("  Rate must be bewteen 1000 and 200000000 bits/sec.");
		PUTS("  Payload size must be between 2 and 1000000 bytes.");
		PUTS("  To use payload sizes chosen at random from the");
	       	PUTS("	range 1 to 65536, specify payload size 1.");
		PUTS("");
		return 0;
	}

	if (dtpc_attach() < 0)
	{
		putErrmsg("Can't attach to DTPC.", NULL);
		return 0;
	}

	if (dtpc_open(topicID, elisionFn, &sap) < 0)
	{
		putErrmsg("Can't open own topic.", itoa(topicID));
		return 0;
	}

	oK(_dtpcsap(&sap));
	sdr = getIonsdr();
	if (recordLength == 1)
	{
		randomLength = 1;
		srand((unsigned int) time(NULL));
	}

	isignal(SIGINT, handleQuit);
	cyclesRemaining = cycles;
	startTime = time(NULL);
	while (_running(NULL) && cyclesRemaining > 0)
	{
		if (randomLength)
		{
			recordLength = (rand() % 65536) + 1;
		}

		bytesRemaining = recordLength;
		if (sdr_begin_xn(sdr) == 0)
		{
			putErrmsg("Can't begin ION transaction.", NULL);
			oK(_running(&stop));
			continue;
		}

		aduObj = sdr_malloc(sdr, recordLength);
		if (aduObj == 0)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("No space in SDR for adu.", NULL);
			oK(_running(&stop));
			continue;
		}

		addr = aduObj;
		while (bytesRemaining > 0)
		{
			if (bytesRemaining > BUF_SIZE)
			{
				bytesToWrite = BUF_SIZE;
			}
			else
			{
				bytesToWrite = bytesRemaining;
			}

			sdr_write(sdr, addr, (char *) buffer, bytesToWrite);
			bytesRemaining -= bytesToWrite;
			addr += bytesToWrite;
		}

		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Can't create adu object in SDR.", NULL);
			oK(_running(&stop));
			continue;
		}

		switch (dtpc_send(profileID, sap, dstEid, 0, 0, 0, 0, NULL, 0,
				0, NULL, 0, aduObj, recordLength))
		{
		case -1:
			putErrmsg("Can't send adu.", NULL);
			oK(_running(&stop));
			continue;

		case 0:		/* This payload does not fit in an adu	*/
			if (sdr_begin_xn(sdr) == 0)
			{
				putErrmsg("Can't discard payload.", NULL);
				oK(_running(&stop));
				continue;
			}

			sdr_free(sdr, aduObj);
			if (sdr_end_xn(sdr) < 0)
			{
				putErrmsg("Can't discard payload.", NULL);
				oK(_running(&stop));
				continue;
			}

			break;

		case 1:
			bytesSent += recordLength;

		default:
			break;
		}

		sleepSecs = (1.0 / ((float) rate)) * ((float) recordLength) * 8;
		usecs = (unsigned int)( sleepSecs * 1000000 );
		microsnooze(usecs);
		cyclesRemaining--;
		if ((cyclesRemaining % 1000) == 0 && cyclesRemaining != 0)
		{
			printf("Cycles remaining: %d\n",cyclesRemaining);
		}
	}

	dtpc_close(sap);
	endTime = time(NULL);
	writeErrmsgMemos();
	PUTS("Stopping dtpcsend.");
	interval = endTime - startTime;
	PUTMEMO("Time (seconds)", itoa(interval));
	PUTMEMO("Total payloads", itoa(cycles - cyclesRemaining));
	isprintf(totalbytes, sizeof totalbytes, "%li", bytesSent);
	PUTMEMO("Total bytes", totalbytes);
	if (interval <= 0)
	{
		PUTS("Interval is too short to measure rate.");
	}
	else
	{
		PUTMEMO("Throughput (bytes per second)",
				itoa(bytesSent / interval));
	}

	ionDetach();
	return 0;
}

#if defined (ION_LWT)
int	dtpcsend(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	int	cycles = atoi((char *) a1);
	int	rate = atoi((char *) a2);
	int	recordLength = atoi((char *) a3);
	int	topicID = atoi((char *) a4);
	int	profileID = atoi((char *) a5);
	char	*dstEid = (char *) a6;
#else
int	main(int argc, char **argv)
{
	int	cycles = 0;
	int	rate = 0;
	int	recordLength = 0;
	int	topicID = 0;
	int	profileID = 0;
	char	*dstEid = NULL;

	if (argc > 7) argc = 7;
	switch (argc)
	{
	case 7:
		dstEid = argv[6];
	case 6:
		profileID = atoi(argv[5]);
	case 5:
	  	topicID = atoi(argv[4]);
	case 4:
		recordLength = atoi(argv[3]);
	case 3:
		rate = atoi(argv[2]);
	case 2:
		cycles = atoi(argv[1]);
	default:
		break;
	}
#endif
	return run_dtpcsend(cycles, rate, recordLength, topicID, profileID,
			dstEid);
}
