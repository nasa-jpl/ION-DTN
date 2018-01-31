/*
	pmqlsi.c:	LTP PMQ-based link service daemon.  Receives
			LTP segments via a POSIX message queue.

	Author: Scott Burleigh, JPL

	Copyright (c) 2010, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include "pmqlsa.h"

static void	interruptThread()
{
	isignal(SIGTERM, interruptThread);
}

/*	*	*	Receiver thread functions	*	*	*/

typedef struct
{
	mqd_t		mq;
	pthread_t	mainThread;
	int		running;
} ReceiverThreadParms;

static void	*handleMessages(void *parm)
{
	/*	Main loop for message reception and handling.		*/

	ReceiverThreadParms	*rtp = (ReceiverThreadParms *) parm;
	int			segLength;
	char			msgbuf[PMQLSA_MSGSIZE];
	unsigned int		mqp;	/*	Priority of rec'd msg.	*/

	iblock(SIGTERM);
	while (rtp->running)
	{	
		segLength = mq_receive(rtp->mq, msgbuf, sizeof msgbuf, &mqp);
		switch (segLength)
		{
		case 1:				/*	Normal stop.	*/
			continue;

		case -1:
			putSysErrmsg("pmqlsi failed receiving msg", NULL);
			pthread_kill(rtp->mainThread, SIGTERM);
			rtp->running = 0;
			continue;
		}

		if (ltpHandleInboundSegment(msgbuf, segLength) < 0)
		{
			putErrmsg("Can't handle inbound segment.", NULL);
			pthread_kill(rtp->mainThread, SIGTERM);
			rtp->running = 0;
			continue;
		}

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();
	}

	writeErrmsgMemos();
	writeMemo("[i] pmqlsi receiver thread has ended.");
	return NULL;
}

/*	*	*	Main thread functions	*	*	*	*/

#if defined (VXWORKS) || defined (RTEMS)
int	pmqlsi(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char	*mqName = (char *) a1;
#else
int	main(int argc, char *argv[])
{
	char	*mqName = (argc > 1 ? argv[1] : NULL);
#endif
	LtpVdb			*vdb;
	struct mq_attr		mqAttributes =
					{ 0, PMQLSA_MAXMSG, PMQLSA_MSGSIZE, 0 };
	ReceiverThreadParms	rtp;
	pthread_t		receiverThread;
	char			stop = '0';

	if (mqName == NULL)
	{
		puts("Usage: pmqlsi <message queue name>");
		return 0;
	}

	/*	Note that ltpadmin must be run before the first
	 *	invocation of ltplsi, to initialize the LTP database
	 *	(as necessary) and dynamic database.			*/ 

	if (ltpInit(0) < 0)
	{
		putErrmsg("pmqlsi can't initialize LTP.", NULL);
		return 1;
	}

	vdb = getLtpVdb();
	if (vdb->lsiPid > 0 && vdb->lsiPid != sm_TaskIdSelf())
	{
		putErrmsg("LSI task is already started.", itoa(vdb->lsiPid));
		return 1;
	}

	/*	All command-line arguments are now validated.		*/

	rtp.mq = mq_open(mqName, O_RDWR | O_CREAT, 0777, &mqAttributes);
	if (rtp.mq == (mqd_t) -1)
	{
		putSysErrmsg("pmglsi can't open message queue", mqName);
		return 1;
	}

	/*	Set up signal handling; SIGTERM is shutdown signal.	*/

	isignal(SIGTERM, interruptThread);

	/*	Start the receiver thread.				*/

	rtp.running = 1;
	rtp.mainThread = pthread_self();
	if (pthread_create(&receiverThread, NULL, handleMessages, &rtp))
	{
		mq_close(rtp.mq);
		putSysErrmsg("pmqlsi can't create receiver thread", NULL);
		return 1;
	}

	/*	Now sleep until interrupted by SIGTERM, at which point
	 *	it's time to stop the link service.			*/

	writeMemo("[i] pmqlsi is running");
	snooze(2000000000);

	/*	Time to shut down.					*/

	rtp.running = 0;
	mq_send(rtp.mq, &stop, 1, 0);	/*	Tell receiver to stop.	*/
	pthread_join(receiverThread, NULL);
	mq_close(rtp.mq);
	writeErrmsgMemos();
	writeMemo("[i] pmqlsi duct has ended.");
	ionDetach();
	return 0;
}
