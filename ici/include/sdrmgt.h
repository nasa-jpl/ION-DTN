/*

	sdrmgt.h:	definitions supporting use of the SDR
			database space management mechanism.

	Author: Scott Burleigh, JPL
	
	Modification History:
	Date      Who	What
	06-05-07  SCB	Initial abstraction from original SDR API.

	Copyright (c) 2001-2007 California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.

									*/
#ifndef _SDRMGT_H_
#define _SDRMGT_H_

#include "sdrxn.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	char		sdrName[MAX_SDR_NAME + 1];
	size_t		heapSize;
	size_t		smallPoolSize;
	size_t		smallPoolFreeBlockCount[SMALL_SIZES];
	size_t		smallPoolFree;
	size_t		smallPoolAllocated;
	size_t		largePoolSize;
	size_t		largePoolFreeBlockCount[LARGE_ORDERS];
	size_t		largePoolFree;
	size_t		largePoolAllocated;
	size_t		unusedSize;
	size_t		maxLogLength;
} SdrUsageSummary;

/*		Low-level SDR space management functions.		*/

#define sdr_malloc(sdr, size) \
Sdr_malloc(__FILE__, __LINE__, sdr, size)
extern Object		Sdr_malloc(const char *file, int line,
				Sdr sdr, size_t size);

#define sdr_insert(sdr, from, size) \
Sdr_insert(__FILE__, __LINE__, sdr, from, size)
extern Object		Sdr_insert(const char *file, int line,
				Sdr sdr, char *from, size_t size);

#define sdr_stow(sdr, variable) \
Sdr_insert(__FILE__, __LINE__, sdr, (char *) &variable, sizeof variable)

extern size_t		sdr_object_length(Sdr sdr, Object object);

#define sdr_free(sdr, object) \
Sdr_free(__FILE__, __LINE__, sdr, object)
extern void		Sdr_free(const char *file, int line,
				Sdr sdr, Object object);

extern void		sdr_set_search_limit(Sdr sdr, unsigned int newLimit);
			/*	Sets limit on the number of free
			 *	blocks sdr_malloc will search through
			 *	in the nominal free space bucket,
			 *	looking for a sufficiently large free
			 *	block, before giving up and switching
			 *	to the next higher non-empty free space
			 *	bucket.					*/

extern void		sdr_stage(Sdr sdr, char *into, Object from,
				size_t size);

extern size_t		sdr_unused(Sdr sdr);
			/*	Returns the number of bytes of heap
				space not yet allocated to either the
				large or small objects pool.		*/

extern void		sdr_usage(Sdr sdr, SdrUsageSummary *);
			/*	Loads SdrUsageSummary structure with
				snapshot of SDR's usage status.		*/

extern void		sdr_report(SdrUsageSummary *);
			/*	Sends to log a snapshot of the
				SDR's usage status.			*/

extern void		sdr_stats(Sdr sdrv);
			/*	Sends to log an overview of the
				SDR's usage status.			*/

extern void		sdr_reset_stats(Sdr sdrv);
			/*	Reset max log length, print stats.	*/

extern int		sdr_heap_depleted(Sdr sdrv);
			/*	A Boolean function: returns 1 if total
			 *	available space in the SDR's data space
			 *	(small pool free, large pool free,
			 *	and unused) is less than 1/16 of
			 *	the total size of the data space, 0
			 *	otherwise.				*/

extern int		sdr_start_trace(Sdr sdr, size_t size, char *shm);
			/*	Begins an episode of SDR space usage
				tracing.  The size argument specifies
				the amount of shared memory to use
				for the trace operations; this memory
				will be dynamically allocated unless
				the shm region pointer argument is
				non-NULL.  Returns 0 on success, -1
				on any error.				*/

extern void		sdr_print_trace(Sdr sdr, int verbose);
			/*	Prints cumulative trace report and
				usage report, using writeMemo.  If
				'verbose' is zero, only exceptions
				are reported; otherwise, a log of
				all activity is printed.		*/

extern void		sdr_clear_trace(Sdr sdr);
			/*	Deletes closed events from trace log.	*/

extern void		sdr_stop_trace(Sdr sdr);
			/*	Ends the current episode of SDR space
				management tracing and releases
				the shared memory allocated to the
				trace operations.			*/
#ifdef __cplusplus
}
#endif

#endif  /* _SDRMGT_H_ */
