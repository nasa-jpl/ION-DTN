/*
 *	cbr.h:		Public API for Compressed Bundle Reporting (CBR)
 *			and Custody Transfer (CT) for BPv7.
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

#ifndef _CBR_H_
#define _CBR_H_

#include "bp.h"

/*	NOTE: Functions in this header that take Bundle* parameters
 *	are internal APIs called from within libbpP.c. Code using
 *	these functions must include bpP.h before cbr.h to get the
 *	Bundle type definition.
 *
 *	For external applications, only use the configuration and
 *	query functions that do not require Bundle*.			*/

#ifdef __cplusplus
extern "C" {
#endif

/*	*	*	Block and Admin Record Type Numbers	*	*/

/*	Extension Block Types (per Orange Book)				*/
#define CBR_BLOCK_TYPE_CTEB		13	/* Custody Transfer Ext Block */
#define CBR_BLOCK_TYPE_CREB		14	/* Compressed Reporting Ext Block */

/*	Admin Record Types (per Orange Book)				*/
#define CBR_ADMIN_RECORD_CCS		13	/* Compressed Custody Signal */
#define CBR_ADMIN_RECORD_CRS		14	/* Compressed Reporting Signal */

/*	*	*	Retransmission Strategies	*	*	*/

/*
 * Per Orange Book, implementations may use timer-based, signal-based, or
 * command-based (manual) retransmission strategies.
 */
#define CBR_RETX_NONE			0	/* Manual/command-based (DEFAULT) */
#define CBR_RETX_TIMER			1	/* Automatic timer-based */
#define CBR_RETX_SIGNAL			2	/* Signal-based (on CCS refusal) */

/*	*	*	Status Reason Codes (CRS)	*	*	*/

/*
 * Status reason codes for CRS, per CCSDS 734.6-O-1 Section 5.2.3.
 *
 * Issue 1 removed the "reassembly" reason that existed in the earlier
 * draft and shifted custody acceptance/rejection down to codes 4 and 5;
 * codes above 6 are reserved for future use.
 */
#define CBR_STATUS_RECEIVED		0	/* Bundle was received */
#define CBR_STATUS_FORWARDED		1	/* Bundle was forwarded */
#define CBR_STATUS_DELIVERED		2	/* Bundle was delivered */
#define CBR_STATUS_DELETED		3	/* Bundle was deleted */
#define CBR_STATUS_CUSTODY_ACCEPTED	4	/* Custody acceptance (info) */
#define CBR_STATUS_CUSTODY_REFUSED	5	/* Custody refusal (info) */

/*	*	*	Custody Disposition Codes (CCS)	*	*	*/

/*
 * Per Orange Book, disposition uses SIGNED integers:
 *   Positive 1: custody accepted
 *   Negative -1: custody refused (no further information)
 *   Values below -1: reserved for future use
 *
 * This implementation uses ONLY spec-compliant values to ensure interoperability.
 * All custody refusals are encoded as -1 regardless of internal reason.
 */
#define CBR_CUSTODY_ACCEPTED		1	/* CBOR: 1 */
#define CBR_CUSTODY_REFUSED		(-1)	/* CBOR: -1 */

/*	*	*	Custody Query Status Results	*	*	*/

/*
 * Return values from cbr_getCustodyStatus().
 * Used to query the state of a bundle in the custody system.
 */
#define CBR_CUSTODY_STATUS_NOT_FOUND	0	/* Bundle not in custody tracking */
#define CBR_CUSTODY_STATUS_PENDING	1	/* Waiting for CCS from next hop */
#define CBR_CUSTODY_STATUS_RELEASED	2	/* CCS accepted received, released */
#define CBR_CUSTODY_STATUS_REFUSED	3	/* CCS refused received */

/*	*	*	Signal Types	*	*	*	*	*/

#define CBR_SIGNAL_CRS			1
#define CBR_SIGNAL_CCS			2
#define CBR_SIGNAL_ALL			3

/*	*	*	CREB Request Flags	*	*	*	*/

/*
 * CREB Status Report Request Flags (bitfield).
 * Per Orange Book Section 5.1.4.1, bit assignments are:
 *   Bit 0: Reception
 *   Bit 1: Forwarding
 *   Bit 2: Delivery
 *   Bit 3: Deletion
 *   Bit 4: Custody Acceptance
 *   Bit 5: Custody Rejection
 */
#define CREB_REQUEST_RECEPTION		0x01	/* Bit 0: Report reception */
#define CREB_REQUEST_FORWARDING		0x02	/* Bit 1: Report forwarding */
#define CREB_REQUEST_DELIVERY		0x04	/* Bit 2: Report delivery */
#define CREB_REQUEST_DELETION		0x08	/* Bit 3: Report deletion */
#define CREB_REQUEST_CUSTODY_ACCEPT	0x10	/* Bit 4: Report custody accept */
#define CREB_REQUEST_CUSTODY_REFUSE	0x20	/* Bit 5: Report custody reject */

/*	*	*	Block Processing Flags	*	*	*	*/

/*
 * Block processing control flags for CREB/CTEB.
 * Per BPv7 RFC 9171 Section 4.2.4, these flags ensure non-supporting
 * nodes forward the blocks transparently:
 *   Bit 0 (0x01): Block must be replicated in every fragment
 *   Bit 1 (0x02): Transmit status report if block can't be processed
 *   Bit 2 (0x04): Delete bundle if block can't be processed
 *   Bit 4 (0x10): Discard block if it can't be processed
 *
 * For transparent forwarding, we set bit 0 and clear bits 2 and 4.
 */
#define CBR_BLOCK_FLAGS			0x01	/* Replicate, allow forward */

/*	*	*	Counter Maximum Values	*	*	*	*/

/*
 * Predefined counter maximum values for interoperability.
 * Per Orange Book Section 3.2.9, counters may use 16, 32, or 64-bit
 * maximums for wraparound detection. Configure to match peer capabilities.
 */
#define CBR_COUNTER_MAX_16BIT		0xFFFFULL		/* 65535 */
#define CBR_COUNTER_MAX_32BIT		0xFFFFFFFFULL		/* 4294967295 */
#define CBR_COUNTER_MAX_64BIT		0xFFFFFFFFFFFFFFFFULL	/* Default */

/*	*	*	Error Codes	*	*	*	*	*/

#define CBR_OK				0
#define CBR_ERR_SDR			(-1)	/* SDR operation failed */
#define CBR_ERR_MEMORY			(-2)	/* Memory allocation failed */
#define CBR_ERR_CBOR			(-3)	/* CBOR encoding/decoding error */
#define CBR_ERR_INVALID_PARAM		(-4)	/* Invalid parameter */
#define CBR_ERR_NOT_FOUND		(-5)	/* Entry not found */
#define CBR_ERR_CUSTODY_FULL		(-6)	/* Custody bundle limit reached */

/*	*	*	Internal Refusal Reasons	*	*	*/

/*
 * Internal refusal reason codes (for local logging/diagnostics only).
 * These are NOT transmitted on the wire - all refusals use -1 in CCS.
 */
typedef enum {
	CbrRefuseUnspecified = 0,
	CbrRefuseDepleted,		/* Resources depleted */
	CbrRefuseNoRoute,		/* No known route to destination */
	CbrRefuseNoContact,		/* No timely contact available */
	CbrRefuseBlockUnintel,		/* Extension block unintelligible */
	CbrRefusePolicy			/* Local policy rejection */
} CbrRefuseReason;

/*	*	*	In-Memory Structures	*	*	*	*/

/*
 * In-memory representation of CREB block content.
 *
 * Per Orange Book Section 5.1.3/5.1.4, the CBOR array structure is:
 *   Array len 1: [seq_num]
 *   Array len 2: [seq_num, seq_id]
 *   Array len 3: [seq_num, seq_id, request_flags]
 *   Array len 4: [seq_num, seq_id, request_flags, source_eid]
 *   Array len 5: [seq_num, seq_id, request_flags, source_eid, report_to_eid]
 *
 * IMPORTANT: request_flags appears at index 2 (after seq_id), NOT at index 1.
 */
typedef struct {
	uvast		seqNum;		/* Bundle sequence number (always present) */
	uvast		seqId;		/* Sequence identifier (array len >= 2) */
	unsigned char	requestFlags;	/* Status report request flags (len >= 3) */
	char		*sourceEid;	/* Block source EID (array len >= 4) */
	char		*reportToEid;	/* Report-to EID (array len 5) */
	int		arrayLen;	/* Actual CBOR array length (1-5) */
} CrebBlk;

/*
 * In-memory representation of CTEB block content.
 * CBOR: [seq_num, seq_id, custodian_eid]
 */
typedef struct {
	uvast		seqNum;		/* Bundle sequence number */
	uvast		seqId;		/* Sequence identifier */
	char		*custodianEid;	/* Custodian EID (block source) */
} CtebBlk;

/*
 * One entry in the received-CRS history log.
 * One record is created per status-code per received CRS signal.
 * Stored persistently in CbrDb.crsHistory.
 */
typedef struct {
	time_t		receivedAt;	/* Wall-clock time of reception */
	SdrObject	senderEid;	/* SDR string (0 if sender unknown) */
	int		statusCode;	/* CBR_STATUS_* value */
	uvast		bundleCount;	/* Sum of bundleLen per entry;
					   range-array entries counted as 1 */
} ReceivedCrsRecord;

/*
 * Information about a bundle under custody.
 * Used for listing and monitoring custody bundles.
 */
typedef struct {
	char		*destEid;	/* Next custodian EID */
	uvast		seqId;		/* CTEB sequence identifier */
	uvast		seqNum;		/* CTEB sequence number */
	time_t		custodyAccepted;/* When custody was accepted */
	time_t		lastTransmit;	/* When last transmitted */
	int		retransmitCount;/* Number of retransmissions */
} CbrCustodyInfo;

/*	*	*	Function Prototypes	*	*	*	*/

/*	Initialization and Shutdown					*/

/**
 * Initialize CBR/CT subsystem.
 * Must be called during BPA startup after SDR is available.
 *
 * @return	0 on success, -1 on error
 */
extern int		cbr_initialize(Sdr sdr);

/**
 * Shutdown CBR/CT subsystem.
 * Flushes pending signals and releases resources.
 */
extern void		cbr_shutdown(Sdr sdr);

/**
 * Attach to CBR/CT subsystem (for worker processes).
 *
 * @return	0 on success, -1 on error
 */
extern int		cbr_attach(void);

/*	Sequence Management						*/

/**
 * Allocate a sequence number for a bundle.
 *
 * Per Orange Book Section 3.2.4, Sequence ID '0' is reserved and indicates
 * destination-specific counters. When seqId == 0, destEid MUST be provided
 * and is used for counter lookup. When seqId != 0, destEid is ignored.
 *
 * @param sdr		SDR handle
 * @param sourceEid	Source endpoint ID
 * @param destEid	Destination endpoint ID (REQUIRED when seqId == 0)
 * @param seqId		Sequence identifier (0 = destination-specific counter)
 * @param forCustody	0 for status reporting, 1 for custody
 * @param seqNum	Output: allocated sequence number
 * @return		0 on success, -1 on error
 */
extern int		cbr_allocateSeqNum(Sdr sdr, char *sourceEid,
				char *destEid, uvast seqId, int forCustody,
				uvast *seqNum);

/**
 * Get or create a sequence counter for the given parameters.
 *
 * Counter lookup logic per Section 3.2.4:
 *   If seqId == 0: lookup by (sourceEid, destEid, forCustody)
 *   If seqId != 0: lookup by (sourceEid, seqId, forCustody)
 *
 * @param sdr		SDR handle
 * @param sourceEid	Source endpoint ID
 * @param destEid	Destination endpoint ID (REQUIRED when seqId == 0)
 * @param seqId		Sequence identifier (0 = destination-specific)
 * @param forCustody	0 for status reporting, 1 for custody
 * @return		SDR Object of BundleSeqCounter, 0 on error
 */
extern SdrObject cbr_getSeqCounter(Sdr sdr, char *sourceEid, char *destEid,
		uvast seqId, int forCustody);

/*	Status Reporting (CRS)						*/

/**
 * Report a status assertion for a bundle.
 * Status will be aggregated and sent as CRS.
 *
 * @param sdr		SDR handle
 * @param bundle	Bundle for which to report status
 * @param statusReason	Status reason code (CBR_STATUS_*)
 * @param creb		CREB block data (NULL if no CREB present)
 * @return		0 on success, -1 on error
 */
extern int		cbr_reportStatus(Sdr sdr, Bundle *bundle,
				int statusReason, CrebBlk *creb);

/*	Custody Transfer (CCS)						*/

/**
 * Custody acceptance whitelist management.
 *
 * When forCustodian=1 the custodian-EID list is affected;
 * when forCustodian=0 the source-EID list is affected.
 * An empty list means "accept from anyone" for that dimension.
 * Both dimensions must pass for custody to be accepted.
 */

/** Returns the SDR list SdrObject for the given whitelist dimension. */
extern SdrObject cbr_getCustodyAcceptList(Sdr sdr, int forCustodian);

extern int		cbr_addCustodyAccept(Sdr sdr, int forCustodian,
					const char *eid);
extern int		cbr_removeCustodyAccept(Sdr sdr, int forCustodian,
					const char *eid);

/**
 * Returns 1 if custody should be accepted from this custodian/source pair,
 * 0 if it should be refused.  Either EID may be NULL (that dimension is
 * skipped).
 */
extern int		cbr_isCustodyAccepted(Sdr sdr,
					const char *custodianEid,
					const char *sourceEid);

/*	Auto Custody-Request Policy					*/

/**
 * Returns 1 if custody transfer should be automatically requested for
 * bundles destined to destEid, 0 otherwise.
 * An empty custodyReqDests list means no auto-request policy is active.
 */
extern int		cbr_isCustodyRequired(Sdr sdr, const char *destEid);

/** Returns the SDR list SdrObject for the custody-request destination list. */
extern SdrObject cbr_getCustodyReqList(Sdr sdr);

extern int		cbr_addCustodyReq(Sdr sdr, const char *eid);
extern int		cbr_removeCustodyReq(Sdr sdr, const char *eid);

/**
 * Accept custody of a bundle.
 * Adds bundle to custody tracking and queues acceptance signal.
 *
 * @param sdr		SDR handle
 * @param bundle	Bundle to take custody of
 * @param bundleAddr	SDR address of bundle (for custody tracking)
 * @param cteb		CTEB block data
 * @return		0 on success, -1 on error
 */
extern int cbr_acceptCustody(Sdr sdr, Bundle *bundle, SdrObject bundleAddr,
		CtebBlk *cteb);

/**
 * Refuse custody of a bundle.
 * Queues refusal signal. Reason is logged locally but not transmitted.
 *
 * @param sdr		SDR handle
 * @param bundle	Bundle to refuse custody of
 * @param cteb		CTEB block data
 * @param reason	Internal refusal reason (for local logging)
 * @return		0 on success, -1 on error
 */
extern int		cbr_refuseCustody(Sdr sdr, Bundle *bundle,
				CtebBlk *cteb, CbrRefuseReason reason);

/**
 * Release custody of a bundle.
 * Called when CCS received indicating next hop accepted custody.
 *
 * @param sdr		SDR handle
 * @param sourceEid	Source EID from CCS
 * @param seqId		Sequence identifier from CCS
 * @param seqNumStart	Start of sequence range
 * @param length	Number of bundles in range
 * @return		Number of bundles released, -1 on error
 */
extern int		cbr_releaseCustody(Sdr sdr, char *sourceEid,
				uvast seqId, uvast seqNumStart, uvast length);

/*	Signal Transmission						*/

/**
 * Flush pending signals for immediate transmission.
 * Called periodically or when aggregation limits reached.
 *
 * @param sdr		SDR handle
 * @param signalType	CBR_SIGNAL_CRS, CBR_SIGNAL_CCS, or CBR_SIGNAL_ALL
 * @return		Number of signals sent, -1 on error
 */
extern int		cbr_flushSignals(Sdr sdr, int signalType);

/**
 * Process aggregation timeouts.
 * Should be called periodically by clock thread.
 *
 * @param sdr		SDR handle
 * @return		Number of signals sent due to timeout
 */
extern int		cbr_processTimeouts(Sdr sdr);

/*	Admin Record Handling						*/

/**
 * Handle received CRS admin record.
 * Decodes the CBOR map, logs each status entry, stores a ReceivedCrsRecord
 * in CbrDb.crsHistory, and increments crsSignalsRecv.
 *
 * @param sdr		SDR handle
 * @param adminRecord	Raw admin record content
 * @param length	Length of admin record
 * @param senderEid	Source EID of the bundle carrying this CRS (may be NULL)
 * @return		0 on success, -1 on error
 */
extern int		cbr_handleCrs(Sdr sdr, unsigned char *adminRecord,
				int length, const char *senderEid);

/*	CRS History							*/

/**
 * Return the SDR list SdrObject for the CRS reception history log.
 * Caller walks the list (each element is a ReceivedCrsRecord in SDR) and
 * prints or processes entries.  Must be called inside a transaction.
 *
 * @param sdr		SDR handle
 * @return		SDR list SdrObject, or 0 on error
 */
extern SdrObject cbr_getCrsHistoryList(Sdr sdr);

/**
 * Set the maximum number of entries in the CRS reception history log.
 * When the limit is exceeded, the oldest entry is evicted.
 * A value of 0 means the log is unbounded.
 *
 * @param sdr		SDR handle
 * @param max		New maximum (0 = unlimited)
 * @return		0 on success, -1 on error
 */
extern int		cbr_setCrsHistoryMax(Sdr sdr, unsigned int max);

/**
 * Handle received CCS admin record.
 *
 * @param sdr		SDR handle
 * @param adminRecord	Raw admin record content
 * @param length	Length of admin record
 * @return		0 on success, -1 on error
 */
extern int		cbr_handleCcs(Sdr sdr, unsigned char *adminRecord,
				int length);

/*	Configuration							*/

/**
 * Configure CBR/CT signal aggregation parameters.
 *
 * @param sdr			SDR handle
 * @param crsAggregateLimit	Max bundles before sending CRS (0=immediate)
 * @param ccsAggregateLimit	Max bundles before sending CCS (0=immediate)
 * @param aggregateTimeoutSec	Seconds before forced signal send
 * @return			0 on success, -1 on error
 */
extern int		cbr_configure(Sdr sdr, unsigned int crsAggregateLimit,
				unsigned int ccsAggregateLimit,
				unsigned int aggregateTimeoutSec);

/**
 * Enable or disable explicit sourceEid in outbound CREB blocks.
 * When enabled, creb_offer includes the source EID in the block (arrayLen=4)
 * rather than relying on the implicit bundle-source convention. Useful for
 * interoperability testing and for nodes whose block-source differs from the
 * bundle source.
 *
 * @param sdr		SDR handle
 * @param enable	1 to include EID explicitly, 0 to use implicit form
 * @return		0 on success, -1 on error
 */
extern int		cbr_getCrebExplicitEid(void);
extern int		cbr_setCrebExplicitEid(Sdr sdr, int enable);

/**
 * Get the current CREB default report-to EID override.
 * When set, all CREB blocks generated by this node will include this EID as
 * element 4 (arrayLen=5), directing relay and destination nodes to send CRS
 * to this EID instead of the bundle's reportTo field.
 *
 * @param sdr		SDR handle
 * @param buf		Output buffer for EID string
 * @param bufLen	Buffer length (should be >= MAX_EID_LEN)
 * @return		0 on success (buf filled; empty string means not set), -1 on error
 */
extern int		cbr_getCrebReportTo(Sdr sdr, char *buf, size_t bufLen);

/**
 * Set the CREB default report-to EID override.
 * When non-NULL and non-empty, all CREB blocks generated by this node will
 * carry this EID as the reportToEid field (arrayLen=5).
 * Pass NULL or "" to clear the override (revert to using bundle's reportTo).
 *
 * @param sdr		SDR handle
 * @param eid		EID string, or NULL/"" to clear
 * @return		0 on success, -1 on error
 */
extern int		cbr_setCrebReportTo(Sdr sdr, const char *eid);

/**
 * Get the sequence-counter wraparound max value applied to new counters.
 * Returns CBR_COUNTER_MAX_64BIT when unset.
 *
 * @return		Configured max value (CBR_COUNTER_MAX_{16,32,64}BIT)
 */
extern uvast		cbr_getCounterMaxValue(void);

/**
 * Set the sequence-counter wraparound max value applied to new counters.
 * Per Orange Book Section 3.2.9, counters may use 16/32/64-bit maximums for
 * interoperability with constrained peers.  Existing counters keep the value
 * they were created with; the new value applies to counters created after
 * this call.
 *
 * @param sdr		SDR handle
 * @param maxValue	One of CBR_COUNTER_MAX_{16,32,64}BIT
 * @return		0 on success, -1 on error (including invalid value)
 */
extern int		cbr_setCounterMaxValue(Sdr sdr, uvast maxValue);

/**
 * Configure custody retransmission strategy.
 * Per Orange Book, three strategies are supported:
 *   - CBR_RETX_NONE (default): Manual retransmission via command only
 *   - CBR_RETX_TIMER: Automatic periodic retransmission
 *   - CBR_RETX_SIGNAL: Retransmit when CCS refusal received
 *
 * @param sdr			SDR handle
 * @param strategy		Retransmission strategy (CBR_RETX_*)
 * @param intervalSec		For CBR_RETX_TIMER: interval in seconds
 * @param maxRetransmissions	Max retransmit attempts (0=unlimited)
 * @return			0 on success, -1 on error
 */
extern int		cbr_configureRetransmission(Sdr sdr, int strategy,
				unsigned int intervalSec,
				unsigned int maxRetransmissions);

/**
 * Get current CBR/CT configuration.
 */
extern int		cbr_getConfig(Sdr sdr, unsigned int *crsAggregateLimit,
				unsigned int *ccsAggregateLimit,
				unsigned int *aggregateTimeoutSec);

/**
 * Get current retransmission configuration.
 */
extern int		cbr_getRetransmissionConfig(Sdr sdr, int *strategy,
				unsigned int *intervalSec,
				unsigned int *maxRetransmissions);

/*	Status Report Mode (defined in bpP.h as BpStatusReportMode)	*/

/**
 * Get current status report mode.
 * NOTE: BpStatusReportMode enum is defined in bpP.h.
 *   BP_SR_MODE_TRADITIONAL (0): Standard BPv7 status reports only
 *   BP_SR_MODE_COMPRESSED  (1): CREB/CRS only
 *   BP_SR_MODE_BOTH        (2): Both mechanisms (for testing)
 *
 * @param sdr		SDR handle
 * @return		Current mode, or -1 on error
 */
extern int		cbr_getStatusReportMode(Sdr sdr);

/**
 * Set status report mode for locally-sourced bundles.
 * Does not affect processing of received bundles.
 *
 * @param sdr		SDR handle
 * @param mode		New status report mode (BP_SR_MODE_*)
 * @return		0 on success, -1 on error
 */
extern int		cbr_setStatusReportMode(Sdr sdr, int mode);

/**
 * Get custody mode setting.
 * Determines which custody transfer mechanism is active:
 *   BP_CUSTODY_NONE       (0): No custody transfer
 *   BP_CUSTODY_BIBE       (1): BIBE custody transfer (RFC 9171)
 *   BP_CUSTODY_ORANGEBOOK (2): Orange Book CBR/CT (CTEB/CCS)
 *
 * @param sdr		SDR handle
 * @return		Current mode, or -1 on error
 */
extern int		cbr_getCustodyMode(Sdr sdr);

/**
 * Set custody mode for the node.
 * Only one custody mechanism can be active at a time.
 *
 * @param sdr		SDR handle
 * @param mode		Custody mode (BP_CUSTODY_*)
 * @return		0 on success, -1 on error
 */
extern int		cbr_setCustodyMode(Sdr sdr, int mode);

/*	Manual Retransmission (Command-Based)				*/

/**
 * Manually trigger retransmission of a custody bundle.
 * Used when retransmitStrategy is CBR_RETX_NONE (manual mode).
 *
 * @param sdr		SDR handle
 * @param sourceEid	Source EID of bundle to retransmit
 * @param seqId		Sequence identifier
 * @param seqNum	Sequence number
 * @return		0 on success, -1 on error or bundle not found
 */
extern int		cbr_retransmitBundle(Sdr sdr, char *sourceEid,
				uvast seqId, uvast seqNum);

/**
 * Manually trigger retransmission of all custody bundles to a destination.
 * Used when retransmitStrategy is CBR_RETX_NONE (manual mode).
 *
 * @param sdr		SDR handle
 * @param destEid	Destination EID (NULL for all destinations)
 * @return		Number of bundles retransmitted, -1 on error
 */
extern int		cbr_retransmitAllCustody(Sdr sdr, char *destEid);

/**
 * List bundles currently held in custody.
 * Useful for administrative monitoring and manual retransmission decisions.
 *
 * @param sdr		SDR handle
 * @param callback	Function called for each custody bundle
 * @param userData	User data passed to callback
 * @return		Number of custody bundles, -1 on error
 */
typedef void		(*CbrCustodyCallback)(CbrCustodyInfo *info,
				void *userData);

extern int		cbr_listCustodyBundles(Sdr sdr,
				CbrCustodyCallback callback, void *userData);

/**
 * Query the custody status of a specific bundle.
 * Used by test applications to verify custody transfer completion.
 *
 * @param sdr		SDR handle
 * @param sourceEid	Source EID of the bundle
 * @param seqId		Sequence identifier
 * @param seqNum	Sequence number
 * @return		CBR_CUSTODY_STATUS_* value, or -1 on error
 */
extern int		cbr_getCustodyStatus(Sdr sdr, char *sourceEid,
				uvast seqId, uvast seqNum);

/*	Statistics							*/

/**
 * Statistics counters for CBR/CT operations.
 * Used to verify that CCS signals are actually being generated and processed.
 */
typedef struct {
	unsigned int	ccsAcceptSent;		/* CCS acceptance signals sent */
	unsigned int	ccsRefuseSent;		/* CCS refusal signals sent */
	unsigned int	ccsAcceptRecv;		/* CCS acceptance received */
	unsigned int	ccsRefuseRecv;		/* CCS refusal received */
	unsigned int	custodyOriginated;	/* Bundles originated with custody */
	unsigned int	custodyAccepted;	/* Bundles accepted into custody */
	unsigned int	custodyReleased;	/* Bundles released from custody */
	unsigned int	crsSignalsSent;		/* CRS signals sent */
	unsigned int	crsSignalsRecv;		/* CRS signals received */
} CbrStatistics;

/**
 * Get CBR/CT statistics counters.
 * Provides definitive proof that CCS/CRS signals were generated and processed.
 *
 * @param sdr		SDR handle
 * @param stats		Output: statistics counters
 * @return		0 on success, -1 on error
 */
extern int		cbr_getStatistics(Sdr sdr, CbrStatistics *stats);

/**
 * Reset CBR/CT statistics counters to zero.
 *
 * @param sdr		SDR handle
 * @return		0 on success, -1 on error
 */
extern int		cbr_resetStatistics(Sdr sdr);

/*	CBOR Encoding/Decoding Utilities				*/

/**
 * Encode a bundle sequence entry to CBOR for CRS/CCS transmission.
 * Handles both contiguous (single length) and non-contiguous (range array)
 * sequences per Orange Book Section 3.3.
 *
 * @param seqId		Sequence identifier
 * @param seqNumStart	Starting sequence number
 * @param length	Number of bundles (for contiguous sequences)
 * @param rangeArray	Array of alternating lengths (NULL if contiguous)
 * @param rangeCount	Number of elements in rangeArray
 * @param sourceEid	Source EID (may be omitted based on context)
 * @param signalDestEid	Destination of the signal (for omission logic)
 * @param localAdminEid	This node's admin EID (for omission logic)
 * @param buffer	Output buffer
 * @param buflen	Buffer length
 * @return		Bytes written, -1 on error
 */
extern int		cbr_encodeBundleSequence(uvast seqId, uvast seqNumStart,
				uvast length, uvast *rangeArray, int rangeCount,
				char *sourceEid, char *signalDestEid,
				char *localAdminEid, unsigned char *buffer,
				size_t buflen);

/**
 * Decode a bundle sequence entry from CBOR.
 *
 * @param cursor	Pointer to input buffer (updated on return)
 * @param bytesRemaining Bytes remaining (updated on return)
 * @param seqId		Output: sequence identifier (0 for per-destination mode)
 * @param seqNumStart	Output: starting sequence number
 * @param length	Output: length (for contiguous) or 0 (non-contiguous)
 * @param rangeArray	Output: allocated range array (caller must free)
 * @param rangeCount	Output: number of elements in rangeArray
 * @param sourceEid	Output: source EID (allocated, caller must free)
 * @param seqDestEid	Output: per-destination EID when seqId==0 (allocated,
 *			caller must free); NULL when seqId != 0
 * @return		0 on success, -1 on error
 */
extern int		cbr_decodeBundleSequence(unsigned char **cursor,
				unsigned int *bytesRemaining, uvast *seqId,
				uvast *seqNumStart, uvast *length,
				uvast **rangeArray, int *rangeCount,
				char **sourceEid, char **seqDestEid);

#ifdef __cplusplus
}
#endif

#endif /* _CBR_H_ */
