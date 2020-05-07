/*
	ltpclock.c:	scheduled-event management daemon for LTP.

	Author: Scott Burleigh, JPL

	Copyright (c) 2007, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include "ltpP.h"

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

static void	shutDown(int signum)	/*	Stops ltpclock.		*/
{
	uaddr	stop = 0;

	oK(_running(&stop));	/*	Terminates ltpclock.		*/
}

static int	dispatchEvents(Sdr sdr, Object events, time_t currentTime)
{
	Object		elt;
	Object		eventObj;
	LtpEvent	event;
	int		result;

	while (1)
	{
		CHKERR(sdr_begin_xn(sdr));
		elt = sdr_list_first(sdr, events);
		if (elt == 0)	/*	No more events to dispatch.	*/
		{
			sdr_exit_xn(sdr);
			return 0;
		}

		eventObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &event, eventObj, sizeof(LtpEvent));
		if (event.scheduledTime > currentTime)
		{
			/*	This is the first future event.		*/

			sdr_exit_xn(sdr);
			return 0;
		}

		sdr_free(sdr, eventObj);
		sdr_list_delete(sdr, elt, NULL, NULL);
		switch (event.type)
		{
		case LtpResendCheckpoint:
			result = ltpResendCheckpoint(event.refNbr2,
					event.refNbr3);
			break;		/*	Out of switch.		*/

		case LtpResendXmitCancel:
			result = ltpResendXmitCancel(event.refNbr2);
			break;		/*	Out of switch.		*/

		case LtpResendReport:
			result = ltpResendReport(event.refNbr1,
					event.refNbr2, event.refNbr3);
			break;		/*	Out of switch.		*/

		case LtpResendRecvCancel:
			result = ltpResendRecvCancel(event.refNbr1,
					event.refNbr2);
			break;		/*	Out of switch.		*/

		case LtpForgetImportSession:
			sdr_list_delete(sdr, event.parm, NULL, NULL);
			result = 0;
			break;		/*	Out of switch.		*/

#if CLOSED_EXPORTS_ENABLED
		case LtpForgetExportSession:
			ltpForgetClosedExport(event.parm);
			result = 0;
			break;		/*	Out of switch.		*/
#endif
		default:		/*	Spurious event.		*/
			result = 0;	/*	Event is ignored.	*/
		}

		if (result < 0)		/*	Dispatching failed.	*/
		{
			sdr_cancel_xn(sdr);
			putErrmsg("failed handing LTP event", NULL);
			return result;
		}

		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("failed dispatching LTP event", NULL);
			return -1;
		}
	}
}

static int	manageLinks(Sdr sdr, time_t currentTime)
{
	PsmPartition	ionwm = getIonwm();
	LtpVdb		*ltpvdb = getLtpVdb();
	IonVdb		*ionvdb = getIonVdb();
	PsmAddress	elt;
	LtpVspan	*vspan;
	Object		obj;
	LtpSpan		span;
	IonNeighbor	*neighbor;
	PsmAddress	nextElt;
	unsigned int	priorXmitRate;

	CHKERR(sdr_begin_xn(sdr));
	for (elt = sm_list_first(ionwm, ltpvdb->spans); elt;
			elt = sm_list_next(ionwm, elt))
	{
		vspan = (LtpVspan *) psp(ionwm, sm_list_data(ionwm, elt));

		/*	Finish aggregation as necessary.		*/

		obj = sdr_list_data(sdr, vspan->spanElt);
		sdr_stage(sdr, (char *) &span, obj, sizeof(LtpSpan));
		if (span.lengthOfBufferedBlock > 0)
		{
			span.ageOfBufferedBlock++;
			sdr_write(sdr, obj, (char *) &span, sizeof(LtpSpan));
			if (span.ageOfBufferedBlock >= span.aggrTimeLimit)
			{
				sm_SemGive(vspan->bufClosedSemaphore);
			}
		}

		/*	Find Neighbor object encapsulating the current
		 *	known state of this LTP engine.			*/

		neighbor = findNeighbor(ionvdb, vspan->engineId, &nextElt);
		if (neighbor == NULL)
		{
			neighbor = addNeighbor(ionvdb, vspan->engineId);
			if (neighbor == NULL)
			{
				putErrmsg("Can't update span.", NULL);
				return -1;
			}
		}

		if (neighbor->xmitRate == 0)
		{
			if (vspan->localXmitRate > 0)
			{
				vspan->localXmitRate = 0;
				ltpStopXmit(vspan);
			}
		}
		else
		{
			if (vspan->localXmitRate == 0)
			{
				vspan->localXmitRate = neighbor->xmitRate;
				ltpStartXmit(vspan);
			}
		}

		if (neighbor->fireRate == 0)
		{
			if (vspan->remoteXmitRate > 0)
			{
				priorXmitRate = vspan->remoteXmitRate;
				vspan->remoteXmitRate = 0;
				if (ltpSuspendTimers(vspan, elt, currentTime,
						priorXmitRate))
				{
					putErrmsg("Can't manage links.", NULL);
					return -1;
				}
			}
		}
		else
		{
			if (vspan->remoteXmitRate == 0)
			{
				vspan->remoteXmitRate = neighbor->fireRate;
				if (ltpResumeTimers(vspan, elt, currentTime,
						vspan->remoteXmitRate))
				{
					putErrmsg("Can't manage links.", NULL);
					return -1;
				}
			}
		}

		vspan->receptionRate = neighbor->recvRate;
		vspan->owltInbound = neighbor->owltInbound;
		vspan->owltOutbound = neighbor->owltOutbound;
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("ltpclock failed managing links.", NULL);
		return -1;
	}

	return 0;
}

#if defined (ION_LWT)
int	ltpclock(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
#else
int	main(int argc, char *argv[])
{
#endif
	Sdr	sdr;
	LtpDB	*ltpConstants;
	uaddr	state = 1;
	time_t	currentTime;

	if (ltpInit(0) < 0)
	{
		putErrmsg("ltpclock can't initialize LTP.", NULL);
		return 1;
	}

	sdr = getIonsdr();
	ltpConstants = getLtpConstants();
	isignal(SIGTERM, shutDown);

	/*	Main loop: wait for event occurrence time, then
	 *	execute applicable events.				*/

	oK(_running(&state));
	writeMemo("[i] ltpclock is running.");
	while (_running(NULL))
	{
		/*	Sleep for 1 second, then dispatch all events
		 *	whose executions times have now been reached.	*/

		snooze(1);
		currentTime = getCtime();

		/*	Infer link state changes from rate changes
		 *	noted in the shared ION database.		*/

		if (manageLinks(sdr, currentTime) < 0)
		{
			putErrmsg("Can't manage links.", NULL);
			state = 0;	/*	Terminate loop.		*/
			oK(_running(&state));
			continue;
		}

		/*	Then dispatch retransmission events, as
		 *	constrained by the new link state.		*/

		if (dispatchEvents(sdr, ltpConstants->timeline, currentTime)
				< 0)
		{
			putErrmsg("Can't dispatch events.", NULL);
			state = 0;	/*	Terminate loop.		*/
			oK(_running(&state));
			continue;
		}
	}

	writeErrmsgMemos();
	writeMemo("[i] ltpclock has ended.");
	ionDetach();
	return 0;
}
