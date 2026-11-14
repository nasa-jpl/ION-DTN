# TCPCLv4 Guide

> **Status:** ION's TCPCLv4 convergence layer adapter is implemented
> entirely in C: the `tcp4cli`/`tcp4clo` ION daemons plus a standalone
> `tcpclv4_agent` process that implements the RFC 9174 protocol,
> including optional TLS 1.3. No Python runtime or external protocol
> package is required. Wire interoperability has been verified in both
> directions against
> [dtn-demo-agent](https://github.com/BrianSipos/dtn-demo-agent), an
> independent TCPCLv4 implementation. The CLA remains gated behind a
> configure flag while it matures toward a fully integrated convergence
> layer.

## Overview

TCPCLv4 (TCP Convergence Layer version 4) implements the IETF RFC 9174
standard for reliable bundle transfer over TCP connections.

**Key Features:**
- RFC 9174 compliant TCPCLv4 protocol, implemented in C
- TLS 1.3 encryption support (contact-header CAN_TLS negotiation per
  RFC 9174 §4)
- Automatic BP-layer fragmentation for large bundles
- Handles bundles of arbitrary size (tested up to 50MB+)
- Default configuration works without manual tuning

**Architecture:** ION's `tcp4clo` (outduct) and `tcp4cli` (induct)
daemons exchange bundle data with the external `tcpclv4_agent` process
through lock-free shared-memory pools, coordinated over Unix-socket
control channels. The agent owns the TCP/TLS connection and the
RFC 9174 session state machine. One agent serves one duct.

```
ION Node A                                           ION Node B
┌─────────────┐                                     ┌─────────────┐
│  Application│                                     │ Application │
└──────┬──────┘                                     └──────▲──────┘
       │                                                   │
┌──────▼──────┐      ┌───────────────┐   ┌───────────────┐│
│   tcp4clo   │─shm─►│ tcpclv4_agent │──►│ tcpclv4_agent ├┤
│   (ION CLA) │      │  (TCPCLv4/TLS)│TCP│  (TCPCLv4/TLS)││
└─────────────┘      └───────────────┘   └───────┬───────┘│
                                                 │  shm ┌─┴──────────┐
                                                 └─────►│  tcp4cli   │
                                                        │  (ION CLA) │
                                                        └────────────┘
```

### Design rationale: isolation from the SDR

The `tcpclv4_agent` runs as a **separate process** that does not attach
to or operate on ION's SDR, and it is deliberately kept that way.
It exchanges bundle bytes with `tcp4clo`/`tcp4cli` only through the
POSIX shared-memory pool and the Unix-socket control channel,
and it does not route the pool's slot-state atomics through ION's
shared-memory atomic abstraction (`ion_ipc_atomic_t`):
it uses bare POSIX shared memory and C11 atomics.

ION's SDR is guarded by a single node-global transaction lock,
so any work done while holding or waiting on the SDR is serialized
across the whole node.
Keeping the agent off the SDR lets its TLS and TCP work —
the handshake, record encryption, segmentation, and socket I/O —
run concurrently with ION's bundle processing instead of queuing
behind that lock.
The ION-side daemons take the SDR only briefly, to copy a bundle into
or out of a pool slot, and release it before any wait on the agent or
the network.
This matters most on ground nodes and other capable hosts, where the
TLS/TCP path is the expensive work and there are cores to spare.

Although the agent, the pool, and the control channel are all part of
ION, this process boundary — and the bare shared memory and C11 atomics
that come with it — is an intentional design choice, not an oversight.
It is what keeps the convergence layer off the SDR's critical path.

## Building ION with TCPCLv4 Support

The TCPCLv4 CLA is **not built by default**. To enable it, pass the
`--enable-tcpclv4-external` flag to `./configure`:

```bash
autoreconf -fi   # if configure is not present
./configure --enable-tcpclv4-external
make
sudo make install
sudo ldconfig
```

This flag enables:
- The `tcp4cli` and `tcp4clo` convergence layer daemons
- The `tcpclv4_agent` executable
- The shared-memory IPC code (`ion_transfer_pool`, `ion_ctrl_channel`) in `libici`
- The `libtcp4cla`, `libtcpclv4_msg`, `libtcpclv4_tls`, and
  `libtcpclv4_session` libraries
- OpenSSL detection for TLS support

**Requires:** OpenSSL development headers (`libssl-dev` on
Debian/Ubuntu, `openssl-devel` on RHEL/Fedora). This is the only
dependency beyond a standard ION build.

## Quick Start

### Running the Two-Node Test

Verify the installation with the basic two-node test:

```bash
cd tests/tcp4-twonodes
./dotest
```

This test:
1. Starts two ION nodes communicating via TCPCLv4
2. Sends a small bundle, then a 6MB bundle (exercising BP fragmentation)
3. Verifies successful delivery, XFER_SUCCESS feedback, and SDR cleanup
4. Expected result: "SUCCESS" messages and exit code 0

### Running the TLS Test

Test TCPCLv4 with TLS encryption:

```bash
cd tests/tcp4-twonodes-tls
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
1. **BP configuration** - Standard ION BP setup with the TCP4 protocol
2. **One `tcpclv4_agent` per duct** - launched with command-line options

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

### Starting the Agent

Start one agent per duct after (or before — the agent waits) the ION
node is up:

**Node that initiates connections (active):**
```bash
tcpclv4_agent --duct peer3 --node-id ipn:2.0 --connect 10.0.1.3:4556 \
    > agent.log 2>&1 &
```

**Node that accepts connections (passive):**
```bash
tcpclv4_agent --duct peer2 --node-id ipn:3.0 --listen 0.0.0.0:4556 \
    > agent.log 2>&1 &
```

The `--duct` name must match the BP induct/outduct name; the agent
derives the shared-memory pool names (`/ion_tcp4_tx_<duct>`,
`/ion_tcp4_rx_<duct>`) and control socket paths
(`/tmp/ion_tcp4/tx_<duct>.sock`, `rx_<duct>.sock`) from it. The agent
waits up to 30 seconds for the ION daemons to create these, so start
order is flexible; an active-side agent retries its peer connection
every 5 seconds.

Additional options: `--keepalive <seconds>` proposes a session
keepalive interval (RFC 9174 negotiates the minimum of both proposals).

### TLS Configuration (Optional)

To enable TLS, pass certificate paths on the agent command line:

```bash
tcpclv4_agent --duct peer2 --node-id ipn:3.0 --listen 0.0.0.0:4556 \
    --tls-ca /path/to/ca.crt \
    --tls-cert /path/to/node-chain.pem \
    --tls-key /path/to/node.key
```

Per RFC 9174, contact headers are exchanged in plaintext first; the TLS
handshake occurs only when both sides advertise CAN_TLS. If TLS is
configured locally but the peer cannot do TLS, the session is refused
(configured TLS is required TLS). `--tls-noverify` disables peer
certificate verification for testing.

See `tests/tcp4-twonodes-tls/gen_test_pki.sh` for an example
certificate generation script.

## Bundle Size Handling

### Default Configuration

The TCP4 CLA is configured to handle bundles of any size automatically:

- **Default slot size**: 1 MB (configurable at compile time)
- **Automatic fragmentation**: Enabled by default
- **Maximum bundle size**: Limited only by available memory
- **Tested range**: Up to 50MB+ bundles

**No manual configuration is required** - the system automatically
fragments large bundles into slot-sized pieces for transmission.

### How It Works

When you send a bundle larger than the slot size:

1. ION's Bundle Protocol automatically fragments the bundle
2. Each fragment is transmitted independently via TCPCLv4
3. The receiver reassembles fragments at the final destination
4. The process is transparent to applications

**Example**: With default 1MB slots, a 50MB bundle is automatically
split into ~50 fragments and transmitted successfully.

### Custom Slot Sizes (Optional)

For specialized deployments, you can configure larger slot sizes at
compile time:

```bash
# For frequent large bundles (reduces fragmentation overhead)
./configure --enable-tcpclv4-external --with-tcp4-slot-size=5 --with-tcp4-num-slots=8
make && sudo make install
```

**Memory usage**: Each duct (send/receive pair) uses
`slot_size × num_slots × 2` of shared memory.

| Slot Size | Num Slots | Memory Per Duct | Use Case |
|-----------|-----------|-----------------|----------|
| 1 MB (default) | 8 | 16 MB | General purpose |
| 5 MB | 8 | 80 MB | Large bundles (10-100 MB) |
| 10 MB | 4 | 80 MB | Bulk transfers |

### Runtime slot configuration (without rebuilding)

The compile-time defaults can be overridden at run time — per node,
without recompiling — with two environment variables read by `tcp4clo`
and `tcp4cli` when they start:

| Variable | Meaning | Default |
|----------|---------|---------|
| `TCP4_POOL_SLOTS` | number of slots per pool (min 2) | compile-time `--with-tcp4-num-slots` (8) |
| `TCP4_POOL_SLOT_BYTES` | data bytes per slot (min 65536) | compile-time `--with-tcp4-slot-size` (1 MB) |

Set them in the environment that launches ION, e.g.:

```bash
export TCP4_POOL_SLOTS=4
export TCP4_POOL_SLOT_BYTES=524288   # 512 KB slots
```

The daemons log the resulting geometry (`tcp4clo TX pool ...: N slots x M
bytes`), and the auto-configured `maxPayloadLen` tracks the *runtime*
slot size, so BP fragments to fit. Environment variables are used rather
than CLA command-line arguments because ION appends the duct name to a
CLA's argument list. (These are process-wide, so all `tcp4` ducts on a
node share the same geometry.)

### Memory discipline

The convergence layer is designed for the flight-software memory model —
fixed, up-front allocation rather than a churning heap:

- **Shared-memory pool**: a single `mmap`'d region per pool, sized once
  at daemon start (`num_slots × slot_size`); no per-bundle allocation and
  no runtime growth. It is a POSIX shared-memory region, separate from
  ION's SDR/PSM budget, so account for it separately.
- **Agent session buffers**: the external `tcpclv4_agent` bounds its
  session transfer/segment MRUs to the attached pool's slot capacity
  (rather than a large default) and **preallocates** its reassembly,
  segment, receive, and output-queue buffers to those maxima when a
  session opens. The data path performs no `realloc`; a transfer that
  would exceed a bound is refused, not grown. Per-session agent memory is
  therefore fixed and derivable from the pool geometry.

### Troubleshooting

**Issue**: Bundles rejected with "exceeds slot capacity" error

**Solution**: The tcp4clo daemon should auto-configure fragmentation.
Check `ion.log` for:
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

# Check the agent
pgrep -f tcpclv4_agent

# Check logs
tail -f ion.log
tail -f agent.log
```

### Common Issues

**Agent fails to start:**
- Check the agent's log (wherever its stderr was redirected)
- "timed out waiting for pool/socket": the ION node's tcp4cli/tcp4clo
  daemons are not running for that duct name
- "bind: Address already in use": another listener owns the port

**Bundles not received:**
1. Verify contact plan is loaded: `ionadmin` → `l contact`
2. Check TCPCLv4 session: look for "session established" in the agent
   log ("(TLS)" suffix confirms an encrypted session)
3. Verify routing plan: `ipnadmin` → `l plan`

**Clean up after failed test:**
```bash
killm                              # Stop ION and clean IPC
rm -f /tmp/ion_tcp4/*.sock         # Remove sockets
rm -f /dev/shm/ion_tcp4_*          # Remove shared memory
```

### Getting Help

For issues or questions:
- Review the system tests in `tests/tcp4-twonodes/` and
  `tests/tcp4-twonodes-tls/`
- Check ION logs and the agent log for detailed error messages
- Consult the design document: `dev-docs/ION_TCPCLv4_Integration_Design.md`

## Development Status and Future Work

**Current Status (July 2026)**: The TCPCLv4 protocol stack — wire
format, transport with TLS, session state machine, and agent — is
implemented in C with no external runtime dependencies beyond OpenSSL.
Library unit tests build under `make check`; two-node system tests
(plain and TLS) run under `tests/`. RFC 9174 wire interoperability has
been verified bidirectionally against dtn-demo-agent.

**Limitations**:
- One TCPCL session per duct (one peer per agent process); additional
  inbound connections are refused gracefully with SESS_TERM reason BUSY
- Session-layer transfer retransmission (XFER_REFUSE retry policies)
  is minimal: refused/failed transfers are reported to BP, which owns
  retransmission policy
- Must be explicitly enabled at build time (`--enable-tcpclv4-external`)

**Future Work**:
- Folding the agent into an ION daemon (removing the separate process
  and build flag) once the external architecture has seen more field
  use
- Multiple concurrent sessions per agent

## Related Documentation

- [ION Quick Start Guide](./ION-Quick-Start-Guide.md) - Basic ION configuration and operation
- [CLA API](./CLA-API.md) - Convergence Layer Adapter API reference
- [ION Design and API Overview](./ION-Design-and-API-Overview.md) - ION architecture overview
- Technical design: `dev-docs/ION_TCPCLv4_Integration_Design.md`
- [RFC 9174](https://www.rfc-editor.org/rfc/rfc9174.html) - TCPCLv4 specification
- [dtn-demo-agent Repository](https://github.com/BrianSipos/dtn-demo-agent) - Independent TCPCLv4 implementation used for interoperability testing
