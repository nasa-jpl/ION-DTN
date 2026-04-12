/*
 * ion_atomic.c
 *
 * POSIX/C99 Fallback Implementation for Process-Local Atomic Operations.
 *
 * This module provides a thread-safe, mutex-backed fallback for atomic
 * operations on UNIX-like environments where C11 <stdatomic.h> is natively
 * unavailable. It ensures that all read, write, and read-modify-write
 * operations on 'ion_atomic_t' variables are strictly serialized within
 * the local process address space.
 *
 * Note: This implementation is strictly for process-local memory.
 * Shared memory (IPC) atomics rely on the lock-free __sync hardware
 * built-ins defined in the header to ensure async-signal safety.
 */

#include "ion_atomic.h"

#if !ION_HAVE_C11_ATOMICS       /* Compile only on fallback */

/**
 * @brief Initializes an atomic variable and its backing mutex.
 * * This must be called before any other operations are performed on the
 * atomic variable. It initializes the POSIX mutex with default attributes.
 *
 * @param a Pointer to the atomic variable to initialize.
 * @param v The initial value to assign to the variable.
 */
void ion_atomic_init(ion_atomic_t *a, vast v)
{
    pthread_mutex_init(&a->fallback.lock, NULL);
    a->fallback.value = v;
}

/**
 * @brief Destroys the backing mutex of an atomic variable.
 * * Frees the resources associated with the POSIX mutex. This should be
 * called during subsystem teardown when the atomic variable is no longer needed.
 *
 * @param a Pointer to the atomic variable to destroy.
 */
void ion_atomic_mutex_destroy(ion_atomic_t *a)
{
    pthread_mutex_destroy(&a->fallback.lock);
}

/**
 * @brief Thread-safe setter for the atomic variable.
 * * Acquires the lock, updates the internal value, and releases the lock.
 *
 * @param a Pointer to the atomic variable.
 * @param v The new value to assign.
 */
void ion_atomic_set(ion_atomic_t *a, vast v)
{
    pthread_mutex_lock(&a->fallback.lock);
    a->fallback.value = v;
    pthread_mutex_unlock(&a->fallback.lock);
}

/**
 * @brief Thread-safe getter for the atomic variable.
 * * Acquires the lock, reads the internal value, and releases the lock.
 * * @note The 'const' qualifier is cast away locally because locking and
 * unlocking the POSIX mutex physically mutates the lock's memory state,
 * even though the logical 'value' of the atomic variable remains unmodified.
 *
 * @param a Pointer to the constant atomic variable.
 * @return The current value of the atomic variable.
 */
uvast ion_atomic_get(ion_atomic_t *a)
{
    uvast v;

    pthread_mutex_lock((pthread_mutex_t *)&a->fallback.lock);
    v = a->fallback.value;
    pthread_mutex_unlock((pthread_mutex_t *)&a->fallback.lock);

    return v;
}

/**
 * @brief Thread-safe fetch-and-add operation.
 * * Atomically adds the specified delta to the variable and returns the
 * value that existed *before* the addition.
 *
 * @param a Pointer to the atomic variable.
 * @param delta The value to add.
 * @return The previous value of the atomic variable.
 */
uvast ion_atomic_get_and_increment(ion_atomic_t *a, vast delta)
{
    uvast old;

    pthread_mutex_lock(&a->fallback.lock);
    old = a->fallback.value;
    a->fallback.value += delta;
    pthread_mutex_unlock(&a->fallback.lock);

    return old;
}

/**
 * @brief Thread-safe fetch-and-subtract operation.
 * * Atomically subtracts the specified delta from the variable and returns
 * the value that existed *before* the subtraction.
 *
 * @param a Pointer to the atomic variable.
 * @param delta The value to subtract.
 * @return The previous value of the atomic variable.
 */
uvast ion_atomic_get_and_decrement(ion_atomic_t *a, vast delta)
{
    uvast old;

    pthread_mutex_lock(&a->fallback.lock);
    old = a->fallback.value;
    a->fallback.value -= delta;
    pthread_mutex_unlock(&a->fallback.lock);

    return old;
}

/**
 * @brief Thread-safe value exchange (swap) operation.
 * * Atomically replaces the current value of the variable with a new value
 * and returns the value that existed *before* the exchange.
 *
 * @param a Pointer to the atomic variable.
 * @param v The new value to assign.
 * @return The previous value of the atomic variable.
 */
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
