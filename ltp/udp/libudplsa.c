/*

	libudplsa.c:	Common functions for the LTP UDP-based link
			service.

	Author: Scott Burleigh, JPL

	Copyright (c) 2020, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include "udplsa.h"

void	*udplsa_handle_datagrams(void *parm)
{
	/*	Main loop for UDP datagram reception and handling.	*/

	ReceiverThreadParms	*rtp = (ReceiverThreadParms *) parm;
	char			*procName = "udplsi";
	char			*buffer;
	int			segmentLength;
#ifdef UDP_MULTISEND
	char			*buffers;
	struct iovec		*iovecs;
	struct mmsghdr		*msgs;
	unsigned int		batchLength;
	int			i;

	snooze(1);	/*	Let main thread become interruptable.	*/

	/*	Initialize recvmmsg buffers.				*/

	buffers = MTAKE((UDPLSA_BUFSZ + 1)* MULTIRECV_BUFFER_COUNT);
	if (buffers == NULL)
	{
		putErrmsg("No space for segment buffer array.", NULL);
		ionKillMainThread(procName);
		return NULL;
	}

	iovecs = MTAKE(sizeof(struct iovec) * MULTIRECV_BUFFER_COUNT);
	if (iovecs == NULL)
	{
		MRELEASE(buffers);
		putErrmsg("No space for iovec array.", NULL);
		ionKillMainThread(procName);
		return NULL;
	}

	msgs = MTAKE(sizeof(struct mmsghdr) * MULTIRECV_BUFFER_COUNT);
	if (msgs == NULL)
	{
		MRELEASE(iovecs);
		MRELEASE(buffers);
		putErrmsg("No space for mmsghdr array.", NULL);
		ionKillMainThread(procName);
		return NULL;
	}

	memset(msgs, 0, sizeof(struct mmsghdr) * MULTIRECV_BUFFER_COUNT);
	for (i = 0; i < MULTIRECV_BUFFER_COUNT; i++)
	{
		iovecs[i].iov_base = buffers + (i * (UDPLSA_BUFSZ + 1));
		iovecs[i].iov_len = UDPLSA_BUFSZ;
		msgs[i].msg_hdr.msg_iov = iovecs + i;
		msgs[i].msg_hdr.msg_iovlen = 1;
	}

	/*	Can now start receiving bundles.  On failure, take
	 *	down the daemon.					*/

	while (rtp->running)
	{	
		batchLength = recvmmsg(rtp->linkSocket, msgs,
				MULTIRECV_BUFFER_COUNT, MSG_WAITFORONE, NULL);
		switch (batchLength)
		{
		case -1:
			putSysErrmsg("Can't acquire segments", NULL);
			ionKillMainThread(procName);
			rtp->running = 0;

			/*	Intentional fall-through to next case.	*/

		case 0:	/*	Interrupted system call.		*/
			continue;
		}

		buffer = buffers;
		for (i = 0; i < batchLength; i++)
		{
			segmentLength = msgs[i].msg_len;
			if (segmentLength == 1)
			{
				/*	Normal stop.			*/

				rtp->running = 0;
				break;
			}

			if (ltpHandleInboundSegment(buffer, segmentLength) < 0)
			{
				putErrmsg("Can't handle inbound segment.",
						NULL);
				ionKillMainThread(procName);
				rtp->running = 0;
				break;
			}

			buffer += (UDPLSA_BUFSZ + 1);
		}

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();
	}

	MRELEASE(msgs);
	MRELEASE(iovecs);
	MRELEASE(buffers);
#else
	struct sockaddr_in	fromAddr;
	socklen_t		fromSize;

	snooze(1);	/*	Let main thread become interruptable.	*/

	/*	Initialize buffer.					*/

	buffer = MTAKE(UDPLSA_BUFSZ);
	if (buffer == NULL)
	{
		putErrmsg("udplsi can't get UDP buffer.", NULL);
		ionKillMainThread(procName);
		return NULL;
	}

	/*	Can now start receiving bundles.  On failure, take
	 *	down the link service input thread.			*/

	while (rtp->running)
	{	
		fromSize = sizeof fromAddr;
		segmentLength = irecvfrom(rtp->linkSocket, buffer, UDPLSA_BUFSZ,
				0, (struct sockaddr *) &fromAddr, &fromSize);
		switch (segmentLength)
		{
		case 0:	/*	Interrupted system call.		*/
			continue;

		case -1:
			putSysErrmsg("Can't acquire segment", NULL);
			ionKillMainThread(procName);

			/*	Intentional fall-through to next case.	*/

		case 1:				/*	Normal stop.	*/
			rtp->running = 0;
			continue;
		}

		if (ltpHandleInboundSegment(buffer, segmentLength) < 0)
		{
			putErrmsg("Can't handle inbound segment.", NULL);
			ionKillMainThread(procName);
			rtp->running = 0;
			continue;
		}

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();
	}

	MRELEASE(buffer);
#endif
	writeErrmsgMemos();
	writeMemo("[i] udplsa receiver thread has ended.");

	/*	Free resources.						*/

	return NULL;
}
