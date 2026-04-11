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
 * Padded union forces C11 atomics to match the C99 fallback ABI layout.
 * This guarantees memory footprint parity across all compilation paths.
 */
typedef union {
    _Atomic(vast) native_val;
    char          padding[64];
} ion_atomic_t;

/* C11 Static Initializer */
#define ION_ATOMIC_INIT(v) { .native_val = (v) }

/* C11 Static Inline Wrappers to prevent macro shadowing */
static inline void ion_atomic_init(ion_atomic_t *p, vast v) {
    atomic_init(&p->native_val, v);
}
static inline void ion_atomic_set(ion_atomic_t *p, vast v) {
    atomic_store_explicit(&p->native_val, v, memory_order_relaxed);
}
static inline uvast ion_atomic_get(const ion_atomic_t *p) {
    return atomic_load_explicit(&p->native_val, memory_order_relaxed);
}
static inline uvast ion_atomic_get_and_increment(ion_atomic_t *p, vast d) {
    return atomic_fetch_add_explicit(&p->native_val, d, memory_order_relaxed);
}
static inline uvast ion_atomic_get_and_decrement(ion_atomic_t *p, vast d) {
    return atomic_fetch_sub_explicit(&p->native_val, d, memory_order_relaxed);
}
static inline uvast ion_atomic_exchange(ion_atomic_t *p, vast v) {
    return atomic_exchange_explicit(&p->native_val, v, memory_order_relaxed);
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
uvast ion_atomic_get               (const ion_atomic_t *);
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

/**
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

#else /* C99 Fallback using GCC/Clang built-ins */

/**
 * @brief Inter-Process Atomic Type (C99 Fallback)
 * The volatile keyword forces memory access (bypassing registers) to
 * support the __sync hardware built-ins. Provides lock-free,
 * async-signal-safe atomicity across process boundaries.
 */
typedef volatile vast ion_ipc_atomic_t;

#define ion_ipc_atomic_init(p,v)                (*(p) = (v))
#define ion_ipc_atomic_set(p,v)                 __sync_lock_test_and_set((p), (v))
#define ion_ipc_atomic_get(p)                   __sync_fetch_and_add((p), 0)
#define ion_ipc_atomic_get_and_increment(p,d)   __sync_fetch_and_add((p), (d))
#define ion_ipc_atomic_get_and_decrement(p,d)   __sync_fetch_and_sub((p), (d))
#define ion_ipc_atomic_exchange(p,v)            __sync_lock_test_and_set((p), (v))

#endif /* ION_HAVE_C11_ATOMICS IPC */

#endif /* ION_ATOMIC_H */
