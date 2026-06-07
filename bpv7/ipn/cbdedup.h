/*
	cbdedup.h:	per-node "already-forwarded" seen-set for
			critical (BP_MINIMUM_LATENCY) bundles.

	A critical bundle is flooded across all CGR-suitable
	neighbors at every hop.  Without de-duplication, a single
	critical bundle replicates exponentially in dense topologies
	(bounded only by ION_HOP_LIMIT).  This module keeps a
	seen-set in ION working memory keyed by bundle ID; once a
	critical bundle has been forwarded out of this node, any
	subsequent copy received here is dropped instead of re-flooded.

	The table lives in the ION working-memory partition and is
	anchored in the PSM catalog, so it survives an ipnfw restart
	within an ION session (and is reclaimed when the working
	memory is torn down, e.g. by ionstop / killm).  It is bounded
	by FIFO eviction and lazy TTL purge, so it cannot grow without
	limit.
									*/
#ifndef CBDEDUP_H
#define CBDEDUP_H

#include "bpP.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Size cap.  When a successful insert pushes the table past
 * CBDEDUP_HIGH_WATER, the oldest entries (by insertion order)
 * are evicted until length is back at CBDEDUP_LOW_WATER.
 * Each dedup entry takes ~160 bytes on 64bit systems.
 * for high water of 5000 this takes ~800KB of cache memory.
 */
#ifndef CBDEDUP_HIGH_WATER
#define CBDEDUP_HIGH_WATER 5000
#endif
#ifndef CBDEDUP_LOW_WATER
#define CBDEDUP_LOW_WATER 4000
#endif

extern int  cbdedup_init(void);
extern void cbdedup_shutdown(void);

/*	Returns 1 if the bundle's ID is in the seen-set
 *	0 otherwise.  Returns 0 for non-critical bundles or non-ipn
 *	sources.							*/
extern int cbdedup_seen(Bundle *bundle);

/*	Records that a critical bundle has been forwarded out of
 *	this node.  Idempotent: a duplicate key is a no-op. Returns 0
 *	on success, -1 on allocation failure.				*/
extern int cbdedup_record(Bundle *bundle);

#ifdef __cplusplus
}
#endif

#endif
