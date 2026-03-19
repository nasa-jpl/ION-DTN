/*
	ion_atomic.h:	Portable atomic operations for ION.

	If the compiler provides C11 <stdatomic.h>, use it
	directly.  Otherwise fall back to GCC/Clang __atomic
	built-ins, which are available in C99 mode on GCC 4.7+
	and all versions of Clang.

	Author: ION team, JPL

	Copyright (c) 2024, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
								*/
#ifndef _ION_ATOMIC_H_
#define _ION_ATOMIC_H_

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L \
    && !defined(__STDC_NO_ATOMICS__)

/*	Native C11 atomics.					*/

#include <stdatomic.h>

#else  /* C99 fallback using GCC/Clang __atomic built-ins.	*/

#define _Atomic(T)		T
#define atomic_int		int
#define atomic_uint		unsigned int
#define atomic_ullong		unsigned long long

#define atomic_init(p, v)	(*(p) = (v))
#define atomic_store(p, v)	__atomic_store_n((p), (v), __ATOMIC_SEQ_CST)
#define atomic_load(p)		__atomic_load_n((p), __ATOMIC_SEQ_CST)
#define atomic_fetch_add(p, v)	__atomic_fetch_add((p), (v), __ATOMIC_SEQ_CST)
#define atomic_fetch_sub(p, v)	__atomic_fetch_sub((p), (v), __ATOMIC_SEQ_CST)
#define atomic_exchange(p, v)	__atomic_exchange_n((p), (v), __ATOMIC_SEQ_CST)

#endif
#endif /* _ION_ATOMIC_H_ */
