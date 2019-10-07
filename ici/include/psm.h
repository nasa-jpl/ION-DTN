/*
 	psm.h:		definitions supporting personal space
			management.  See the psm man page for details.

	Author: Scott Burleigh, JPL

	Modification History:
	Date      Who   What
	11-26-02  SCB   Added "trace" functions.

	Copyright (c) 2001, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
 									*/
#ifndef _PSM_H_
#define _PSM_H_

#include "platform.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	char	partitionName[32];
	size_t	partitionSize;
	size_t	smallPoolSize;
	size_t	smallPoolFreeBlockCount[SMALL_SIZES];
	size_t	smallPoolFree;
	size_t	smallPoolAllocated;
	size_t	largePoolSize;
	size_t	largePoolFreeBlockCount[LARGE_ORDERS];
	size_t 	largePoolFree;
	size_t	largePoolAllocated;
	size_t	unusedSize;
} PsmUsageSummary;

typedef struct psm_str		/*	Local view of managed memory.	*/
{
	char		*space;		/*	Local pointer.		*/
	long		freeNeeded;	/*	Free PsmView?  Boolean.	*/
	struct psm_str	*trace;		/*	For sptrace.		*/
	long		traceArea[3];	/*	psm_str for sptrace.	*/
} PsmView, *PsmPartition;

typedef enum { Okay, Redundant, Refused } PsmMgtOutcome;
typedef uaddr		PsmAddress;

extern int		psm_manage(char *, size_t, char *, PsmPartition *psmp,
					PsmMgtOutcome *outcome);
			/*	Arguments are:
					pointer to start of the
						memory to manage
					length of memory area (bytes)
					name of partition (max 31
						characters)
					pointer to variable in which
						psm space management
						handle will be returned
					pointer to variable in which
						outcome will be returned

		 		A new psm space management structure
				will be dynamically allocated (and its
				address returned as the space management
				handle) unless *psmp is non-NULL, i.e.,
				*psmp is already a handle pointing to a
				previously allocated psm space management
				structure.		 		*/

extern char		*psm_name(PsmPartition);
			/*	Returns name of partition.		*/

extern char		*psm_space(PsmPartition);
			/*	Returns pointer to the start of the
				memory region managed by this PSM
				partition.  This function is provided
				to enable the application to do an
				operating-system release (such as
				free()) of this memory when the managed
				partition is no longer needed.  NOTE(!)
				that calling psm_erase() or psm_unmanage()
				[or any other PSM function, for that
				matter] after releasing that space is
				virtually guaranteed to result in a
				segmentation fault or other seriously
				bad behavior.				*/

extern void		*psp(PsmPartition, PsmAddress);
			/*	Argument is an offset within the
				partition.  Returns the conversion of
				that offset into an absolute pointer.	*/

extern PsmAddress	psa(PsmPartition, void *);
			/*	Argument is an absolute pointer.
				Returns the conversion of that pointer
				into an offset within the partition.	*/

extern void		psm_panic(PsmPartition);
			/*	Forces standard memory allocation
				algorithm to hunt laboriously for
				free blocks in pools that may not
				contain any.				*/

extern void		psm_relax(PsmPartition);
			/*	Reverses psm_panic.  Lets standard
				memory allocation algorithm return NULL
				when no free block can be found easily.	*/

#define psm_malloc(partition, size) \
Psm_malloc(__FILE__, __LINE__, partition, size)

extern PsmAddress	Psm_malloc(const char *, int, PsmPartition, size_t);
			/*	Argument is size of block to allocate;
				maximum size is 1/2 of the total
				address space (i.e., 2G for a 32-bit
				machine).  Returns zero if no free
				block could be found.  Block returned
				is aligned on doubleword boundary.	*/

#define psm_zalloc(partition, size) \
Psm_zalloc(__FILE__, __LINE__, partition, size)

extern PsmAddress	Psm_zalloc(const char *, int, PsmPartition, size_t);
			/*	Argument is size of block to allocate;
				maximum size is 64 words (i.e., 256 for
				a 32-bit machine).  Allocation is
				performed by an especially speedy
				algorithm and minimum space is consumed
				in memory management overhead.  Returns
				0 if no free block could be found.
				Block returned is aligned on word
				boundary.				*/

#define psm_free(partition, address) \
Psm_free(__FILE__, __LINE__, partition, address)

extern void		Psm_free(const char *, int, PsmPartition, PsmAddress);
			/*	Argument is a block allocated by
				psm_malloc or psm_zalloc.		*/

extern int		psm_set_root(PsmPartition, PsmAddress);
			/*	Sets a pointer at a fixed location in
				the partition to the indicated value,
				so that this value can be retrieved if
				access to the partition is lost and
				then re-acquired (typically for
				partitions allocated in shared or
				static RAM).  The argument -- the
				"root object" -- is normally a pointer
				to a list (or tree) of the lists or
				trees that populate the partition.
	 			Returns 0 on success, -1 on any failure
				(e.g., the partition already has some
				other root object).			 */

extern PsmAddress	psm_get_root(PsmPartition);
			/*	Retrieves the pointer value set by
				psm_set_root().				*/

extern void		psm_erase_root(PsmPartition);
			/*	Detaches the partition from its current
			 	"root object".				*/

#define psm_add_catlg(partition) \
Psm_add_catlg(__FILE__, __LINE__, partition)

extern int		Psm_add_catlg(const char *, int, PsmPartition);
			/*	Allocates space for an object catalog
			 	in the indicated partition, establishes
			 	the new catalog as the partition's
			 	root object.  Returns 0 on success, -1
			 	on any failure (e.g., the partition
			 	already has some other root object).	*/

#define psm_catlg(partition, name, address) \
Psm_catlg(__FILE__, __LINE__, partition, name, address)

extern int		Psm_catlg(const char *, int, PsmPartition,
					char *objName,
					PsmAddress objLocation);
			/*	Inserts an entry for the indicated
			 	object into the catalog that is the
				root object for this partition.  The
				length of objName cannot exceed 32
				bytes, and objName must be unique in
				the catalog.  Returns 0 on success, -1
				on any failure.				*/

#define psm_uncatlg(partition, name) \
Psm_uncatlg(__FILE__, __LINE__, partition, name)

extern int		Psm_uncatlg(const char *, int, PsmPartition,
					char *objName);
			/*	Removes the entry for the indicated
			 	object from the catalog that is the
				root object for this partition, if it
				exists.  Returns 0 on success, -1 on
				any failure.				*/

extern int		psm_locate(PsmPartition, char *objName,
					PsmAddress *objLocation,
					PsmAddress *entryElt);
			/*	Places in "objLocation" the address
			 	associated with the indicated object
				name in the catalog that is the root
				object for this partition and places
				in "entryElt" the address of the list
				element that points to this catalog
				entry; if name is not found in catalog,
				sets entryElt to zero.  Returns 0 on
				success, -1 on any failure.		*/

extern void		psm_usage(PsmPartition, PsmUsageSummary *);
			/*	Loads PsmUsageSummary structure with
				snapshot of partition's usage status.	*/

extern void		psm_report(PsmUsageSummary *);
			/*	Sends to stdout a snapshot of the
				partition's usage status.		*/

extern int		psm_start_trace(PsmPartition, size_t, char *);
                        /*	Begins an episode of psm space usage
				tracing.  The size_t argument specifies
				the amount of shared memory to use
				for the trace operations; this memory
				will be dynamically allocated unless
				the (char *) region pointer argument
				is non-NULL.  Returns 0 on success, -1
				on any error.				*/

extern void		psm_print_trace(PsmPartition, int);
                        /*	Prints cumulative trace report and
				usage report, using writeMemo.  If
				'verbose' int argument is zero, only
				exceptions are reported; otherwise,
				a log of all activity is printed.	*/

extern void		psm_clear_trace(PsmPartition);
                        /*	Deletes closed trace log events.	*/

extern void		psm_stop_trace(PsmPartition);
                        /*	Ends the current episode of psm space
				management tracing and releases
				the shared memory allocated to the
				trace operations.			*/

extern void		psm_unmanage(PsmPartition);
			/*	Terminates psm management of the
				space in the partition and destroys
				this psm management handle to that
				space.					*/

extern void		psm_erase(PsmPartition);
			/*	Discards all information in the space
				managed by this psm management handle
				(preventing re-management of that
				space), in addition to terminating psm
				management of that space and destroying
				the psm management handle.		*/
#ifdef __cplusplus
}
#endif

#endif
