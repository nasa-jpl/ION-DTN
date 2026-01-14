# QoS Best-Effort to Reliable CLA Fallback Test

## Purpose

This test verifies that bundles marked with `BP_BEST_EFFORT` QoS flag can be
forwarded via reliable CLAs (like TCP) when no unreliable CLA is available.

## Background

Prior to the fix in `bpv7/daemon/bpclm.c`, bundles with only the `BP_BEST_EFFORT`
flag set would fail to find a matching outduct if only reliable CLAs were
available. This resulted in bundles being placed in limbo and effectively dropped.

The fix adds a fallback mechanism: if a bundle requests `BP_BEST_EFFORT` and no
unreliable CLA matches, the code retries with `BP_RELIABLE` to select any
available reliable CLA.

## Network Topology

```
Node 1 (LTP)  ----LTP--->  Node 2 (relay)  ----TCP--->  Node 3 (TCP)
   |                           |                           |
   |  Sends best-effort        |  Only TCP available       |  Receives bundle
   |  bundle via LTP           |  to reach Node 3          |
   |  (unreliable/green)       |                           |
```

- **Node 1**: Source node with LTP outduct to Node 2
- **Node 2**: Relay node with LTP induct from Node 1, TCP outduct to Node 3
- **Node 3**: Destination node with TCP induct only

## Test Procedure

1. Start all three nodes
2. Start `bprecvfile` on Node 3
3. Send a file from Node 1 to Node 3 with best-effort QoS (`0.1.0.1.0`)
4. Verify the file arrives at Node 3

## Expected Results

- **With fix**: Bundle arrives at Node 3 via TCP
- **Without fix**: Bundle gets stuck in limbo at Node 2, never reaches Node 3

## QoS String Format

The `bpsendfile` QoS parameter format is:
```
custody.priority.ordinal.unreliable.critical
```

In this test, we use `0.1.0.1.0`:
- `0` = no custody requested
- `1` = priority level 1 (standard)
- `0` = ordinal 0
- `1` = unreliable (BP_BEST_EFFORT flag set)
- `0` = not critical

## Related Files

- `bpv7/daemon/bpclm.c` - Contains the fix in `getOutduct()` function
- `bpv7/include/bp.h` - QoS flag definitions (`BP_BEST_EFFORT`, `BP_RELIABLE`)

## Running the Test

```bash
cd tests/qos-besteffort-tcp-fallback
./dotest
```

Exit code 0 indicates success, non-zero indicates failure.
