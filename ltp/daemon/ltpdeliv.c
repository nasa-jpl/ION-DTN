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
	/* Tell the compiler that we are not using 'signum' */
	(void)signum;

	sm_SemEnd((getLtpVdb())->deliverySemaphore);
	ionPauseAttendant(_attendant(NULL));
}

#if defined (ION_LWT)
int	ltpdeliv(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
#else
int	main(void)
{
#endif
	Sdr			sdr;
	LtpDB			*db;
	LtpVdb			*vdb;
	ReqAttendant		attendant;
	char			*buffer;
	SdrObject		elt;
	SdrObject		delivObj;
	LtpDeliverable		deliv;
	LtpVclient		*client;
	LtpVspan		*vspan;
	PsmAddress		vspanElt;
	LtpVImportSession	*vsession;
	SdrObject		sessionObj;
	LtpImportSession	sessionBuf;
	vast			heapSpaceNeeded = 0;
	vast			fileSpaceNeeded = 0;
	SdrObject		currentElt;
	unsigned int		clientSvcId;
	uvast			sourceEngineId;
	unsigned int		sessionNbr;
	ReqTicket		ticket;
	SdrObject		svcDataObject;
	SdrObject		extentObj;

	if (ltpInit(0) < 0)
	{
		putErrmsg("ltpdeliv can't initialize LTP.", NULL);
		return 1;
	}

	sdr = getIonsdr();
	db = getLtpConstants();
	vdb = getLtpVdb();
	if (ionStartAttendant(&attendant) < 0
	|| (buffer = MTAKE(LTP_MAX_HEAP_LIMIT)) == 0)
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
		sdr_read(sdr, (char *) &deliv, delivObj,
				sizeof(LtpDeliverable));
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
				sizeof(LtpImportSession));

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
		sdr_read(sdr, (char *) &deliv, delivObj,
				sizeof(LtpDeliverable));
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

		/*	The deliverable still matches, but the import
		 *	session it refers to may have been canceled and
		 *	torn down by another LTP task while we were waiting
		 *	for ZCO space: the SDR lock was released over that
		 *	interval, so sessionObj sampled in the first
		 *	transaction can now be a freed session record.  A
		 *	freed record's segment lists have already been
		 *	destroyed by clearImportSession(), so reusing the
		 *	stale handle here and walking those lists below reads
		 *	freed SDR space and trips the SDR boundaries/integrity
		 *	guard.  Re-resolve the session under the reacquired
		 *	lock and discard the deliverable if it is gone.		*/

		findSpan(sourceEngineId, &vspan, &vspanElt);
		if (vspanElt != 0)
		{
			getImportSession(vspan, sessionNbr, &vsession,
					&sessionObj);
		}

		if (vspanElt == 0 || sessionObj == 0)
		{
			sdr_free(sdr, delivObj);
			sdr_list_delete(sdr, elt, NULL, NULL);
			ionShred(ticket);		/*	Cancel.	*/
			if (sdr_end_xn(sdr) < 0)
			{
				putErrmsg("LTP delivery failed.", NULL);
				break;
			}

			continue;
		}

		sdr_stage(sdr, (char *) &sessionBuf, sessionObj,
				sizeof(LtpImportSession));
		svcDataObject = zco_create(sdr, 0, 0, 0, 0, ZcoInbound);
		switch (svcDataObject)
		{
		case (SdrObject) ERROR:
		case 0:
			sdr_cancel_xn(sdr);
			putErrmsg("Can't create service data object.", NULL);
			ionShred(ticket);	/*	Cancel.		*/
			sm_SemEnd(vdb->deliverySemaphore);
			continue;
		}

		/*	First deliver the heap buffer (if any).
		 *
		 *	The session always has a heapBufferObj after
		 *	openImportSession() allocates it, but heapBufferBytes
		 *	is only updated when a red-part segment lands in the
		 *	heap-buffer range (libltpP.c bytesForHeap > 0).  If
		 *	no such segment was non-redundantly inserted before
		 *	this block was queued for delivery, heapBufferBytes
		 *	stays 0 and sdr_insert(_, _, 0) trips the
		 *	XNCHKZERO in _sdrmalloc, leaving the SDR transaction
		 *	half-modified and unrecoverable.  Guard on the
		 *	actual byte count, not just the buffer's presence.	*/

		if (sessionBuf.heapBufferObj && sessionBuf.heapBufferBytes == 0)
		{
			/*	Diagnostic for #1013: the previously buggy
			 *	state.  Log session number plus the heap
			 *	buffer's address so we can correlate against
			 *	libltpP.c's session-create / segment-insert
			 *	paths and figure out the actual precondition
			 *	that leaves heapBufferBytes at 0.  Remove
			 *	once the trigger is understood.			*/

			char	diagBuf[160];

			isprintf(diagBuf, sizeof diagBuf,
				"session=%u heapBufferObj="
				UVAST_FIELDSPEC " heapBufferBytes=0 "
				"blockFileSize=" UVAST_FIELDSPEC,
				sessionBuf.sessionNbr,
				(uvast) sessionBuf.heapBufferObj,
				sessionBuf.blockFileSize);
			writeMemoNote("[?] ltpdeliv: heap buffer allocated "
				"but empty; skipping heap extent "
				"(#1013 diagnostic)", diagBuf);
		}

		if (sessionBuf.heapBufferObj && sessionBuf.heapBufferBytes > 0)
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
			case (SdrObject) ERROR:
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
			case (SdrObject) ERROR:
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
					sizeof(LtpImportSession));
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
	MRELEASE(buffer);
	writeErrmsgMemos();
	writeMemo("[i] ltpdeliv has ended.");
	ionDetach();
	return 0;
}
