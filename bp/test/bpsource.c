/*
	bpsource.c:	a test bundle source.
									*/
/*									*/
/*	Copyright (c) 2004, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include <bp.h>

static void	handleQuit()
{
	PUTS("Please enter a '!' character to stop the program.");
}

#if defined (VXWORKS) || defined (RTEMS)
int	bpsource(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	char	*destEid = (char *) a1;
	char	*text = (char *) a2;
#else
int	main(int argc, char **argv)
{
	char	*destEid = (argc > 1 ? argv[1] : NULL);
	char	*text = (argc > 2 ? argv[2] : NULL);
#endif
	Sdr	sdr;
	char	line[256];
	int	lineLength;
	Object	extent;
	Object	bundleZco;
	Object	newBundle;

#ifndef FSWLOGGER	/*	Need stdin/stdout for interactivity.	*/
	if (destEid == NULL)
	{
		PUTS("Usage: bpsource <destination endpoint ID>");
		return 0;
	}

	if (bp_attach() < 0)
	{
		putErrmsg("Can't attach to BP.", NULL);
		return 0;
	}

	sdr = bp_get_sdr();
	if (text)
	{
		lineLength = strlen(text);
		if (lineLength == 0)
		{
			writeMemo("No text for bpsource to send.");
			bp_detach();
			return 0;
		}

		sdr_begin_xn(sdr);
		extent = sdr_malloc(sdr, lineLength);
		if (extent == 0)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("No space for ZCO extent.", NULL);
			bp_detach();
			return 0;
		}

		sdr_write(sdr, extent, text, lineLength);
		bundleZco = zco_create(sdr, ZcoSdrSource, extent,
				0, lineLength);
		if (sdr_end_xn(sdr) < 0 || bundleZco == 0)
		{
			putErrmsg("Can't create ZCO extent.", NULL);
			bp_detach();
			return 0;
		}

		if (bp_send(NULL, BP_BLOCKING, destEid, NULL, 300,
				BP_STD_PRIORITY, NoCustodyRequested,
				0, 0, NULL, bundleZco, &newBundle) < 1)
		{
			putSysErrmsg("bpsource can't send ADU", NULL);
		}

		bp_detach();
		return 0;
	}

	isignal(SIGINT, handleQuit);
	while (1)
	{
		printf(": ");
		if (fgets(line, sizeof line, stdin) == NULL)
		{
			putSysErrmsg("bpsource fgets failed", NULL);
			break;
		}

		lineLength = strlen(line) - 1;	/*	lose newline	*/
		line[lineLength] = 0;
		switch (line[0])
		{
		case '!':
			break;

		case 0:
			continue;

		default:
			sdr_begin_xn(sdr);
			extent = sdr_malloc(sdr, lineLength);
			if (extent == 0)
			{
				sdr_cancel_xn(sdr);
				putErrmsg("No space for ZCO extent.", NULL);
				break;
			}

			sdr_write(sdr, extent, line, lineLength);
			bundleZco = zco_create(sdr, ZcoSdrSource, extent,
					0, lineLength);
			if (sdr_end_xn(sdr) < 0 || bundleZco == 0)
			{
				putErrmsg("Can't create ZCO extent.", NULL);
				break;
			}

			if (bp_send(NULL, BP_BLOCKING, destEid, NULL, 300,
					BP_STD_PRIORITY, NoCustodyRequested,
					0, 0, NULL, bundleZco, &newBundle) < 1)
			{
				putSysErrmsg("bpsource can't send ADU", NULL);
				break;
			}

			continue;
		}

		break;
	}

	writeMemo("Stopping bpsource.");
	bp_detach();
#endif
	return 0;
}
