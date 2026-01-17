# ION Shutdown Guide

This guide provides comprehensive documentation on the various methods available for stopping ION nodes and cleaning up system resources. Understanding when and how to use each method is critical for proper ION operation.

## Overview

ION provides four primary shutdown methods, each suited for different scenarios:

| Method | Use Case | Preserves SDR | Graceful | Destructive |
|--------|----------|---------------|----------|-------------|
| **Admin Programs (`.`)** | Manual control of individual subsystems | Configurable | Yes | No |
| **ionexit** | Normal shutdown (recommended) | Optional (`k` flag) | Yes | No |
| **ionstop/killm** | Complete system cleanup | No | Partial | Yes |
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

The `ionexit` program is the recommended method for normal ION shutdown. It gracefully stops all ION daemon services in the correct dependency order while optionally preserving the SDR (Shared Data Region) state.

**Usage:**
```bash
ionexit      # Stop ION and destroy SDR
ionexit k    # Stop ION but keep/preserve SDR
```

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
10. **IPC** - Inter-process communication resources

**When to use:**
- Normal operational shutdown
- When you want to preserve SDR state for later restart (`ionexit k`)
- Before system maintenance
- When transitioning between configurations

**Preserving SDR State:**

Using `ionexit k` preserves the SDR state, which is useful for:
- Saving state before planned maintenance
- Enabling restart with `ionrestart` after a controlled shutdown
- Preserving bundle queue state for later transmission

**Important: SDR Storage Mode Requirement**

SDR preservation with `ionexit k` only works when the SDR is configured with file-based storage (`SDR_IN_FILE`). If the SDR is configured with DRAM-only storage (`SDR_IN_DRAM`), the data resides in shared memory and will be lost when processes exit, regardless of the `k` option.

| SDR Config | `ionexit k` Effect | Restart Possible |
|------------|-------------------|------------------|
| `SDR_IN_FILE` | SDR file preserved on disk | Yes, via `ionrestart` |
| `SDR_IN_DRAM` only | Shared memory destroyed on exit | No |
| `SDR_IN_FILE \| SDR_IN_DRAM` | SDR file preserved, memory cache lost | Yes, via `ionrestart` |

To check your ION configuration, look for `configFlags` in your ionconfig file or initialization code. For persistent storage, ensure `SDR_IN_FILE` is set.

**Important Notes:**
- User applications attached to ION must detach separately
- Custom services started by the user must be stopped manually
- The `k` option only preserves SDR; processes are still terminated
- SDR preservation requires `SDR_IN_FILE` configuration

**Multi-Node Configuration Warning:**

`ionexit` is **not suitable for multi-node per host configurations** (such as regression test environments). This is because `ionexit` calls `sm_ipc_stop()` at the end, which destroys the IPC system shared by all ION instances on the host. Running `ionexit` when multiple nodes are active will terminate all nodes, not just the one you attached to.

| Configuration | ionexit Suitable? | Recommended Shutdown Method |
|---------------|-------------------|----------------------------|
| Single node per host (deployment, beta testing) | **Yes** | `ionexit` or `ionexit k` |
| Multiple nodes per host (regression testing) | **No** | Admin programs (e.g., `bpadmin .`, `ltpadmin .`, `ionadmin .`) |

For multi-node test environments, shut down individual nodes using the admin programs with the `.` argument in each node's working directory, or use `killm` at the end of testing to clean up all nodes simultaneously.

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

**Usage:**
```bash
killm
```

**WARNING:** This is a destructive operation that force-terminates all ION processes system-wide.

**What killm does:**
1. Reads process names from `ionprocesses.txt` (installed alongside `killm`)
2. Sends SIGTERM to all ION processes
3. Waits briefly for graceful termination
4. Sends SIGKILL to any remaining ION processes
5. Destroys all System V shared memory segments owned by current user
6. Destroys all System V semaphores owned by current user
7. Removes all POSIX named semaphores matching ION patterns

**When to use:**
- After a failed normal shutdown
- When ION processes are hung or unresponsive
- When shared resources are corrupted
- During system recovery after crashes
- Before a fresh ION installation test

**Cross-Platform Support:**
`killm` works on Linux, macOS, Solaris, and Windows (with appropriate tools).

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

/* Clean up IPC resources */
sm_ipc_stop();
```

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

    /* Clean up SDR */
    if (!preserve_sdr) {
        ionTerminate(1);  /* Destroy SDR */
    } else {
        ionTerminate(0);  /* Preserve SDR for restart */
        /* Note: SDR preservation only works with SDR_IN_FILE config.
         * If SDR_IN_DRAM only, shared memory is lost on exit. */
    }

    /* Clean up IPC */
    sm_ipc_stop();
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
├─► Want to preserve SDR state?
│   └─► YES: Use `ionexit k` or ionTerminate(0) via API
│
├─► Normal operational shutdown?
│   └─► YES: Use `ionexit`
│
├─► Need to stop specific subsystem only?
│   └─► YES: Use appropriate admin program with `.` or API
│
├─► Multiple ION instances running?
│   └─► YES: Use local ionstop script, admin programs, or APIs
│
├─► Normal shutdown failed or processes hung?
│   └─► YES: Use `killm`
│
└─► Complete system cleanup needed?
    └─► YES: Use `ionstop` (single instance) or `killm`
```

### Comparison Matrix

| Scenario | Recommended Method | Reason |
|----------|-------------------|--------|
| End of day shutdown | `ionexit` | Graceful, cleans up properly |
| Before maintenance | `ionexit k` | Preserves state for restart |
| Debug specific subsystem | `bpadmin .`, `ltpadmin .`, etc. | Targeted control |
| System crash recovery | `killm` | Force cleanup of all resources |
| Multi-node: stop one node | Local `ionstop` or admin programs | Avoids affecting other nodes |
| Test environment reset | `killm` | Complete cleanup |
| Production shutdown | `ionexit` then verify with `ps` | Graceful with verification |
| Embedded/flight software | Public APIs (`bp_stop()`, etc.) | No shell required |
| Automated test framework | Public APIs | Programmatic control |
| Custom ION management app | Public APIs | Full lifecycle control |

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

1. **Use ionexit for normal operations** - It's the cleanest shutdown method

2. **Preserve SDR when appropriate** - Use `ionexit k` before planned maintenance

3. **Verify shutdown completed** - Always check for remaining processes and resources

4. **Use killm sparingly** - Only when normal shutdown fails

5. **Document your shutdown procedure** - Especially in multi-node environments

6. **Clean up before fresh starts** - Run `killm` before testing new configurations

7. **Handle multi-ION carefully** - Set `ION_NODE_WDNAME` appropriately

8. **Check logs** - Review `ion.log` if shutdown behaves unexpectedly

## Related Documentation

- [ION Utilities](ION-Utilities.md) - Overview of ION utility programs
- [ION Deployment Guide](ION-Deployment-Guide.md) - Comprehensive deployment instructions
- [SOP for ION](SOP-for-ION.md) - Standard Operating Procedures
- [ION Quick Start Guide](ION-Quick-Start-Guide.md) - Getting started guide

## See Also

- Man pages: `ionadmin(1)`, `bpadmin(1)`, `ltpadmin(1)`, `cfdpadmin(1)`
- Configuration files: `ionrc(5)`, `bprc(5)`, `ltprc(5)`
