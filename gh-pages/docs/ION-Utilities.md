# ION Utility Programs

Here is a short list of utility programs that come with ION that are frequently used by users to launch, stop, and query ION/BP operation status.

For comprehensive shutdown documentation, see the [ION Shutdown Guide](ION-Shutdown-Guide.md).

## Startup and Shutdown Utilities

There are three main ways to stop ION, each suited for different scenarios:

### ionexit (Recommended for Normal Shutdown)

The `ionexit` program is the recommended method for normal ION shutdown. It gracefully stops all ION daemon services in the correct dependency order.

**Usage:**
```bash
ionexit      # Stop ION and destroy SDR
ionexit k    # Stop ION but keep/preserve SDR
```

**Daemons stopped (in order):**
1. DTPC - Delay Tolerant Payload Conditioning (if enabled)
2. TCA/TCC - Trusted Custody services (if enabled)
3. BP - Bundle Protocol
4. LTP - Licklider Transmission Protocol
5. BSSP - Bundle Streaming Service Protocol (if enabled)
6. CFDP - CCSDS File Delivery Protocol
7. RFX - Contact plan system
8. IPC resources cleanup

The `k` option preserves the SDR state, which is useful for:
- Saving state before planned maintenance
- Enabling restart with `ionrestart` after a controlled shutdown
- Preserving bundle queue state for later transmission

**Important:** SDR preservation only works when the SDR is configured with file-based storage (`SDR_IN_FILE`). If the SDR uses DRAM-only storage (`SDR_IN_DRAM`), the data resides in shared memory and will be lost when processes exit, regardless of the `k` option.

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

* `killm` - Utility to forcefully terminate ION processes and clean up shared memory.
    **WARNING:** Force-kills all ION processes and clears System V IPC resources. Use only when normal shutdown fails. This is a destructive last-resort operation that:
    - Sends SIGTERM then SIGKILL to all ION processes
    - Destroys all System V shared memory and semaphores
    - Removes all ION POSIX named semaphores

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
