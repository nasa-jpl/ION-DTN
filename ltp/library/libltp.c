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
	ionDetach();
}

int	ltp_engine_is_started()
{
	LtpVdb	*vdb = getLtpVdb();

	return (vdb && vdb->clockPid != ERROR);
}

int	ltp_send(unsigned long destinationEngineId, unsigned long clientSvcId,
		Object clientServiceData, unsigned int redPartLength,
		LtpSessionId *sessionId)
{
	LtpVdb		*vdb = getLtpVdb();
	Sdr		sdr = getIonsdr();
	Object		dbobj = getLtpDbObject();
	LtpVspan	*vspan;
	PsmAddress	vspanElt;
	unsigned int	dataLength;
	unsigned int	occupancy;
	Object		spanObj;
	LtpSpan		span;
	LtpDB		db;
			OBJ_POINTER(ExportSession, session);

	CHKERR(clientSvcId <= MAX_LTP_CLIENT_NBR);
	CHKERR(clientServiceData);
	sdr_begin_xn(sdr);
	findSpan(destinationEngineId, &vspan, &vspanElt);
	if (vspanElt == 0)
	{
		sdr_exit_xn(sdr);
		putErrmsg("Destination engine unknown.",
				utoa(destinationEngineId));
		return -1;
	}

	dataLength = zco_length(sdr, clientServiceData);
	occupancy = zco_occupancy(sdr, clientServiceData);

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

			if (span.lengthOfBufferedBlock == 0)
			{
				/*	Brand-new session; any SDU
				 *	of any size can be inserted
				 *	into the block at this time.	*/

				break;		/*	Out of loop.	*/
			}

			/*	Block already contains some data.	*/

			if (span.lengthOfBufferedBlock
				== span.redLengthOfBufferedBlock
			/*	No green data in block.			*/
			&& span.lengthOfBufferedBlock < span.aggrSizeLimit
			/*	Not yet aggregated up to nominal limit.	*/
			&& clientSvcId == span.clientSvcIdOfBufferedBlock
			/*	This SDU is destined for the same
			 *	engine as all other data in the block.	*/)
			{
				/*	Okay to insert this service
				 *	data unit into the block.	*/

				break;		/*	Out of loop.	*/
			}
		}

		/*	Can't insert service data unit into block.  Wait
		 *	until span has been initialized for new session
		 *	with new session buffer -- either by ltpmeter's
		 *	initialization or by ltpmeter segmenting the
		 *	current block and writing the segments out to
		 *	the link service layer.				*/

		sdr_exit_xn(sdr);
		if (sm_SemTake(vspan->bufEmptySemaphore) < 0)
		{
			putErrmsg("Can't take buffer-empty semaphore.",
					itoa(vspan->engineId));
			return -1;
		}

		if (sm_SemEnded(vspan->bufEmptySemaphore))
		{
			putErrmsg("Span has been stopped.",
					itoa(vspan->engineId));
			return 0;
		}

		sdr_begin_xn(sdr);
		sdr_stage(sdr, (char *) &span, spanObj, sizeof(LtpSpan));
	}

	/*	Now append the outbound SDU to the block that is
	 *	currently being aggregated for this span and, if the
	 *	block buffer is now full or the block buffer contains
	 *	any green data, notify ltpmeter that block segmentation
	 *	can begin.						*/

	sdr_stage(sdr, (char *) &db, dbobj, sizeof(LtpDB));
	if (db.heapSpaceBytesOccupied + occupancy > db.heapSpaceBytesReserved)
	{
		sdr_exit_xn(sdr);
		putErrmsg("ltp_send failed, would exceed LTP heap space \
reservation; restart LTP client.", utoa(occupancy));
		return -1;
	}

	db.heapSpaceBytesOccupied += occupancy;
	sdr_write(sdr, dbobj, (char *) &db, sizeof(LtpDB));
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
		sm_SemGive(vspan->bufFullSemaphore);
	}

	if (vdb->watching & WATCH_d)
	{
		putchar('d');
		fflush(stdout);
	}

	if (sdr_end_xn(sdr))
	{
		putErrmsg("Can't send data.", NULL);
		return -1;
	}

	sessionId->sourceEngineId = db.ownEngineId;
	sessionId->sessionNbr = session->sessionNbr;
	return 1;
}

int	ltp_open(unsigned long clientSvcId)
{
	return ltpAttachClient(clientSvcId);
}

int	ltp_get_notice(unsigned long clientSvcId, LtpNoticeType *type,
		LtpSessionId *sessionId, unsigned char *reasonCode,
		unsigned char *endOfBlock, unsigned long *dataOffset,
		unsigned long *dataLength, Object *data)
{
	Sdr		sdr = getIonsdr();
	LtpVdb		*vdb = getLtpVdb();
	LtpVclient	*client;
	Object		elt;
	Object		noticeAddr;
	LtpNotice	notice;
	LtpDB		db;
	Object		dbobj;

	CHKERR(clientSvcId <= MAX_LTP_CLIENT_NBR);
	CHKERR(type);
	CHKERR(sessionId);
	CHKERR(reasonCode);
	CHKERR(endOfBlock);
	CHKERR(dataOffset);
	CHKERR(dataLength);
	CHKERR(data);
	*type = LtpNoNotice;	/*	Default.			*/
	sdr_begin_xn(sdr);
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

		sdr_begin_xn(sdr);
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

	if (notice.data)
	{
		dbobj = getLtpDbObject();
		sdr_stage(sdr, (char *) &db, dbobj, sizeof(LtpDB));
		db.heapSpaceBytesOccupied -= zco_occupancy(sdr, notice.data);
		sdr_write(sdr, dbobj, (char *) &db, sizeof(LtpDB));
	}

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

void	ltp_interrupt(unsigned long clientSvcId)
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
		sdr_begin_xn(ltpSdr);
		zco_destroy_reference(ltpSdr, data);
		if (sdr_end_xn(ltpSdr) < 0)
		{
			putErrmsg("Failed releasing LTP notice object.", NULL);
		}
	}
}

void	ltp_close(unsigned long clientSvcId)
{
	ltpDetachClient(clientSvcId);
}
