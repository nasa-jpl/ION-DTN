/*
 	ionpatch.h:	private definitions supporting the use
			of ION's private memory management system
			in the BPSec Library.

	Author: Scott Burleigh

	Copyright (c) 2025, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
 									*/

#ifndef IONPATCH_H
#define IONPATCH_H

#include "platform.h" // IWYU pragma: keep
#include <stddef.h>

extern void		*ion_bsl_malloc_cb(size_t);
extern void		*ion_bsl_realloc_cb(void *, size_t);
extern void		*ion_bsl_calloc_cb(size_t, size_t);
extern void		ion_bsl_free_cb(void *);

#endif /* IONPATCH_H */
