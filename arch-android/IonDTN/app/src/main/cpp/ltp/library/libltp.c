/*
 *	libltp.c:	functions enabling the implementation of
 *			LTP applications.
 *
 *	Copyright (c) 2007, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 *
 */

#include "ltpP.h"

int	ltp_attach()
{
	return ltpAttach();
}

void	ltp_detach()
{
#if (!(defined (ION_LWT)))
	ltpDetach();
#endif
	ionDetach();
}

int	ltp_engine_is_started()
{
	LtpVdb	*vdb = getLtpVdb();

	return (vdb && vdb->clockPid != ERROR);
}

static int	sduCanBeAppendedToBlock(LtpSpan *span,
			unsigned int clientSvcId,
			unsigned int redPartLength)
{
	Sdr	sdr = getIonsdr();

	if (span->lengthOfBufferedBlock == 0)
	{
		/*	This is a brand-new, empty block.
		 *
		 *	If the span's session count exceeds the
		 *	limit, then this is the over-limit session
		 *	and it can ONLY be used for transmission
		 *	of an all-green block.
		 *
		 *	Note that appending any single SDU with
		 *	green length greater than zero terminates
		 *	aggregation for this session (whether the
		 *	block was previously empty or not) and
		 *	causes the block to be segmented immediately.
		 *	When transmission via the Span is enabled,
		 *	these segments will be dequeued; dequeuing
		 *	the last segment in the block -- a green
		 *	EOB -- will cause the session to be closed,
		 *	reducing the length of the session list to
		 *	no more than the limit.  This will enable
		 *	another (possibly over-limit) session to
		 *	be started, which again can be used for
		 *	transmission of all-green data even if the
		 *	session limit has been reached (meaning
		 *	the transmission of new red data must wait
		 *	until one of the blocked -- therefore
		 *	implicitly all-red -- sessions is closed).
		 *
		 *	So green data may continue to flow even
		 *	while the session count is at the limit,
		 *	because it doesn't result in growth in
		 *	resource allocation (no retransmission
		 *	buffers are maintained).  The LTP flow
		 *	control window applies only to the flow
		 *	of red data.					*/

		if (sdr_list_length(sdr, span->exportSessions)
				> span->maxExportSessions)
		{
			if (redPartLength == 0)	/*	All-green SDU.	*/
			{
				return 1;	/*	Okay.		*/
			}

			return 0;	/*	Red data; no good.	*/
		}

		/*	This is not the over-limit session, so any SDU
		 *	of any size can be inserted into the block at
		 *	this time.					*/

		return 1;			/*	Okay.		*/
	}

	/*	The current export block already contains some data.	*/

	if (span->lengthOfBufferedBlock > span->redLengthOfBufferedBlock)
	{
		/*	Block contains some green data, so it's
		 *	already released for transmission and just
		 *	hasn't been fully segmented yet, possibly
		 *	because transmission is stopped.  Can't
		 *	append the SDU to this block, must wait for
		 *	the next session.				*/

		return 0;
	}

	if (span->lengthOfBufferedBlock >= span->aggrSizeLimit)
	{
		/*	Block has reached its aggregation limit, so
		 *	it's already released for transmission and
		 *	just hasn't been fully segmented yet, possibly
		 *	because transmission is stopped.  Can't
		 *	append the SDU to this block, must wait for
		 *	the next session.				*/

		return 0;
	}

	if (clientSvcId != span->clientSvcIdOfBufferedBlock)
	{
		/*	This SDU is destined for a different client
		 *	than the client for all other data in the
		 *	block.  Can't append the SDU to this block,
		 *	must wait for the next session.  (This block
		 *	will be released for transmission either when
		 *	its aggregation size limit is reached, due to
		 *	additional traffic inserted by some other
		 *	thread, or when the session's aggregation
		 *	time limit is reached.				*/

		return 0;
	}

	/*	No problems, can append SDU to this block.		*/

	return 1;
}

int	ltp_send(uvast destinationEngineId, unsigned int clientSvcId,
		Object clientServiceData, unsigned int redPartLength,
		LtpSessionId *sessionId)
{
	LtpVdb		*vdb = getLtpVdb();
	Sdr		sdr = getIonsdr();
	LtpVspan	*vspan;
	PsmAddress	vspanElt;
	unsigned int	dataLength;
	Object		spanObj;
	LtpSpan		span;
			OBJ_POINTER(ExportSession, session);

	CHKERR(clientSvcId <= MAX_LTP_CLIENT_NBR);
	CHKERR(clientServiceData);
	CHKERR(sdr_begin_xn(sdr));
	findSpan(destinationEngineId, &vspan, &vspanElt);
	if (vspanElt == 0)
	{
		sdr_exit_xn(sdr);
		putErrmsg("Destination engine unknown.",
				utoa(destinationEngineId));
		return -1;
	}

	dataLength = zco_length(sdr, clientServiceData);

	/*	We spare the client service from needing to know the
	 *	exact length of the ZCO before calling ltp_send():
	 *	if the client service data is all red, the red length
	 *	LTP_ALL_RED can be specified and we simply reduce it
	 *	to the actual ZCO length here.				*/

	if (redPartLength > dataLength)
	{
		redPartLength = dataLength;
	}

	spanObj = sdr_list_data(sdr, vspan->spanElt);
	sdr_stage(sdr, (char *) &span, spanObj, sizeof(LtpSpan));

	/*	All service data units aggregated into any single
	 *	block must have the same client service ID, and
	 *	no service data unit can be added to a block that
	 *	has any green data (only all-red service data units
	 *	can be aggregated in a single block).			*/

	while (1)
	{
		if (span.currentExportSessionObj)
		{
			/*	Span has been initialized with a
			 *	session buffer (block) into which
			 *	service data can be inserted.		*/

			if (sduCanBeAppendedToBlock(&span, clientSvcId,
					redPartLength))
			{
				break;		/*	Out of loop.	*/
			}
		}

		/*	Can't append service data unit to block.  Wait
		 *	until block is open for insertion of SDUs of
		 *	the same color as the SDU we're trying to send,
		 *	based on redPartLength.				*/

		sdr_exit_xn(sdr);
		if (redPartLength > 0)
		{
			if (sm_SemTake(vspan->bufOpenRedSemaphore) < 0)
			{
				putErrmsg("Can't take buffer open semaphore.",
						itoa(vspan->engineId));
				return -1;
			}

			if (sm_SemEnded(vspan->bufOpenRedSemaphore))
			{
				putErrmsg("Span has been stopped.",
						itoa(vspan->engineId));
				return 0;
			}
		}
		else
		{
			if (sm_SemTake(vspan->bufOpenGreenSemaphore) < 0)
			{
				putErrmsg("Can't take buffer open semaphore.",
						itoa(vspan->engineId));
				return -1;
			}

			if (sm_SemEnded(vspan->bufOpenGreenSemaphore))
			{
				putErrmsg("Span has been stopped.",
						itoa(vspan->engineId));
				return 0;
			}
		}

		CHKERR(sdr_begin_xn(sdr));
		sdr_stage(sdr, (char *) &span, spanObj, sizeof(LtpSpan));
	}

	/*	Now append the outbound SDU to the block that is
	 *	currently being aggregated for this span and, if the
	 *	block buffer is now full or the block buffer contains
	 *	any green data, notify ltpmeter that block segmentation
	 *	can begin.						*/

	GET_OBJ_POINTER(sdr, ExportSession, session,
			span.currentExportSessionObj);
	sdr_list_insert_last(sdr, session->svcDataObjects, clientServiceData);
	span.clientSvcIdOfBufferedBlock = clientSvcId;
	span.lengthOfBufferedBlock += dataLength;
	span.redLengthOfBufferedBlock += redPartLength;
	sdr_write(sdr, spanObj, (char *) &span, sizeof(LtpSpan));
	if (span.lengthOfBufferedBlock >= span.aggrSizeLimit
	|| span.redLengthOfBufferedBlock < span.lengthOfBufferedBlock)
	{
		sm_SemGive(vspan->bufClosedSemaphore);
	}

	if (vdb->watching & WATCH_d)
	{
		iwatch('d');
	}

	if (sdr_end_xn(sdr))
	{
		putErrmsg("Can't send data.", NULL);
		return -1;
	}

	sessionId->sourceEngineId = vdb->ownEngineId;
	sessionId->sessionNbr = session->sessionNbr;
	return 1;
}

int	ltp_open(unsigned int clientSvcId)
{
	return ltpAttachClient(clientSvcId);
}

int	ltp_get_notice(unsigned int clientSvcId, LtpNoticeType *type,
		LtpSessionId *sessionId, unsigned char *reasonCode,
		unsigned char *endOfBlock, unsigned int *dataOffset,
		unsigned int *dataLength, Object *data)
{
	Sdr		sdr = getIonsdr();
	LtpVdb		*vdb = getLtpVdb();
	LtpVclient	*client;
	Object		elt;
	Object		noticeAddr;
	LtpNotice	notice;

	CHKERR(clientSvcId <= MAX_LTP_CLIENT_NBR);
	CHKERR(type);
	CHKERR(sessionId);
	CHKERR(reasonCode);
	CHKERR(endOfBlock);
	CHKERR(dataOffset);
	CHKERR(dataLength);
	CHKERR(data);
	*type = LtpNoNotice;	/*	Default.			*/
	*data = 0;		/*	Default.			*/
	CHKERR(sdr_begin_xn(sdr));
	client = vdb->clients + clientSvcId;
	if (client->pid != sm_TaskIdSelf())
	{
		sdr_exit_xn(sdr);
		putErrmsg("Can't get notice: not owner of client service.",
				itoa(client->pid));
		return -1;
	}

	elt = sdr_list_first(sdr, client->notices);
	if (elt == 0)
	{
		sdr_exit_xn(sdr);

		/*	Wait until LTP engine announces an event
		 *	by giving the client's semaphore.		*/

		if (sm_SemTake(client->semaphore) < 0)
		{
			putErrmsg("LTP client can't take semaphore.", NULL);
			return -1;
		}

		if (sm_SemEnded(client->semaphore))
		{
			writeMemo("[?] Client access terminated.");

			/*	End task, but without error.		*/

			return -1;
		}

		CHKERR(sdr_begin_xn(sdr));
		elt = sdr_list_first(sdr, client->notices);
		if (elt == 0)	/*	Function was interrupted.	*/
		{
			sdr_exit_xn(sdr);
			return 0;
		}
	}

	/*	Got next inbound notice.  Remove it from the queue
	 *	for this client.					*/

	noticeAddr = sdr_list_data(sdr, elt);
	sdr_list_delete(sdr, elt, (SdrListDeleteFn) NULL, NULL);
	sdr_read(sdr, (char *) &notice, noticeAddr, sizeof(LtpNotice));
	sdr_free(sdr, noticeAddr);
	*data = notice.data;

	/*	Note that an ExportSessionCanceled notice may have
	 *	associated data of zero, in the event that local
	 *	cancellation by the sender and remote cancellation
	 *	by the receiver happen to occur at almost exactly
	 *	the same time: the first cancellation delivers all
	 *	service data units in the export session, causing
	 *	the session's list of service data units to be
	 *	destroyed, but both cancellations cause notices
	 *	to be sent to the user.					*/

	if (sdr_end_xn(sdr))
	{
		putErrmsg("Can't get inbound notice.", NULL);
		return -1;
	}

	*type = notice.type;
	sessionId->sourceEngineId = notice.sessionId.sourceEngineId;
	sessionId->sessionNbr = notice.sessionId.sessionNbr;
	*reasonCode = notice.reasonCode;
	*endOfBlock = notice.endOfBlock;
	*dataOffset = notice.dataOffset;
	*dataLength = notice.dataLength;
	return 0;
}

void	ltp_interrupt(unsigned int clientSvcId)
{
	LtpVdb		*vdb;
	LtpVclient	*client;

	if (clientSvcId <= MAX_LTP_CLIENT_NBR)
	{
		vdb = getLtpVdb();
		client = vdb->clients + clientSvcId;
		if (client->semaphore != SM_SEM_NONE)
		{
			sm_SemGive(client->semaphore);
		}
	}
}

void	ltp_release_data(Object data)
{
	Sdr	ltpSdr = getIonsdr();

	if (data)
	{
		CHKVOID(sdr_begin_xn(ltpSdr));
		zco_destroy(ltpSdr, data);
		if (sdr_end_xn(ltpSdr) < 0)
		{
			putErrmsg("Failed releasing LTP notice object.", NULL);
		}
	}
}

void	ltp_close(unsigned int clientSvcId)
{
	ltpDetachClient(clientSvcId);
}
