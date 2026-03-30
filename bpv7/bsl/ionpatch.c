/*
 *	ionpatch.c:	functions enabling the use of ION's
 *			private memory management system in
 *			the BPSec Library.
 *
 *	Author: Scott Burleigh
 *
 *	Copyright (c) 2025, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
 */

#include "ionpatch.h"

extern void	*ion_malloc(const char *file, int line, size_t size)
{
#if 0
	writeMemo("[i] ION BSL_MALLOC called.");
#endif
	return allocFromIonMemory(file, line, size);
}

extern void	*ion_calloc(const char *file, int line, size_t ct, size_t size)
{
#if 0
	writeMemo("[i] ION BSL_CALLOC called.");
#endif
	return allocFromIonMemory(file, line, ct * size);
}

extern void	*ion_realloc(const char *file, int line, void *mem, size_t size)
{
	void	*newMem;
#if 0
	writeMemo("[?] ION's dubious function for BSL_REALLOC invoked.");
	printStackTrace();
#endif
	/*	Note that this function is of no use to the 
	 *	BSL_BundleCtx_ReallocBTSD function, since
		ION stores extension blocks in the SDR heap
		rather than in system memory.
	 
	 	Note also that the ION implementation of this
		function does not work in the way that standard
		realloc() works.  The space immediately adjacent
		to a block of ION working memory will typically
		NOT be unoccupied, i.e, will NOT be available
		to be silently appended to that block.

		So the content of the original block is instead
		copied to a newly allocated block of the requested
		size; the address of that new block is returned.	*/

	if (size == 0)
	{
		if (mem)
		{
			releaseToIonMemory(file, line, mem);
		}

		newMem = NULL;
	}
	else
	{
		newMem = allocFromIonMemory(file, line, size);
		if (newMem == NULL)
		{
			newMem = mem;	/*	Retain orginal data.	*/
		}
		else	/*	New memory block was obtained.		*/
		{
			if (mem)
			{
				memcpy(newMem, mem, size);
				releaseToIonMemory(file, line, mem);
			}
		}
	}

	return newMem;
}

extern void	ion_free(const char *file, int line,  void *mem)
{
#if 0
	writeMemo("[i] ION BSL_FREE called.");
#endif
	releaseToIonMemory(file, line, mem);
}
