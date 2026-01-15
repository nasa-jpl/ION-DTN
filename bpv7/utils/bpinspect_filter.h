/*
 *	bpinspect_filter.h - Filtering and search functions for bundles
 *
 *	Copyright (c) 2025, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
 *
 *	Author: ION Development Team
 */

#ifndef BPINSPECT_FILTER_H
#define BPINSPECT_FILTER_H

#include "bpinspect_data.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Filter match mode */
typedef enum
{
	MATCH_EXACT = 0,	/* Exact string match */
	MATCH_PREFIX,		/* Prefix match */
	MATCH_CONTAINS,		/* Substring match */
	MATCH_REGEX		/* Regular expression (future) */
} MatchMode;

/* Filter criteria structure */
typedef struct
{
	/* EID filters */
	char		sourceFilter[MAX_EID_LEN];
	char		destFilter[MAX_EID_LEN];
	MatchMode	sourceMatchMode;
	MatchMode	destMatchMode;

	/* Priority filter (-1 = all, 0=bulk, 1=std, 2=urgent) */
	int		priorityFilter;

	/* Size filters (0 = no limit) */
	vast		minSize;
	vast		maxSize;

	/* Time filters (0 = no limit) */
	time_t		minCreationTime;
	time_t		maxCreationTime;
	int		expiringWithinSecs;	/* Show bundles expiring within N seconds */

	/* Bundle type filters */
	int		adminOnly;		/* 1=admin bundles only, 0=all */
	int		dataOnly;		/* 1=data bundles only, 0=all */
	int		fragmentsOnly;		/* 1=fragments only, 0=all */

	/* Queue state filters */
	int		showForwarding;		/* Show bundles in forwarding queue */
	int		showDelivery;		/* Show bundles in delivery queue */
	int		showTransmission;	/* Show bundles in transmission queue */
	int		showLimbo;		/* Show bundles in limbo */
} FilterCriteria;

/*
 * Initialize filter criteria to default (show all)
 */
extern void	bpinspect_filter_init(FilterCriteria *filter);

/*
 * Apply filter to a single bundle cache entry
 * Returns: 1 if bundle matches filter, 0 if not
 */
extern int	bpinspect_filter_match(const BundleCacheEntry *entry, const FilterCriteria *filter);

/*
 * Apply filter to entire bundle list, creating filtered subset
 * Parameters:
 *   state      - Bundle list state with all bundles
 *   filter     - Filter criteria
 *   filtered   - Output array for filtered results (caller allocates)
 *   maxResults - Maximum number of results to return
 * Returns: number of matching bundles, -1 on error
 */
extern int	bpinspect_filter_apply(const BundleListState *state,
				       const FilterCriteria *filter,
				       BundleCacheEntry **filtered,
				       int maxResults);

/*
 * String matching helper
 * Returns: 1 if str matches pattern according to mode, 0 if not
 */
extern int	bpinspect_filter_str_match(const char *str, const char *pattern, MatchMode mode);

#ifdef __cplusplus
}
#endif

#endif /* BPINSPECT_FILTER_H */
