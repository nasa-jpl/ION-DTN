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

static void	shutDown(int signum)
{
	sm_SemEnd((getLtpVdb())->deliverySemaphore);
	ionPauseAttendant(_attendant(NULL));
}

#if defined (ION_LWT)
int	ltpdeliv(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
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
	Deliverable	deliv;
	LtpVclient	*client;
	LtpVspan	*vspan;
	PsmAddress	vspanElt;
	VImportSession	*vsession;
	Object		sessionObj;
	ImportSession	sessionBuf;
	vast		heapSpaceNeeded = 0;
	vast		fileSpaceNeeded = 0;
	Object		currentElt;
	unsigned int	clientSvcId;
	uvast		sourceEngineId;
	unsigned int	sessionNbr;
	ReqTicket	ticket;
	Object		svcDataObject;
	Object		extentObj;

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

		/*	Got a deliverable, still in transaction.	*/

		delivObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &deliv, delivObj, sizeof(Deliverable));
		client = vdb->clients + deliv.clientSvcId;
		findSpan(deliv.sourceEngineId, &vspan, &vspanElt);
		if (vspanElt == 0)	/*	Discard deliverable.	*/
		{
			sdr_free(sdr, delivObj);
			sdr_list_delete(sdr, elt, NULL, NULL);
			if (sdr_end_xn(sdr) < 0)
			{
				putErrmsg("LTP delivery failed.", NULL);
				break;
			}

			continue;
		}

		getImportSession(vspan, deliv.sessionNbr, &vsession,
				&sessionObj);
		if (sessionObj == 0)	/*	Discard deliverable.	*/
		{
			sdr_free(sdr, delivObj);
			sdr_list_delete(sdr, elt, NULL, NULL);
			if (sdr_end_xn(sdr) < 0)
			{
				putErrmsg("LTP delivery failed.", NULL);
				break;
			}

			continue;
		}

		sdr_read(sdr, (char *) &sessionBuf, sessionObj,
				sizeof(ImportSession));

		/*	Delivery ZCO will have up to two extents,
		 *	one for each of the session's two possible
		 *	data reception buffers (SDR heap object for
		 *	leading bytes, file for the remainder).		*/

		if (sessionBuf.heapBufferObj)
		{
			heapSpaceNeeded = sessionBuf.heapBufferBytes;
		}
		else
		{
			heapSpaceNeeded = 0;
		}

		if (sessionBuf.blockFileRef)
		{
			fileSpaceNeeded = sessionBuf.blockFileSize;
		}
		else
		{
			fileSpaceNeeded = 0;
		}

		sdr_exit_xn(sdr);

		/*	Remember this candidate deliverable.		*/

		currentElt = elt;
		clientSvcId = deliv.clientSvcId;
		sourceEngineId = deliv.sourceEngineId;
		sessionNbr = deliv.sessionNbr;

		/*	Reserve space for the delivery ZCO.		*/

		if (ionRequestZcoSpace(ZcoInbound, fileSpaceNeeded, 0,
				heapSpaceNeeded, 0, 0, &attendant, &ticket) < 0)
		{
			putErrmsg("Failed trying to reserve Zco space.", NULL);
			break;
		}

		if (!(ionSpaceAwarded(ticket)))
		{
			/*	Space not currently available.		*/

			if (sm_SemTake(attendant.semaphore) < 0)
			{
				putErrmsg("Failed taking semaphore.", NULL);
				ionShred(ticket);	/*	Cancel.	*/
				break;
			}

			if (sm_SemEnded(attendant.semaphore))
			{
				writeMemo("[i] ZCO request interrupted.");
				ionShred(ticket);	/*	Cancel.	*/
				break;
			}

			/*	ZCO space has now been reserved.	*/
		}

		/*	At this point ZCO space is known to be avbl.	*/

		CHKERR(sdr_begin_xn(sdr));
		elt = sdr_list_first(sdr, db->deliverables);
		if (elt != currentElt)
		{
			/*	Something happened while we were
			 *	waiting for space to become available.
			 *	Forget about this deliverable and
			 *	start over again.			*/

			sdr_exit_xn(sdr);
			ionShred(ticket);		/*	Cancel.	*/
			continue;
		}

		delivObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &deliv, delivObj, sizeof(Deliverable));
		if (deliv.clientSvcId != clientSvcId
		|| deliv.sourceEngineId != sourceEngineId
		|| deliv.sessionNbr != sessionNbr)
		{
			/*	Very unlikely, but it's possible that
			 *	the deliverable we're expecting got
			 *	removed and a different deliverable
			 *	got appended to the list in a list
			 *	element that is at the same address
			 *	as the one we got last time (deleted
			 *	and then recycled).  If the values
			 *	don't match, start over again.		*/

			sdr_exit_xn(sdr);
			ionShred(ticket);		/*	Cancel.	*/
			continue;
		}

		sdr_stage(sdr, (char *) &sessionBuf, sessionObj,
				sizeof(ImportSession));
		svcDataObject = zco_create(sdr, 0, 0, 0, 0, ZcoInbound);
		switch (svcDataObject)
		{
		case (Object) ERROR:
		case 0:
			sdr_cancel_xn(sdr);
			putErrmsg("Can't create service data object.", NULL);
			ionShred(ticket);	/*	Cancel.		*/
			sm_SemEnd(vdb->deliverySemaphore);
			continue;
		}

		/*	First deliver the heap buffer (if any).		*/

		if (sessionBuf.heapBufferObj)
		{
			/*	Copy heap bytes from public SDR heap
			 *	buffer to temporary local buffer and
			 *	from there back to a private SDR heap
			 *	buffer, which can then be appended
			 *	to the ZCO.				*/

			sdr_read(sdr, buffer, sessionBuf.heapBufferObj,
					sessionBuf.heapBufferBytes);
			extentObj = sdr_insert(sdr, buffer,
					sessionBuf.heapBufferBytes);

			/*	Pass additive inverse of buffer's
			 *	length to zco_append_extent to
			 *	indicate that space has already
			 *	been awarded.				*/

			switch (zco_append_extent(sdr, svcDataObject,
					ZcoSdrSource, extentObj, 0,
					0 - sessionBuf.heapBufferBytes))
			{
			case (Object) ERROR:
			case 0:
				sdr_cancel_xn(sdr);
				putErrmsg("Can't append ZCO extent.", NULL);
				ionShred(ticket);	/*	Cancel.	*/
				sm_SemEnd(vdb->deliverySemaphore);
				continue;

			default:
				break;	/*	Out of switch.		*/
			}
		}

		/*	Now deliver the file buffer (if any).		*/

		if (sessionBuf.blockFileRef)
		{
			switch (zco_append_extent(sdr, svcDataObject,
					ZcoFileSource, sessionBuf.blockFileRef,
					0, 0 - sessionBuf.blockFileSize))
			{
			case (Object) ERROR:
			case 0:
				sdr_cancel_xn(sdr);
				putErrmsg("Can't append ZCO extent.", NULL);
				ionShred(ticket);	/*	Cancel.	*/
				sm_SemEnd(vdb->deliverySemaphore);
				continue;

			default:
				break;	/*	Out of switch.		*/
			}

			zco_destroy_file_ref(sdr, sessionBuf.blockFileRef);
			sessionBuf.blockFileRef = 0;
		}

		ionShred(ticket);	/*	Dismiss reservation.	*/

		/*	Can now cease all reception for this session.	*/

		clearImportSession(&sessionBuf);
		if (sessionBuf.finalRptAcked)
		{
			stopImportSession(&sessionBuf);
			removeImportSession(sessionObj);
			closeImportSession(sessionObj);
			ltpSpanTally(vspan, IMPORT_COMPLETE, 0);
		}
		else
		{
			sessionBuf.delivered = 1;
			sdr_write(sdr, sessionObj, (char *) &sessionBuf,
					sizeof(ImportSession));
		}

		/*	Pass the new service data ZCO to the client
		 *	service.					*/

		if (enqueueNotice(client, sourceEngineId, sessionNbr, 0,
				sessionBuf.redPartLength, LtpRecvRedPart, 0,
				sessionBuf.endOfBlockRecd, svcDataObject) < 0)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("Can't post RecvRedPart notice.", NULL);
			sm_SemEnd(vdb->deliverySemaphore);
			continue;
		}

#if LTPDEBUG
putErrmsg("LTP delivered service data.", itoa(sessionBuf.redPartLength));
#endif
		/*	Discard the deliverable.			*/

		sdr_free(sdr, delivObj);
		sdr_list_delete(sdr, elt, NULL, NULL);
		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("LTP delivery failed.", NULL);
			break;
		}

		/*	Print watch character if necessary.		*/

		if (vdb->watching & WATCH_t)
		{
			iwatch('t');
		}

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();
	}

	shutDown(SIGTERM);
	ionStopAttendant(&attendant);
	writeErrmsgMemos();
	writeMemo("[i] ltpdeliv has ended.");
	ionDetach();
	return 0;
}
