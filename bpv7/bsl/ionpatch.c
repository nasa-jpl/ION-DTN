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
#include "ion.h"
#include <stddef.h>
#include <string.h>

void	*ion_bsl_malloc_cb(size_t size)
{
	return allocFromIonMemory(__FILE__, __LINE__, size);
}

void	*ion_bsl_realloc_cb(void *ptr, size_t size)
{
	void	*newMem;

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
		if (ptr)
		{
			releaseToIonMemory(__FILE__, __LINE__, ptr);
		}

		newMem = NULL;
	}
	else
	{
		newMem = allocFromIonMemory(__FILE__, __LINE__, size);
		if (newMem == NULL)
		{
			newMem = ptr;	/*	Retain orginal data.	*/
		}
		else	/*	New memory block was obtained.		*/
		{
			if (ptr)
			{
				memcpy(newMem, ptr, size);
				releaseToIonMemory(__FILE__, __LINE__, ptr);
			}
		}
	}

	return newMem;
}

void	*ion_bsl_calloc_cb(size_t nmemb, size_t size)
{
	return allocFromIonMemory(__FILE__, __LINE__, nmemb * size);
}

void	ion_bsl_free_cb(void *ptr)
{
	releaseToIonMemory(__FILE__, __LINE__, ptr);
}
