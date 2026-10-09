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
#include <stdint.h>
#include <string.h>

/*
 * Prefix allocations with a header recording the usable size.
 * ion_bsl_realloc_cb needs the old size to grow a block without reading past
 * the original (ION working memory can't grow in place, so it copies to a new
 * block), and the ION realloc/free interface does not otherwise carry the old
 * size.  A magic word marks blocks allocated through this layer;
 * ion_bsl_free_cb ignores any other pointer.
 */

#define ION_BSL_HDR_MAGIC ((uint64_t) 0xB5100FFEE0000001ULL)
#define ION_BSL_HDR_SZ	  (2 * sizeof(uint64_t)) /* 16; preserves 8/16 align */

static void *ion_bsl_hdr_alloc(const char *file, int line, size_t size, int zero)
{
	uint64_t *hdr;
	char	 *user;

	(void) zero; /* allocFromIonMemory zeroes the block */

	if (size > SIZE_MAX - ION_BSL_HDR_SZ)
	{
		return NULL;
	}

	hdr = (uint64_t *) allocFromIonMemory(file, line, ION_BSL_HDR_SZ + size);
	if (hdr == NULL)
	{
		return NULL;
	}

	hdr[0] = ION_BSL_HDR_MAGIC;
	hdr[1] = (uint64_t) size;
	user = ((char *) hdr) + ION_BSL_HDR_SZ;

	return user;
}

/* Extract header if valid; returns 1 if allocated here, 0 otherwise. */
static int ion_bsl_hdr_read(const void *user, size_t *oldSize)
{
	uint64_t hdr[2];
	memcpy(hdr, (const char *) user - ION_BSL_HDR_SZ, sizeof(hdr));
	if (hdr[0] == ION_BSL_HDR_MAGIC)
	{
		*oldSize = (size_t) hdr[1];
		return 1;
	}

	*oldSize = 0;
	return 0;
}

static void ion_bsl_hdr_free(const char *file, int line, void *user)
{
	char  *hdr_ptr = (char *) user - ION_BSL_HDR_SZ;
	size_t oldSize = 0;
	size_t i;

	volatile unsigned char *p;

	if (ion_bsl_hdr_read(user, &oldSize))
	{
		/*
		 * Zeroize header (defusing magic) and cryptographic/user
		 * payload.
		 */
		p = (volatile unsigned char *) hdr_ptr;
		for (i = 0; i < ION_BSL_HDR_SZ + oldSize; i++)
		{
			p[i] = 0;
		}

		releaseToIonMemory(file, line, hdr_ptr);
	}
	else
	{
		writeMemo("[!] ion_bsl_hdr_free: invalid or double-freed pointer ignored.");
	}
}

void *ion_bsl_malloc_cb(size_t size)
{
	return ion_bsl_hdr_alloc(__FILE__, __LINE__, size, 0);
}

void *ion_bsl_realloc_cb(void *ptr, size_t size)
{
	void  *newMem;
	size_t oldSize;
	size_t copyLen;

	/*
	 * ION working memory cannot be grown in place (the adjacent space is
	 * typically occupied), so allocate a new block and copy the old
	 * contents.  The header lets us copy exactly min(oldSize, size)
	 * instead of reading past the old block.
	 */

	if (size == 0)
	{
		if (ptr)
		{
			ion_bsl_hdr_free(__FILE__, __LINE__, ptr);
		}

		return NULL;
	}

	newMem = ion_bsl_hdr_alloc(__FILE__, __LINE__, size, 0);
	if (newMem == NULL)
	{
		return NULL;
	}

	if (ptr)
	{
		if (ion_bsl_hdr_read(ptr, &oldSize))
		{
			copyLen = (oldSize < size) ? oldSize : size;
		}
		else
		{
			copyLen = 0;
		}

		if (copyLen > 0)
		{
			memcpy(newMem, ptr, copyLen);
		}

		ion_bsl_hdr_free(__FILE__, __LINE__, ptr);
	}

	return newMem;
}

void *ion_bsl_calloc_cb(size_t nmemb, size_t size)
{
	if (size != 0 && nmemb > SIZE_MAX / size)
	{
		return NULL;
	}

	return ion_bsl_hdr_alloc(__FILE__, __LINE__, nmemb * size, 1);
}

void ion_bsl_free_cb(void *ptr)
{
	if (ptr)
	{
		ion_bsl_hdr_free(__FILE__, __LINE__, ptr);
	}
}
