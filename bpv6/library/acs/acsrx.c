/*
	acsrx.c: implementation supporting the reception of 
		Aggregate Custody Signals (ACS) for the bundle protocol.

	Authors: Andrew Jenkins, Sebastian Kuzminsky, 
				University of Colorado at Boulder

	Copyright (c) 2008-2011, Regents of the University of Colorado.
	This work was supported by NASA contracts NNJ05HE10G, NNC06CB40C, and
	NNC07CB47C.
 											*/

#include "acsP.h"

static char		*TooShortMemo = "ACS error: not enough to parse.";

static void eraseAggregateCtSignal(Acs *acs)
{	
	LystElt	fillLElt;
	AcsFill *fillElt;

	for(fillLElt = lyst_first(acs->fills);
		fillLElt;
		fillLElt = lyst_first(acs->fills))
	{
		fillElt = lyst_data(fillLElt);
		MRELEASE(fillElt);
		lyst_delete(fillLElt);
	}
	lyst_destroy(acs->fills);

	MRELEASE(acs);
}


/* Generates a printable representation of a Aggregate Custody Signal.
 * Returns a string that may be up to MAX_REPRACS_LEN+4 long; if this function
 * ran out of space before it finished generating the representation, the
 * returned string will end in "...".
 *
 * Returns NULL if out of memory.
 * Caller must MRELEASE() the returned string once finished. */
static char *printAcs(const Acs *acs)
{
	char	*reprAcs = MTAKE(MAX_REPRACS_LEN + 4);
	char	*cursor = reprAcs;		
	char	*toReturn;
	int		rc = 0;
	int		reprAcsLeft = MAX_REPRACS_LEN;
	LystElt	fillLElt;
	AcsFill *fillElt;
	
	if(reprAcs == NULL) {
		putSysErrmsg("Couldn't print ACS", NULL);
		return NULL;
	}

	rc = snprintf(cursor, reprAcsLeft, "%s:", acs->succeeded ? "SACK" : "SNACK");
	reprAcsLeft -= rc;
	cursor += rc;

	for(fillLElt = lyst_first(acs->fills);
		fillLElt;
		fillLElt = lyst_next(fillLElt))
	{
		fillElt = lyst_data(fillLElt);
		rc = snprintf(cursor, reprAcsLeft, "%u(%u),", fillElt->start, fillElt->length);
		if (rc < 0 || rc >= reprAcsLeft) {
			sprintf(cursor, "...");
			return reprAcs;
		}
		reprAcsLeft -= rc;
		cursor += rc;
	}

	toReturn = MTAKE(MAX_REPRACS_LEN - reprAcsLeft + 1);
	memcpy(toReturn, reprAcs, MAX_REPRACS_LEN - reprAcsLeft);
	toReturn[MAX_REPRACS_LEN - reprAcsLeft] = '\0';
	MRELEASE(reprAcs);
	return toReturn;
}


/*  Parses a string of bytes at cursor into a BpAcs structure.
 *  May allocate many Lysts of other Acs-related structures.
 *
 *  The resulting ACS must be freed with bpEraseAggregateCtSignal(), even
 *  if this function is not successful.
 *
 *  Returns 0 on a parsing error, -1 when out-of-memory, and 1 if
 *  successful.
 */
int	parseAggregateCtSignal(void **acsptr, unsigned char *cursor,
			int unparsedBytes, int bundleIsFragment)
{
	AcsFill *fillElt;
	char	*acsAsString;
	Acs		*acs = MTAKE(sizeof(Acs));

	/* Fill start and fill length fields in ACS are SDNVs
	 * that are encoded as deltas from the last SDNV.  These variables manage
	 * storing these last SDNVs. */
	unsigned long lastFill = 0;
	unsigned char isLastFill = 0;

	if (unparsedBytes < 1)
	{
		putErrmsg(TooShortMemo, NULL);
		errno = EMSGSIZE;
		eraseAggregateCtSignal(acs);
		return 0;
	}

	if (bundleIsFragment)
	{
		putErrmsg("ACS can't be marked 'for a fragment.'", NULL);
		errno = EINVAL;
		eraseAggregateCtSignal(acs);
		return 0;
	}

	acs->succeeded = ((*cursor & 0x80) > 0);
	acs->reasonCode = *cursor & 0x7f;
	cursor++;
	unparsedBytes--;
	
	acs->fills = lyst_create_using( getIonMemoryMgr() );

	/* Read ACS fills from the ACS until out-of-bytes. */
	/* FIXME: Handle parse error. */
	while (unparsedBytes > 0)
	{
		fillElt = MTAKE(sizeof(AcsFill));
		if (fillElt == 0)
		{
			putErrmsg("No space for fillElt", itoa(sizeof(AcsFill)));
			eraseAggregateCtSignal(acs);
			return -1;
		}
		extractSmallSdnv(&fillElt->start, &cursor, &unparsedBytes);
		if (isLastFill)
		{
			fillElt->start += lastFill;
		}

		extractSmallSdnv(&fillElt->length, &cursor, &unparsedBytes);

		lastFill = fillElt->start + fillElt->length - 1;
		isLastFill = 1;

		lyst_insert_last(acs->fills, fillElt);
	}

	/* Generate a printable representation of the ACS we just parsed, and
	 * print it to the ION log. */
	acsAsString = printAcs(acs);
	if(acsAsString == NULL) {
		putErrmsg("Problem parsing ACS", NULL);
		eraseAggregateCtSignal(acs);
		return 0;
	}
	ACSLOG_DEBUG("Parsed ACS %s", acsAsString);
	MRELEASE(acsAsString);
	*acsptr = acs;
	return 1;
}

/* Calls acsCallback() with an abstract custody signal for each bundle
 * referenced by an aggregate custody signal. */
static void acsForeach(const Acs *acs, 
	void acsCallback(BpCtSignal *, void *), void *userdata)
{	
	unsigned long i;
	int rc;
	BpCtSignal	ctSig;
	LystElt	fillLElt;
	AcsFill *fillElt;
	AcsCustodyId cid;
	AcsBundleId bid;

	ctSig.succeeded = acs->succeeded;
	ctSig.reasonCode = acs->reasonCode;
	ctSig.signalTime = acs->signalTime;

	for(fillLElt = lyst_first(acs->fills);
		fillLElt;
		fillLElt = lyst_next(fillLElt))
	{
		fillElt = lyst_data(fillLElt);

		for (i = fillElt->start; i < fillElt->start + fillElt->length; i++)
		{
			cid.id = i;
			rc = get_bundle_id(&cid, &bid);
			if (rc == -1) {
				/* system error already logged. */
				return;
			}
			if (rc == 1) {
				ACSLOG_WARN("Received an unknown custody ID (%u)", cid.id);
				continue;
			}

			/* FIXME: Make ACS compatible with fragments. */
			ctSig.isFragment = 0;
			ctSig.fragmentOffset = 0;
			ctSig.fragmentLength = 0;

			memcpy(&ctSig.creationTime, &bid.creationTime, sizeof(BpTimestamp));
			ctSig.sourceEid = bid.sourceEid;

			acsCallback(&ctSig, userdata);
		}
	}
}

typedef struct
{
	BpDelivery 	*dlv;
	CtSignalCB	handleCtSignal;
	int		*running;
} handleAcssArgs_t;

static void handleAcss(BpCtSignal *ctSignal, void *userdata)
{
	handleAcssArgs_t *args = (handleAcssArgs_t *)(userdata);

	ACSLOG_DEBUG("Handling an ACS for %s, %u.%u, %u, %u",
		ctSignal->sourceEid,
		(unsigned int) (ctSignal->creationTime.seconds),
		ctSignal->creationTime.count, ctSignal->fragmentOffset,
		ctSignal->fragmentLength);

	if (args->handleCtSignal(args->dlv, ctSignal) < 0)
	{
		putSysErrmsg("Custody signal handler failed",
				NULL);
		*args->running = 0;
		return;
	}

	if (applyCtSignal(ctSignal, args->dlv->bundleSourceEid) < 0)
	{
		putSysErrmsg("Abstract custody signal handler failed",
				NULL);
		*args->running = 0;
		return;
	}
}


int handleAcs(void *acs_untyped, BpDelivery *dlv, 
	CtSignalCB handleCtSignal)
{
	/*	An aggregate custody signal is a custody signal that
	 *	covers possibly many bundles, instead of just one.
	 *	Expand the ACS into a list of "logical" custody signals
	 *	and apply each one in the list.				*/

	Acs *acs = (Acs *)(acs_untyped);
	int rc = 1;		/*	1 means "okay, still running."	*/
	handleAcssArgs_t args;

	args.dlv = dlv;
	args.handleCtSignal = handleCtSignal;
	args.running = &rc;

	acsForeach(acs, handleAcss, &args);
	eraseAggregateCtSignal(acs);

	return rc;
}

