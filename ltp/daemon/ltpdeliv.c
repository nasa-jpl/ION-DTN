/*

	ltpdeliv.c:	LTP delivery daemon; manages ZCO space.

	Author: Scott Burleigh, JPL

	Copyright (c) 2017, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
	
									*/
#include "ltpP.h"
#include "ltpei.h"

static ReqAttendant	*_attendant(ReqAttendant *newAttendant)
{
	static ReqAttendant	*attendant = NULL;

	if (newAttendant)
	{
		attendant = newAttendant;
	}

	return attendant;
}

static void	shutDown()
{
	sm_SemEnd((getLtpVdb())->deliverySemaphore);
	ionPauseAttendant(_attendant(NULL));
}

static int	deliverSvcData(int clientSvcId, uvast sourceEngineId,
			unsigned int sessionNbr, ReqAttendant *attendant,
			char *buffer)
{
	Sdr		sdr = getIonsdr();
	LtpVdb		*vdb = getLtpVdb();
	LtpVclient	*client;
	LtpVspan	*vspan;
	PsmAddress	vspanElt;
	VImportSession	*vsession;
	Object		sessionObj;
	ImportSession	sessionBuf;
	vast		heapSpaceNeeded = 0;
	vast		fileSpaceNeeded = 0;
	ReqTicket	ticket;
	Object		svcDataObject;
	Object		extentObj;
	Object		elt;
	Object		segObj;
			OBJ_POINTER(LtpRecvSeg, segment);

	/*	Already in a transaction at this point.			*/

	client = vdb->clients + clientSvcId;
	findSpan(sourceEngineId, &vspan, &vspanElt);
	if (vspanElt == 0)
	{
		return 1;	/*	Just discard the deliverable.	*/
	}

	getImportSession(vspan, sessionNbr, &vsession, &sessionObj);
	if (sessionObj == 0)
	{
		return 1;	/*	Just discard the deliverable.	*/
	}

	sdr_stage(sdr, (char *) &sessionBuf, sessionObj, sizeof(ImportSession));

	/*	Construct a ZCO with up to two extents, one for
	 *	each of the session's two possible data reception
	 *	buffers (SDR heap object for leading bytes, file
	 *	for the remainder).  Wait for this space to become
	 *	available.						*/

	if (sessionBuf.heapBufferObj)
	{
		heapSpaceNeeded = sessionBuf.heapBufferBytes;
	}

	if (sessionBuf.blockFileRef)
	{
		fileSpaceNeeded = sessionBuf.blockFileSize;
	}

	if (ionRequestZcoSpace(ZcoInbound, fileSpaceNeeded, 0, heapSpaceNeeded,
			0, 0, attendant, &ticket) < 0)
	{
		putErrmsg("Failed trying to reserve Zco space.", NULL);
		return -1;
	}

	if (!(ionSpaceAwarded(ticket)))
	{
		/*	Space not currently available.			*/

		sdr_exit_xn(sdr);

		/*	Ticket is req list element for the request.
		 *	Wait until space is available.			*/

		if (sm_SemTake(attendant->semaphore) < 0)
		{
			putErrmsg("Failed taking attendant semaphore.", NULL);
			ionShred(ticket);	/*	Cancel request.	*/
			return 0;
		}

		if (sm_SemEnded(attendant->semaphore) < 0)
		{
			writeMemo("[i] ZCO space reservation interrupted.");
			ionShred(ticket);	/*	Cancel request.	*/
			return 0;
		}

		/*	ZCO space has now been reserved, but span or
		 *	session might have disappeared while we were
		 *	waiting, so re-retrieve them.			*/

		CHKERR(sdr_begin_xn(sdr));
		client = vdb->clients + clientSvcId;
		findSpan(sourceEngineId, &vspan, &vspanElt);
		if (vspanElt == 0)
		{
			ionShred(ticket);	/*	Cancel request.	*/
			return 1;	/*	Discard deliverable.	*/
		}

		getImportSession(vspan, sessionNbr, &vsession, &sessionObj);
		if (sessionObj == 0)
		{
			ionShred(ticket);	/*	Cancel request.	*/
			return 1;	/*	Discard deliverable.	*/
		}

		sdr_stage(sdr, (char *) &sessionBuf, sessionObj,
				sizeof(ImportSession));
	}

	/*	At this point, ZCO space is known to be available
	 *	(and therefore SDR heap space should be available as
	 *	needed).						*/

	svcDataObject = zco_create(sdr, 0, 0, 0, 0, ZcoInbound);
	switch (svcDataObject)
	{
	case (Object) ERROR:
	case 0:				/*	Out of ZCO space.	*/
		putErrmsg("Can't create service data object.", NULL);
		ionShred(ticket);	/*	Cancel request.		*/
		return -1;
	}

	/*	First deliver the heap buffer (if any).			*/

	if (sessionBuf.heapBufferObj)
	{
		/*	Copy heap bytes from public SDR heap buffer to
		 *	temporary local buffer and from there back to
		 *	a private SDR heap buffer, which can then be
		 *	appended to the ZCO.				*/

		sdr_read(sdr, buffer, sessionBuf.heapBufferObj,
				sessionBuf.heapBufferBytes);
		extentObj = sdr_insert(sdr, buffer, sessionBuf.heapBufferBytes);
		CHKERR(extentObj);

		/*	Pass additive inverse of buffer's length to
		 *	zco_append_extent to indicate that space has
		 *	already been awarded.				*/

		switch (zco_append_extent(sdr, svcDataObject, ZcoSdrSource,
				extentObj, 0, 0 - sessionBuf.heapBufferBytes))
		{
		case (Object) ERROR:
		case 0:
			putErrmsg("Can't append ZCO extent.", NULL);
			ionShred(ticket);	/*	Cancel request.	*/
			return -1;

		default:
			break;		/*	Out of switch.		*/
		}
	}

	/*	Now deliver the file buffer (if any).			*/

	if (sessionBuf.blockFileRef)
	{
		switch (zco_append_extent(sdr, svcDataObject, ZcoFileSource,
				sessionBuf.blockFileRef, 0,
				0 - sessionBuf.blockFileSize))
		{
		case (Object) ERROR:
		case 0:
			putErrmsg("Can't append ZCO extent.", NULL);
			ionShred(ticket);	/*	Cancel request.	*/
			return -1;

		default:
			break;		/*	Out of switch.		*/
		}

		zco_destroy_file_ref(sdr, sessionBuf.blockFileRef);
		sessionBuf.blockFileRef = 0;
	}

	ionShred(ticket);	/*	Dismiss reservation.		*/

	/*	Can now discard all red segments.			*/

	while ((elt = sdr_list_first(sdr, sessionBuf.redSegments)))
	{
		segObj = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, LtpRecvSeg, segment, segObj);
		sdr_list_delete(sdr, elt, NULL, NULL);
		if (segment->pdu.headerExtensions)
		{
			sdr_list_destroy(sdr, segment->pdu.headerExtensions,
					ltpei_destroy_extension, NULL);
		}

		if (segment->pdu.trailerExtensions)
		{
			sdr_list_destroy(sdr, segment->pdu.trailerExtensions,
					ltpei_destroy_extension, NULL);
		}

		sdr_free(sdr, segObj);
	}

	sdr_list_destroy(sdr, sessionBuf.redSegments, NULL, NULL);
	sessionBuf.redSegments = 0;
	sdr_write(sdr, sessionObj, (char *) &sessionBuf, sizeof(ImportSession));

	/*	Pass the new service data ZCO to the client service.	*/

	if (enqueueNotice(client, sourceEngineId, sessionNbr, 0,
			sessionBuf.redPartLength, LtpRecvRedPart, 0,
			sessionBuf.endOfBlockRecd, svcDataObject) < 0)
	{
		putErrmsg("Can't post RecvRedPart notice.", NULL);
		return -1;
	}

	/*	Print watch character if necessary, and return.		*/

	if (vdb->watching & WATCH_t)
	{
		iwatch('t');
	}

#if LTPDEBUG
putErrmsg("LTP delivered service data.", itoa(sessionBuf.redPartLength));
#endif
	return 1;
}

#if defined (ION_LWT)
int	ltpdeliv(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
#else
int	main(int argc, char *argv[])
{
#endif
	Sdr		sdr;
	LtpDB		*db;
	LtpVdb		*vdb;
	ReqAttendant	attendant;
	char		*buffer;
	Object		elt;
	Object		delivObj;
			OBJ_POINTER(Deliverable, deliv);
	int		result;

	if (ltpInit(0) < 0)
	{
		putErrmsg("ltpdeliv can't initialize LTP.", NULL);
		return 1;
	}

	sdr = getIonsdr();
	db = getLtpConstants();
	vdb = getLtpVdb();
	if (ionStartAttendant(&attendant) < 0
	|| (buffer = MTAKE(db->maxAcqInHeap)) == 0)
	{
		putErrmsg("Can't initialize blocking LTP acquisition.", NULL);
		return 1;
	}

	oK(_attendant(&attendant));
	isignal(SIGTERM, shutDown);

	/*	Main loop: wait until deliverables queue is non-
	 *	empty, then drain it.					*/

	writeMemo("[i] ltpdeliv is running.");
	while (sm_SemEnded(vdb->deliverySemaphore) == 0)
	{
		CHKZERO(sdr_begin_xn(sdr));
		elt = sdr_list_first(sdr, db->deliverables);
		if (elt == 0)	/*	Wait for next deliverable.	*/
		{
			sdr_exit_xn(sdr);
			if (sm_SemTake(vdb->deliverySemaphore) < 0)
			{
				putErrmsg("Can't take delivery semaphore.",
						NULL);
				break;
			}

			continue;
		}

		/*	Deliverable is ready.				*/

		delivObj = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, Deliverable, deliv, delivObj);
		result = deliverSvcData(deliv->clientSvcId,
				deliv->sourceEngineId, deliv->sessionNbr,
				&attendant, buffer);
		if (result < 0)
		{
			sdr_cancel_xn(sdr);
			break;
		}

		if (result == 0)
		{
			/*	ZCO space reservation failed, no
			 *	longer in transaction.			*/

			break;
		}

		/*	Retrieve and delete the deliverable object.	*/

		elt = sdr_list_first(sdr, db->deliverables);
		if (elt)
		{
			delivObj = sdr_list_data(sdr, elt);
			sdr_free(sdr, delivObj);
			sdr_list_delete(sdr, elt, NULL, NULL);
		}

		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("LTP delivery failed.", NULL);
			break;
		}
	}

	shutDown();
	ionStopAttendant(&attendant);
	writeErrmsgMemos();
	writeMemo("[i] ltpdeliv has ended.");
	ionDetach();
	return 0;
}
