/*==================================================================*/
/* portable_atomic.h                                                */
/*==================================================================*/
#ifndef ION_PORTABLE_ATOMIC_H
#define ION_PORTABLE_ATOMIC_H

#include "platform.h"
#include "config.h"

/* Test Harness Override */
#if defined(ION_TEST_FORCE_FALLBACK)
# undef ION_HAVE_C11_ATOMICS
# define ION_HAVE_C11_ATOMICS 0
#endif

#if !defined(ION_HAVE_C11_ATOMICS)
# if __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_ATOMICS__)
#  define ION_HAVE_C11_ATOMICS 1
# else
#  define ION_HAVE_C11_ATOMICS 0
# endif
#endif

/*==================================================================*/
/* Phase 1 & 2: Process-Local Atomics                               */
/*==================================================================*/
#if ION_HAVE_C11_ATOMICS

#include <stdatomic.h>

/* Padded union forces C11 atomics to match C99 fallback ABI layout */
typedef union {
    _Atomic(vast) native_val;
    char          padding[64];
} ion_atomic_t;

/* C11 Static Initializer */
#define ION_ATOMIC_INIT(v) { .native_val = (v) }

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

#define ion_atomic_mutex_destroy(p)         ((void)0)

#else /* POSIX / C99 mutex fallback */

#include <pthread.h>

typedef union {
    struct {
        pthread_mutex_t lock;
        uvast           value;
    } fallback;
    char padding[64];
} ion_atomic_t;

/* C99 Static Initializer */
#define ION_ATOMIC_INIT(v) { .fallback = { PTHREAD_MUTEX_INITIALIZER, (v) } }

void  ion_atomic_init              (ion_atomic_t *, vast);
void  ion_atomic_mutex_destroy     (ion_atomic_t *);
void  ion_atomic_set               (ion_atomic_t *, vast);
uvast ion_atomic_get               (const ion_atomic_t *);
uvast ion_atomic_get_and_increment (ion_atomic_t *, vast);
uvast ion_atomic_get_and_decrement (ion_atomic_t *, vast);
uvast ion_atomic_exchange          (ion_atomic_t *, vast);

#endif /* ION_HAVE_C11_ATOMICS */

/*==================================================================*/
/* Phase 3: Inter-Process (IPC) Atomics (Async-Signal-Safe)         */
/*==================================================================*/
#if ION_HAVE_C11_ATOMICS

typedef _Atomic(vast) ion_ipc_atomic_t;

#define ion_ipc_atomic_init(p,v)                atomic_init((p), (v))
#define ion_ipc_atomic_set(p,v)                 atomic_store_explicit((p), (v), memory_order_relaxed)
#define ion_ipc_atomic_get(p)                   atomic_load_explicit((p), memory_order_relaxed)
#define ion_ipc_atomic_get_and_increment(p,d)   atomic_fetch_add_explicit((p), (d), memory_order_relaxed)
#define ion_ipc_atomic_get_and_decrement(p,d)   atomic_fetch_sub_explicit((p), (d), memory_order_relaxed)
#define ion_atomic_exchange(p,v)                atomic_exchange_explicit(&(p)->native_val, (v), memory_order_relaxed)

#else /* C99 Fallback using GCC/Clang built-ins */

/* volatile ensures the compiler does not optimize away memory accesses */
typedef volatile vast ion_ipc_atomic_t;

/* Hardware built-ins provide lock-free, async-signal-safe atomicity */
#define ion_ipc_atomic_init(p,v)                (*(p) = (v))
#define ion_ipc_atomic_set(p,v)                 __sync_lock_test_and_set((p), (v))
#define ion_ipc_atomic_get(p)                   __sync_fetch_and_add((p), 0)
#define ion_ipc_atomic_get_and_increment(p,d)   __sync_fetch_and_add((p), (d))
#define ion_ipc_atomic_get_and_decrement(p,d)   __sync_fetch_and_sub((p), (d))


#endif /* ION_HAVE_C11_ATOMICS IPC */

#endif /* ION_PORTABLE_ATOMIC_H */
