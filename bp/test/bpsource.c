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

static void	_zcoControl(int *controlPtr)
{
	static int	*ptr = NULL;

	if (controlPtr)	/*	Initializing ZCO request cancellation.	*/
	{
		ptr = controlPtr;
	}
	else		/*	Canceling ZCO request.			*/
	{
		ionCancelZcoSpaceRequest(ptr);
	}
}

static void	handleQuit()
{
	int	stop = 0;

	oK(_running(&stop));
	_zcoControl(NULL);
}

#if defined (VXWORKS) || defined (RTEMS) || defined (bionic)
int	bpsource(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	char	*destEid = (char *) a1;
	char	*text = (char *) a2;
	int 	ttl = (a5 == 0 ? DEFAULT_TTL : atoi((char *) a5));
#else
int	main(int argc, char **argv)
{
	char	*destEid = NULL;
	char	*text = NULL;
	int 	ttl = 300;

	if(argc > 4) argc=4;

	switch (argc)
	{
	case 4:
		if(argv[3][0]=='-' && argv[3][1]=='t')
		{
			ttl=atoi(&argv[3][2]);
		}else{
			text=argv[3];
		}
		/*Fall Through*/
	case 3:
		if(argv[2][0]=='-' && argv[2][1]=='t')
		{
			ttl=atoi(&argv[2][2]);
		}else{
			text=argv[2];
		}
		/*Fall Through*/
	case 2:
		destEid = argv[1];
		/*Fall Through*/
	default:
		break;
	}


#endif
	Sdr	sdr;
	char	line[256];
	int	lineLength;
	Object	extent;
	Object	bundleZco;
	int	controlZco = 0;
	Object	newBundle;
	int	fd;

	if (destEid == NULL)
	{
		PUTS("Usage: bpsource <destination endpoint ID> ['<text>'] [-t<Bundle TTL>]");
		return 0;
	}

	if (ttl <= 0)
	{
		PUTS("Usage: bpsource <destination endpoint ID> ['<text>'] \
[-t<Bundle TTL>]");
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
				&controlZco);
		if (bundleZco == 0)
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

		bp_detach();
		return 0;
	}

#ifndef FSWLOGGER	/*	Need stdin/stdout for interactivity.	*/
	fd = fileno(stdin);
	_zcoControl(&controlZco);
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
					0, lineLength, &controlZco);
			if (bundleZco == 0)
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

	writeMemo("[i] Stopping bpsource.");
	bp_detach();
#endif
	return 0;
}
