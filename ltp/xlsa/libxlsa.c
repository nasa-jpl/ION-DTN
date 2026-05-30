/*
	libxlsa.c:	Common functions for the generic "X" LTP link
			service -- the receiver thread.

	This is the transport-agnostic mirror of ltp/udp/libudplsa.c.
	It owns the receive buffer (allocated from ION working memory,
	freed via a pthread_cleanup handler so a cancelled thread does
	not leak), and translates the xport_recv() return-value contract
	into ltpHandleInboundSegment() calls.

	Patterned on ltp/udp/libudplsa.c from nasa-jpl/ion-dtn (integration).
									*/

#include "xlsa.h"

static void	cleanupBuffer(void *arg)
{
	if (arg != NULL)
	{
		MRELEASE(arg);
	}
}

void	*xlsa_handle_datagrams(void *parm)
{
	xlsa_ReceiverThreadParms	*rtp =
					(xlsa_ReceiverThreadParms *) parm;
	char				*procName = "xlsi";
	char				*buffer;
	int				segmentLength;

	snooze(1);	/*	Let main thread become interruptable.	*/

	buffer = MTAKE(XLSA_BUFSZ);
	if (buffer == NULL)
	{
		putErrmsg("xlsi can't get receive buffer.", NULL);
		ionKillMainThread(procName);
		return NULL;
	}

	pthread_cleanup_push(cleanupBuffer, buffer);

	while (1)
	{
		int	keepRunning;

		keepRunning = (int) ion_atomic_get(&rtp->running);
		if (!keepRunning)
		{
			break;
		}

		/*	Pull exactly one segment from the transport.	*/

		segmentLength = xport_recv(rtp->link, buffer, XLSA_BUFSZ);
		switch (segmentLength)
		{
		case 0:				/*	Interrupted.	*/
			continue;

		case -1:
			putSysErrmsg("xlsi can't acquire segment.", NULL);
			ionKillMainThread(procName);

			/* FALLTHROUGH */

		case 1:				/*	Shutdown sentinel.*/
			ion_atomic_set(&rtp->running, 0);
			continue;
		}

		/*	Hand the single complete segment to the engine.	*/

		if (ltpHandleInboundSegment(buffer, segmentLength) < 0)
		{
			putErrmsg("xlsi can't handle inbound segment.", NULL);
			ionKillMainThread(procName);
			ion_atomic_set(&rtp->running, 0);
			continue;
		}

		sm_TaskYield();		/*	Give other tasks a turn.*/
	}

	pthread_cleanup_pop(1);		/*	Free buffer.		*/
	writeErrmsgMemos();
	writeMemo("[i] xlsa receiver thread has ended.");
	return NULL;
}
