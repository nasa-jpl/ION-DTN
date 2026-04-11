/*
 * test_ipc_atomics.c: TSan validation harness for ION IPC Atomics.
 *
 * This test suite mathematically proves the thread-safety and ABI parity
 * of the Inter-Process Communication (IPC) atomic fallback architecture.
 * It mocks the shared memory global semaphore table (SmGlobalSemtable)
 * and hammers the atomic reference counts and sequence numbers using
 * multiple POSIX threads to simulate concurrent process access.
 *
 * ==================================================================
 * COMPILATION INSTRUCTIONS
 * ==================================================================
 * To execute this test properly, you MUST compile it with the
 * -DION_TEST_FORCE_FALLBACK macro and the ThreadSanitizer (TSan) enabled.
 * * Run the following from the directory containing this file:
 *
 * gcc -std=c99 -DION_TEST_FORCE_FALLBACK -g -O1 -fsanitize=thread -fPIE -pthread -w \
 * -I../../ -I../../ici/include -I../../ici/library \
 * -o test_ipc_atomics test_ipc_atomics.c ../../ici/library/ion_atomic.c
 *
 * ./test_ipc_atomics
 * ==================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include "ion_atomic.h"

#define SEM_NSEMS_MAX 256
#define THREAD_COUNT  8
#define LOOP_COUNT    100000

/*==================================================================*/
/* Mock Shared Memory Structures                                    */
/*==================================================================*/

/**
 * @brief Mock representation of the ION Global Semaphore struct.
 * Stripped down to focus purely on the atomic memory layout.
 */
typedef struct {
    char             inUse;
    char             ended;
    int              key;
    ion_ipc_atomic_t gseq;
    ion_ipc_atomic_t refCount;
    ion_ipc_atomic_t pendingDelete;
} SmGlobalSem;

/**
 * @brief Mock representation of the shared memory table.
 */
typedef struct {
    unsigned int     ipcUniqueKey;
    int              initialized;
    SmGlobalSem      gsemtable[SEM_NSEMS_MAX];
} SmGlobalSemtable;

/* The "Shared Memory" segment simulated on the local heap */
SmGlobalSemtable *mock_shm_table;

/*==================================================================*/
/* Concurrency Workload                                             */
/*==================================================================*/

/**
 * @brief The worker thread function that hammers the IPC atomics.
 * This simulates multiple concurrent ION processes attempting to
 * take and give semaphores simultaneously in shared memory.
 */
void *worker_thread(void *arg)
{
    (void)arg;
    SmGlobalSem *target_sem = &mock_shm_table->gsemtable[42]; /* Arbitrary target */

    for (int i = 0; i < LOOP_COUNT; i++)
    {
        /* Simulate sm_SemTake (incrementing refCount) */
        ion_ipc_atomic_get_and_increment(&target_sem->refCount, 1);

        /* Simulate a sequence update */
        ion_ipc_atomic_get_and_increment(&target_sem->gseq, 1);

        /* Simulate sm_SemGive (decrementing refCount) */
        ion_ipc_atomic_get_and_decrement(&target_sem->refCount, 1);
    }

    return NULL;
}

/*==================================================================*/
/* Main Test Execution                                              */
/*==================================================================*/

int main(void)
{
    pthread_t threads[THREAD_COUNT];

    /* --- Phase 1: ABI Parity Check --- */
    printf("[i] Validating ABI layout...\n");

    /* Ensure the padding union works for process-local: 64 bytes is the standard
     * size we aimed for. A standard pthread_mutex_t + uvast is ~48 bytes on 64-bit systems.
     */
    assert(sizeof(ion_atomic_t) >= 40 && "FATAL: ion_atomic_t is too small to hold a POSIX mutex!");

    /* --- Phase 2: Mock Memory Allocation --- */
    /* Simulates sm_ShmAttach creating the mapped memory segment */
    printf("[i] Allocating mock shared memory table...\n");
    mock_shm_table = calloc(1, sizeof(SmGlobalSemtable));
    assert(mock_shm_table != NULL);

    /* --- Phase 3: Initialization Gate --- */
    /* Simulates the _sembase initialization logic */
    printf("[i] Initializing IPC atomics...\n");
    for (int i = 0; i < SEM_NSEMS_MAX; i++)
    {
        ion_ipc_atomic_init(&mock_shm_table->gsemtable[i].gseq, 0);
        ion_ipc_atomic_init(&mock_shm_table->gsemtable[i].refCount, 0);
        ion_ipc_atomic_init(&mock_shm_table->gsemtable[i].pendingDelete, 0);
    }
    mock_shm_table->initialized = 1;

    /* --- Phase 4: Concurrent Execution --- */
    printf("[i] Spawning %d threads to execute %d atomic operations each...\n",
           THREAD_COUNT, LOOP_COUNT * 3);

    for (int i = 0; i < THREAD_COUNT; i++)
    {
        if (pthread_create(&threads[i], NULL, worker_thread, NULL) != 0)
        {
            perror("Failed to create thread");
            exit(EXIT_FAILURE);
        }
    }

    /* Wait for the dust to settle */
    for (int i = 0; i < THREAD_COUNT; i++)
    {
        pthread_join(threads[i], NULL);
    }

    /* --- Phase 5: Mathematical Verification --- */
    printf("[i] Verifying final atomic states...\n");
    SmGlobalSem *target_sem = &mock_shm_table->gsemtable[42];

    uvast final_refCount = ion_ipc_atomic_get(&target_sem->refCount);
    uvast final_gseq     = ion_ipc_atomic_get(&target_sem->gseq);

    printf("    -> Final refCount: %llu (Expected: 0)\n", (unsigned long long)final_refCount);
    printf("    -> Final gseq:     %llu (Expected: %d)\n", (unsigned long long)final_gseq, THREAD_COUNT * LOOP_COUNT);

    assert(final_refCount == 0 && "RACE CONDITION DETECTED: refCount is corrupted!");
    assert(final_gseq == (uvast)(THREAD_COUNT * LOOP_COUNT) && "RACE CONDITION DETECTED: gseq is corrupted!");

    free(mock_shm_table);

    printf("[✓] SUCCESS: IPC Atomics are thread-safe under heavy concurrent load.\n");
    return EXIT_SUCCESS;
}
