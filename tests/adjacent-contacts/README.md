# Adjacent Contacts Regression Test

## Purpose

This test demonstrates ION's ability to handle adjacent contacts - contacts between the same node pair that share an exact time boundary (contact1.toTime == contact2.fromTime).

## Background

Previously, ION would fail or behave incorrectly when adjacent contacts were configured because the `maxClockError` adjustment would cause temporal reversal:
- Contact 1 stopXmit/stopFire would be delayed beyond the boundary
- Contact 2 startXmit/startFire would be delayed beyond the boundary
- This could cause gaps or reversed event ordering (startRecv > stopRecv)

## Solution

The code now:
1. Detects adjacent contacts in [rfx.c](../../ici/library/rfx.c) using `findAdjacentContact()`
2. Avoids applying `maxClockError` at shared boundaries in transmission/firing events
3. Skips applying `maxClockError` for reception events when adjacent contacts are detected
4. In [rfxclock.c](../../ici/daemon/rfxclock.c), looks ahead to skip setting rates to 0 when adjacent events exist:
   - IonStopXmit skips if adjacent IonStartXmit found
   - IonStopFire skips if adjacent IonStartFire found
   - IonStopRecv skips if adjacent IonStartRecv found
5. Ensures seamless transitions with no gaps or LTP segment rejections

## Test Structure

Following the `demos/bench-ltp` pattern, this test uses a proper node directory structure:

```
tests/adjacent-contacts/
├── 1.adjacent/              # Node 1 directory
│   ├── adjacent.ionconfig   # SDR configuration (wmKey=65281, sdrName=ion1)
│   ├── adjacent.ionrc       # Local ION config
│   ├── adjacent.ionsecrc    # Security config
│   ├── adjacent.ltprc       # LTP config with span to node 2
│   ├── adjacent.bprc        # Bundle Protocol config with LTP induct/outduct
│   ├── adjacent.ipnrc       # IPN routing config (plan to node 2)
│   └── ionstart             # Startup script
├── 2.adjacent/              # Node 2 directory
│   ├── adjacent.ionconfig   # SDR configuration (wmKey=65282, sdrName=ion2)
│   ├── adjacent.ionrc       # Local ION config
│   ├── adjacent.ionsecrc    # Security config
│   ├── adjacent.ltprc       # LTP config with span to node 1
│   ├── adjacent.bprc        # Bundle Protocol config with LTP induct/outduct
│   ├── adjacent.ipnrc       # IPN routing config (plan to node 1)
│   └── ionstart             # Startup script
├── global.ionrc             # Shared contact plan (adjacent contacts defined here)
├── dotest                   # 2-node bundle transfer test (120 seconds)
├── test_quick.sh            # Quick verification test (deprecated - use dotest)
└── README.md                # This file
```

## Test Configuration

### Two-Node Bundle Transfer Test

The main test runs two ION nodes (1 and 2) connected via UDP/LTP and sends 6000 bundles (10KB each) across adjacent contacts.

**Contact Plan (in `global.ionrc`):**
- Range: OWLT = 1 second between nodes 1 and 2
- Contact 1: Node 1->2, time +10 to +60 (50 seconds), rate 1 MB/s
- Contact 2: Node 1->2, time +60 to +2000 (1940 seconds), rate 2 MB/s **(ADJACENT)**
- Contact 3: Node 2->1, time +10 to +60 (50 seconds), rate 1 MB/s
- Contact 4: Node 2->1, time +60 to +2000 (1940 seconds), rate 2 MB/s **(ADJACENT)**

**LTP Configuration:**
- Node 1: UDP span to node 2 via localhost:1113→localhost:1114
- Node 2: UDP span to node 1 via localhost:1114→localhost:1113
- Segment size: 1400 bytes
- Max sessions: 100 export, 100 import

**Bundle Transfer:**
- Source: ipn:1.1 (Node 1)
- Destination: ipn:2.1 (Node 2)
- Count: 6000 bundles
- Size: 10 KB per bundle
- Send rate: 5.6 Mbps (approximately 70 bundles/sec)
- Total data: ~60 MB transferred over ~85 seconds (spans both contacts)

**Expected Event Sequence (with DEBUG_RFX enabled):**

For outbound (1->2) on Node 1:
1. At t=10: IonStartXmit (1->2) - start first contact
2. At t=60: IonStopXmit (1->2) - end first contact
3. At t=60: IonStartXmit (1->2) - start second contact **(skip message, seamless!)**
4. At t=110: IonStopXmit (1->2) - end second contact

For inbound (2->1) on Node 1:
1. At t=10+OWLT: IonStartRecv (2->1) - start receiving (no maxClockError at boundary)
2. At t=60+OWLT: IonStopRecv (2->1) - would stop, but...
3. At t=60+OWLT: IonStartRecv (2->1) - adjacent contact found **(skip stop, seamless!)**
4. At t=110+OWLT: IonStopRecv (2->1) - end reception

For firing (1->2 inbound) on Node 2:
1. At t=10: IonStartFire (1->2)
2. At t=60: IonStopFire (1->2) - would stop, but...
3. At t=60: IonStartFire (1->2) - adjacent contact found **(skip stop, seamless!)**
4. At t=110: IonStopFire (1->2)

Key behaviors:
- No maxClockError applied at adjacent boundaries
- Stop events skip setting rate to 0 when adjacent Start events exist
- This eliminates the reception gap that would cause LTP segment rejections

## Running the Test

### Prerequisites
- ION must be compiled with `-DDEBUG_RFX=1` to see event debug output

### Compile with DEBUG_RFX:
```bash
make clean
./configure CFLAGS="-DDEBUG_RFX=1 -g -O2"
make
```

### Run the 2-node bundle transfer test:
```bash
cd tests/adjacent-contacts
./dotest
```

This test:
- Starts both ION nodes with proper directory structure
- Verifies adjacent contacts are accepted (no overlap errors)
- Waits for first contact to start (t=10)
- Starts bpcounter on Node 2 (receiving 7000 bundles)
- Starts bpdriver on Node 1 (sending 7000 × 10KB bundles)
- Monitors bundle transfer across both contacts
- Reports success rate (expects ≥99.5% delivery)
- Runs for 120 seconds total

### Expected Output

The test reports bundle transfer progress and results:
```
Adjacent Contacts 2-Node Transfer Test
==========================================

Starting ION nodes...
Node 1 contact plan:
  [contact list showing adjacent contacts at t=60]

No overlap errors - adjacent contacts accepted.

Waiting 20 seconds for first contact...
Starting bpcounter on Node 2 (expecting 7000 bundles)...
Starting bpdriver on Node 1 (sending 7000 bundles)...

Bundle transfer in progress...
  [t+10s] Transfer ongoing...
  [t+20s] Transfer ongoing...
  ...

Test Results
==========================================
Bundles Received: 7000 / 7000
Success Rate: 100.00%
RESULT: PASS (>=99.5% delivery)
```

With DEBUG_RFX enabled in the build, you'll see detailed event logs showing:
- `[RFX] Skipping IonStopRecv (adjacent IonStartRecv found)` at t=60
- `[RFX] Skipping IonStopXmit (adjacent IonStartXmit found)` at t=60
- `[RFX] Skipping IonStopFire (adjacent IonStartFire found)` at t=60

## Validation Criteria

- No "Overlapping contact ignored" errors
- Adjacent contact detection in rfx.c (with DEBUG_RFX)
- Skip messages in rfxclock.c for Stop events (with DEBUG_RFX)
- Bundle delivery rate ≥99.5%
- No LTP segment rejection errors during contact transition
- Seamless rate transitions at boundary (t=60) with no gap

## Key Implementation Details

### ION_NODE_LIST_DIR
Following the bench-ltp pattern, `ION_NODE_LIST_DIR` is set to the parent directory (`tests/adjacent-contacts/`), and the node runs from its subdirectory (`1.adjacent/`). This allows proper isolation of ION instances.

### Startup Sequence
The `ionstart` script in `1.adjacent/` executes:
1. `ionadmin adjacent.ionrc` - Initialize ION with local config
2. `ionadmin ../global.ionrc` - Load shared contact plan
3. `ionsecadmin adjacent.ionsecrc` - Initialize security
4. `ltpadmin adjacent.ltprc` - Initialize LTP
5. `bpadmin adjacent.bprc` - Initialize Bundle Protocol

### Contact Plan Location
The adjacent contact definitions are in `global.ionrc` (shared), not in the node-specific config. This follows the bench-ltp pattern where contact plans are typically shared across nodes.

## Files

- `1.adjacent/` - Node 1 working directory with all config files
- `2.adjacent/` - Node 2 working directory with all config files
- `global.ionrc` - Shared contact plan with adjacent contact definitions
- `dotest` - 2-node bundle transfer test (120 seconds)
- `test_quick.sh` - Quick verification test (deprecated)
- `README.md` - This file

## Implementation Files

The adjacent contact support is implemented in:
- [ici/library/rfx.c](../../ici/library/rfx.c) - `findAdjacentContact()` and boundary detection
- [ici/daemon/rfxclock.c](../../ici/daemon/rfxclock.c) - Lookahead logic for seamless transitions
- [ici/include/platform.h](../../ici/include/platform.h) - DEBUG_RFX macro definition
