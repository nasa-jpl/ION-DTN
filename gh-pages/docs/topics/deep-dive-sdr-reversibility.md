# SDR Transaction Reversibility and Crash Recovery

This document provides a deep dive into ION's SDR (Simple Data Recorder) transaction reversibility mechanism, including how transactions work, how crash recovery is triggered, and how daemons coordinate during recovery.

## Overview

ION's SDR provides ACID-like transaction semantics for its shared memory dataspace. The reversibility mechanism ensures that if a daemon crashes mid-transaction, the dataspace can be restored to a consistent state. This is achieved through:

1. **Write-Ahead Logging (WAL)** - Original data is logged before any modification
2. **Transaction Ownership Tracking** - Only one task can own a transaction at a time
3. **Automatic Reversal** - Incomplete transactions are automatically rolled back
4. **Coordinated Recovery** - `ionrestart` orchestrates daemon restart after crashes

## Transaction Lifecycle

### Starting a Transaction

```c
CHKERR(sdr_begin_xn(sdr));
```

`sdr_begin_xn()` performs:
- Acquires the SDR semaphore to serialize access
- Records the owner task ID and thread ID in `SdrState`
- Initializes `xnDepth` to 1 (supports nested transactions)
- Clears `xnCanceled` and `modified` flags

### Committing a Transaction

```c
if (sdr_end_xn(sdr) < 0) {
    putErrmsg("Transaction failed.", NULL);
}
```

`sdr_end_xn()` performs:
- Decrements `xnDepth`
- When `xnDepth` reaches 0, calls `terminateXn()` to finalize
- If successful, truncates the transaction log to zero length
- Releases the SDR semaphore

### Canceling a Transaction

```c
sdr_cancel_xn(sdr);
```

`sdr_cancel_xn()` performs:
- Sets `xnCanceled` flag to 1
- Decrements `xnDepth`
- Triggers `terminateXn()` when `xnDepth` reaches 0
- `terminateXn()` calls `reverseTransaction()` to undo all modifications

### Exiting a Transaction (Defensive)

```c
sdr_exit_xn(sdr);
```

`sdr_exit_xn()` is used when unwinding from errors:
- If no modifications were made: safely releases the transaction
- If modifications were made (`modified=1`): automatically triggers cancellation
- Handles cases where call stack is unwinding after a crash

## Write-Ahead Logging

The transaction log is the core mechanism enabling reversibility. Every write to the SDR dataspace is preceded by logging the original data.

### Log Entry Format

Each log entry contains:

| Offset | Size | Content |
|--------|------|---------|
| 0 | W bytes | Start address within SDR heap |
| W | W bytes | Length (L) of data being written |
| 2W | L bytes | Original data at that address |

Where W = `WORD_SIZE` (4 on 32-bit, 8 on 64-bit systems)

### Log Storage Options

**File-based logging** (`logSize == 0`):
- Non-volatile, survives power cycles and crashes
- Location: `<pathName>/<sdrname>.sdrlog`
- Required for crash recovery across restarts

**Memory-based logging** (`logSize > 0`):
- Volatile, lost on process termination
- Faster, suitable for SDRs that don't require crash recovery
- Allocated in shared memory

### Write Operation Flow

When `sdr_write()` is called:

1. **Ownership Validation**: Verify current task still owns the transaction
2. **Log Control Write**: Write address and length to log
3. **Data Capture**: Write original data from target address to log
4. **Entry Registration**: Insert log offset into `logEntries` list
5. **Actual Write**: Write new data to target address in dataspace

```
┌─────────────────────────────────────────────────────────────┐
│                    sdr_write() Flow                         │
├─────────────────────────────────────────────────────────────┤
│  1. Check sdrOwnerTask == current task                      │
│     └─ If not, silently return (defensive)                  │
│                                                             │
│  2. Write to transaction log:                               │
│     ┌────────────┬────────────┬─────────────────┐          │
│     │  Address   │   Length   │  Original Data  │          │
│     │  (W bytes) │  (W bytes) │   (L bytes)     │          │
│     └────────────┴────────────┴─────────────────┘          │
│                                                             │
│  3. Add log entry offset to logEntries list                 │
│                                                             │
│  4. Write new data to SDR heap address                      │
└─────────────────────────────────────────────────────────────┘
```

## Transaction Reversal

When a transaction is canceled (explicitly or due to crash), `reverseTransaction()` restores the dataspace to its pre-transaction state.

### Reversal Algorithm

1. **Iterate in Reverse Order**: Process log entries LIFO (last-in, first-out)
2. **Read Log Entry**: Parse address, length, and original data from log
3. **Restore Data**: Write original data back to the heap address
4. **Dual-Site Consistency**: If using file-backed SDR, write to both shared memory and file

```c
for (elt = sm_list_last(logEntries); elt; elt = sm_list_prev(elt)) {
    // Read log entry: address, length, original_data
    // Write original_data back to heap[address]
    // If file-backed, also write to dsfile
}
```

The reverse order is critical for correctness: if the same address was written multiple times, the earliest value must be restored last.

## Transaction Ownership and Hijacking

### Owner Tracking

The `SdrState` structure tracks transaction ownership:

```c
typedef struct {
    sm_SemId  sdrSemaphore;     // Mutex for serialization
    int       sdrOwnerTask;     // Task ID holding transaction
    pthread_t sdrOwnerThread;   // Thread ID for nested calls
    int       xnDepth;          // Nesting level
    int       xnCanceled;       // Flag: marked for reversal
    int       modified;         // Flag: dataspace modified
    int       halted;           // Flag: daemons initializing
    char      restartCmd[32];   // Command to invoke ionrestart
} SdrState;
```

### Ownership Validation

Every SDR operation validates ownership:

```c
int sdr_in_xn(Sdr sdrv) {
    return (sdrv->sdr->sdrOwnerTask == sm_TaskIdSelf()
        && pthread_equal(sdrv->sdr->sdrOwnerThread, pthread_self()));
}
```

### Defensive Owner Check During Writes

A critical safety feature prevents stale writes after ownership transfer:

```c
// In _sdrput():
if (sdr->sdrOwnerTask != sm_TaskIdSelf()) {
    // Task no longer owns transaction
    // Silently return without corrupting new owner's data
    return;
}
```

This handles the scenario where:
1. Daemon A crashes mid-transaction
2. `ionrestart` hijacks the transaction
3. Daemon A's call stack continues unwinding, attempting more writes
4. These writes are silently ignored because ownership transferred

## Crash Detection and Recovery

### Automatic Recovery at Startup

When `sdr_load_profile()` is called (typically during `ionAttach()`):

1. **Open Log File**: Check if log file exists and has content
2. **Reload Log Entries**: Reconstruct `logEntries` list from log file
3. **Detect Incomplete Transaction**: Non-empty log indicates crash
4. **Automatic Reversal**: Call `reverseTransaction()` to restore consistency
5. **Truncate Log**: Clear log after successful reversal

```
┌─────────────────────────────────────────────────────────────┐
│              sdr_load_profile() Recovery                    │
├─────────────────────────────────────────────────────────────┤
│  1. Open log file                                           │
│     └─ If empty: no recovery needed                         │
│                                                             │
│  2. If log has content:                                     │
│     ├─ reloadLogEntries() - scan log, rebuild list          │
│     ├─ reverseTransaction() - restore original data         │
│     └─ Truncate log file                                    │
│                                                             │
│  3. Load dataspace from file into shared memory             │
└─────────────────────────────────────────────────────────────┘
```

### Lock Implementation: Robust Mutex vs. Legacy Semaphore

The lock that serializes SDR transactions — and the broader ION global IPC lock
and PSM (shared-memory partition) lock — have two interchangeable
implementations, selected at **compile time** by the `ION_HAVE_ROBUST_MUTEX`
macro.

| | Robust mutex (`ION_HAVE_ROBUST_MUTEX` defined) | Legacy semaphore (undefined) |
|---|---|---|
| Primitive | Process-shared `pthread_mutex` with `PTHREAD_MUTEX_ROBUST` | System V / POSIX semaphore (`sm_SemTake`, `sem_wait`) |
| Holder dies mid-critical-section | The next locker's `pthread_mutex_lock` returns `EOWNERDEAD`; ION rolls back the orphaned work and marks the mutex consistent | No notification on acquisition; recovery relies on `sm_SemUnwedge` heuristics and the startup log-reversal path |
| SDR transaction lock | Orphaned transaction reversed immediately via `recoverOrphanedXn()` | Reversed at the next `sdr_load_profile()` from the write-ahead log |
| Global IPC lock (`takeIpcLock`) | `EOWNERDEAD` recovery of the shared semaphore table | `sem_wait` with no dead-owner recovery |

Robust mutexes let a surviving process recover *immediately* when a daemon is
killed while holding a lock, instead of waiting for the next node restart.

!!! warning "Robust mutexes require an autotools build"
    `ION_HAVE_ROBUST_MUTEX` is defined **only** in `config.h`, which is
    generated by `./configure` and pulled in only behind `#ifdef HAVE_CONFIG_H`.
    A standard autotools build (`./configure && make`) enables it by default
    (subject to the gates below). The **development Makefile does not define it**
    — it compiles without `config.h` — so dev-Makefile builds use the legacy
    semaphore path, and the robust-recovery regression tests
    (`robust_lock_recovery_test`, `ipc_lock_recovery_test`,
    `psm_robust_recovery_test`) **skip**. Build and install via autotools to
    exercise robust-mutex crash recovery.

Even under autotools, `configure` enables robust mutexes only when all of the
following hold; otherwise it falls back to the legacy semaphore implementation,
which remains fully functional and simply defers orphaned-lock recovery to node
restart:

- the toolchain supports `pthread_mutexattr_setrobust(PTHREAD_MUTEX_ROBUST)`
  (verified by a configure-time compile test),
- the target platform is on the supported list (for example, RTEMS is
  excluded), and
- `--disable-robust-sdr-lock` was not passed.

### ionrestart Recovery Orchestration

When a daemon crash is detected, `ionrestart` coordinates full system recovery:

#### Phase 1: Transaction Hijacking

```c
// Hijack transaction ownership from crashed task
sdrv->sdr->sdrOwnerTask = sm_TaskIdSelf();
sdrv->sdr->sdrOwnerThread = pthread_self();

// Block new transactions from crashed task
sdrSemaphore = sdrv->sdr->sdrSemaphore;
sdrv->sdr->sdrSemaphore = -1;

// Wait for crashed task to terminate
snooze(RESTART_GRACE_PERIOD);

// Re-enable transactions
sdrv->sdr->sdrSemaphore = sdrSemaphore;
```

#### Phase 2: Stop All Daemons

```c
// Stop protocol daemons in order
cfdp_stop();
bp_stop();
ltp_stop();
rfx_stop();

// Wait for daemons to terminate, force SIGKILL if needed
```

#### Phase 3: Drop and Rebuild Volatile Databases

```c
// Terminate SDR semaphore to interrupt waiting tasks
sm_SemEnd(sdr->sdrSemaphore);

// Drop corrupted volatile databases
cfdpDropVdb();
bpDropVdb();
ltpDropVdb();
ionDropVdb();

// Restore semaphore
sm_SemUnend();
sm_SemGive();

// Reconstruct volatile databases from persistent SDR
ionRaiseVdb();
ltpRaiseVdb();
bpRaiseVdb();
cfdpRaiseVdb();
```

#### Phase 4: Restart Daemons

```c
// Set halted flag to allow safe daemon initialization
sdrv->sdr->halted = 1;

// Start daemons in order
rfx_start();
ltpStart();
bpStart();
cfdpStart();

// Clear halted flag when all daemons initialized
sdrv->sdr->halted = 0;
```

## Daemon Coordination During Recovery

### The Halted Flag

The `halted` flag in `SdrState` coordinates daemon startup:

- **Set by ionrestart** before starting daemons
- **Cleared by ionrestart** after all daemons are initialized
- **Checked by sdrFetchSafe()** to allow safe SDR access during startup

### sdrFetchSafe() Check

Read-only SDR functions use `sdrFetchSafe()` for safe access:

```c
int sdrFetchSafe(Sdr sdrv) {
    return (sdr_in_xn(sdrv) || sdr_heap_is_halted(sdrv));
}
```

Two conditions allow safe reads:
1. **In Transaction**: Current task owns the transaction
2. **Heap Halted**: Daemons are starting up under ionrestart control

### Defensive SDR List Functions

The SDR list functions (`sdr_list_first`, `sdr_list_next`, etc.) check `sdrFetchSafe()` defensively:

```c
SdrObject sdr_list_first(Sdr sdrv, SdrObject list) {
    if (!sdrFetchSafe(sdrv)) {
        writeMemoNote("[?] sdr_list_first called but SDR not accessible",
                      itoa(list));
        return 0;  // Defensive: SDR not accessible
    }
    // ... normal operation
}
```

This prevents cascading crashes during recovery when newly restarted daemons attempt SDR operations before fully initialized.

## Crash Recovery Flow

### Complete Recovery Scenario

```
┌─────────────────────────────────────────────────────────────┐
│           Daemon Crash Recovery Timeline                    │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  T0: Daemon A holds transaction, executes sdr_write()       │
│      └─ Log entries accumulate                              │
│                                                             │
│  T1: Daemon A crashes (SIGSEGV, assertion, etc.)            │
│      └─ Transaction incomplete, log file has entries        │
│                                                             │
│  T2: Crash detected, ionrestart invoked                     │
│      └─ ionAttach() → sdr_load_profile()                    │
│      └─ reloadLogEntries() reconstructs log entry list      │
│      └─ reverseTransaction() restores original data         │
│                                                             │
│  T3: ionrestart hijacks transaction                         │
│      └─ sdrOwnerTask = ionrestart's task ID                 │
│      └─ sdrSemaphore = -1 (block crashed task)              │
│                                                             │
│  T4: Crashed daemon's call stack unwinds                    │
│      └─ Stale sdr_write() calls check ownership             │
│      └─ Owner mismatch → silently return (no corruption)    │
│                                                             │
│  T5: Grace period expires                                   │
│      └─ Crashed daemon fully terminated                     │
│      └─ Semaphore re-enabled                                │
│                                                             │
│  T6: Volatile databases dropped and rebuilt                 │
│      └─ Corrupted in-memory state discarded                 │
│      └─ Clean state reconstructed from persistent SDR       │
│                                                             │
│  T7: Daemons restarted with halted=1                        │
│      └─ sdrFetchSafe() returns true for all daemons         │
│      └─ Safe initialization without transaction             │
│                                                             │
│  T8: halted=0, normal operation resumes                     │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

## Restart Loop Prevention

To prevent infinite restart loops when crashes recur rapidly:

```c
time_t prevRestartTime = sdrv->sdr->restartTime;
sdrv->sdr->restartTime = getCtime();

if ((sdrv->sdr->restartTime - prevRestartTime) < RESTART_LOOP_INTERVAL) {
    writeMemo("Inferred restart loop. Tasks not restarted.");
    return;  // Skip daemon restart
}
```

If `ionrestart` is invoked too frequently (within `RESTART_LOOP_INTERVAL`), daemon restart is skipped to prevent thrashing.

## Configuration

### Enabling Reversibility

SDR reversibility is enabled via the `configFlags` in the `.ionconfig` file:

```
configFlags 13
```

The flags are a bitmask:
- Bit 0 (1): `SDR_IN_DRAM` - Use shared memory for dataspace
- Bit 2 (4): `SDR_IN_FILE` - Use file for persistent storage
- Bit 3 (8): `SDR_REVERSIBLE` - Enable transaction logging

`configFlags 13` = 1 + 4 + 8 = DRAM + FILE + REVERSIBLE

### Log Size Configuration

```
logSize 5000000
```

- `logSize 0`: Use file-based logging (persistent)
- `logSize > 0`: Use memory-based logging with specified size (volatile)

For crash recovery across system restarts, use file-based logging (`logSize 0`) or ensure sufficient log size for transaction workload.

## Safety Features Summary

| Feature | Purpose |
|---------|---------|
| Write-Ahead Logging | Log original data before modification |
| LIFO Reversal | Correct order for multi-write transactions |
| Ownership Tracking | Prevent concurrent transaction conflicts |
| Defensive Owner Check | Ignore stale writes after ownership transfer |
| Dual-Site Consistency | Keep shared memory and file synchronized |
| Halted Flag | Safe daemon initialization during restart |
| sdrFetchSafe() | Allow reads during halted state |
| Restart Loop Detection | Prevent infinite crash/restart cycles |
| Grace Period | Allow crashed task to fully terminate |

## Key Functions Reference

| Function | File | Purpose |
|----------|------|---------|
| `sdr_begin_xn` | sdrxn.c | Start transaction, acquire lock |
| `sdr_end_xn` | sdrxn.c | Commit transaction, release lock |
| `sdr_cancel_xn` | sdrxn.c | Rollback transaction |
| `sdr_exit_xn` | sdrxn.c | Safely exit (with auto-cancel if modified) |
| `sdr_in_xn` | sdrxn.c | Check if current task owns transaction |
| `sdr_heap_is_halted` | sdrxn.c | Check if heap is in halted state |
| `sdrFetchSafe` | sdrxn.c | Check if safe to read (in-xn OR halted) |
| `terminateXn` | sdrxn.c | Finalize transaction (commit or reverse) |
| `reverseTransaction` | sdrxn.c | Undo all writes using log |
| `reloadLogEntries` | sdrxn.c | Reconstruct log entry list from file |
| `restartION` | ionrestart.c | Full system recovery orchestration |
