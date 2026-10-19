# TCPCLv4 Guide

> **Note:** The TCPCLv4 CLA is currently a **demonstration prototype**. It is intended for testing and evaluation purposes and may not be suitable for production deployments.

## Overview

TCPCLv4 (TCP Convergence Layer version 4) is ION's implementation of the IETF RFC 9174 standard for bundle transfer over TCP connections. Unlike the older TCPCLv3 implementation (`tcpcli`/`tcpclo`), TCPCLv4 uses a bridge architecture that connects ION's internal shared-memory interface to an external Python-based TCPCLv4 protocol handler.

The architecture consists of:

1. **tcp4cli/tcp4clo**: ION CLA daemons that communicate via shared memory pools
2. **Bridge Adapter**: Python module that bridges the shared-memory interface to TCPCLv4 sessions
3. **dtn-demo-agent**: External Python package providing the TCPCLv4 protocol implementation

```
ION Node A                                           ION Node B
┌─────────────┐                                     ┌─────────────┐
│   bpsource  │                                     │   bpsink    │
└──────┬──────┘                                     └──────▲──────┘
       │                                                   │
┌──────▼──────┐                                     ┌──────┴──────┐
│   tcp4clo   │                                     │   tcp4cli   │
└──────┬──────┘                                     └──────▲──────┘
       │ (shared memory)                                   │ (shared memory)
┌──────▼──────┐                                     ┌──────┴──────┐
│   Bridge    │◄────────── TCPCLv4 Session ────────►│   Bridge    │
│  (Python)   │                                     │  (Python)   │
└─────────────┘                                     └─────────────┘
```

## Dependencies

### System Packages (Ubuntu/Debian)

The following system packages are required for building the Python dependencies:

```bash
sudo apt-get install -y \
    python3-dev \
    libdbus-1-dev \
    libdbus-glib-1-dev \
    libgirepository1.0-dev \
    libgirepository-2.0-dev \
    libcairo2-dev \
    pkg-config \
    gir1.2-glib-2.0
```

### Python Virtual Environment

It is recommended to use a Python virtual environment:

```bash
python3 -m venv ~/.default-venv
source ~/.default-venv/bin/activate
```

### dtn-demo-agent Package

The `dtn-demo-agent` package provides the TCPCLv4 protocol implementation. Install it from the repository:

```bash
# Clone the repository (if not already present)
git clone https://github.com/BrianSipos/dtn-demo-agent.git ~/dtn-demo-agent

# Install in editable mode
pip install -e ~/dtn-demo-agent
```

Alternatively, if `dtn-demo-agent` is available via pip:

```bash
pip install ietf-dtn-demo-agent
```

### Python Package Dependencies

The `dtn-demo-agent` package installs the following key dependencies:

| Package | Purpose |
|---------|---------|
| `PyYAML` | Configuration file parsing |
| `dbus-python` | D-Bus integration for system services |
| `PyGObject` | GLib main loop integration |
| `cbor2` | CBOR encoding for bundle serialization |
| `cryptography` | TLS support |
| `scapy` | Network packet handling |
| `zeroconf` | Service discovery |

## Verifying the Installation

After installation, verify all dependencies are available:

```bash
source ~/.default-venv/bin/activate
python3 -c "import yaml; print('yaml: OK')"
python3 -c "import dbus; print('dbus: OK')"
python3 -c "import gi; print('gi: OK')"
python3 -c "import tcpcl; print('tcpcl: OK')"
```

All four checks should print "OK".

## Running the Two-Node Test

The `tcp4-twonodes` test demonstrates basic TCPCLv4 operation between two ION nodes:

```bash
source ~/.default-venv/bin/activate
cd tests/tcp4-twonodes
./dotest
```

### What the Test Does

1. Starts two ION nodes (Node 2 and Node 3)
2. Launches `tcp4cli` and `tcp4clo` daemons on each node
3. Starts Bridge adapters connecting the nodes via TCPCLv4
4. Sends a bundle from Node 2 to Node 3 using `bpsource`
5. Verifies receipt at Node 3 using `bpsink`
6. Confirms the source SDR is cleared (XFER_SUCCESS feedback)

## Configuration

### Bridge Configuration (bridge.yaml)

Each node requires a `bridge.yaml` file to configure the TCPCLv4 bridge:

**Active node (initiates connection):**
```yaml
duct_name: "peer3"
node_id: "ipn:2.0"
peer_address: "127.0.0.1"
peer_port: 4556
tls_enable: false
```

**Passive node (listens for connections):**
```yaml
duct_name: "peer2"
node_id: "ipn:3.0"
listen_address: "127.0.0.1"
listen_port: 4556
tls_enable: false
```

### BP Configuration (amroc.bprc)

Configure TCP4 inducts and outducts in the BP configuration:

```
# Add TCP4 protocol
a protocol tcp4 1400 100

# Add induct (receiver)
a induct tcp4 peer2 tcp4cli

# Add outduct (sender)
a outduct tcp4 peer3 tcp4clo
```

### Plan Configuration

Configure the egress plan to use TCP4:

```
a plan ipn:3.0 tcp4/peer3
```

## Bundle Size Handling

### Understanding TCPCLv4 Bundle Sizes

Unlike UDP-based CLAs which are constrained by the UDP MTU (~65KB), **TCPCLv4 supports arbitrarily large bundles**:

- TCPCLv4 operates above TCP, which is a stream protocol
- RFC 9174 defines transfer segmentation (XFER_SEGMENT messages)
- Bundle size is limited only by available memory, not network MTU
- MTU (1500 bytes for Ethernet, 9000 for jumbo frames) only affects TCP socket efficiency

### Shared Memory Slot Size

The TCP4 CLA uses shared memory pools with a default slot size of **1MB** (configurable). This is NOT a bundle size limit - it's a design parameter that determines how BP-layer fragmentation works.

### Large Bundle Support

For bundles larger than the slot size, ION uses **BP-layer fragmentation**:

#### Automatic Fragmentation (Enabled by Default)

The TCP4 CLA automatically configures `maxPayloadLen` to match the slot size, triggering BP fragmentation. When a bundle exceeds the slot size:

1. **bpclm** daemon fragments the bundle into slot-sized pieces
2. Each fragment is a complete BP bundle with its own header
3. Each fragment is transmitted independently via TCP4
4. Receiver reassembles at the final destination

**Example**: With 5MB slots, a 50MB bundle is automatically split into 10 fragments of 5MB each.

#### Configuration

The fragmentation threshold is auto-configured to match the slot size, but can be overridden in `.bprc`:

```bash
# Default: Auto-set to match slot size (recommended)
a outduct tcp4 peer3 'tcp4clo peer3'

# Custom threshold (e.g., 10MB fragments)
a outduct tcp4 peer3 'tcp4clo peer3' 10000000

# Disable fragmentation (0 = unlimited, not recommended)
a outduct tcp4 peer3 'tcp4clo peer3' 0
```

#### Compile-Time Slot Size Configuration

The slot size can be configured at compile time for different deployment scenarios:

```bash
# Recommended for 50MB bundles: 5MB slots
./configure --with-tcp4-slot-size=5 --with-tcp4-num-slots=8
make

# Alternative: 10MB slots with fewer slots
./configure --with-tcp4-slot-size=10 --with-tcp4-num-slots=4
make

# High throughput: 50MB slots (high memory usage)
./configure --with-tcp4-slot-size=50 --with-tcp4-num-slots=8
make
```

**Memory Impact:**

| Configuration | Slot Size | Slots | Memory Per Duct (TX+RX) | Use Case |
|---------------|-----------|-------|-------------------------|----------|
| Default | 1 MB | 8 | 16 MB | Standard traffic |
| Recommended | 5 MB | 8 | 80 MB | 50MB bundles |
| Bulk | 10 MB | 4 | 80 MB | Bulk transfer |
| High-Throughput | 50 MB | 8 | 800 MB | Dedicated links |

### BP Fragmentation vs CL Segmentation

It's important to distinguish between two types of fragmentation:

**BP-Layer Fragmentation** (RFC 9171):
- Splits ONE bundle into MULTIPLE bundles
- Each fragment is a complete bundle with its own headers
- Handled by `bpclm` daemon based on `maxPayloadLen`
- Configurable via `.bprc`
- Reassembled at final destination

**CL-Layer Segmentation** (RFC 9174):
- Splits ONE bundle's transmission into segments
- NOT new bundles - same bundle, chunked on the wire
- Handled automatically by TCPCLv4 agent
- Transparent to ION
- Reassembled immediately by TCPCLv4 receiver

**Example**:
- 50MB bundle with 5MB `maxPayloadLen`:
  - BP layer: Creates 10 fragment bundles (5MB each)
  - CL layer: Each 5MB fragment sent via multiple XFER_SEGMENT messages
  - Total: 10 bundles, each transmitted via ~5000 XFER_SEGMENTs (1KB each)

### Performance Considerations

**5MB Slots vs 1MB Slots:**
- 50MB bundle: 10 fragments vs 50 fragments (80% reduction)
- Less BP overhead, fewer SDR operations
- Memory copy time negligible (0.3ms difference per fragment)
- Network transmission time dominates (same for both)
- **Recommended: 5MB slots** for bundles up to 50MB

**When to use smaller `maxPayloadLen`:**
- Network with frequent disruptions (fragments can take different paths)
- Memory-constrained receivers
- Need for incremental progress tracking

**When to use larger slots:**
- Reliable, dedicated links
- Bulk file transfer deployments
- Minimize BP fragmentation overhead

### Troubleshooting

#### Bundle Rejected: Exceeds Slot Capacity

If you see:
```
[w] tcp4clo: bundle 50000000 bytes exceeds slot capacity 1048576 (1 MB).
```

This should not occur if auto-configuration is working. If it does:

**Solution 1**: Verify auto-configuration (check ion.log for auto-set message):
```bash
grep "Auto-set maxPayloadLen" ion.log
```

**Solution 2**: Manually configure BP fragmentation:
```bash
# In .bprc
c outduct tcp4 peer3 'tcp4clo peer3' 1048576   # Match slot size
```

**Solution 3**: Increase slot size (requires recompilation):
```bash
./configure --with-tcp4-slot-size=5
make && sudo make install
```

#### System Shared Memory Limits

For large slot sizes (50MB+), check system limits:

```bash
# Check current limits
cat /proc/sys/kernel/shmmax  # Max segment size
cat /proc/sys/kernel/shmall  # Total shared memory

# Increase if needed (requires root)
sudo sysctl -w kernel.shmmax=2147483648   # 2GB
sudo sysctl -w kernel.shmall=2097152      # 8GB in 4KB pages
```

For production deployments with large slots, add these to `/etc/sysctl.conf`.

## TLS Support

For TLS-enabled connections, use the `tcp4-twonodes-tls` test as a reference. TLS configuration requires:

1. Generate test PKI certificates using `gen_test_pki.sh`
2. Set `tls_enable: true` in bridge.yaml
3. Configure certificate paths in the bridge configuration

## Troubleshooting

### Bridge fails to start

Check the bridge log file:
```bash
cat bridge.log
```

Common issues:
- Missing Python dependencies
- Incorrect PYTHONPATH
- Port already in use

### Bundles not received

1. Verify both bridges are running: `pgrep -f bridge_adapter`
2. Check ION logs: `tail -f ion.log`
3. Verify TCP4 CLAs are running: `pgrep -f tcp4cli && pgrep -f tcp4clo`

### Shared memory issues

Clean up stale shared memory and sockets:
```bash
rm -f /tmp/ion_tcp4/*.sock
rm -f /dev/shm/ion_tcp4_*
```

## Related Documentation

- [ION Quick Start Guide](./ION-Quick-Start-Guide.md) - Basic ION setup
- [CLA API](./CLA-API.md) - Convergence Layer Adapter API reference
- [ION Design and API Overview](./ION-Design-and-API-Overview.md) - Architecture overview
- [dtn-demo-agent Repository](https://github.com/BrianSipos/dtn-demo-agent) - TCPCLv4 implementation
