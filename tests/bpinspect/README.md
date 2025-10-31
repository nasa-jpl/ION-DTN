# bpinspect Test Environment

3-node ION network for testing the bpinspect utility.

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

## Network Topology

- **Node 1 (ipn:1.0)**: Hub node
- **Node 2 (ipn:2.0)**: Immediate contact with Node 1 at 5 KB/s
- **Node 3 (ipn:3.0)**: Delayed contact with Node 1 (+24 hours) at 50 KB/s

Bundles sent from Node 1 to Node 3 will queue for 24 hours, creating a perfect scenario for testing bpinspect filtering, sorting, and cancellation features.

## Quick Start

### 1. Start the Network

```bash
./start-network.sh
```

### 2. Send Test Bundles

**From Node 1 to Node 2 (transmits immediately):**

```bash
cd node1
echo "Test to node 2" | bpsource ipn:1.1 ipn:2.1
```

**From Node 1 to Node 3 (queues for 24 hours):**

```bash
cd node1
echo "Bundle 1 to node 3" | bpsource ipn:1.1 ipn:3.1
echo "Bundle 2 to node 3" | bpsource ipn:1.1 ipn:3.1
echo "Bundle 3 to node 3" | bpsource ipn:1.1 ipn:3.1
```

### 3. Test bpinspect

```bash
cd node1

# List all bundles
bpinspect -l

# Show bundles destined for node 3 with details
bpinspect -t ipn:3. -d 1

# Sort by expiration time
bpinspect -o exp

# Show only bundles in forwarding queue
bpinspect -q fwd

# Dry-run cancel bundles to node 3
bpinspect -t ipn:3.1 -c -D

# Actually cancel bundles to node 3 (with confirmation)
bpinspect -t ipn:3.1 -c
```

### 4. Cleanup

```bash
cd ..  # Return to test directory
./cleanup
```

## Node Directory Structure

Each node has its own subdirectory:
- `node1/` - Node 1 (hub)
- `node2/` - Node 2 (immediate link)
- `node3/` - Node 3 (delayed link)

To interact with a specific node, cd into its directory and run commands.

## Testing Scenarios

### 1. Queue Buildup

Send multiple bundles to node 3 to build up the queue:

```bash
cd node1
for i in {1..10}; do
    echo "Bundle $i to node 3" | bpsource ipn:1.1 ipn:3.1
done
```

Verify queue with bpinspect:

```bash
bpinspect -l
# Should see 10 bundles queued
```

### 2. Filtering Tests

Test different filter combinations:

```bash
# Filter by destination
bpinspect -t ipn:3.1 -l

# Filter by source
bpinspect -s ipn:1.1 -l

# Filter by size (bundles > 100 bytes)
bpinspect -m 100 -l

# Filter by expiring within 1 hour
bpinspect -x 3600 -l

# Filter by queue state
bpinspect -q fwd -l
```

### 3. Sorting Tests

Test different sort options:

```bash
# Sort by creation time (newest first)
bpinspect -o time -l

# Sort by expiration (soonest first)
bpinspect -o exp -l

# Sort by size (largest first)
bpinspect -o size -l

# Sort by destination (alphabetical)
bpinspect -o dst -l

# Reverse sort (ascending)
bpinspect -o size -r -l
```

### 4. Detail Levels

Test different verbosity levels:

```bash
# Summary table (default)
bpinspect -l -d 0

# Detailed view
bpinspect -l -d 1

# Full view with extension blocks
bpinspect -l -d 2
```

### 5. Cancellation Tests

Test bundle cancellation:

```bash
# Dry-run first (see what would be canceled)
bpinspect -t ipn:3.1 -c -D

# Cancel with confirmation
bpinspect -t ipn:3.1 -c

# Cancel without confirmation (automated)
bpinspect -t ipn:3.1 -c -n
```

### 6. Export Tests

Export bundle details to file:

```bash
# Export all bundles to node 3
bpinspect -t ipn:3. -e bundles.txt

# Export with detailed info
bpinspect -t ipn:3. -d 2 -e bundles_detailed.txt

# View exported data
cat bundles.txt
```

### 7. Mixed Traffic

Create realistic traffic mix:

```bash
cd node1

# Some bundles to node 2 (immediate)
for i in {1..3}; do
    echo "Immediate bundle $i" | bpsource ipn:1.1 ipn:2.1
done

# Many bundles to node 3 (queued)
for i in {1..20}; do
    echo "Queued bundle $i" | bpsource ipn:1.1 ipn:3.1
done

# Use bpinspect to analyze
bpinspect -l
bpinspect -t ipn:3. -l     # See only queued bundles
bpinspect -q fwd -l        # See forwarding queue
```

## Configuration Details

### Node 1 (Hub)
- IPC Key: 11111
- SDR Name: ion1
- LTP Ports: 1111 (listen), 1112/1113 (send to 2/3)
- 20MB SDR storage

### Node 2 (Immediate Link)
- IPC Key: 22222
- SDR Name: ion2
- LTP Ports: 1112 (listen), 1111 (send to 1)
- 5MB SDR storage
- Contact: Available immediately at 5 KB/s

### Node 3 (Delayed Link)
- IPC Key: 33333
- SDR Name: ion3
- LTP Ports: 1113 (listen), 1111 (send to 1)
- 5MB SDR storage
- Contact: Available in 24 hours at 50 KB/s

## Troubleshooting

### Nodes Won't Start

```bash
# Make sure ION is not already running
killm

# Clean up and try again
./cleanup
./start-network.sh
```

### Can't Send Bundles

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
tail -f node3/ion.log
```

## Future: Automated Regression Test

The `dotest` script (to be created) will automate:
1. Start network
2. Send bundles with known parameters
3. Run bpinspect with various filters
4. Verify output matches expected results
5. Test cancellation functionality
6. Cleanup and return 0 (pass) or 1 (fail)
