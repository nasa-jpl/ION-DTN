# ION Utility Programs

Here is a short list of utility programs that come with ION that are frequently used by users to launch, stop, and query ION/BP operation status.

For comprehensive shutdown documentation, see the [ION Shutdown Guide](ION-Shutdown-Guide.md).

## Startup and Shutdown Utilities

There are three main ways to stop ION, each suited for different scenarios:

### ionexit (Recommended for Normal Shutdown)

The `ionexit` program is the recommended method for normal ION shutdown. It gracefully stops all ION daemon services in the correct dependency order. It is the primary way to shut down ION for most purposes.

**Modes:**

| Command | SDR | IPC | Use Case |
|---------|-----|-----|----------|
| `ionexit` | Destroyed | Destroyed | Normal shutdown. Clean slate for next run. |
| `ionexit k` | Preserved | Destroyed | Forensics only. SDR file remains on disk for inspection, but ION cannot be restarted (IPC is gone). |
| `ionexit n` | Destroyed | Preserved | Multi-node per host. Shut down one ION instance without destroying the shared IPC (sdrwm catalog, semaphores) that other instances on the same host depend on. |
| `ionexit k n` | Preserved | Preserved | Planned maintenance / restart. Only mode that allows a subsequent `ionstart` to reattach and resume where it left off. Works for all SDR storage modes. |

Flags can be combined in any order.

**Why `ionexit k n` is required for restart:**

`ionexit k` preserves the SDR data (the `.sdr` file on disk or the DRAM shared-memory segment) but still calls `sm_ipc_stop()`, which destroys the global semaphore table and sdrwm catalog. Without those IPC structures, a subsequent `ionstart` cannot cleanly reattach to the preserved SDR. The `n` flag skips `sm_ipc_stop()`, keeping both SDR data and the IPC infrastructure intact so that `ionstart` can resume normally.

**Daemons stopped (in order):**
1. DTPC - Delay Tolerant Payload Conditioning (if enabled)
2. TCA/TCC - Trusted Custody services (if enabled)
3. BP - Bundle Protocol
4. LTP - Licklider Transmission Protocol
5. BSSP - Bundle Streaming Service Protocol (if enabled)
6. CFDP - CCSDS File Delivery Protocol
7. RFX - Contact plan system
8. SDR cleanup (unless `k` flag used)
9. IPC resources cleanup (unless `n` flag used)

**Scope:** `ionexit` operates on a single ION instance — the one associated with the current working directory. It does not affect other ION instances on the same host. In multi-node environments, each node must be shut down individually by running `ionexit` from that node's working directory, or use `killm f` to stop all instances at once.

**Multi-node environments:** Use the `n` flag when multiple ION instances share the same host. Without `n`, `ionexit` calls `sm_ipc_stop()` which destroys the global semaphore table shared by all instances. See the [ION Shutdown Guide](ION-Shutdown-Guide.md) for details.

**Note:** User applications attached to ION must detach separately, and custom services started by the user must be stopped manually.

### Admin Programs with Period (`.`) Argument

Each admin program can stop its associated daemons by passing `.` as a command:

```bash
dtpcadmin .    # Stop DTPC daemons
cfdpadmin .    # Stop CFDP daemons
bpadmin .      # Stop BP daemons
ltpadmin .     # Stop LTP daemons
bsspadmin .    # Stop BSSP daemons
ionadmin .     # Stop ION core (rfxclock)
```

Use this method when you need fine-grained control over individual subsystems.

### ionstop and killm (Complete Cleanup)

* `ionstart` - Script to initialize and start ION node with configuration files

* `ionstop` - Script to gracefully stop all ION daemons and services. Calls each admin program with `.` to stop subsystems, then calls `killm` for single-ION instances. For multi-ION instances, it does NOT call `killm` to avoid affecting other instances.

* `killm` - Script to ensure a clean start by wiping out all prior ION instances and their resources. `killm` first attempts graceful shutdown via `ionexit`, then cleans up any remaining processes and IPC resources. It is multi-node aware: in multi-node environments, it defaults to shutting down only the current node.

    **Usage:**
    ```bash
    killm      # Graceful shutdown; multi-node safe (node-only if detected)
    killm f    # Force full cleanup of all ION instances on host
    ```

    **Behavior:**
    1. Attempts graceful shutdown via `ionexit` (uses `ionexit n` in multi-node environments to preserve shared IPC)
    2. Checks for surviving ION processes
    3. In multi-node mode (without `f`): stops here, preserving shared IPC for other instances
    4. In single-node or forced mode: sends SIGTERM then SIGKILL to remaining processes, then destroys all System V IPC and POSIX named semaphores

    **Multi-node detection:** `killm` detects a multi-node environment when `ION_NODE_LIST_DIR` is set and `$ION_NODE_LIST_DIR/ion_nodes` contains entries.

    **Exit codes:**

    | Code | Meaning |
    |------|---------|
    | `0` | Clean: no surviving ION processes, no IPC resources remaining, all survivor checks succeeded |
    | `1` | ION processes remained after the SIGTERM/SIGKILL cycle |
    | `2` | Could not verify survivor state (one or more `ps` snapshots failed); treat as unclean |
    | `3` | POSIX named semaphore files could not be removed (rerun with `sudo`) |

    See the [ION Shutdown Guide](ION-Shutdown-Guide.md#exit-codes) for caller guidance.

    The long-term plan is to transition `killm` into a backup script for clearing ION in abnormal situations, with `ionexit` serving as the primary graceful shutdown command for most purposes.

## Other Utilities
* `ionrestart` - Utility to restart ION node after a crash while preserving SDR state
* `ionwatch` - Continuous monitoring utility that displays ION node status and statistics in real-time, including contact plan, bundle statistics, and protocol state
* `sdrwatch` - Monitor and display SDR (Shared Data Region) memory usage statistics
* `psmwatch` - Monitor and display private shared memory usage statistics
* `bpstats` - Display Bundle Protocol statistics including bundle counts and transmission rates
* `bpstats2` - Enhanced bundle statistics utility with additional metrics
* `ltpstats` - Display LTP (Licklider Transmission Protocol) statistics for all configured spans with flexible reporting modes (key, all, or grouped statistics)
* `bplist` - List all bundles currently in custody showing source, destination, and status
* `bpcancel` - Cancel transmission of specified bundles by ID or endpoint. Bundles are permanently deleted and cannot be recovered. **Note:** Consider using `bpinspect` instead, which provides bundle inspection before deletion.
* `bptrace` - Trace bundle transmission path and report hop-by-hop delivery status
* `cfdptest` - Interactive test utility for CFDP file transfer operations
* `cbrcustodytest` - Test and monitor Compressed Bundle Reporting / Custody Transfer (CBR/CT, CCSDS Orange Book). Normal mode sends a custody-enabled bundle and verifies the accept → CCS → release lifecycle; `-l` lists bundles currently held in custody tracking along with CBR/CT statistics. Requires `m custodymode orangebook` in the `bprc` file.
* `bpcrash` - Testing utility to simulate controlled bundle protocol failures. **WARNING:** This intentionally crashes the BP system for testing purposes. Use only in test environments.
* `runtests` - Execute ION regression test suite
* `owltsim` - One-Way Light Time simulator for testing DTN protocols with realistic delays

## New Bundle Management Utilities (ION 4.1.4-b.1)

* `bpinspect` - A utility for inspecting, filtering, and managing bundles in ION's custody.

`bpinspect` provides fine-grained control over bundles stored in ION's bundle database. It enables users to list bundles with detailed metadata, filter bundles by source, destination, creation time, and other attributes, and perform suspend and resume operations for selective bundle processing. This tool is particularly useful for debugging bundle routing issues, managing storage capacity, and controlling bundle transmission priorities.

* `bptracker` - An enhanced interactive bundle tracking utility with demonstration capabilities.

`bptracker` provides real-time bundle status monitoring with flexible send syntax and improved source routing record (SRR) parsing. It offers fine-grained control over individual bundles, making it valuable for testing, demonstration, and debugging of bundle transmission and routing behavior. The interactive mode allows users to track bundles throughout their lifecycle in the DTN network.

**Note:** To receive status reports, you must run `bptrace -listen` in a separate terminal before using `bptracker`, as it relies on `bptrace` for status report reception.

* `bpcrash_hard` - A testing utility for validating ION's crash recovery and reversibility features.

`bpcrash_hard` is designed to test ION's resilience under extreme failure conditions. It validates the system's ability to recover from crashes and maintain data integrity through ION's reversibility mechanisms. This tool is primarily used during system testing and validation to ensure proper operation of ION's fault tolerance features.

**WARNING:** This utility intentionally causes hard system crashes to test recovery mechanisms. Use only in isolated test environments, never in production systems.
