/*
	bpsink.c:	a test bundle sink.
									*/
/*									*/
/*	Copyright (c) 2004, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include <bp.h>

static BpSAP	_bpsap(BpSAP *newSAP)
{
	static BpSAP	sap = NULL;

	if (newSAP)
	{
		sap = *newSAP;
		sm_TaskVarAdd((int *) &sap);
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

	PUTS("BP reception interrupted.");
	oK(_running(&stop));
	bp_interrupt(_bpsap(NULL));
}

#if defined (VXWORKS) || defined (RTEMS)
int	bpsink(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
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
	BpSAP		sap;
	Sdr		sdr;
	BpDelivery	dlv;
	int		stop = 0;
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

	if (bp_open(ownEid, &sap) < 0)
	{
		putErrmsg("Can't open own endpoint.", ownEid);
		return 0;
	}

	oK(_bpsap(&sap));
	sdr = bp_get_sdr();
	isignal(SIGINT, handleQuit);
	while (_running(NULL))
	{
		if (bp_receive(sap, &dlv, BP_BLOCKING) < 0)
		{
			putErrmsg("bpsink bundle reception failed.", NULL);
			oK(_running(&stop));
			continue;
		}

		PUTMEMO("ION event", deliveryTypes[dlv.result - 1]);
		if (dlv.result == BpReceptionInterrupted)
		{
			continue;
		}

		if (dlv.result == BpEndpointStopped)
		{
			oK(_running(&stop));
			continue;
		}

		if (dlv.result == BpPayloadPresent)
		{
			contentLength = zco_source_data_length(sdr, dlv.adu);
			isprintf(line, sizeof line, "\tpayload length is %d.",
					contentLength);
			PUTS(line);
			if (contentLength < sizeof content)
			{
				sdr_begin_xn(sdr);
				zco_start_receiving(sdr, dlv.adu, &reader);
				len = zco_receive_source(sdr, &reader,
						contentLength, content);
				zco_stop_receiving(sdr, &reader);

				if (sdr_end_xn(sdr) < 0 || len < 0)
				{
					putErrmsg("Can't handle delivery.",
							NULL);
					oK(_running(&stop));
					continue;
				}

				content[contentLength] = '\0';
				isprintf(line, sizeof line, "\t'%s'", content);
				PUTS(line);
			}
		}

		bp_release_delivery(&dlv, 1);
	}

	bp_close(sap);
	writeErrmsgMemos();
	PUTS("Stopping bpsink.");
	bp_detach();
	return 0;
}
