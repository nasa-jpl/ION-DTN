/*
	sdatest.c:	SDA test daemon, running either as sender
			or as receiver.  If passed destination
			engine ID on command line, acts as sender.
			Sender obtains data from redirected stdin,
			one service data item per line of text.
			NULL-terminated text lines (minus newlines) 
			are the transmitted SDA service data items.
			Receiver handles service data items by
			simply printing them to stdout.
									*/
/*	Copyright (c) 2013, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include "platform.h"
#include "zco.h"
#include "ion.h"
#include "sda.h"

#define	SDA_TEST_CLIENT	(135)
#define	MAX_LINE_LEN	(1023)

static void	interruptThread()
{
	isignal(SIGTERM, interruptThread);
	sda_interrupt();
}

static vast	getLengthOfItem(unsigned char *buffer, vast bufferLength)
{
	return 1 + istrlen((char *) buffer, bufferLength);
}

static int	handleItem(uvast sourceEngineId, unsigned int clientId,
			Object clientServiceData)
{
	Sdr		sdr = getIonsdr();
	ZcoReader	reader;
	char		buffer[MAX_LINE_LEN + 1];

	zco_start_receiving(clientServiceData, &reader);
	zco_receive_source(sdr, &reader, sizeof buffer, buffer);
	printf("%s", buffer);
	return 0;
}

/*	*	*	Sender thread function	*	*	*	*/

typedef struct
{
	uvast	destEngineId;
	int	running;
} SenderThreadParms;

static void	*sendItems(void *parm)
{
	SenderThreadParms	*stp = (SenderThreadParms *) parm;
	Sdr			sdr;
	char			buffer[MAX_LINE_LEN + 1];
	int			length;
	Object			extent;
	Object			item = 0;

	snooze(3);	/*	Let sda_run get started.		*/
	sdr = getIonsdr();
	while (stp->running)
	{
		if (fgets(buffer, MAX_LINE_LEN, stdin) == NULL)
		{
			sda_interrupt();
			stp->running = 0;
			continue;	/*	End of file, and test.	*/
		}

		length = istrlen(buffer, MAX_LINE_LEN) + 1;

		/*	Send NULL-terminated text line as an SDA item.	*/

		CHKNULL(sdr_begin_xn(sdr));
		extent = sdr_insert(sdr, buffer, length);
		if (extent)
		{
			item = zco_create(sdr, ZcoSdrSource, extent, 0, length);
		}

		if (sdr_end_xn(sdr) < 0 || item == 0)
		{
			putErrmsg("Service data item insertion failed.", NULL);
			sda_interrupt();
			stp->running = 0;
			continue;
		}

		if (sda_send(stp->destEngineId, SDA_TEST_CLIENT, item) < 0)
		{
			putErrmsg("Service data item sending failed.", NULL);
			sda_interrupt();
			stp->running = 0;
			continue;
		}

		CHKNULL(sdr_begin_xn(sdr));
		zco_destroy(sdr, item);
		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Service data item deletion failed.", NULL);
			sda_interrupt();
			stp->running = 0;
			continue;
		}
	}

	writeErrmsgMemos();
	writeMemo("[i] sdatest sender thread has ended.");
	return NULL;
}

/*	*	*	Main function	*	*	*	*	*/

static int	run_sdatest(uvast destEngineId)
{
	SenderThreadParms	parms;
	pthread_t		senderThread;

	isignal(SIGTERM, interruptThread);
	if (destEngineId)
	{
		/*	Must start sender thread.			*/

		parms.destEngineId = destEngineId;
		parms.running = 1;
		if (pthread_begin(&senderThread, NULL, sendItems, &parms))
		{
			putSysErrmsg("sdatest can't create send thread", NULL);
			return 1;
		}
	}

	if (sda_run(getLengthOfItem, handleItem) < 0)
	{
		putErrmsg("sdatest sda_run failed.", NULL);
	}

	if (destEngineId)
	{
		parms.running = 0;
		pthread_join(senderThread, NULL);
	}

	writeErrmsgMemos();
	writeMemo("[i] sdatest main thread has ended.");
	ionDetach();
	return 0;
}

#if defined (VXWORKS) || defined (RTEMS) || defined (bionic)
int	sdatest(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	unsigned int	sdaItemLength = a1;
	uvast		destEngineId = (uvast) a2;
#else
int	main(int argc, char **argv)
{
	uvast		destEngineId = 0;

	if (argc > 2) argc = 2;
	switch (argc)
	{
	case 2:
		destEngineId = strtouvast(argv[1]);

	default:
		break;
	}
#endif
	return run_sdatest(destEngineId);
}