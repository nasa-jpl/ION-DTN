# Transaction Reversibility Test Suite

## Overview

This test suite validates ION's transaction reversibility mechanisms, particularly the behavior introduced and optimized in ION 4.1.3. Transaction reversibility is a critical feature that allows ION to recover from transaction failures by rolling back incomplete SDR (Simple Data Recorder) modifications.

## Background

### What is Transaction Reversibility?

Transaction reversibility is ION's ability to recover from incomplete transactions by:
1. Detecting transaction failures (e.g., crashes, resource exhaustion)
2. Rolling back any SDR modifications made during the failed transaction
3. Restoring the SDR to a consistent state
4. Reloading volatile state information
5. Allowing normal operations to resume

### ION 4.1.3 Changes

ION 4.1.3 introduced an important optimization:
- **Before 4.1.3**: Transaction reversal always occurred when a transaction was cancelled with reversibility enabled
- **4.1.3+**: Transaction reversal only occurs when the SDR was actually modified (`sdr->modified == true`)
- **Benefit**: More efficient transaction cancellation for read-only operations

This optimization means:
- Transactions that only read data can be cancelled quickly without reversal overhead
- Transactions that modify data still trigger full reversal and recovery
- The system can distinguish between these two cases automatically

## Test Suite Structure

### Test 1: reversibilityCheck1 - SDR Exhaustion with Modification
**Status**: ENABLED
**Purpose**: Test transaction reversal when SDR space is exhausted during a modified transaction

**What it tests:**
- Single-node scenario with unreachable destination
- SDR space exhaustion forces transaction cancellation
- Verification that "Attempting transaction reversal" message appears
- ionrestart execution and volatile state reload
- System recovery and continued operation

**Key validation points:**
- Transaction abort occurs due to SDR exhaustion
- Transaction reversal IS attempted (sdr->modified was true)
- No unrecoverable SDR errors
- System accepts new bundles after recovery

### Test 2: reversibilityCheck2 - Multi-Node Crash Recovery
**Status**: ENABLED
**Purpose**: Test transaction reversal in a two-node scenario using bpcrash_hard

**What it tests:**
- Two-node system (sender and receiver)
- Forced crash on receiver node using bpcrash_hard
- Transaction cancellation with SDR modifications
- Node recovery and daemon restart
- Inter-node bundle transmission after recovery

**Key validation points:**
- bpcrash_hard triggers transaction abort with modifications
- Transaction reversal IS attempted
- No unrecoverable SDR errors
- Receiver node successfully receives new bundles after recovery
- UDP packet losses are tolerated (≥7/10 bundles acceptable)

### Test 3: reversibilityCheck3 - No-Modification Path (NEW)
**Status**: ENABLED
**Purpose**: Test ION 4.1.3+ optimization where reversal is skipped for unmodified transactions

**What it tests:**
- Transaction creation without SDR modifications (read-only)
- Transaction cancellation without reversal
- Verification that "Transaction reversal not necessary" appears
- System continues without restart
- Reversal still works when modifications ARE present

**Key validation points:**
- Transaction abort occurs
- Transaction reversal is NOT attempted (sdr->modified was false)
- No unrecoverable SDR errors
- System continues operating without ionrestart
- Subsequent test with modifications still triggers reversal

**Why this is important:** This test validates the ION 4.1.3+ optimization that improves efficiency for read-only transaction cancellations.

### Test 4: reversibilityCheck4 - sdrmend Direct Test (NEW)
**Status**: ENABLED
**Purpose**: Test the sdrmend utility for manual SDR repair

**What it tests:**
- Creating a scenario requiring SDR repair
- Extracting SDR configuration parameters
- Running sdrmend with correct parameters
- SDR recovery and ION restart
- Normal operations after manual repair

**Key validation points:**
- sdrmend successfully repairs the SDR
- ION restarts successfully after sdrmend
- SDR is operational after repair
- Normal bundle operations work
- sdrmend error handling works correctly

**Why this is important:** sdrmend is the primary user-facing tool for repairing SDR corruption when ION is not running.

## Test Infrastructure

### New Utilities

#### 1. bpcrash_hard
**Location**: `bpv7/test/bpcrash_hard.c`
**Purpose**: Reliable crash simulation for testing

**Improvements over bpcrash:**
- Explicitly modifies the SDR before cancelling transaction
- Guarantees `sdr->modified` flag is set
- Reliably triggers transaction reversal in ION 4.1.3+
- Works consistently across platforms

**Usage:**
```bash
bpcrash_hard
```

#### 2. sdr_test_util
**Location**: `ici/test/sdr_test_util.c`
**Purpose**: Direct SDR transaction testing

**Modes:**
- `test_reversal`: Creates transaction with modifications, cancels it
- `test_no_reversal`: Creates transaction without modifications, cancels it
- `verify_state`: Verifies SDR is in consistent state

**Usage:**
```bash
sdr_test_util test_reversal ion
sdr_test_util test_no_reversal ion
sdr_test_util verify_state ion
```

#### 3. verify_sdr_recovery.sh
**Location**: `tests/req-0022-reversibility/verify_sdr_recovery.sh`
**Purpose**: Common verification functions for all tests

**Functions:**
- `check_transaction_aborted()` - Verify transaction abort occurred
- `check_reversal_attempted()` - Verify reversal was attempted
- `check_reversal_skipped()` - Verify reversal was skipped
- `check_no_unrecoverable_error()` - Verify no fatal errors
- `check_ionrestart_executed()` - Verify ionrestart ran
- `check_sdr_operational()` - Verify SDR functionality
- `verify_recovery_with_reversal()` - Full verification for modified transactions
- `verify_recovery_without_reversal()` - Full verification for unmodified transactions
- `display_log_summary()` - Show key log messages

**Usage:**
```bash
source verify_sdr_recovery.sh
check_reversal_attempted "ion.log"
verify_recovery_with_reversal "ion.log"
```

## Running the Tests

### Run All Reversibility Tests
```bash
cd tests/req-0022-reversibility
for test in reversibilityCheck*; do
    echo "Running $test..."
    cd $test
    ./dotest
    cd ..
done
```

### Run Individual Test
```bash
cd tests/req-0022-reversibility/reversibilityCheck1
./dotest
```

### Expected Output
Each test will display:
- Configuration information
- Phase-by-phase progress
- Verification results for each check
- Summary of what was validated
- PASSED/FAILED status

## Interpreting Results

### Success Indicators

For **reversibilityCheck1** and **reversibilityCheck2**:
- ✓ Transaction aborted message appears
- ✓ "Attempting transaction reversal" message appears
- ✓ No "Unrecoverable SDR error" messages
- ✓ ionrestart executed successfully
- ✓ New bundles accepted after recovery

For **reversibilityCheck3**:
- ✓ Transaction aborted message appears
- ✓ "Transaction reversal not necessary" message appears
- ✓ No "Unrecoverable SDR error" messages
- ✓ System continues without restart
- ✓ Reversal works when modifications are present

For **reversibilityCheck4**:
- ✓ sdrmend exits with code 0
- ✓ SDR file exists at /tmp/ion1.sdr
- ✓ sdr_reload_profile called
- ✓ ION restarts successfully
- ✓ SDR operational after repair

**IMPORTANT:** Test 4 uses `configFlags=15` (IN_DRAM + IN_FILE + REVERSIBLE + BOUNDED) because **sdrmend requires SDR_IN_FILE** (flag 2) to function. Without persistent file storage, sdrmend has nothing to repair.

### Common Failure Modes

1. **SDR Exhaustion Not Achieved** (Test 1)
   - Symptom: "Didn't cause SDR exhaustion" message
   - Cause: Heap size too large or congestion forecasting prevents exhaustion
   - Solution: Decrease heap size in config.ionrc or increase bundle count

2. **Reversal Not Attempted** (Tests 1, 2)
   - Symptom: No "Attempting transaction reversal" message
   - Cause: Transaction had no modifications
   - Solution: Ensure SDR modifications occur before cancellation

3. **Unrecoverable SDR Error**
   - Symptom: "Unrecoverable SDR error" in ion.log
   - Cause: Transaction reversal failed or SDR corruption
   - Solution: Check SDR configuration, ensure reversibility is enabled

4. **Bundles Not Received** (Test 2)
   - Symptom: < 7 bundles received in recovery test
   - Cause: UDP packet loss or system not fully recovered
   - Solution: Check network configuration, increase wait times

5. **ionrestart Not Executing**
   - Symptom: No ionrestart messages in log
   - Cause: restartCmd not configured or reversal not triggered
   - Solution: Verify ionconfig has restartCmd parameter

6. **sdrmend Fails with "Can't open memory region"** (Test 4)
   - Symptom: sdrmend exits with error "Can't open memory region" or "Invalid size or key"
   - Cause: SDR configured without SDR_IN_FILE flag (e.g., configFlags=13)
   - Solution: Change configFlags to include flag 2 (e.g., use configFlags=15)
   - Why: sdrmend requires persistent file storage to repair; memory-only SDRs cannot be repaired after processes stop

## Troubleshooting

### Viewing Detailed Logs
```bash
cd tests/req-0022-reversibility/reversibilityCheck1
./dotest
# After test completes or fails:
cat ion.log | grep -E "Transaction|reversal|ionrestart"
```

### Manual Recovery Test
```bash
# Start ION
ionstart

# Force a crash
bpcrash_hard

# Check logs
grep "Transaction" ion.log
grep "reversal" ion.log

# Verify recovery
ionadmin restart.ionrc
bpadmin restart.bprc
```

### Testing sdrmend Manually
```bash
# IMPORTANT: sdrmend requires SDR_IN_FILE (flag 2) to be enabled!
# If your config uses flag 13 (IN_DRAM only), sdrmend won't work.
# You need flag 15 (IN_DRAM + IN_FILE) or 14 (IN_FILE only).

# Stop ION (without cleanup)
ionadmin <<EOF
.
EOF
bpadmin <<EOF
.
EOF

# Wait for processes to stop
sleep 3

# Kill remaining processes (without deleting SDR)
pkill -TERM rfxclock bpclock ipnfw

# Get SDR configuration from config.ionrc or config.ionconfig
# For configFlags=15: 1 + 2 + 4 + 8 = IN_DRAM + IN_FILE + REVERSIBLE + BOUNDED

# Run sdrmend (example for configFlags=15)
sdrmend ion 15 100000 -1 65536 -1 /tmp

# Restart ION
ionstart
```

**Note:** If sdrmend fails with "Can't open memory region" errors, your SDR is likely configured with `configFlags=13` (IN_DRAM only) which doesn't support sdrmend. Change to `configFlags=15` to enable file-based repair.

## Key Differences from Pre-4.1.3 Tests

| Aspect | Pre-4.1.3 | ION 4.1.3+ |
|--------|-----------|------------|
| **Reversal Trigger** | Always on transaction cancel | Only when sdr->modified == true |
| **Read-only Transactions** | Full reversal overhead | Quick cancellation, no reversal |
| **bpcrash Reliability** | Works consistently | May not trigger reversal |
| **Test Approach** | Assume reversal always occurs | Test both paths (modified/unmodified) |
| **Verification** | Check for recovery | Check for correct reversal decision |

## Configuration Notes

### SDR Configuration Flags

In `config.ionrc` or `config.ionconfig`, the configFlags parameter controls SDR behavior:

**Flag values (can be combined by adding):**
- `1` = SDR_IN_DRAM (SDR in shared memory - fast access)
- `2` = SDR_IN_FILE (SDR persisted to file - fault tolerance)
- `4` = SDR_REVERSIBLE (enable transaction reversal)
- `8` = SDR_BOUNDED (enable object boundary checking)

**Common configurations:**

```
# Configuration 1: Performance-optimized (default)
configFlags 13  # 1 + 4 + 8 = IN_DRAM + REVERSIBLE + BOUNDED
# - Fastest performance (memory-only)
# - Transaction reversal works (via ionrestart)
# - sdrmend NOT useful (no persistent file)
# - Used by Tests 1, 2, 3
```

```
# Configuration 2: Fault-tolerant
configFlags 15  # 1 + 2 + 4 + 8 = IN_DRAM + IN_FILE + REVERSIBLE + BOUNDED
# - Write-through to file (slower but persistent)
# - Survives power loss
# - sdrmend CAN repair the file
# - Used by Test 4
```

```
# Configuration 3: File-based only
configFlags 14  # 2 + 4 + 8 = IN_FILE + REVERSIBLE + BOUNDED
# - Slowest (all I/O goes to file)
# - Maximum persistence
# - sdrmend CAN repair the file
# - Not commonly used
```

### SDR Reversibility Configuration

**For testing transaction reversibility (Tests 1-3):**
- Must include `SDR_REVERSIBLE` (4) flag
- Recommended: `13` (IN_DRAM + REVERSIBLE + BOUNDED)

**For testing sdrmend (Test 4):**
- Must include `SDR_IN_FILE` (2) flag
- Must include `SDR_REVERSIBLE` (4) flag
- Recommended: `15` (IN_DRAM + IN_FILE + REVERSIBLE + BOUNDED)

### Heap Size Considerations

- **Too large**: SDR exhaustion tests may not trigger
- **Too small**: Normal operations may fail
- **Recommended for tests**: 50,000 - 100,000 words
- **Production**: Much larger (depends on workload)

## Future Improvements

### Potential Enhancements
1. Add platform-specific tests (Windows, Solaris)
2. Test with different SDR configurations (file-based, bounded)
3. Add performance benchmarks for reversal operations
4. Test reversal with multiple concurrent transactions
5. Add stress tests with rapid transaction creation/cancellation

### Known Limitations
1. UDP-based tests (Test 2) have inherent unreliability
2. Timing-dependent tests may behave differently under heavy load
3. Platform-specific differences in signal handling
4. Cannot test all possible corruption scenarios

## References

- **sdrmend(1)**: Manual page for sdrmend utility
- **sdr(3)**: SDR library API documentation
- **ionadmin(1)**: ION administration commands
- **ION 4.1.3 Release Notes**: Changes to transaction termination

## Authors

- **Original tests (1 & 2):** Samuel Jero (Ohio University, 2012)
- **Updates:** Jay L. Gao (JPL, 2024)
- **Test suite refactor & new tests (3 & 4):** JPL ION Development Team (November 2024)

## License

Copyright (c) 2012-2024, California Institute of Technology.
All rights reserved.
