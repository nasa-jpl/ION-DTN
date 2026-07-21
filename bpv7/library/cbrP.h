/*
 *	cbrP.h:		Private definitions supporting the implementation
 *			of Compressed Bundle Reporting (CBR) and Custody
 *			Transfer (CT) for BPv7.
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

#ifndef _CBRP_H_
#define _CBRP_H_

/*	Include bpP.h first to define Bundle type needed by cbr.h	*/
#include "bpP.h"
#include "cbr.h"

#ifdef __cplusplus
extern "C" {
#endif

/*	*	*	SDR Persistent Structures	*	*	*/

/**
 * Root structure for CBR/CT database stored in SDR.
 * Single instance per BPA, accessed via getCbrConstants().
 */
typedef struct {
	SdrObject	seqCounters;		/* SDR list: BundleSeqCounter */
	SdrObject	pendingCrs;		/* SDR list: PendingSignal for CRS */
	SdrObject	pendingCcs;		/* SDR list: PendingSignal for CCS */
	SdrObject	custodyBundles;		/* SDR list: CustodyBundle */

	/* Custody acceptance whitelist (empty = accept from anyone) */
	SdrObject	custodyAcceptByCustodian; /* SDR list of SDR strings */
	SdrObject	custodyAcceptBySource;	  /* SDR list of SDR strings */

	/* Auto custody-request policy (empty = no auto-request) */
	SdrObject	custodyReqDests;	  /* SDR list of SDR strings */

	/* CRS reception history (ring buffer) */
	SdrObject	crsHistory;		/* SDR list of ReceivedCrsRecord */
	unsigned int	crsHistoryMax;		/* Ring limit; 0 = unlimited */

	/* Configuration */
	unsigned int	crsAggregateLimit;	/* Max bundles before CRS */
	unsigned int	ccsAggregateLimit;	/* Max bundles before CCS */
	unsigned int	aggregateTimeoutSec;	/* Seconds before forced send */

	/* Sequence-counter wraparound width applied to new counters.
	 * One of CBR_COUNTER_MAX_{16,32,64}BIT; 0 => treat as 64-bit. */
	uvast		counterMaxValue;

	/* Retransmission strategy configuration */
	int		retransmitStrategy;	/* CBR_RETX_* */
	unsigned int	retransmitIntervalSec;	/* Timer interval */
	unsigned int	maxRetransmissions;	/* Max attempts (0=unlimited) */

	/* Explicit CREB source EID flag (for testing/interoperability) */
	unsigned int	crebExplicitEid;	/* 1 = include sourceEid in CREB */

	/* Default CREB report-to EID override ("" = use bundle's reportTo) */
	char		crebDefaultReportToEid[MAX_EID_LEN];

	/* Statistics counters (for verification/debugging) */
	unsigned int	ccsAcceptSent;		/* CCS acceptance signals sent */
	unsigned int	ccsRefuseSent;		/* CCS refusal signals sent */
	unsigned int	ccsAcceptRecv;		/* CCS acceptance received */
	unsigned int	ccsRefuseRecv;		/* CCS refusal received */
	unsigned int	custodyOriginated;	/* Bundles originated with custody */
	unsigned int	custodyAccepted;	/* Bundles accepted into custody */
	unsigned int	custodyReleased;	/* Bundles released from custody */
	unsigned int	crsSignalsSent;		/* CRS signals sent */
	unsigned int	crsSignalsRecv;		/* CRS signals received */
} CbrDb;

/**
 * Tracks sequence numbers for a specific source EID and sequence ID.
 * Per Orange Book Section 3: separate counters for custody vs reporting.
 *
 * IMPORTANT: Per Section 3.2.4, Sequence ID '0' is reserved and indicates
 * that separate counters are maintained PER DESTINATION ENDPOINT ID.
 * When seqId == 0, the destEid field is used for counter lookup.
 * When seqId != 0, destEid is ignored (counter is per-seqId only).
 *
 * COUNTER WIDTH AND WRAPAROUND (per Section 3.2.9):
 * The spec allows counters to use 16, 32, or 64-bit maximums for wraparound
 * detection. This implementation uses 64-bit (uvast) by default. When
 * communicating with constrained nodes using smaller counter widths:
 *   - Configure counterMaxValue to match peer's counter width
 *   - Wraparound occurs when nextSeqNum exceeds counterMaxValue
 *   - End-to-End Gap Detection may fail if widths are mismatched
 */
typedef struct {
	SdrObject	sourceEid;	/* SDR string: source endpoint ID */
	uvast		seqId;		/* Sequence identifier (0 = dest-specific) */
	SdrObject	destEid;	/* SDR string: dest EID (when seqId==0) */
	uvast		nextSeqNum;	/* Next sequence number to assign */
	uvast		counterMaxValue;/* Max before wraparound (default: MAX_64) */
	int		forCustody;	/* 0 = status reporting, 1 = custody */
	time_t		lastUsed;	/* For LRU cleanup */
} BundleSeqCounter;

/**
 * Represents a pending CRS or CCS being aggregated before transmission.
 * Grouped by destination, signal type, and status/disposition code.
 */
typedef struct {
	SdrObject	destEid;	/* SDR string: signal destination */
	int		signalType;	/* CBR_SIGNAL_CRS or CBR_SIGNAL_CCS */
	int		statusCode;	/* For CRS: status assertion reason */
	int		dispCode;	/* For CCS: disposition code */
	SdrObject	sequences;	/* SDR list: BundleSequenceEntry */
	time_t		aggregateStart;	/* When aggregation began */
	unsigned int	bundleCount;	/* Number of bundles aggregated */
} PendingSignal;

/**
 * Entry in a bundle sequence collection.
 * Per Orange Book Section 3.3, uses LENGTH-based encoding for wire format.
 *
 * For contiguous sequences: encode as [seqId, seqNumStart, length]
 * For non-contiguous: encode as [seqId, seqNumStart, [range-array]]
 *
 * Internally we track start/length for contiguous; rangeArray for gaps.
 */
typedef struct {
	uvast		seqId;		/* Sequence identifier */
	uvast		seqNumStart;	/* Starting sequence number */
	uvast		length;		/* Number of bundles (contiguous) */
	SdrObject	rangeArray;	/* SDR list for non-contiguous (0 if contiguous) */
	SdrObject	sourceEid;	/* SDR string: source EID */
} BundleSequenceEntry;

/**
 * For non-contiguous sequences, rangeArray contains uvast values
 * representing alternating included/excluded lengths per Orange Book
 * Section 3.3.2:
 *   [included_len, excluded_len, included_len, ...]
 *
 * Example: To represent bundles 100-104, 110-112 (gap at 105-109):
 *   seqNumStart = 100
 *   rangeArray = [5, 5, 3]  ; 5 included, 5 excluded, 3 included
 */

/**
 * Tracks a bundle for which this node has accepted custody.
 * Used for retransmission if custody signal not received.
 */
typedef struct {
	SdrObject	bundleObj;	/* Reference to the Bundle in SDR */
	SdrObject	destEid;	/* SDR string: Next custodian EID */
	uvast		seqId;		/* CTEB sequence identifier */
	uvast		seqNum;		/* CTEB sequence number */
	SdrObject	sourceEid;	/* SDR string: Bundle source EID */
	time_t		custodyAccepted;/* When custody was accepted */
	time_t		lastTransmit;	/* When last transmitted */
	int		retransmitCount;/* Number of retransmissions */
} CustodyBundle;

/*	*	*	Module Access Functions	*	*	*	*/

/**
 * Get the SDR object address of the CbrDb.
 */
extern SdrObject getCbrDbObject(void);

/**
 * Get a pointer to the CbrDb constants (in working memory).
 */
extern CbrDb		*getCbrConstants(void);

/*	*	*	Internal Functions	*	*	*	*/

/*	Sequence Counter Management					*/

/**
 * Find a sequence counter matching the given criteria.
 * Uses the lookup logic defined in Section 3.2.4.
 *
 * @return	SDR list element of matching BundleSeqCounter, 0 if not found
 */
extern SdrObject cbr_findSeqCounter(Sdr sdr, char *sourceEid, char *destEid,
		uvast seqId, int forCustody);

/**
 * Create a new sequence counter.
 *
 * @return	SDR list element of new BundleSeqCounter, 0 on error
 */
extern SdrObject cbr_createSeqCounter(Sdr sdr, char *sourceEid, char *destEid,
		uvast seqId, int forCustody);

/*	Signal Management						*/

/**
 * Find or create a pending signal matching the given criteria.
 *
 * @return	SDR list element of PendingSignal, 0 on error
 */
extern SdrObject cbr_findOrCreatePendingSignal(Sdr sdr, char *destEid,
		int signalType, int statusOrDispCode);

/**
 * Add a bundle sequence to a pending signal's collection.
 * Performs range compression for contiguous sequences.
 */
extern int cbr_addToSignalSequences(Sdr sdr, SdrObject signalElt,
		char *sourceEid, uvast seqId, uvast seqNum);

/**
 * Serialize and transmit a pending signal.
 *
 * @return	0 on success, -1 on error
 */
extern int cbr_transmitSignal(Sdr sdr, SdrObject signalElt);

/*	Custody Bundle Management					*/

/**
 * Find a custody bundle by source EID, seqId, and seqNum.
 *
 * @return	SDR list element of CustodyBundle, 0 if not found
 */
extern SdrObject cbr_findCustodyBundle(Sdr sdr, char *sourceEid, uvast seqId,
		uvast seqNum);

/**
 * Add a bundle to custody tracking.
 *
 * @return	SDR list element of new CustodyBundle, 0 on error
 */
extern SdrObject cbr_trackCustodyBundle(Sdr sdr, SdrObject bundleObj,
		char *destEid, char *sourceEid, uvast seqId, uvast seqNum);

/**
 * Increment the custodyOriginated counter.
 * Called when a bundle is originated with custody transfer at the source.
 */
extern void		cbr_noteCustodyOriginated(Sdr sdr);

/**
 * Remove a bundle from custody tracking.
 */
extern void cbr_untrackCustodyBundle(Sdr sdr, SdrObject custodyElt);

/**
 * Remove custody tracking for a bundle identified by its SDR object address.
 * Called from bpDestroyBundle when a bundle expires or is otherwise destroyed.
 * This ensures custody tracking doesn't hold orphaned references.
 *
 * @param sdr		SDR handle
 * @param bundleObj	SDR object address of the bundle being destroyed
 */
extern void cbr_untrackBundleByObj(Sdr sdr, SdrObject bundleObj);

/*	Retransmission						*/

/**
 * Check for custody bundles needing retransmission.
 * Called periodically by clock thread when strategy is CBR_RETX_TIMER.
 *
 * @return	Number of bundles retransmitted
 */
extern int		cbr_checkRetransmissions(Sdr sdr);

/**
 * Retransmit a specific custody bundle.
 */
extern int cbr_doRetransmit(Sdr sdr, CustodyBundle *cb, SdrObject cbElt);

/*	CBOR Encoding/Decoding					*/

/**
 * Encode a CRS admin record to CBOR.
 * Format: #6.14({ status-reason => Bundle-Sequence-Collection, ... })
 *
 * @return	Bytes written, -1 on error
 */
extern int cbr_encodeCrs(Sdr sdr, SdrObject signalElt, unsigned char *buffer,
		size_t buflen);

/**
 * Decode a CRS admin record from CBOR.
 *
 * @return	0 on success, -1 on error
 */
extern int		cbr_decodeCrs(unsigned char *buffer, size_t length,
				Sdr sdr);

/**
 * Encode a CCS admin record to CBOR.
 * Format: #6.13({ disposition => Bundle-Sequence-Collection, ... })
 *
 * @return	Bytes written, -1 on error
 */
extern int cbr_encodeCcs(Sdr sdr, SdrObject signalElt, unsigned char *buffer,
		size_t buflen);

/**
 * Decode a CCS admin record from CBOR.
 *
 * @return	0 on success, -1 on error
 */
extern int		cbr_decodeCcs(unsigned char *buffer, size_t length,
				Sdr sdr);

/*	Range Compression Utilities					*/

/**
 * Attempt to extend an existing sequence entry with a new sequence number.
 * If the seqNum is consecutive, extends the range. If not, converts to
 * or extends the range array.
 *
 * @return	1 if extended, 0 if new entry needed, -1 on error
 */
extern int cbr_extendSequenceEntry(Sdr sdr, SdrObject entryObj,
		BundleSequenceEntry *entry, uvast seqNum);

/**
 * Convert a simple contiguous entry to a range array.
 * Called when a gap is detected in a previously contiguous sequence.
 */
extern int cbr_convertToRangeArray(Sdr sdr, SdrObject entryObj,
		BundleSequenceEntry *entry, uvast currentLen, uvast gap,
		uvast newLen);

/**
 * Extend an existing range array with a new subsequence.
 */
extern int cbr_extendRangeArray(Sdr sdr, SdrObject rangeArray,
		uvast expectedNext, uvast seqNum);

/*	Admin Endpoint Helpers						*/

/**
 * Get this node's administrative endpoint ID string.
 * Used for source-EID omission logic in signal serialization.
 */
extern char		*cbr_getLocalAdminEid(void);

#ifdef __cplusplus
}
#endif

#endif /* _CBRP_H_ */
