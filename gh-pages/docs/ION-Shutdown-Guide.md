# ION Shutdown Guide

This guide provides comprehensive documentation on the various methods available for stopping ION nodes and cleaning up system resources. Understanding when and how to use each method is critical for proper ION operation.

## Overview

ION provides four primary shutdown methods, each suited for different scenarios:

| Method | Use Case | Preserves SDR | Graceful | Destructive |
|--------|----------|---------------|----------|-------------|
| **Admin Programs (`.`)** | Manual control of individual subsystems | Configurable | Yes | No |
| **ionexit** | Normal shutdown (recommended) | Optional (`k n` flags) | Yes | No |
| **killm** | Ensure clean state; backup for abnormal situations | No | Yes (via ionexit), then forced | Yes (single-node/forced mode) |
| **ionstop** | Legacy shutdown script | No | Partial | Yes |
| **Public APIs** | Embedded/programmatic control | Configurable | Yes | No |

## Shutdown Methods

### Method 1: Admin Programs with Period (`.`) Argument

This method provides fine-grained manual control over individual ION subsystems. Each admin program can be instructed to stop its associated daemons by passing `.` as a command file argument.

**Usage:**
```bash
# Stop subsystems in reverse dependency order (application layer first)
dtpcadmin .    # Stop DTPC daemons (if running)
cfdpadmin .    # Stop CFDP daemons
bpadmin .      # Stop BP daemons
ltpadmin .     # Stop LTP daemons
bsspadmin .    # Stop BSSP daemons (if running)
ionadmin .     # Stop ION core (rfxclock)
```

**When to use:**
- When you need to stop specific subsystems while keeping others running
- For debugging purposes
- When scripting custom shutdown sequences
- When troubleshooting shutdown issues

**Admin Programs Supporting `.` Shutdown:**

| Program | Subsystem | Daemons Stopped |
|---------|-----------|-----------------|
| `ionadmin .` | ION Core | rfxclock |
| `ltpadmin .` | LTP | ltpclock, ltpmeter, link service adapters |
| `bpadmin .` | BP | bpclock, forwarders, CLAs, transit daemons |
| `cfdpadmin .` | CFDP | cfdpclock, UT layer service |
| `bsspadmin .` | BSSP | bsspclock, link service adapters |
| `dtpcadmin .` | DTPC | dtpcclock, dtpcd |

### Method 2: ionexit (Recommended for Normal Shutdown)

The `ionexit` program is the recommended method for normal ION shutdown. It gracefully stops all ION daemon services in the correct dependency order while optionally preserving the SDR (Shared Data Region) state and/or IPC resources.

**Modes:**

`ionexit` has two independent flags (`k` and `n`) that can be combined, giving four distinct modes:

| Command | SDR | IPC | Restartable? | Use Case |
|---------|-----|-----|--------------|----------|
| `ionexit` | Destroyed | Destroyed | No (clean slate) | **Normal shutdown.** Removes all ION state. Use when you want a fresh start next time. |
| `ionexit k` | Preserved | Destroyed | **No** | **Forensics/inspection only.** The `.sdr` file remains on disk for examination, but IPC destruction prevents `ionstart` from reattaching. |
| `ionexit n` | Destroyed | Preserved | No (SDR gone) | **Multi-node per host.** Shut down one ION instance and discard its SDR, without destroying the shared IPC (sdrwm catalog, global semaphores) that other instances on the same host depend on. |
| `ionexit k n` | Preserved | Preserved | **Yes** | **Planned maintenance / restart.** The only mode that allows a subsequent `ionstart` to reattach and resume where it left off. Works for all SDR storage modes. |

Flags can be combined in any order.

**Why `ionexit k n` is required for restart:**

`ionexit k` preserves SDR data (the `.sdr` file on disk or the DRAM shared-memory segment) but still calls `sm_ipc_stop()`, which destroys the global semaphore table and sdrwm catalog. Without those IPC structures, a subsequent `ionstart` cannot cleanly reattach to the preserved SDR. The `n` flag skips `sm_ipc_stop()`, keeping both SDR data and the IPC infrastructure intact so that `ionstart` can resume normally.

**Shutdown Order:**
`ionexit` stops services in the following order (application layer first, then core):

1. **DTPC** - Delay Tolerant Payload Conditioning (if enabled)
2. **TCA** - Trusted Custody Authority instances (if enabled)
3. **TCC** - Trusted Custody Client instances (if enabled)
4. **BP** - Bundle Protocol
5. **LTP** - Licklider Transmission Protocol
6. **BSSP** - Bundle Streaming Service Protocol (if enabled)
7. **CFDP** - CCSDS File Delivery Protocol
8. **RFX** - Contact plan/range system
9. **SDR** - Shared Data Region cleanup (unless `k` flag used)
10. **IPC** - Inter-process communication resources (unless `n` flag used)

**When to use each mode:**

| Scenario | Command | Why |
|----------|---------|-----|
| Normal operational shutdown | `ionexit` | Clean slate; no residual state |
| Preserve state for restart | `ionexit k n` | Only mode that supports restart via `ionstart` |
| Inspect SDR after shutdown | `ionexit k` | Preserves `.sdr` file for post-mortem analysis |
| Multi-node: stop one node, discard its data | `ionexit n` | Preserves shared IPC for other instances |
| Multi-node: stop one node, keep its data | `ionexit k n` | Preserves both node SDR and shared IPC |
| Multi-node: stop last node | `ionexit` | Safe to release IPC when no other instances remain |

**Important Notes:**
- `ionexit` operates on a single ION instance — the one associated with the current working directory. It does **not** stop other ION instances on the same host, even with `killm f`. In a multi-node environment, each node must be shut down individually by running `ionexit` from that node's working directory. To stop **all** ION instances at once, use `killm f` instead, which sends SIGTERM/SIGKILL to all ION processes and destroys all shared IPC resources.
- User applications attached to ION must detach separately
- Custom services started by the user must be stopped manually
- All modes stop all ION daemon processes for the current node; only SDR data and IPC resources are optionally preserved
- When in doubt about multi-node, use `n` — IPC can always be cleaned up later with `killm f`

### Method 3: ionstop and killm (Complete Cleanup)

The `ionstop` script and `killm` utility provide complete system cleanup, ensuring all ION processes are terminated and all shared resources are released.

#### ionstop Script

**Usage:**
```bash
ionstop
```

**Behavior:**
- Calls each admin program with `.` to gracefully stop subsystems
- For single-ION instances: calls `killm` automatically
- For multi-ION instances: does NOT call `killm` (to avoid affecting other instances)
- Uses `ION_NODE_WDNAME` environment variable to determine which instance to stop

**Multi-ION Instance Considerations:**

When running multiple ION instances on the same host:

1. Set the environment variable before calling `ionstop`:
   ```bash
   export ION_NODE_WDNAME=/path/to/ion/working/directory
   ionstop
   ```

2. The global `ionstop` will NOT call `killm` when multiple instances are detected

3. Use local `ionstop` scripts in each node's working directory for targeted shutdown

#### killm Script

`killm` is the overall script used to ensure a clean start by wiping out all prior ION instances. It deploys `ionexit` first for graceful shutdown, then cleans up any remaining processes and IPC resources. The long-term plan is to transition `killm` into a backup script for clearing ION in abnormal situations, with `ionexit` serving as the primary graceful shutdown command for most purposes.

**Usage:**
```bash
killm      # Graceful shutdown; multi-node safe (node-only if detected)
killm f    # Force full cleanup of all ION instances on host
```

**What killm does:**
1. Detects multi-node environment (`ION_NODE_LIST_DIR` set with `ion_nodes` file)
2. Attempts graceful shutdown via `ionexit` (uses `ionexit n` in multi-node mode)
3. Checks for surviving ION processes after `ionexit`
4. **Multi-node mode (without `f`):** Stops here, preserving shared IPC for other instances
5. **Single-node or forced mode (`f`):**
   - If `ionexit` stopped all processes, skips the SIGTERM/SIGKILL cycle
   - Otherwise sends SIGTERM, waits, then SIGKILL to remaining ION processes
   - Destroys all System V shared memory segments and semaphores owned by current user
   - Removes all POSIX named semaphores matching ION patterns

**When to use:**
- Before a fresh ION start to ensure a clean state
- After a failed normal shutdown
- When ION processes are hung or unresponsive
- When shared resources are corrupted
- During system recovery after crashes
- In multi-node environments: `killm` (without `f`) safely stops only the current node
- Use `killm f` to force full cleanup of all instances on the host

**Cross-Platform Support:**
`killm` works on Linux, macOS, and Solaris.

### Method 4: Programmatic Shutdown via Public APIs

ION provides public C APIs that enable applications to configure, start, and stop ION subsystems programmatically without using command-line tools. This method is ideal for embedded systems, automated test frameworks, and applications that need full control over the ION lifecycle.

**Available API Headers:**

| Header | Subsystem | Key Functions |
|--------|-----------|---------------|
| `ion_admin.h` | ION Core | Contact/range management |
| `ltp_admin.h` | LTP | `ltp_init()`, `ltp_start()`, `ltp_stop()` |
| `bp_admin.h` | BP | `bp_init()`, `bp_start()`, `bp_stop()` |
| `rfx.h` | RFX | `rfx_start()`, `rfx_stop()` |

**Shutdown Functions:**

```c
#include "bp_admin.h"
#include "ltp_admin.h"
#include "rfx.h"
#include "ion.h"
#include "sdr.h"
#include "platform.h"

/* Stop BP agent and all its daemons */
bp_stop();

/* Wait for BP to fully stop */
while (bp_agent_is_started()) {
    snooze(1);
}

/* Stop LTP engine and all LSO/LSI processes */
ltp_stop();

/* Wait for LTP to fully stop */
while (ltp_engine_is_started()) {
    snooze(1);
}

/* Stop RFX (contact plan system) */
rfx_stop();

/* Wait for RFX to fully stop */
while (rfx_system_is_started()) {
    snooze(1);
}

/* Delete SDR (pass 1 to destroy, 0 to preserve) */
ionTerminate(1);

/* Clean up IPC resources (skip in multi-node environments) */
sm_ipc_stop();
```

**Note:** To preserve SDR for restart, call `ionTerminate(0)` instead of `ionTerminate(1)` AND omit the `sm_ipc_stop()` call (equivalent to `ionexit k n`). The `sm_ipc_stop()` function destroys the global semaphore table and sdrwm catalog; without them, a subsequent `ionstart` cannot reattach to the preserved SDR regardless of storage mode. In multi-node environments, also omit `sm_ipc_stop()` to avoid destroying shared IPC used by other instances.

**Complete Cleanup Example:**

```c
void programmatic_shutdown(int preserve_sdr)
{
    int loopcount;

    /* Stop BP (stops bpclock, bptransit, forwarders, CLAs) */
    bp_stop();
    for (loopcount = 5; bp_agent_is_started() && loopcount; loopcount--) {
        snooze(1);
    }

    /* Stop LTP (stops ltpclock, ltpdeliv, all LSO/LSI) */
    ltp_stop();
    for (loopcount = 5; ltp_engine_is_started() && loopcount; loopcount--) {
        snooze(1);
    }

    /* Stop RFX (stops rfxclock) */
    rfx_stop();
    for (loopcount = 5; rfx_system_is_started() && loopcount; loopcount--) {
        snooze(1);
    }

    /* Clean up SDR and IPC */
    if (!preserve_sdr) {
        ionTerminate(1);  /* Destroy SDR */
        sm_ipc_stop();    /* Release IPC resources */
    } else {
        /* To enable restart: preserve SDR AND skip sm_ipc_stop().
         * Calling sm_ipc_stop() destroys the sdrwm catalog and
         * semaphores, preventing ionstart from reattaching. */
    }
}
```

**Fine-Grained Control:**

The APIs also support stopping individual components:

```c
/* Stop a specific scheme forwarder */
bp_stop_scheme("ipn");

/* Stop a specific egress plan */
bp_stop_plan("ipn:2.0");

/* Stop a specific outduct */
bp_stop_outduct("ltp", "2");

/* Stop a specific LTP span */
ltp_stop_span(2);  /* Stop LSO for engine ID 2 */
```

**When to use:**
- Embedded systems without shell access
- Automated testing frameworks
- Custom ION management applications
- Flight software requiring programmatic control
- Applications needing graceful shutdown with state preservation

**Demonstration Tests:**

The `tests/admin_public_api/` directory contains working examples:

| Test | Description |
|------|-------------|
| `ltp_loopback/` | Complete node initialization, configuration, and shutdown via API |
| `tcp_2nodes/` | Multi-node setup demonstrating per-node shutdown |
| `ltp_span_management/` | Runtime span start/stop operations |
| `bp_plan_crash_recovery/` | Daemon restart after crash using `bp_start_plan()` |

These tests demonstrate the complete lifecycle from `ionInitialize()` through configuration, operation, and cleanup using `ionTerminate()` and `sm_ipc_stop()`.

**API Documentation:**

For complete API documentation, see the [Public Administration API Guide](Public-Admin-API-Guide.md).

## Choosing the Right Shutdown Method

### Decision Tree

```
Need to stop ION?
│
├─► Embedded system or programmatic control needed?
│   └─► YES: Use public APIs (bp_stop, ltp_stop, etc.)
│
├─► Want to preserve state and restart later?
│   └─► YES: Use `ionexit k n` (only mode that supports restart)
│
├─► Want to preserve SDR file for inspection (not restart)?
│   └─► YES: Use `ionexit k`
│
├─► Normal operational shutdown?
│   └─► YES: Use `ionexit` (primary recommended method)
│
├─► Need to stop specific subsystem only?
│   └─► YES: Use appropriate admin program with `.` or API
│
├─► Multiple ION instances on same host?
│   ├─► Stop one node, discard data: `ionexit n`
│   ├─► Stop one node, keep data: `ionexit k n`
│   └─► Stop all nodes: `killm f`
│
├─► Need a clean start (pre-test reset)?
│   └─► YES: Use `killm` (graceful via ionexit, then cleanup)
│
├─► Normal shutdown failed or processes hung?
│   └─► YES: Use `killm f` to force full cleanup
│
└─► Complete system cleanup needed?
    └─► YES: Use `killm f`
```

### Comparison Matrix

| Scenario | Recommended Method | Reason |
|----------|-------------------|--------|
| Normal operational shutdown | `ionexit` | Clean slate; primary graceful shutdown |
| Preserve state for restart | `ionexit k n` | Only mode that keeps both SDR and IPC, enabling `ionstart` to resume |
| Inspect SDR after shutdown | `ionexit k` | Preserves `.sdr` file for forensics; cannot restart |
| Debug specific subsystem | `bpadmin .`, `ltpadmin .`, etc. | Targeted control |
| Multi-node: stop one node, discard data | `ionexit n` | Destroys node SDR, preserves shared IPC for other instances |
| Multi-node: stop one node, keep data | `ionexit k n` | Preserves node SDR and shared IPC |
| Multi-node: stop all nodes | `killm f` | Full cleanup of all instances and IPC |
| Pre-test clean start | `killm` | Graceful shutdown via ionexit, then cleanup |
| System crash recovery | `killm f` | Force cleanup of all resources |
| Production shutdown | `ionexit` then verify with `ps` | Graceful with verification |
| Embedded/flight software | Public APIs (`bp_stop()`, etc.) | No shell required |
| Automated test framework | Public APIs | Programmatic control |

## Verifying Shutdown

After shutdown, verify that ION has fully stopped:

### Check for Running Processes

```bash
# Look for key ION daemon processes
ps -ef | grep -E "rfxclock|bpclock|ltpclock|bpclm|ipnfw|bptransit"

# Use the comprehensive ION process list file for pattern matching
ps -ef | grep -f /usr/local/bin/ionprocesses.txt
# Or from source directory:
ps -ef | grep -f ionprocesses.txt
```

**Note:** The `ionprocesses.txt` file contains one ION process name per line and is used by both `killm` and for `grep -f` pattern matching.

### Check for Shared Memory

```bash
ipcs

# Look for ION-related keys:
# 0x0000ee02 - SM_SEMBASEKEY (semaphore tracking)
# 0x0000ff00 - SDR working memory
# 0x0000ff01 - ION working memory
```

### Check for POSIX Named Semaphores

```bash
# Linux (Ubuntu)
ls /dev/shm/ | grep "sem.ion"

# Pattern: sem.ion:GLOBAL:<integer>
```

### Automated Check with ionwatch

```bash
ionwatch -e  # Shows daemon status, exits if ION not running
```

## Troubleshooting Shutdown Issues

### Shutdown Hangs

If `ionexit` or admin programs hang:

1. Check `ion.log` for error messages
2. Try stopping subsystems individually with admin programs
3. Use `killm` as last resort

### Processes Won't Terminate

```bash
# Find stubborn processes
ps -ef | grep ion

# Force kill specific process
kill -9 <pid>

# Or use killm for complete cleanup
killm
```

### Shared Memory Not Released

```bash
# List shared memory
ipcs -m

# Remove specific segment (use with caution)
ipcrm -m <shmid>

# Or let killm handle it
killm
```

### Semaphores Left Behind

```bash
# POSIX named semaphores (Linux)
rm /dev/shm/sem.ion:GLOBAL:*

# System V semaphores
ipcs -s
ipcrm -s <semid>
```

### Docker/Kubernetes Issues

If running ION in Docker with PID 1:
- ION process with PID 1 cannot be killed normally
- Use [dumb-init](https://github.com/Yelp/dumb-init) as entrypoint
- Override entrypoint in Kubernetes manifest

## Best Practices

1. **Use `ionexit` for normal shutdown** - Clean slate; primary graceful shutdown method

2. **Use `ionexit k n` to preserve state for restart** - This is the only mode that supports restart via `ionstart`. Both SDR data and IPC must be preserved.

3. **Use `ionexit k` only for forensics** - The `.sdr` file is preserved on disk for inspection, but ION cannot be restarted because IPC is destroyed

4. **Use `ionexit n` in multi-node environments** - Prevents destroying shared IPC used by other instances on the same host

5. **Verify shutdown completed** - Always check for remaining processes and resources

6. **Use killm for clean starts** - `killm` deploys ionexit first, then cleans up remaining resources

7. **Use killm f sparingly** - Force full cleanup only when graceful methods fail or all instances need clearing

8. **Handle multi-ION carefully** - Set `ION_NODE_WDNAME` and `ION_NODE_LIST_DIR` appropriately

9. **Check logs** - Review `ion.log` if shutdown behaves unexpectedly

## Related Documentation

- [ION Utilities](ION-Utilities.md) - Overview of ION utility programs
- [ION Deployment Guide](ION-Deployment-Guide.md) - Comprehensive deployment instructions
- [SOP for ION](SOP-for-ION.md) - Standard Operating Procedures
- [ION Quick Start Guide](ION-Quick-Start-Guide.md) - Getting started guide

## See Also

- Man pages: `ionadmin(1)`, `bpadmin(1)`, `ltpadmin(1)`, `cfdpadmin(1)`
- Configuration files: `ionrc(5)`, `bprc(5)`, `ltprc(5)`
