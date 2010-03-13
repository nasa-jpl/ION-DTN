/*
	bpsink.c:	a test bundle sink.
									*/
/*									*/
/*	Copyright (c) 2004, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include <bp.h>

static char	*deliveryTypes[] =	{
			"Payload delivered",
			"Reception timed out",
			"Reception interrupted"
					};
static int	running;
static BpSAP	sap;

static void	handleQuit()
{
	running = 0;
	bp_interrupt(sap);
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
	Sdr		sdr;
	BpDelivery	dlv;
	int		contentLength;
	ZcoReader	reader;
	int		len;
	char		content[80];

	setlinebuf(stdout);
	if (ownEid == NULL)
	{
		puts("Usage: bpsink <own endpoint ID>");
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
	isignal(SIGINT, handleQuit);
	running = 1;
	while (running)
	{
		if (bp_receive(sap, &dlv, BP_BLOCKING) < 0)
		{
			putErrmsg("bpsink bundle reception failed.", NULL);
			running = 0;
			continue;
		}

		printf("ION event: %s.\n", deliveryTypes[dlv.result - 1]);
		if (dlv.result == BpPayloadPresent)
		{
			contentLength = zco_source_data_length(sdr, dlv.adu);
			printf("\tpayload length is %d.\n", contentLength);
			if (contentLength < 80)
			{
				sdr_begin_xn(sdr);
				zco_start_receiving(sdr, dlv.adu, &reader);
				len = zco_receive_source(sdr, &reader,
						contentLength, content);
				zco_stop_receiving(sdr, &reader);
				if (len < 0)
				{
					sdr_cancel_xn(sdr);
					putErrmsg("Can't receive payload.",
							NULL);
					running = 0;
					continue;
				}

				if (sdr_end_xn(sdr) < 0)
				{
					putErrmsg("Can't handle delivery.",
							NULL);
					running = 0;
					continue;
				}

				content[contentLength] = '\0';
				printf("\t'%s'\n", content);
			}
		}

		bp_release_delivery(&dlv, 1);
	}

	bp_close(sap);
	writeErrmsgMemos();
	puts("Stopping bpsink.");
	bp_detach();
	return 0;
}
