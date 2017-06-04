/*
	memmgr.h:	public header file for memmgr library.

	Copyright (c) 2001, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/
#ifndef _MEMMGR_H_
#define _MEMMGR_H_

#include "psm.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void		*(* MemAllocator)(const char *fileName, int lineNbr,
				size_t size);
typedef void		(* MemDeallocator)(const char *fileName, int lineNbr,
				void * block);
typedef void    	*(* MemAtoPConverter)(uaddr address);
typedef uaddr		(* MemPtoAConverter)(void * pointer);

/*	NOTE: memmgr_add is NOT thread-safe.  In a multithreaded
	execution image (e.g., VxWorks), all memory managers must be
	loaded *before* any subordinate threads or tasks are spawned.

	The memory manager with ID = 0 is always available; its name
	is "std".  Its "take" function is memalign() with the
	allocated space initialized to binary zeros; its "release"
	function is free().  Its AtoP and PtoA functions are no-ops.	*/

int		memmgr_add(	char 		*name,
				MemAllocator	take,
				MemDeallocator	release,
				MemAtoPConverter AtoP,
				MemPtoAConverter PtoA);
		/*	Appends new memory manager of indicated name
			to memmgr's internal array of memory managers
			and returns the ID number assigned to this
			memory manager.					*/

int		memmgr_find(	char *name);
		/*	Returns the ID number assigned to the named
			memory manager.					*/

char		*memmgr_name(	int mgrId);
		/*	Returns the name of this memory manager.	*/

MemAllocator	memmgr_take(	int mgrId);
		/*	Returns the "take" function of this memory
			manager.					*/

MemDeallocator	memmgr_release(	int mgrId);
		/*	Returns the "release" function of this memory
			manager.					*/

MemAtoPConverter memmgr_AtoP(   int mgrId);
		/*	Returns the "AtoP" function of this memory
			manager.					*/

MemPtoAConverter memmgr_PtoA(   int mgrId);
		/*	Returns the "PtoA" function of this memory
			manager.					*/

int		memmgr_open(int memKey, size_t memSize, char **memPtr,
			uaddr *smId, char *partitionName,
			PsmPartition *partition, int *memMgr,
			MemAllocator afn, MemDeallocator ffn,
			MemAtoPConverter apfn, MemPtoAConverter pafn);
		/*	Opens one avenue of access to a PSM-managed
			region of shared memory, initializing as
			necessary.					*/

void		memmgr_destroy(uaddr smId, PsmPartition *partition);
		/*	Terminates all access to a PSM-managed region
			of shared memory.				*/

#ifdef __cplusplus
}
#endif

#endif  /* _MEMMGR_H_ */
