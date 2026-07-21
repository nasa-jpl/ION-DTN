/*
 *	bpinspect_data.h - Data structures and functions for bundle enumeration
 *
 *	Copyright (c) 2025, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
 *
 *	Author: ION Development Team
 */

#ifndef BPINSPECT_DATA_H
#define BPINSPECT_DATA_H

#include "bpP.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum queue state description length */
#define MAX_QUEUE_STATE_LEN	32

/* Bundle cache entry - stores key metadata for filtering and display */
typedef struct
{
	SdrObject	bundleObj;		/* SDR address of Bundle */
	char		source[MAX_EID_LEN];	/* Source EID */
	char		dest[MAX_EID_LEN];	/* Destination EID */
	uvast		creationMsec;		/* Creation timestamp (msec) */
	unsigned int	creationCount;		/* Creation timestamp (count) */
	unsigned int	fragmentOffset;		/* Fragment offset (0 if not fragment) */
	unsigned int	fragmentLength;		/* Fragment length (0 if not fragment) */
	time_t		expirationTime;		/* Expiration time (Unix epoch) */
	vast		payloadLength;		/* Payload size in bytes */
	unsigned int	totalAduLength;		/* Total ADU length */
	unsigned char	priority;		/* Priority: 0=bulk, 1=std, 2=urgent */
	unsigned char	bundleProcFlags;	/* Bundle processing flags */
	int		timeRemaining;		/* Seconds until expiration */
	char		queueState[MAX_QUEUE_STATE_LEN];  /* Queue state description */
} BundleCacheEntry;

/* Sort field enumeration */
typedef enum
{
	SORT_BY_CREATION_TIME = 0,
	SORT_BY_EXPIRATION_TIME,
	SORT_BY_SIZE,
	SORT_BY_SOURCE,
	SORT_BY_DEST,
	SORT_BY_PRIORITY
} SortField;

/* Bundle list state - manages cache and filtering */
typedef struct
{
	BundleCacheEntry	*entries;	/* Array of bundle cache entries */
	int			count;		/* Number of bundles in cache */
	int			capacity;	/* Allocated capacity */
	time_t			lastRefresh;	/* Last cache refresh time */
	SortField		sortField;	/* Current sort field */
	int			sortAscending;	/* Sort direction (1=asc, 0=desc) */
} BundleListState;

/*
 * Initialize bundle list state
 * Returns: 0 on success, -1 on error
 */
extern int	bpinspect_data_init(BundleListState *state);

/*
 * Free resources used by bundle list state
 */
extern void	bpinspect_data_cleanup(BundleListState *state);

/*
 * Enumerate all bundles from SDR timeline and populate cache
 * Returns: number of bundles enumerated, -1 on error
 */
extern int	bpinspect_data_refresh(BundleListState *state);

/*
 * Sort bundle list by specified field
 * Returns: 0 on success, -1 on error
 */
extern int	bpinspect_data_sort(BundleListState *state, SortField field, int ascending);

/*
 * Get queue state description for a bundle
 * Returns: queue state string
 */
extern void	bpinspect_data_get_queue_state(Bundle *bundle, char *queueState, int maxLen);

/*
 * Calculate time remaining until expiration
 * Returns: seconds remaining, or -1 if already expired
 */
extern int	bpinspect_data_time_remaining(time_t expirationTime);

#ifdef __cplusplus
}
#endif

#endif /* BPINSPECT_DATA_H */
