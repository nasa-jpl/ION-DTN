/*
 *	cbr.c:		Core implementation of Compressed Bundle Reporting
 *			(CBR) and Custody Transfer (CT) for BPv7.
 *
 *	Based on CCSDS 734.6-O-1: Custody Transfer and Compressed
 *	Bundle Status Reporting (Experimental Specification, Issue 1,
 *	June 2026).
 *
 *	Copyright (c) 2026, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
 *
 *	Author: ION Development Team
 */

#include "cbrP.h"
#include "cbor.h"

/*	*	*	Module-Level Variables	*	*	*	*/

static SdrObject _cbrDbObject(SdrObject *newObj)
{
	static SdrObject cbrDbObj = 0;

	if (newObj)
	{
		cbrDbObj = *newObj;
	}

	return cbrDbObj;
}

static CbrDb	*_cbrConstants(void)
{
	static CbrDb	cbrBuf;
	static CbrDb	*cbrConstants = NULL;
	Sdr		sdr;
	SdrObject	cbrDbObj;

	if (cbrConstants == NULL)
	{
		sdr = getIonsdr();
		CHKNULL(sdr);
		cbrDbObj = _cbrDbObject(NULL);
		if (cbrDbObj == 0)
		{
			return NULL;
		}

		CHKNULL(sdr_begin_xn(sdr));
		sdr_read(sdr, (char *) &cbrBuf, cbrDbObj, sizeof(CbrDb));
		sdr_exit_xn(sdr);
		cbrConstants = &cbrBuf;
	}

	return cbrConstants;
}

SdrObject getCbrDbObject(void)
{
	return _cbrDbObject(NULL);
}

CbrDb	*getCbrConstants(void)
{
	return _cbrConstants();
}

/*	*	*	Initialization Functions	*	*	*/

int	cbr_initialize(Sdr sdr)
{
	SdrObject cbrDbObj;
	CbrDb	cbrBuf;

	CHKERR(sdr);

	/*	Recover the CBR database, creating it if necessary.
	 *	Must be inside a transaction for sdr_find().		*/

	CHKERR(sdr_begin_xn(sdr));
	cbrDbObj = sdr_find(sdr, "cbrdb", NULL);
	switch (cbrDbObj)
	{
	case -1:		/*	SDR error.			*/
		putErrmsg("Can't search for CBR database in SDR.", NULL);
		sdr_cancel_xn(sdr);
		return -1;

	case 0:			/*	Not found; must create new DB.	*/
		cbrDbObj = sdr_malloc(sdr, sizeof(CbrDb));
		if (cbrDbObj == 0)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("No space for CBR database.", NULL);
			return -1;
		}

		memset(&cbrBuf, 0, sizeof(CbrDb));
		cbrBuf.seqCounters = sdr_list_create(sdr);
		cbrBuf.pendingCrs = sdr_list_create(sdr);
		cbrBuf.pendingCcs = sdr_list_create(sdr);
		cbrBuf.custodyBundles = sdr_list_create(sdr);
		cbrBuf.custodyAcceptByCustodian = sdr_list_create(sdr);
		cbrBuf.custodyAcceptBySource = sdr_list_create(sdr);
		cbrBuf.custodyReqDests = sdr_list_create(sdr);
		cbrBuf.crsHistory = sdr_list_create(sdr);
		cbrBuf.crsHistoryMax = 100;

		/*	Default configuration			*/
		cbrBuf.crsAggregateLimit = 10;
		cbrBuf.ccsAggregateLimit = 10;
		cbrBuf.aggregateTimeoutSec = 5;
		cbrBuf.counterMaxValue = CBR_COUNTER_MAX_64BIT;
		cbrBuf.retransmitStrategy = CBR_RETX_NONE;
		cbrBuf.retransmitIntervalSec = 60;
		cbrBuf.maxRetransmissions = 3;

		sdr_write(sdr, cbrDbObj, (char *) &cbrBuf, sizeof(CbrDb));
		sdr_catlg(sdr, "cbrdb", 0, cbrDbObj);

		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Can't create CBR database.", NULL);
			return -1;
		}

		break;

	default:		/*	Found DB in the SDR.		*/
		sdr_exit_xn(sdr);	/*	Nothing to write.	*/
	}

	oK(_cbrDbObject(&cbrDbObj));
	return 0;
}

void	cbr_shutdown(Sdr sdr)
{
	/*	Flush any pending signals before shutdown.		*/
	if (sdr)
	{
		oK(cbr_flushSignals(sdr, CBR_SIGNAL_ALL));
	}
}

int	cbr_attach(void)
{
	Sdr	sdr;
	SdrObject cbrDbObj;

	sdr = getIonsdr();
	if (sdr == NULL)
	{
		putErrmsg("Can't attach to CBR: no SDR.", NULL);
		return -1;
	}

	/*	Must be inside a transaction for sdr_find().		*/

	CHKERR(sdr_begin_xn(sdr));
	cbrDbObj = sdr_find(sdr, "cbrdb", NULL);
	sdr_exit_xn(sdr);		/*	Read-only transaction.	*/

	if (cbrDbObj == 0)
	{
		putErrmsg("CBR database not found.", NULL);
		return -1;
	}

	if (cbrDbObj == (SdrObject) -1)
	{
		putErrmsg("Can't search for CBR database.", NULL);
		return -1;
	}

	oK(_cbrDbObject(&cbrDbObj));
	return 0;
}

/*	*	*	Configuration Functions	*	*	*	*/

int	cbr_configure(Sdr sdr, unsigned int crsAggregateLimit,
		unsigned int ccsAggregateLimit,
		unsigned int aggregateTimeoutSec)
{
	SdrObject cbrDbObj;
	CbrDb	cbrBuf;

	CHKERR(sdr);
	cbrDbObj = _cbrDbObject(NULL);
	CHKERR(cbrDbObj);

	CHKERR(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &cbrBuf, cbrDbObj, sizeof(CbrDb));

	cbrBuf.crsAggregateLimit = crsAggregateLimit;
	cbrBuf.ccsAggregateLimit = ccsAggregateLimit;
	cbrBuf.aggregateTimeoutSec = aggregateTimeoutSec;

	sdr_write(sdr, cbrDbObj, (char *) &cbrBuf, sizeof(CbrDb));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't update CBR configuration.", NULL);
		return -1;
	}

	return 0;
}

int	cbr_getCrebExplicitEid(void)
{
	CbrDb	*cbrConstants = getCbrConstants();

	return cbrConstants ? (int) cbrConstants->crebExplicitEid : 0;
}

int	cbr_setCrebExplicitEid(Sdr sdr, int enable)
{
	SdrObject cbrDbObj;
	CbrDb	cbrBuf;

	CHKERR(sdr);
	cbrDbObj = _cbrDbObject(NULL);
	CHKERR(cbrDbObj);

	CHKERR(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &cbrBuf, cbrDbObj, sizeof(CbrDb));
	cbrBuf.crebExplicitEid = (enable ? 1 : 0);
	sdr_write(sdr, cbrDbObj, (char *) &cbrBuf, sizeof(CbrDb));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't update CREB explicit EID setting.", NULL);
		return -1;
	}

	return 0;
}

int	cbr_getCrebReportTo(Sdr sdr, char *buf, size_t bufLen)
{
	SdrObject cbrDbObj;
	CbrDb	cbrBuf;

	CHKERR(sdr);
	CHKERR(buf);
	CHKERR(bufLen > 0);
	cbrDbObj = _cbrDbObject(NULL);
	CHKERR(cbrDbObj);

	CHKERR(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &cbrBuf, cbrDbObj, sizeof(CbrDb));
	sdr_exit_xn(sdr);
	istrcpy(buf, cbrBuf.crebDefaultReportToEid, bufLen);
	return 0;
}

int	cbr_setCrebReportTo(Sdr sdr, const char *eid)
{
	SdrObject cbrDbObj;
	CbrDb	cbrBuf;

	CHKERR(sdr);
	cbrDbObj = _cbrDbObject(NULL);
	CHKERR(cbrDbObj);

	CHKERR(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &cbrBuf, cbrDbObj, sizeof(CbrDb));
	if (eid == NULL || *eid == '\0')
	{
		cbrBuf.crebDefaultReportToEid[0] = '\0';
	}
	else
	{
		istrcpy(cbrBuf.crebDefaultReportToEid, eid,
				sizeof cbrBuf.crebDefaultReportToEid);
	}

	sdr_write(sdr, cbrDbObj, (char *) &cbrBuf, sizeof(CbrDb));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't update CREB report-to override.", NULL);
		return -1;
	}

	return 0;
}

uvast	cbr_getCounterMaxValue(void)
{
	CbrDb	*cbrConstants = getCbrConstants();

	if (cbrConstants == NULL || cbrConstants->counterMaxValue == 0)
	{
		return CBR_COUNTER_MAX_64BIT;
	}

	return cbrConstants->counterMaxValue;
}

int	cbr_setCounterMaxValue(Sdr sdr, uvast maxValue)
{
	SdrObject cbrDbObj;
	CbrDb	cbrBuf;

	CHKERR(sdr);

	if (maxValue != CBR_COUNTER_MAX_16BIT
	&& maxValue != CBR_COUNTER_MAX_32BIT
	&& maxValue != CBR_COUNTER_MAX_64BIT)
	{
		putErrmsg("Invalid counter max value.", NULL);
		return -1;
	}

	cbrDbObj = _cbrDbObject(NULL);
	CHKERR(cbrDbObj);

	CHKERR(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &cbrBuf, cbrDbObj, sizeof(CbrDb));
	cbrBuf.counterMaxValue = maxValue;
	sdr_write(sdr, cbrDbObj, (char *) &cbrBuf, sizeof(CbrDb));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't update counter max value.", NULL);
		return -1;
	}

	return 0;
}

int	cbr_configureRetransmission(Sdr sdr, int strategy,
		unsigned int intervalSec, unsigned int maxRetransmissions)
{
	SdrObject cbrDbObj;
	CbrDb	cbrBuf;

	CHKERR(sdr);
	cbrDbObj = _cbrDbObject(NULL);
	CHKERR(cbrDbObj);

	if (strategy < CBR_RETX_NONE || strategy > CBR_RETX_SIGNAL)
	{
		putErrmsg("Invalid retransmission strategy.", itoa(strategy));
		return -1;
	}

	CHKERR(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &cbrBuf, cbrDbObj, sizeof(CbrDb));

	cbrBuf.retransmitStrategy = strategy;
	cbrBuf.retransmitIntervalSec = intervalSec;
	cbrBuf.maxRetransmissions = maxRetransmissions;

	sdr_write(sdr, cbrDbObj, (char *) &cbrBuf, sizeof(CbrDb));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't update retransmission config.", NULL);
		return -1;
	}

	return 0;
}

int	cbr_getConfig(Sdr sdr, unsigned int *crsAggregateLimit,
		unsigned int *ccsAggregateLimit,
		unsigned int *aggregateTimeoutSec)
{
	SdrObject cbrDbObj;
	CbrDb	cbrBuf;

	CHKERR(sdr);
	cbrDbObj = _cbrDbObject(NULL);
	CHKERR(cbrDbObj);

	/*	Read straight from the SDR rather than the cached snapshot
	 *	so the reported limits reflect any runtime "m cbraggr".	*/
	CHKERR(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &cbrBuf, cbrDbObj, sizeof(CbrDb));
	sdr_exit_xn(sdr);

	if (crsAggregateLimit)
	{
		*crsAggregateLimit = cbrBuf.crsAggregateLimit;
	}

	if (ccsAggregateLimit)
	{
		*ccsAggregateLimit = cbrBuf.ccsAggregateLimit;
	}

	if (aggregateTimeoutSec)
	{
		*aggregateTimeoutSec = cbrBuf.aggregateTimeoutSec;
	}

	return 0;
}

int	cbr_getRetransmissionConfig(Sdr sdr, int *strategy,
		unsigned int *intervalSec, unsigned int *maxRetransmissions)
{
	CbrDb	*cbrConstants;

	(void) sdr;	/*	Needed for interface consistency.	*/
	cbrConstants = _cbrConstants();
	CHKERR(cbrConstants);

	if (strategy)
	{
		*strategy = cbrConstants->retransmitStrategy;
	}

	if (intervalSec)
	{
		*intervalSec = cbrConstants->retransmitIntervalSec;
	}

	if (maxRetransmissions)
	{
		*maxRetransmissions = cbrConstants->maxRetransmissions;
	}

	return 0;
}

/*	*	*	Statistics Functions	*	*	*	*/

int	cbr_getStatistics(Sdr sdr, CbrStatistics *stats)
{
	CbrDb	cbrDb;
	SdrObject cbrDbObj;

	CHKERR(stats);
	CHKERR(sdr);

	/*	Read fresh from SDR to get current values.
	 *	This is needed because cbrcustodytest runs as a
	 *	separate process and won't see cached updates.	*/

	cbrDbObj = getCbrDbObject();
	if (cbrDbObj == 0)
	{
		memset(stats, 0, sizeof(CbrStatistics));
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &cbrDb, cbrDbObj, sizeof(CbrDb));
	sdr_exit_xn(sdr);

	stats->ccsAcceptSent = cbrDb.ccsAcceptSent;
	stats->ccsRefuseSent = cbrDb.ccsRefuseSent;
	stats->ccsAcceptRecv = cbrDb.ccsAcceptRecv;
	stats->ccsRefuseRecv = cbrDb.ccsRefuseRecv;
	stats->custodyOriginated = cbrDb.custodyOriginated;
	stats->custodyAccepted = cbrDb.custodyAccepted;
	stats->custodyReleased = cbrDb.custodyReleased;
	stats->crsSignalsSent = cbrDb.crsSignalsSent;
	stats->crsSignalsRecv = cbrDb.crsSignalsRecv;

	return 0;
}

int	cbr_resetStatistics(Sdr sdr)
{
	CbrDb		*cbrConstants;
	SdrObject	 cbrDbObj;

	cbrConstants = _cbrConstants();
	CHKERR(cbrConstants);
	cbrDbObj = getCbrDbObject();
	CHKERR(cbrDbObj);

	CHKERR(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) cbrConstants, cbrDbObj, sizeof(CbrDb));

	cbrConstants->ccsAcceptSent = 0;
	cbrConstants->ccsRefuseSent = 0;
	cbrConstants->ccsAcceptRecv = 0;
	cbrConstants->ccsRefuseRecv = 0;
	cbrConstants->custodyOriginated = 0;
	cbrConstants->custodyAccepted = 0;
	cbrConstants->custodyReleased = 0;
	cbrConstants->crsSignalsSent = 0;
	cbrConstants->crsSignalsRecv = 0;

	sdr_write(sdr, cbrDbObj, (char *) cbrConstants, sizeof(CbrDb));

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Failed to reset CBR statistics.", NULL);
		return -1;
	}

	return 0;
}

/*	*	*	Status Report Mode Functions	*	*	*/

int	cbr_getStatusReportMode(Sdr sdr)
{
	BpDB	*bpConstants;

	(void) sdr;	/*	Needed for interface consistency.	*/

	/*	Use getBpConstants() which caches the BpDB values
	 *	and avoids needing a transaction for each read.		*/

	bpConstants = getBpConstants();
	if (bpConstants == NULL)
	{
		return BP_SR_MODE_TRADITIONAL;	/*	Safe default.	*/
	}

	return (int) bpConstants->statusRptMode;
}

int	cbr_setStatusReportMode(Sdr sdr, int mode)
{
	SdrObject	bpDbObj;
	BpDB		bpDb;

	CHKERR(mode >= BP_SR_MODE_TRADITIONAL && mode <= BP_SR_MODE_NONE);

	bpDbObj = getBpDbObject();
	if (bpDbObj == 0)
	{
		putErrmsg("CBR: BpDB not available.", NULL);
		return -1;
	}

	CHKERR(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &bpDb, bpDbObj, sizeof(BpDB));
	bpDb.statusRptMode = (BpStatusReportMode) mode;
	sdr_write(sdr, bpDbObj, (char *) &bpDb, sizeof(BpDB));

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("CBR: failed to set status report mode.", NULL);
		return -1;
	}

	return 0;
}

/*	*	*	Custody Mode Functions	*	*	*	*/

int	cbr_getCustodyMode(Sdr sdr)
{
	SdrObject bpDbObj;
	BpDB	bpDb;

	bpDbObj = getBpDbObject();
	if (bpDbObj == 0)
	{
		return BP_CUSTODY_NONE;	/*	Safe default.		*/
	}

	/*	Read custodyMode directly from SDR so that runtime
	 *	changes via "m custodymode" are visible immediately,
	 *	even to processes that cached BpDB at startup.		*/

	if (sdr_in_xn(sdr))
	{
		sdr_read(sdr, (char *) &bpDb, bpDbObj, sizeof(BpDB));
	}
	else
	{
		CHKERR(sdr_begin_xn(sdr));
		sdr_read(sdr, (char *) &bpDb, bpDbObj, sizeof(BpDB));
		sdr_exit_xn(sdr);
	}

	return (int) bpDb.custodyMode;
}

int	cbr_setCustodyMode(Sdr sdr, int mode)
{
	SdrObject	bpDbObj;
	BpDB		bpDb;

	CHKERR(mode >= BP_CUSTODY_NONE && mode <= BP_CUSTODY_ORANGEBOOK);

	bpDbObj = getBpDbObject();
	if (bpDbObj == 0)
	{
		putErrmsg("CBR: BpDB not available.", NULL);
		return -1;
	}

	CHKERR(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &bpDb, bpDbObj, sizeof(BpDB));
	bpDb.custodyMode = (BpCustodyMode) mode;
	sdr_write(sdr, bpDbObj, (char *) &bpDb, sizeof(BpDB));

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("CBR: failed to set custody mode.", NULL);
		return -1;
	}

	return 0;
}

/*	*	*	Sequence Counter Functions	*	*	*/

SdrObject cbr_findSeqCounter(Sdr sdr, char *sourceEid, char *destEid,
		uvast seqId, int forCustody)
{
	CbrDb	*cbrConstants;
	SdrObject	 elt;
	SdrObject	 counterObj;
	BundleSeqCounter counter;
	char	srcEidBuf[MAX_EID_LEN];
	char	destEidBuf[MAX_EID_LEN];

	cbrConstants = _cbrConstants();
	if (cbrConstants == NULL)
	{
		return 0;
	}

	for (elt = sdr_list_first(sdr, cbrConstants->seqCounters);
			elt; elt = sdr_list_next(sdr, elt))
	{
		counterObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &counter, counterObj,
				sizeof(BundleSeqCounter));

		/*	Check forCustody match				*/
		if (counter.forCustody != forCustody)
		{
			continue;
		}

		/*	Read source EID for comparison			*/
		sdr_string_read(sdr, srcEidBuf, counter.sourceEid);
		if (strcmp(srcEidBuf, sourceEid) != 0)
		{
			continue;
		}

		/*	Per Section 3.2.4: counter lookup logic		*/
		if (seqId == 0)
		{
			/*	SeqID 0: lookup by destEid		*/
			if (counter.seqId != 0)
			{
				continue;
			}

			if (counter.destEid == 0)
			{
				continue;
			}

			sdr_string_read(sdr, destEidBuf, counter.destEid);
			if (destEid && strcmp(destEidBuf, destEid) == 0)
			{
				return elt;
			}
		}
		else
		{
			/*	SeqID != 0: lookup by seqId only	*/
			if (counter.seqId == seqId)
			{
				return elt;
			}
		}
	}

	return 0;	/*	Not found				*/
}

SdrObject cbr_createSeqCounter(Sdr sdr, char *sourceEid, char *destEid,
		uvast seqId, int forCustody)
{
	CbrDb		*cbrConstants;
	BundleSeqCounter counter;
	SdrObject	 counterObj;
	SdrObject	 elt;

	cbrConstants = _cbrConstants();
	CHKZERO(cbrConstants);
	CHKZERO(sourceEid);

	/*	For seqId == 0, destEid is required			*/
	if (seqId == 0 && destEid == NULL)
	{
		putErrmsg("SeqID 0 requires destEid.", NULL);
		return 0;
	}

	memset(&counter, 0, sizeof(BundleSeqCounter));
	counter.sourceEid = sdr_string_create(sdr, sourceEid);
	counter.seqId = seqId;
	if (seqId == 0 && destEid)
	{
		counter.destEid = sdr_string_create(sdr, destEid);
	}

	counter.nextSeqNum = 0;
	counter.counterMaxValue = cbrConstants->counterMaxValue ?
			cbrConstants->counterMaxValue : CBR_COUNTER_MAX_64BIT;
	counter.forCustody = forCustody;
	counter.lastUsed = getCtime();

	counterObj = sdr_malloc(sdr, sizeof(BundleSeqCounter));
	if (counterObj == 0)
	{
		putErrmsg("No space for sequence counter.", NULL);
		return 0;
	}

	sdr_write(sdr, counterObj, (char *) &counter,
			sizeof(BundleSeqCounter));
	elt = sdr_list_insert_last(sdr, cbrConstants->seqCounters, counterObj);

	return elt;
}

SdrObject cbr_getSeqCounter(Sdr sdr, char *sourceEid, char *destEid,
		uvast seqId, int forCustody)
{
	SdrObject elt;

	CHKZERO(sdr);
	CHKZERO(sourceEid);

	/*	First, try to find existing counter			*/
	elt = cbr_findSeqCounter(sdr, sourceEid, destEid, seqId, forCustody);
	if (elt != 0)
	{
		return sdr_list_data(sdr, elt);
	}

	/*	Counter not found, create new one			*/
	elt = cbr_createSeqCounter(sdr, sourceEid, destEid, seqId, forCustody);
	if (elt == 0)
	{
		return 0;
	}

	return sdr_list_data(sdr, elt);
}

int	cbr_allocateSeqNum(Sdr sdr, char *sourceEid, char *destEid,
		uvast seqId, int forCustody, uvast *seqNum)
{
	SdrObject	 counterObj;
	BundleSeqCounter counter;
	uvast		allocatedSeqNum;
	int		ownTransaction = 0;

	CHKERR(sdr);
	CHKERR(sourceEid);
	CHKERR(seqNum);

	/*	For seqId == 0, destEid is required			*/
	if (seqId == 0 && destEid == NULL)
	{
		putErrmsg("SeqID 0 requires destEid for counter lookup.",
				NULL);
		return -1;
	}

	/*	Start transaction only if not already in one.
	 *	This function may be called from cteb_offer()
	 *	during bpSend() which already has an active
	 *	transaction.					*/
	if (!sdr_in_xn(sdr))
	{
		CHKERR(sdr_begin_xn(sdr));
		ownTransaction = 1;
	}

	counterObj = cbr_getSeqCounter(sdr, sourceEid, destEid, seqId,
			forCustody);
	if (counterObj == 0)
	{
		if (ownTransaction)
		{
			sdr_cancel_xn(sdr);
		}

		putErrmsg("Can't get sequence counter.", sourceEid);
		return -1;
	}

	sdr_stage(sdr, (char *) &counter, counterObj, sizeof(BundleSeqCounter));

	/*	Allocate the next sequence number			*/
	allocatedSeqNum = counter.nextSeqNum;

	/*	Handle wraparound per Section 3.2.9			*/
	if (counter.nextSeqNum >= counter.counterMaxValue)
	{
		counter.nextSeqNum = 0;	/*	Wrap around		*/
	}
	else
	{
		counter.nextSeqNum++;
	}

	counter.lastUsed = getCtime();

	sdr_write(sdr, counterObj, (char *) &counter,
			sizeof(BundleSeqCounter));

	if (ownTransaction)
	{
		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Can't allocate sequence number.", NULL);
			return -1;
		}
	}

	*seqNum = allocatedSeqNum;
	return 0;
}

/*	*	*	Range Compression Functions	*	*	*/

int cbr_extendSequenceEntry(Sdr sdr, SdrObject entryObj,
		BundleSequenceEntry *entry, uvast seqNum)
{
	uvast	expectedNext;
	uvast	gap;

	/*	Calculate expected next sequence number			*/
	if (entry->rangeArray == 0)
	{
		/*	Simple contiguous entry				*/
		expectedNext = entry->seqNumStart + entry->length;

		if (seqNum == expectedNext)
		{
			/*	Consecutive - extend contiguous range	*/
			entry->length++;
			sdr_write(sdr, entryObj, (char *) entry,
					sizeof(BundleSequenceEntry));
			return 1;
		}

		if (seqNum > expectedNext)
		{
			/*	Gap detected - convert to range array	*/
			gap = seqNum - expectedNext;
			return cbr_convertToRangeArray(sdr, entryObj, entry,
					entry->length, gap, 1);
		}

		/*	seqNum < expectedNext: duplicate or out of order */
		return 0;
	}
	else
	{
		/*	Already using range array - extend it		*/
		return cbr_extendRangeArray(sdr, entry->rangeArray,
				entry->seqNumStart, seqNum);
	}
}

int cbr_convertToRangeArray(Sdr sdr, SdrObject entryObj,
		BundleSequenceEntry *entry, uvast currentLen,
		uvast gap, uvast newLen)
{
	SdrObject rangeArray;
	SdrObject lenObj;
	uvast	lenBuf;

	rangeArray = sdr_list_create(sdr);
	if (rangeArray == 0)
	{
		putErrmsg("Can't create range array.", NULL);
		return -1;
	}

	/*	Add: [currentLen, gap, newLen]				*/

	/*	First: included length (current contiguous run)		*/
	lenObj = sdr_malloc(sdr, sizeof(uvast));
	if (lenObj == 0)
	{
		return -1;
	}

	lenBuf = currentLen;
	sdr_write(sdr, lenObj, (char *) &lenBuf, sizeof(uvast));
	sdr_list_insert_last(sdr, rangeArray, lenObj);

	/*	Second: excluded length (gap)				*/
	lenObj = sdr_malloc(sdr, sizeof(uvast));
	if (lenObj == 0)
	{
		return -1;
	}

	lenBuf = gap;
	sdr_write(sdr, lenObj, (char *) &lenBuf, sizeof(uvast));
	sdr_list_insert_last(sdr, rangeArray, lenObj);

	/*	Third: included length (new subsequence)		*/
	lenObj = sdr_malloc(sdr, sizeof(uvast));
	if (lenObj == 0)
	{
		return -1;
	}

	lenBuf = newLen;
	sdr_write(sdr, lenObj, (char *) &lenBuf, sizeof(uvast));
	sdr_list_insert_last(sdr, rangeArray, lenObj);

	/*	Update entry to use range array			*/
	entry->rangeArray = rangeArray;
	entry->length = 0;	/*	No longer used for simple count */
	sdr_write(sdr, entryObj, (char *) entry, sizeof(BundleSequenceEntry));

	return 1;
}

int cbr_extendRangeArray(Sdr sdr, SdrObject rangeArray,
		uvast seqNumStart, uvast seqNum)
{
	SdrObject elt;
	SdrObject lastElt = 0;
	SdrObject lenObj;
	uvast	position = seqNumStart;
	uvast	lenBuf;
	int	included = 1;
	uvast	expectedNext;
	uvast	gap;

	/*	Calculate current position by walking the range array	*/
	for (elt = sdr_list_first(sdr, rangeArray);
			elt; elt = sdr_list_next(sdr, elt))
	{
		lenObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &lenBuf, lenObj, sizeof(uvast));
		position += lenBuf;
		lastElt = elt;
		included = !included;	/*	Alternates		*/
	}

	/*	After walking, position is the expected next and
	 *	included indicates whether we're in an included region.
	 *	(After full walk, we've consumed all ranges, so next
	 *	bundle would extend the last included region or start
	 *	a new gap.)						*/

	/*	Re-read last element (stage for SDR_BOUNDED write)	*/
	lenObj = sdr_list_data(sdr, lastElt);
	sdr_stage(sdr, (char *) &lenBuf, lenObj, sizeof(uvast));

	expectedNext = position;

	if (seqNum == expectedNext)
	{
		/*	Consecutive with last included region		*/
		/*	Last element in range array should be included	*/
		lenBuf++;
		sdr_write(sdr, lenObj, (char *) &lenBuf, sizeof(uvast));
		return 1;
	}

	if (seqNum > expectedNext)
	{
		/*	Gap - add excluded and new included		*/
		gap = seqNum - expectedNext;

		/*	Add excluded (gap) length			*/
		lenObj = sdr_malloc(sdr, sizeof(uvast));
		if (lenObj == 0)
		{
			return -1;
		}

		sdr_write(sdr, lenObj, (char *) &gap, sizeof(uvast));
		sdr_list_insert_last(sdr, rangeArray, lenObj);

		/*	Add included (new single bundle)		*/
		lenObj = sdr_malloc(sdr, sizeof(uvast));
		if (lenObj == 0)
		{
			return -1;
		}

		lenBuf = 1;
		sdr_write(sdr, lenObj, (char *) &lenBuf, sizeof(uvast));
		sdr_list_insert_last(sdr, rangeArray, lenObj);

		return 1;
	}

	/*	seqNum < expectedNext: duplicate or out of order	*/
	return 0;
}

/*	*	*	CBOR Encoding Functions	*	*	*	*/

int	cbr_encodeBundleSequence(uvast seqId, uvast seqNumStart,
		uvast length, uvast *rangeArray, int rangeCount,
		char *sourceEid, char *signalDestEid, char *localAdminEid,
		unsigned char *buffer, size_t buflen)
{
	unsigned char	*cursor = buffer;
	int		includeSourceEid = 1;
	uvast		arrayLen;
	int		i;

	(void) buflen;	/*	Reserved for future bounds checking.	*/

	/*	Determine if source-eid should be included		*/
	/*	Per Orange Book Section 3.3.1, omit if:			*/
	/*	  1. No sourceEid supplied (nothing to emit)		*/
	/*	  2. Source matches signal destination			*/
	/*	  3. Source matches local admin endpoint		*/
	/*	Callers that build a CCS are additionally subject to	*/
	/*	rule 4.2.4 ("shall never contain a block source AEID")	*/
	/*	and must arrange for one of the above to fire; the	*/
	/*	CCS encoder does this by passing the CCS destination as	*/
	/*	the sourceEid argument.					*/
	if (sourceEid == NULL)
	{
		includeSourceEid = 0;
	}

	if (includeSourceEid && signalDestEid)
	{
		if (strcmp(sourceEid, signalDestEid) == 0)
		{
			includeSourceEid = 0;
		}
	}

	if (includeSourceEid && localAdminEid)
	{
		if (strcmp(sourceEid, localAdminEid) == 0)
		{
			includeSourceEid = 0;
		}
	}

	/*	Bundle-Sequence array: [seqId, seqNumStart, len-or-range, ?srcEid] */
	arrayLen = includeSourceEid ? 4 : 3;

	if (cbor_encode_array_open(arrayLen, &cursor) < 1)
	{
		return -1;
	}

	/*	Item 1: seq-id-ref = (uint .gt 0) / eid
	 *	Per Orange Book CDDL: seqId 0 means per-destination mode;
	 *	encode the destination EID instead of integer 0.		*/
	if (seqId == 0)
	{
		int	eidLen;

		if (signalDestEid == NULL)
		{
			putErrmsg("Bundle-Sequence: seqId 0 requires destEid.",
					NULL);
			return -1;
		}

		eidLen = serializeEidString(signalDestEid, cursor);
		if (eidLen < 1)
		{
			putErrmsg("Bundle-Sequence: can't serialize dest EID.",
					signalDestEid);
			return -1;
		}

		cursor += eidLen;
	}
	else
	{
		if (cbor_encode_integer(seqId, &cursor) < 1)
		{
			return -1;
		}
	}

	/*	Item 2: seqNumStart					*/
	if (cbor_encode_integer(seqNumStart, &cursor) < 1)
	{
		return -1;
	}

	/*	Item 3: length-or-range					*/
	if (rangeArray == NULL || rangeCount == 0)
	{
		/*	Contiguous: encode length as uint		*/
		if (cbor_encode_integer(length, &cursor) < 1)
		{
			return -1;
		}
	}
	else
	{
		/*	Non-contiguous: encode as array			*/
		if (cbor_encode_array_open((uvast) rangeCount, &cursor) < 1)
		{
			return -1;
		}

		for (i = 0; i < rangeCount; i++)
		{
			if (cbor_encode_integer(rangeArray[i], &cursor) < 1)
			{
				return -1;
			}
		}
	}

	/*	Item 4 (optional): blk_source (source EID as structured
	 *	eid per Orange Book Annex E).				*/

	if (includeSourceEid && sourceEid)
	{
		int	eidLen;

		eidLen = serializeEidString(sourceEid, cursor);
		if (eidLen < 1)
		{
			putErrmsg("Bundle-Sequence: can't serialize source EID.",
					sourceEid);
			return -1;
		}

		cursor += eidLen;
	}

	return cursor - buffer;
}

int	cbr_decodeBundleSequence(unsigned char **cursor,
		unsigned int *bytesRemaining, uvast *seqId,
		uvast *seqNumStart, uvast *length,
		uvast **rangeArray, int *rangeCount, char **sourceEid,
		char **seqDestEid)
{
	uvast		arrayLen;
	int		majorType;
	uvast		rangeLen;
	int		i;
	char		eidBuf[MAX_EID_LEN];

	*rangeArray = NULL;
	*rangeCount = 0;
	*sourceEid = NULL;
	*seqDestEid = NULL;

	/*	Decode outer array					*/
	arrayLen = 0;
	if (cbor_decode_array_open(&arrayLen, cursor, bytesRemaining) < 1)
	{
		return -1;
	}

	if (arrayLen < 3)
	{
		putErrmsg("Bundle-Sequence array too short.", NULL);
		return -1;
	}

	/*	Item 1: seq-id-ref = (uint .gt 0) / eid
	 *	If the CBOR item is an array, it is a per-destination EID
	 *	(seqId == 0 mode); otherwise it is the integer seqId.	*/
	majorType = (**cursor >> 5) & 0x07;
	if (majorType == 4)	/*	CborArray: per-destination EID	*/
	{
		if (acquireEidString(eidBuf, sizeof eidBuf, cursor,
				bytesRemaining) < 1)
		{
			return -1;
		}

		*seqId = 0;
		*seqDestEid = MTAKE(strlen(eidBuf) + 1);
		if (*seqDestEid == NULL)
		{
			putErrmsg("No memory for seq-dest EID.", NULL);
			return -1;
		}

		istrcpy(*seqDestEid, eidBuf, strlen(eidBuf) + 1);
	}
	else
	{
		if (cbor_decode_integer(seqId, CborAny, cursor,
				bytesRemaining) < 1)
		{
			return -1;
		}
	}

	/*	Item 2: seqNumStart					*/
	if (cbor_decode_integer(seqNumStart, CborAny, cursor, bytesRemaining) < 1)
	{
		return -1;
	}

	/*	Item 3: length-or-range					*/
	/*	Peek at major type to determine if uint or array	*/
	/*	CBOR major type 0 = unsigned int, major type 4 = array	*/
	majorType = (**cursor >> 5) & 0x07;

	if (majorType == 0)	/*	CborUnsignedInteger		*/
	{
		/*	Contiguous: single length			*/
		if (cbor_decode_integer(length, CborAny, cursor,
				bytesRemaining) < 1)
		{
			return -1;
		}
	}
	else if (majorType == 4)	/*	CborArray		*/
	{
		/*	Non-contiguous: range array			*/
		rangeLen = 0;
		if (cbor_decode_array_open(&rangeLen, cursor,
				bytesRemaining) < 1)
		{
			return -1;
		}

		/*	Each range element is at least one byte on the wire,
		 *	so a range count exceeding the bytes remaining is
		 *	malformed.  Reject it before sizing the allocation:
		 *	on 32-bit builds *rangeCount * sizeof(uvast) would
		 *	otherwise overflow, under-allocate, and be written
		 *	past by the decode loop below.				*/
		if (rangeLen > (uvast) *bytesRemaining)
		{
			return -1;
		}

		*rangeCount = (int) rangeLen;
		*rangeArray = MTAKE(*rangeCount * sizeof(uvast));
		if (*rangeArray == NULL)
		{
			putErrmsg("No memory for range array.", NULL);
			return -1;
		}

		for (i = 0; i < *rangeCount; i++)
		{
			if (cbor_decode_integer(&((*rangeArray)[i]),
					CborAny, cursor, bytesRemaining) < 1)
			{
				MRELEASE(*rangeArray);
				*rangeArray = NULL;
				*rangeCount = 0;
				return -1;
			}
		}

		*length = 0;	/*	Indicate non-contiguous		*/
	}
	else
	{
		putErrmsg("Invalid Bundle-Sequence item 3 type.", NULL);
		return -1;
	}

	/*	Item 4 (optional): blk_source (source EID as structured
	 *	eid per Orange Book Annex E).				*/

	if (arrayLen >= 4)
	{
		if (acquireEidString(eidBuf, sizeof eidBuf, cursor,
				bytesRemaining) < 1)
		{
			return -1;
		}

		*sourceEid = MTAKE(strlen(eidBuf) + 1);
		if (*sourceEid == NULL)
		{
			putErrmsg("No memory for source EID.", NULL);
			return -1;
		}

		istrcpy(*sourceEid, eidBuf, strlen(eidBuf) + 1);
	}

	return 0;
}

/*	*	*	Signal Management Functions	*	*	*/

SdrObject cbr_findOrCreatePendingSignal(Sdr sdr, char *destEid,
		int signalType, int statusOrDispCode)
{
	CbrDb		*cbrConstants;
	SdrObject	signalList;
	SdrObject	elt;
	SdrObject	signalObj;
	PendingSignal	signal;
	char		destEidBuf[MAX_EID_LEN];

	cbrConstants = _cbrConstants();
	CHKZERO(cbrConstants);

	/*	Select appropriate pending signal list			*/
	if (signalType == CBR_SIGNAL_CRS)
	{
		signalList = cbrConstants->pendingCrs;
	}
	else if (signalType == CBR_SIGNAL_CCS)
	{
		signalList = cbrConstants->pendingCcs;
	}
	else
	{
		putErrmsg("Invalid signal type.", itoa(signalType));
		return 0;
	}

	/*	Search for existing matching pending signal		*/
	for (elt = sdr_list_first(sdr, signalList);
			elt; elt = sdr_list_next(sdr, elt))
	{
		signalObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &signal, signalObj,
				sizeof(PendingSignal));

		sdr_string_read(sdr, destEidBuf, signal.destEid);
		if (strcmp(destEidBuf, destEid) != 0)
		{
			continue;
		}

		if (signalType == CBR_SIGNAL_CRS &&
				signal.statusCode == statusOrDispCode)
		{
			return elt;
		}

		if (signalType == CBR_SIGNAL_CCS &&
				signal.dispCode == statusOrDispCode)
		{
			return elt;
		}
	}

	/*	Not found - create new pending signal			*/
	memset(&signal, 0, sizeof(PendingSignal));
	signal.destEid = sdr_string_create(sdr, destEid);
	signal.signalType = signalType;
	signal.statusCode = (signalType == CBR_SIGNAL_CRS) ?
			statusOrDispCode : 0;
	signal.dispCode = (signalType == CBR_SIGNAL_CCS) ?
			statusOrDispCode : 0;
	signal.sequences = sdr_list_create(sdr);
	signal.aggregateStart = getCtime();
	signal.bundleCount = 0;

	signalObj = sdr_malloc(sdr, sizeof(PendingSignal));
	if (signalObj == 0)
	{
		putErrmsg("No space for pending signal.", NULL);
		return 0;
	}

	sdr_write(sdr, signalObj, (char *) &signal, sizeof(PendingSignal));
	elt = sdr_list_insert_last(sdr, signalList, signalObj);

	return elt;
}

int cbr_addToSignalSequences(Sdr sdr, SdrObject signalElt,
		char *sourceEid, uvast seqId, uvast seqNum)
{
	SdrObject		signalObj;
	PendingSignal		signal;
	SdrObject		seqElt;
	SdrObject		entryObj;
	BundleSequenceEntry	entry;
	char			srcEidBuf[MAX_EID_LEN];
	int			extended;

	signalObj = sdr_list_data(sdr, signalElt);
	sdr_stage(sdr, (char *) &signal, signalObj, sizeof(PendingSignal));

	/*	Search for existing entry with matching sourceEid and seqId */
	for (seqElt = sdr_list_first(sdr, signal.sequences);
			seqElt; seqElt = sdr_list_next(sdr, seqElt))
	{
		entryObj = sdr_list_data(sdr, seqElt);
		sdr_stage(sdr, (char *) &entry, entryObj,
				sizeof(BundleSequenceEntry));

		if (entry.seqId != seqId)
		{
			continue;
		}

		sdr_string_read(sdr, srcEidBuf, entry.sourceEid);
		if (strcmp(srcEidBuf, sourceEid) != 0)
		{
			continue;
		}

		/*	Found matching entry - try to extend		*/
		extended = cbr_extendSequenceEntry(sdr, entryObj, &entry,
				seqNum);
		if (extended > 0)
		{
			/*	Successfully extended			*/
			signal.bundleCount++;
			sdr_write(sdr, signalObj, (char *) &signal,
					sizeof(PendingSignal));
			return 0;
		}
		else if (extended < 0)
		{
			return -1;	/*	Error			*/
		}

		/*	extended == 0: couldn't extend, fall through	*/
		/*	to create new entry (out-of-order seqNum)	*/
	}

	/*	Create new sequence entry				*/
	memset(&entry, 0, sizeof(BundleSequenceEntry));
	entry.seqId = seqId;
	entry.seqNumStart = seqNum;
	entry.length = 1;
	entry.rangeArray = 0;
	entry.sourceEid = sdr_string_create(sdr, sourceEid);

	entryObj = sdr_malloc(sdr, sizeof(BundleSequenceEntry));
	if (entryObj == 0)
	{
		putErrmsg("No space for sequence entry.", NULL);
		return -1;
	}

	sdr_write(sdr, entryObj, (char *) &entry, sizeof(BundleSequenceEntry));
	sdr_list_insert_last(sdr, signal.sequences, entryObj);

	signal.bundleCount++;
	sdr_write(sdr, signalObj, (char *) &signal, sizeof(PendingSignal));

	return 0;
}

/*	*	*	Signal Flush/Transmit Functions	*	*	*/

int	cbr_flushSignals(Sdr sdr, int signalType)
{
	CbrDb	*cbrConstants;
	SdrObject elt;
	SdrObject nextElt;
	int	count = 0;
	int	result;

	CHKERR(sdr);
	cbrConstants = _cbrConstants();
	CHKERR(cbrConstants);

	CHKERR(sdr_begin_xn(sdr));

	/*	Flush CRS signals if requested				*/
	if (signalType == CBR_SIGNAL_CRS || signalType == CBR_SIGNAL_ALL)
	{
		for (elt = sdr_list_first(sdr, cbrConstants->pendingCrs);
				elt; elt = nextElt)
		{
			nextElt = sdr_list_next(sdr, elt);
			result = cbr_transmitSignal(sdr, elt);
			if (result < 0)
			{
				sdr_cancel_xn(sdr);
				return -1;
			}

			count++;
		}
	}

	/*	Flush CCS signals if requested				*/
	if (signalType == CBR_SIGNAL_CCS || signalType == CBR_SIGNAL_ALL)
	{
		for (elt = sdr_list_first(sdr, cbrConstants->pendingCcs);
				elt; elt = nextElt)
		{
			nextElt = sdr_list_next(sdr, elt);
			result = cbr_transmitSignal(sdr, elt);
			if (result < 0)
			{
				sdr_cancel_xn(sdr);
				return -1;
			}

			count++;
		}
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't flush CBR signals.", NULL);
		return -1;
	}

	return count;
}

/*	Reads the aggregation limits straight from the SDR rather than the
 *	cached _cbrConstants() snapshot, so that a runtime "m cbraggr" change
 *	takes effect in already-running daemons (notably bpclock, which caches
 *	its snapshot at startup).  Must be called within an SDR transaction;
 *	any output pointer may be NULL.					*/
static void	cbr_liveAggregateConfig(Sdr sdr, unsigned int *crsLimit,
			unsigned int *ccsLimit, unsigned int *timeoutSec)
{
	SdrObject cbrDbObj = _cbrDbObject(NULL);
	CbrDb	cbrBuf;

	if (cbrDbObj == 0)
	{
		return;
	}

	sdr_read(sdr, (char *) &cbrBuf, cbrDbObj, sizeof(CbrDb));
	if (crsLimit)
	{
		*crsLimit = cbrBuf.crsAggregateLimit;
	}

	if (ccsLimit)
	{
		*ccsLimit = cbrBuf.ccsAggregateLimit;
	}

	if (timeoutSec)
	{
		*timeoutSec = cbrBuf.aggregateTimeoutSec;
	}
}

int	cbr_processTimeouts(Sdr sdr)
{
	CbrDb		*cbrConstants;
	unsigned int	timeoutSec;
	time_t		now;
	SdrObject	elt;
	SdrObject	nextElt;
	SdrObject	signalObj;
	PendingSignal	signal;
	SdrObject	cbObj;
	CustodyBundle	cb;
	int		count = 0;

	CHKERR(sdr);
	cbrConstants = _cbrConstants();
	if (cbrConstants == NULL)
	{
		/*	CBR not initialized; nothing to process.	*/
		return 0;
	}

	now = getCtime();

	CHKERR(sdr_begin_xn(sdr));

	/*	Use the live timeout so a runtime "m cbraggr" is honored.	*/
	timeoutSec = cbrConstants->aggregateTimeoutSec;
	cbr_liveAggregateConfig(sdr, NULL, NULL, &timeoutSec);

	/*	Check CRS timeouts					*/
	for (elt = sdr_list_first(sdr, cbrConstants->pendingCrs);
			elt; elt = nextElt)
	{
		nextElt = sdr_list_next(sdr, elt);
		signalObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &signal, signalObj,
				sizeof(PendingSignal));

		if (now - signal.aggregateStart >= (time_t) timeoutSec)
		{
			if (cbr_transmitSignal(sdr, elt) == 0)
			{
				count++;
			}
		}
	}

	/*	Check CCS timeouts					*/
	for (elt = sdr_list_first(sdr, cbrConstants->pendingCcs);
			elt; elt = nextElt)
	{
		nextElt = sdr_list_next(sdr, elt);
		signalObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &signal, signalObj,
				sizeof(PendingSignal));

		if (now - signal.aggregateStart >= (time_t) timeoutSec)
		{
			if (cbr_transmitSignal(sdr, elt) == 0)
			{
				count++;
			}
		}
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't process CBR timeouts.", NULL);
		return -1;
	}

	/*	Custody retransmit timer walk (CBR_RETX_TIMER only).	*/

	if (cbrConstants->retransmitStrategy != CBR_RETX_TIMER)
	{
		return count;
	}

	CHKERR(sdr_begin_xn(sdr));

	for (elt = sdr_list_first(sdr, cbrConstants->custodyBundles);
			elt; elt = nextElt)
	{
		nextElt = sdr_list_next(sdr, elt);
		cbObj = sdr_list_data(sdr, elt);
		sdr_stage(sdr, (char *) &cb, cbObj, sizeof(CustodyBundle));

		if (cb.bundleObj == 0)
		{
			continue;
		}

		if (cbrConstants->maxRetransmissions > 0
		&& (unsigned int) cb.retransmitCount >= cbrConstants->maxRetransmissions)
		{
			continue;
		}

		if (now - cb.lastTransmit
				< (time_t) cbrConstants->retransmitIntervalSec)
		{
			continue;
		}

		cb.lastTransmit = now;
		cb.retransmitCount++;
		sdr_write(sdr, cbObj, (char *) &cb, sizeof(CustodyBundle));

		if (bpReforwardBundle(cb.bundleObj) < 0)
		{
			putErrmsg("CBR: Failed to reforward timed-out bundle.",
					NULL);
			sdr_cancel_xn(sdr);
			return -1;
		}

		writeMemoNote("[i] CBR: Timer-triggered retransmit, count",
				itoa(cb.retransmitCount));
		count++;
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't process CBR retransmit timeouts.", NULL);
		return -1;
	}

	return count;
}

int cbr_doRetransmit(Sdr sdr, CustodyBundle *cb, SdrObject cbElt)
{
	SdrObject cbObj;

	CHKERR(cb);
	CHKERR(cbElt);

	if (cb->bundleObj == 0)
	{
		return 0;	/*	Bundle already expired; skip.	*/
	}

	cbObj = sdr_list_data(sdr, cbElt);
	cb->lastTransmit = getCtime();
	cb->retransmitCount++;
	sdr_write(sdr, cbObj, (char *) cb, sizeof(CustodyBundle));

	if (bpReforwardBundle(cb->bundleObj) < 0)
	{
		putErrmsg("CBR: Failed to reforward bundle.", NULL);
		return -1;
	}

	return 0;
}

/*	Maximum buffer size for CRS/CCS encoding.			*/
#define CBR_MAX_SIGNAL_SIZE	8192

int cbr_encodeCrs(Sdr sdr, SdrObject signalElt, unsigned char *buffer,
		size_t buflen)
{
	SdrObject		signalObj;
	PendingSignal		signal;
	unsigned char		*cursor = buffer;
	uvast			uvtemp;
	SdrObject		seqElt;
	SdrObject		entryObj;
	BundleSequenceEntry	entry;
	char			srcEidBuf[MAX_EID_LEN];
	char			destEidBuf[MAX_EID_LEN];
	int			seqBytes;
	uvast			*rangeData = NULL;
	int			rangeCount = 0;
	SdrObject		rangeElt;
	int			i;

	(void) buflen;		/*	Reserved for future use.	*/

	signalObj = sdr_list_data(sdr, signalElt);
	sdr_read(sdr, (char *) &signal, signalObj, sizeof(PendingSignal));
	sdr_string_read(sdr, destEidBuf, signal.destEid);

	/*	CRS format per Orange Book Section 6:			*/
	/*	Admin Record: [record-type, CRS-content]		*/
	/*	CRS-content: #6.14({ status-reason => Bundle-Sequence-Collection })
	 *
	 *	For simplicity, we encode as:
	 *	[14, {status-reason => [Bundle-Sequence, ...]}]
	 */

	/*	Admin record array open (size 2)			*/
	uvtemp = 2;
	oK(cbor_encode_array_open(uvtemp, &cursor));

	/*	Admin record type = 14 (CRS)				*/
	uvtemp = CBR_ADMIN_RECORD_CRS;
	oK(cbor_encode_integer(uvtemp, &cursor));

	/*	CRS content: CBOR tag #6.14 followed by map		*/
	/*	For now, skip the tag and just encode the map		*/

	/*	Map with 1 entry: status-reason => Bundle-Sequence-Collection */
	uvtemp = 1;
	oK(cbor_encode_map_open(uvtemp, &cursor));

	/*	Key: status-reason code					*/
	uvtemp = signal.statusCode;
	oK(cbor_encode_integer(uvtemp, &cursor));

	/*	Value: array of Bundle-Sequence				*/
	uvtemp = sdr_list_length(sdr, signal.sequences);
	oK(cbor_encode_array_open(uvtemp, &cursor));

	/*	Encode each Bundle-Sequence entry			*/
	for (seqElt = sdr_list_first(sdr, signal.sequences);
			seqElt; seqElt = sdr_list_next(sdr, seqElt))
	{
		entryObj = sdr_list_data(sdr, seqElt);
		sdr_read(sdr, (char *) &entry, entryObj,
				sizeof(BundleSequenceEntry));
		sdr_string_read(sdr, srcEidBuf, entry.sourceEid);

		/*	Build range array if non-contiguous		*/
		if (entry.rangeArray != 0)
		{
			rangeCount = sdr_list_length(sdr, entry.rangeArray);
			rangeData = MTAKE(rangeCount * sizeof(uvast));
			if (rangeData == NULL)
			{
				return -1;
			}

			i = 0;
			for (rangeElt = sdr_list_first(sdr, entry.rangeArray);
					rangeElt;
					rangeElt = sdr_list_next(sdr, rangeElt))
			{
				SdrObject lenObj = sdr_list_data(sdr, rangeElt);

				sdr_read(sdr, (char *) &rangeData[i], lenObj,
						sizeof(uvast));
				i++;
			}
		}
		else
		{
			rangeData = NULL;
			rangeCount = 0;
		}

		seqBytes = cbr_encodeBundleSequence(entry.seqId,
				entry.seqNumStart, entry.length,
				rangeData, rangeCount,
				srcEidBuf, destEidBuf, NULL,
				cursor, buflen - (cursor - buffer));

		if (rangeData)
		{
			MRELEASE(rangeData);
			rangeData = NULL;
		}

		if (seqBytes < 0)
		{
			return -1;
		}

		cursor += seqBytes;
	}

	return cursor - buffer;
}

int cbr_encodeCcs(Sdr sdr, SdrObject signalElt, unsigned char *buffer,
		size_t buflen)
{
	SdrObject		signalObj;
	PendingSignal		signal;
	unsigned char		*cursor = buffer;
	uvast			uvtemp;
	SdrObject		seqElt;
	SdrObject		entryObj;
	BundleSequenceEntry	entry;
	char			destEidBuf[MAX_EID_LEN];
	int			seqBytes;
	uvast			*rangeData = NULL;
	int			rangeCount = 0;
	SdrObject		rangeElt;
	int			i;

	(void) buflen;		/*	Reserved for future use.	*/

	signalObj = sdr_list_data(sdr, signalElt);
	sdr_read(sdr, (char *) &signal, signalObj, sizeof(PendingSignal));
	sdr_string_read(sdr, destEidBuf, signal.destEid);

	/*	CCS format per Orange Book Section 5:
	 *	Admin Record: [record-type, CCS-content]
	 *	CCS-content: #6.13({ disposition => Bundle-Sequence-Collection })
	 *
	 *	For simplicity, we encode as:
	 *	[13, {disposition => [Bundle-Sequence, ...]}]
	 *
	 *	Disposition is SIGNED: 1=accepted, -1=refused
	 */

	/*	Admin record array open (size 2)			*/
	uvtemp = 2;
	oK(cbor_encode_array_open(uvtemp, &cursor));

	/*	Admin record type = 13 (CCS)				*/
	uvtemp = CBR_ADMIN_RECORD_CCS;
	oK(cbor_encode_integer(uvtemp, &cursor));

	/*	CCS content: CBOR tag #6.13 followed by map		*/
	/*	For now, skip the tag and just encode the map		*/

	/*	Map with 1 entry: disposition => Bundle-Sequence-Collection */
	uvtemp = 1;
	oK(cbor_encode_map_open(uvtemp, &cursor));

	/*	Key: disposition code (SIGNED integer)
	 *	Per Orange Book: 1=accepted, -1=refused			*/
	oK(cbor_encode_signed_int(signal.dispCode, &cursor));

	/*	Value: array of Bundle-Sequence				*/
	uvtemp = sdr_list_length(sdr, signal.sequences);
	oK(cbor_encode_array_open(uvtemp, &cursor));

	/*	Encode each Bundle-Sequence entry.
	 *
	 *	Orange Book 4.2.4: the bundle sequences carried in a CCS
	 *	shall never contain a block source AEID.  We therefore
	 *	skip reading entry.sourceEid and pass NULL for sourceEid
	 *	below, so the encoder unconditionally emits a 3-element
	 *	BSC regardless of what is stored on the sequence entry.	*/
	for (seqElt = sdr_list_first(sdr, signal.sequences);
			seqElt; seqElt = sdr_list_next(sdr, seqElt))
	{
		entryObj = sdr_list_data(sdr, seqElt);
		sdr_read(sdr, (char *) &entry, entryObj,
				sizeof(BundleSequenceEntry));

		/*	Build range array if non-contiguous		*/
		if (entry.rangeArray != 0)
		{
			rangeCount = sdr_list_length(sdr, entry.rangeArray);
			rangeData = MTAKE(rangeCount * sizeof(uvast));
			if (rangeData == NULL)
			{
				return -1;
			}

			i = 0;
			for (rangeElt = sdr_list_first(sdr, entry.rangeArray);
					rangeElt;
					rangeElt = sdr_list_next(sdr, rangeElt))
			{
				SdrObject lenObj = sdr_list_data(sdr, rangeElt);

				sdr_read(sdr, (char *) &rangeData[i], lenObj,
						sizeof(uvast));
				i++;
			}
		}
		else
		{
			rangeData = NULL;
			rangeCount = 0;
		}

		seqBytes = cbr_encodeBundleSequence(entry.seqId,
				entry.seqNumStart, entry.length,
				rangeData, rangeCount,
				NULL, destEidBuf, NULL,
				cursor, buflen - (cursor - buffer));

		if (rangeData)
		{
			MRELEASE(rangeData);
			rangeData = NULL;
		}

		if (seqBytes < 0)
		{
			return -1;
		}

		cursor += seqBytes;
	}

	return cursor - buffer;
}

static void cleanupSignal(Sdr sdr, SdrObject signalElt)
{
	SdrObject		signalObj;
	PendingSignal		signal;
	SdrObject		seqElt;
	SdrObject		entryObj;
	BundleSequenceEntry	entry;

	signalObj = sdr_list_data(sdr, signalElt);
	sdr_read(sdr, (char *) &signal, signalObj, sizeof(PendingSignal));

	/*	Clean up sequence entries				*/
	while ((seqElt = sdr_list_first(sdr, signal.sequences)) != 0)
	{
		entryObj = sdr_list_data(sdr, seqElt);
		sdr_read(sdr, (char *) &entry, entryObj,
				sizeof(BundleSequenceEntry));

		/*	Free range array if present			*/
		if (entry.rangeArray != 0)
		{
			SdrObject rangeElt;
			SdrObject nextRangeElt;

			for (rangeElt = sdr_list_first(sdr, entry.rangeArray);
					rangeElt; rangeElt = nextRangeElt)
			{
				nextRangeElt = sdr_list_next(sdr, rangeElt);
				sdr_free(sdr, sdr_list_data(sdr, rangeElt));
				sdr_list_delete(sdr, rangeElt, NULL, NULL);
			}

			sdr_list_destroy(sdr, entry.rangeArray, NULL, NULL);
		}

		sdr_free(sdr, entry.sourceEid);
		sdr_free(sdr, entryObj);
		sdr_list_delete(sdr, seqElt, NULL, NULL);
	}

	/*	Clean up signal structure				*/
	sdr_list_destroy(sdr, signal.sequences, NULL, NULL);
	sdr_free(sdr, signal.destEid);
	sdr_free(sdr, signalObj);
	sdr_list_delete(sdr, signalElt, NULL, NULL);
}

int cbr_transmitSignal(Sdr sdr, SdrObject signalElt)
{
	SdrObject		signalObj;
	PendingSignal		signal;
	char			destEidBuf[MAX_EID_LEN];
	unsigned char		*buffer;
	int			buflen;
	SdrObject		payloadZco;
	BpAncillaryData		ancillary = { 0 };
	int			result;

	signalObj = sdr_list_data(sdr, signalElt);
	sdr_read(sdr, (char *) &signal, signalObj, sizeof(PendingSignal));
	sdr_string_read(sdr, destEidBuf, signal.destEid);

	/*	Allocate buffer for signal encoding			*/
	buffer = MTAKE(CBR_MAX_SIGNAL_SIZE);
	if (buffer == NULL)
	{
		putErrmsg("No memory for CRS encoding buffer.", NULL);
		return -1;
	}

	/*	Encode the signal based on type				*/
	if (signal.signalType == CBR_SIGNAL_CRS)
	{
		buflen = cbr_encodeCrs(sdr, signalElt, buffer,
				CBR_MAX_SIGNAL_SIZE);
	}
	else if (signal.signalType == CBR_SIGNAL_CCS)
	{
		buflen = cbr_encodeCcs(sdr, signalElt, buffer,
				CBR_MAX_SIGNAL_SIZE);
	}
	else
	{
		writeMemo("[?] Unknown CBR signal type.");
		MRELEASE(buffer);
		cleanupSignal(sdr, signalElt);
		return -1;
	}

	if (buflen < 0)
	{
		putErrmsg("Can't encode CBR signal.", NULL);
		MRELEASE(buffer);
		return -1;
	}

	/*	Create ZCO from encoded buffer				*/
	payloadZco = zco_create(sdr, ZcoSdrSource,
			sdr_insert(sdr, (char *) buffer, buflen),
			0, buflen, ZcoOutbound);
	MRELEASE(buffer);

	if (payloadZco == 0 || payloadZco == (SdrObject) ERROR)
	{
		putErrmsg("Can't create ZCO for CBR signal.", NULL);
		return -1;
	}

	/*	Send as admin bundle					*/
	result = bpSend(NULL, destEidBuf, NULL, 3600, BP_STD_PRIORITY,
			NoCustodyRequested, 0, 0, &ancillary, payloadZco,
			NULL, signal.signalType == CBR_SIGNAL_CRS ?
				CBR_ADMIN_RECORD_CRS : CBR_ADMIN_RECORD_CCS);

	if (result < 0)
	{
		putErrmsg("Can't send CBR signal bundle.", destEidBuf);
		/*	ZCO already released by bpSend on failure	*/
	}
	else
	{
		SdrObject cbrDbObj = getCbrDbObject();

		writeMemoNote(signal.signalType == CBR_SIGNAL_CRS ?
				"[i] CBR CRS transmitted to" :
				"[i] CBR CCS transmitted to", destEidBuf);

		/*	Increment statistics counters.			*/
		if (cbrDbObj)
		{
			CbrDb	cbrDb;

			sdr_stage(sdr, (char *) &cbrDb, cbrDbObj,
					sizeof(CbrDb));
			if (signal.signalType == CBR_SIGNAL_CRS)
			{
				cbrDb.crsSignalsSent++;
			}
			else if (signal.signalType == CBR_SIGNAL_CCS)
			{
				if (signal.dispCode == CBR_CUSTODY_ACCEPTED)
				{
					cbrDb.ccsAcceptSent++;
				}
				else
				{
					cbrDb.ccsRefuseSent++;
				}
			}

			sdr_write(sdr, cbrDbObj, (char *) &cbrDb,
					sizeof(CbrDb));
		}
	}

	/*	Clean up signal structure				*/
	cleanupSignal(sdr, signalElt);

	return (result < 0) ? -1 : 0;
}

/*	*	*	Status Reporting Functions	*	*	*/

int	cbr_reportStatus(Sdr sdr, Bundle *bundle, int statusReason,
		CrebBlk *creb)
{
	char		*reportToEid = NULL;
	char		*sourceEid = NULL;
	uvast		seqId;
	uvast		seqNum;
	SdrObject	signalElt;
	unsigned int	crsAggregateLimit;
	PendingSignal	signal;
	SdrObject	signalObj;
	int		mustFreeReportTo = 0;
	int		mustFreeSource = 0;

	CHKERR(sdr);
	CHKERR(bundle);

	/*	Determine report destination				*/
	if (creb && creb->reportToEid)
	{
		reportToEid = creb->reportToEid;
	}
	else
	{
		/*	Use bundle's report-to if no CREB report-to	*/
		readEid(&bundle->reportTo, &reportToEid);
		if (reportToEid)
		{
			mustFreeReportTo = 1;
		}
	}

	if (reportToEid == NULL)
	{
		/*	No report destination specified			*/
		return 0;
	}

	/*	Get sequence info from CREB				*/
	if (creb)
	{
		seqId = creb->seqId;
		seqNum = creb->seqNum;

		/*	Get effective source EID			*/
		if (creb->sourceEid)
		{
			sourceEid = creb->sourceEid;
		}
		else
		{
			/*	Use bundle source as default		*/
			readEid(&bundle->id.source, &sourceEid);
			if (sourceEid)
			{
				mustFreeSource = 1;
			}
		}
	}
	else
	{
		/*	No CREB - can't send CRS			*/
		if (mustFreeReportTo)
		{
			MRELEASE(reportToEid);
		}

		return 0;
	}

	if (sourceEid == NULL)
	{
		if (mustFreeReportTo)
		{
			MRELEASE(reportToEid);
		}

		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));

	/*	Find or create pending signal for this destination/status */
	signalElt = cbr_findOrCreatePendingSignal(sdr, reportToEid,
			CBR_SIGNAL_CRS, statusReason);
	if (signalElt == 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't get pending CRS signal.", NULL);
		if (mustFreeReportTo)
		{
			MRELEASE(reportToEid);
		}

		if (mustFreeSource)
		{
			MRELEASE(sourceEid);
		}

		return -1;
	}

	/*	Add bundle to signal sequences				*/
	if (cbr_addToSignalSequences(sdr, signalElt, sourceEid, seqId,
			seqNum) < 0)
	{
		sdr_cancel_xn(sdr);
		if (mustFreeReportTo)
		{
			MRELEASE(reportToEid);
		}

		if (mustFreeSource)
		{
			MRELEASE(sourceEid);
		}

		return -1;
	}

	/*	Check if aggregation limit reached.  Read the limit live so a
	 *	runtime "m cbraggr" is honored without a restart.		*/
	crsAggregateLimit = 0;
	cbr_liveAggregateConfig(sdr, &crsAggregateLimit, NULL, NULL);
	signalObj = sdr_list_data(sdr, signalElt);
	sdr_read(sdr, (char *) &signal, signalObj, sizeof(PendingSignal));

	if (crsAggregateLimit > 0 &&
			signal.bundleCount >= crsAggregateLimit)
	{
		if (cbr_transmitSignal(sdr, signalElt) < 0)
		{
			sdr_cancel_xn(sdr);
			if (mustFreeReportTo)
			{
				MRELEASE(reportToEid);
			}

			if (mustFreeSource)
			{
				MRELEASE(sourceEid);
			}

			return -1;
		}
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't report bundle status.", NULL);
		if (mustFreeReportTo)
		{
			MRELEASE(reportToEid);
		}

		if (mustFreeSource)
		{
			MRELEASE(sourceEid);
		}

		return -1;
	}

	if (mustFreeReportTo)
	{
		MRELEASE(reportToEid);
	}

	if (mustFreeSource)
	{
		MRELEASE(sourceEid);
	}

	return 0;
}

/*	*	*	Custody Transfer Functions	*	*	*/

/*	Internal helpers for custody bundle tracking.			*/

/**
 * Find a custody bundle by source EID, seqId, and seqNum.
 *
 * @return	SDR list element of CustodyBundle, 0 if not found
 */
SdrObject cbr_findCustodyBundle(Sdr sdr, char *sourceEid, uvast seqId,
		uvast seqNum)
{
	CbrDb		*cbrConstants;
	SdrObject	elt;
	SdrObject	cbObj;
	CustodyBundle	cb;
	char		eidBuf[MAX_EID_LEN];

	if (sourceEid == NULL)
	{
		/*	The (sourceEid, seqId, seqNum) tuple is the
		 *	custody-tracking key; NULL is not a valid value.
		 *	Defensive guard: a future caller that omits the
		 *	BSC source EID after Orange Book 4.2.4 must first
		 *	substitute the local admin EID.			*/
		return 0;
	}

	cbrConstants = getCbrConstants();
	if (cbrConstants == NULL)
	{
		return 0;
	}

	for (elt = sdr_list_first(sdr, cbrConstants->custodyBundles);
			elt; elt = sdr_list_next(sdr, elt))
	{
		cbObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &cb, cbObj, sizeof(CustodyBundle));

		if (cb.seqId == seqId && cb.seqNum == seqNum)
		{
			/*	Check source EID match.			*/
			sdr_string_read(sdr, eidBuf, cb.sourceEid);
			if (strcmp(eidBuf, sourceEid) == 0)
			{
				return elt;
			}
		}
	}

	return 0;
}

/**
 * Add a bundle to custody tracking.
 *
 * @return	SDR list element of new CustodyBundle, 0 on error
 */
SdrObject cbr_trackCustodyBundle(Sdr sdr, SdrObject bundleObj, char *destEid,
		char *sourceEid, uvast seqId, uvast seqNum)
{
	CbrDb		*cbrConstants;
	CustodyBundle	cb;
	SdrObject	cbObj;
	SdrObject	elt;

	cbrConstants = getCbrConstants();
	if (cbrConstants == NULL)
	{
		putErrmsg("CBR: Can't get CBR constants.", NULL);
		return 0;
	}

	/*	Check if already tracked (shouldn't happen).		*/
	if (cbr_findCustodyBundle(sdr, sourceEid, seqId, seqNum) != 0)
	{
		writeMemo("[?] CBR: Bundle already in custody tracking.");
		return 0;
	}

	/*	Create CustodyBundle entry.				*/
	memset(&cb, 0, sizeof(CustodyBundle));
	cb.bundleObj = bundleObj;
	cb.seqId = seqId;
	cb.seqNum = seqNum;
	cb.custodyAccepted = getCtime();
	cb.lastTransmit = cb.custodyAccepted;
	cb.retransmitCount = 0;

	cb.destEid = sdr_string_create(sdr, destEid);
	if (cb.destEid == 0)
	{
		putErrmsg("CBR: Can't store dest EID.", NULL);
		return 0;
	}

	cb.sourceEid = sdr_string_create(sdr, sourceEid);
	if (cb.sourceEid == 0)
	{
		sdr_free(sdr, cb.destEid);
		putErrmsg("CBR: Can't store source EID.", NULL);
		return 0;
	}

	/*	Store in SDR.						*/
	cbObj = sdr_malloc(sdr, sizeof(CustodyBundle));
	if (cbObj == 0)
	{
		sdr_free(sdr, cb.destEid);
		sdr_free(sdr, cb.sourceEid);
		putErrmsg("CBR: Can't allocate CustodyBundle.", NULL);
		return 0;
	}

	sdr_write(sdr, cbObj, (char *) &cb, sizeof(CustodyBundle));

	/*	Add to custody list.					*/
	elt = sdr_list_insert_last(sdr, cbrConstants->custodyBundles, cbObj);
	if (elt == 0)
	{
		sdr_free(sdr, cb.destEid);
		sdr_free(sdr, cb.sourceEid);
		sdr_free(sdr, cbObj);
		putErrmsg("CBR: Can't add to custody list.", NULL);
		return 0;
	}

	return elt;
}

void	cbr_noteCustodyOriginated(Sdr sdr)
{
	SdrObject cbrDbObj = getCbrDbObject();

	if (cbrDbObj)
	{
		CbrDb	cbrDb;

		sdr_stage(sdr, (char *) &cbrDb, cbrDbObj, sizeof(CbrDb));
		cbrDb.custodyOriginated++;
		sdr_write(sdr, cbrDbObj, (char *) &cbrDb, sizeof(CbrDb));
	}
}

/**
 * Remove a bundle from custody tracking.
 */
void cbr_untrackCustodyBundle(Sdr sdr, SdrObject custodyElt)
{
	SdrObject	cbObj;
	CustodyBundle	cb;

	if (custodyElt == 0)
	{
		return;
	}

	cbObj = sdr_list_data(sdr, custodyElt);
	sdr_read(sdr, (char *) &cb, cbObj, sizeof(CustodyBundle));

	/*	Free string fields.					*/
	if (cb.destEid != 0)
	{
		sdr_free(sdr, cb.destEid);
	}

	if (cb.sourceEid != 0)
	{
		sdr_free(sdr, cb.sourceEid);
	}

	/*	Free the CustodyBundle object and list element.		*/
	sdr_free(sdr, cbObj);
	sdr_list_delete(sdr, custodyElt, NULL, NULL);
}

/**
 * Remove custody tracking for a bundle being destroyed.
 *
 * Called from bpDestroyBundle AFTER the detained check, so it is only
 * invoked when the bundle is truly being destroyed (either TTL expired
 * or custody was released via CCS and bundle is no longer detained).
 *
 * For custody bundles awaiting CCS, the detained flag keeps bpDestroyBundle
 * from reaching this point - custody tracking is preserved. When CCS is
 * received, cbr_releaseCustody clears detained and removes the custody
 * entry before calling bpDestroyBundle.
 */
void cbr_untrackBundleByObj(Sdr sdr, SdrObject bundleObj)
{
	CbrDb		*cbrConstants;
	SdrObject	elt;
	SdrObject	nextElt;
	SdrObject	cbObj;
	CustodyBundle	cb;

	if (bundleObj == 0)
	{
		return;
	}

	cbrConstants = getCbrConstants();
	if (cbrConstants == NULL)
	{
		return;		/*	CBR not initialized.		*/
	}

	/*	Search custody list for entry with matching bundleObj.	*/
	for (elt = sdr_list_first(sdr, cbrConstants->custodyBundles);
			elt; elt = nextElt)
	{
		nextElt = sdr_list_next(sdr, elt);
		cbObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &cb, cbObj, sizeof(CustodyBundle));

		if (cb.bundleObj == bundleObj)
		{
			/*	Bundle is being destroyed (TTL expired
			 *	or custody released). Remove custody
			 *	tracking entry entirely.		*/
			writeMemo("[i] CBR: Removing custody tracking for "
					"destroyed bundle.");
			cbr_untrackCustodyBundle(sdr, elt);
			return;		/*	Only one entry per bundle.*/
		}
	}
}

/**
 * Queue a CCS (Custody Confirmation Signal) for transmission.
 * Internal helper used by accept and refuse functions.
 */
static int	queueCcs(Sdr sdr, char *destEid, char *sourceEid,
			uvast seqId, uvast seqNum, int disposition)
{
	SdrObject	signalElt;
	CbrDb		*cbrConstants;
	PendingSignal	signal;
	unsigned int	ccsAggregateLimit;
	char		msgBuf[256];

	cbrConstants = getCbrConstants();
	if (cbrConstants == NULL)
	{
		return -1;
	}

	isprintf(msgBuf, sizeof(msgBuf),
			"[i] CBR: Queueing CCS to %s (seqId=%lu seqNum=%lu disp=%d)",
			destEid, (unsigned long) seqId,
			(unsigned long) seqNum, disposition);
	writeMemo(msgBuf);

	/*	Find or create pending CCS signal to this destination.	*/
	signalElt = cbr_findOrCreatePendingSignal(sdr, destEid,
			CBR_SIGNAL_CCS, disposition);
	if (signalElt == 0)
	{
		putErrmsg("CBR: Can't create pending CCS.", NULL);
		return -1;
	}

	/*	Add bundle sequence to the signal.			*/
	if (cbr_addToSignalSequences(sdr, signalElt, sourceEid, seqId,
			seqNum) < 0)
	{
		putErrmsg("CBR: Can't add to CCS sequences.", NULL);
		return -1;
	}

	/*	Check if we should transmit immediately.  Read the limit live
	 *	so a runtime "m cbraggr" is honored without a restart.		*/
	ccsAggregateLimit = cbrConstants->ccsAggregateLimit;
	cbr_liveAggregateConfig(sdr, NULL, &ccsAggregateLimit, NULL);
	sdr_read(sdr, (char *) &signal,
			sdr_list_data(sdr, signalElt), sizeof(PendingSignal));

	isprintf(msgBuf, sizeof(msgBuf),
			"[i] CBR: CCS bundleCount=%u limit=%u",
			signal.bundleCount, ccsAggregateLimit);
	writeMemo(msgBuf);

	if (signal.bundleCount >= ccsAggregateLimit)
	{
		writeMemo("[i] CBR: Aggregate limit reached, transmitting CCS now.");
		if (cbr_transmitSignal(sdr, signalElt) < 0)
		{
			putErrmsg("CBR: Can't transmit CCS.", NULL);
			return -1;
		}
	}

	return 0;
}

/*	*	Custody Acceptance Whitelist	*	*	*	*/

static int acceptListContains(Sdr sdr, SdrObject list, const char *eid)
{
	SdrObject elt;
	SdrObject obj;
	char	buf[SDRSTRING_BUFSZ];

	if (list == 0 || sdr_list_length(sdr, list) == 0)
	{
		return 1;	/* empty list = accept all */
	}

	for (elt = sdr_list_first(sdr, list); elt;
			elt = sdr_list_next(sdr, elt))
	{
		obj = sdr_list_data(sdr, elt);
		if (sdr_string_read(sdr, buf, obj) >= 0
				&& strcmp(buf, eid) == 0)
		{
			return 1;
		}
	}

	return 0;
}

int	cbr_isCustodyAccepted(Sdr sdr, const char *custodianEid,
		const char *sourceEid)
{
	SdrObject cbrDbObj;
	CbrDb	cbrDb;

	CHKZERO(sdr);
	cbrDbObj = getCbrDbObject();
	if (cbrDbObj == 0)
	{
		return 1;	/* no DB yet — accept (safe fallback) */
	}

	sdr_read(sdr, (char *) &cbrDb, cbrDbObj, sizeof(CbrDb));

	if (custodianEid != NULL
			&& !acceptListContains(sdr,
			cbrDb.custodyAcceptByCustodian, custodianEid))
	{
		return 0;
	}

	if (sourceEid != NULL
			&& !acceptListContains(sdr,
			cbrDb.custodyAcceptBySource, sourceEid))
	{
		return 0;
	}

	return 1;
}

SdrObject cbr_getCustodyAcceptList(Sdr sdr, int forCustodian)
{
	SdrObject cbrDbObj;
	CbrDb	cbrDb;

	cbrDbObj = getCbrDbObject();
	if (cbrDbObj == 0)
	{
		return 0;
	}

	sdr_read(sdr, (char *) &cbrDb, cbrDbObj, sizeof(CbrDb));
	return forCustodian ? cbrDb.custodyAcceptByCustodian
			: cbrDb.custodyAcceptBySource;
}

int	cbr_addCustodyAccept(Sdr sdr, int forCustodian, const char *eid)
{
	SdrObject	cbrDbObj;
	CbrDb	cbrDb;
	SdrObject	list;
	SdrObject	elt;
	SdrObject	obj;
	char	buf[SDRSTRING_BUFSZ];

	CHKERR(sdr);
	CHKERR(eid);
	cbrDbObj = getCbrDbObject();
	CHKERR(cbrDbObj);

	CHKERR(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &cbrDb, cbrDbObj, sizeof(CbrDb));
	list = forCustodian ? cbrDb.custodyAcceptByCustodian
			: cbrDb.custodyAcceptBySource;

	/*	Reject duplicates.					*/
	for (elt = sdr_list_first(sdr, list); elt;
			elt = sdr_list_next(sdr, elt))
	{
		obj = sdr_list_data(sdr, elt);
		if (sdr_string_read(sdr, buf, obj) >= 0
				&& strcmp(buf, eid) == 0)
		{
			sdr_exit_xn(sdr);
			return 0;	/* already present */
		}
	}

	/*	Copy to non-const buffer; sdr_string_create takes char *.*/
	istrcpy(buf, eid, sizeof buf);
	obj = sdr_string_create(sdr, buf);
	if (obj == 0 || sdr_list_insert_last(sdr, list, obj) == 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't add CBR accept entry.", NULL);
		return -1;
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't add CBR accept entry.", NULL);
		return -1;
	}

	return 0;
}

int	cbr_removeCustodyAccept(Sdr sdr, int forCustodian, const char *eid)
{
	SdrObject cbrDbObj;
	CbrDb	cbrDb;
	SdrObject list;
	SdrObject elt;
	SdrObject obj;
	char	buf[SDRSTRING_BUFSZ];

	CHKERR(sdr);
	CHKERR(eid);
	cbrDbObj = getCbrDbObject();
	CHKERR(cbrDbObj);

	CHKERR(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &cbrDb, cbrDbObj, sizeof(CbrDb));
	list = forCustodian ? cbrDb.custodyAcceptByCustodian
			: cbrDb.custodyAcceptBySource;

	for (elt = sdr_list_first(sdr, list); elt;
			elt = sdr_list_next(sdr, elt))
	{
		obj = sdr_list_data(sdr, elt);
		if (sdr_string_read(sdr, buf, obj) >= 0
				&& strcmp(buf, eid) == 0)
		{
			sdr_free(sdr, obj);
			sdr_list_delete(sdr, elt, NULL, NULL);
			break;
		}
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't remove CBR accept entry.", NULL);
		return -1;
	}

	return 0;
}

/*	*	Auto Custody-Request Policy				*/

int	cbr_isCustodyRequired(Sdr sdr, const char *destEid)
{
	SdrObject cbrDbObj;
	CbrDb	cbrDb;
	SdrObject elt;
	SdrObject obj;
	char	buf[SDRSTRING_BUFSZ];

	CHKZERO(sdr);
	CHKZERO(destEid);
	cbrDbObj = getCbrDbObject();
	if (cbrDbObj == 0)
	{
		return 0;
	}

	sdr_read(sdr, (char *) &cbrDb, cbrDbObj, sizeof(CbrDb));
	if (cbrDb.custodyReqDests == 0
			|| sdr_list_length(sdr, cbrDb.custodyReqDests) == 0)
	{
		return 0;	/* empty list = no auto-request policy */
	}

	for (elt = sdr_list_first(sdr, cbrDb.custodyReqDests); elt;
			elt = sdr_list_next(sdr, elt))
	{
		obj = sdr_list_data(sdr, elt);
		if (sdr_string_read(sdr, buf, obj) >= 0
				&& strcmp(buf, destEid) == 0)
		{
			return 1;
		}
	}

	return 0;
}

SdrObject cbr_getCustodyReqList(Sdr sdr)
{
	SdrObject cbrDbObj;
	CbrDb	cbrDb;

	cbrDbObj = getCbrDbObject();
	if (cbrDbObj == 0)
	{
		return 0;
	}

	sdr_read(sdr, (char *) &cbrDb, cbrDbObj, sizeof(CbrDb));
	return cbrDb.custodyReqDests;
}

int	cbr_addCustodyReq(Sdr sdr, const char *eid)
{
	SdrObject cbrDbObj;
	CbrDb	cbrDb;
	SdrObject elt;
	SdrObject obj;
	char	buf[SDRSTRING_BUFSZ];

	CHKERR(sdr);
	CHKERR(eid);
	cbrDbObj = getCbrDbObject();
	CHKERR(cbrDbObj);

	CHKERR(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &cbrDb, cbrDbObj, sizeof(CbrDb));

	/*	Reject duplicates.					*/
	for (elt = sdr_list_first(sdr, cbrDb.custodyReqDests); elt;
			elt = sdr_list_next(sdr, elt))
	{
		obj = sdr_list_data(sdr, elt);
		if (sdr_string_read(sdr, buf, obj) >= 0
				&& strcmp(buf, eid) == 0)
		{
			sdr_exit_xn(sdr);
			return 0;	/* already present */
		}
	}

	istrcpy(buf, eid, sizeof buf);
	obj = sdr_string_create(sdr, buf);
	if (obj == 0
			|| sdr_list_insert_last(sdr, cbrDb.custodyReqDests,
			obj) == 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't add custody-req entry.", NULL);
		return -1;
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't add custody-req entry.", NULL);
		return -1;
	}

	return 0;
}

int	cbr_removeCustodyReq(Sdr sdr, const char *eid)
{
	SdrObject cbrDbObj;
	CbrDb	cbrDb;
	SdrObject elt;
	SdrObject obj;
	char	buf[SDRSTRING_BUFSZ];

	CHKERR(sdr);
	CHKERR(eid);
	cbrDbObj = getCbrDbObject();
	CHKERR(cbrDbObj);

	CHKERR(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &cbrDb, cbrDbObj, sizeof(CbrDb));

	for (elt = sdr_list_first(sdr, cbrDb.custodyReqDests); elt;
			elt = sdr_list_next(sdr, elt))
	{
		obj = sdr_list_data(sdr, elt);
		if (sdr_string_read(sdr, buf, obj) >= 0
				&& strcmp(buf, eid) == 0)
		{
			sdr_free(sdr, obj);
			sdr_list_delete(sdr, elt, NULL, NULL);
			break;
		}
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't remove custody-req entry.", NULL);
		return -1;
	}

	return 0;
}

int cbr_acceptCustody(Sdr sdr, Bundle *bundle, SdrObject bundleAddr,
		CtebBlk *cteb)
{
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	int		result;

	CHKERR(cteb);
	CHKERR(cteb->custodianEid);

	/*	The block source AEID for any future CTEB this node mints
	 *	on behalf of the bundle is the local admin EID, and per
	 *	Orange Book 4.2.4 the next-hop CCS coming back to us will
	 *	omit that AEID from its BSC.  We therefore key custody
	 *	tracking on the local admin EID, so cbr_handleCcs can look
	 *	the bundle up after substituting that same EID for the
	 *	missing wire field.					*/
	findScheme("ipn", &vscheme, &vschemeElt);
	if (vschemeElt == 0 || vscheme->adminEid[0] == '\0')
	{
		putErrmsg("CBR: Can't find local admin EID for ipn scheme.",
				NULL);
		return -1;
	}

	/*	Add bundle to custody tracking (skip for destination).
	 *	bundleAddr == 0 indicates destination delivery - no need
	 *	to track custody since there's no next hop to wait for.	*/
	if (bundleAddr != 0)
	{
		if (cbr_trackCustodyBundle(sdr, bundleAddr,
				bundle->proxNodeEid ? "" : cteb->custodianEid,
				vscheme->adminEid, cteb->seqId, cteb->seqNum)
				== 0)
		{
			/*	May already be tracked or allocation failed. */
			writeMemoNote("[?] CBR: Failed to track custody bundle",
					vscheme->adminEid);
			/*	Continue anyway to send CCS.		*/
		}
	}

	/*	Queue CCS acceptance to previous custodian.
	 *
	 *	Per Orange Book 4.2.4, the bundle sequences carried in
	 *	a CCS shall never contain a block source AEID, because
	 *	the CCS is addressed back to the node that minted the
	 *	BSN (the previous custodian named in the CTEB).  We
	 *	therefore pass cteb->custodianEid -- not the bundle's
	 *	primary-block source -- as the BSC source EID, so that
	 *	cbr_encodeBundleSequence()'s "source == dest" omission
	 *	rule fires at every hop, not just at the originator.	*/
	result = queueCcs(sdr, cteb->custodianEid, cteb->custodianEid,
			cteb->seqId, cteb->seqNum, CBR_CUSTODY_ACCEPTED);

	if (result < 0)
	{
		putErrmsg("CBR: Can't queue CCS acceptance.", NULL);
		return -1;
	}

	/*	Increment custody accepted counter.
	 *	Must use a transaction since queueCcs may have
	 *	committed its own transaction already.			*/
	{
		SdrObject cbrDbObj = getCbrDbObject();

		if (cbrDbObj)
		{
			CbrDb	cbrDb;

			CHKERR(sdr_begin_xn(sdr));
			sdr_stage(sdr, (char *) &cbrDb, cbrDbObj,
					sizeof(CbrDb));
			cbrDb.custodyAccepted++;
			sdr_write(sdr, cbrDbObj, (char *) &cbrDb,
					sizeof(CbrDb));
			if (sdr_end_xn(sdr) < 0)
			{
				putErrmsg("CBR: Can't update custody counter.",
						NULL);
				return -1;
			}
		}
	}

	writeMemo("[i] CBR: Custody accepted.");
	return 0;
}

int	cbr_refuseCustody(Sdr sdr, Bundle *bundle, CtebBlk *cteb,
		CbrRefuseReason reason)
{
	int		result;

	(void) bundle;		/*	May use for logging later.	*/

	CHKERR(cteb);
	CHKERR(cteb->custodianEid);

	/*	Log refusal reason locally (not transmitted per spec).	*/
	writeMemoNote("[i] CBR: Custody refused, reason", itoa(reason));

	/*	Queue CCS refusal to previous custodian.
	 *	Per Orange Book, refusal is just -1 with no reason.
	 *	See cbr_acceptCustody() for why cteb->custodianEid is
	 *	used as the BSC source EID (Orange Book 4.2.4).		*/
	result = queueCcs(sdr, cteb->custodianEid, cteb->custodianEid,
			cteb->seqId, cteb->seqNum, CBR_CUSTODY_REFUSED);

	if (result < 0)
	{
		putErrmsg("CBR: Can't queue CCS refusal.", NULL);
		return -1;
	}

	return 0;
}

int	cbr_releaseCustody(Sdr sdr, char *sourceEid, uvast seqId,
		uvast seqNumStart, uvast length)
{
	uvast		i;
	SdrObject	custodyElt;
	CustodyBundle	cb;
	SdrObject	cbObj;

	/*	Release custody for each bundle in the range.		*/
	for (i = 0; i < length; i++)
	{
		custodyElt = cbr_findCustodyBundle(sdr, sourceEid, seqId,
				seqNumStart + i);
		if (custodyElt != 0)
		{
			SdrObject cbrDbObj = getCbrDbObject();

			/*	Get custody bundle data before untracking.	*/
			cbObj = sdr_list_data(sdr, custodyElt);
			sdr_read(sdr, (char *) &cb, cbObj, sizeof(CustodyBundle));

			/*	Clear detained flag and destroy bundle.
			 *	bpDestroyBundle will call cbr_untrackBundleByObj
			 *	which removes the custody tracking entry.	*/
			if (cb.bundleObj != 0)
			{
				Bundle	bundle;

				sdr_stage(sdr, (char *) &bundle, cb.bundleObj,
						sizeof(Bundle));
				bundle.detained = 0;
				sdr_write(sdr, cb.bundleObj, (char *) &bundle,
						sizeof(Bundle));
				bpDestroyBundle(cb.bundleObj, 0);
				/*	Custody entry removed by bpDestroyBundle
				 *	via cbr_untrackBundleByObj.		*/
			}
			else
			{
				/*	Bundle already destroyed (TTL expired).
				 *	Remove orphaned custody entry.		*/
				cbr_untrackCustodyBundle(sdr, custodyElt);
			}

			/*	Increment custody released counter.	*/
			if (cbrDbObj)
			{
				CbrDb	cbrDb;

				sdr_stage(sdr, (char *) &cbrDb, cbrDbObj,
						sizeof(CbrDb));
				cbrDb.custodyReleased++;
				sdr_write(sdr, cbrDbObj, (char *) &cbrDb,
						sizeof(CbrDb));
			}

			writeMemoNote("[i] CBR: Custody released for seqNum",
					itoa(seqNumStart + i));
		}
	}

	return 0;
}

int	cbr_handleCrs(Sdr sdr, unsigned char *adminRecord, int length,
		const char *senderEid)
{
	unsigned char		*cursor = adminRecord;
	unsigned int		unparsedBytes = length;
	uvast			mapLen;
	uvast			statusCode;
	uvast			arrayLen;
	uvast			seqId;
	uvast			seqNumStart;
	uvast			bundleLen;
	uvast			*rangeArray;
	int			rangeCount;
	char			*sourceEid;
	char			*seqDestEid;
	uvast			i;
	uvast			bundleCount;
	SdrObject		cbrDbObj;
	CbrDb			cbrDb;
	ReceivedCrsRecord	rec;
	SdrObject		recObj;
	SdrObject		firstElt;
	SdrObject		firstObj;
	ReceivedCrsRecord	oldest;

	/*	CRS content format (after stripping admin record header):
	 *	{ status-reason => [Bundle-Sequence, ...], ... }	*/

	CHKERR(sdr_begin_xn(sdr));

	cbrDbObj = getCbrDbObject();
	if (cbrDbObj == 0)
	{
		sdr_cancel_xn(sdr);
		return -1;
	}

	sdr_read(sdr, (char *) &cbrDb, cbrDbObj, sizeof(CbrDb));

	/*	Decode map						*/
	mapLen = 0;
	if (cbor_decode_map_open(&mapLen, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] CRS: Can't decode map open.");
		sdr_cancel_xn(sdr);
		return -1;
	}

	/*	Process each status-reason entry			*/
	for (i = 0; i < mapLen; i++)
	{
		/*	Key: status-reason code				*/
		if (cbor_decode_integer(&statusCode, CborAny, &cursor,
				&unparsedBytes) < 1)
		{
			writeMemo("[?] CRS: Can't decode status code.");
			sdr_cancel_xn(sdr);
			return -1;
		}

		/*	Value: array of Bundle-Sequence			*/
		arrayLen = 0;
		if (cbor_decode_array_open(&arrayLen, &cursor,
				&unparsedBytes) < 1)
		{
			writeMemo("[?] CRS: Can't decode sequence array.");
			sdr_cancel_xn(sdr);
			return -1;
		}

		bundleCount = 0;

		/*	Decode each Bundle-Sequence			*/
		while (arrayLen > 0)
		{
			if (cbr_decodeBundleSequence(&cursor, &unparsedBytes,
					&seqId, &seqNumStart, &bundleLen,
					&rangeArray, &rangeCount,
					&sourceEid, &seqDestEid) < 0)
			{
				writeMemo("[?] CRS: Can't decode Bundle-Sequence.");
				sdr_cancel_xn(sdr);
				return -1;
			}

			writeMemoNote("[i] CRS received: status",
					itoa(statusCode));
			if (rangeCount > 0)
			{
				writeMemoNote("[i] CRS received: range-array count",
						itoa(rangeCount));
			}

			bundleCount += (bundleLen > 0 ? bundleLen : 1);

			if (rangeArray)
			{
				MRELEASE(rangeArray);
			}

			if (sourceEid)
			{
				MRELEASE(sourceEid);
			}

			if (seqDestEid)
			{
				MRELEASE(seqDestEid);
			}

			arrayLen--;
		}

		/*	Store one history record per status-code entry.	*/
		if (cbrDb.crsHistory)
		{
			memset(&rec, 0, sizeof(ReceivedCrsRecord));
			rec.receivedAt = getCtime();
			if (senderEid && *senderEid)
			{
				char	senderBuf[SDRSTRING_BUFSZ];

				istrcpy(senderBuf, senderEid, sizeof senderBuf);
				rec.senderEid = sdr_string_create(sdr, senderBuf);
			}
			else
			{
				rec.senderEid = 0;
			}
			rec.statusCode = (int) statusCode;
			rec.bundleCount = bundleCount;

			recObj = sdr_malloc(sdr, sizeof(ReceivedCrsRecord));
			if (recObj == 0)
			{
				if (rec.senderEid)
				{
					sdr_free(sdr, rec.senderEid);
				}
			}
			else
			{
				sdr_write(sdr, recObj, (char *) &rec,
						sizeof(ReceivedCrsRecord));
				if (sdr_list_insert_last(sdr, cbrDb.crsHistory,
						recObj) == 0)
				{
					sdr_free(sdr, recObj);
					if (rec.senderEid)
					{
						sdr_free(sdr, rec.senderEid);
					}
				}
				else if (cbrDb.crsHistoryMax > 0
					&& sdr_list_length(sdr, cbrDb.crsHistory)
						> cbrDb.crsHistoryMax)
				{
					/*	Evict oldest entry.	*/
					firstElt = sdr_list_first(sdr,
							cbrDb.crsHistory);
					if (firstElt)
					{
						firstObj = sdr_list_data(sdr,
								firstElt);
						sdr_read(sdr, (char *) &oldest,
							firstObj,
							sizeof(ReceivedCrsRecord));
						if (oldest.senderEid)
						{
							sdr_free(sdr,
								oldest.senderEid);
						}

						sdr_free(sdr, firstObj);
						sdr_list_delete(sdr, firstElt,
								NULL, NULL);
					}
				}
			}
		}
	}

	/*	Increment CRS received counter.				*/
	sdr_stage(sdr, (char *) &cbrDb, cbrDbObj, sizeof(CbrDb));
	cbrDb.crsSignalsRecv++;
	sdr_write(sdr, cbrDbObj, (char *) &cbrDb, sizeof(CbrDb));

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("CRS: Transaction failed.", NULL);
		return -1;
	}

	return 0;
}

int	cbr_setCrsHistoryMax(Sdr sdr, unsigned int max)
{
	SdrObject cbrDbObj;
	CbrDb	cbrDb;

	CHKERR(sdr);
	cbrDbObj = getCbrDbObject();
	CHKERR(cbrDbObj);

	CHKERR(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &cbrDb, cbrDbObj, sizeof(CbrDb));
	cbrDb.crsHistoryMax = max;
	sdr_write(sdr, cbrDbObj, (char *) &cbrDb, sizeof(CbrDb));

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't set CRS history max.", NULL);
		return -1;
	}

	return 0;
}

SdrObject cbr_getCrsHistoryList(Sdr sdr)
{
	SdrObject cbrDbObj;
	CbrDb	cbrDb;

	cbrDbObj = getCbrDbObject();
	if (cbrDbObj == 0)
	{
		return 0;
	}

	sdr_read(sdr, (char *) &cbrDb, cbrDbObj, sizeof(CbrDb));
	return cbrDb.crsHistory;
}

int	cbr_handleCcs(Sdr sdr, unsigned char *adminRecord, int length)
{
	unsigned char	*cursor = adminRecord;
	unsigned int	unparsedBytes = length;
	uvast		mapLen;
	vast		disposition;
	uvast		arrayLen;
	uvast		seqId;
	uvast		seqNumStart;
	uvast		bundleLen;
	uvast		*rangeArray;
	int		rangeCount;
	char		*sourceEid;
	char		*seqDestEid;
	uvast		i;
	uvast		j;
	uvast		k;
	uvast		rangeStart;
	uvast		rangeLen;
	CbrDb		*cbrConstants;

	/*	CCS content format (after stripping admin record header):
	 *	{ disposition => [Bundle-Sequence, ...], ... }
	 *
	 *	Disposition is SIGNED: 1=accepted, -1=refused		*/

	cbrConstants = getCbrConstants();

	/*	Start transaction for SDR operations (custody release,
	 *	statistics updates).					*/

	CHKERR(sdr_begin_xn(sdr));

	/*	Decode map						*/
	mapLen = 0;
	if (cbor_decode_map_open(&mapLen, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] CCS: Can't decode map open.");
		sdr_cancel_xn(sdr);
		return -1;
	}

	/*	Process each disposition entry				*/
	for (i = 0; i < mapLen; i++)
	{
		/*	Key: disposition code (signed integer).
		 *	Orange Book encodes 1=accepted, -1=refused.
		 *	Must use cbor_decode_signed_int to handle both
		 *	CBOR major type 0 (positive) and type 1 (negative).	*/

		if (cbor_decode_signed_int(&disposition, &cursor,
				&unparsedBytes) < 1)
		{
			writeMemo("[?] CCS: Can't decode disposition.");
			sdr_cancel_xn(sdr);
			return -1;
		}

		/*	Value: array of Bundle-Sequence			*/
		arrayLen = 0;
		if (cbor_decode_array_open(&arrayLen, &cursor,
				&unparsedBytes) < 1)
		{
			writeMemo("[?] CCS: Can't decode sequence array.");
			sdr_cancel_xn(sdr);
			return -1;
		}

		/*	Decode each Bundle-Sequence			*/
		while (arrayLen > 0)
		{
			if (cbr_decodeBundleSequence(&cursor, &unparsedBytes,
					&seqId, &seqNumStart, &bundleLen,
					&rangeArray, &rangeCount,
					&sourceEid, &seqDestEid) < 0)
			{
				writeMemo("[?] CCS: Can't decode Bundle-Sequence.");
				sdr_cancel_xn(sdr);
				return -1;
			}

			/*	Orange Book 4.2.4: a CCS BSC omits the
			 *	block source AEID because the CCS receiver
			 *	is by definition the BSN provider.  When
			 *	the decoder reports a missing sourceEid,
			 *	substitute our own admin EID so the custody
			 *	lookup keys match what cbr_trackCustodyBundle
			 *	stored.						*/
			if (sourceEid == NULL)
			{
				VScheme		*vscheme;
				PsmAddress	vschemeElt;
				size_t		eidLen;

				findScheme("ipn", &vscheme, &vschemeElt);
				if (vschemeElt == 0
						|| vscheme->adminEid[0] == '\0')
				{
					writeMemo("[?] CCS: No local admin EID;"
						" can't resolve omitted"
						" block source AEID.");
					if (rangeArray)
					{
						MRELEASE(rangeArray);
					}
					if (seqDestEid)
					{
						MRELEASE(seqDestEid);
					}
					sdr_cancel_xn(sdr);
					return -1;
				}

				eidLen = strlen(vscheme->adminEid) + 1;
				sourceEid = MTAKE(eidLen);
				if (sourceEid == NULL)
				{
					writeMemo("[?] CCS: Out of memory"
						" substituting source EID.");
					if (rangeArray)
					{
						MRELEASE(rangeArray);
					}
					if (seqDestEid)
					{
						MRELEASE(seqDestEid);
					}
					sdr_cancel_xn(sdr);
					return -1;
				}
				istrcpy(sourceEid, vscheme->adminEid, eidLen);
			}

			/*	Process based on disposition.		*/
			if (disposition == CBR_CUSTODY_ACCEPTED)
			{
				/*	Next custodian accepted.
				 *	Release our custody.		*/

				if (rangeArray == NULL)
				{
					/*	Simple contiguous range.*/
					cbr_releaseCustody(sdr, sourceEid,
							seqId, seqNumStart,
							bundleLen);
				}
				else
				{
					/*	Non-contiguous ranges.	*/
					rangeStart = seqNumStart;
					for (j = 0; j < (uvast)rangeCount; j++)
					{
						rangeLen = rangeArray[j];
						if ((j % 2) == 0)
						{
							/*	Included.*/
							cbr_releaseCustody(sdr,
								sourceEid,
								seqId,
								rangeStart,
								rangeLen);
						}
						rangeStart += rangeLen;
					}
				}

				writeMemoNote("[i] CCS: Custody accepted by next hop",
						sourceEid ? sourceEid : "(null)");

				/*	Increment CCS accept recv counter. */
				{
					SdrObject cbrDbObj = getCbrDbObject();

					if (cbrDbObj)
					{
						CbrDb	cbrDb;

						sdr_stage(sdr, (char *) &cbrDb,
							cbrDbObj, sizeof(CbrDb));
						cbrDb.ccsAcceptRecv++;
						sdr_write(sdr, cbrDbObj,
							(char *) &cbrDb,
							sizeof(CbrDb));
					}
				}
			}
			else
			{
				/*	Custody refused.
				 *	Bundle will be retransmitted per
				 *	configured strategy.		*/
				writeMemoNote("[i] CCS: Custody refused by next hop",
						sourceEid ? sourceEid : "(null)");

				/*	Increment CCS refuse recv counter. */
				{
					SdrObject cbrDbObj = getCbrDbObject();

					if (cbrDbObj)
					{
						CbrDb	cbrDb;

						sdr_stage(sdr, (char *) &cbrDb,
							cbrDbObj, sizeof(CbrDb));
						cbrDb.ccsRefuseRecv++;
						sdr_write(sdr, cbrDbObj,
							(char *) &cbrDb,
							sizeof(CbrDb));
					}
				}

				/*	CBR_RETX_SIGNAL: reforward refused
				 *	bundles immediately.		*/
				if (cbrConstants->retransmitStrategy
						== CBR_RETX_SIGNAL)
				{
					SdrObject     custodyElt;
					CustodyBundle retxCb;

					if (rangeArray == NULL)
					{
						for (k = 0; k < bundleLen; k++)
						{
							custodyElt =
							cbr_findCustodyBundle(
								sdr, sourceEid,
								seqId,
								seqNumStart + k);
							if (custodyElt == 0)
							{
								continue;
							}

							sdr_stage(sdr,
								(char *) &retxCb,
								sdr_list_data(sdr,
								custodyElt),
								sizeof(CustodyBundle));
							if (cbrConstants->maxRetransmissions > 0
							&& (unsigned int) retxCb.retransmitCount
									>= cbrConstants->maxRetransmissions)
							{
								continue;
							}

							if (cbr_doRetransmit(sdr,
									&retxCb,
									custodyElt)
									< 0)
							{
								sdr_cancel_xn(sdr);
								return -1;
							}

							writeMemoNote("[i] CBR: Signal-triggered retransmit, count",
								itoa(retxCb.retransmitCount));
						}
					}
					else
					{
						rangeStart = seqNumStart;
						for (j = 0; j < (uvast)rangeCount;
								j++)
						{
							rangeLen = rangeArray[j];
							if ((j % 2) == 0) /* included */
							{
								for (k = 0; k < rangeLen;
										k++)
								{
									custodyElt =
									cbr_findCustodyBundle(
										sdr, sourceEid,
										seqId,
										rangeStart + k);
									if (custodyElt == 0)
									{
										continue;
									}

									sdr_stage(sdr,
										(char *) &retxCb,
										sdr_list_data(sdr,
										custodyElt),
										sizeof(CustodyBundle));
									if (cbrConstants->maxRetransmissions > 0
									&& (unsigned int) retxCb.retransmitCount
											>= cbrConstants->maxRetransmissions)
									{
										continue;
									}

									if (cbr_doRetransmit(
											sdr,
											&retxCb,
											custodyElt)
											< 0)
									{
										sdr_cancel_xn(sdr);
										return -1;
									}

									writeMemoNote("[i] CBR: Signal-triggered retransmit, count",
										itoa(retxCb.retransmitCount));
								}
							}

							rangeStart += rangeLen;
						}
					}
				}
			}		/*	end custody refused	*/

			/*	Clean up				*/
			if (rangeArray)
			{
				MRELEASE(rangeArray);
			}

			if (sourceEid)
			{
				MRELEASE(sourceEid);
			}

			if (seqDestEid)
			{
				MRELEASE(seqDestEid);
			}

			arrayLen--;
		}
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("CCS: Transaction failed.", NULL);
		return -1;
	}

	return 0;
}

/*	*	*	Manual Retransmission Functions	*	*	*/

int	cbr_retransmitBundle(Sdr sdr, char *sourceEid, uvast seqId,
		uvast seqNum)
{
	SdrObject	custodyElt;
	SdrObject	cbObj;
	CustodyBundle	cb;
	int		result;

	CHKERR(sdr_begin_xn(sdr));

	/*	Find the custody bundle.				*/
	custodyElt = cbr_findCustodyBundle(sdr, sourceEid, seqId, seqNum);
	if (custodyElt == 0)
	{
		writeMemo("[?] CBR: Bundle not found in custody.");
		sdr_exit_xn(sdr);
		return -1;
	}

	cbObj = sdr_list_data(sdr, custodyElt);
	sdr_stage(sdr, (char *) &cb, cbObj, sizeof(CustodyBundle));

	/*	Verify bundle object still exists (not expired/deleted).*/
	if (cb.bundleObj == 0)
	{
		writeMemo("[?] CBR: Custody bundle has no bundle reference.");
		sdr_exit_xn(sdr);
		return -1;
	}

	/*	Update retransmit tracking before re-forwarding.	*/
	cb.lastTransmit = getCtime();
	cb.retransmitCount++;
	sdr_write(sdr, cbObj, (char *) &cb, sizeof(CustodyBundle));

	/*	Re-queue the bundle for transmission.
	 *	bpReforwardBundle clears existing queue references
	 *	and calls forwardBundle to compute a new route.		*/
	result = bpReforwardBundle(cb.bundleObj);
	if (result < 0)
	{
		putErrmsg("CBR: Failed to reforward custody bundle.", NULL);
		sdr_cancel_xn(sdr);
		return -1;
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("CBR: Failed ending retransmit transaction.", NULL);
		return -1;
	}

	writeMemoNote("[i] CBR: Retransmitted custody bundle, seqNum",
			itoa(seqNum));
	return 0;
}

int	cbr_retransmitAllCustody(Sdr sdr, char *destEid)
{
	CbrDb		*cbrConstants;
	SdrObject	elt;
	SdrObject	nextElt;
	SdrObject	cbObj;
	CustodyBundle	cb;
	char		eidBuf[MAX_EID_LEN];
	int		count = 0;
	int		result;

	cbrConstants = getCbrConstants();
	if (cbrConstants == NULL)
	{
		return -1;
	}

	CHKERR(sdr_begin_xn(sdr));

	/*	Iterate through all custody bundles.
	 *	Cache nextElt before reforwarding since list may change.*/
	for (elt = sdr_list_first(sdr, cbrConstants->custodyBundles);
			elt; elt = nextElt)
	{
		nextElt = sdr_list_next(sdr, elt);
		cbObj = sdr_list_data(sdr, elt);
		sdr_stage(sdr, (char *) &cb, cbObj, sizeof(CustodyBundle));

		/*	Check if destEid filter matches.		*/
		if (destEid != NULL && destEid[0] != '\0')
		{
			sdr_string_read(sdr, eidBuf, cb.destEid);
			if (strcmp(eidBuf, destEid) != 0)
			{
				continue;
			}
		}

		/*	Skip if bundle reference is invalid.		*/
		if (cb.bundleObj == 0)
		{
			continue;
		}

		/*	Update retransmit tracking.			*/
		cb.lastTransmit = getCtime();
		cb.retransmitCount++;
		sdr_write(sdr, cbObj, (char *) &cb, sizeof(CustodyBundle));

		/*	Re-queue the bundle for transmission.		*/
		result = bpReforwardBundle(cb.bundleObj);
		if (result < 0)
		{
			putErrmsg("CBR: Failed reforward in bulk retransmit.",
					NULL);
			sdr_cancel_xn(sdr);
			return -1;
		}

		count++;
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("CBR: Failed ending bulk retransmit transaction.",
				NULL);
		return -1;
	}

	writeMemoNote("[i] CBR: Bulk retransmit completed, count",
			itoa(count));
	return count;
}

int	cbr_listCustodyBundles(Sdr sdr, CbrCustodyCallback callback,
		void *userData)
{
	CbrDb		*cbrConstants;
	SdrObject	elt;
	SdrObject	cbObj;
	CustodyBundle	cb;
	CbrCustodyInfo	info;
	char		destEidBuf[MAX_EID_LEN];
	int		count = 0;

	CHKERR(sdr);
	CHKERR(callback);

	cbrConstants = getCbrConstants();
	if (cbrConstants == NULL)
	{
		return -1;
	}

	/*	Must be in transaction to read from SDR.		*/
	if (!sdr_in_xn(sdr))
	{
		CHKERR(sdr_begin_xn(sdr));
	}

	/*	Iterate through all custody bundles.			*/
	for (elt = sdr_list_first(sdr, cbrConstants->custodyBundles);
			elt; elt = sdr_list_next(sdr, elt))
	{
		cbObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &cb, cbObj, sizeof(CustodyBundle));

		/*	Populate info structure.			*/
		sdr_string_read(sdr, destEidBuf, cb.destEid);
		info.destEid = destEidBuf;
		info.seqId = cb.seqId;
		info.seqNum = cb.seqNum;
		info.custodyAccepted = cb.custodyAccepted;
		info.lastTransmit = cb.lastTransmit;
		info.retransmitCount = cb.retransmitCount;

		/*	Invoke callback.				*/
		callback(&info, userData);
		count++;
	}

	sdr_exit_xn(sdr);
	return count;
}

int	cbr_getCustodyStatus(Sdr sdr, char *sourceEid, uvast seqId,
		uvast seqNum)
{
	SdrObject	custodyElt;
	int		startedXn = 0;

	CHKERR(sdr);

	/*	Must be in transaction to read from SDR.		*/
	if (!sdr_in_xn(sdr))
	{
		CHKERR(sdr_begin_xn(sdr));
		startedXn = 1;
	}

	/*	Look for the bundle in custody tracking.		*/
	custodyElt = cbr_findCustodyBundle(sdr, sourceEid, seqId, seqNum);

	if (startedXn)
	{
		sdr_exit_xn(sdr);
	}

	if (custodyElt == 0)
	{
		/*	Bundle not in custody tracking.
		 *	This means either:
		 *	- Never tracked (not a custody bundle)
		 *	- Released (CCS accepted received)
		 *
		 *	We can't distinguish these cases without
		 *	additional tracking, so return NOT_FOUND.	*/
		return CBR_CUSTODY_STATUS_NOT_FOUND;
	}

	/*	Bundle is in custody tracking, so we're still
	 *	waiting for the next hop to acknowledge custody.	*/
	return CBR_CUSTODY_STATUS_PENDING;
}
