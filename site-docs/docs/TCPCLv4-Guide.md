# TCPCLv4 Guide

> **Status:** This is a **demonstration prototype** for interfacing ION with an external TCPCLv4 implementation. It delegates RFC 9174 protocol handling to the [dtn-demo-agent](https://github.com/BrianSipos/dtn-demo-agent) Python package via a shared-memory bridge. An integrated, pure-C TCPCLv4 convergence layer is planned for a future ION release.

## Overview

TCPCLv4 (TCP Convergence Layer version 4) implements the IETF RFC 9174 standard for reliable bundle transfer over TCP connections. This prototype demonstrates ION's ability to interoperate with an external standards-compliant TCPCLv4 implementation.

**Key Features:**
- RFC 9174 compliant TCPCLv4 protocol (via external agent)
- TLS 1.3 encryption support
- Automatic BP-layer fragmentation for large bundles
- Handles bundles of arbitrary size (tested up to 50MB+)
- Default configuration works without manual tuning

**Architecture:** The current implementation uses a Python bridge that connects ION's C convergence layer daemons to an external TCPCLv4 protocol handler ([dtn-demo-agent](https://github.com/BrianSipos/dtn-demo-agent)). An integrated pure-C TCPCLv4 convergence layer is planned for a future ION release.

```
ION Node A                                           ION Node B
┌─────────────┐                                     ┌─────────────┐
│  Application│                                     │ Application │
└──────┬──────┘                                     └──────▲──────┘
       │                                                   │
┌──────▼──────┐                                     ┌──────┴──────┐
│   tcp4clo   │◄────────── TCPCLv4 over TCP ───────►│   tcp4cli   │
│   (ION CLA) │         (Python bridge)             │  (ION CLA)  │
└─────────────┘                                     └─────────────┘
```

## Building ION with TCPCLv4 External CLA Support

The TCPCLv4 external CLA (demonstration prototype) is **not built by default**. To enable it, pass the `--enable-tcpclv4-external` flag to `./configure`:

```bash
autoreconf -fi   # if configure is not present
./configure --enable-tcpclv4-external
make
sudo make install
sudo ldconfig
```

This flag enables:
- The `tcp4cli` and `tcp4clo` convergence layer daemons
- The shared-memory IPC code (`ion_transfer_pool`, `ion_ctrl_channel`) in `libici`
- The `libtcp4cla`, `libtcpclv4_msg`, and `libtcpclv4_tls` libraries
- OpenSSL detection for TLS support

**Requires:** OpenSSL development headers (`libssl-dev` on Debian/Ubuntu, `openssl-devel` on RHEL/Fedora).

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

## Quick Start

### Running the Two-Node Test

Verify the installation with the basic two-node test:

```bash
source ~/.default-venv/bin/activate
cd demos/tcp4-twonodes
./dotest
```

This test:
1. Starts two ION nodes communicating via TCPCLv4
2. Sends a 6MB bundle from Node 2 to Node 3
3. Verifies successful delivery and cleanup
4. Expected result: "SUCCESS" messages and exit code 0

### Running the TLS Test

Test TCPCLv4 with TLS encryption:

```bash
source ~/.default-venv/bin/activate
cd demos/tcp4-twonodes-tls
./dotest
```

This test:
1. Generates test PKI certificates automatically
2. Establishes TLS-encrypted TCPCLv4 sessions (TLS 1.3)
3. Transfers both small and large bundles (6MB with BP fragmentation)
4. Verifies TLS negotiation and bundle integrity

## Basic Configuration

### Overview

TCPCLv4 configuration requires:
1. **bridge.yaml** - TCPCLv4 connection settings (per node)
2. **BP configuration** - Standard ION BP setup with TCP4 protocol

### Bridge Configuration (bridge.yaml)

Create a `bridge.yaml` file for each node:

**Node that initiates connections:**
```yaml
duct_name: "peer3"           # Must match BP outduct/induct name
node_id: "ipn:2.0"           # This node's ID
peer_address: "10.0.1.3"     # Remote node's IP
peer_port: 4556              # Remote node's port
tls_enable: false            # Set to true for TLS
```

**Node that accepts connections:**
```yaml
duct_name: "peer2"           # Must match BP outduct/induct name
node_id: "ipn:3.0"           # This node's ID
listen_address: "0.0.0.0"    # Listen on all interfaces
listen_port: 4556            # Port to listen on
tls_enable: false            # Set to true for TLS
```

### ION BP Configuration

Add TCP4 protocol and ducts to your `.bprc` file:

```
# Add TCP4 protocol (required once)
a protocol tcp4 1400 100

# Add TCP4 induct for receiving
a induct tcp4 peer2 tcp4cli

# Add TCP4 outduct for sending
a outduct tcp4 peer3 tcp4clo

# Route traffic to remote node via TCP4
# In .ipnrc:
a plan 3 tcp4/peer3
```

### TLS Configuration (Optional)

To enable TLS, set `tls_enable: true` and add certificate paths to `bridge.yaml`:

```yaml
tls_enable: true
tls_ca_file: "/path/to/ca.crt"
tls_cert_file: "/path/to/node-chain.pem"
tls_key_file: "/path/to/node.key"
```

See `demos/tcp4-twonodes-tls/gen_test_pki.sh` for an example certificate generation script.

## Bundle Size Handling

### Default Configuration

The TCP4 CLA is configured to handle bundles of any size automatically:

- **Default slot size**: 1 MB (configurable at compile time)
- **Automatic fragmentation**: Enabled by default
- **Maximum bundle size**: Limited only by available memory
- **Tested range**: Up to 50MB+ bundles

**No manual configuration is required** - the system automatically fragments large bundles into slot-sized pieces for transmission.

### How It Works

When you send a bundle larger than the slot size:

1. ION's Bundle Protocol automatically fragments the bundle
2. Each fragment is transmitted independently via TCPCLv4
3. The receiver reassembles fragments at the final destination
4. The process is transparent to applications

**Example**: With default 1MB slots, a 50MB bundle is automatically split into ~50 fragments and transmitted successfully.

### Custom Slot Sizes (Optional)

For specialized deployments, you can configure larger slot sizes at compile time:

```bash
# For frequent large bundles (reduces fragmentation overhead)
./configure --enable-tcpclv4-external --with-tcp4-slot-size=5 --with-tcp4-num-slots=8
make && sudo make install
```

**Memory usage**: Each duct (send/receive pair) uses `slot_size × num_slots × 2` of shared memory.

| Slot Size | Num Slots | Memory Per Duct | Use Case |
|-----------|-----------|-----------------|----------|
| 1 MB (default) | 8 | 16 MB | General purpose |
| 5 MB | 8 | 80 MB | Large bundles (10-100 MB) |
| 10 MB | 4 | 80 MB | Bulk transfers |

### Troubleshooting

**Issue**: Bundles rejected with "exceeds slot capacity" error

**Solution**: The tcp4clo daemon should auto-configure fragmentation. Check `ion.log` for:
```
[i] tcp4clo: Auto-set maxPayloadLen=1047552 (1 MB - 1024 B margin)
```

If this message is missing, manually set fragmentation in `.bprc`:
```
# Match the slot capacity (e.g., 1 MB)
a outduct tcp4 peer3 'tcp4clo peer3' 1048576
```

## Troubleshooting

### Check System Status

Verify all components are running:

```bash
# Check ION is running
pgrep ionadmin

# Check TCP4 CLA daemons
pgrep tcp4cli && pgrep tcp4clo

# Check bridge adapter
pgrep -f bridge_adapter

# Check logs
tail -f ion.log
```

### Common Issues

**Bridge fails to start:**
- Check `bridge.log` in the node directory
- Verify Python dependencies are installed (`python3 -c "import tcpcl"`)
- Ensure port is not already in use

**Bundles not received:**
1. Verify contact plan is loaded: `ionadmin` → `l contact`
2. Check TCPCLv4 session: Look for "TLS secured" or "session up" in `bridge.log`
3. Verify routing plan: `ipnadmin` → `l plan`

**Clean up after failed test:**
```bash
killm                              # Stop ION and clean IPC
rm -f /tmp/ion_tcp4/*.sock         # Remove sockets
rm -f /dev/shm/ion_tcp4_*          # Remove shared memory
```

### Getting Help

For issues or questions:
- Review demo scripts in `demos/tcp4-twonodes/` and `demos/tcp4-twonodes-tls/`
- Check ION logs for detailed error messages
- Consult the design document: `dev-docs/ION_TCPCLv4_Integration_Design.md`

## Development Status and Future Work

**Current Status (February 2026)**: This is a demonstration prototype for interfacing ION with an external TCPCLv4 implementation. It is functional and tested for evaluation purposes, but is not a production-integrated convergence layer.

**Limitations**:
- Requires external Python runtime and dtn-demo-agent package
- Python bridge introduces latency compared to native C convergence layers
- Not recommended for production use without further validation
- Must be explicitly enabled at build time (`--enable-tcpclv4-external`)

**Future Work**:
- An integrated, pure-C TCPCLv4 convergence layer is planned for a future ION release, which would not require the external Python bridge or separate build flag

## Related Documentation

- [ION Quick Start Guide](./ION-Quick-Start-Guide.md) - Basic ION configuration and operation
- [CLA API](./CLA-API.md) - Convergence Layer Adapter API reference
- [ION Design and API Overview](./ION-Design-and-API-Overview.md) - ION architecture overview
- Technical design: `dev-docs/ION_TCPCLv4_Integration_Design.md`
- [RFC 9174](https://www.rfc-editor.org/rfc/rfc9174.html) - TCPCLv4 specification
- [dtn-demo-agent Repository](https://github.com/BrianSipos/dtn-demo-agent) - Reference TCPCLv4 implementation
