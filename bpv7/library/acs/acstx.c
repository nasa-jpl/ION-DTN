/*
	acstx.c: implementation supporting the transmission of
		Aggregate Custody Signals (ACS) for the bundle protocol.

	Authors: Andrew Jenkins, Sebastian Kuzminsky, 
				University of Colorado at Boulder

	Copyright (c) 2008-2011, Regents of the University of Colorado.
	This work was supported by NASA contracts NNJ05HE10G, NNC06CB40C, and
	NNC07CB47C.
 											*/
#include "acsP.h"
#include "sdrhash.h"

static char     *acssdrName = ACS_SDR_NAME;
static char	*acsDbName = ACS_DBNAME;
static Object	acsdbObject = 0;
static AcsDB	acsConstantsBuf;
AcsDB           *acsConstants = NULL;
static Sdr	acsSdr = NULL;

Sdr getAcssdr(void)
{
	static int haveTriedAcsSdr = 0;

	if (acsSdr || haveTriedAcsSdr)
	{
		return acsSdr;
	}

	if (ionAttach() < 0)
	{
		putErrmsg("Can't attach to ION.", NULL);
		return NULL;
	}

	haveTriedAcsSdr = 1;
	acsSdr = sdr_start_using(acssdrName);
	if (acsSdr == NULL)
	{
		writeMemoNote("[i] This task is unable to exercise ACS \
because the ACS SDR is not initialized, as noted in message above",
			itoa(sm_TaskIdSelf()));
	}

	return acsSdr;
}


int acsInitialize(long heapWords, int logLevel)
{
	AcsDB	acsdbBuf;
	unsigned long zero = 0;     /* sdr_stow() wants this */

	if (heapWords == 0)
	{
		/* Caller wants us to supply a default. */
		heapWords = ACS_SDR_DEFAULT_HEAPWORDS;
	}

	if (ionAttach() < 0)
	{
		putErrmsg("Can't attach to ION.", NULL);
		return -1;
	}

        {
	   Sdr		sdr = getIonsdr();
           IonDB        iondb;
           char         *pathname = iondb.parmcopy.pathName;

	   CHKERR(sdr_begin_xn(sdr));
           sdr_read(sdr, (char *) &iondb, getIonDbObject(), sizeof(IonDB));
	   sdr_exit_xn(sdr);

#if 0
           {
              char text[100];

              sprintf( text, "ION parms pathname : %s", pathname );

              writeMemo( text );
           }
#endif
           
           if (sdr_load_profile(acssdrName, SDR_IN_DRAM, heapWords,
                                SM_NO_KEY, 0, SM_NO_KEY, pathname, NULL) < 0)
           {
              putErrmsg("Unable to load SDR profile for ACS.", NULL);
              return -1;
           } else {
              writeMemo("ACS SDR profile loaded.");
           }
        }

	acsSdr = sdr_start_using(acssdrName);
	if (acsSdr == NULL)
	{
		putErrmsg("Can't start using SDR for ACS.", NULL);
		return -1;
	}


	if (getAcssdr() == NULL)
	{
		putErrmsg("ACS can't find ACS SDR.", NULL);
		return -1;
	}

	CHKERR(sdr_begin_xn(acsSdr));
	acsdbObject = sdr_find(acsSdr, acsDbName, NULL);
	switch (acsdbObject)
	{
	case -1:		/*	SDR error.			*/
		sdr_cancel_xn(acsSdr);
		putErrmsg("Can't seek ACS database in SDR.", NULL);
		return -1;

	case 0:			/*	Not found must create new DB.	*/
		memset((char *) &acsdbBuf, 0, sizeof(AcsDB));
		acsdbBuf.pendingCusts = sdr_list_create(acsSdr);
		acsdbBuf.logLevel = logLevel;
		acsdbBuf.cidHash = sdr_hash_create(acsSdr, sizeof(AcsCustodyId),
				ACS_CIDHASH_ROWCOUNT, 1);
		acsdbBuf.bidHash = sdr_hash_create(acsSdr, sizeof(AcsBundleId),
				ACS_BIDHASH_ROWCOUNT, 1);
		acsdbBuf.id = sdr_stow(acsSdr, zero);
		acsdbObject = sdr_malloc(acsSdr, sizeof(AcsDB));
		if (acsdbObject == 0)
		{
			sdr_cancel_xn(acsSdr);
			putErrmsg("No space for ACS database.", NULL);
			return -1;
		}

		sdr_write(acsSdr, acsdbObject, (char *) &acsdbBuf, sizeof(AcsDB));
		sdr_catlg(acsSdr, acsDbName, 0, acsdbObject);
		if (sdr_end_xn(acsSdr))
		{
			putErrmsg("Can't create ACS database.", NULL);
			return -1;
		}

		break;

	default:
		sdr_exit_xn(acsSdr);
	}

	acsConstants = &acsConstantsBuf;
	CHKERR(sdr_begin_xn(acsSdr));
	sdr_read(acsSdr, (char *) acsConstants, acsdbObject, sizeof(AcsDB));
	sdr_exit_xn(acsSdr);
	return 0;
}

int acsAttach()
{
	if (acsConstants)
	{
           return 0;
	}

	if (getAcssdr() == NULL)
	{
		/* ACS can't find ACS SDR. */
		return -1;
	}

	CHKERR(sdr_begin_xn(acsSdr));
	if (acsdbObject == 0)
	{
		acsdbObject = sdr_find(acsSdr, acsDbName, NULL);
		if (acsdbObject == 0)
		{
			sdr_exit_xn(acsSdr);
			return -1;
		}
	}
	acsConstants = &acsConstantsBuf;
	sdr_read(acsSdr, (char *) acsConstants, acsdbObject, sizeof(AcsDB));
	sdr_exit_xn(acsSdr);
	return 0;
}

void acsDetach()
{
	if (acsSdr == NULL)
	{
		return;
	}

	sdr_stop_using(acsSdr);
	acsSdr = NULL;
}

Object getPendingCustodians(void)
{
	return acsConstants->pendingCusts;
}

int cmpSdrAcsSignals(Sdr acsSdr, Address lhsAddr, void *argData)
{
	SdrAcsSignal	*rhsp = (SdrAcsSignal *)(argData);
	SdrAcsSignal	lhs;

	sdr_peek(acsSdr, lhs, lhsAddr);

	if(lhs.succeeded < rhsp->succeeded)
	{
		return -1;
	}
	if(lhs.succeeded == rhsp->succeeded)
	{
		if(lhs.reasonCode < rhsp->reasonCode)
		{
			return -1;
		}
		if(lhs.reasonCode == rhsp->reasonCode)
		{
			return 0;
		}
	}
	return 1;
}

Object findSdrAcsSignal(Object acsSignals, BpCtReason reasonCode,
		unsigned char succeeded, Object *signalAddrPtr)
{
	SdrAcsSignal signal;
	Object acsSignalLElt;
	Object acsSignalAddr;

	signal.reasonCode = reasonCode;
	signal.succeeded  = succeeded;

	ASSERT_ACSSDR_XN;

	/* Get the first element of the list. */
	acsSignalLElt = sdr_list_first(acsSdr, acsSignals);
	if (acsSignalLElt == 0)
	{
		ACSLOG_INFO("Couldn't find ACS signal (%s, %d)", 
				succeeded ? "success" : "fail", reasonCode);
		return 0;
	}

	acsSignalLElt = sdr_list_search(acsSdr, acsSignalLElt, 0, cmpSdrAcsSignals, &signal);
	if(acsSignalLElt == 0)
	{
		ACSLOG_INFO("Couldn't find ACS signal (%s, %d)", 
				succeeded ? "success" : "fail", reasonCode);
		return 0;
	}
	acsSignalAddr = sdr_list_data(acsSdr, acsSignalLElt);

	if(signalAddrPtr) *signalAddrPtr = acsSignalAddr;
	return acsSignalLElt;
}
	
static int cmpSdrAcsPendingCust(Sdr sdr, Address eltData, void *argData)
{
	const char *eid = (const char *)(argData);
	SdrAcsPendingCust	pendingCust;
	int					rc;

	sdr_peek(acsSdr, pendingCust, eltData);
	rc = strncmp(pendingCust.eid, eid, MAX_EID_LEN);

	return (rc == 0) ? 0 : -1;
}

Object findCustodianByEid(Object custodians, const char *eid)
{
	Object	pendingCustLElt;

	acsSdr = getAcssdr();

	/* Get the first element of the list. */
	pendingCustLElt = sdr_list_first(acsSdr, custodians);
	if (pendingCustLElt == 0)
	{
		return 0;
	}

	pendingCustLElt = sdr_list_search(acsSdr, pendingCustLElt, 0, cmpSdrAcsPendingCust, (void *)(eid));
	if(pendingCustLElt == 0)
	{
		return 0;
	}
	return sdr_list_data(acsSdr, pendingCustLElt);
}

static void releaseSdrAcsFill(Sdr sdr, Object elt, void *arg)
{
	sdr_free(sdr, sdr_list_data(sdr, elt));
}

static void releaseSdrAcsSignal(Object signalLElt)
{
	Sdr			bpSdr = getIonsdr();
	Sdr                 acsSdr = getAcssdr();
	Object              signalAddr;
	SdrAcsSignal        signal;
	SdrAcsPendingCust   pendingCust;

	assert(signalLElt != 0);
	ASSERT_ACSSDR_XN;
	ASSERT_BPSDR_XN;

	if (acsSdr == NULL)
	{
		putErrmsg("Can't release ACS, SDR not available.", NULL);
		return;
	}

	signalAddr = sdr_list_data(acsSdr, signalLElt);
	if (signalAddr == 0) {
		ACSLOG_ERROR("Can't derefence ACS signal to release it.");
		return;
	}

	sdr_peek(acsSdr, signal, signalAddr);
	sdr_peek(acsSdr, pendingCust, signal.pendingCustAddr);

	/* Destroy the objects this AcsSignal contains */
	sdr_list_destroy(acsSdr, signal.acsFills, releaseSdrAcsFill, NULL);

	if(signal.acsDue != 0) {
		destroyBpTimelineEvent(signal.acsDue);
	}

	if(signal.serializedZco != 0) {
		zco_destroy(bpSdr, signal.serializedZco);
	}

	/* Destroy this AcsSignal */
	sdr_free(acsSdr, signalAddr);
	sdr_list_delete(acsSdr, signalLElt, NULL, NULL);
}

int sendAcs(Object signalLElt)
{
	BpAncillaryData		ancillaryData = { 0, 0, 255 };
	Object			signalAddr;
	SdrAcsSignal        	signal;
	SdrAcsPendingCust	pendingCust;
	int			result;
	Sdr			bpSdr = getIonsdr();

	assert(signalLElt != 0);

	if ((acsSdr = getAcssdr()) == NULL)
	{
		putErrmsg("Can't send ACS, SDR not available.", NULL);
		return -1;
	}

	/* To prevent deadlock, we take the BP SDR before the ACS SDR. */
	CHKERR(sdr_begin_xn(bpSdr));
	CHKERR(sdr_begin_xn(acsSdr));

	signalAddr = sdr_list_data(acsSdr, signalLElt);
	if (signalAddr == 0) {
		ACSLOG_ERROR("Can't derefence ACS signal to send it.");
		sdr_cancel_xn(acsSdr);
		sdr_cancel_xn(bpSdr);
		return -1;
	}

	sdr_peek(acsSdr, signal, signalAddr);
	sdr_peek(acsSdr, pendingCust, signal.pendingCustAddr);

	/* Remove ref to this serialized ZCO from signal; also remove the bundle
	 * IDs covered by this serialized ZCO. */
	result = bpSend(NULL, pendingCust.eid, NULL, ACS_TTL,
			BP_EXPEDITED_PRIORITY, NoCustodyRequested, 0, 0,
			&ancillaryData, signal.serializedZco, NULL,
			BP_CUSTODY_SIGNAL);
	switch (result)
	{
	/* All return codes from bpSend() still cause us to continue processing
	 * to free this ACS.  If it was sent successfully, good.  If it wasn't,
	 * that's due to a system failure or problem with this ACS, so the best
	 * we can do is delete it from our node without sending. */
	case -1:
		ACSLOG_ERROR("Can't send custody transfer signal.");
		zco_destroy(bpSdr, signal.serializedZco);
		break;

	case 0:
		ACSLOG_ERROR("Custody transfer signal not transmitted.");
		zco_destroy(bpSdr, signal.serializedZco);
		break;

	default:
		/* bpSend() gave the serializedZco to a forwarder, so don't
		 * zco_destroy(). */
		break;
	}

	if (signal.acsDue != 0)
	{
		destroyBpTimelineEvent(signal.acsDue);
	}

	signal.acsDue = 0;
	signal.serializedZco = 0;
	sdr_poke(acsSdr, signalAddr, signal);

	releaseSdrAcsSignal(signalLElt);

	if (sdr_end_xn(acsSdr) < 0)
	{
		ACSLOG_ERROR("Couldn't mark a serialized ACS as sent.");
		sdr_cancel_xn(bpSdr);
		return -1;
	}

	if(sdr_end_xn(bpSdr) < 0)
	{
		return -1;
	}

	return result > 0 ? 0 : -1;
}

int trySendAcs(SdrAcsPendingCust *custodian, 
		BpCtReason reasonCode, unsigned char succeeded, 
		const CtebScratchpad *cteb)
{
	Object				signalLElt;
	Object				signalAddr;
	SdrAcsSignal        signal;
	BpEvent				timelineEvent;
	Object				newSerializedZco;
	unsigned long		newSerializedLength;
	int					result;
	Sdr					bpSdr = getIonsdr();

	/* To prevent deadlock, take bpSdr before acsSdr. */
	CHKERR(sdr_begin_xn(bpSdr));
	CHKERR(sdr_begin_xn(acsSdr));

	signalLElt = findSdrAcsSignal(custodian->signals, reasonCode, succeeded,
			&signalAddr);
	if (signalAddr == 0)
	{
		ACSLOG_ERROR("Can't find ACS signal");
		sdr_exit_xn(acsSdr);
		sdr_exit_xn(bpSdr);
		return -1;
	}
	sdr_peek(acsSdr, signal, signalAddr);


	newSerializedLength = serializeAcs(signalAddr, &newSerializedZco,
			signal.serializedZcoLength);
	if (newSerializedLength == 0)
	{
		ACSLOG_ERROR("Can't serialize new ACS (%lu)", signal.serializedZcoLength);
		sdr_cancel_xn(acsSdr);
		sdr_cancel_xn(bpSdr);
		return -1;
	}
	ACSLOG_DEBUG("Serialized a new ACS to %s that is %lu long (old: %lu)",
			custodian->eid, newSerializedLength, signal.serializedZcoLength);


	/* If serializeAcs() (which serializes an ACS that covers all the "old"
	 * custody IDs as well as 1 "new" custody ID that we're trying to append)
	 * returned an ACS that's larger than the custodian' preferred size, then:
	 *  1) Send the old ACS (the biggest ACS that's smaller than custodian's
	 *     preferred size), covering all the old custody IDs but not the new
	 *     one.
	 *  2) Make a new ACS that includes only the new custody ID.
	 */
	if (custodian->acsSize > 0 && newSerializedLength >= custodian->acsSize)
	{
		if(signal.serializedZco == 0)
		{
			/* We don't have an old unserialized ACS to send.  This means the
			 * first custody signal appended to this ACS exceeded the acsSize
			 * parameter.  The best we can do is send this ACS even though it's
			 * bigger than the recommended acsSize. */
			ACSLOG_WARN("Appending first CS to %s was bigger than %lu", 
					custodian->eid, custodian->acsSize);
			signal.serializedZcoLength = newSerializedLength;
			signal.serializedZco = newSerializedZco;
			sdr_poke(acsSdr, signalAddr, signal);
			sendAcs(signalLElt);
			if(sdr_end_xn(acsSdr) < 0)
			{
				ACSLOG_ERROR("Can't serialize ACS bundle.");
				sdr_cancel_xn(bpSdr);
				return -1;
			}
			if (sdr_end_xn(bpSdr) < 0)
			{
				ACSLOG_ERROR("Can't send ACS bundle.");
				return -1;
			}
			return 0;
		}

		/* Calling this invalidates our signalLElt and signalAddr pointers, so
		 * we must re-find the signal before using them again. */
		sendAcs(signalLElt);

		/* Add the one that was uncovered by the serialized payload back in */
		result = appendToSdrAcsSignals(custodian->signals,
				signal.pendingCustAddr, reasonCode, succeeded,
				cteb);
		switch (result)
		{
			case 0:
				/* Success; continue processing. */
				break;
			default:
				ACSLOG_ERROR("Can't carry size-limited ID to new ACS");
				sdr_cancel_xn(acsSdr);
				sdr_cancel_xn(bpSdr);
				return -1;
		}

		/* Find the uncovered one that we just added. */
		signalLElt = findSdrAcsSignal(custodian->signals, reasonCode,
				succeeded, &signalAddr);
		if (signalAddr == 0)
		{
			ACSLOG_ERROR("Can't find ACS signal");
			sdr_cancel_xn(acsSdr);
			sdr_cancel_xn(bpSdr);
			return -1;
		}
		sdr_peek(acsSdr, signal, signalAddr);

		/* Serialize the new one */
		newSerializedLength = serializeAcs(signalAddr, &newSerializedZco, 0);
		if (newSerializedLength <= 0)
		{
			ACSLOG_ERROR("Can't serialize new ACS (%lu)", newSerializedLength);
			sdr_cancel_xn(acsSdr);
			sdr_cancel_xn(bpSdr);
			return -1;
		}
	} else {
		if (signal.serializedZco != 0)
		{
			/* Free the old payload zco. */
			zco_destroy(bpSdr, signal.serializedZco);
		}
	}

	/* Store the new ZCO */
	signal.serializedZco = newSerializedZco;
	signal.serializedZcoLength = newSerializedLength;

	/* If there is not an ACS generation countdown timer, create one. */
	if(signal.acsDue == 0)
	{
		timelineEvent.type = csDue;
		if(custodian->acsDelay == 0) {
			timelineEvent.time = getCtime() + DEFAULT_ACS_DELAY;
		} else {
			timelineEvent.time = getCtime() + custodian->acsDelay;
		}
		timelineEvent.ref  = signalLElt;
		signal.acsDue = insertBpTimelineEvent(&timelineEvent);
		if (signal.acsDue == 0)
		{
			ACSLOG_ERROR("Can't add timeline event to generate ACS");
			sdr_cancel_xn(acsSdr);
			sdr_cancel_xn(bpSdr);
			return -1;
		}
	}
	sdr_poke(acsSdr, signalAddr, signal);
	if(sdr_end_xn(acsSdr) < 0)
	{
		ACSLOG_ERROR("Can't track ACS");
		sdr_cancel_xn(bpSdr);
		return -1;
	}
	if (sdr_end_xn(bpSdr) < 0)
	{
		ACSLOG_ERROR("Can't add timeline event to generate ACS");
		return -1;
	}
	return 0;
}

static Object newSdrAcsCustodian(Object custodians, const char *eid)
{
	Object newCustodianAddr;
	SdrAcsPendingCust newCustodian;

	memset(&newCustodian, 0, sizeof(newCustodian));
	strncpy(newCustodian.eid, eid, MAX_EID_LEN);
	newCustodian.eid[MAX_EID_LEN] = '\0';

	/* Set default ACS size and delay. */
	newCustodian.acsDelay = 2;
	newCustodian.acsSize = 300;

	CHKZERO(sdr_begin_xn(acsSdr));
	newCustodianAddr = sdr_malloc(acsSdr, sizeof(newCustodian));
	newCustodian.signals = sdr_list_create(acsSdr);
	sdr_poke(acsSdr, newCustodianAddr, newCustodian);
	sdr_list_insert_last(acsSdr, custodians, newCustodianAddr);
	if(sdr_end_xn(acsSdr) < 0)
	{
		ACSLOG_WARN("Couldn't create new custodian info for %s", eid);
		return 0;
	}
	return newCustodianAddr;
}

Object getOrMakeCustodianByEid(Object custodians, const char *eid)
{
	Object custodianAddr;

	/* Try to find custodian info if it exists. */
	CHKZERO(sdr_begin_xn(acsSdr));
	custodianAddr = findCustodianByEid(custodians, eid);
	if(sdr_end_xn(acsSdr) < 0)
	{
		putErrmsg("Can't search custodian database", eid);
		return 0;
	}

	/* If no custodian info, make it. */
	if(custodianAddr == 0)
	{
		custodianAddr = newSdrAcsCustodian(custodians, eid);
		if(custodianAddr == 0)
		{
			ACSLOG_WARN("Couldn't record new custodian info for %s", eid);
			return 0;
		}
	}
	return custodianAddr;
}

int updateCustodianAcsDelay(const char *custodianEid,
		unsigned long acsDelay)
{
	Object custodianAddr;
	SdrAcsPendingCust	custodian;
	
 	custodianAddr = getOrMakeCustodianByEid(acsConstants->pendingCusts, custodianEid);
	if(custodianAddr == 0)
	{
		return -1;
	}

	CHKERR(sdr_begin_xn(acsSdr));
	sdr_peek(acsSdr, custodian, custodianAddr);
	custodian.acsDelay = acsDelay;
	sdr_poke(acsSdr, custodianAddr, custodian);
	if(sdr_end_xn(acsSdr) < 0)
	{
		ACSLOG_WARN("Couldn't write new custodian info for %s", custodianEid);
		return -1;
	}
	return 0;
}

int updateCustodianAcsSize(const char *custodianEid,
		unsigned long acsSize)
{
	Object custodianAddr;
	SdrAcsPendingCust	custodian;

	custodianAddr = getOrMakeCustodianByEid(acsConstants->pendingCusts, custodianEid);
	if(custodianAddr == 0)
	{
		return -1;
	}

	CHKERR(sdr_begin_xn(acsSdr));
	sdr_peek(acsSdr, custodian, custodianAddr);
	custodian.acsSize = acsSize;
	sdr_poke(acsSdr, custodianAddr, custodian);
	if(sdr_end_xn(acsSdr) < 0)
	{
		ACSLOG_WARN("Couldn't write new custodian info for %s", custodianEid);
		return -1;
	}
	return 0;
}

int updateMinimumCustodyId(unsigned int minimumCustodyId)
{
	CHKERR(sdr_begin_xn(acsSdr));
	sdr_poke(acsSdr, acsConstants->id, minimumCustodyId);
	if(sdr_end_xn(acsSdr) < 0)
	{
		ACSLOG_ERROR("Couldn't update minimum custody ID to %u", minimumCustodyId);
		return -1;
	}
	return 0;
}

void listCustodianInfo(void (*printer)(const char *))
{
	Object custodianLElt;
	Object custodianAddr;
	SdrAcsPendingCust	custodian;
	char buffer[1024];

	CHKVOID(sdr_begin_xn(acsSdr));
	for(custodianLElt = sdr_list_first(acsSdr, acsConstants->pendingCusts);
		custodianLElt;
		custodianLElt = sdr_list_next(acsSdr, custodianLElt))
	{
		custodianAddr = sdr_list_data(acsSdr, custodianLElt);
		sdr_peek(acsSdr, custodian, custodianAddr);

		snprintf(buffer, sizeof(buffer), "%.*s\tDelay: %lu Size: %lu",
			MAX_EID_LEN, custodian.eid, 
			custodian.acsDelay,
			custodian.acsSize);
		printer(buffer);
	}
	oK(sdr_end_xn(acsSdr));
}
