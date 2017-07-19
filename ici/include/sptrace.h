/*

	sptrace.h:	API for using the sptrace space utilization
			trace system.  sptrace is designed to monitor
			space utilization in a generic heap, i.e., a
			region of "space" (nominally memory; possibly
			shared and possibly persistent) with a flat
			address space.  Its main purpose is finding
			leaks.

	Copyright (c) 2002 California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.

	Author: Scott Burleigh, JPL
	
	Modification History:
	Date      Who	What
	11-30-02  SCB   Original development.

									*/
#include "psm.h"

#ifndef _SPTRACE_H_
#define _SPTRACE_H_

#ifdef __cplusplus
extern "C" {
#endif

extern PsmPartition	sptrace_start(int key, size_t size, char *region,
					PsmPartition psm, char *name);
			/*	Begins an episode of heap usage
				tracing.  The shared memory key
				specifies the region of shared memory
				to use for the operations of this
				trace episode; a shared memory region
				of the indicated size will be dynamically
				allocated (unless the region argument
				is non-NULL) and the PSM space
				management structure for that region
				will also be dynamically allocated
				unless the psm argument is non-NULL.
				The name argument specifies an episode
				label that will be used in messages
				issued by the trace operations; it
				may be helpful to use a name that
				meaningfully describes the heap that's
				being traced (you can trace in multiple
				heaps at the same time) or the trace
				episode's purpose.  If the sptrace
				episode uniquely identified by the
				shared memory key already exists, the
				effect is the same as sptrace_join
				(below).  On success, returns a space
				management handle for the episode's
				PSM-managed shared memory region.
				Returns NULL on any error.		*/

extern PsmPartition	sptrace_join(int key, size_t size, char *region,
					PsmPartition psm, char *name);
			/*	Locates the sptrace episode identified
				by key (see sptrace_start), verifies
				size, region, and name against that
				episode, and returns a space management
				handle for the episode's PSM-managed
				shared memory region; the PSM space
				management structure that the handle
				points to will be be dynamically
				allocated as needed unless the psm
				argument is non-NULL.  If no such
				sptrace episode exists, or on any
				other error, returns NULL.		*/

extern void		sptrace_log_alloc(PsmPartition trace,
					uaddr addr, size_t size,
					const char *fileName, int lineNbr);
			/*	Causes sptrace to log a space allocation
				event in the indicated sptrace episode.
				addr and size are the address and size
				of the newly allocated object.  fileName
				and lineNbr identify the line of
				application source code at which the
				activity being logged was initiated.	*/

extern void		sptrace_log_free(PsmPartition trace, uaddr addr,
					const char *fileName, int lineNbr);
			/*	Causes sptrace to log a space release
				event.  addr is the address of the newly
				freed object.  fileName and lineNbr
				identify the line of application source
				code at which the activity being logged
				was initiated.				*/

extern void		sptrace_log_memo(PsmPartition trace, uaddr addr,
					char *msg, const char *fileName,
					int lineNbr);
			/*	Causes sptrace to log a heap management
				memo.  addr is the address to which the
				memo refers; msg is the memo text.
				fileName and lineNbr identify the line
				of application source code at which the
				activity being logged was initiated.	*/

extern void		sptrace_report(PsmPartition trace, int verbose);
			/*	Prints a report from the trace log,
				starting at the beginning of the trace
				episode, using the writeMemo function
				in 'platform'.  If the 'verbose' flag
				is zero, only exceptions (e.g., objects
				that have been allocated but not yet
				freed) and memos are printed; if it is
				non-zero, the entire log is printed.	*/

extern void		sptrace_clear(PsmPartition trace);
			/*	Deletes from the trace log all
				allocation events for which matching
				release events have been logged, and
				also deletes those matching release
				events, leaving only events that would
				be reportable as exceptions.  Enables
				trace to run for a long time without
				running out of log space.		*/

extern void		sptrace_stop(PsmPartition trace);
			/*	Ends the current episode of Sptrace
				space management tracing and releases
				the shared memory allocated to the
				trace operations.			*/

#ifdef __cplusplus
}
#endif

#endif  /* _SPTRACE_H_ */
