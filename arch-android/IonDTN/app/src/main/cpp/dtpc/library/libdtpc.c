/*
	libdtpc.c:	Functions enabling the implementation of
			DTPC applications.

	Authors: Giorgos Papastergiou, SPICE
		 Ioannis Alexiadis, SPICE

	Copyright (c) 2011, Space Internetworking Center,
	Democritus University of Thrace. ALL RIGHTS RESERVED.
										*/

#include "dtpcP.h"

typedef struct
{
	int		interval;	/*	Seconds.		*/
	sm_SemId	semaphore;
} TimerParms;

int     dtpc_attach()
{
        return dtpcAttach();
}

void     dtpc_detach()
{
        ionDetach();
}

int     dtpc_entity_is_started()
{
        DtpcVdb *vdb = getDtpcVdb();

        return (vdb && vdb->clockPid > 0) ? 1 : 0;
}

char    *dtpc_working_directory()
{
        return getIonWorkingDirectory();
}

int      dtpc_send(unsigned int profileID, DtpcSAP sap, char *dstEid,
			unsigned int maxRtx, unsigned int aggrSizeLimit,
			unsigned int aggrTimeLimit, int lifespan,
			BpAncillaryData *ancillaryData, unsigned char srrFlags,
			BpCustodySwitch custodySwitch, char *reportToEid,
			int classOfService, Object item, unsigned int length)
{
	unsigned int	topicID;
	
	CHKERR(item);
	if (sap)
	{
		 topicID = sap->vsap->topicID;
	}
	else
	{
		putErrmsg("Can't find Sap.", NULL);
		return -1;
	}

	if (profileID == 0)	
	{
		profileID = dtpcGetProfile(maxRtx, aggrSizeLimit, aggrTimeLimit,
				lifespan, ancillaryData, srrFlags,
				custodySwitch, reportToEid, classOfService);
		if (profileID == 0)
		{
			writeMemo("[?] No profile found.");
			return 0;
		}
	}
	
	return insertRecord(sap, dstEid, profileID, topicID, item, length);
}

int	dtpc_open(unsigned int topicID, DtpcElisionFn elisionFn,
		DtpcSAP *dtpcsapPtr)
{
	PsmPartition	wm = getIonwm();
	DtpcVdb		*vdb;
	Sap		sap;
	VSap		*vsap;
	PsmAddress	vsapElt;
	PsmAddress	addr;
	Sdr		sdr;
	
	CHKERR(dtpcsapPtr);	
	vdb = getDtpcVdb();
	*dtpcsapPtr = NULL;	/*	Default, in case of failure.	*/
	sdr = getIonsdr();
	CHKERR(sdr_begin_xn(sdr));	/* 	Just to lock memory.	*/

	/*	Searching for an existing VSap for this topicID	 */

	for (vsapElt = sm_list_first(wm, vdb->vsaps); vsapElt;
			vsapElt = sm_list_next(wm, vsapElt))
	{
		vsap = (VSap *) psp(wm, sm_list_data(wm, vsapElt));
		if (vsap->topicID == topicID)
		{
			break;
                }
	}

	if (vsapElt == 0)	/* No VSap found. Create a new one */
	{
		addr = psm_malloc(wm, sizeof(VSap));
		if (addr == 0)
		{
			sdr_exit_xn(sdr);
			return -1;
		}
		
		vsapElt = sm_list_insert_last(wm, vdb->vsaps, addr);
		if (vsapElt == 0)
		{
			psm_free(wm, addr);
			sdr_exit_xn(sdr);
			return -1;
		}

		vsap = (VSap *) psp(wm, addr);
		memset ((char *) vsap, 0, sizeof(VSap));
		vsap->topicID = topicID;
		vsap->semaphore = sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO);
		vsap->dlvQueue = sdr_list_create(sdr);
		if (vsap->dlvQueue == 0)
		{
			putErrmsg("No space for delivery queue.", NULL);
			sdr_cancel_xn(sdr);
			return -1;
		}

		sdr_list_insert_last(sdr, (getDtpcConstants())->queues,
				vsap->dlvQueue);
		sdr_list_user_data_set(sdr, vsap->dlvQueue, topicID);
		sm_SemTake(vsap->semaphore);     /*      Lock.   */
	}
	else 
	{
		/*	VSap exists; make sure it's not already opened
		 *	by some application.				*/
	
		addr = sm_list_data(wm, vsapElt);
		vsap = (VSap *) psp(wm, addr);
		if (vsap->appPid > 0)  /*	VSap not closed.	*/
		{
			if (sm_TaskExists(vsap->appPid))
			{
				if (vsap->appPid == sm_TaskIdSelf())
				{
					sdr_exit_xn(sdr);
					return 0;
				}

				putErrmsg("Sap is already open.",
						itoa(vsap->appPid));
				sdr_exit_xn(sdr);
				return -1;
			}
			
			/* 	Application terminated without closing the
				VSap, so simply close it now.		*/

			vsap->appPid = -1;
		}
	}

	/*	Construct the service access point.	*/
	
	sap.vsap = vsap;
	sap.elisionFn = elisionFn;
	sap.semaphore = vsap->semaphore;
	*dtpcsapPtr = MTAKE(sizeof(Sap));
	if (*dtpcsapPtr == NULL)
	{
		putErrmsg("Can't create dtpcSAP.", NULL);
		return -1;
	}

	/* Having created the SAP, give its owner exclusive access
	 * to the VSap							*/

	vsap->appPid = sm_TaskIdSelf();
	memcpy((char *) *dtpcsapPtr, (char *) &sap, sizeof(Sap));
	if (sdr_end_xn(sdr) < 0)
	{
		return -1;
	}

	return 0;
}

void	dtpc_close(DtpcSAP sap)
{
	VSap	*vsap;

        if (sap == NULL)
        {
                return;
        }
	
	vsap = sap->vsap;
        if (vsap->appPid == sm_TaskIdSelf())
        {
                vsap->appPid = -1;
        }

	MRELEASE(sap);
}

static void	*timerMain(void *parm)
{
	TimerParms	*timer = (TimerParms *) parm;
	pthread_mutex_t	mutex;
	pthread_cond_t	cv;
	struct timeval	workTime;
	struct timespec	deadline;
	int		result;

	memset((char *) &mutex, 0, sizeof mutex);
	if (pthread_mutex_init(&mutex, NULL))
	{
		putSysErrmsg("can't start timer, mutex init failed", NULL);
		sm_SemGive(timer->semaphore);
		return NULL;
	}

	memset((char *) &cv, 0, sizeof cv);
	if (pthread_cond_init(&cv, NULL))
	{
		putSysErrmsg("can't start timer, cond init failed", NULL);
		sm_SemGive(timer->semaphore);
		return NULL;
	}

	getCurrentTime(&workTime);
	deadline.tv_sec = workTime.tv_sec + timer->interval;
	deadline.tv_nsec = workTime.tv_usec * 1000;
	pthread_mutex_lock(&mutex);
	result = pthread_cond_timedwait(&cv, &mutex, &deadline);
	pthread_mutex_unlock(&mutex);
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&cv);
	if (result)
	{
		errno = result;
		if (errno != ETIMEDOUT)
		{
			putSysErrmsg("timer failure", NULL);
			sm_SemGive(timer->semaphore);
			return NULL;
		}
	}

	/*	Timed out; must wake up the main thread.		*/

	timer->interval = 0;	/*	Indicate genuine timeout.	*/
	sm_SemGive(timer->semaphore);
	return NULL;
}

int	dtpc_receive(DtpcSAP sap, DtpcDelivery *dlvBuffer, int timeoutSeconds)
{
	Sdr		sdr = getIonsdr();
	VSap		*vsap;
	Object		dlvElt;
	Object		dlvObj;
			OBJ_POINTER(DlvPayload, payload);
	TimerParms      timerParms;
	pthread_t	timerThread;
	char		srcEid[SDRSTRING_BUFSZ];

	CHKERR(sap && dlvBuffer);
	if (timeoutSeconds < DTPC_BLOCKING)
	{
		putErrmsg("Illegal timeout interval.", itoa(timeoutSeconds));
		return -1;
	}

	vsap = sap->vsap;
	CHKERR(sdr_begin_xn(sdr));
	if (vsap->appPid != sm_TaskIdSelf())
	{
		sdr_exit_xn(sdr);
		putErrmsg("Can't receive: not owner of the sap.",
				itoa(vsap->appPid));
		return -1;
	}

	if (sm_SemEnded(vsap->semaphore))
	{
		sdr_exit_xn(sdr);
		writeMemo("[?] DTPC service for this topic has been stopped.");
		dlvBuffer->result = DtpcServiceStopped;

		/*	End task, but without error.			*/

		return 0;
	}

	/*	Get oldest payload  record in delivery queue, if any;
         *	wait for one if necessary.				*/

	dlvElt = sdr_list_first(sdr, vsap->dlvQueue);
	 if (dlvElt == 0)
	{
		sdr_exit_xn(sdr);
		if (timeoutSeconds == DTPC_POLL)
		{
			dlvBuffer->result = ReceptionTimedOut;
			return 0;
		}

		/*	Wait for semaphore to be given, either by the
		 *	enqueueForDelivery() function or by timer 
		 *	thread.						*/

		if (timeoutSeconds == DTPC_BLOCKING)
		{
			timerParms.interval = -1;
		}
 		else	/*	This is a receive() with a deadline.	*/
                {
			timerParms.interval = timeoutSeconds;
			timerParms.semaphore = vsap->semaphore;
			if (pthread_create(&timerThread, NULL, timerMain,
					&timerParms) < 0)
			{
				putSysErrmsg("Can't enable interval timer",
							NULL);
				return -1;
			}
		}

		/*	Take vsap semaphore.				*/

		if (sm_SemTake(vsap->semaphore) < 0)
		{
			putErrmsg("Can't take vsap semaphore.", NULL);
			return -1;
		}

		if (sm_SemEnded(vsap->semaphore))
		{
			writeMemoNote("[i] DTPC service for this topic has \
been stopped", itoa(vsap->topicID));
			dlvBuffer->result = DtpcServiceStopped;

			/*	End task, but without error.		*/

			return 0;
		}

		/*	Have taken the semaphore, one way or another.	*/

		CHKERR(sdr_begin_xn(sdr));
		dlvElt = sdr_list_first(sdr, vsap->dlvQueue);
		if (dlvElt == 0)	/*	Still nothing.		*/
		{
			/*	Either sm_SemTake() was interrupted
			 *	or else timer thread gave semaphore.	*/

			sdr_exit_xn(sdr);
			if (timerParms.interval == 0)
			{
				/*	Timer expired.			*/

				dlvBuffer->result = ReceptionTimedOut;
				pthread_join(timerThread, NULL);
			}
			else	/*	Interrupted.			*/
			{
				dlvBuffer->result = ReceptionInterrupted;
				if (timerParms.interval != -1)
				{
					pthread_cancel(timerThread);
					pthread_join(timerThread, NULL);
				}
			}

			return 0;
		}
		else	/*	Payload record was delivered.		*/
		{
			if (timerParms.interval != -1)
			{
				pthread_cancel(timerThread);
				pthread_join(timerThread, NULL);
			}
		}
	}
	
	/*	At this point, we have got a dlvElt and are in an SDR
	 *	transaction.						*/

	dlvObj = sdr_list_data(sdr,dlvElt);
	GET_OBJ_POINTER(sdr, DlvPayload, payload, dlvObj);
	sdr_string_read(sdr, srcEid, payload->srcEid);
	sdr_free(sdr, payload->srcEid);

	/*	Now fill in the data indication structure.	*/

	dlvBuffer->result = PayloadPresent;
	dlvBuffer->item = payload->content;
	dlvBuffer->length = payload->length;
	dlvBuffer->srcEid = MTAKE(SDRSTRING_BUFSZ);
	if (dlvBuffer->srcEid == NULL)
	{
		sdr_exit_xn(sdr);
		putErrmsg("Can't create source EID string.", NULL);
		return -1;
	}

	memcpy((char *) dlvBuffer->srcEid, (char *) srcEid, SDRSTRING_BUFSZ);

	/*	Finally delete the delivery list element and destroy
	 *	the dlvPayload, but not its content.			*/

	sdr_free(sdr, dlvObj);
	sdr_list_delete(sdr, dlvElt, NULL, NULL);

	if (sdr_end_xn(sdr) < 0)
	{
		MRELEASE(dlvBuffer->srcEid);
		dlvBuffer->srcEid = NULL;
		putErrmsg("Failure in payload reception.", NULL);
		return -1;
	}
	return 0;
}

void	dtpc_interrupt(DtpcSAP sap)
{
	/*	Give semaphore, simulating reception notice.		*/

	if (sap != NULL && sap->semaphore != SM_SEM_NONE)
	{
		sm_SemGive(sap->semaphore);
	}
}

void	dtpc_release_delivery(DtpcDelivery *dlvBuffer)
{
	Sdr	sdr = getIonsdr();

	CHKVOID(dlvBuffer);
	if (dlvBuffer->result == PayloadPresent)
	{
		if (dlvBuffer->srcEid)
		{
			MRELEASE(dlvBuffer->srcEid);
			dlvBuffer->srcEid = NULL;
		}	

		CHKVOID(sdr_begin_xn(sdr));
		sdr_free(sdr, dlvBuffer->item);
		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Failed releasing delivery.", NULL);
		}

		dlvBuffer->item = 0;
	}
}
