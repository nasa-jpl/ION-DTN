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

static char	*NullParmsMemo = "LTP app error: null input parameter(s).";

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

	return (vdb && vdb->clockPid > 0) ? 1 : 0;
}

int	ltp_send(unsigned long destinationEngineId, unsigned long clientSvcId,
		Object clientServiceData, unsigned int redPartLength,
		LtpSessionId *sessionId)
{
	LtpVdb		*vdb = getLtpVdb();
	Sdr		sdr = getIonsdr();
	LtpDB		*ltpConstants = getLtpConstants();
	LtpVspan	*vspan;
	PsmAddress	vspanElt;
	unsigned int	dataLength;
	unsigned int	greenPartLength;
	Object		spanObj;
	LtpSpan		span;
			OBJ_POINTER(ExportSession, session);

	if (clientSvcId > MAX_LTP_CLIENT_NBR || clientServiceData == 0)
	{
		errno = EINVAL;
		putSysErrmsg(NullParmsMemo, NULL);
		return -1;
	}

	sdr_begin_xn(sdr);
	findSpan(destinationEngineId, &vspan, &vspanElt);
	if (vspanElt == 0)
	{
		sdr_exit_xn(sdr);
		errno = EINVAL;
		putSysErrmsg("Destination engine unknown",
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

	greenPartLength = dataLength - redPartLength;

	/*	Red data size is limited by span export max block size.
	 *	Green data size is limited by span max segment size.	*/

	spanObj = sdr_list_data(sdr, vspan->spanElt);
	sdr_stage(sdr, (char *) &span, spanObj, sizeof(LtpSpan));
	if (redPartLength > span.maxExportBlockSize)
	{
		sdr_exit_xn(sdr);
		errno = EINVAL;
		putSysErrmsg("Client service data size exceeds max block size",
			utoa(redPartLength - span.maxExportBlockSize));
		return 0;
	}

	if (greenPartLength > span.maxSegmentSize)
	{
		sdr_exit_xn(sdr);
		errno = EINVAL;
		putSysErrmsg("Client service data size exceeds max frame size",
			utoa(greenPartLength - span.maxSegmentSize));
		return 0;
	}

	/*	Wait until there's enough room in the span's session
	 *	buffer (block) for this service data unit.  Also, if
	 *	current block has different client service ID or if
	 *	it contains any green data, wait until it has been
	 *	written out to the link service layer.			*/

	while (1)
	{
		if (span.currentExportSessionObj)
		{
			/*	Span has been initialized with a
			 *	session buffer (block) into which
			 *	bundles can be inserted.		*/

			if (span.lengthOfBufferedBlock == 0)
			{
				/*	Brand-new session; any bundle
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
			&& (span.maxExportBlockSize
				- span.lengthOfBufferedBlock) > dataLength
			/*	Inserting this bundle into the block
			 *	would not cause block size to exceed
			 *	the span's export block size limit.	*/
			&& clientSvcId == span.clientSvcIdOfBufferedBlock
			/*	This bundle is destined for the same
			 *	endpoint as all other data in the block.*/)
			{
				/*	Okay to insert this bundle
				 *	into the block.			*/

				break;		/*	Out of loop.	*/
			}
		}

		/*	Can't insert bundle into block.  Wait until
		 *	span has been initialized for new session
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

	/*	Now append the outbound bundle to the block that is
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
		sm_SemGive(vspan->bufFullSemaphore);
	}

	if (sdr_end_xn(sdr))
	{
		putErrmsg("Can't send data.", NULL);
		return -1;
	}

	if (vdb->watching & WATCH_d)
	{
		putchar('d');
		fflush(stdout);
	}

	sessionId->sourceEngineId = ltpConstants->ownEngineId;
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
		unsigned long *dataLength, char **data)
{
	Sdr		sdr = getIonsdr();
	LtpVdb		*vdb = getLtpVdb();
	LtpVclient	*client;
	Object		elt;
	Object		noticeAddr;
	LtpNotice	notice;
	char		*cursor;
	Object		segmentObj;
			OBJ_POINTER(LtpRecvSeg, segment);
	Object		zcoRef;
	int		objectLength;
	ZcoReader	reader;

	if (clientSvcId > MAX_LTP_CLIENT_NBR || type == NULL
	|| sessionId == NULL || reasonCode == NULL || endOfBlock == NULL
	|| dataOffset == NULL || dataLength == NULL || data == NULL)
	{
		putErrmsg(NullParmsMemo, NULL);
		errno = EINVAL;
		return -1;
	}

	*type = LtpNoNotice;	/*	Default.			*/
	sdr_begin_xn(sdr);
	client = vdb->clients + clientSvcId;
	if (client->pid != sm_TaskIdSelf())
	{
		sdr_exit_xn(sdr);
		putErrmsg("Can't get notice: not owner of client service.",
				itoa(client->pid));
		errno = EINVAL;
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
			putErrmsg("Client access terminated.", NULL);
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
	if (notice.data == 0)
	{
		*data = NULL;

		/*	Note that an ExportSessionCanceled notice
		 *	may have associated data of zero, in the
		 *	event that local cancellation by the sender
		 *	and remote cancellation by the receiver
		 *	happen to occur at almost exactly the same
		 *	time: the first cancellation causes the
		 *	session's list of service data units to be
		 *	set to zero, but both cancellations cause
		 *	notices to be sent to the user.			*/
	}
	else
	{
		*data = MTAKE(notice.dataLength);
		if (*data == NULL)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("Can't deliver segment data.", NULL);
			return -1;
		}

		switch (notice.type)
		{
		case LtpRecvGreenSegment:

			/*	notice.data is a single heap object.	*/

			sdr_read(sdr, *data, notice.data, notice.dataLength);
			sdr_free(sdr, notice.data);
			break;

		case LtpRecvRedPart:	/*	Entire red part.	*/

			/*	notice.data is a linked list of heap
			 *	objects, one for each red-part segment.	*/

			cursor = *data;
			while ((elt = sdr_list_first(sdr, notice.data)) != 0)
			{
				segmentObj = sdr_list_data(sdr, elt);
				GET_OBJ_POINTER(sdr, LtpRecvSeg, segment,
						segmentObj);
				sdr_read(sdr, cursor,
						segment->pdu.clientSvcData,
						segment->pdu.length);
				sdr_free(sdr, segment->pdu.clientSvcData);
				cursor += segment->pdu.length;
				sdr_free(sdr, segmentObj);
				sdr_list_delete(sdr, elt, NULL, NULL);
			}

			sdr_list_destroy(sdr, notice.data, NULL, NULL);
			break;

		case LtpExportSessionCanceled:

			/*	notice.data is a linked list of ZCO
			 *	references, one for each service data
			 *	object in the aggregated block.		*/

			cursor = *data;
			while ((elt = sdr_list_first(sdr, notice.data)) != 0)
			{
				zcoRef = sdr_list_data(sdr, elt);
				objectLength = zco_length(sdr, zcoRef);

				/*	Omit from the delivered notice
				 *	all SDOs that consist solely
				 *	of original source data, since
				 *	the layer of protocol above
				 *	LTP won't be able to deal with
				 *	them.				*/

				if (objectLength == zco_source_data_length(sdr,
						zcoRef))
				{
					/*	No header.		*/

					notice.dataLength -= objectLength;
				}
				else
				{
					zco_start_transmitting(sdr, zcoRef,
							&reader);
					zco_transmit(sdr, &reader, objectLength,
							cursor);
					zco_stop_transmitting(sdr, &reader);
					cursor += objectLength;
				}

				/*	Note that the service data
				 *	objects list has been removed
				 *	from the session object, so
				 *	closeExportSession can't clear
				 *	it out.  We must do that here.	*/

				zco_destroy_reference(sdr, zcoRef);
				sdr_list_delete(sdr, elt, NULL, NULL);
			}

			sdr_list_destroy(sdr, notice.data, NULL, NULL);
			break;

		default:
			MRELEASE(*data);
			putErrmsg("Notice has invalid non-zero dataLength.",
					itoa(notice.type));
		}
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

void	ltp_release_data(char *data)
{
	MRELEASE(data);
}

void	ltp_close(unsigned long clientSvcId)
{
	ltpDetachClient(clientSvcId);
}
