# ION Memory Protection

## Overview

ION manages two memory pools that are critical to node operation:

- **SDR heap** -- the persistent Simple Data Recorder used for bundle storage, queues, and protocol state.  Configured via `heapWords` in the `.ionconfig` file.
- **Working memory (WM)** -- the volatile shared-memory partition (PSM) used for runtime data structures such as volatile database objects, semaphores, and ZCO requisition lists.  Configured via `wmSize` in the `.ionconfig` file.

If either pool is exhausted, ION daemons can fail to allocate memory for essential operations, potentially crashing the node or corrupting state.  The **memory protection** feature lets operators reserve a safety margin in each pool.  When free space drops below the configured threshold, ION rejects new bundle origination and acquisition rather than risking exhaustion.

## Configuration

Memory protection thresholds are managed at runtime through `ionadmin`:

```
## Set thresholds: heap 15% free, working memory 20% free
m memprotect 15 20

## Query current thresholds and free-space status
i memprotect

## Disable protection entirely
m memprotect 0 0
```

Each threshold specifies the **minimum percentage of free space** that must remain in the pool.  Valid values are 0 through 50; out-of-range values are clamped.  Setting a threshold to 0 disables protection for that pool.

The default thresholds on a fresh ION node are **10% heap, 10% working memory**.

Thresholds take effect immediately (no restart required) and apply independently per node.

### Querying Status

The `i memprotect` command reports:

```
Memory protection: heap 15%, working memory 20%
Current SDR heap: 92.3% free (OK)
Current working memory: 87.1% free (OK)
```

If a pool's free space is below its threshold the status shows `BREACHED` instead of `OK`.

## What It Protects

Memory protection guards two bundle-processing paths:

### 1. Bundle origination (`bp_send`)

When an application calls `bp_send()`, ION checks both pools before accepting the bundle.  If either pool's free space is below its threshold:

- `bp_send()` returns **0** (not accepted).
- A diagnostic message is written to `ion.log`:
  ```
  [?] Memory protection active, rejecting bp_send
  ```
- The first time a threshold is breached, a one-time warning is logged:
  ```
  [!] ION heap memory protection threshold breached (48% free < 50% threshold). Rejecting new bundles.
  ```
  or
  ```
  [!] ION working memory protection threshold breached (44% free < 50% threshold). Rejecting new bundles.
  ```
- The ADU ZCO is **not consumed** by `bp_send()`.  The caller is responsible for destroying it via `zco_destroy()` or retaining it for a later retry.

### 2. Bundle acquisition (inbound)

When a convergence-layer adapter delivers an inbound bundle, ION checks the same thresholds in `bpBeginAcq()` and `acquireBundle()`.  If either pool is below threshold:

- The acquisition work area is marked **congestive**.
- Subsequent data appends (`bpContinueAcq`) are skipped.
- The partially-received bundle is silently discarded in `acquireBundle()` and tallied under `BP_INDUCT_CONGESTIVE`.
- A threshold-breach message is logged (same format as above).

No error is returned to the convergence-layer adapter; the bundle is simply dropped.  This prevents a flood of inbound traffic from exhausting the node's memory.

### Recovery

When free space rises back above the threshold (e.g., after bundles are delivered, forwarded, or expired), a recovery message is logged:

```
[i] ION heap memory protection recovered (52% free >= 50% threshold). Accepting bundles.
```

After recovery, new bundles are accepted normally with no operator intervention required.

## Behavior Summary

| Condition | Origination (bp_send) | Acquisition (inbound) |
|---|---|---|
| Both pools above threshold | Bundle accepted (returns 1) | Bundle received normally |
| Heap below threshold | Returns 0, ZCO not consumed | Bundle silently discarded |
| Working memory below threshold | Returns 0, ZCO not consumed | Bundle silently discarded |
| Protection disabled (0/0) | No check performed | No check performed |

## Application Error Handling

When `bp_send()` returns 0, applications should:

1. Destroy the ADU ZCO (via `zco_destroy()` inside an SDR transaction) to free the space it occupies.
2. Report the failure to the user.  ION's bundled applications (bpsource, bpdriver, bpsendfile, etc.) print a message to stderr and, where applicable, exit with a non-zero status.
3. Optionally retry the send after a delay, or abort.

Example (C):

```c
if (bp_send(sap, destEid, NULL, ttl, priority,
        NoCustodyRequested, 0, 0, NULL, bundleZco,
        &newBundle) < 1)
{
    fprintf(stderr, "Send rejected -- memory protection may be active.\n");

    /* The ZCO was not consumed; destroy it. */
    CHKERR(sdr_begin_xn(sdr));
    zco_destroy(sdr, bundleZco);
    if (sdr_end_xn(sdr) < 0)
    {
        putErrmsg("Can't destroy ZCO.", NULL);
    }

    return 1;  /* non-zero exit */
}
```

## Relationship to ZCO Occupancy Limits

ION independently enforces **ZCO occupancy limits** that partition the SDR heap into inbound and outbound pools (each approximately 20% of total heap, with 60% reserved for operational overhead).  These limits prevent one traffic direction from starving the other.

Memory protection is a separate, complementary mechanism:

- **ZCO occupancy limits** are a fairness partition -- they cap how much heap each traffic direction can use for ZCO payload data.  When the limit is reached, `ionCreateZco` blocks or returns 0.
- **Memory protection thresholds** are a safety margin -- they monitor **total** free space across all allocations (ZCO payload, bundle metadata, SDR bookkeeping, protocol state) and reject new bundles before the pool is fully exhausted.

Both protections operate simultaneously.  In practice, the ZCO occupancy limit is usually the first constraint reached for any single traffic direction, while memory protection guards against the combined effect of all allocations approaching the pool's capacity.

## Monitoring

Use the existing monitoring tools alongside memory protection:

- `ionadmin` `i memprotect` -- snapshot of thresholds and free-space percentages
- `sdrwatch` -- continuous SDR heap usage monitoring (see [ION Monitoring Guide](ION-Monitoring-Guide.md))
- `psmwatch` -- continuous working memory usage monitoring
- `ion.log` -- all threshold breach and recovery messages are written here

## Configuration Guidance

| Scenario | Recommended thresholds |
|---|---|
| General-purpose node | `m memprotect 10 10` (default) |
| Memory-constrained embedded node | `m memprotect 15 15` or higher |
| High-throughput relay | `m memprotect 5 5` (tighter margin acceptable) |
| Debugging / development | `m memprotect 0 0` (disabled) |

Setting the threshold too high (e.g., 50%) will cause ION to reject bundles even when substantial free space remains.  Setting it too low (e.g., 1%) provides little protection.  The default of 10% is a reasonable starting point for most deployments.
