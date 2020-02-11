/*
	bpcounter.c:	a test bundle counter.
									*/
/*									*/
/*	Copyright (c) 2006, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include <bp.h>
#include <rfx.h>

typedef struct
{
	BpSAP	sap;
	int	running;
} BptestState;

static BptestState	*_bptestState(BptestState *newState)
{
	void		*value;
	BptestState	*state;

	if (newState)			/*	Add task variable.	*/
	{
		value = (void *) (newState);
		state = (BptestState *) sm_TaskVar(&value);
	}
	else				/*	Retrieve task variable.	*/
	{
		state = (BptestState *) sm_TaskVar(NULL);
	}

	return state;
}

static void	handleQuit(int signum)
{
	BptestState	*state;

	isignal(SIGINT, handleQuit);
	PUTS("BP reception interrupted.");
	state = _bptestState(NULL);
	bp_interrupt(state->sap);
	state->running = 0;
}

static int	_bundleCount(int increment)
{
	static int	count = 0;

	if (increment)
	{
		count++;
	}

	return count;
}

static void	*printCounts(void *parm)
{
	PsmAddress	alarm = (PsmAddress) parm;

	while (1)
	{
		if (rfx_alarm_raised(alarm) == 0)
		{
			return NULL;
		}

		PUTMEMO("Bundles received", itoa(_bundleCount(0)));
		fflush(stdout);
	}
}

#if defined (ION_LWT)
int	bpcounter(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char		*ownEid = (char *) a1;
	int		maxCount = a2;
#else
int	main(int argc, char **argv)
{
	char		*ownEid = (argc > 1 ? argv[1] : NULL);
	int		maxCount = (argc > 2 ? atoi(argv[2]) : 0);
#endif
	BptestState	state = { NULL, 1 };
	Sdr		sdr;
	BpDelivery	dlv;
	struct timeval	startTime;
	double		bytesReceived = 0.0;
	int		bundlesReceived = -1;
	PsmAddress	alarm;
	pthread_t	printThread;
	struct timeval	endTime;
	double		interval;
	char		textBuf[256];

	if (ownEid == NULL)
	{
		PUTS("Usage: bpcounter <own endpoint ID> [<max count>]");
		return 0;
	}

	if (maxCount < 1)
	{
		maxCount = 2000000000;
	}

	if (bp_attach() < 0)
	{
		putErrmsg("Can't attach to BP.", NULL);
		return 0;
	}

	if (bp_open(ownEid, &state.sap) < 0)
	{
		putErrmsg("Can't open own endpoint.", ownEid);
		bp_detach();
		return 0;
	}

	oK(_bptestState(&state));
	sdr = bp_get_sdr();
	alarm = rfx_insert_alarm(5, 0);
	if (pthread_begin(&printThread, NULL, printCounts, (void *) alarm, "bpcounter_print"))
	{
		putErrmsg("Can't start print thread.", NULL);
		rfx_remove_alarm(alarm);
		bp_close(state.sap);
		bp_detach();
		return 0;
	}

	isignal(SIGINT, handleQuit);
	while (state.running)
	{
		if (bp_receive(state.sap, &dlv, BP_BLOCKING) < 0)
		{
			putErrmsg("bpcounter bundle reception failed.", NULL);
			state.running = 0;
			continue;
		}

		switch (dlv.result)
		{
		case BpReceptionInterrupted:
			continue;

		case BpEndpointStopped:
			state.running = 0;
			continue;

		case BpPayloadPresent:
			if (bundlesReceived < 0)
			{
				/*	This is just the pilot bundle
				 *	that starts bpcounter's timer.	*/

				getCurrentTime(&startTime);
				bundlesReceived = 0;
			}
			else
			{
				bundlesReceived = _bundleCount(1);
				CHKZERO(sdr_begin_xn(sdr));
				bytesReceived += zco_length(sdr, dlv.adu);
				sdr_exit_xn(sdr);
			}

			break;

		default:
			break;
		}

		bp_release_delivery(&dlv, 1);
		if (bundlesReceived == maxCount)
		{
			state.running = 0;
		}
	}

	getCurrentTime(&endTime);
	rfx_remove_alarm(alarm);
	pthread_join(printThread, NULL);
	bp_close(state.sap);
	PUTMEMO("Stopping bpcounter; bundles received", itoa(bundlesReceived));
	if (bundlesReceived > 0)
	{
		if (endTime.tv_usec < startTime.tv_usec)
		{
			endTime.tv_usec += 1000000;
			endTime.tv_sec -= 1;
		}

		interval = (endTime.tv_usec - startTime.tv_usec)
			+ (1000000 * (endTime.tv_sec - startTime.tv_sec));
		isprintf(textBuf, sizeof textBuf, "%.3f", interval / 1000000);
		PUTMEMO("Time (seconds)", textBuf);
		isprintf(textBuf, sizeof textBuf, "%.0f", bytesReceived);
		PUTMEMO("Total bytes", textBuf);
		if (interval > 0.0)
		{
			isprintf(textBuf, sizeof textBuf, "%.3f",
				((bytesReceived * 8) / (interval / 1000000))
				/ 1000000);
			PUTMEMO("Throughput (Mbps)", textBuf);
		}
		else
		{
			PUTS("Interval is too short to measure rate.");
		}
	}

	writeMemo("[i] bpcounter has ended.");
	bp_detach();
	return 0;
}
