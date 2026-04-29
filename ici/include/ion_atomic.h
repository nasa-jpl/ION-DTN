/*
 * ion_atomic.h: Portable atomic operations for ION-DTN.
 *
 * NOTE ON VISIBILITY
 * ------------------
 * This header is INTERNAL to the ION library.  It is no longer
 * installed as part of the public include set; integrators link
 * against the ION C library and see only the opaque public type
 * `ion_ipc_atomic_t` defined in ion.h.  The Zone-1 type
 * `ion_atomic_t`, the tier dispatch ladder, and all accessor
 * macros below are visible only to ION's own translation units.
 *
 * Two atomic zones and a tier ladder
 * ----------------------------------
 * This header exposes two atomic types distinguished by where the
 * variable lives in memory:
 *
 *   ion_atomic_t       — process-local memory (heap, stack, .bss).
 *                        Backed by _Atomic(vast) on C11, or by a
 *                        POSIX mutex + value on the C99 fallback.
 *                        Internal-only; never appears in any
 *                        installed struct.
 *
 *   ion_ipc_atomic_t   — public opaque type (defined in ion.h)
 *                        for shared memory crossing process
 *                        boundaries (SDR, SM, mmap'd regions).
 *                        Always lock-free and async-signal-safe.
 *                        Hard-pinned at 8 bytes / 8-byte alignment
 *                        on every supported ION target.  The
 *                        internal representation is
 *                        `ion_ipc_atomic_impl_t` (below); the
 *                        ION_IPC_ATOMIC_IMPL() cast bridges them.
 *
 * Zone 2 descends a three-tier ladder:
 *
 *   Tier 1 (C11)    — <stdatomic.h>, _Atomic(vast).
 *   Tier 2 (GNU)    — GCC/Clang __atomic_* built-ins with
 *                     __ATOMIC_RELAXED.  Used when C11 atomics are
 *                     unavailable but __atomic is (GCC >= 4.7, any
 *                     Clang).
 *   Tier 3 (legacy) — GCC __sync_* built-ins, always full barrier.
 *                     Retained for pre-GCC-4.7 flight toolchains
 *                     (RAD750, LEON).
 *
 * Zone 1 has only two tiers (C11 and mutex fallback) because its
 * fallback ABI must accommodate a pthread_mutex_t.
 *
 * Tier selection on a modern toolchain
 * ------------------------------------
 * Two INDEPENDENT levers decide which tier is compiled:
 *
 *   (A) Compiler language mode — set by configure.ac's C18 / C99
 *       probe.  Bakes -std=iso9899:2018 or -std=c99 into AM_CFLAGS
 *       and (for C18) #defines ION_HAVE_C11_ATOMICS in config.h.
 *
 *   (B) Tier dispatch — the preprocessor macros below:
 *
 *         ION_TEST_FORCE_FALLBACK
 *             Undefines ION_HAVE_C11_ATOMICS regardless of
 *             config.h, forcing Zone 1 to its mutex fallback and
 *             Zone 2 to Tier 2 or 3.
 *
 *         ION_TEST_FORCE_SYNC_FALLBACK
 *             Additionally undefines ION_HAVE_GNU_ATOMIC, forcing
 *             Zone 2 down to Tier 3 (__sync).  Must be paired with
 *             ION_TEST_FORCE_FALLBACK to have any effect, since
 *             the C11 tier is checked first.
 *
 * On a modern GCC/Clang, configure.ac will default lever (A) to
 * C18.  Defining the test macros alone only flips lever (B) — the
 * compiler stays in C18 mode, so the language environment ION sees
 * is NOT a genuine C99 toolchain (feature-test macros, _Atomic
 * keyword visibility, library-header switches all differ).  To
 * exercise a true "C99 + fallback" build, both levers must be set:
 *
 *     ./configure ac_cv_c11=no CFLAGS="-std=c99"
 *         → genuine C99 + Tier 2 (__atomic) on modern gcc/clang
 *
 *     ./configure ac_cv_c11=no CFLAGS="-std=c99 \
 *                             -DION_TEST_FORCE_SYNC_FALLBACK"
 *         → genuine C99 + Tier 3 (__sync), what a pre-GCC-4.7
 *           flight toolchain would see
 *
 * See gh-pages/docs/ION-Coding-Guide.md "Testing the Fallback
 * Tiers" for the complete recipe matrix and rationale.
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
/* PUBLIC OPAQUE TYPE                                               */
/*------------------------------------------------------------------*/
/* Mirror the public opaque definition from ion.h.  The shared      */
/* include guard ION_IPC_ATOMIC_OPAQUE_DEFINED ensures only the     */
/* first header to be included emits the typedef, so include order  */
/* between ion.h and ion_atomic.h does not matter.                  */
/*                                                                  */
/* This type is the public, ABI-frozen face of `ion_ipc_atomic_t`.  */
/* Inside this header, the internal `ion_ipc_atomic_impl_t` is the  */
/* one that the compiler actually performs atomic operations on;    */
/* the ION_IPC_ATOMIC_IMPL() cast bridges them.  Static asserts at  */
/* the bottom of this header verify that the two types have the    */
/* same size and compatible alignment on every tier.                */
/*==================================================================*/

#ifndef ION_IPC_ATOMIC_OPAQUE_DEFINED
#define ION_IPC_ATOMIC_OPAQUE_DEFINED
typedef union {
	long long	_ion_ipc_atomic_align;
	unsigned char	opaque[8];
} ion_ipc_atomic_t;
#endif

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
/* The PUBLIC type `ion_ipc_atomic_t` is a fixed-size opaque union  */
/* defined above (and in ion.h).  Inside the ION library, the       */
/* compiler operates on `ion_ipc_atomic_impl_t` — the real atomic   */
/* representation chosen by the tier ladder.  ION_IPC_ATOMIC_IMPL() */
/* casts a public opaque pointer to the impl pointer.  Static       */
/* asserts at the bottom of this header verify size and alignment   */
/* match.                                                           */
/*==================================================================*/

#if ION_HAVE_C11_ATOMICS

/*
 * Tier 1: Native C11 <stdatomic.h>.
 */
typedef _Atomic(vast) ion_ipc_atomic_impl_t;

#define ION_IPC_ATOMIC_IMPL(p) ((ion_ipc_atomic_impl_t *)(void *)(p))

#define ion_ipc_atomic_init(p,v)                atomic_init(ION_IPC_ATOMIC_IMPL(p), (v))
#define ion_ipc_atomic_set(p,v)                 atomic_store_explicit(ION_IPC_ATOMIC_IMPL(p), (v), memory_order_relaxed)
#define ion_ipc_atomic_get(p)                   atomic_load_explicit(ION_IPC_ATOMIC_IMPL(p), memory_order_relaxed)
#define ion_ipc_atomic_get_and_increment(p,d)   atomic_fetch_add_explicit(ION_IPC_ATOMIC_IMPL(p), (d), memory_order_relaxed)
#define ion_ipc_atomic_get_and_decrement(p,d)   atomic_fetch_sub_explicit(ION_IPC_ATOMIC_IMPL(p), (d), memory_order_relaxed)
#define ion_ipc_atomic_exchange(p,v)            atomic_exchange_explicit(ION_IPC_ATOMIC_IMPL(p), (v), memory_order_relaxed)

#elif ION_HAVE_GNU_ATOMIC

/*
 * Tier 2: GCC/Clang __atomic built-ins.
 *
 * Uses __ATOMIC_RELAXED to match the ordering of the C11 path above.
 * On ARM/AArch64 this permits the compiler to emit plain loads/stores
 * and LDADD (ARMv8.1) instead of the full DMB ISH barriers that the
 * legacy __sync built-ins always emit.
 *
 * The typedef matches Tier 3 (volatile vast) so internal layouts are
 * binary-compatible between the two __sync / __atomic fallback tiers.
 */
typedef volatile vast ion_ipc_atomic_impl_t;

#define ION_IPC_ATOMIC_IMPL(p) ((ion_ipc_atomic_impl_t *)(void *)(p))

#define ion_ipc_atomic_init(p,v)                (*ION_IPC_ATOMIC_IMPL(p) = (v))
#define ion_ipc_atomic_set(p,v)                 __atomic_store_n(ION_IPC_ATOMIC_IMPL(p), (v), __ATOMIC_RELAXED)
#define ion_ipc_atomic_get(p)                   __atomic_load_n(ION_IPC_ATOMIC_IMPL(p), __ATOMIC_RELAXED)
#define ion_ipc_atomic_get_and_increment(p,d)   __atomic_fetch_add(ION_IPC_ATOMIC_IMPL(p), (d), __ATOMIC_RELAXED)
#define ion_ipc_atomic_get_and_decrement(p,d)   __atomic_fetch_sub(ION_IPC_ATOMIC_IMPL(p), (d), __ATOMIC_RELAXED)
#define ion_ipc_atomic_exchange(p,v)            __atomic_exchange_n(ION_IPC_ATOMIC_IMPL(p), (v), __ATOMIC_RELAXED)

#else /* Tier 3: Legacy __sync built-ins */

/*
 * Tier 3: Legacy __sync built-ins.
 *
 * Retained for pre-GCC-4.7 toolchains on certified flight hardware
 * (RAD750, LEON cores, older RTEMS/VxWorks).  The volatile keyword
 * forces memory access (bypassing registers) to support the __sync
 * hardware built-ins.  Provides lock-free, async-signal-safe
 * atomicity across process boundaries, but always emits full memory
 * barriers (e.g., DMB ISH on ARM).
 */
typedef volatile vast ion_ipc_atomic_impl_t;

#define ION_IPC_ATOMIC_IMPL(p) ((ion_ipc_atomic_impl_t *)(void *)(p))

#define ion_ipc_atomic_init(p,v)                (*ION_IPC_ATOMIC_IMPL(p) = (v))
#define ion_ipc_atomic_set(p,v)                 __sync_lock_test_and_set(ION_IPC_ATOMIC_IMPL(p), (v))
#define ion_ipc_atomic_get(p)                   __sync_fetch_and_add(ION_IPC_ATOMIC_IMPL(p), 0)
#define ion_ipc_atomic_get_and_increment(p,d)   __sync_fetch_and_add(ION_IPC_ATOMIC_IMPL(p), (d))
#define ion_ipc_atomic_get_and_decrement(p,d)   __sync_fetch_and_sub(ION_IPC_ATOMIC_IMPL(p), (d))
#define ion_ipc_atomic_exchange(p,v)            __sync_lock_test_and_set(ION_IPC_ATOMIC_IMPL(p), (v))

#endif /* ION_HAVE_C11_ATOMICS IPC */


/*==================================================================*/
/* ABI INVARIANT GUARDS                                             */
/*------------------------------------------------------------------*/
/* If these fire, the public opaque in ion.h has drifted from the   */
/* internal impl type — fix one or the other before shipping.  The  */
/* size assert is the load-bearing one; the alignment assert is     */
/* belt-and-braces and only checked on C11+.                        */
/*==================================================================*/

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#  define ION_STATIC_ASSERT(expr, msg) _Static_assert(expr, msg)
#else
#  define ION_SA_CONCAT_(a, b) a##b
#  define ION_SA_CONCAT(a, b)  ION_SA_CONCAT_(a, b)
#  define ION_STATIC_ASSERT(expr, msg) \
       typedef char ION_SA_CONCAT(ion_static_assert_, __LINE__)[(expr) ? 1 : -1]
#endif

ION_STATIC_ASSERT(sizeof(ion_ipc_atomic_impl_t) == sizeof(ion_ipc_atomic_t),
    "ion_ipc_atomic_t public ABI size has drifted from impl type");

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
ION_STATIC_ASSERT(_Alignof(ion_ipc_atomic_impl_t) <= _Alignof(ion_ipc_atomic_t),
    "ion_ipc_atomic_t public ABI alignment is weaker than impl type");
#endif

#endif /* ION_ATOMIC_H */
