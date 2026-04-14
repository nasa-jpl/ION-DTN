/*
 * ion_atomic.h: Portable atomic operations for ION-DTN.
 *
 * Three compilation paths:
 *
 * 1. C++ — uses <atomic> with std::atomic<T> type aliases.
 * C11 <stdatomic.h> is not available to C++ compilers
 * (optional since C++23). std::atomic<T> and C11
 * _Atomic T are layout-compatible on GCC, Clang, and
 * MSVC, so struct layouts in ion.h / bpP.h / ltpP.h
 * match across C and C++ translation units.
 *
 * 2. C11/C18 — includes <stdatomic.h> directly.
 *
 * 3. C99 — falls back to GCC/Clang __atomic built-ins,
 * which are available in C99 mode on GCC 4.7+ and all
 * versions of Clang.
 *
 * Author: ION team, JPL
 *
 * Copyright (c) 2024, California Institute of Technology.
 * ALL RIGHTS RESERVED. U.S. Government Sponsorship acknowledged.
 */

#ifndef ION_ATOMIC_H
#define ION_ATOMIC_H

#include "platform.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/*==================================================================*/
/* C++ COMPATIBILITY PATH                                           */
/*------------------------------------------------------------------*/
/* External C++ programs that link against the ION C library need   */
/* the headers (platform.h, ion.h, bpP.h, ltpP.h, etc.) to parse    */
/* cleanly under a C++ compiler, and they need struct layouts to    */
/* match the C compilation so pointers can cross the C/C++ boundary */
/* without corruption.                                              */
/*                                                                  */
/* The C11 `_Atomic(T)` type qualifier and GCC/Clang `__atomic_*`   */
/* built-ins used by the C paths below are not valid in C++.        */
/* (C++23 added `std::atomic_ref<T>` but we cannot depend on it.)   */
/*                                                                  */
/* C++ consumers typically call the ION C API (bp_attach, bp_send,  */
/* ipnadminep, etc.) and do not touch `ion_atomic_t` /              */
/* `ion_ipc_atomic_t` fields directly — the atomic manipulation     */
/* happens inside the C library.  This path therefore exposes the   */
/* two atomic types as opaque byte blobs with sizes and alignments  */
/* matching the C compilation, and omits the accessor macros and    */
/* inline functions (which are C-only).                             */
/*                                                                  */
/* The size of `ion_atomic_t` in C++ is selected to match whichever */
/* representation the linked ION C library was built with, using    */
/* the `ION_HAVE_C11_ATOMICS` flag from config.h:                   */
/*                                                                  */
/*   - If the library was built with C11 <stdatomic.h>, Zone 1 in   */
/*     C is a plain `_Atomic(vast)` (8 bytes), so the C++ type is   */
/*     `opaque[sizeof(long long)]` (also 8 bytes).                  */
/*                                                                  */
/*   - Otherwise the C fallback is a mutex-backed union (64 bytes   */
/*     to fit a pthread_mutex_t plus a value), and the C++ type    */
/*     is `opaque[64]` to match.                                    */
/*                                                                  */
/* `ion_ipc_atomic_t` is always 8 bytes (`sizeof(long long)`) on    */
/* every Zone 2 tier (`_Atomic(vast)` on C11, `volatile vast` on    */
/* the __atomic / __sync fallbacks), because `vast` is an 8-byte    */
/* integer on all supported ION targets.                            */
/*==================================================================*/

#ifdef __cplusplus

/*
 * Zone 1 (ion_atomic_t) size depends on how the linked ION C
 * library was compiled.  The build system (configure.ac) defines
 * ION_HAVE_C11_ATOMICS=1 in config.h when it detects a working
 * <stdatomic.h>.  The C path above then picks the native
 * _Atomic(vast) representation (8 bytes), otherwise it falls
 * back to a 64-byte mutex-backed union.
 *
 * The C++ path must match whichever representation the library
 * was built with, so consumers see the same struct layout on
 * both sides of the C/C++ boundary.  Since config.h is available
 * here (via HAVE_CONFIG_H above), we read the same flag.
 */

#if defined(ION_HAVE_C11_ATOMICS) && ION_HAVE_C11_ATOMICS

typedef struct {
	alignas(alignof(long long))	unsigned char	opaque[sizeof(long long)];
} ion_atomic_t;

#else /* C99 mutex-backed fallback — 64-byte padded union in C */

typedef struct {
	alignas(alignof(long long))	unsigned char	opaque[64];
} ion_atomic_t;

#endif

/* Zone 2 (ion_ipc_atomic_t) is always the same size on every
 * fallback tier: an 8-byte integer (_Atomic(vast) on C11, or
 * volatile vast on the __atomic / __sync fallbacks).		*/
typedef struct {
	alignas(alignof(long long))	unsigned char	opaque[sizeof(long long)];
} ion_ipc_atomic_t;

/* C++ code must not construct these directly; any initialization
 * happens inside the C library (e.g., via ion_atomic_init()).  The
 * zero-initializer is provided only so that static struct
 * initializers in ION C code remain syntactically valid if the
 * header is accidentally parsed as C++.				*/
#define ION_ATOMIC_INIT(v)		{ { 0 } }

#else /* !__cplusplus — C compilation paths begin */

/*==================================================================*/
/* Feature Flag Initialization & Test Override                      */
/*==================================================================*/

/* * Test Harness Override:
 * Force the C99 fallback on modern compilers to validate thread safety.
 */
#if defined(ION_TEST_FORCE_FALLBACK)
# undef ION_HAVE_C11_ATOMICS
# define ION_HAVE_C11_ATOMICS 0
#endif

/* Detect C11 stdatomic.h support if not explicitly overridden */
#if !defined(ION_HAVE_C11_ATOMICS)
# if __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_ATOMICS__)
#  define ION_HAVE_C11_ATOMICS 1
# else
#  define ION_HAVE_C11_ATOMICS 0
# endif
#endif

/*
 * Detect GCC/Clang __atomic built-ins (available in GCC 4.7+ and all
 * versions of Clang).  These are used as an intermediate fallback tier
 * on C99 builds, so that modern compilers can emit weak memory-ordered
 * instructions (e.g., plain LDR/STR, LDADD on ARMv8.1) instead of the
 * heavyweight full-barrier sequences that __sync built-ins always emit.
 *
 * This tier exists primarily for ARM/AArch64 targets where the cost of
 * __sync's mandatory DMB ISH barriers is significant on hot paths.
 */
#if !defined(ION_HAVE_GNU_ATOMIC)
# if defined(__clang__)
#  define ION_HAVE_GNU_ATOMIC 1
# elif defined(__GNUC__) && \
       (__GNUC__ * 10000 + __GNUC_MINOR__ * 100) >= 40700
#  define ION_HAVE_GNU_ATOMIC 1
# else
#  define ION_HAVE_GNU_ATOMIC 0
# endif
#endif

/*
 * Test Harness Override:
 * Force the legacy __sync fallback path even on modern compilers, so
 * the __sync tier can be validated under ION_TEST_FORCE_FALLBACK.
 */
#if defined(ION_TEST_FORCE_SYNC_FALLBACK)
# undef  ION_HAVE_GNU_ATOMIC
# define ION_HAVE_GNU_ATOMIC 0
#endif

/*==================================================================*/
/* ZONE 1: PROCESS-LOCAL ATOMICS (HEAP / STACK ONLY)                */
/*------------------------------------------------------------------*/
/* WARNING: DO NOT use ion_atomic_t in shared memory (SDR or SM).   */
/* On C99 fallbacks, this type is backed by a standard POSIX mutex. */
/* Sharing a standard process-local mutex across discrete process   */
/* boundaries will cause deadlocks or catastrophic segfaults.       */
/*==================================================================*/

#if ION_HAVE_C11_ATOMICS

#include <stdatomic.h>

/**
 * @brief Process-Local Atomic Type (C11)
 *
 * On the C11 path, ion_atomic_t is a plain _Atomic(vast) — 8 bytes
 * on 64-bit targets — matching the natural size of a C11 atomic
 * integer.  No padding is applied.
 *
 * Earlier iterations of this header wrapped the type in a 64-byte
 * padded union to force ABI parity with the C99 mutex-backed
 * fallback.  That rationale no longer applies:
 *
 *   1. All hot-path arrays of atomics (BP/LTP TallyDelta, BpVdb
 *      delDeltas) were moved to Zone 2 (ion_ipc_atomic_t) in the
 *      shared-memory-safety fix, so the large memory-inflation
 *      concern that motivated the padding is already resolved.
 *
 *   2. Remaining Zone 1 uses are scattered single-field atomics
 *      (daemon shutdown flags, per-SAP state, init flags).  These
 *      are embedded inside much larger enclosing structs that are
 *      written concurrently as a whole, so cache-line padding on
 *      just the atomic field provides no real false-sharing
 *      protection.
 *
 *   3. A single ION binary is compiled under exactly one C
 *      language standard — all .c files share the same -std flag
 *      from configure.ac — so a mixed C11/C99 build within the
 *      same process cannot legitimately arise.  ABI parity
 *      between compilation standards has no operational meaning.
 */
typedef _Atomic(vast) ion_atomic_t;

/* C11 Static Initializer */
#define ION_ATOMIC_INIT(v) (v)

/* C11 Static Inline Wrappers to prevent macro shadowing */
static inline void ion_atomic_init(ion_atomic_t *p, vast v) {
    atomic_init(p, v);
}
static inline void ion_atomic_set(ion_atomic_t *p, vast v) {
    atomic_store_explicit(p, v, memory_order_relaxed);
}
static inline uvast ion_atomic_get(ion_atomic_t *p) {
    return atomic_load_explicit(p, memory_order_relaxed);
}
static inline uvast ion_atomic_get_and_increment(ion_atomic_t *p, vast d) {
    return atomic_fetch_add_explicit(p, d, memory_order_relaxed);
}
static inline uvast ion_atomic_get_and_decrement(ion_atomic_t *p, vast d) {
    return atomic_fetch_sub_explicit(p, d, memory_order_relaxed);
}
static inline uvast ion_atomic_exchange(ion_atomic_t *p, vast v) {
    return atomic_exchange_explicit(p, v, memory_order_relaxed);
}

/* Mutex destruction is a no-op under native C11 atomics */
#define ion_atomic_mutex_destroy(p)         ((void)0)

#else /* POSIX / C99 mutex fallback */

#include <pthread.h>

/**
 * @brief Process-Local Atomic Type (C99 Fallback)
 * Encapsulates the integer within a POSIX mutex to guarantee strict
 * serialization and thread-safety on older compilers.
 */
typedef union {
    struct {
        pthread_mutex_t lock;
        uvast           value;
    } fallback;
    char padding[64];
} ion_atomic_t;

/* C99 Static Initializer */
#define ION_ATOMIC_INIT(v) { .fallback = { PTHREAD_MUTEX_INITIALIZER, (v) } }

/* Function prototypes implemented in ion_atomic.c */
void  ion_atomic_init              (ion_atomic_t *, vast);
void  ion_atomic_mutex_destroy     (ion_atomic_t *);
void  ion_atomic_set               (ion_atomic_t *, vast);
uvast ion_atomic_get               (ion_atomic_t *);
uvast ion_atomic_get_and_increment (ion_atomic_t *, vast);
uvast ion_atomic_get_and_decrement (ion_atomic_t *, vast);
uvast ion_atomic_exchange          (ion_atomic_t *, vast);

#endif /* ION_HAVE_C11_ATOMICS Process-Local */


/*==================================================================*/
/* ZONE 2: SHARED MEMORY / IPC ATOMICS (SDR / SM ONLY)              */
/*------------------------------------------------------------------*/
/* Use ion_ipc_atomic_t strictly for variables residing in memory   */
/* mapped across multiple processes. This type strips out mutexes   */
/* entirely to guarantee lock-free, async-signal-safe execution     */
/* across strict process boundaries.                                */
/*==================================================================*/

#if ION_HAVE_C11_ATOMICS

/*
 * Tier 1: Native C11 <stdatomic.h>.
 *
 * @brief Inter-Process Atomic Type (C11)
 * Used strictly for variables residing in shared memory mapping (e.g., SDR).
 */
typedef _Atomic(vast) ion_ipc_atomic_t;

#define ion_ipc_atomic_init(p,v)                atomic_init((p), (v))
#define ion_ipc_atomic_set(p,v)                 atomic_store_explicit((p), (v), memory_order_relaxed)
#define ion_ipc_atomic_get(p)                   atomic_load_explicit((p), memory_order_relaxed)
#define ion_ipc_atomic_get_and_increment(p,d)   atomic_fetch_add_explicit((p), (d), memory_order_relaxed)
#define ion_ipc_atomic_get_and_decrement(p,d)   atomic_fetch_sub_explicit((p), (d), memory_order_relaxed)
#define ion_ipc_atomic_exchange(p,v)            atomic_exchange_explicit((p), (v), memory_order_relaxed)

#elif ION_HAVE_GNU_ATOMIC

/*
 * Tier 2: GCC/Clang __atomic built-ins.
 *
 * @brief Inter-Process Atomic Type (GCC/Clang __atomic fallback)
 *
 * Uses __ATOMIC_RELAXED to match the ordering of the C11 path above.
 * On ARM/AArch64 this permits the compiler to emit plain loads/stores
 * and LDADD (ARMv8.1) instead of the full DMB ISH barriers that the
 * legacy __sync built-ins always emit.
 *
 * The typedef matches Tier 3 (volatile vast) so struct layouts are
 * binary-compatible between the two __sync / __atomic fallback tiers.
 */
typedef volatile vast ion_ipc_atomic_t;

#define ion_ipc_atomic_init(p,v)                (*(p) = (v))
#define ion_ipc_atomic_set(p,v)                 __atomic_store_n((p), (v), __ATOMIC_RELAXED)
#define ion_ipc_atomic_get(p)                   __atomic_load_n((p), __ATOMIC_RELAXED)
#define ion_ipc_atomic_get_and_increment(p,d)   __atomic_fetch_add((p), (d), __ATOMIC_RELAXED)
#define ion_ipc_atomic_get_and_decrement(p,d)   __atomic_fetch_sub((p), (d), __ATOMIC_RELAXED)
#define ion_ipc_atomic_exchange(p,v)            __atomic_exchange_n((p), (v), __ATOMIC_RELAXED)

#else /* Tier 3: Legacy __sync built-ins */

/*
 * Tier 3: Legacy __sync built-ins.
 *
 * @brief Inter-Process Atomic Type (legacy __sync fallback)
 *
 * Retained for pre-GCC-4.7 toolchains on certified flight hardware
 * (RAD750, LEON cores, older RTEMS/VxWorks).  The volatile keyword
 * forces memory access (bypassing registers) to support the __sync
 * hardware built-ins.  Provides lock-free, async-signal-safe
 * atomicity across process boundaries, but always emits full memory
 * barriers (e.g., DMB ISH on ARM).
 */
typedef volatile vast ion_ipc_atomic_t;

#define ion_ipc_atomic_init(p,v)                (*(p) = (v))
#define ion_ipc_atomic_set(p,v)                 __sync_lock_test_and_set((p), (v))
#define ion_ipc_atomic_get(p)                   __sync_fetch_and_add((p), 0)
#define ion_ipc_atomic_get_and_increment(p,d)   __sync_fetch_and_add((p), (d))
#define ion_ipc_atomic_get_and_decrement(p,d)   __sync_fetch_and_sub((p), (d))
#define ion_ipc_atomic_exchange(p,v)            __sync_lock_test_and_set((p), (v))

#endif /* ION_HAVE_C11_ATOMICS IPC */

#endif /* !__cplusplus */

#endif /* ION_ATOMIC_H */
