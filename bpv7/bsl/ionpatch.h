/*
 	ionpatch.h:	private definitions supporting the use
			of ION's private memory management system
			in the BPSec Library.

	Author: Scott Burleigh

	Copyright (c) 2025, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
 									*/
#include <sys/time.h>
#include "ion.h"

/*	Undefine BSL's default memory allocators (from BSLConfig.h) so we
 *	can redefine them to use ION's memory management system instead.	*/
#undef BSL_MALLOC
#undef BSL_CALLOC
#undef BSL_REALLOC
#undef BSL_FREE

#define BSL_MALLOC(size)	ion_malloc(__FILE__, __LINE__, size)
#define BSL_CALLOC(count, size)	ion_calloc(__FILE__, __LINE__, count, size)
#define BSL_REALLOC(mem, size)	ion_realloc(__FILE__, __LINE__, mem, size)
#define BSL_FREE(mem)		ion_free(__FILE__, __LINE__, mem)

extern void		*ion_malloc(const char *, int, size_t);
extern void		*ion_calloc(const char *, int, size_t, size_t);
extern void		*ion_realloc(const char *, int, void *, size_t);
extern void		ion_free(const char *, int,  void *);

extern void		*ion_bsl_malloc_cb(size_t);
extern void		*ion_bsl_realloc_cb(void *, size_t);
extern void		*ion_bsl_calloc_cb(size_t, size_t);
extern void		ion_bsl_free_cb(void *);
