/*
	bpsource.c:	a test bundle source.
									*/
/*									*/
/*	Copyright (c) 2004, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include <bp.h>

#define	DEFAULT_TTL 300


static int	_running(int *newState)
{
	int	state = 1;

	if (newState)
	{
		state = *newState;
	}

	return state;
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
	int	stop = 0;

	oK(_running(&stop));
	ionPauseAttendant(_attendant(NULL));
}

#if defined (ION_LWT)
int	bpsource(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char		*destEid = (char *) a1;
	char		*arg2 = (char *) a2;
	char		*arg3 = (char *) a3;
#else
int	main(int argc, char **argv)
{
	char		*destEid = NULL;
	char		*arg2 = NULL;
	char		*arg3 = NULL;

	if (argc > 4)
	{
		argc = 4;
	}

	switch (argc)
	{
	case 4:
		arg3 = argv[3];

		/*	Intentional fall-through to next case.		*/
	case 3:
		arg2 = argv[2];

		/*	Intentional fall-through to next case.		*/
	case 2:
		destEid = argv[1];

		/*	Intentional fall-through to next case.		*/
	default:
		break;
	}
#endif
	int		ttl = 300;
	char		*text = NULL;
	Sdr		sdr;
	char		line[256];
	int		lineLength;
	Object		extent;
	Object		bundleZco;
	ReqAttendant	attendant;
	Object		newBundle;
	int		fd;

	if (arg2)
	{
		if (arg2[0] == '-' && arg2[1] == 't')
		{
			ttl = atoi(arg2 + 2);
		}
		else
		{
			text = arg2;
		}
	}

	if (arg3)
	{
		if (arg3[0] == '-' && arg3[1] == 't')
		{
			ttl = atoi(arg3 + 2);
		}
		else
		{
			text = arg3;
		}
	}

	if (destEid == NULL || ttl <= 0)
	{
		PUTS("Usage: bpsource <destination endpoint ID> [\"<text>\"] \
[-t<Bundle TTL>]");
		return 0;
	}

	if (bp_attach() < 0)
	{
		putErrmsg("Can't attach to BP.", NULL);
		return 0;
	}

	if (ionStartAttendant(&attendant))
	{
		putErrmsg("Can't initialize blocking transmission.", NULL);
		return 0;
	}

	_attendant(&attendant);
	sdr = bp_get_sdr();
	if (text)
	{
		lineLength = strlen(text);
		if (lineLength == 0)
		{
			writeMemo("[?] No text for bpsource to send.");
			bp_detach();
			return 0;
		}

		CHKZERO(sdr_begin_xn(sdr));
		extent = sdr_malloc(sdr, lineLength);
		if (extent)
		{
			sdr_write(sdr, extent, text, lineLength);
		}

		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("No space for ZCO extent.", NULL);
			bp_detach();
			return 0;
		}

		bundleZco = ionCreateZco(ZcoSdrSource, extent, 0, lineLength,
				BP_STD_PRIORITY, 0, ZcoOutbound, &attendant);
		if (bundleZco == 0 || bundleZco == (Object) ERROR)
		{
			putErrmsg("Can't create ZCO extent.", NULL);
			bp_detach();
			return 0;
		}

		if (bp_send(NULL, destEid, NULL, ttl, BP_STD_PRIORITY,
				NoCustodyRequested, 0, 0, NULL, bundleZco,
				&newBundle) < 1)
		{
			putErrmsg("bpsource can't send ADU.", NULL);
		}

		ionStopAttendant(_attendant(NULL));
		bp_detach();
		return 0;
	}

#ifndef FSWLOGGER	/*	Need stdin/stdout for interactivity.	*/
	fd = fileno(stdin);
	isignal(SIGINT, handleQuit);
	while (_running(NULL))
	{
		printf(": ");
		fflush(stdout);
		if (igets(fd, line, sizeof line, &lineLength) == NULL)
		{
			if (lineLength == 0)	/*	EOF.		*/
			{
				break;
			}

			putErrmsg("igets failed.", NULL);
			break;
		}

		switch (line[0])
		{
		case '!':
			break;

		case 0:
			continue;

		default:
			CHKZERO(sdr_begin_xn(sdr));
			extent = sdr_malloc(sdr, lineLength);
			if (extent)
			{
				sdr_write(sdr, extent, line, lineLength);
			}

			if (sdr_end_xn(sdr) < 0)
			{
				putErrmsg("No space for ZCO extent.", NULL);
				break;
			}

			bundleZco = ionCreateZco(ZcoSdrSource, extent,
					0, lineLength, BP_STD_PRIORITY, 0,
					ZcoOutbound, &attendant);
			if (bundleZco == 0 || bundleZco == (Object) ERROR)
			{
				putErrmsg("Can't create ZCO extent.", NULL);
				break;
			}

			if (bp_send(NULL, destEid, NULL, ttl, BP_STD_PRIORITY,
					NoCustodyRequested, 0, 0, NULL,
					bundleZco, &newBundle) < 1)
			{
				putErrmsg("bpsource can't send ADU.", NULL);
				break;
			}

			continue;
		}

		break;
	}
#endif
	writeMemo("[i] Stopping bpsource.");
	ionStopAttendant(_attendant(NULL));
	bp_detach();
	return 0;
}
