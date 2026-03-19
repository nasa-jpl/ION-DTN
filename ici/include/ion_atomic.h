/*
	ion_atomic.h:	Portable atomic operations for ION.

	Three compilation paths:

	1. C++ — uses <atomic> with std::atomic<T> type aliases.
	   C11 <stdatomic.h> is not available to C++ compilers
	   (optional since C++23).  std::atomic<T> and C11
	   _Atomic T are layout-compatible on GCC, Clang, and
	   MSVC, so struct layouts in ion.h / bpP.h / ltpP.h
	   match across C and C++ translation units.

	2. C11/C18 — includes <stdatomic.h> directly.

	3. C99 — falls back to GCC/Clang __atomic built-ins,
	   which are available in C99 mode on GCC 4.7+ and all
	   versions of Clang.

	Author: ION team, JPL

	Copyright (c) 2024, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
								*/
#ifndef _ION_ATOMIC_H_
#define _ION_ATOMIC_H_

#ifdef __cplusplus

/*	C++ path: <atomic> provides std::atomic<T>.  The
 *	type aliases (atomic_int, atomic_uint, atomic_ullong)
 *	are kept identical to the C names so that shared
 *	struct definitions compile in both languages.
 *
 *	platform.h wraps its contents in extern "C", which
 *	must be temporarily closed for the C++ <atomic>
 *	header, then reopened.					*/

}  /* Close extern "C" from platform.h. */

#include <atomic>
using std::atomic_int;
using std::atomic_uint;
using std::atomic_ullong;

extern "C" {  /* Reopen extern "C" for the rest of platform.h. */

#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L \
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
