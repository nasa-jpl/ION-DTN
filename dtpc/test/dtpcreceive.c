/*
	dtpcreceive.c:	Receiver for DTPC benchmark test.

	Authors: Giorgos Papastergiou, SPICE
		 Ioannis Alexiadis, SPICE

	Copyright (c) 2011, Space Internetworking Center,
	Democritus University of Thrace. ALL RIGHTS RESERVED.
										*/

#include <dtpc.h>

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

static int	_running(int *newState)
{
	static int	state = 1;

	if (newState)
	{
		state = *newState;
	}

	return state;
}

static void	handleQuit()
{
	int	stop = 0;

	PUTS("DTPC reception interrupted.");
	oK(_running(&stop));
	dtpc_interrupt(_dtpcsap(NULL));
}

static int	_payloadCount(int increment)
{
	static int	count = 0;

	if (increment)
	{
		count++;
	}

	return count;
}

static int	_bytesReceived(int increment)
{
	static int	bytes = 0;

	if (increment)
	{
		bytes += increment;
	}

	return bytes;
}

static int	_startTime(time_t newStartTime)
{
	static time_t start = 0;
	
	if (newStartTime)
	{
		start = newStartTime;
	}
	
	return start;
}

static int	printCount(void *userData)
{
	PUTMEMO("Payloads received", itoa(_payloadCount(0)));
	fflush(stdout);
	return 0;
}

#if defined (ION_LWT)
int	dtpcreceive(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	int		topicID = a1;
#else
int	main(int argc, char **argv)
{
	int		topicID = (argc > 1 ? atoi(argv[1]) : 0);
#endif
	DtpcSAP		sap;
	DtpcDelivery	dlv;
	int		stop = 0;
	IonAlarm	alarm = { 5, 0, printCount, NULL };
	pthread_t	alarmThread;
	time_t		startTime = 0;
	time_t		endTime;
	int 		interval;

	if (topicID == 0)
	{
		PUTS("Usage: dtpcreceive <topic ID>");
		return 0;
	}

	if (dtpc_attach() < 0)
	{
		putErrmsg("Can't attach to DTPC.", NULL);
		return 0;
	}

	if (dtpc_open(topicID, NULL, &sap) < 0)
	{
		putErrmsg("Can't open own topic.", itoa(topicID));
		return 0;
	}

	oK(_dtpcsap(&sap));
	ionSetAlarm(&alarm, &alarmThread);
	isignal(SIGINT, handleQuit);
	while (_running(NULL))
	{
		if (dtpc_receive(sap, &dlv, DTPC_BLOCKING) < 0)
		{
			putErrmsg("dtpcreceive payload reception failed.",
					NULL);
			oK(_running(&stop));
			continue;
		}

		switch (dlv.result)
		{
		case ReceptionInterrupted:
			continue;

		case DtpcServiceStopped:
			oK(_running(&stop));
			continue;

		case PayloadPresent:
			if ( _payloadCount(1) == 1)
			{
				_startTime(time(NULL));
			}

			_bytesReceived(dlv.length);
			break;

		default:
			break;
		}

		dtpc_release_delivery(&dlv);
	}

	dtpc_close(sap);
	startTime = _startTime(0);
	if (startTime)
	{
		endTime = time(NULL);
		interval = endTime - startTime;
		PUTMEMO("Time (seconds)", itoa(interval));
		if (interval > 0)
		{
			PUTMEMO("Throughput (bytes per second)",
					itoa(_bytesReceived(0) / interval));
		}
	}

	PUTMEMO("Stopping dtpcreceive on topic", itoa(topicID));
	PUTMEMO("Final tally of payloads received", itoa(_payloadCount(0)));
	ionDetach();
	return 0;
}
