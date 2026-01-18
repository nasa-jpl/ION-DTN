/*
 *	cbr.c:		Core implementation of Compressed Bundle Reporting
 *			(CBR) and Custody Transfer (CT) for BPv7.
 *
 *	Based on CCSDS Orange Book Draft K: Compressed Bundle Status
 *	Reporting and Custody Signaling (August 2025).
 *
 *	Copyright (c) 2026, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
 *
 *	Author: ION Development Team
 */

#include "cbrP.h"
#include "cbor.h"

/*	*	*	Module-Level Variables	*	*	*	*/

static Object	_cbrDbObject(Object *newObj)
{
	static Object	cbrDbObj = 0;

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
	Object		cbrDbObj;

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

Object	getCbrDbObject(void)
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
	Object	cbrDbObj;
	CbrDb	cbrBuf;

	CHKERR(sdr);

	/*	Recover the CBR database if it already exists.		*/

	cbrDbObj = sdr_find(sdr, "cbrdb", NULL);
	if (cbrDbObj == 0)
	{
		/*	CBR database doesn't exist; create it.		*/

		CHKERR(sdr_begin_xn(sdr));
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

		/*	Default configuration			*/
		cbrBuf.crsAggregateLimit = 10;
		cbrBuf.ccsAggregateLimit = 10;
		cbrBuf.aggregateTimeoutSec = 5;
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
	Object	cbrDbObj;

	sdr = getIonsdr();
	if (sdr == NULL)
	{
		putErrmsg("Can't attach to CBR: no SDR.", NULL);
		return -1;
	}

	cbrDbObj = sdr_find(sdr, "cbrdb", NULL);
	if (cbrDbObj == 0)
	{
		putErrmsg("CBR database not found.", NULL);
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
	Object	cbrDbObj;
	CbrDb	cbrBuf;

	CHKERR(sdr);
	cbrDbObj = _cbrDbObject(NULL);
	CHKERR(cbrDbObj);

	CHKERR(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &cbrBuf, cbrDbObj, sizeof(CbrDb));

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

int	cbr_configureRetransmission(Sdr sdr, int strategy,
		unsigned int intervalSec, unsigned int maxRetransmissions)
{
	Object	cbrDbObj;
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
	sdr_read(sdr, (char *) &cbrBuf, cbrDbObj, sizeof(CbrDb));

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
	CbrDb	*cbrConstants;

	(void) sdr;	/*	Needed for interface consistency.	*/
	cbrConstants = _cbrConstants();
	CHKERR(cbrConstants);

	if (crsAggregateLimit)
	{
		*crsAggregateLimit = cbrConstants->crsAggregateLimit;
	}

	if (ccsAggregateLimit)
	{
		*ccsAggregateLimit = cbrConstants->ccsAggregateLimit;
	}

	if (aggregateTimeoutSec)
	{
		*aggregateTimeoutSec = cbrConstants->aggregateTimeoutSec;
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

/*	*	*	Status Report Mode Functions	*	*	*/

int	cbr_getStatusReportMode(Sdr sdr)
{
	Object		bpDbObj;
	BpDB		bpDb;

	(void) sdr;	/*	Needed for interface consistency.	*/
	bpDbObj = getBpDbObject();
	if (bpDbObj == 0)
	{
		putErrmsg("CBR: BpDB not available.", NULL);
		return -1;
	}

	sdr_read(getIonsdr(), (char *) &bpDb, bpDbObj, sizeof(BpDB));
	return (int) bpDb.statusRptMode;
}

int	cbr_setStatusReportMode(Sdr sdr, int mode)
{
	Object		bpDbObj;
	BpDB		bpDb;

	CHKERR(mode >= BP_SR_MODE_TRADITIONAL && mode <= BP_SR_MODE_BOTH);

	bpDbObj = getBpDbObject();
	if (bpDbObj == 0)
	{
		putErrmsg("CBR: BpDB not available.", NULL);
		return -1;
	}

	CHKERR(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &bpDb, bpDbObj, sizeof(BpDB));
	bpDb.statusRptMode = (BpStatusReportMode) mode;
	sdr_write(sdr, bpDbObj, (char *) &bpDb, sizeof(BpDB));

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("CBR: failed to set status report mode.", NULL);
		return -1;
	}

	return 0;
}

/*	*	*	Sequence Counter Functions	*	*	*/

Object	cbr_findSeqCounter(Sdr sdr, char *sourceEid, char *destEid,
		uvast seqId, int forCustody)
{
	CbrDb	*cbrConstants;
	Object	elt;
	Object	counterObj;
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

Object	cbr_createSeqCounter(Sdr sdr, char *sourceEid, char *destEid,
		uvast seqId, int forCustody)
{
	CbrDb		*cbrConstants;
	BundleSeqCounter counter;
	Object		counterObj;
	Object		elt;

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
	counter.counterMaxValue = CBR_COUNTER_MAX_64BIT;
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

Object	cbr_getSeqCounter(Sdr sdr, char *sourceEid, char *destEid,
		uvast seqId, int forCustody)
{
	Object	elt;

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
	Object		counterObj;
	BundleSeqCounter counter;
	uvast		allocatedSeqNum;

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

	CHKERR(sdr_begin_xn(sdr));

	counterObj = cbr_getSeqCounter(sdr, sourceEid, destEid, seqId,
			forCustody);
	if (counterObj == 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't get sequence counter.", sourceEid);
		return -1;
	}

	sdr_read(sdr, (char *) &counter, counterObj, sizeof(BundleSeqCounter));

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

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't allocate sequence number.", NULL);
		return -1;
	}

	*seqNum = allocatedSeqNum;
	return 0;
}

/*	*	*	Range Compression Functions	*	*	*/

int	cbr_extendSequenceEntry(Sdr sdr, Object entryObj,
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

int	cbr_convertToRangeArray(Sdr sdr, Object entryObj,
		BundleSequenceEntry *entry, uvast currentLen,
		uvast gap, uvast newLen)
{
	Object	rangeArray;
	Object	lenObj;
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

int	cbr_extendRangeArray(Sdr sdr, Object rangeArray,
		uvast seqNumStart, uvast seqNum)
{
	Object	elt;
	Object	lastElt;
	Object	lenObj;
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

	/*	Re-read last element					*/
	lenObj = sdr_list_data(sdr, lastElt);
	sdr_read(sdr, (char *) &lenBuf, lenObj, sizeof(uvast));

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
	/*	  1. Source matches signal destination			*/
	/*	  2. Source matches local admin endpoint		*/
	if (sourceEid && signalDestEid)
	{
		if (strcmp(sourceEid, signalDestEid) == 0)
		{
			includeSourceEid = 0;
		}
	}

	if (includeSourceEid && sourceEid && localAdminEid)
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

	/*	Item 1: seqId						*/
	if (cbor_encode_integer(seqId, &cursor) < 1)
	{
		return -1;
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

	/*	Item 4 (optional): sourceEid				*/
	if (includeSourceEid && sourceEid)
	{
		/*	Encode EID as text string			*/
		if (cbor_encode_text_string(sourceEid, strlen(sourceEid),
				&cursor) < 1)
		{
			return -1;
		}
	}

	return cursor - buffer;
}

int	cbr_decodeBundleSequence(unsigned char **cursor,
		unsigned int *bytesRemaining, uvast *seqId,
		uvast *seqNumStart, uvast *length,
		uvast **rangeArray, int *rangeCount, char **sourceEid)
{
	uvast		arrayLen;
	int		majorType;
	uvast		rangeLen;
	int		i;
	char		eidBuf[MAX_EID_LEN];
	uvast		eidLen;

	*rangeArray = NULL;
	*rangeCount = 0;
	*sourceEid = NULL;

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

	/*	Item 1: seqId						*/
	if (cbor_decode_integer(seqId, CborAny, cursor, bytesRemaining) < 1)
	{
		return -1;
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

	/*	Item 4 (optional): sourceEid				*/
	if (arrayLen >= 4)
	{
		eidLen = MAX_EID_LEN - 1;
		if (cbor_decode_text_string(eidBuf, &eidLen, cursor,
				bytesRemaining) < 1)
		{
			return -1;
		}

		*sourceEid = MTAKE(eidLen + 1);
		if (*sourceEid == NULL)
		{
			putErrmsg("No memory for source EID.", NULL);
			return -1;
		}

		memcpy(*sourceEid, eidBuf, eidLen);
		(*sourceEid)[eidLen] = '\0';
	}

	return 0;
}

/*	*	*	Signal Management Functions	*	*	*/

Object	cbr_findOrCreatePendingSignal(Sdr sdr, char *destEid,
		int signalType, int statusOrDispCode)
{
	CbrDb		*cbrConstants;
	Object		signalList;
	Object		elt;
	Object		signalObj;
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

int	cbr_addToSignalSequences(Sdr sdr, Object signalElt,
		char *sourceEid, uvast seqId, uvast seqNum)
{
	Object			signalObj;
	PendingSignal		signal;
	Object			seqElt;
	Object			entryObj;
	BundleSequenceEntry	entry;
	char			srcEidBuf[MAX_EID_LEN];
	int			extended;

	signalObj = sdr_list_data(sdr, signalElt);
	sdr_read(sdr, (char *) &signal, signalObj, sizeof(PendingSignal));

	/*	Search for existing entry with matching sourceEid and seqId */
	for (seqElt = sdr_list_first(sdr, signal.sequences);
			seqElt; seqElt = sdr_list_next(sdr, seqElt))
	{
		entryObj = sdr_list_data(sdr, seqElt);
		sdr_read(sdr, (char *) &entry, entryObj,
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
	Object	elt;
	Object	nextElt;
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

int	cbr_processTimeouts(Sdr sdr)
{
	CbrDb		*cbrConstants;
	time_t		now;
	Object		elt;
	Object		nextElt;
	Object		signalObj;
	PendingSignal	signal;
	int		count = 0;

	CHKERR(sdr);
	cbrConstants = _cbrConstants();
	CHKERR(cbrConstants);

	now = getCtime();

	CHKERR(sdr_begin_xn(sdr));

	/*	Check CRS timeouts					*/
	for (elt = sdr_list_first(sdr, cbrConstants->pendingCrs);
			elt; elt = nextElt)
	{
		nextElt = sdr_list_next(sdr, elt);
		signalObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &signal, signalObj,
				sizeof(PendingSignal));

		if (now - signal.aggregateStart >=
				(time_t) cbrConstants->aggregateTimeoutSec)
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

		if (now - signal.aggregateStart >=
				(time_t) cbrConstants->aggregateTimeoutSec)
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

	return count;
}

/*	Maximum buffer size for CRS/CCS encoding.			*/
#define CBR_MAX_SIGNAL_SIZE	8192

int	cbr_encodeCrs(Sdr sdr, Object signalElt, unsigned char *buffer,
		size_t buflen)
{
	Object			signalObj;
	PendingSignal		signal;
	unsigned char		*cursor = buffer;
	uvast			uvtemp;
	Object			seqElt;
	Object			entryObj;
	BundleSequenceEntry	entry;
	char			srcEidBuf[MAX_EID_LEN];
	char			destEidBuf[MAX_EID_LEN];
	int			seqBytes;
	uvast			*rangeData = NULL;
	int			rangeCount = 0;
	Object			rangeElt;
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
				Object	lenObj = sdr_list_data(sdr, rangeElt);

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

static void	cleanupSignal(Sdr sdr, Object signalElt)
{
	Object			signalObj;
	PendingSignal		signal;
	Object			seqElt;
	Object			entryObj;
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
			Object	rangeElt;
			Object	nextRangeElt;

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

int	cbr_transmitSignal(Sdr sdr, Object signalElt)
{
	Object			signalObj;
	PendingSignal		signal;
	char			destEidBuf[MAX_EID_LEN];
	unsigned char		*buffer;
	int			buflen;
	Object			payloadZco;
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
	else
	{
		/*	CCS encoding - Phase 5				*/
		writeMemo("[i] CCS transmission not yet implemented.");
		MRELEASE(buffer);
		cleanupSignal(sdr, signalElt);
		return 0;
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

	if (payloadZco == 0 || payloadZco == (Object) ERROR)
	{
		putErrmsg("Can't create ZCO for CRS.", NULL);
		return -1;
	}

	/*	Send as admin bundle					*/
	result = bpSend(NULL, destEidBuf, NULL, 3600, BP_STD_PRIORITY,
			NoCustodyRequested, 0, 0, &ancillary, payloadZco,
			NULL, CBR_ADMIN_RECORD_CRS);

	if (result < 0)
	{
		putErrmsg("Can't send CRS bundle.", destEidBuf);
		/*	ZCO already released by bpSend on failure	*/
	}
	else
	{
		writeMemoNote("[i] CBR CRS transmitted to", destEidBuf);
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
	Object		signalElt;
	CbrDb		*cbrConstants;
	PendingSignal	signal;
	Object		signalObj;
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

	/*	Check if aggregation limit reached			*/
	cbrConstants = _cbrConstants();
	signalObj = sdr_list_data(sdr, signalElt);
	sdr_read(sdr, (char *) &signal, signalObj, sizeof(PendingSignal));

	if (cbrConstants->crsAggregateLimit > 0 &&
			signal.bundleCount >= cbrConstants->crsAggregateLimit)
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

/*	Note: Full custody implementation is Phase 4-5.			*/
/*	Phase 1 provides stubs to establish the API.			*/

int	cbr_acceptCustody(Sdr sdr, Bundle *bundle, CtebBlk *cteb)
{
	(void) sdr;
	(void) bundle;
	(void) cteb;
	/*	TODO: Phase 5 - Full custody acceptance implementation	*/
	writeMemo("[i] cbr_acceptCustody: Not yet implemented.");
	return 0;
}

int	cbr_refuseCustody(Sdr sdr, Bundle *bundle, CtebBlk *cteb,
		CbrRefuseReason reason)
{
	(void) sdr;
	(void) bundle;
	(void) cteb;
	(void) reason;
	/*	TODO: Phase 5 - Full custody refusal implementation	*/
	writeMemo("[i] cbr_refuseCustody: Not yet implemented.");
	return 0;
}

int	cbr_releaseCustody(Sdr sdr, char *sourceEid, uvast seqId,
		uvast seqNumStart, uvast length)
{
	(void) sdr;
	(void) sourceEid;
	(void) seqId;
	(void) seqNumStart;
	(void) length;
	/*	TODO: Phase 5 - Full custody release implementation	*/
	writeMemo("[i] cbr_releaseCustody: Not yet implemented.");
	return 0;
}

int	cbr_handleCrs(Sdr sdr, unsigned char *adminRecord, int length)
{
	unsigned char	*cursor = adminRecord;
	unsigned int	unparsedBytes = length;
	uvast		mapLen;
	uvast		statusCode;
	uvast		arrayLen;
	uvast		seqId;
	uvast		seqNumStart;
	uvast		bundleLen;
	uvast		*rangeArray;
	int		rangeCount;
	char		*sourceEid;
	uvast		i;

	(void) sdr;		/*	May use later for state tracking. */

	/*	CRS content format (after stripping admin record header):
	 *	{ status-reason => [Bundle-Sequence, ...], ... }	*/

	/*	Decode map						*/
	mapLen = 0;
	if (cbor_decode_map_open(&mapLen, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] CRS: Can't decode map open.");
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
			return -1;
		}

		/*	Value: array of Bundle-Sequence			*/
		arrayLen = 0;
		if (cbor_decode_array_open(&arrayLen, &cursor,
				&unparsedBytes) < 1)
		{
			writeMemo("[?] CRS: Can't decode sequence array.");
			return -1;
		}

		/*	Decode each Bundle-Sequence			*/
		while (arrayLen > 0)
		{
			if (cbr_decodeBundleSequence(&cursor, &unparsedBytes,
					&seqId, &seqNumStart, &bundleLen,
					&rangeArray, &rangeCount,
					&sourceEid) < 0)
			{
				writeMemo("[?] CRS: Can't decode Bundle-Sequence.");
				return -1;
			}

			/*	Log received CRS info			*/
			writeMemoNote("[i] CRS received: status",
					itoa(statusCode));

			/*	For now, just log. Future: could track
			 *	for end-to-end acknowledgment.		*/

			/*	Clean up				*/
			if (rangeArray)
			{
				MRELEASE(rangeArray);
			}

			if (sourceEid)
			{
				MRELEASE(sourceEid);
			}

			arrayLen--;
		}
	}

	return 0;
}

int	cbr_handleCcs(Sdr sdr, unsigned char *adminRecord, int length)
{
	(void) sdr;
	(void) adminRecord;
	(void) length;
	/*	TODO: Phase 5 - CCS reception handling			*/
	writeMemo("[i] cbr_handleCcs: Not yet implemented.");
	return 0;
}

/*	*	*	Manual Retransmission Functions	*	*	*/

int	cbr_retransmitBundle(Sdr sdr, char *sourceEid, uvast seqId,
		uvast seqNum)
{
	(void) sdr;
	(void) sourceEid;
	(void) seqId;
	(void) seqNum;
	/*	TODO: Phase 5 - Manual retransmission			*/
	writeMemo("[i] cbr_retransmitBundle: Not yet implemented.");
	return 0;
}

int	cbr_retransmitAllCustody(Sdr sdr, char *destEid)
{
	(void) sdr;
	(void) destEid;
	/*	TODO: Phase 5 - Bulk manual retransmission		*/
	writeMemo("[i] cbr_retransmitAllCustody: Not yet implemented.");
	return 0;
}

int	cbr_listCustodyBundles(Sdr sdr, CbrCustodyCallback callback,
		void *userData)
{
	(void) sdr;
	(void) callback;
	(void) userData;
	/*	TODO: Phase 5 - Custody bundle listing			*/
	writeMemo("[i] cbr_listCustodyBundles: Not yet implemented.");
	return 0;
}
