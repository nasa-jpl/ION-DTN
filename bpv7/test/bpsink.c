/*
	bpsink.c:	a test bundle sink.
									*/
/*									*/
/*	Copyright (c) 2004, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include <bp.h>

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

#if defined (ION_LWT)
int	bpsink(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char		*ownEid = (char *) a1;
#else
int	main(int argc, char **argv)
{
	char		*ownEid = (argc > 1 ? argv[1] : NULL);
#endif
	static char	*deliveryTypes[] =	{
				"Payload delivered.",
				"Reception timed out.",
				"Reception interrupted.",
				"Endpoint stopped."
						};
	BptestState	state = { NULL, 1 };
	Sdr		sdr;
	BpDelivery	dlv;
	int		contentLength;
	ZcoReader	reader;
	int		len;
	char		content[80];
	char		line[84];

#ifndef mingw
	setlinebuf(stdout);
#endif
	if (ownEid == NULL)
	{
		PUTS("Usage: bpsink <own endpoint ID>");
		return 0;
	}

	if (bp_attach() < 0)
	{
		putErrmsg("Can't attach to BP.", NULL);
		return 0;
	}

	if (bp_open(ownEid, &state.sap) < 0)
	{
		putErrmsg("Can't open own endpoint.", ownEid);
		return 0;
	}

	oK(_bptestState(&state));
	sdr = bp_get_sdr();
	isignal(SIGINT, handleQuit);
	while (state.running)
	{
		if (bp_receive(state.sap, &dlv, BP_BLOCKING) < 0)
		{
			putErrmsg("bpsink bundle reception failed.", NULL);
			state.running = 0;
			continue;
		}

		PUTMEMO("ION event", deliveryTypes[dlv.result - 1]);
		if (dlv.result == BpReceptionInterrupted)
		{
			continue;
		}

		if (dlv.result == BpEndpointStopped)
		{
			state.running = 0;
			continue;
		}

		if (dlv.result == BpPayloadPresent)
		{
			CHKZERO(sdr_begin_xn(sdr));
			contentLength = zco_source_data_length(sdr, dlv.adu);
			sdr_exit_xn(sdr);
			isprintf(line, sizeof line, "\tpayload length is %d.",
					contentLength);
			PUTS(line);
			if (contentLength < sizeof content)
			{
				zco_start_receiving(dlv.adu, &reader);
				CHKZERO(sdr_begin_xn(sdr));
				len = zco_receive_source(sdr, &reader,
						contentLength, content);
				if (sdr_end_xn(sdr) < 0 || len < 0)
				{
					putErrmsg("Can't handle delivery.",
							NULL);
					state.running = 0;
					continue;
				}

				content[contentLength] = '\0';
				isprintf(line, sizeof line, "\t'%s'", content);
				PUTS(line);
			}
		}

		bp_release_delivery(&dlv, 1);
	}

	bp_close(state.sap);
	writeErrmsgMemos();
	PUTS("Stopping bpsink.");
	bp_detach();
	return 0;
}
