/*
 *	bpinspect_filter.c - Filtering and search implementation
 *
 *	Copyright (c) 2025, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
 *
 *	Author: ION Development Team
 */

#include "bpinspect_filter.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>

/*
 * Initialize filter criteria to default (show all)
 */
void bpinspect_filter_init(FilterCriteria *filter)
{
	if (filter == NULL)
	{
		return;
	}

	memset(filter, 0, sizeof(FilterCriteria));

	/* Default: show all */
	filter->priorityFilter = -1;  /* -1 means all priorities */
	filter->minSize = 0;
	filter->maxSize = 0;  /* 0 means no limit */
	filter->minCreationTime = 0;
	filter->maxCreationTime = 0;
	filter->expiringWithinSecs = 0;
	filter->adminOnly = 0;
	filter->dataOnly = 0;
	filter->fragmentsOnly = 0;
	filter->showForwarding = 1;
	filter->showDelivery = 1;
	filter->showTransmission = 1;
	filter->showLimbo = 1;
	filter->sourceMatchMode = MATCH_PREFIX;
	filter->destMatchMode = MATCH_PREFIX;
}

/*
 * String matching helper
 */
int bpinspect_filter_str_match(const char *str, const char *pattern, MatchMode mode)
{
	int	patternLen;

	if (str == NULL || pattern == NULL)
	{
		return 0;
	}

	/* Empty pattern matches everything */
	if (pattern[0] == '\0')
	{
		return 1;
	}

	patternLen = strlen(pattern);

	switch (mode)
	{
	case MATCH_EXACT:
		return strcmp(str, pattern) == 0;

	case MATCH_PREFIX:
		return strncmp(str, pattern, patternLen) == 0;

	case MATCH_CONTAINS:
		return strstr(str, pattern) != NULL;

	case MATCH_REGEX:
		/* TODO: Implement regex matching if needed */
		/* For now, fall back to contains */
		return strstr(str, pattern) != NULL;

	default:
		return 0;
	}
}

/*
 * Check if bundle matches priority filter
 */
static int match_priority(const BundleCacheEntry *entry, const FilterCriteria *filter)
{
	if (filter->priorityFilter < 0)
	{
		return 1;  /* Show all priorities */
	}

	return entry->priority == filter->priorityFilter;
}

/*
 * Check if bundle matches size filter
 */
static int match_size(const BundleCacheEntry *entry, const FilterCriteria *filter)
{
	if (filter->minSize > 0 && entry->payloadLength < filter->minSize)
	{
		return 0;
	}

	if (filter->maxSize > 0 && entry->payloadLength > filter->maxSize)
	{
		return 0;
	}

	return 1;
}

/*
 * Check if bundle matches time filter
 */
static int match_time(const BundleCacheEntry *entry, const FilterCriteria *filter)
{
	time_t	creationTime;

	/* Convert creation timestamp to Unix epoch for comparison */
	/* BPv7 uses 2000-01-01 00:00:00 UTC as epoch */
	creationTime = entry->creationMsec / 1000 + 946684800;  /* Add seconds from 1970 to 2000 */

	/* Check creation time range */
	if (filter->minCreationTime > 0 && creationTime < filter->minCreationTime)
	{
		return 0;
	}

	if (filter->maxCreationTime > 0 && creationTime > filter->maxCreationTime)
	{
		return 0;
	}

	/* Check expiring within specified seconds */
	if (filter->expiringWithinSecs > 0)
	{
		if (entry->timeRemaining < 0 || entry->timeRemaining > filter->expiringWithinSecs)
		{
			return 0;
		}
	}

	return 1;
}

/*
 * Check if bundle matches bundle type filter
 */
static int match_bundle_type(const BundleCacheEntry *entry, const FilterCriteria *filter)
{
	int	isAdmin;
	int	isFragment;

	/* Check admin/data filter */
	isAdmin = (entry->bundleProcFlags & BDL_IS_ADMIN) != 0;

	if (filter->adminOnly && !isAdmin)
	{
		return 0;
	}

	if (filter->dataOnly && isAdmin)
	{
		return 0;
	}

	/* Check fragment filter */
	isFragment = (entry->bundleProcFlags & BDL_IS_FRAGMENT) != 0;

	if (filter->fragmentsOnly && !isFragment)
	{
		return 0;
	}

	return 1;
}

/*
 * Check if bundle matches queue state filter
 */
static int match_queue_state(const BundleCacheEntry *entry, const FilterCriteria *filter)
{
	if (strcmp(entry->queueState, "Forwarding") == 0)
	{
		return filter->showForwarding;
	}

	if (strcmp(entry->queueState, "Delivery") == 0)
	{
		return filter->showDelivery;
	}

	if (strcmp(entry->queueState, "Transmit") == 0 ||
			strcmp(entry->queueState, "Planned") == 0)
	{
		return filter->showTransmission;
	}

	if (strcmp(entry->queueState, "Limbo") == 0)
	{
		return filter->showLimbo;
	}

	return 1;  /* Unknown state, show by default */
}

/*
 * Apply filter to a single bundle cache entry
 */
int bpinspect_filter_match(const BundleCacheEntry *entry, const FilterCriteria *filter)
{
	if (entry == NULL || filter == NULL)
	{
		return 0;
	}

	/* Check source EID filter */
	if (!bpinspect_filter_str_match(entry->source, filter->sourceFilter,
					filter->sourceMatchMode))
	{
		return 0;
	}

	/* Check destination EID filter */
	if (!bpinspect_filter_str_match(entry->dest, filter->destFilter,
					filter->destMatchMode))
	{
		return 0;
	}

	/* Check priority filter */
	if (!match_priority(entry, filter))
	{
		return 0;
	}

	/* Check size filter */
	if (!match_size(entry, filter))
	{
		return 0;
	}

	/* Check time filter */
	if (!match_time(entry, filter))
	{
		return 0;
	}

	/* Check bundle type filter */
	if (!match_bundle_type(entry, filter))
	{
		return 0;
	}

	/* Check queue state filter */
	if (!match_queue_state(entry, filter))
	{
		return 0;
	}

	return 1;  /* All filters passed */
}

/*
 * Apply filter to entire bundle list, creating filtered subset
 */
int bpinspect_filter_apply(const BundleListState *state,
			   const FilterCriteria *filter,
			   BundleCacheEntry **filtered,
			   int maxResults)
{
	int	i;
	int	matchCount = 0;
	BundleCacheEntry	*results;

	if (state == NULL || filter == NULL || filtered == NULL)
	{
		return -1;
	}

	/* Allocate results array */
	results = (BundleCacheEntry *) MTAKE(maxResults * sizeof(BundleCacheEntry));
	if (results == NULL)
	{
		putErrmsg("Can't allocate filter results.", NULL);
		return -1;
	}

	/* Apply filter to each entry */
	for (i = 0; i < state->count && matchCount < maxResults; i++)
	{
		if (bpinspect_filter_match(&state->entries[i], filter))
		{
			memcpy(&results[matchCount], &state->entries[i],
					sizeof(BundleCacheEntry));
			matchCount++;
		}
	}

	*filtered = results;
	return matchCount;
}
