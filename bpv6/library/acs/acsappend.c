/*
	acsappend.c: Append a bundle ID to an aggregate custody signal.

	Authors: Andrew Jenkins, Sebastian Kuzminsky,
				University of Colorado at Boulder

	Copyright (c) 2008-2011, Regents of the University of Colorado.
	This work was supported by NASA contracts NNJ05HE10G, NNC06CB40C, and
	NNC07CB47C.
											*/

#include "acsP.h"

static Sdr		acsSdr = NULL;

static int appendToSdrAcsFills(Object fills, const CtebScratchpad *cteb)
{
	unsigned int id = cteb->id;

	Object   curFillLElt;
	Object   curFillAddr;
	AcsFill  curFill;
	Object   nextFillLElt;
	Object   nextFillAddr;
	AcsFill  nextFill;
	Object   newFillAddr;
	AcsFill  newFill;
	unsigned int curFillUntil;	/* Last sequence number covered by curId. */

	ASSERT_ACSSDR_XN;

	/* If we insert a new sequence element into the list, it will be this. */
	newFill.start = id;
	newFill.length = 1;

	/* If the list is empty, then add newFill. */
	if (sdr_list_length(acsSdr, fills) == 0) {
		newFillAddr = sdr_malloc(acsSdr, sizeof(newFill));
		if (newFillAddr == 0)
		{
			ACSLOG_WARN("Couldn't sdr_malloc() new fill %u (%s)",
				id, strerror(errno));
			return -1;			/* Error */
		}
		sdr_poke(acsSdr, newFillAddr, newFill);

		if (sdr_list_insert_first(acsSdr, fills, newFillAddr) == 0)
		{
			ACSLOG_WARN("Couldn't insert new fill %u into list (%s)",
				id, strerror(errno));
			/* Stale newFill object will be backed out of SDR on
			 * sdr_end_xn() */
			return -1;			/* Error */
		}
		return 0;
	}

	for(curFillLElt = sdr_list_first(acsSdr, fills);
		curFillLElt;
		curFillLElt = sdr_list_next(acsSdr, curFillLElt))
	{
		curFillAddr = sdr_list_data(acsSdr, curFillLElt);
		if (curFillAddr == 0) {
			ACSLOG_ERROR("Can't read ACS list element (%s)", strerror(errno));
			return -1;
		}
		sdr_peek(acsSdr, curFill, curFillAddr);

		if (curFill.start <= id)
		{
			curFillUntil = curFill.start + curFill.length - 1;

			if (curFillUntil >= id)
			{
				return -2;		/* Already added */
			}

			nextFillLElt = sdr_list_next(acsSdr, curFillLElt);
			if (nextFillLElt != 0) {
				nextFillAddr = sdr_list_data(acsSdr, nextFillLElt);
				if(nextFillAddr == 0) {
					ACSLOG_ERROR("Couldn't read next ACS list element");
					return -1;
				}
				sdr_peek(acsSdr, nextFill, nextFillAddr);
			} else {
				nextFillAddr = 0;
			}

			/* If the current fill is just one too small on the right, 
			 * we should extend it. */
			if (curFillUntil + 1 == id)
			{
				/* We need to extend curFill.length by one.  But if that means
				 * we completely fill the hole between curFill and nextFill,
				 * then we should smush nextFill into curFill, too. */
				if (nextFillAddr && nextFill.start == id + 1)
				{
					curFill.length += 1 + nextFill.length;
					sdr_list_delete(acsSdr, nextFillLElt, NULL, NULL);
					sdr_free(acsSdr, nextFillAddr);
				} else {
					curFill.length++;
				}

				sdr_poke(acsSdr, curFillAddr, curFill);
				return 0;
			}

			/* If the next fill is just one too small on the left, we should
			 * extend it. */
			if (nextFillAddr && nextFill.start == id + 1)
			{
				nextFill.start = id;
				nextFill.length++;
				sdr_poke(acsSdr, nextFillAddr, nextFill);
				return 0;
			}

			/* If the next fill is after our id or doesn't exist, then
			 * we should insert this id in between current and next. */
			if (nextFillAddr == 0 || nextFill.start > id)
			{
				newFillAddr = sdr_malloc(acsSdr, sizeof(newFill));
				if (newFillAddr == 0)
				{
					ACSLOG_WARN("Couldn't sdr_malloc() new fill %u (%s)",
							id, strerror(errno));
					return -1;			/* Error */
				}
				sdr_poke(acsSdr, newFillAddr, newFill);

				if (sdr_list_insert_after(acsSdr, curFillLElt, newFillAddr) == 0)
				{
					ACSLOG_WARN("Couldn't insert new fill %u into list (%s)",
							id, strerror(errno));
					/* Stale newFill object will be backed out of SDR on
					 * sdr_end_xn() */
					return -1;			/* Error */
				}
				return 0;
			}

			/* If here, curFill.start <= id, but this wasn't the appropriate
			 * place to insert this sequence, because nextFill->id is
			 * also <= id. */
			continue;
		}

		/* curFill.id > id, and curFill.id wasn't ever <= id, so we need to
		 * insert a new id at the head of the list. */
		if (curFill.start == id + 1) {
			/* Extend curFill to the left by 1. */
			curFill.start = id;
			curFill.length++;
			sdr_poke(acsSdr, curFillAddr, curFill);
			return 0;
		} else {
			/* Make a new object at the beginning of the list. */
			newFillAddr = sdr_malloc(acsSdr, sizeof(newFill));
			if (newFillAddr == 0)
			{
				ACSLOG_WARN("Couldn't sdr_malloc() new fill %u (%s)",
						id, strerror(errno));
				return -1;			/* Error */
			}
			sdr_poke(acsSdr, newFillAddr, newFill);

			if (sdr_list_insert_first(acsSdr, fills, newFillAddr) == 0)
			{
				ACSLOG_WARN("Couldn't insert new fill %u into list (%s)",
						id, strerror(errno));
				/* Stale newFill object will be backed out of SDR on
				 * sdr_end_xn() */
				return -1;			/* Error */
			}
			return 0;
		}
	}

	ACSLOG_ERROR("Hit bottom of appendToAcsFills for %u", id);
	return -1;
}

static int newSdrAcsSignal(Object acsSignals, Object pendingCustAddr,
	BpCtReason reasonCode, unsigned char succeeded, const CtebScratchpad *cteb)
{
	SdrAcsSignal	newAcsSignal;
	Address			newAcsSignalAddr;
	Object			newAcsSignalLElt;

	ASSERT_ACSSDR_XN;

	newAcsSignalAddr = sdr_malloc(acsSdr, sizeof(newAcsSignal));
	if (newAcsSignalAddr == 0)
	{
		ACSLOG_WARN("Couldn't sdr_malloc() new ACS signal %s, %d, (%s)",
				succeeded ? "success" : "fail", reasonCode, strerror(errno));
		return -1;			/* Error */
	}

	newAcsSignal.succeeded  = succeeded;
	newAcsSignal.reasonCode = reasonCode;
	newAcsSignal.acsFills   = sdr_list_create(acsSdr);
	newAcsSignal.acsDue		= 0;
	newAcsSignal.pendingCustAddr = pendingCustAddr;
	newAcsSignal.serializedZco = 0;
	newAcsSignal.serializedZcoLength = 0;
	if(newAcsSignal.acsFills == 0) {
		ACSLOG_WARN("Couldn't allocate new ACS fills list (%s)", strerror(errno));
		return -1;
	}
	sdr_poke(acsSdr, newAcsSignalAddr, newAcsSignal);

	newAcsSignalLElt = sdr_list_insert(acsSdr, acsSignals, newAcsSignalAddr, cmpSdrAcsSignals,
			&newAcsSignal);

	if(newAcsSignalLElt == 0)
	{
		ACSLOG_WARN("Couldn't insert new ACS signal %s, %d, into list (%s)",
				succeeded ? "success" : "fail", reasonCode, strerror(errno));
		return -1;			/* Error */
	}

	return appendToSdrAcsFills(newAcsSignal.acsFills, cteb);
}

int appendToSdrAcsSignals(Object acsSignals, Object pendingCustAddr, 
	BpCtReason reasonCode, unsigned char succeeded,
	const CtebScratchpad *cteb)
{
	Object			curAcsSignalAddr;
	SdrAcsSignal	curAcsSignal;

	ASSERT_ACSSDR_XN;

	if(findSdrAcsSignal(acsSignals, reasonCode, succeeded,
							&curAcsSignalAddr) != 0)
	{
		ACSLOG_DEBUG("Found existing custody signal (%s,%d)",
			succeeded ? "SUCCESS" : "FAIL", reasonCode);
		sdr_peek(acsSdr, curAcsSignal, curAcsSignalAddr);
		return appendToSdrAcsFills(curAcsSignal.acsFills, cteb);
	}

	ACSLOG_DEBUG("Making new custody signal for (%s,%d)",
			succeeded ? "SUCCESS" : "FAIL", reasonCode);

	/* previous < (reasonCode, succeeded) < curAcsSignal; insert here */
	return newSdrAcsSignal(acsSignals, pendingCustAddr, reasonCode,
						succeeded, cteb);
}

static char *printSdrAcs(Object acsSignalAddr, const char *custodianEid)
{
	char	*reprAcs = MTAKE(MAX_REPRACS_LEN + 4);
	char	*cursor = reprAcs;
	int 	rc = 0;
	int		reprAcsLeft = MAX_REPRACS_LEN;
	int		first = 1;

	SdrAcsSignal		acsSignal;
	Object				fillsLElt;
	Object 				fillsAddr;
	SdrAcsFill		 	fill;

	if(reprAcs == NULL) {
		ACSLOG_ERROR("Couldn't print ACS (%s)", strerror(errno));
		return NULL;
	}

	ASSERT_ACSSDR_XN;

	sdr_peek(acsSdr, acsSignal, acsSignalAddr);

	rc = snprintf(cursor, reprAcsLeft, "%s: ",
				acsSignal.succeeded ? "SACK" : "SNACK");
	reprAcsLeft -= rc;
	cursor += rc;

	if(custodianEid != NULL)
	{
		rc = snprintf(cursor, reprAcsLeft, "to %s, ", custodianEid);
		reprAcsLeft -= rc;
		cursor += rc;
	}

	for(fillsLElt = sdr_list_first(acsSdr, acsSignal.acsFills);
		fillsLElt;
		fillsLElt = sdr_list_next(acsSdr, fillsLElt))
	{
		fillsAddr = sdr_list_data(acsSdr, fillsLElt);
		if(fillsAddr == 0)
		{
			ACSLOG_ERROR("Can't get list to print SDR ACS (%s)", strerror(errno));
			MRELEASE(reprAcs);
			return NULL;
		}
		sdr_peek(acsSdr, fill, fillsAddr);

		rc = snprintf(cursor, reprAcsLeft, "%s%u(%u)", first ? "" : " +",
					fill.start, fill.length);
		if (rc < 0 || rc >= reprAcsLeft) {
			sprintf(cursor, "...");
			return reprAcs;
		}
		reprAcsLeft -= rc;
		cursor += rc;
		first = 0;
	}

	return reprAcs;
}

static void printSdrAcsSignal(int loglevel, Object acsSignals, BpCtReason reasonCode,
				unsigned char succeeded, const char *custodianEid)
{
	Object signalAddr;
	char *reprAcsSignal;

	CHKVOID(sdr_begin_xn(acsSdr));
	if (findSdrAcsSignal(acsSignals, reasonCode, succeeded,
				&signalAddr))
	{
		reprAcsSignal = printSdrAcs(signalAddr, custodianEid);
		ACSLOG(loglevel, reprAcsSignal, NULL);
		MRELEASE(reprAcsSignal);
	}

	sdr_exit_xn(acsSdr);
}

static void printAcsInformation(int loglevel, const char *note, Bundle *bundle,
		char *dictionary, int succeeded, BpCtReason reasonCode)
{
	char *currentCustodianEid;
	char *sourceEid;

	if (printEid(&bundle->id.source, dictionary, &sourceEid) < 0)
	{
		putErrmsg("Couldn't print source of bundle.", NULL);
		return;
	}
	if (printEid(&bundle->custodian, dictionary, &currentCustodianEid) < 0)
	{
		putErrmsg("Couldn't print source of bundle.", NULL);
		MRELEASE(sourceEid);
		return;
	}

	if (bundle->bundleProcFlags & BDL_IS_FRAGMENT)
	{
		ACSLOG(loglevel, "%s: %s,%u.%u(%u:%u) (reason: %d, current \
custodian: %s)", succeeded ? "SACK" : "SNACK", sourceEid, 
			(unsigned int) (bundle->id.creationTime.seconds),
			bundle->id.creationTime.count,
			bundle->id.fragmentOffset, bundle->payload.length,
			reasonCode, currentCustodianEid);
	} else {
		ACSLOG(loglevel, "%s: %s,%u.%u (reason: %d, current \
custodian: %s)", succeeded ? "SACK" : "SNACK", sourceEid, 
			(unsigned int) (bundle->id.creationTime.seconds),
			bundle->id.creationTime.count,
			reasonCode, currentCustodianEid);
	}
	MRELEASE(sourceEid);
	MRELEASE(currentCustodianEid);
}

int	offerNoteAcs(Bundle *bundle, AcqWorkArea *work, char *dictionary,
				 int succeeded, BpCtReason reasonCode)
{
	Object				pendingCustAddr;
	SdrAcsPendingCust	pendingCust;
	Object				pendingCustodians;
	int					rc;
	char				*currentCustodianEid;
	Sdr					bpSdr = getIonsdr();
	CtebScratchpad		cteb;

	if (acsAttach() < 0)
	{
		//Couldn't offerNoteAcs(): ACS SDR not available.
		return 0;
	}

	if ((acsSdr = getAcssdr()) == NULL)
	{
		putErrmsg("Couldn't offerNoteAcs(): Can't get ACS SDR.", NULL);
		return 0;
	}

	if (printEid(&bundle->custodian, dictionary, &currentCustodianEid) < 0)
	{
		ACSLOG_ERROR("Couldn't print current custodian of a bundle.");
		return 0;		/* Couldn't note, so caller should send normal CT */
	}

	/* To prevent deadlock, take bpSdr before acsSdr. */
	CHKZERO(sdr_begin_xn(bpSdr));
	CHKZERO(sdr_begin_xn(acsSdr));

	/* Find the valid CTEB for this bundle; else, we can't ACS. */
	if(loadCtebScratchpad(bpSdr, bundle, work, &cteb) < 0)
	{
		MRELEASE(currentCustodianEid);
		sdr_exit_xn(acsSdr);
		sdr_exit_xn(bpSdr);
		return 0;
	}

	/* Find the custodian information associated with this EID */
	pendingCustodians = getPendingCustodians();
	pendingCustAddr = getOrMakeCustodianByEid(pendingCustodians, currentCustodianEid);
	sdr_peek(acsSdr, pendingCust, pendingCustAddr);

	printAcsInformation(ACSLOGLEVEL_DEBUG, "Appending ACS", bundle, dictionary, succeeded, reasonCode);
	rc = appendToSdrAcsSignals(pendingCust.signals, pendingCustAddr,
				reasonCode, succeeded, &cteb);
	if (rc == -2) {
		ACSLOG_INFO("Duplicate ACS signalled for %s, ignored.", pendingCust.eid);
		printSdrAcsSignal(ACSLOGLEVEL_INFO, pendingCust.signals, reasonCode, 
				succeeded, pendingCust.eid);
		MRELEASE(currentCustodianEid);
		sdr_exit_xn(acsSdr);
		sdr_exit_xn(bpSdr);
		return 1;		/* A dup is successfully noted */
	}
	if (rc == -1) {
		ACSLOG_ERROR("Problem signalling ACS for %s (%s)", pendingCust.eid,
				strerror(errno));
		MRELEASE(currentCustodianEid);
		sdr_cancel_xn(acsSdr);
		sdr_exit_xn(bpSdr);
		return 0;		/* Couldn't note, so caller should send normal CT */
	}
	printSdrAcsSignal(ACSLOGLEVEL_DEBUG, pendingCust.signals, reasonCode,
				succeeded, pendingCust.eid);

	if (trySendAcs(&pendingCust, reasonCode, succeeded, &cteb) != 0)
	{
		/* We failed to attempt sending an ACS handling this new signal
		 * (either itself containing the signal, or carrying over this signal
		 * to a new, empty ACS).  We should revert the SDR changes we made
		 * and return 0 so that the caller will send a normal custody signal. */
		MRELEASE(currentCustodianEid);
		sdr_cancel_xn(acsSdr);
		sdr_cancel_xn(bpSdr);
		return 0;
	}

	MRELEASE(currentCustodianEid);

	if(sdr_end_xn(acsSdr) < 0)
	{
		putSysErrmsg("Can't commit new ACS", pendingCust.eid);
		sdr_cancel_xn(bpSdr);
		return 0;
	}
	if(sdr_end_xn(bpSdr) < 0)
	{
		putSysErrmsg("Can't transmit new ACS", pendingCust.eid);
		return 0;
	}
	return 1;			/* Successfully noted */
}
