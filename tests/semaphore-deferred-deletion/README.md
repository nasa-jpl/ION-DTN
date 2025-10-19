# Semaphore Deferred Deletion Test

This test demonstrates the reference counting and deferred deletion mechanism for POSIX Named Semaphores in ION.

**Note:** This test is only built when ION is configured to use POSIX Named Semaphores (the default on Linux, macOS, FreeBSD, and Solaris). If you configure with `--enable-force-svr4-semaphores`, this test will not be built.

## Purpose

The test validates that:
1. Semaphores are protected from premature deletion while in use
2. Deletion is deferred when refCount > 0
3. Deletion completes automatically when the last user releases the semaphore
4. Race conditions are prevented through proper synchronization

## Implementation

The reference counting mechanism works as follows:

### Data Structures

```c
typedef struct {
    char        inUse;
    char        ended;
    int         key;
    smSequence  gseq;
    int         refCount;       /* Number of active users */
    int         pendingDelete;  /* Marked for deletion when refCount reaches 0 */
} SmGlobalSem;
```

### Reference Counting Protocol

1. **sm_SemTake():**
   - Locks IPC
   - Checks if semaphore is valid and not pending deletion
   - Increments refCount
   - Unlocks IPC
   - Performs actual sem_wait()
   - On error, decrements refCount and checks for deferred deletion

2. **sm_SemGive():**
   - Performs actual sem_post()
   - Locks IPC
   - Decrements refCount
   - If pendingDelete && refCount == 0, completes deletion
   - Unlocks IPC

3. **sm_SemDelete():**
   - Locks IPC
   - If refCount > 0, sets pendingDelete flag and returns
   - If refCount == 0, immediately deletes semaphore
   - Unlocks IPC

## Test Cases

The test program includes four test cases:

1. **Normal Deletion:** Deletes a semaphore with no active users (refCount = 0)
2. **Deferred Deletion (Single User):** Takes semaphore, deletes it (deferred), then releases (completes deletion)
3. **Deferred Deletion (Multiple Users):** Multiple threads using semaphore concurrently, deletion deferred until all release
4. **Race Condition Protection:** Verifies that new Take operations are rejected on pending-delete semaphores

## Building

The test is built automatically as part of the ION build process when using POSIX Named Semaphores:

```bash
cd /path/to/ion-ios-dev
./configure                    # Uses POSIX Named Semaphores by default
make buildcheck
```

Or build just this test:

```bash
make tests/semaphore-deferred-deletion/dotest
```

### Configuration Options

ION supports different semaphore implementations:

- **Default (recommended):** POSIX Named Semaphores
  - Used on Linux, macOS, FreeBSD, Solaris by default
  - This test WILL be built

- **Force SVR4 Semaphores:**
  ```bash
  ./configure --enable-force-svr4-semaphores
  ```
  - This test WILL NOT be built (reference counting not implemented for SVR4)

- **Force POSIX Named Semaphores:**
  ```bash
  ./configure --enable-force-posix-named-semaphores
  ```
  - Explicitly request POSIX Named Semaphores
  - This test WILL be built

To check which semaphore implementation will be used:

```bash
./configure --help | grep -i semaphore
```

## Running

Run the test directly:

```bash
./tests/semaphore-deferred-deletion/dotest
```

Or through the ION test framework:

```bash
cd tests
./runtests semaphore-deferred-deletion
```

## Cleanup

If the test fails and leaves IPC resources:

```bash
cd tests/semaphore-deferred-deletion
./cleanup
```

## Expected Output

```
========================================
  ION Semaphore Deferred Deletion Test
========================================

    [INFO] Initializing IPC system...
    [INFO] IPC system initialized

==> Test: Normal Deletion (no active users)
    [INFO] Creating semaphore...
    [INFO] Created semaphore 0
    [INFO] Deleting semaphore (should succeed immediately)...
    [INFO] Deletion completed
    [INFO] Attempting to take deleted semaphore (should fail)...
    [INFO] Correctly rejected take on deleted semaphore
    [PASS] Normal deletion with immediate cleanup

==> Test: Deferred Deletion (single user)
    [INFO] Creating semaphore...
    [INFO] Created semaphore 0
    [INFO] Taking semaphore (refCount should become 1)...
    [INFO] Successfully took semaphore
    [INFO] Attempting delete while in use (should be deferred)...
    [INFO] Delete returned (deletion should be pending)
    [INFO] Releasing semaphore (should complete deferred deletion)...
    [INFO] Released semaphore, deletion should now be complete
    [INFO] Attempting to take semaphore after deferred deletion...
    [INFO] Correctly rejected take after deferred deletion
    [PASS] Deferred deletion completed successfully

... (additional tests)

========================================
  Test Summary
========================================
  Total:  4
  Passed: 4
  Failed: 0
========================================

✓ All tests passed!
```

## Technical Details

### Files Modified

- `ici/library/platform_sm.c`: Implementation of reference counting
  - Modified `SmGlobalSem` and `SmLocalSem` structures
  - Modified `sm_SemTake()`, `sm_SemGive()`, `sm_SemDelete()`
  - Added `_sm_SemCompleteDeletePosix()` helper function

### Key Features

- **Atomic Operations:** All refCount modifications protected by IPC lock
- **Deferred Deletion:** Deletion marked pending, completed in last sm_SemGive()
- **Race Prevention:** New takes rejected on pending-delete semaphores
- **Thread Safety:** Works correctly with multiple concurrent threads/processes

## Limitations

- Only implemented for POSIX Named Semaphores (not SVR4 System V semaphores)
- Requires platforms that support POSIX named semaphores
- Small performance overhead due to additional locking

## See Also

- ION Security Database Reference Counting (similar pattern)
- `ici/include/platform.h` - Semaphore API definitions
- `ici/library/platform_sm.c` - Semaphore implementation
