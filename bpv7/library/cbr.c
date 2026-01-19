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

int	cbr_encodeCcs(Sdr sdr, Object signalElt, unsigned char *buffer,
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

	if (payloadZco == 0 || payloadZco == (Object) ERROR)
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
		writeMemoNote(signal.signalType == CBR_SIGNAL_CRS ?
				"[i] CBR CRS transmitted to" :
				"[i] CBR CCS transmitted to", destEidBuf);
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

/*	Internal helpers for custody bundle tracking.			*/

/**
 * Find a custody bundle by source EID, seqId, and seqNum.
 *
 * @return	SDR list element of CustodyBundle, 0 if not found
 */
Object	cbr_findCustodyBundle(Sdr sdr, char *sourceEid, uvast seqId,
		uvast seqNum)
{
	CbrDb		*cbrConstants;
	Object		elt;
	Object		cbObj;
	CustodyBundle	cb;
	char		eidBuf[MAX_EID_LEN];

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
Object	cbr_trackCustodyBundle(Sdr sdr, Object bundleObj, char *destEid,
		char *sourceEid, uvast seqId, uvast seqNum)
{
	CbrDb		*cbrConstants;
	CustodyBundle	cb;
	Object		cbObj;
	Object		elt;

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

/**
 * Remove a bundle from custody tracking.
 */
void	cbr_untrackCustodyBundle(Sdr sdr, Object custodyElt)
{
	Object		cbObj;
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
 * Remove custody tracking for a bundle identified by its SDR object address.
 * Called from bpDestroyBundle when a bundle expires or is otherwise destroyed.
 * This ensures custody tracking doesn't hold orphaned references.
 */
void	cbr_untrackBundleByObj(Sdr sdr, Object bundleObj)
{
	CbrDb		*cbrConstants;
	Object		elt;
	Object		nextElt;
	Object		cbObj;
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
			/*	Found matching entry. Remove it.	*/
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
	Object		signalElt;
	CbrDb		*cbrConstants;
	PendingSignal	signal;

	cbrConstants = getCbrConstants();
	if (cbrConstants == NULL)
	{
		return -1;
	}

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

	/*	Check if we should transmit immediately.		*/
	sdr_read(sdr, (char *) &signal,
			sdr_list_data(sdr, signalElt), sizeof(PendingSignal));

	if (signal.bundleCount >= cbrConstants->ccsAggregateLimit)
	{
		if (cbr_transmitSignal(sdr, signalElt) < 0)
		{
			putErrmsg("CBR: Can't transmit CCS.", NULL);
			return -1;
		}
	}

	return 0;
}

int	cbr_acceptCustody(Sdr sdr, Bundle *bundle, CtebBlk *cteb)
{
	char		*sourceEid = NULL;
	Object		bundleAddr;
	int		result;

	CHKERR(cteb);
	CHKERR(cteb->custodianEid);

	/*	Get source EID from bundle.				*/
	readEid(&bundle->id.source, &sourceEid);
	if (sourceEid == NULL)
	{
		putErrmsg("CBR: Can't read bundle source EID.", NULL);
		return -1;
	}

	/*	Get bundle address for tracking.			*/
	/*	NOTE: In a full implementation, we'd get this from
	 *	the bundle's hash entry or similar. For now, we use
	 *	a simplified approach - the bundle object address
	 *	should be passed from the caller context.		*/
	bundleAddr = 0;		/*	Placeholder - see note above.	*/

	/*	Add bundle to custody tracking.				*/
	if (cbr_trackCustodyBundle(sdr, bundleAddr,
			bundle->proxNodeEid ? "" : cteb->custodianEid,
			sourceEid, cteb->seqId, cteb->seqNum) == 0)
	{
		/*	May already be tracked or allocation failed.	*/
		writeMemoNote("[?] CBR: Failed to track custody bundle",
				sourceEid);
		/*	Continue anyway to send CCS.			*/
	}

	/*	Queue CCS acceptance to previous custodian.		*/
	result = queueCcs(sdr, cteb->custodianEid, sourceEid,
			cteb->seqId, cteb->seqNum, CBR_CUSTODY_ACCEPTED);

	MRELEASE(sourceEid);

	if (result < 0)
	{
		putErrmsg("CBR: Can't queue CCS acceptance.", NULL);
		return -1;
	}

	writeMemo("[i] CBR: Custody accepted.");
	return 0;
}

int	cbr_refuseCustody(Sdr sdr, Bundle *bundle, CtebBlk *cteb,
		CbrRefuseReason reason)
{
	char		*sourceEid = NULL;
	int		result;

	(void) bundle;		/*	May use for logging later.	*/

	CHKERR(cteb);
	CHKERR(cteb->custodianEid);

	/*	Get source EID from bundle.				*/
	readEid(&bundle->id.source, &sourceEid);
	if (sourceEid == NULL)
	{
		putErrmsg("CBR: Can't read bundle source EID.", NULL);
		return -1;
	}

	/*	Log refusal reason locally (not transmitted per spec).	*/
	writeMemoNote("[i] CBR: Custody refused, reason", itoa(reason));

	/*	Queue CCS refusal to previous custodian.
	 *	Per Orange Book, refusal is just -1 with no reason.	*/
	result = queueCcs(sdr, cteb->custodianEid, sourceEid,
			cteb->seqId, cteb->seqNum, CBR_CUSTODY_REFUSED);

	MRELEASE(sourceEid);

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
	uvast	i;
	Object	custodyElt;

	/*	Release custody for each bundle in the range.		*/
	for (i = 0; i < length; i++)
	{
		custodyElt = cbr_findCustodyBundle(sdr, sourceEid, seqId,
				seqNumStart + i);
		if (custodyElt != 0)
		{
			cbr_untrackCustodyBundle(sdr, custodyElt);
			writeMemoNote("[i] CBR: Custody released for seqNum",
					itoa(seqNumStart + i));
		}
	}

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
	uvast		i;
	uvast		j;
	uvast		rangeStart;
	uvast		rangeLen;

	/*	CCS content format (after stripping admin record header):
	 *	{ disposition => [Bundle-Sequence, ...], ... }
	 *
	 *	Disposition is SIGNED: 1=accepted, -1=refused		*/

	/*	Decode map						*/
	mapLen = 0;
	if (cbor_decode_map_open(&mapLen, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] CCS: Can't decode map open.");
		return -1;
	}

	/*	Process each disposition entry				*/
	for (i = 0; i < mapLen; i++)
	{
		/*	Key: disposition code (signed integer)
		 *	CBOR signed int: positive = unsigned, negative = -1-n
		 *	We need to handle this specially.		*/

		/*	For simplicity, decode as unsigned first.	*/
		if (cbor_decode_integer((uvast *)&disposition, CborAny, &cursor,
				&unparsedBytes) < 1)
		{
			writeMemo("[?] CCS: Can't decode disposition.");
			return -1;
		}

		/*	CBOR negative int encoding: if major type 1,
		 *	value is -1-n. For now we just check the value.
		 *	Disposition 1 = accepted, -1 = refused.		*/

		/*	Value: array of Bundle-Sequence			*/
		arrayLen = 0;
		if (cbor_decode_array_open(&arrayLen, &cursor,
				&unparsedBytes) < 1)
		{
			writeMemo("[?] CCS: Can't decode sequence array.");
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
				writeMemo("[?] CCS: Can't decode Bundle-Sequence.");
				return -1;
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
			}
			else
			{
				/*	Custody refused.
				 *	Bundle will be retransmitted per
				 *	configured strategy.		*/
				writeMemoNote("[i] CCS: Custody refused by next hop",
						sourceEid ? sourceEid : "(null)");

				/*	TODO: Trigger retransmission based
				 *	on configured strategy.		*/
			}

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

/*	*	*	Manual Retransmission Functions	*	*	*/

int	cbr_retransmitBundle(Sdr sdr, char *sourceEid, uvast seqId,
		uvast seqNum)
{
	Object		custodyElt;
	Object		cbObj;
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
	sdr_read(sdr, (char *) &cb, cbObj, sizeof(CustodyBundle));

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
	Object		elt;
	Object		nextElt;
	Object		cbObj;
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
		sdr_read(sdr, (char *) &cb, cbObj, sizeof(CustodyBundle));

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
	Object		elt;
	Object		cbObj;
	CustodyBundle	cb;
	CbrCustodyInfo	info;
	char		destEidBuf[MAX_EID_LEN];
	int		count = 0;

	CHKERR(callback);

	cbrConstants = getCbrConstants();
	if (cbrConstants == NULL)
	{
		return -1;
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

	return count;
}

int	cbr_getCustodyStatus(Sdr sdr, char *sourceEid, uvast seqId,
		uvast seqNum)
{
	Object		custodyElt;

	/*	Look for the bundle in custody tracking.		*/
	custodyElt = cbr_findCustodyBundle(sdr, sourceEid, seqId, seqNum);
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
