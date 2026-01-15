/*
 *	bpinspect_ops.h - Bundle operations (cancel, export, etc.)
 *
 *	Copyright (c) 2025, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
 *
 *	Author: ION Development Team
 */

#ifndef BPINSPECT_OPS_H
#define BPINSPECT_OPS_H

#include "bpinspect_data.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Cancel a single bundle by cache entry
 * Returns: 0 on success, -1 on error
 */
extern int	bpinspect_ops_cancel_bundle(const BundleCacheEntry *entry);

/*
 * Cancel multiple bundles from an array of cache entries
 * Parameters:
 *   entries - Array of bundle cache entries to cancel
 *   count   - Number of entries in array
 * Returns: number of bundles successfully canceled, -1 on error
 */
extern int	bpinspect_ops_cancel_bundles(const BundleCacheEntry *entries, int count);

/*
 * Suspend a bundle by cache entry
 * Returns: 0 on success, -1 on error
 */
extern int	bpinspect_ops_suspend_bundle(const BundleCacheEntry *entry);

/*
 * Resume a bundle by cache entry
 * Returns: 0 on success, -1 on error
 */
extern int	bpinspect_ops_resume_bundle(const BundleCacheEntry *entry);

/*
 * Suspend multiple bundles from an array of cache entries
 * Parameters:
 *   entries - Array of bundle cache entries to suspend
 *   count   - Number of entries in array
 * Returns: number of bundles successfully suspended, -1 on error
 */
extern int	bpinspect_ops_suspend_bundles(const BundleCacheEntry *entries, int count);

/*
 * Resume multiple bundles from an array of cache entries
 * Parameters:
 *   entries - Array of bundle cache entries to resume
 *   count   - Number of entries in array
 * Returns: number of bundles successfully resumed, -1 on error
 */
extern int	bpinspect_ops_resume_bundles(const BundleCacheEntry *entries, int count);

/*
 * Export bundle details to a file
 * Parameters:
 *   entry    - Bundle cache entry
 *   filename - Output filename (NULL for stdout)
 *   verbose  - Verbosity level (0=basic, 1=detailed, 2=include payload)
 * Returns: 0 on success, -1 on error
 */
extern int	bpinspect_ops_export_bundle(const BundleCacheEntry *entry,
					    const char *filename,
					    int verbose);

/*
 * Print bundle details to stdout
 * Parameters:
 *   entry   - Bundle cache entry
 *   verbose - Verbosity level (0=basic, 1=detailed, 2=include payload/extensions)
 */
extern void	bpinspect_ops_print_bundle(const BundleCacheEntry *entry, int verbose);

/*
 * Format bundle ID as string (source, creation time, fragment offset/length)
 * Parameters:
 *   entry - Bundle cache entry
 *   buf   - Output buffer
 *   len   - Buffer length
 * Returns: formatted bundle ID string
 */
extern char*	bpinspect_ops_format_bundle_id(const BundleCacheEntry *entry,
					       char *buf,
					       int len);

#ifdef __cplusplus
}
#endif

#endif /* BPINSPECT_OPS_H */
