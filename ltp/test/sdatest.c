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

static void	interruptThread(int signum)
{
	isignal(SIGTERM, interruptThread);
	sda_interrupt();
}

static vast	getLengthOfItem(unsigned int clientId, unsigned char *buffer,
			vast bufferLength)
{
	return 1 + istrlen((char *) buffer, bufferLength);
}

static int	handleItem(uvast sourceEngineId, unsigned int clientId,
			Object clientServiceData)
{
	Sdr		sdr = getIonsdr();
	ZcoReader	reader;
	char		buffer[MAX_LINE_LEN + 1];
	vast		len;

	zco_start_receiving(clientServiceData, &reader);
	memset(buffer, 0, sizeof buffer);
	len = zco_receive_source(sdr, &reader, MAX_LINE_LEN, buffer);
	if (len < 0)
	{
		return 0;
	}

	buffer[len] = 0;
	printf("%s", buffer);
	return 0;
}

/*	*	*	Sender thread function	*	*	*	*/

typedef struct
{
	uvast		destEngineId;
	ReqAttendant	attendant;
	int		running;
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
		if (sdr_end_xn(sdr) < 0 || extent == 0)
		{
			putErrmsg("Service data item insertion failed.", NULL);
			sda_interrupt();
			stp->running = 0;
			continue;
		}

		item = ionCreateZco(ZcoSdrSource, extent, 0, length, 0, 0,
				ZcoOutbound, &(stp->attendant));
		if (item == 0 || item == (Object) ERROR)
		{
			putErrmsg("Service data item zco create failed.", NULL);
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
		if (ionStartAttendant(&parms.attendant) < 0)
		{
			putErrmsg("sdatest can't start attendant.", NULL);
			return 1;
		}

		parms.running = 1;
		if (pthread_begin(&senderThread, NULL, sendItems,
			&parms, "sdatest_sender"))
		{
			putSysErrmsg("sdatest can't create send thread", NULL);
			ionStopAttendant(&parms.attendant);
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
		ionPauseAttendant(&parms.attendant);
		pthread_join(senderThread, NULL);
		ionStopAttendant(&parms.attendant);
	}

	writeErrmsgMemos();
	writeMemo("[i] sdatest main thread has ended.");
	ionDetach();
	return 0;
}

#if defined (ION_LWT)
int	sdatest(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	uvast	destEngineId = (uvast) a1;
#else
int	main(int argc, char **argv)
{
	uvast	destEngineId = 0;

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
