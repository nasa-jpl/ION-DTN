# bpinspect Operations Test

Automated regression test for bpinspect utility operations including bundle cancellation, suspension, resumption, filtering, and dry-run modes.

## Test Overview

This test validates the following bpinspect operations:
1. **Basic bundle cancellation** - Cancel bundles with confirmation bypass (-n)
2. **Filtered cancellation** - Cancel with destination filter (-t)
3. **Dry-run cancellation** - Preview cancellation without executing (-D)
4. **Multiple bundle cancellation** - Cancel multiple bundles in one operation
5. **Priority-filtered cancellation** - Cancel bundles by priority level (-p)
6. **Bundle suspension** - Suspend bundles and move them to limbo (-u)
7. **Bundle resumption** - Resume suspended bundles from limbo (-R)
8. **Filtered suspension** - Suspend with destination filter
9. **Dry-run suspend/resume** - Preview operations without executing

## Network Topology

- **Node 1 (ipn:1.0)**: Test source node (LTP port 1111)
- **Node 2 (ipn:2.0)**: Test destination node (LTP port 1112)
- Contact: Immediate bidirectional link at 5 KB/s

The test intentionally sends bundles to Node 2 with short TTL (7200s) to create queued bundles for manipulation.

## Prerequisites

The `bpinspect` utility must be built and installed before running this test:

```bash
# Build bpinspect
cd ../../bpv7/i86_64-fedora
make bpinspect

# Install to /usr/local/bin (requires sudo)
sudo make install

# Or install manually
sudo cp bin/bpinspect /usr/local/bin/
```

Verify installation:
```bash
which bpinspect
# Should show: /usr/local/bin/bpinspect
```

## Running the Test

### Automated Test (Recommended)

Run the complete regression test:

```bash
./dotest
```

The test will:
- Start a 2-node ION network
- Execute 9 test scenarios automatically
- Report PASS/FAIL for each test
- Return exit code 0 on success, 1 on failure

### Manual Testing

If you prefer to run tests manually:

```bash
# Start the network
export ION_NODE_LIST_DIR=$PWD
cd node1
./ionstart
cd ../node2
./ionstart
cd ..
sleep 5

# Send test bundles from node1
cd node1
bpsource ipn:2.1 "test data" -t7200

# Test bpinspect operations
bpinspect -l                    # List all bundles
bpinspect -t ipn:2.1 -l         # Filter by destination
bpinspect -t ipn:2.1 -c -n      # Cancel bundles (no confirmation)
bpinspect -t ipn:2.1 -u -n      # Suspend bundles
bpinspect -q limbo -l           # List suspended bundles
bpinspect -q limbo -R -n        # Resume suspended bundles
bpinspect -t ipn:2.1 -c -D      # Dry-run cancel

# Cleanup
cd ..
./cleanup
```

## Test Details

The automated test (`dotest`) executes 9 test scenarios:

### Test 1: Basic Cancel Operation
- Sends 3 bundles to ipn:2.1
- Verifies bundles are queued
- Cancels all bundles with `-c -n` (no confirmation)
- Verifies bundles are removed

### Test 2: Filter + Cancel (Destination Filter)
- Sends bundles to ipn:2.2 and ipn:2.3
- Cancels only bundles to ipn:2.2 using `-t ipn:2.2 -c -n`
- Verifies only ipn:2.2 bundles canceled, ipn:2.3 remains

### Test 3: Dry-run Mode
- Sends bundle to ipn:2.3
- Runs dry-run cancel with `-D -c -n`
- Verifies bundle is NOT canceled (dry-run only shows what would happen)

### Test 4: Multiple Bundle Cancel
- Sends 2 bundles to ipn:2.1
- Cancels all at once
- Verifies both removed

### Test 5: Priority Filter + Cancel
- Sends bulk (priority 0) and standard (priority 1) bundles
- Cancels only bulk priority with `-p 0 -c -n`
- Verifies only bulk bundles canceled

### Test 6: Basic Suspend Operation
- Sends 2 bundles to ipn:2.4
- Suspends them with `-u -n`
- Verifies bundles moved to limbo queue using `-q limbo`

### Test 7: Resume Suspended Bundles
- Resumes bundles from limbo with `-q limbo -R -n`
- Verifies bundles removed from limbo and re-queued for transmission

### Test 8: Filter + Suspend
- Sends bundles to ipn:2.5 and ipn:2.6
- Suspends only ipn:2.5 bundles with `-t ipn:2.5 -u -n`
- Verifies selective suspension

### Test 9: Dry-run Suspend/Resume
- Tests dry-run suspend (bundle NOT moved to limbo)
- Tests dry-run resume (bundle stays in limbo)
- Verifies `-D` flag works for both operations

## Node Directory Structure

- `node1/` - Node 1 (test source, ipn:1.0)
- `node2/` - Node 2 (test destination, ipn:2.0)

## Configuration Details

### Node 1 (Test Source)
- Node number: ipn:1.0
- IPC Key: 11111
- SDR Name: ion1
- LTP Port: 1111 (listen), 1112 (send to node 2)
- 20MB SDR storage

### Node 2 (Test Destination)
- Node number: ipn:2.0
- IPC Key: 22222
- SDR Name: ion2
- LTP Port: 1112 (listen), 1111 (send to node 1)
- 5MB SDR storage
- Contact: Immediate bidirectional link at 5 KB/s

## Expected Output

When the test passes, you should see:

```
=========================================
bpinspect Operations Test
=========================================

Starting 2-node test network...
Verifying nodes...
  Node 1: OK
  Node 2: OK

Test 1: Basic bundle cancellation
  ...
  PASS: All bundles successfully canceled

Test 2: Filter + cancel (destination filter)
  ...
  PASS: Filtered cancel successful (1 bundle to ipn:2.3 remains)

...

=========================================
PASS: All tests passed
=========================================
```

## Troubleshooting

### Test Fails

Check the detailed output to see which test failed. Common issues:

```bash
# Make sure ION is not already running
killm

# Clean up and try again
./cleanup
./dotest
```

### Nodes Won't Start

```bash
# Verify node is running
cd node1
bpadmin
i  # info command
q  # quit

# Check contact plan
ionadmin
l contact
q
```

### Check Logs

```bash
# View node logs
tail -f node1/ion.log
tail -f node2/ion.log
```

## Integration with ION Test Suite

This test is designed to be run by ION's automated test framework. It:
- Returns exit code 0 on success, 1 on failure
- Provides clear PASS/FAIL output
- Cleans up after itself (when run via `cleanup` script)
