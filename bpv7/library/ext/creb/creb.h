/*
 *	creb.h:		definitions supporting implementation of
 *			the Compressed Reporting Extension Block (CREB).
 *
 *	Per CCSDS 734.6-O-1: Custody Transfer and Compressed Bundle
 *	Status Reporting (Experimental Specification, Issue 1, June 2026).
 *
 *	Copyright (c) 2026, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: ION Development Team
 */

#ifndef _CREB_H_
#define _CREB_H_

#include "bei.h"

/*	CREB Extension Block Callbacks					*/

/**
 * Offer CREB block for inclusion in bundle.
 * Called during bundle creation. Attaches CREB when:
 *   1. Status reporting mode is COMPRESSED or BOTH
 *   2. Status report request flags (srrFlags) are set
 *
 * Per Orange Book, bundles with CREB MUST NOT be fragmented.
 */
extern int	creb_offer(ExtensionBlock *, Bundle *);

/**
 * Serialize CREB block to CBOR for transmission.
 * Called at dequeue time when block content is finalized.
 */
extern int	creb_serialize(ExtensionBlock *, Bundle *);

/**
 * Release CREB block resources.
 */
extern void	creb_release(ExtensionBlock *);

/**
 * Record CREB block from acquisition to SDR.
 */
extern int	creb_record(ExtensionBlock *, AcqExtBlock *);

/**
 * Copy CREB block during bundle copy.
 */
extern int	creb_copy(ExtensionBlock *, ExtensionBlock *);

/**
 * Process CREB block on bundle forward.
 * Extracts sequence info for compressed status reporting.
 */
extern int	creb_processOnFwd(ExtensionBlock *, Bundle *, void *);

/**
 * Process CREB block on custody acceptance.
 */
extern int	creb_processOnAccept(ExtensionBlock *, Bundle *, void *);

/**
 * Process CREB block on bundle enqueue.
 */
extern int	creb_processOnEnqueue(ExtensionBlock *, Bundle *, void *);

/**
 * Process CREB block on bundle dequeue.
 */
extern int	creb_processOnDequeue(ExtensionBlock *, Bundle *, void *);

/**
 * Parse CREB block from incoming bundle.
 * Handles variable-length CBOR arrays (1-5 elements).
 */
extern int	creb_parse(AcqExtBlock *, AcqWorkArea *);

/**
 * Check CREB block validity.
 */
extern int	creb_check(AcqExtBlock *, AcqWorkArea *);

/**
 * Clear CREB block acquisition resources.
 */
extern void	creb_clear(AcqExtBlock *);

/*	Utility Functions						*/

/**
 * Extract CREB reporting info from extension block.
 * Used by sendStatusRpt() to get sequence info for CRS generation.
 *
 * @param blk		The CREB extension block
 * @param seqId		Output: sequence identifier
 * @param seqNum	Output: sequence number
 * @return		0 on success, -1 on error
 */
extern int	creb_getReportInfo(ExtensionBlock *blk, uvast *seqId,
			uvast *seqNum);

/**
 * Extract CREB status report request flags from extension block.
 * Used by custody handlers to check for custody-event CRS requests
 * (CREB_REQUEST_CUSTODY_ACCEPT, CREB_REQUEST_CUSTODY_REFUSE).
 *
 * @param blk		The CREB extension block
 * @param requestFlags	Output: request flag byte
 * @return		0 on success, -1 on error
 */
extern int	creb_getRequestFlags(ExtensionBlock *blk,
			unsigned char *requestFlags);

/**
 * Extract CREB report-to EID override from extension block (element 4).
 * Copies the overridden report-to EID into buf, or sets buf[0]='\0' if
 * element 4 is absent (arrayLen < 5) or empty.
 *
 * @param blk		The CREB extension block
 * @param buf		Output buffer for the EID string
 * @param bufLen	Size of buf (should be MAX_EID_LEN)
 * @return		0 on success, -1 on error
 */
extern int	creb_getReportToEid(ExtensionBlock *blk, char *buf,
			size_t bufLen);

#endif /* _CREB_H_ */
