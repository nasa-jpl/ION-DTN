# ION Monitoring Guide

## Overview

ION provides three essential monitoring utilities to track system health and resource usage:

- **ionwatch** - Monitors daemon process status across all ION protocols
- **sdrwatch** - Monitors SDR (non-volatile data store) memory usage
- **psmwatch** - Monitors PSM (private shared memory) partition usage

These tools are critical for:
- Verifying that all required daemons are running
- Detecting memory leaks in SDR and PSM
- Troubleshooting performance issues
- Monitoring system health in production environments
- Diagnosing storage space problems

## ionwatch - Daemon Status Monitoring

### Purpose

`ionwatch` monitors the status of all ION daemon processes by checking their PIDs in the volatile database and verifying that the processes are actually running. It provides a comprehensive view of daemon health across all ION protocols.

### Usage

```bash
# Display daemon status once
ionwatch

# Continuously monitor, refreshing every 5 seconds
ionwatch -w 5

# Monitor for 10 iterations with 2-second intervals
ionwatch -w 2 -c 10

# Show only running daemons
ionwatch -r

# Log output to ion.log instead of stdout
ionwatch -l

# Quiet mode: show full status initially, then only changes
ionwatch -w 10 -q

# Recommended watchdog mode: log only changes every 10 seconds
ionwatch -w 10 -q -l
```

### Command-Line Options

| Option | Description |
|--------|-------------|
| `-w, --watch <interval>` | Watch mode: refresh every `interval` seconds |
| `-c, --count <count>` | Number of refresh cycles (default: 1) |
| `-r, --running-only` | Show only running daemons |
| `-l, --log` | Output to ion.log instead of stdout |
| `-q, --quiet` | Show full status initially, then only changes |
| `-h, --help` | Show help message |

### Output Format

```
Protocol Daemon               PID        Status       Notes
--------------------------------------------------------------------------------
ICI      rfxclock             26         Running                  Contact plan manager
LTP      ltpclock             36         Running                  Event scheduler
LTP      ltpdeliv             37         Running                  Delivery service
LTP      udplso n20:1113 [2]  39         Running                  Link service output
LTP      ltpmeter [2]         38         Running                  Meter
LTP      udplsi [::]:1113     40         Running                  Link service input
BP       bpclock              47         Running                  Event scheduler
BP       cpsd                 48         STALE                    Contact plan sync
BP       bptransit            49         Running                  Transit processor
BP       bpclm [ipn:2.0]      46         Running                  CL manager
BP       ltpcli [1]           52         Running                  CL input
BP       ltpclo [2]           53         Running                  CL output
--------------------------------------------------------------------------------
```

**Note:** In this example, the `cpsd` daemon shows **STALE** status (PID 48), indicating it has crashed or been killed. This requires investigation and likely a daemon restart or full ION restart.

### Output Fields Explained

#### Protocol Column
Identifies which ION protocol layer the daemon belongs to:
- **ICI** - Inter-node Communication Infrastructure (core layer)
- **LTP** - Licklider Transmission Protocol (convergence layer)
- **BP** - Bundle Protocol (application layer)
- **CFDP** - CCSDS File Delivery Protocol
- **DTPC** - Delay-Tolerant Payload Conditioning
- **BSSP** - Bundle Streaming Service Protocol

#### Daemon Column
The name of the daemon process. Common daemons include:

**ICI Daemons:**
- `rfxclock` - Contact plan manager that maintains the contact graph routing table

**LTP Daemons:**
- `ltpclock` - Event scheduler for timer-driven activities
- `ltpdeliv` - Delivery service that delivers received data blocks
- `udplso [engineId]` - Link Service Output for specific span
- `ltpmeter [engineId]` - Metering daemon for rate control
- `udplsi` - Link Service Input (per-seat daemon)

**BP Daemons:**
- `bpclock` - Event scheduler for Bundle Protocol events
- `cpsd` - Contact Plan Sync daemon
- `bptransit` - Transit processor for bundle forwarding
- `bpclm [eid]` - Convergence Layer Manager for specific endpoint
- `udpcli [duct]` - Convergence Layer Input for specific induct
- `udpclo [duct]` - Convergence Layer Output for specific outduct

**CFDP Daemons:**
- `cfdpclock` - Event scheduler for CFDP
- `cfdp UT layer` - User Transaction adapter

**DTPC Daemons:**
- `dtpcclock` - Event scheduler for DTPC
- `dtpcd` - Main DTPC daemon

**BSSP Daemons:**
- `bsspclock` - Event scheduler for BSSP

#### PID Column
The Process ID of the daemon as registered in the volatile database. A PID of -1 or 0 indicates the daemon was never started.

#### Status Column
The current operational status of the daemon:

- **Running** - The daemon's PID is registered and the process exists and is executing normally. This is the expected state for all operational daemons.

- **Not Started** - The daemon has never been started. The PID is -1 or unset in the volatile database. This may be intentional if the daemon is not needed for your configuration (e.g., optional protocols), or it may indicate a startup problem.

- **STALE** - The daemon's PID is registered in the database, but the process no longer exists. This indicates the daemon terminated abnormally without properly clearing its PID. In ION 4.1.5+, daemons implement self-cleanup, so STALE entries typically indicate the daemon crashed or was forcibly killed (kill -9).

#### Notes Column
Brief description of the daemon's function to help understand its role in the ION stack.

### Interpreting ionwatch Output

**Healthy System:**
All required daemons show "Running" status. Optional protocol daemons (CFDP, DTPC, BSSP) may show "Not Started" if those protocols are not configured.

**Daemon Failure:**
If a daemon shows "STALE" status, it has crashed or been killed. Check ion.log for error messages and restart the daemon or the entire ION node.

**Configuration Issue:**
If a required daemon shows "Not Started" after running ionstart, there may be a configuration error or startup script issue. Check your configuration files and ion.log.

**Performance Monitoring:**
Use watch mode (`-w`) to continuously monitor daemon health, especially useful during testing or when diagnosing intermittent failures.

## sdrwatch - SDR Memory Monitoring

### Purpose

`sdrwatch` monitors the SDR (Simple Data Recorder), ION's non-volatile data store that holds persistent protocol state, bundle payload data, and other critical information. It helps detect memory leaks and monitor heap space utilization.

### Usage

```bash
# Display current SDR usage summary once
sdrwatch ion -t 0

# Print statistics for current transaction
sdrwatch ion -s

# Reset log length high-water mark and print stats
sdrwatch ion -r

# Print stats and ZCO status
sdrwatch ion -z

# Trace mode: monitor allocation/deallocation every 5 seconds
sdrwatch ion -t 5

# Continuous tracing with 10 iterations at 3-second intervals
sdrwatch ion -t 3 10

# Verbose trace showing all allocations (not just leaks)
sdrwatch ion -t 5 10 verbose
```

### Operating Modes

| Mode | Description |
|------|-------------|
| `-t` (default) | Trace mode: reports on SDR space allocation and release activity |
| `-s` | Statistics mode: prints current transaction statistics |
| `-r` | Reset mode: resets max log length high-water mark, then prints stats |
| `-z` | ZCO mode: prints stats plus Zero-Copy Objects status |

### Output Format (Trace Mode with interval=0)

```
-- sdr 'ion' usage report --
small pool free blocks:
           8 of size            8
          12 of size           16
           5 of size           24
       total avbl:      1245680
     total unavbl:       234320
       total size:      1480000

large pool free blocks:
           3 of order          256
           1 of order          512
       total avbl:      8942560
     total unavbl:      1057440
       total size:     10000000

total heap size:       11480000
total unused:           2156320
max total used:         9323680
total now in use:       8031360

max xn log len:           45632
```

### Output Fields Explained

#### Small Pool Section

The small pool manages small allocations efficiently using fixed-size blocks.

**Small pool free blocks:**
Lists the count and size of available free blocks in the small pool, grouped by size. Each size class represents a multiple of WORD_SIZE (typically 8 bytes). This shows the fragmentation level of small allocations.
- Format: `count of size bytes`
- Example: `12 of size 16` means there are 12 free blocks of 16 bytes each

**total avbl (available):**
Total bytes available in the small pool's free block lists. This memory can be immediately allocated for small objects without fragmentation. Higher values indicate good availability of small blocks.

**total unavbl (unavailable):**
Total bytes currently allocated from the small pool and in use by ION. This represents small objects currently holding data.

**total size:**
Total size of the entire small pool (avbl + unavbl). This is the configured capacity for small allocations.

#### Large Pool Section

The large pool manages larger allocations using a buddy system with power-of-two sized blocks.

**Large pool free blocks:**
Lists the count and order (size) of available free blocks in the large pool. The buddy system organizes blocks by powers of two.
- Format: `count of order bytes`
- Example: `3 of order 256` means there are 3 free blocks of 256 bytes each
- Orders typically range from WORD_SIZE to large sizes (512, 1024, 2048, etc.)

**total avbl (available):**
Total bytes available in the large pool's free block lists. This memory can be allocated for large objects using the buddy algorithm.

**total unavbl (unavailable):**
Total bytes currently allocated from the large pool and in use. This represents large objects currently holding data.

**total size:**
Total size of the entire large pool (avbl + unavbl). This is the configured capacity for large allocations.

#### Heap Summary

**total heap size:**
The complete size of the SDR heap (small pool size + large pool size). This is the total configured SDR capacity from your ionconfig file.

**total unused:**
Bytes in the heap that have never been allocated. This is "virgin" space that can be used for either pool as needed. As the system runs, this value decreases as more heap space is put into use.

**max total used:**
The maximum amount of heap space that has ever been in use simultaneously since ION started. This is calculated as: heap size - unused size. This high-water mark indicates peak memory demand.

**total now in use:**
Current amount of heap space actively allocated and in use. Calculated as: heap size - small pool free - large pool free - unused. This should fluctuate as bundles are created, forwarded, and delivered.

**max xn log len (transaction log length):**
The maximum length of the transaction log that has been observed. The transaction log records all modifications during a transaction. If this value grows very large, it may indicate:
- Very large transactions that should be split up
- Inefficient use of transactions
- Potential memory pressure during transaction processing

### Detecting Memory Leaks with sdrwatch

**Normal Operation:**
- "total now in use" fluctuates as bundles arrive, are forwarded, and delivered
- "max total used" increases initially then stabilizes
- "total unused" decreases initially as pools are allocated, then stabilizes
- Free block counts remain reasonable

**Memory Leak Indicators:**
- "total now in use" continuously increases over time without decreasing
- "total unused" continuously decreases
- "max total used" keeps growing toward heap size
- Free block counts decrease toward zero
- Eventually: "Can't allocate heap space" errors in ion.log

**How to Detect Leaks:**
1. Run `sdrwatch ion -t 30 100` to monitor every 30 seconds for 100 iterations
2. Observe the "total now in use" value over time
3. If it grows continuously without dropping, investigate recent code changes
4. Use trace mode with verbose output to see exactly what's being allocated
5. Check ion.log for "unfreed" allocation reports when the trace ends

## psmwatch - PSM Memory Monitoring

### Purpose

`psmwatch` monitors PSM (Private Shared Memory) partitions used by ION for working memory. PSM holds in-memory data structures like lists, databases, and volatile state that doesn't need to be persistent.

### Usage

```bash
# Display current usage for ionwm partition once (no tracing)
psmwatch 0xff01 5000000 ionwm 0 1

# Monitor with trace every 5 seconds for 10 iterations
psmwatch 0xff01 5000000 ionwm 5 10

# Verbose trace showing all allocations
psmwatch 0xff01 5000000 ionwm 5 10 verbose

# Poll without tracing (interval must be negative)
psmwatch 0xff01 5000000 ionwm -10 100
```

### Parameters

| Parameter | Description |
|-----------|-------------|
| `shared_memory_key` | IPC key for the shared memory segment (hex or decimal, typically 0xff01 / 65281) |
| `memory_size` | Size of the shared memory segment (must match .ionconfig wmSize, typically 5000000) |
| `partition_name` | Name of the partition to monitor: `ionwm` (ION working memory) or `sdrwm` (SDR working memory) |
| `interval` | Polling interval in seconds. Use 0 for single poll. Use negative value to disable tracing and just show summaries |
| `count` | Number of polling iterations |
| `verbose` | Optional: enable verbose output showing all allocations |

### Common Partition Names

- **ionwm** - ION working memory, used for volatile ION data structures
- **sdrwm** - SDR working memory, used internally by SDR for heap management

### Output Format

```
-- partition 'ionwm' usage report --
small pool free blocks:
        45 of size         8
        38 of size        16
        22 of size        24
        15 of size        32
       total avbl:    234560
     total unavbl:    512440
       total size:    747000

large pool free blocks:
         5 of order       128
         3 of order       256
         1 of order       512
       total avbl:   2456320
     total unavbl:   1543680
       total size:   4000000

total partition:     5000000
total unused:          253000
```

### Output Fields Explained

#### Small Pool Section

**Small pool free blocks:**
Lists available free blocks in the small pool by size. Each entry shows the count and size of free blocks.
- Format: `count of size bytes`
- Sizes increment by WORD_SIZE (typically 8 bytes)
- Example: `45 of size 8` means 45 free 8-byte blocks available

**total avbl (available):**
Total bytes in the small pool's free lists, immediately available for small allocations. Higher values indicate good availability.

**total unavbl (unavailable):**
Total bytes allocated from the small pool and currently in use by ION data structures.

**total size:**
Total capacity of the small pool (avbl + unavbl).

#### Large Pool Section

**Large pool free blocks:**
Lists available free blocks in the large pool organized by order (power-of-two sizes).
- Format: `count of order bytes`
- Uses buddy system allocation
- Example: `5 of order 128` means 5 free 128-byte blocks

**total avbl (available):**
Total bytes in the large pool's free lists, available for large allocations.

**total unavbl (unavailable):**
Total bytes allocated from the large pool and currently in use.

**total size:**
Total capacity of the large pool (avbl + unavbl).

#### Partition Summary

**total partition:**
The complete size of the PSM partition. This should match the memory_size parameter and the wmSize configuration in .ionconfig.

**total unused:**
Bytes that have never been allocated from this partition. This "virgin" memory can be added to either pool as needed. Decreases over time as memory is first used.

### Understanding Memory States: avbl, unavbl, and unused

**The terminology can be confusing, especially "unavbl" (unavailable). Here's what each state means:**

#### The Three Memory States

1. **"avbl" (available)** - Memory in free block lists, ready to allocate immediately
2. **"unavbl" (unavailable)** - Memory that has been allocated and is actively in use by ION data structures
3. **"unused"** - Virgin memory that has never been allocated to either pool

**Key Point:** "unavbl" means ALLOCATED and IN USE, not "reserved but free". It's "unavailable" because it's already occupied by active data structures.

#### Memory Flow

```
[Unused Space] → [Allocated to Pool] → [Given to Application]
               → [Small/Large Pool]   → [unavbl = in use]
                  (total size)          [avbl = free blocks]
```

When memory is allocated:
- **First allocation**: Comes from "unused" → becomes part of pool "total size" → marked as "unavbl"
- **Subsequent allocation**: Comes from "avbl" (free blocks) → becomes "unavbl"
- **When freed**: Goes from "unavbl" → back to "avbl" (free blocks)

#### Real Example Breakdown

```
-- partition 'ionwm' usage report --
small pool free blocks:
            14 of size         16
             4 of size         24
             6 of size         32
             2 of size         40
            18 of size         48
             3 of size         56
             4 of size         80
             1 of size        184
             1 of size        192
             4 of size        272
       total avbl:       3408      ← Free blocks ready to allocate
     total unavbl:      13232      ← Allocated and currently IN USE
       total size:      16640      ← Total small pool (3408 + 13232)

large pool free blocks:
             1 of order       1024
             1 of order       2048
       total avbl:       5168      ← Free blocks ready to allocate
     total unavbl:     611184      ← Allocated and currently IN USE
       total size:     616352      ← Total large pool (5168 + 611184)

total partition:     50000000      ← Total PSM partition size
total unused:        49364408      ← Never allocated to any pool yet
```

**Interpretation:**
- **Small pool unavbl (13,232 bytes)**: Active ION data structures using small allocations
- **Large pool unavbl (611,184 bytes)**: Active ION data structures using large allocations
- **Unused (49.3 MB)**: Plenty of virgin space left to grow pools as needed
- This is a **healthy system** with lots of headroom!

**Summary:**
- `total size` = `avbl` + `unavbl` (all memory in this pool, whether free or in use)
- `total partition` = `small pool size` + `large pool size` + `unused`
- Rising "unavbl" over time without dropping = memory leak
- Stable or fluctuating "unavbl" = normal operation

### Trace Mode vs Polling Mode

**Trace Mode (positive interval):**
Monitors memory allocations and deallocations, reporting on potential leaks and showing allocation activity. Useful for debugging memory issues. Due to limited tracing memory, it is best used for short-term monitoring only.

**Polling Mode (negative interval):**
Only displays usage summaries at each interval without detailed allocation tracing. More efficient for long-term monitoring with less overhead.

### Detecting PSM Memory Leaks

**Normal Operation:**
- "total unavbl" values fluctuate as data structures are created and freed
- "total unused" decreases initially then stabilizes
- Free block counts remain healthy

**Memory Leak Indicators:**
- "total unavbl" continuously increases without decreasing
- "total unused" steadily decreases toward zero
- Free block counts trending toward zero
- "Can't allocate space" errors in ion.log

**Troubleshooting Steps:**
1. Use trace mode to see allocation patterns: `psmwatch 0xff01 5000000 ionwm 30 50`
2. Compare "total unavbl" values over time
3. Check for "unfreed" allocations in trace output
4. Review recent code changes for missing psm_free() calls
5. Verify that cleanup routines are being called properly

## Monitoring Best Practices - (DRAFT procedure)

### Regular Health Checks

**Daily Monitoring:**
```bash
# Quick daemon status check
ionwatch

# Memory usage snapshot
sdrwatch ion -t 0
psmwatch 0xff01 5000000 ionwm 0 1
```

### Continuous Monitoring

**Production Watchdog Setup:**
```bash
# Monitor daemon changes every 10 seconds, log to ion.log
ionwatch -w 10 -q -l &

# Monitor SDR usage every 5 minutes (300 seconds)
while true; do
    sdrwatch ion -s >> sdr_usage.log
    sleep 300
done &

# Monitor PSM usage every 5 minutes
while true; do
    psmwatch 0xff01 5000000 ionwm -1 1 >> psm_usage.log
    sleep 300
done &
```

### Memory Leak Detection

**Weekly Leak Check:**
```bash
# Run extended memory monitoring for 1 hour
sdrwatch ion -t 300 12 > sdr_trace_$(date +%Y%m%d).log
psmwatch 0xff01 5000000 ionwm 300 12 > psm_trace_$(date +%Y%m%d).log

# Analyze the logs for continuously increasing "total now in use" / "total unavbl"
```

### Performance Monitoring

**During High-Load Testing:**
```bash
# Watch daemon status with high frequency
ionwatch -w 2 -r

# Monitor SDR allocations in real-time
sdrwatch ion -t 5 120

# Monitor PSM with minimal overhead (no tracing)
psmwatch 0xff01 5000000 ionwm -5 120
```

### Integration with System Monitoring

You can parse the output of these tools in monitoring scripts:

```bash
#!/bin/bash
# Example: Alert if any daemon is not running

ionwatch > /tmp/daemon_status.txt

if grep -q "STALE" /tmp/daemon_status.txt; then
    echo "ALERT: Stale daemon detected!"
    cat /tmp/daemon_status.txt | grep "STALE"
    # Send alert notification
fi

if grep -q "Not Started" /tmp/daemon_status.txt | grep -v "CFDP\|DTPC\|BSSP"; then
    echo "WARNING: Required daemon not started!"
    cat /tmp/daemon_status.txt | grep "Not Started"
fi
```

```bash
#!/bin/bash
# Example: Alert if SDR usage exceeds 90%

sdrwatch ion -t 0 > /tmp/sdr_status.txt

heap_size=$(grep "total heap size:" /tmp/sdr_status.txt | awk '{print $4}')
now_in_use=$(grep "total now in use:" /tmp/sdr_status.txt | awk '{print $5}')

usage_percent=$((now_in_use * 100 / heap_size))

if [ $usage_percent -gt 90 ]; then
    echo "ALERT: SDR usage at ${usage_percent}%!"
    # Send alert notification
fi
```

## Example Monitoring Scenarios

### Scenario 1: Basic Health Check

**Goal:** Verify all daemons are running and memory usage is reasonable.

```bash
# Check daemon status
ionwatch

# Check SDR usage
sdrwatch ion -t 0

# Check PSM usage
psmwatch 0xff01 5000000 ionwm 0 1
```

**Expected Results:**
- All required daemons show "Running" status
- SDR "total now in use" is well below heap size
- PSM "total unavbl" is reasonable (not near partition size)

### Scenario 2: Diagnosing a Memory Leak

**Symptoms:** ION runs fine initially but slows down or crashes after hours/days.

**Investigation Steps:**

```bash
# Start extended monitoring
sdrwatch ion -t 600 100 > sdr_leak_test.log &
psmwatch 0xff01 5000000 ionwm 600 100 > psm_leak_test.log &

# Let the system run for several hours while processing traffic

# After monitoring completes, analyze the logs:
grep "total now in use:" sdr_leak_test.log
grep "total unavbl:" psm_leak_test.log
```

**Analysis:**
- If "total now in use" steadily increases without decreases: SDR leak
- If "total unavbl" (small or large pool) steadily increases: PSM leak
- Use verbose trace mode to see what's not being freed
- Check for recent code changes or configuration updates

### Scenario 3: Monitoring During File Transfers

**Goal:** Observe memory usage patterns during large CFDP file transfers.

```bash
# Start monitoring before initiating transfers
ionwatch -w 5 -r &
sdrwatch ion -t 10 360 > cfdp_sdr_monitor.log &

# Initiate file transfers
cfdptest
# ... perform puts/gets ...

# Observe real-time patterns:
# - SDR usage should increase as file data is received
# - SDR usage should decrease as file data is delivered
# - LTP/BP daemons should remain Running
# - CFDP daemon should remain Running
```

### Scenario 4: Detecting Daemon Failures

**Goal:** Set up automated daemon failure detection.

```bash
# Run ionwatch in quiet log mode
ionwatch -w 30 -q -l &

# The -q flag ensures only status changes are logged
# Check ion.log periodically:
tail -f ion.log | grep "status changed"

# Example log entry on failure:
# [i] ionwatch: LTP/ltpclock Running (PID 12346) - status changed from Running
# [i] ionwatch: LTP/ltpclock STALE (PID 12346) - status changed from Running
```

**Response to Failures:**
- Check ion.log for error messages before the daemon crashed
- Attempt to restart the specific daemon or the entire node
- If crashes persist, enable additional debugging and collect core dumps

## Interpreting Memory Trends

### Healthy Memory Patterns

**SDR:**
- Initial increase as bundles arrive and are queued
- Cyclic pattern as bundles are forwarded and delivered
- "total now in use" stays well below heap size
- "total unused" stabilizes after initial operations

**PSM:**
- Relatively stable "total unavbl" during steady-state
- Small fluctuations as data structures are created/destroyed
- Healthy free block counts in both pools

### Warning Signs

**Gradual Memory Growth:**
- Indicates possible slow leak
- May take hours or days to become critical
- Investigate recent code changes

**Rapid Memory Growth:**
- Indicates major leak or misconfiguration
- May exhaust memory in minutes to hours
- Check for runaway bundle generation or reception

**Fragmentation:**
- Many small free blocks but allocation failures
- "total avbl" is high but allocations still fail
- May need to restart ION to defragment
- Consider adjusting heap configuration

**Transaction Log Growth:**
- "max xn log len" keeps increasing
- May indicate transactions that are too large
- Consider breaking up large operations

## Troubleshooting Tips

### STALE Daemons

If `ionwatch` shows STALE daemons:

1. Check ion.log for crash messages
2. Look for segmentation faults or assertion failures
3. Restart ION: `ionstop; ionstart -I host.rc`
4. If problem persists, enable core dumps and collect debugging info

### Memory Allocation Failures

If you see "Can't allocate" errors:

1. Check current usage: `sdrwatch ion -t 0`
2. If near capacity, increase heap size in .ionconfig
3. If well below capacity, possible fragmentation - restart ION
4. If persistent, investigate for memory leaks

### Performance Degradation

If ION becomes slow over time:

1. Monitor memory: Look for leaks reducing available space
2. Check daemon status: Ensure all daemons are Running
3. Review transaction log length: Very long logs slow operations
4. Consider increasing memory if legitimate usage growth

## See Also

- ionwatch(1) - Daemon status monitor man page
- sdrwatch(1) - SDR activity monitor man page
- psmwatch(1) - PSM activity monitor man page
- sdr(3) - SDR API documentation
- psm(3) - PSM API documentation
- ION-Watch-Characters.md - Watch character documentation for real-time monitoring
- ION-Utilities.md - Overview of all ION utility programs
