/*
 *	cteb.h:		definitions supporting implementation of
 *			the Custody Transfer Extension Block (CTEB).
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

#ifndef _CTEB_H_
#define _CTEB_H_

#include "bei.h"

/*	CTEB Extension Block Callbacks					*/

/**
 * Offer CTEB block for inclusion in bundle.
 * Called during bundle creation. Attaches CTEB when:
 *   1. Custody transfer is requested for this bundle
 *   2. Bundle destination supports custody transfer
 *
 * Per Orange Book, bundles with CTEB MUST NOT be fragmented.
 */
extern int	cteb_offer(ExtensionBlock *, Bundle *);

/**
 * Serialize CTEB block to CBOR for transmission.
 * CBOR format: [seq_num, seq_id, custodian_eid]
 */
extern int	cteb_serialize(ExtensionBlock *, Bundle *);

/**
 * Release CTEB block resources.
 */
extern void	cteb_release(ExtensionBlock *);

/**
 * Record CTEB block from acquisition to SDR.
 */
extern int	cteb_record(ExtensionBlock *, AcqExtBlock *);

/**
 * Copy CTEB block during bundle copy.
 */
extern int	cteb_copy(ExtensionBlock *, ExtensionBlock *);

/**
 * Process CTEB block on bundle forward.
 */
extern int	cteb_processOnFwd(ExtensionBlock *, Bundle *, void *);

/**
 * Process CTEB block on custody acceptance.
 * This is where custody transfer actually occurs:
 *   1. Send CCS acceptance to previous custodian
 *   2. Update CTEB with new custodian EID
 *   3. Add bundle to local custody tracking
 */
extern int	cteb_processOnAccept(ExtensionBlock *, Bundle *, void *);

/**
 * Process CTEB block on bundle enqueue.
 */
extern int	cteb_processOnEnqueue(ExtensionBlock *, Bundle *, void *);

/**
 * Process CTEB block on bundle dequeue.
 * Updates custodian EID to local node before transmission.
 */
extern int	cteb_processOnDequeue(ExtensionBlock *, Bundle *, void *);

/**
 * Parse CTEB block from incoming bundle.
 * CBOR format: [seq_num, seq_id, custodian_eid]
 */
extern int	cteb_parse(AcqExtBlock *, AcqWorkArea *);

/**
 * Check CTEB block validity.
 */
extern int	cteb_check(AcqExtBlock *, AcqWorkArea *);

/**
 * Clear CTEB block acquisition resources.
 */
extern void	cteb_clear(AcqExtBlock *);

/**
 * Get custody tracking info from CTEB block.
 * Used by bpSend to track custody at bundle creation.
 *
 * @param blk		Extension block containing CTEB
 * @param seqId		Output: sequence identifier
 * @param seqNum	Output: sequence number
 * @return		0 on success, -1 on error or no CTEB
 */
extern int	cteb_getCustodyInfo(ExtensionBlock *blk, uvast *seqId,
				uvast *seqNum);

#endif /* _CTEB_H_ */
