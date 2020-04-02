/*
 *	libbp.c:	functions enabling the implementation of
 *			BP applications.
 *
 *	Copyright (c) 2003, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 *
 *	Modification History:
 *	Date	  Who	What
 *	06-27-03  SCB	Original development.
 *	01-09-06  SCB	Revision per version 4 of Bundle Protocol spec.
 */

#include "bpP.h"

extern void	bpEndpointTally(VEndpoint *vpoint, unsigned int idx,
			unsigned int size);

typedef struct
{
	int		interval;	/*	Seconds.		*/
	sm_SemId	semaphore;
} TimerParms;

int	bp_attach()
{
	return bpAttach();
}

int	bp_agent_is_started()
{
	BpVdb	*vdb = getBpVdb();

	return (vdb && vdb->clockPid != ERROR);
}

Sdr	bp_get_sdr()
{
	return getIonsdr();
}

void	bp_detach()
{
#if (!(defined (ION_LWT)))
	bpDetach();
#endif
	ionDetach();
}

static int	createBpSAP(Sdr sdr, char *eidString, BpSAP *bpsapPtr,
			VEndpoint **vpointRef)
{
	MetaEid		metaEid;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	Sap		sap;
	VEndpoint	*vpoint;
	PsmAddress	vpointElt;

	memset((char *) &sap, 0, sizeof(Sap));

	/*	First validate the endpoint ID.				*/

	if (parseEidString(eidString, &metaEid, &vscheme, &vschemeElt) == 0)
	{
		putErrmsg("Malformed EID.", eidString);
		return -1;
	}

	if (metaEid.nullEndpoint)	/*	No SAP is possible.	*/
	{
		restoreEidString(&metaEid);
		return 0;
	}

	if (vschemeElt == 0)
	{
		putErrmsg("Scheme not known.", metaEid.schemeName);
		restoreEidString(&metaEid);
		return -1;
	}

	findEndpoint(NULL, metaEid.nss, vscheme, &vpoint, &vpointElt);
	if (vpointElt == 0)
	{
		putErrmsg("Endpoint not known.", metaEid.nss);
		restoreEidString(&metaEid);
		return -1;
	}

	/*	Endpoint exists; if SAP will be used for bundle
	 *	reception, make sure endpoint is not already opened
	 *	by some other application.				*/


	if (vpointRef)	/*	SAP will be used for bundle reception.	*/
	{
		if (vpoint->appPid != ERROR)	/*	Not closed.	*/
		{
			if (sm_TaskExists(vpoint->appPid))
			{
				restoreEidString(&metaEid);
				if (vpoint->appPid == sm_TaskIdSelf())
				{
					return 0;
				}

				putErrmsg("Endpoint is already open.",
						itoa(vpoint->appPid));
				return -1;
			}

			/*	Application terminated without closing
			 *	the endpoint, so simply close it now.	*/

			vpoint->appPid = ERROR;
		}

		*vpointRef = sap.vpoint = vpoint;
	}

	/*	Construct the service access point.			*/

	memcpy(&sap.endpointMetaEid, &metaEid, sizeof(MetaEid));
	sap.endpointMetaEid.colon = NULL;
	sap.endpointMetaEid.schemeName = MTAKE(metaEid.schemeNameLength + 1);
	if (sap.endpointMetaEid.schemeName == NULL)
	{
		restoreEidString(&metaEid);
		putErrmsg("Can't create BpSAP.", NULL);
		return -1;
	}

	sap.endpointMetaEid.nss = MTAKE(metaEid.nssLength + 1);
	if (sap.endpointMetaEid.nss == NULL)
	{
		MRELEASE(sap.endpointMetaEid.schemeName);
		restoreEidString(&metaEid);
		putErrmsg("Can't create BpSAP.", NULL);
		return -1;
	}

	*bpsapPtr = MTAKE(sizeof(Sap));
	if (*bpsapPtr == NULL)
	{
		MRELEASE(sap.endpointMetaEid.nss);
		MRELEASE(sap.endpointMetaEid.schemeName);
		restoreEidString(&metaEid);
		putErrmsg("Can't create BpSAP.", NULL);
		return -1;
	}

	istrcpy(sap.endpointMetaEid.schemeName, metaEid.schemeName,
			metaEid.schemeNameLength + 1);
	istrcpy(sap.endpointMetaEid.nss, metaEid.nss,
			metaEid.nssLength + 1);
	memcpy((char *) (*bpsapPtr), (char *) &sap, sizeof(Sap));
	restoreEidString(&metaEid);
	return 0;
}

int	bp_open(char *eidString, BpSAP *bpsapPtr)
{
	Sdr		sdr;
	VEndpoint	*vpoint;
	int		result;

	CHKERR(eidString && *eidString && bpsapPtr);
	*bpsapPtr = NULL;	/*	Default, in case of failure.	*/
	sdr = getIonsdr();
	CHKERR(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	result = createBpSAP(sdr, eidString, bpsapPtr, &vpoint);
	if (*bpsapPtr)
	{
		(*bpsapPtr)->recvSemaphore = vpoint->semaphore;
		(*bpsapPtr)->detain = 0;

		/*	Give owner of this SAP exclusive reception
		 *	access on the endpoint.				*/

		vpoint->appPid = sm_TaskIdSelf();
	}

	sdr_exit_xn(sdr);		/*	Unlock memory.		*/
	return result;
}

int	bp_open_source(char *eidString, BpSAP *bpsapPtr, int detain)
{
	Sdr		sdr;
	int		result;

	CHKERR(eidString && *eidString && bpsapPtr);
	*bpsapPtr = NULL;	/*	Default, in case of failure.	*/
	sdr = getIonsdr();
	CHKERR(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	result = createBpSAP(sdr, eidString, bpsapPtr, NULL);
	if (*bpsapPtr)
	{
		(*bpsapPtr)->recvSemaphore = SM_SEM_NONE;
		(*bpsapPtr)->detain = detain;
	}

	sdr_exit_xn(sdr);		/*	Unlock memory.		*/
	return result;
}

void	bp_close(BpSAP sap)
{
	if (sap == NULL)
	{
		return;
	}

	if (sap->vpoint)	/*	SAP is configured for reception.*/
	{
		if (sap->vpoint->appPid == sm_TaskIdSelf())
		{
			/*	Must detach the endpoint.		*/

			sap->vpoint->appPid = ERROR;
		}
	}

	MRELEASE(sap->endpointMetaEid.nss);
	MRELEASE(sap->endpointMetaEid.schemeName);
	MRELEASE(sap);
}

int	bp_parse_quality_of_service(const char *token,
		BpAncillaryData *ancillaryData, BpCustodySwitch *custodySwitch,
		int *priority)
{
	int	count;
	unsigned int myCustodyRequested;
	unsigned int myPriority;
	unsigned int myOrdinal;
	unsigned int myUnreliable;
	unsigned int myCritical;
	unsigned int myDataLabel;

	count = sscanf(token, "%11u.%11u.%11u.%11u.%11u.%11u",
			&myCustodyRequested, &myPriority, &myOrdinal,
			&myUnreliable, &myCritical, &myDataLabel);
	switch (count)
	{
	case 6:
		/*	All unsigned ints are valid data labels.	*/
		/*	Intentional fall-through to next case.		*/

	case 5:
		if ((myCritical != 0 && myCritical != 1)
		|| (myUnreliable != 0 && myUnreliable != 1))
		{
			return 0;	/*	Invalid format.		*/
		}

		/*	Intentional fall-through to next case.		*/

	case 3:
		if (myOrdinal > 254)
		{
			return 0;	/*	Invalid format.		*/
		}

		/*	Intentional fall-through to next case.		*/

	case 2:
		if ((myPriority > 2)
		||  (myCustodyRequested != 0 && myCustodyRequested != 1))
		{
			return 0;	/*	Invalid format.		*/
		}

		break;

	default:
		return 0;		/*	Invalid format.		*/
	}

	/*	Syntax and bounds-checking passed; assign to outputs.	*/

	ancillaryData->flags = 0;
	if (count >= 6)
	{
		ancillaryData->dataLabel = myDataLabel;
		ancillaryData->flags |= BP_DATA_LABEL_PRESENT;
	}
	else
	{
		ancillaryData->dataLabel = 0;
	}

	if (count >= 5)
	{
		ancillaryData->flags |= ((myUnreliable ? BP_BEST_EFFORT : 0)
				| (myCritical ? BP_MINIMUM_LATENCY : 0));
	}
	else
	{
		ancillaryData->flags = 0;
	}

	if (count >= 3)
	{
		ancillaryData->ordinal = myOrdinal;
	}
 
	*priority = myPriority;
	*custodySwitch = (myCustodyRequested ? 
			SourceCustodyRequired : NoCustodyRequested);
	return 1;
}

int	bp_send(BpSAP sap, char *destEid, char *reportToEid, int lifespan,
		int classOfService, BpCustodySwitch custodySwitch,
		unsigned char srrFlags, int ackRequested,
		BpAncillaryData *ancillaryData, Object adu, Object *bundleObj)
{
	BpAncillaryData	defaultAncillaryData = { 0, 0, 0 };
	MetaEid		*sourceMetaEid;

	if (adu == 0)
	{
		writeMemo("[?] No application data unit to send.");
		return 0;
	}

	if (ancillaryData == NULL)
	{
		ancillaryData = &defaultAncillaryData;
	}
	else
	{
		if (ancillaryData->ordinal == 255)	/*	Reserve	*/
		{
			ancillaryData->ordinal = 254;
		}
	}

	if (sap)
	{
		sourceMetaEid = &(sap->endpointMetaEid);
		if (sap->detain)
		{
			if (bundleObj == NULL)
			{
				writeMemo("[?] Can't return bundle address.");
				return 0;
			}
		}
		else
		{
			if (bundleObj)
			{
				*bundleObj = 0;
				bundleObj = NULL;
			}
		}
	}
	else					/*	Anonymous.	*/
	{
		sourceMetaEid = NULL;
		if (bundleObj)
		{
			*bundleObj = 0;
			bundleObj = NULL;
		}
	}

	return bpSend(sourceMetaEid, destEid, reportToEid, lifespan,
			classOfService, custodySwitch, srrFlags, ackRequested,
			ancillaryData, adu, bundleObj, 0);
}

int	bp_track(Object bundleObj, Object trackingElt)
{
	Sdr	sdr = getIonsdr();
		OBJ_POINTER(Bundle, bundle);

	CHKERR(bundleObj && trackingElt);
	CHKERR(sdr_begin_xn(sdr));
	GET_OBJ_POINTER(sdr, Bundle, bundle, bundleObj);
	if (bundle->trackingElts == 0)
	{
		sdr_exit_xn(sdr);
		putErrmsg("Corrupt bundle?  Has no trackingElts list.", NULL);
		return -1;
	}

	sdr_list_insert_last(sdr, bundle->trackingElts, trackingElt);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Failed adding bundle tracking elt.", NULL);
		return -1;
	}

	return 0;
}

void	bp_untrack(Object bundleObj, Object trackingElt)
{
	Sdr	sdr = getIonsdr();
		OBJ_POINTER(Bundle, bundle);
	Object	elt;

	CHKVOID(bundleObj && trackingElt);
	CHKVOID(sdr_begin_xn(sdr));
	GET_OBJ_POINTER(sdr, Bundle, bundle, bundleObj);
	if (bundle->trackingElts == 0)
	{
		sdr_exit_xn(sdr);
		return;
	}

	for (elt = sdr_list_first(sdr, bundle->trackingElts); elt;
			elt = sdr_list_next(sdr, elt))
	{
		if (sdr_list_data(sdr, elt) == trackingElt)
		{
			break;
		}
	}

	if (elt == 0)		/*	Not found.			*/
	{
		sdr_exit_xn(sdr);
		return;
	}

	sdr_list_delete(sdr, elt, NULL, NULL);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Failed removing bundle tracking elt.", NULL);
	}
}

int	bp_memo(Object bundleObj, unsigned int interval)
{
	if (interval == 0)
	{
		return 0;
	}

	return bpMemo(bundleObj, interval);
}

int	bp_suspend(Object bundleObj)
{
	Sdr		sdr = getIonsdr();
	Bundle		bundle;
	Object		queue;
	Object		planObj;
	BpPlan		plan;

	CHKERR(bundleObj);
	CHKERR(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &bundle, bundleObj, sizeof(Bundle));
	if (bundle.ancillaryData.flags & BP_MINIMUM_LATENCY)
	{
		writeMemo("[?] Attempt to suspend a 'critical' bundle.");
		sdr_exit_xn(sdr);	/*	Nothing to do.		*/
		return 0;
	}

	if (bundle.suspended == 1)	/*	Already suspended.	*/
	{
		sdr_exit_xn(sdr);	/*	Nothing to do.		*/
		return 0;
	}

	bundle.suspended = 1;
	queue = sdr_list_list(sdr, bundle.planXmitElt);
	planObj = sdr_list_user_data(sdr, queue);
	if (planObj == 0)
	{
		/*	Object is already in limbo for other reasons.
		 *	Just record the suspension flag.		*/

		sdr_write(sdr, bundleObj, (char *) &bundle, sizeof(Bundle));
	}
	else
	{
		/*	Must reverse the enqueuing of this bundle
		 *	and place it in limbo.				*/

		sdr_stage(sdr, (char *) &plan, planObj, sizeof(BpPlan));
		if (reverseEnqueue(bundle.planXmitElt, &plan, 1))
		{
			putErrmsg("Can't reverse bundle enqueue.", NULL);
			sdr_cancel_xn(sdr);
			return -1;
		}

		sdr_write(sdr, planObj, (char *) &plan, sizeof(BpPlan));
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Failure in bundle suspension.", NULL);
		return -1;
	}

	return 0;
}

int	bp_resume(Object bundleObj)
{
	Sdr	sdr = getIonsdr();
	Bundle	bundle;

	CHKERR(bundleObj);
	CHKERR(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &bundle, bundleObj, sizeof(Bundle));
	if (bundle.suspended == 0)
	{
		sdr_exit_xn(sdr);	/*	Nothing to do.		*/
		return 0;
	}

	if (releaseFromLimbo(bundle.ductXmitElt, 1) < 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't resume transmission of bundle.", NULL);
		return -1;
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Failure in bundle resumption.", NULL);
		return -1;
	}

	return 0;
}

int	bp_cancel(Object bundleObj)
{
	Sdr	sdr = getIonsdr();

	CHKERR(bundleObj);
	CHKERR(sdr_begin_xn(sdr));
	if (bpDestroyBundle(bundleObj, 1) < 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't cancel bundle.", NULL);
		return -1;
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Failure in bundle cancellation.", NULL);
		return -1;
	}

	return 0;
}

int	bp_release(Object bundleObj)
{
	Sdr	sdr = getIonsdr();
	Bundle	bundle;

	CHKERR(bundleObj);
	CHKERR(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &bundle, bundleObj, sizeof(Bundle));
	bundle.detained = 0;
	sdr_write(sdr, bundleObj, (char *) &bundle, sizeof(Bundle));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Failure in bundle release.", NULL);
		return -1;
	}

	return 0;
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

int	bp_receive(BpSAP sap, BpDelivery *dlvBuffer, int timeoutSeconds)
{
	Sdr		sdr = getIonsdr();
	VEndpoint	*vpoint;
			OBJ_POINTER(Endpoint, endpoint);
	Object		dlvElt;
	Object		bundleAddr;
	Bundle		bundle;
	TimerParms	timerParms;
	pthread_t	timerThread;
	int		result;
	char		*dictionary;

	CHKERR(sap && dlvBuffer);
	if (timeoutSeconds < BP_BLOCKING)
	{
		putErrmsg("Illegal timeout interval.", itoa(timeoutSeconds));
		return -1;
	}

	vpoint = sap->vpoint;
	CHKERR(sdr_begin_xn(sdr));
	if (vpoint->appPid != sm_TaskIdSelf())
	{
		sdr_exit_xn(sdr);
		putErrmsg("Can't receive: not owner of endpoint.",
				itoa(vpoint->appPid));
		return -1;
	}

	if (sm_SemEnded(vpoint->semaphore))
	{
		sdr_exit_xn(sdr);
		writeMemo("[?] Endpoint has been stopped.");
		dlvBuffer->result = BpEndpointStopped;

		/*	End task, but without error.			*/

		return 0;
	}

	/*	Get oldest bundle in delivery queue, if any; wait
	 *	for one if necessary.					*/

	GET_OBJ_POINTER(sdr, Endpoint, endpoint, sdr_list_data(sdr,
			vpoint->endpointElt));
	dlvElt = sdr_list_first(sdr, endpoint->deliveryQueue);
	if (dlvElt == 0)
	{
		sdr_exit_xn(sdr);
		if (timeoutSeconds == BP_POLL)
		{
			dlvBuffer->result = BpReceptionTimedOut;
			return 0;
		}

		/*	Wait for semaphore to be given, either by the
		 *	deliverBundle() function or by timer thread.	*/

		if (timeoutSeconds == BP_BLOCKING)
		{
			timerParms.interval = -1;
		}
		else	/*	This is a receive() with a deadline.	*/
		{
			timerParms.interval = timeoutSeconds;
			timerParms.semaphore = vpoint->semaphore;
			if (pthread_begin(&timerThread, NULL, timerMain,
					&timerParms, "bprcvTimer") < 0)
			{
				putSysErrmsg("Can't enable interval timer",
						NULL);
				return -1;
			}
		}

		/*	Take endpoint semaphore.			*/

		if (sm_SemTake(vpoint->semaphore) < 0)
		{
			putErrmsg("Can't take endpoint semaphore.", NULL);
			return -1;
		}

		if (sm_SemEnded(vpoint->semaphore))
		{
			writeMemo("[i] Endpoint has been stopped.");
			dlvBuffer->result = BpEndpointStopped;

			/*	End task, but without error.		*/

			return 0;
		}

		/*	Have taken the semaphore, one way or another.	*/

		CHKERR(sdr_begin_xn(sdr));
		dlvElt = sdr_list_first(sdr, endpoint->deliveryQueue);
		if (dlvElt == 0)	/*	Still nothing.		*/
		{
			/*	Either sm_SemTake() was interrupted
			 *	or else timer thread gave semaphore.	*/

			sdr_exit_xn(sdr);
			if (timerParms.interval == 0)
			{
				/*	Timer expired.			*/

				dlvBuffer->result = BpReceptionTimedOut;
				pthread_join(timerThread, NULL);
			}
			else	/*	Interrupted.			*/
			{
				dlvBuffer->result = BpReceptionInterrupted;
				if (timerParms.interval != -1)
				{
					pthread_end(timerThread);
					pthread_join(timerThread, NULL);
				}
			}

			return 0;
		}
		else		/*	Bundle was delivered.		*/
		{
			if (timerParms.interval != -1)
			{
				pthread_end(timerThread);
				pthread_join(timerThread, NULL);
			}
		}
	}

	/*	At this point, we have got a dlvElt and are in an SDR
	 *	transaction.						*/

	bundleAddr = sdr_list_data(sdr, dlvElt);
	sdr_stage(sdr, (char *) &bundle, bundleAddr, sizeof(Bundle));
	dictionary = retrieveDictionary(&bundle);
	if (dictionary == (char *) &bundle)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't retrieve dictionary.", NULL);
		return -1;
	}

	/*	Now fill in the data indication structure.		*/

	dlvBuffer->result = BpPayloadPresent;
	if (printEid(&bundle.id.source, dictionary,
			&dlvBuffer->bundleSourceEid) < 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't print source EID.", NULL);
		return -1;
	}

	dlvBuffer->bundleCreationTime.seconds = bundle.id.creationTime.seconds;
	dlvBuffer->bundleCreationTime.count = bundle.id.creationTime.count;
	dlvBuffer->timeToLive = bundle.timeToLive;
	dlvBuffer->adminRecord = bundle.bundleProcFlags & BDL_IS_ADMIN;
	dlvBuffer->adu = bundle.payload.content;
	dlvBuffer->ackRequested = bundle.bundleProcFlags & BDL_APP_ACK_REQUEST;

	dlvBuffer->metadataType = bundle.ancillaryData.metadataType;
	dlvBuffer->metadataLen = bundle.ancillaryData.metadataLen;
	memcpy(dlvBuffer->metadata, bundle.ancillaryData.metadata,
			BP_MAX_METADATA_LEN);

	/*	Now before returning we send delivery status report
	 *	if it is requested.					*/

	if (SRR_FLAGS(bundle.bundleProcFlags) & BP_DELIVERED_RPT)
	{
		bundle.statusRpt.flags |= BP_DELIVERED_RPT;
		getCurrentDtnTime(&bundle.statusRpt.deliveryTime);
	}

	if (bundle.statusRpt.flags)
	{
		result = sendStatusRpt(&bundle, dictionary);
		if (result < 0)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("Can't send status report.", NULL);
			return -1;
		}
	}

	/*	Finally delete the delivery list element and destroy
	 *	the bundle itself.					*/

	if (dictionary)
	{
		MRELEASE(dictionary);
	}

	sdr_list_delete(sdr, dlvElt, (SdrListDeleteFn) NULL, NULL);
	bundle.dlvQueueElt = 0;
	bundle.payload.content = 0;
	sdr_write(sdr, bundleAddr, (char *) &bundle, sizeof(Bundle));
	bpEndpointTally(vpoint, BP_ENDPOINT_DELIVERED, bundle.payload.length);
	if (bpDestroyBundle(bundleAddr, 0) < 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't destroy bundle.", NULL);
		return -1;
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Failure in bundle reception.", NULL);
		return -1;
	}

	return 0;
}

void	bp_interrupt(BpSAP sap)
{
	/*	Give semaphore, simulating reception notice.		*/

	if (sap != NULL && sap->recvSemaphore != SM_SEM_NONE)
	{
		sm_SemGive(sap->recvSemaphore);
	}
}

void	bp_release_delivery(BpDelivery *dlvBuffer, int releasePayload)
{
	Sdr	sdr = getIonsdr();

	CHKVOID(dlvBuffer);
	if (dlvBuffer->result == BpPayloadPresent)
	{
		if (dlvBuffer->bundleSourceEid)
		{
			MRELEASE(dlvBuffer->bundleSourceEid);
			dlvBuffer->bundleSourceEid = NULL;
		}

		if (releasePayload)
		{
			if (dlvBuffer->adu)
			{
				CHKVOID(sdr_begin_xn(sdr));
				zco_destroy(sdr, dlvBuffer->adu);
				if (sdr_end_xn(sdr) < 0)
				{
					putErrmsg("Failed releasing delivery.",
							NULL);
				}

				dlvBuffer->adu = 0;
			}
		}
	}
}
