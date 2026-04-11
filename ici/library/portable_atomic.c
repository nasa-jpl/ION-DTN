/*--------------------------------------------------------------------
 * portable_atomic.c    ──  POSIX/C99 fallback implementation
 *-------------------------------------------------------------------*/
#include "portable_atomic.h"

#if !ION_HAVE_C11_ATOMICS       /* Compile only on fallback */

void ion_atomic_init(ion_atomic_t *a, vast v)
{
    pthread_mutex_init(&a->fallback.lock, NULL);
    a->fallback.value = v;
}

void ion_atomic_mutex_destroy(ion_atomic_t *a)
{
    pthread_mutex_destroy(&a->fallback.lock);
}

void ion_atomic_set(ion_atomic_t *a, vast v)
{
    pthread_mutex_lock(&a->fallback.lock);
    a->fallback.value = v;
    pthread_mutex_unlock(&a->fallback.lock);
}

uvast ion_atomic_get(const ion_atomic_t *a)
{
    uvast v;

    /* Cast drops const specifically for the mutex operations */
    pthread_mutex_lock((pthread_mutex_t *)&a->fallback.lock);
    v = a->fallback.value;
    pthread_mutex_unlock((pthread_mutex_t *)&a->fallback.lock);

    return v;
}

uvast ion_atomic_get_and_increment(ion_atomic_t *a, vast delta)
{
    uvast old;

    pthread_mutex_lock(&a->fallback.lock);
    old = a->fallback.value;
    a->fallback.value += delta;
    pthread_mutex_unlock(&a->fallback.lock);

    return old;
}

uvast ion_atomic_get_and_decrement(ion_atomic_t *a, vast delta)
{
    uvast old;

    pthread_mutex_lock(&a->fallback.lock);
    old = a->fallback.value;
    a->fallback.value -= delta;
    pthread_mutex_unlock(&a->fallback.lock);

    return old;
}

uvast ion_atomic_exchange(ion_atomic_t *a, vast v)
{
    uvast old;

    pthread_mutex_lock(&a->fallback.lock);
    old = a->fallback.value;
    a->fallback.value = v;
    pthread_mutex_unlock(&a->fallback.lock);

    return old;
}

#endif  /* !ION_HAVE_C11_ATOMICS */
