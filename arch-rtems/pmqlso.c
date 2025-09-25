/*
	pmqlso.c:	LTP PMQ-based link service output daemon.
			Transmits LTP segments via a POSIX message queue.

	Author: Scott Burleigh, JPL

	Copyright (c) 2010, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include "pmqlsa.h"

static sm_SemId	_pmqlsoSemaphore(sm_SemId *semptr)
{
	long		temp;
	void		*value;
	sm_SemId	sem;

	if (semptr)			/*	Add task variable.	*/
	{
		temp = *semptr;
		value = (void *) temp;
		sem = (sm_SemId) sm_TaskVar(&value);
	}
	else				/*	Retrieve task variable.	*/
	{
		sem = (sm_SemId) sm_TaskVar(NULL);
	}

	return sem;
}

static void	interruptThread()	/*	Shuts down LSO.		*/
{
	void	*erase = NULL;

	sm_SemEnd(_pmqlsoSemaphore(NULL));
	oK(sm_TaskVar(&erase));
}

int	sendSegmentByPMQ(mqd_t mq, char *from, int length)
{
	int	result;

	while (1)	/*	Continue until not interrupted.		*/
	{
		result = mq_send(mq, from, length, 0);
		if (result < 0)
		{
			if (errno == EINTR)	/*	Interrupted.	*/
			{
				continue;	/*	Retry.		*/
			}
		}

		return result;
	}
}

/*	*	*	Main thread functions	*	*	*	*/

#if defined (VXWORKS) || defined (RTEMS)
int	pmqlso(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char		*mqName = (char *) a1;
	uvast		remoteEngineId = a2 != 0 ? strtouvast((char *) a2) : 0;
#else
int	main(int argc, char *argv[])
{
	char		*mqName = argc > 1 ? argv[1] : NULL;
	uvast		remoteEngineId = argc > 2 ? strtouvast(argv[2]) : 0;
#endif
	Sdr		sdr;
	LtpVspan	*vspan;
	PsmAddress	vspanElt;
	struct mq_attr	mqAttributes = { 0, PMQLSA_MAXMSG, PMQLSA_MSGSIZE, 0 };
	mqd_t		mq;
	int		running;
	int		segmentLength;
	char		*segment;

	if (remoteEngineId == 0 || mqName == NULL)
	{
		puts("Usage: pmqlso <message queue name> <remote engine ID>");
		return 0;
	}

	/*	Note that ltpadmin must be run before the first
	 *	invocation of ltplso, to initialize the LTP database
	 *	(as necessary) and dynamic database.			*/

	if (ltpInit(0) < 0)
	{
		putErrmsg("pmqlso can't initialize LTP.", NULL);
		return 1;
	}

	sdr = getIonsdr();
	CHKERR(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	findSpan(remoteEngineId, &vspan, &vspanElt);
	if (vspanElt == 0)
	{
		sdr_exit_xn(sdr);
		putErrmsg("No such engine in database.", itoa(remoteEngineId));
		return 1;
	}

	if (vspan->lsoPid > 0 && vspan->lsoPid != sm_TaskIdSelf())
	{
		sdr_exit_xn(sdr);
		putErrmsg("LSO task is already started for this span.",
				itoa(vspan->lsoPid));
		return 1;
	}

	/*	All command-line arguments are now validated.		*/

	sdr_exit_xn(sdr);
	mq = mq_open(mqName, O_RDWR | O_CREAT, 0777, &mqAttributes);
	if (mq == (mqd_t) -1)
	{
		putSysErrmsg("pmqlso can't open message queue", mqName);
		return 1;
	}

	oK(_pmqlsoSemaphore(&vspan->segSemaphore));
	isignal(SIGTERM, interruptThread);

	/*	Can now begin transmitting to remote engine.		*/

	writeMemo("[i] pmqlso is running.");
	running = 1;
	while (running && !(sm_SemEnded(_pmqlsoSemaphore(NULL))))
	{
		segmentLength = ltpDequeueOutboundSegment(vspan, &segment);
		if (segmentLength < 0)
		{
			running = 0;	/*	Terminate LSO.		*/
			continue;
		}

		if (segmentLength == 0)	/*	Interrupted.		*/
		{
			continue;
		}

		if (segmentLength > PMQLSA_MSGSIZE)
		{
			putErrmsg("Segment is too big for PMQ LSO.",
					itoa(segmentLength));
			running = 0;	/*	Terminate LSO.		*/
			continue;
		}

		if (sendSegmentByPMQ(mq, segment, segmentLength) < 0)
		{
			putSysErrmsg("pmqlso failed sending segment", mqName);
			running = 0;	/*	Terminate LSO.	*/
			continue;
		}

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();
	}

	mq_close(mq);
	writeErrmsgMemos();
	writeMemo("[i] pmqlso duct has ended.");
	ionDetach();
	return 0;
}
