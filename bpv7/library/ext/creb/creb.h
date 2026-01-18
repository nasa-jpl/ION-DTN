/*
 *	creb.h:		definitions supporting implementation of
 *			the Compressed Reporting Extension Block (CREB).
 *
 *	Per CCSDS Orange Book Draft K: Compressed Bundle Status
 *	Reporting and Custody Signaling.
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

#endif /* _CREB_H_ */
