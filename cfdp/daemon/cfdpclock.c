/*
	cfdpclock.c:	scheduled-event management daemon for CFDP.

	Author: Scott Burleigh, JPL

	Copyright (c) 2007, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include "cfdpP.h"

static uaddr	_running(uaddr *newValue)
{
	void	*value;
	uaddr	state;
	
	if (newValue)			/*	Changing state.		*/
	{
		value = (void *) (*newValue);
		state = (uaddr) sm_TaskVar(&value);
	}
	else				/*	Just check.		*/
	{
		state = (uaddr) sm_TaskVar(NULL);
	}

	return state;
}

static void	shutDown(int signum)	/*	Stops cfdpclock.	*/
{
	uaddr	stop = 0;

	oK(_running(&stop));	/*	Terminates cfdpclock.		*/
}

static int	scanInFdus(Sdr sdr, time_t currentTime)
{
	CfdpDB		*cfdpConstants;
	Object		entityElt;
			OBJ_POINTER(Entity, entity);
	Object		elt;
	Object		nextElt;
	Object		fduObj;
			OBJ_POINTER(InFdu, fdu);
	CfdpHandler	handler;

	cfdpConstants = getCfdpConstants();
	CHKERR(sdr_begin_xn(sdr));
	for (entityElt = sdr_list_first(sdr, cfdpConstants->entities);
			entityElt; entityElt = sdr_list_next(sdr, entityElt))
	{
		GET_OBJ_POINTER(sdr, Entity, entity, sdr_list_data(sdr,
				entityElt));
		for (elt = sdr_list_first(sdr, entity->inboundFdus); elt;
				elt = nextElt)
		{
			nextElt = sdr_list_next(sdr, elt);
			fduObj = sdr_list_data(sdr, elt);
			GET_OBJ_POINTER(sdr, InFdu, fdu, fduObj);
			if (fdu->inactivityDeadline > 0
			&& fdu->inactivityDeadline < currentTime)
			{
				/*	Inactivity limit reached.
				 *	Disable the timer so it won't
				 *	be triggered again in the event
				 *	that the fault handler doesn't
				 *	destroy the InFdu.		*/

				sdr_stage(sdr, NULL, fduObj, 0);
				fdu->inactivityDeadline = 0;
				sdr_write(sdr, fduObj, (char *) fdu,
						sizeof(InFdu));
				if (handleFault(&(fdu->transactionId),
					CfdpInactivityDetected, &handler) < 0)
				{
					sdr_cancel_xn(sdr);
					putErrmsg("Can't handle inactivity.",
							NULL);
					return -1;
				}

				switch (handler)
				{
				case CfdpCancel:
				case CfdpAbandon:
					continue;	/*	Done.	*/

				default:
					break;	/*	Proceed.	*/
				}
			}

			if (fdu->eofReceived && fdu->checkTime < currentTime)
			{
				sdr_stage(sdr, NULL, fduObj, 0);
				fdu->checkTimeouts++;
				fdu->checkTime
					+= cfdpConstants->checkTimerPeriod;
				sdr_write(sdr, fduObj, (char *) fdu,
						sizeof(InFdu));
			}

			if (fdu->checkTimeouts
				> cfdpConstants->checkTimeoutLimit)
			{
				if (handleFault(&(fdu->transactionId),
					CfdpCheckLimitReached, &handler) < 0)
				{
					sdr_cancel_xn(sdr);
					putErrmsg("Can't handle check limit \
reached.", NULL);
					return -1;
				}
			}
		}
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("cfdpclock failed scanning inbound FDUs.", NULL);
		return -1;
	}

	return 0;
}

static int	handleFinishOverdue(Sdr sdr, Object fduObj)
{
	OutFdu		fdu;
	Object		fpObj;
	CfdpEvent	event;

	sdr_stage(sdr, (char *) &fdu, fduObj, sizeof(OutFdu));
	if (fdu.finishReceived)
	{
		return 0;
	}

	fpObj = sdr_list_data(sdr, fdu.closureElt);
	sdr_free(sdr, fpObj);
	sdr_list_delete(sdr, fdu.closureElt, NULL, NULL);
	fdu.closureElt = 0;
	fdu.finishReceived = 1;
	memset((char *) &event, 0, sizeof(CfdpEvent));
	memcpy((char *) &event.transactionId, (char *) &fdu.transactionId,
			sizeof(CfdpTransactionId));
	event.reqNbr = fdu.reqNbr;
	event.type = CfdpTransactionFinishedInd;
	event.condition = CfdpCheckLimitReached;
	event.deliveryCode = CfdpDataIncomplete;
	event.fileStatus = CfdpFileStatusUnreported;
	if (enqueueCfdpEvent(&event) < 0)
	{
		putErrmsg("CFDP can't report on Finish overdue.", NULL);
		sdr_cancel_xn(sdr);
		return -1;
	}

	sdr_write(sdr, fduObj, (char *) &fdu, sizeof(OutFdu));
	return 0;
}

static int	scanFinsPending(Sdr sdr, time_t currentTime)
{
	CfdpDB	*cfdpConstants;
	Object	elt;
	Object	nextElt;
	Object	fpObj;
		OBJ_POINTER(FinishPending, fp);
	Object	fduObj;

	cfdpConstants = getCfdpConstants();
	CHKERR(sdr_begin_xn(sdr));	/*	Lock database.		*/
	for (elt = sdr_list_first(sdr, cfdpConstants->finsPending); elt;
			elt = nextElt)
	{
		nextElt = sdr_list_next(sdr, elt);
		fpObj = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, FinishPending, fp, fpObj);
		if (fp->deadline > currentTime)
		{
			break;
		}

		fduObj = fp->fdu;
		if (handleFinishOverdue(sdr, fduObj) < 0)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("Can't handle overdue closure.", NULL);
			return -1;
		}
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("cfdpclock failed scanning closures.", NULL);
		return -1;
	}

	return 0;
}

static int	noteFinishPending(Sdr sdr, OutFdu *fdu, Object fduObj,
			time_t currentTime)
{
	CfdpDB		*db = getCfdpConstants();
	FinishPending	newFP;
	Object		elt;
			OBJ_POINTER(FinishPending, fp);
	Object		obj;

	newFP.deadline = currentTime + fdu->closureLatency;
	newFP.fdu = fduObj;
	for (elt = sdr_list_last(sdr, db->finsPending); elt;
			elt = sdr_list_prev(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, FinishPending, fp,
				sdr_list_data(sdr, elt));
		if (fp->deadline <= newFP.deadline)
		{
			break;
		}
	}

	obj = sdr_malloc(sdr, sizeof(FinishPending));
	if (obj == 0)
	{
		putErrmsg("Can't write pending finish.", NULL);
		return -1;
	}

	sdr_write(sdr, obj, (char *) &newFP, sizeof(FinishPending));
	if (elt)
	{
		fdu->closureElt = sdr_list_insert_after(sdr, elt, obj);
	}
	else
	{
		fdu->closureElt = sdr_list_insert_first(sdr, db->finsPending,
				obj);
	}

	if (fdu->closureElt == 0)
	{
		putErrmsg("Can't schedule pending finish.", NULL);
		return -1;
	}

	return 0;
}

static int	enqueueIndications(Sdr sdr, OutFdu *fdu)
{
	CfdpEvent	event;

	/*	Post EOF-sent.ind event.				*/

	memset((char *) &event, 0, sizeof(CfdpEvent));
	event.type = CfdpEofSentInd;
	event.fileSize = fdu->fileSize;
	memcpy((char *) &event.transactionId, (char *) &fdu->transactionId,
			sizeof(CfdpTransactionId));
	event.reqNbr = fdu->reqNbr;
	if (enqueueCfdpEvent(&event) < 0)
	{
		putErrmsg("CFDP can't report on EOF sent.", NULL);
		sdr_cancel_xn(sdr);
		return -1;
	}

	if (fdu->closureLatency > 0)
	{
		/*	Transaction-Finished.ind event will be posted
		 *	when Finished PDU arrives.			*/

		return 0;
	}

	/*	Post Transaction-Finished.ind event.  (Unacknowledged)	*/

	memset((char *) &event, 0, sizeof(CfdpEvent));
	event.type = CfdpTransactionFinishedInd;
	event.condition = CfdpNoError;
	event.deliveryCode = CfdpDataIncomplete;
	event.fileStatus = CfdpFileStatusUnreported;
	memcpy((char *) &event.transactionId, (char *) &fdu->transactionId,
			sizeof(CfdpTransactionId));
	event.reqNbr = fdu->reqNbr;
	if (enqueueCfdpEvent(&event) < 0)
	{
		putErrmsg("CFDP can't report on EOF sent.", NULL);
		sdr_cancel_xn(sdr);
		return -1;
	}

	return 0;
}

static int	scanOutFdus(Sdr sdr, time_t currentTime)
{
	CfdpDB	*cfdpConstants;
	Object	elt;
	Object	nextElt;
	Object	fduObj;
	OutFdu	fdu;

	cfdpConstants = getCfdpConstants();
	CHKERR(sdr_begin_xn(sdr));
	for (elt = sdr_list_first(sdr, cfdpConstants->outboundFdus); elt;
			elt = nextElt)
	{
		nextElt = sdr_list_next(sdr, elt);
		fduObj = sdr_list_data(sdr, elt);
		sdr_stage(sdr, (char *) &fdu, fduObj, sizeof(OutFdu));
		if (fdu.state == FduCanceled)
		{
			/*	Simulate completion of transmission.	*/

			fdu.progress = fdu.fileSize;
			sdr_write(sdr, fduObj, (char *) &fdu, sizeof(OutFdu));
		}

		/*	If not all of this FDU's file data PDUs have
		 *	been issued yet, move on to the next FDU.	*/

		if (fdu.progress < fdu.fileSize)
		{
			continue;
		}

		/*	If all of those file data PDUs have actually
		 *	been transmitted, then we infer that the EOF
		 *	PDU has also been transmitted.  So if we
		 *	haven't already delivered the EOF-Sent
		 *	indication and Transaction-Finished indication
		 *	(Unacknowledged procedures) for this FDU, we
		 *	do it now.					*/

		if (fdu.fileRef && !zco_file_ref_xmit_eof(sdr, fdu.fileRef))
		{
			continue;
		}

		if (fdu.eofPdu == 0)
		{
			if (fdu.transmitted == 0 && fdu.state != FduCanceled)
			{
				fdu.transmitted = 1;
				if (fdu.closureLatency > 0)
				{
					if (noteFinishPending(sdr, &fdu, fduObj,
							currentTime) < 0)
					{
						sdr_cancel_xn(sdr);
						putErrmsg("Can't start timer.",
								NULL);
						return -1;
					}
				}

				sdr_write(sdr, fduObj, (char *) &fdu,
						sizeof(OutFdu));
				if (enqueueIndications(sdr, &fdu) < 0)
				{
					sdr_cancel_xn(sdr);
					putErrmsg("Can't note EOF-sent.", NULL);
					return -1;
				}
			}
		}

		/*	If all of the PDUs have been either delivered
		 *	or destroyed (e.g., due to FDU cancellation or
		 *	to expiration of bundle TTL), destroy the FDU.	*/

		if (fdu.transmitted == 1
		&& (fdu.closureLatency == 0 || fdu.finishReceived))
		{
			destroyOutFdu(&fdu, fduObj, elt);
		}
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("cfdpclock failed scanning outbound FDUs.", NULL);
		return -1;
	}

	return 0;
}

#if defined (ION_LWT)
int	cfdpclock(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
#else
int	main(int argc, char *argv[])
{
#endif
	Sdr	sdr;
	uaddr	state = 1;
	time_t	currentTime;

	if (cfdpInit() < 0)
	{
		putErrmsg("cfdpclock can't initialize CFDP.", NULL);
		return -1;
	}

	sdr = getIonsdr();
	isignal(SIGTERM, shutDown);

	/*	Main loop: wait one second, then scan all FDUS.		*/

	oK(_running(&state));
	writeMemo("[i] cfdpclock is running.");
	while (_running(NULL))
	{
		snooze(1);
		currentTime = getCtime();

		/*	Update check counts for inbound FDUs.		*/

		if (scanInFdus(sdr, currentTime) < 0)
		{
			putErrmsg("Can't scan inbound FDUs.", NULL);
			state = 0;	/*	Terminate loop.		*/
			oK(_running(&state));
			continue;
		}

		/*	Check for overdue transaction closures.		*/

		if (scanFinsPending(sdr, currentTime) < 0)
		{
			putErrmsg("Can't scan pending closures.", NULL);
			state = 0;	/*	Terminate loop.		*/
			oK(_running(&state));
			continue;
		}

		/*	Clean out completed outbound FDUs.		*/

		if (scanOutFdus(sdr, currentTime) < 0)
		{
			putErrmsg("Can't scan inbound FDUs.", NULL);
			state = 0;	/*	Terminate loop.		*/
			oK(_running(&state));
		}
	}

	writeErrmsgMemos();
	writeMemo("[i] cfdpclock has ended.");
	ionDetach();
	return 0;
}
