# ION Utility Programs

Here is a short list of utility programs that come with ION that are frequently used by users to launch, stop, and query ION/BP operation status:

* `ionexit` - A program that shuts down ION with the option to preserve the SDR.
    Normally, when ION's various daemons were stopped by calling `ionstop` or issuing the command '.' to the administration programs, the SDR will be modified/destroyed in the process. Calling `ionexit` with the argument `k` (for "keep") preserves the SDR state just prior to execution, allowing it to be saved in non-volatile storage such as a file if ION was configured to use a file-based SDR. It stops the BP, LTP, BSSP, CFDP, and RFX daemons, if they were running. Any other ION services started by the user will need to be stopped manually, and any user applications/processes attached to the SDR will need to detach, in order to notify the OS that ION shared resources are no longer in use and release those resources (e.g. SDR shared memory in DRAM).

    **Usage:** `ionexit` (destroys SDR) or `ionexit k` (preserves SDR)

* `ionstart` - Script to initialize and start ION node with configuration files
* `ionstop` - Script to gracefully stop all ION daemons and services
* `killm` - Utility to forcefully terminate ION processes and clean up shared memory
    **WARNING:** Force-kills all ION processes and clears System V IPC resources. Use only when normal shutdown (`ionstop`) fails. This is a destructive last-resort operation.
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
