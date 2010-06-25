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
	long		sdrSize;
	long		smallPoolSize;
	long		smallPoolFreeBlockCount[SMALL_SIZES];
	long		smallPoolFree;
	long		smallPoolAllocated;
	long		largePoolSize;
	long		largePoolFreeBlockCount[LARGE_ORDERS];
	long		largePoolFree;
	long		largePoolAllocated;
	long		unusedSize;
} SdrUsageSummary;

/*		Low-level SDR space management functions.		*/

#define sdr_malloc(sdr, size) \
Sdr_malloc(__FILE__, __LINE__, sdr, size)
extern Object		Sdr_malloc(char *file, int line,
				Sdr sdr, unsigned long size);

#define sdr_insert(sdr, from, size) \
Sdr_insert(__FILE__, __LINE__, sdr, from, size)
extern Object		Sdr_insert(char *file, int line,
				Sdr sdr, char *from, unsigned long size);

#define sdr_stow(sdr, variable) \
Sdr_insert(__FILE__, __LINE__, sdr, (char *) &variable, sizeof variable)

extern long		sdr_object_length(Sdr sdr, Object object);

#define sdr_free(sdr, object) \
Sdr_free(__FILE__, __LINE__, sdr, object)
extern void		Sdr_free(char *file, int line,
				Sdr sdr, Object object);

extern void		sdr_stage(Sdr sdr, char *into, Object from, long size);

extern long		sdr_unused(Sdr sdr);
			/*	Returns the number of bytes of heap
				space not yet allocated to either the
				large or small objects pool.		*/

extern void		sdr_usage(Sdr sdr, SdrUsageSummary *);
			/*	Loads SdrUsageSummary structure with
				snapshot of SDR's usage status.		*/

extern void		sdr_report(SdrUsageSummary *);
			/*	Sends to log a snapshot of the
				SDR's usage status.			*/

extern int		sdr_heap_depleted(Sdr sdrv);
			/*	A Boolean function: returns 1 if
			 *	total available space in the SDR
			 *	(small pool free, large pool free,
			 *	and unused) is less than 1/16 of
			 *	the total size of the SDR, 0 otherwise.	*/

extern int		sdr_start_trace(Sdr sdr, long size, char *shm);
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
