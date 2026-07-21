/*
 *	bpinspect_data.c - Bundle enumeration and caching implementation
 *
 *	Copyright (c) 2025, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
 *
 *	Author: ION Development Team
 */

#include "bpinspect_data.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>

/* Initial cache capacity */
#define INITIAL_CACHE_CAPACITY	1000

/* Cache growth factor */
#define CACHE_GROWTH_FACTOR	2

/*
 * Initialize bundle list state
 */
int bpinspect_data_init(BundleListState *state)
{
	CHKERR(state);

	memset(state, 0, sizeof(BundleListState));

	state->capacity = INITIAL_CACHE_CAPACITY;
	state->entries = (BundleCacheEntry *) MTAKE(state->capacity * sizeof(BundleCacheEntry));
	if (state->entries == NULL)
	{
		putErrmsg("Can't allocate bundle cache.", NULL);
		return -1;
	}

	memset(state->entries, 0, state->capacity * sizeof(BundleCacheEntry));
	state->count = 0;
	state->lastRefresh = 0;
	state->sortField = SORT_BY_CREATION_TIME;
	state->sortAscending = 0;  /* Default: newest first */

	return 0;
}

/*
 * Free resources used by bundle list state
 */
void bpinspect_data_cleanup(BundleListState *state)
{
	if (state == NULL)
	{
		return;
	}

	if (state->entries != NULL)
	{
		MRELEASE(state->entries);
		state->entries = NULL;
	}

	state->count = 0;
	state->capacity = 0;
}

/*
 * Expand cache capacity if needed
 */
static int expand_cache(BundleListState *state)
{
	int		newCapacity;
	BundleCacheEntry	*newEntries;

	CHKERR(state);

	newCapacity = state->capacity * CACHE_GROWTH_FACTOR;
	newEntries = (BundleCacheEntry *) MTAKE(newCapacity * sizeof(BundleCacheEntry));
	if (newEntries == NULL)
	{
		putErrmsg("Can't expand bundle cache.", NULL);
		return -1;
	}

	/* Copy existing entries */
	memcpy(newEntries, state->entries, state->count * sizeof(BundleCacheEntry));
	memset(newEntries + state->count, 0,
		(newCapacity - state->count) * sizeof(BundleCacheEntry));

	/* Free old array and use new one */
	MRELEASE(state->entries);
	state->entries = newEntries;
	state->capacity = newCapacity;

	return 0;
}

/*
 * Get queue state description for a bundle
 */
void bpinspect_data_get_queue_state(Bundle *bundle, char *queueState, int maxLen)
{
	Sdr		sdr;
	BpDB		*bpConstants;
	SdrObject	queue;

	if (bundle == NULL || queueState == NULL || maxLen <= 0)
	{
		return;
	}

	queueState[0] = '\0';

	if (bundle->dlvQueueElt)
	{
		istrcpy(queueState, "Delivery", maxLen);
	}
	else if (bundle->fwdQueueElt)
	{
		istrcpy(queueState, "Forwarding", maxLen);
	}
	else if (bundle->ductXmitElt)
	{
		istrcpy(queueState, "Transmit", maxLen);
	}
	else if (bundle->planXmitElt)
	{
		/*	Check if this is in the limbo queue or a plan queue.
		 *	When bundles are suspended, they're moved to limbo
		 *	but planXmitElt still points to the queue element.	*/

		sdr = bp_get_sdr();
		bpConstants = getBpConstants();
		queue = sdr_list_list(sdr, bundle->planXmitElt);

		if (queue == bpConstants->limboQueue)
		{
			istrcpy(queueState, "Limbo", maxLen);
		}
		else
		{
			istrcpy(queueState, "Planned", maxLen);
		}
	}
	else
	{
		istrcpy(queueState, "Limbo", maxLen);
	}
}

/*
 * Calculate time remaining until expiration
 */
int bpinspect_data_time_remaining(time_t expirationTime)
{
	time_t	now;
	int	remaining;

	now = time(NULL);
	if (expirationTime <= now)
	{
		return -1;  /* Already expired */
	}

	remaining = (int)(expirationTime - now);
	return remaining;
}

/*
 * Populate cache entry from Bundle object
 */
static int populate_cache_entry(Sdr sdr, SdrObject bundleObj,
		BundleCacheEntry *entry)
{
	Bundle	bundle;
	char	*eid;

	CHKERR(sdr);
	CHKERR(bundleObj);
	CHKERR(entry);

	/* Read bundle from SDR */
	sdr_read(sdr, (char *) &bundle, bundleObj, sizeof(Bundle));

	/* Store bundle object reference */
	entry->bundleObj = bundleObj;

	/* Extract source EID */
	readEid(&(bundle.id.source), &eid);
	if (eid != NULL)
	{
		istrcpy(entry->source, eid, MAX_EID_LEN);
		MRELEASE(eid);
	}
	else
	{
		entry->source[0] = '\0';
	}

	/* Extract destination EID */
	readEid(&(bundle.destination), &eid);
	if (eid != NULL)
	{
		istrcpy(entry->dest, eid, MAX_EID_LEN);
		MRELEASE(eid);
	}
	else
	{
		entry->dest[0] = '\0';
	}

	/* Extract bundle ID components */
	entry->creationMsec = bundle.id.creationTime.msec;
	entry->creationCount = bundle.id.creationTime.count;
	entry->fragmentOffset = bundle.id.fragmentOffset;

	/* Calculate fragment length */
	if (bundle.bundleProcFlags & BDL_IS_FRAGMENT)
	{
		entry->fragmentLength = bundle.payload.length;
	}
	else
	{
		entry->fragmentLength = 0;
	}

	/* Extract bundle metadata */
	entry->expirationTime = bundle.expirationTime;
	entry->payloadLength = bundle.payload.length;
	entry->totalAduLength = bundle.totalAduLength;
	entry->priority = bundle.priority;
	entry->bundleProcFlags = bundle.bundleProcFlags;

	/* Calculate time remaining */
	entry->timeRemaining = bpinspect_data_time_remaining(bundle.expirationTime);

	/* Get queue state */
	bpinspect_data_get_queue_state(&bundle, entry->queueState, MAX_QUEUE_STATE_LEN);

	return 0;
}

/*
 * Enumerate all bundles from SDR timeline and populate cache
 */
int bpinspect_data_refresh(BundleListState *state)
{
	Sdr		sdr;
	BpDB		*bpConstants;
	SdrObject	elt;
	SdrObject	addr;
	BpEvent		event;
	int		bundleCount = 0;

	CHKERR(state);

	sdr = bp_get_sdr();
	bpConstants = getBpConstants();
	if (bpConstants == NULL)
	{
		putErrmsg("Can't get BP constants.", NULL);
		return -1;
	}

	/* Reset cache count */
	state->count = 0;

	CHKERR(sdr_begin_xn(sdr));

	/* Iterate through timeline */
	for (elt = sdr_list_first(sdr, bpConstants->timeline); elt;
			elt = sdr_list_next(sdr, elt))
	{
		addr = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &event, addr, sizeof(BpEvent));

		/* We only care about bundle expiration events (which reference bundles) */
		if (event.type != expiredTTL)
		{
			continue;
		}

		/* Check if we need to expand cache */
		if (state->count >= state->capacity)
		{
			if (expand_cache(state) < 0)
			{
				sdr_exit_xn(sdr);
				return -1;
			}
		}

		/* Populate cache entry */
		if (populate_cache_entry(sdr, event.ref, &state->entries[state->count]) < 0)
		{
			/* Skip bundles that can't be read */
			continue;
		}

		state->count++;
		bundleCount++;
	}

	sdr_exit_xn(sdr);

	/* Update last refresh time */
	state->lastRefresh = time(NULL);

	return bundleCount;
}

/*
 * Comparison functions for qsort
 */
static int compare_creation_time_asc(const void *a, const void *b)
{
	const BundleCacheEntry *e1 = (const BundleCacheEntry *) a;
	const BundleCacheEntry *e2 = (const BundleCacheEntry *) b;

	if (e1->creationMsec < e2->creationMsec) return -1;
	if (e1->creationMsec > e2->creationMsec) return 1;
	if (e1->creationCount < e2->creationCount) return -1;
	if (e1->creationCount > e2->creationCount) return 1;
	return 0;
}

static int compare_creation_time_desc(const void *a, const void *b)
{
	return -compare_creation_time_asc(a, b);
}

static int compare_expiration_time_asc(const void *a, const void *b)
{
	const BundleCacheEntry *e1 = (const BundleCacheEntry *) a;
	const BundleCacheEntry *e2 = (const BundleCacheEntry *) b;

	if (e1->expirationTime < e2->expirationTime) return -1;
	if (e1->expirationTime > e2->expirationTime) return 1;
	return 0;
}

static int compare_expiration_time_desc(const void *a, const void *b)
{
	return -compare_expiration_time_asc(a, b);
}

static int compare_size_asc(const void *a, const void *b)
{
	const BundleCacheEntry *e1 = (const BundleCacheEntry *) a;
	const BundleCacheEntry *e2 = (const BundleCacheEntry *) b;

	if (e1->payloadLength < e2->payloadLength) return -1;
	if (e1->payloadLength > e2->payloadLength) return 1;
	return 0;
}

static int compare_size_desc(const void *a, const void *b)
{
	return -compare_size_asc(a, b);
}

static int compare_source_asc(const void *a, const void *b)
{
	const BundleCacheEntry *e1 = (const BundleCacheEntry *) a;
	const BundleCacheEntry *e2 = (const BundleCacheEntry *) b;

	return strcmp(e1->source, e2->source);
}

static int compare_source_desc(const void *a, const void *b)
{
	return -compare_source_asc(a, b);
}

static int compare_dest_asc(const void *a, const void *b)
{
	const BundleCacheEntry *e1 = (const BundleCacheEntry *) a;
	const BundleCacheEntry *e2 = (const BundleCacheEntry *) b;

	return strcmp(e1->dest, e2->dest);
}

static int compare_dest_desc(const void *a, const void *b)
{
	return -compare_dest_asc(a, b);
}

static int compare_priority_asc(const void *a, const void *b)
{
	const BundleCacheEntry *e1 = (const BundleCacheEntry *) a;
	const BundleCacheEntry *e2 = (const BundleCacheEntry *) b;

	if (e1->priority < e2->priority) return -1;
	if (e1->priority > e2->priority) return 1;
	return 0;
}

static int compare_priority_desc(const void *a, const void *b)
{
	return -compare_priority_asc(a, b);
}

/*
 * Sort bundle list by specified field
 */
int bpinspect_data_sort(BundleListState *state, SortField field, int ascending)
{
	int (*compare_func)(const void *, const void *);

	CHKERR(state);

	if (state->count <= 1)
	{
		return 0;  /* Nothing to sort */
	}

	/* Select comparison function based on field and direction */
	switch (field)
	{
	case SORT_BY_CREATION_TIME:
		compare_func = ascending ? compare_creation_time_asc : compare_creation_time_desc;
		break;

	case SORT_BY_EXPIRATION_TIME:
		compare_func = ascending ? compare_expiration_time_asc : compare_expiration_time_desc;
		break;

	case SORT_BY_SIZE:
		compare_func = ascending ? compare_size_asc : compare_size_desc;
		break;

	case SORT_BY_SOURCE:
		compare_func = ascending ? compare_source_asc : compare_source_desc;
		break;

	case SORT_BY_DEST:
		compare_func = ascending ? compare_dest_asc : compare_dest_desc;
		break;

	case SORT_BY_PRIORITY:
		compare_func = ascending ? compare_priority_asc : compare_priority_desc;
		break;

	default:
		putErrmsg("Invalid sort field.", itoa(field));
		return -1;
	}

	/* Sort the array */
	qsort(state->entries, state->count, sizeof(BundleCacheEntry), compare_func);

	/* Update state */
	state->sortField = field;
	state->sortAscending = ascending;

	return 0;
}
