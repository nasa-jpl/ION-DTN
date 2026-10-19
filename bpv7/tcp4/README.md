# TCP4 CLA - TCPCLv4 Convergence Layer Adapter

## Overview

The TCP4 CLA provides TCPCLv4 (RFC 9174) support for ION through an
external-agent architecture, implemented entirely in C:
- **tcp4cli/tcp4clo**: ION daemons using shared memory for IPC
- **tcpclv4_agent**: Standalone C agent implementing the TCPCLv4 protocol
  (session state machine, wire format, and optional TLS 1.3)

## Bundle Size Support

The TCP4 CLA supports **arbitrarily large bundles** through automatic BP-layer fragmentation.

### How It Works

1. **Auto-Configuration**: tcp4clo automatically sets `maxPayloadLen` to match slot size
2. **BP Fragmentation**: bpclm fragments bundles larger than slot size
3. **Independent Transmission**: Each fragment transmitted as separate bundle
4. **Reassembly**: Receiver reassembles fragments at final destination

**Example**: With 5MB slots, a 50MB bundle becomes 10 fragments of 5MB each.

## Compile-Time Configuration

### Slot Size Configuration

The shared memory slot size determines the fragmentation threshold:

```bash
# Recommended for 50MB bundles: 5MB slots
./configure --enable-tcpclv4-external --with-tcp4-slot-size=5 --with-tcp4-num-slots=8
make

# Alternative configurations:
./configure --enable-tcpclv4-external --with-tcp4-slot-size=10 --with-tcp4-num-slots=4   # 10MB slots
./configure --enable-tcpclv4-external --with-tcp4-slot-size=50 --with-tcp4-num-slots=8   # 50MB slots
```

### Memory Usage

| Slot Size | Num Slots | Memory Per Duct | Use Case |
|-----------|-----------|-----------------|----------|
| 1 MB (default) | 8 | 16 MB | Standard traffic |
| 5 MB (recommended) | 8 | 80 MB | Bundles up to 50MB |
| 10 MB | 4 | 80 MB | Bulk transfer |
| 50 MB | 8 | 800 MB | Dedicated links |

**Note**: "Per Duct" = TX pool + RX pool

### Performance Impact

**5MB vs 1MB Slots (50MB bundle example):**
- Fragments: 10 vs 50 (80% reduction)
- BP overhead: 500 bytes vs 2500 bytes
- Memory copy: +0.3ms per fragment (negligible)
- **Recommendation**: Use 5MB slots for optimal balance

## Runtime Configuration

### BP Configuration (.bprc)

**Auto-Configuration (Recommended):**

TCP4 CLA automatically configures `maxPayloadLen` based on the compiled slot size:

```bash
a protocol tcp4 1400 100
a outduct tcp4 peer3 'tcp4clo peer3'        # Auto-configures maxPayloadLen
```

When tcp4clo starts, it automatically sets:
```
maxPayloadLen = slot_size - 1024 bytes (BP header margin)
```

This ensures BP fragmentation works correctly without manual configuration. The 1KB margin accounts for BP bundle headers (~90-300 bytes per fragment).

**Verification:**

Check ion.log for the auto-configuration message:
```
[i] tcp4clo: Auto-set maxPayloadLen=5241856 (5 MB - 1024 B margin)
    to enable BP fragmentation for bundles > slot size.
```

**Manual Override (Optional):**

You can manually specify maxPayloadLen if needed:

```bash
a outduct tcp4 peer3 'tcp4clo peer3' 5241856   # Manual: 5MB - 1KB margin
```

**Recommended values by slot size:**
- 1MB slots: `1047552` (1MB - 1KB)
- 5MB slots: `5241856` (5MB - 1KB, default)
- 10MB slots: `10484736` (10MB - 1KB)

**Note:** Always leave margin for BP headers. Setting maxPayloadLen equal to slot_size will cause "exceeds slot capacity" errors.

### System Shared Memory Limits

For large slot sizes, verify system limits:

```bash
# Check limits
cat /proc/sys/kernel/shmmax   # Should be > 100MB per duct
cat /proc/sys/kernel/shmall   # Should support all ducts

# Increase if needed (requires root)
sudo sysctl -w kernel.shmmax=2147483648   # 2GB
sudo sysctl -w kernel.shmall=2097152      # 8GB (in 4KB pages)

# Persist across reboot
echo "kernel.shmmax = 2147483648" | sudo tee -a /etc/sysctl.conf
echo "kernel.shmall = 2097152" | sudo tee -a /etc/sysctl.conf
```

## Testing

### Verify Configuration

After building, check compiled configuration:

```bash
# Check build flags
grep "TCP4CLA_SLOT_SIZE" Makefile

# Should show: -DTCP4CLA_SLOT_SIZE=5242880 (for 5MB)
```

### Test with Large Bundles

```bash
# Start two-node test
cd demos/tcp4-twonodes
./dotest

# Send large bundle (adjust size based on slot configuration)
# For 5MB slots, test with 45MB bundle:
dd if=/dev/zero bs=1M count=45 | bpsource ipn:3.1

# Check fragmentation in ion.log
grep "fragmented" ion.log
```

### Expected Behavior

**With 5MB slots and 50MB bundle:**
1. bpclm creates 10 fragments of 5MB each
2. Each fragment transmitted via tcp4clo
3. No "exceeds slot capacity" errors
4. SDR cleaned up after XFER_SUCCESS for each fragment

## Troubleshooting

### "Bundle exceeds slot capacity" Error

If you see:
```
[w] tcp4clo: bundle 5242970 bytes exceeds slot capacity 5242880 (5 MB)
```

**Cause**: maxPayloadLen was manually set equal to slot_size without margin for BP headers

**Explanation**: BP bundle headers add ~90-300 bytes per fragment. If maxPayloadLen equals the slot size, the resulting fragment (payload + headers) will exceed slot capacity.

**Solution 1**: Remove manual maxPayloadLen and use auto-configuration
```bash
# In .bprc, use:
a outduct tcp4 peer3 'tcp4clo peer3'        # Auto-configures with margin
```

**Solution 2**: Manually set with proper margin
```bash
# For 5MB slots:
a outduct tcp4 peer3 'tcp4clo peer3' 5241856   # 5MB - 1KB margin
```

**Verification**: Check ion.log for auto-configuration:
```bash
grep "Auto-set maxPayloadLen" ion.log
# Should show: Auto-set maxPayloadLen=5241856 (5 MB - 1024 B margin)
```

**Solution 3**: Rebuild with larger slots if you need larger fragments
```bash
./configure --enable-tcpclv4-external --with-tcp4-slot-size=10   # 10MB slots
make && sudo make install
```

### Insufficient Shared Memory

If pool creation fails:

```bash
# Check current usage
ipcs -m

# Clean stale segments
ipcrm -M <key>

# Increase system limits (see above)
```

### Agent Connection Issues

If tcp4clo reports "waiting for agent":

```bash
# Verify agent process
pgrep -f tcpclv4_agent

# Check agent log (wherever its stderr was redirected)
cat agent.log

# Verify shared memory pools exist
ls -lh /dev/shm/ion_tcp4_*
```

## Architecture Details

### Data Flow

```
Bundle (50MB)
    ↓
bpclm (fragments into 10 × 5MB bundles)
    ↓
tcp4clo (extracts each fragment to shared memory)
    ↓
Shared Memory Pool (5MB slots)
    ↓
tcpclv4_agent (reads from pool, emits TCPCLv4 XFER_SEGMENT messages)
    ↓
Network (TCP socket, optionally TLS 1.3)
```

### Fragmentation vs Segmentation

**BP Fragmentation** (bpclm):
- Creates multiple bundles from one bundle
- Each fragment has BP headers
- Configured via maxPayloadLen

**CL Segmentation** (TCPCLv4):
- Splits one bundle's transmission
- XFER_SEGMENT messages on wire
- Transparent to ION, handled by the agent

**Both occur independently**: A 5MB fragment can be sent via many 1KB XFER_SEGMENTS.

## Current Status

**Date: 2026-07-02**

### Implementation Notes

The TCPCLv4 protocol stack is implemented natively in C as three layered
libraries plus a standalone agent executable:

- **`tcpclv4_msg.c/h`** (`libtcpclv4_msg.so`) -- Stateless RFC 9174 wire
  format encode/decode for all message types, plus incremental stream
  framing (`tcpclv4_msg_frame_len`).  Unit-tested against hand-computed
  RFC byte vectors.
- **`tcpclv4_tls.c/h`** (`libtcpclv4_tls.so`) -- Transport layer:
  TCP connect/accept, plaintext contact-header exchange, CAN_TLS
  negotiation, optional TLS 1.3 via OpenSSL, and message-framed recv.
  Follows the RFC 9174 establishment order (contact headers first, in
  plaintext; TLS only if both sides set CAN_TLS).
- **`tcpclv4_session.c/h`** (`libtcpclv4_session.so`) -- RFC 9174 session
  state machine: SESS_INIT negotiation (keepalive = min of proposals,
  MRU exchange), keepalive timers and idle timeout, transfer
  segmentation/reassembly, XFER_ACK completion tracking, XFER_REFUSE,
  and graceful SESS_TERM exchange.  Event-driven and callback-based;
  multiple sessions can share one poll loop.
- **`tcpclv4_agent.c`** (`tcpclv4_agent`) -- Standalone agent binding one
  ION duct's shared-memory pools and control channels to one TCPCL
  session (active `--connect` or passive `--listen` mode, TLS optional).
  No Python and no external protocol implementation required.

The architecture uses three IPC mechanisms to interface ION with the
agent:

1. **POSIX Shared Memory Pools (Data Plane)** -- Named regions in
   `/dev/shm/` with lock-free atomic slot state machines
   (`FREE -> WRITING -> READY -> READING -> FREE`). Bundle data is
   copied once from SDR into shared memory via `zco_transmit()`.
   Slot state transitions have a single owner per direction: the
   producer publishes (`FREE->WRITING->READY`), the consumer releases
   (`READY->READING->FREE`); disposition messages carry no slot
   authority.

2. **Unix Domain Socket Control Channel (Control Plane)** -- TLV-framed
   protocol (14-byte header + 0-256 byte payload, network byte order)
   at `/tmp/ion_tcp4/`. Carries only slot indices, transfer IDs, and
   disposition codes. Bundle data never traverses this channel.

3. **Control Message Protocol** -- Defines the vocabulary between ION
   CLAs and the external agent:

   | Type | Code | Direction | Purpose |
   |------|------|-----------|---------|
   | `CTRL_SLOT_READY` | 0x01 | ION -> Agent | Slot has bundle data to transmit |
   | `CTRL_BUNDLE_RECV` | 0x02 | Agent -> ION | Bundle placed in RX slot |
   | `CTRL_XFER_SUCCESS` | 0x03 | Agent -> ION | Transfer completed OK |
   | `CTRL_XFER_FAILURE` | 0x04 | Agent -> ION | Transfer failed |
   | `CTRL_XFER_REFUSED` | 0x05 | Agent -> ION | Transfer refused by peer |
   | `CTRL_SLOT_RELEASE` | 0x06 | ION -> Agent | RX slot consumed (informational) |
   | `CTRL_SESSION_UP` | 0x10 | Agent -> ION | TCPCL session established |
   | `CTRL_SESSION_DOWN` | 0x11 | Agent -> ION | TCPCL session ended |
   | `CTRL_KEEPALIVE` | 0xFF | Bidirectional | Liveness check (15s interval / 45s timeout) |

**Key design properties:**

- **Asynchronous disposition**: `tcp4clo` uses `bpDequeue()` with
  `stewardship=1` and reports xmit success/failure asynchronously from a
  dedicated response thread when the agent confirms transfer outcome.
- **Agent crash recovery**: Both `tcp4cli` and `tcp4clo` detect agent
  disconnection via keepalive timeouts and control socket errors, fail
  all in-flight transfers, reset pool slots, and wait for agent
  reconnect.
- **BP fragmentation for large bundles**: `tcp4clo` auto-sets
  `maxPayloadLen` so bpclm fragments at the BP layer before the CLA
  sees them, avoiding slot-size limits.
- **Interoperability**: verified bidirectionally against
  [dtn-demo-agent](https://github.com/BrianSipos/dtn-demo-agent), an
  independent TCPCLv4 implementation.

### Running the agent

```bash
# Active (connecting) side:
tcpclv4_agent --duct peer3 --node-id ipn:2.0 --connect 10.0.0.3:4556

# Passive (listening) side:
tcpclv4_agent --duct peer2 --node-id ipn:3.0 --listen 0.0.0.0:4556

# With TLS 1.3:
tcpclv4_agent --duct peer2 --node-id ipn:3.0 --listen 0.0.0.0:4556 \
    --tls-ca ca.crt --tls-cert node3-chain.pem --tls-key node3.key
```

The agent waits up to 30 seconds for the ION daemons' pools and control
sockets to appear, so start order is flexible.  An active-side agent
retries the peer connection every 5 seconds until it succeeds.

### Address families

The duct name is an opaque IPC key,
so the address family is chosen entirely by the agent's endpoint
argument; no ION configuration changes between IPv4 and IPv6.
An IPv6 address must be bracketed.

```bash
# IPv6 peer, and an IPv6 listener:
tcpclv4_agent --duct peer3 --node-id ipn:2.0 --connect [2001:db8::3]:4556
tcpclv4_agent --duct peer2 --node-id ipn:3.0 --listen [::]:4556

# Dual-stack: one listener accepting peers of either family.
tcpclv4_agent --duct peer2 --node-id ipn:3.0 --listen :4556
```

For `--listen`, an empty or `*` host binds every address family the host
provides -- one socket each for IPv4 and IPv6 -- while an explicit
`0.0.0.0` or `[::]` binds only that one.
`IPV6_V6ONLY` is set on every IPv6 socket,
so the two families stay disjoint and can share a port;
an IPv4 peer is never reported as an IPv4-mapped IPv6 address.

A wildcard listen succeeds as long as one family binds,
so a host without IPv6 still runs (the unavailable family is logged).
An explicit address that cannot be bound is a startup failure.

### Tests

- `tcpclv4_test_msg`, `tcpclv4_test_tls`, `tcpclv4_test_session` --
  library unit tests, built by `make check` when configured with
  `--enable-tcpclv4-external`.
- `tests/tcp4-twonodes` and `tests/tcp4-twonodes-tls` -- two-node
  system tests (plain TCP and TLS 1.3), each covering a small bundle
  and a 6MB bundle that exercises BP fragmentation.  They skip
  themselves when `tcpclv4_agent` is not installed.

## Files

| File | Purpose |
|------|---------|
| tcp4cla.h | Common header, defines slot size |
| tcp4cli.c | Induct daemon (RX) |
| tcp4clo.c | Outduct daemon (TX) |
| libtcp4cla.c | Shared functions |
| tcpclv4_msg.c/h | RFC 9174 wire format + framing |
| tcpclv4_tls.c/h | Transport: contact exchange, TLS |
| tcpclv4_session.c/h | RFC 9174 session state machine |
| tcpclv4_agent.c | Standalone C agent executable |

## References

- [RFC 9174](https://www.rfc-editor.org/rfc/rfc9174.html) - TCPCLv4 specification
- [RFC 9171](https://www.rfc-editor.org/rfc/rfc9171.html) - Bundle Protocol v7
- [TCPCLv4 Guide](../../gh-pages/docs/TCPCLv4-Guide.md) - User documentation
- [dtn-demo-agent](https://github.com/BrianSipos/dtn-demo-agent) - Independent TCPCLv4 implementation used for interop testing
- [dtn-demo-agent](https://github.com/BrianSipos/dtn-demo-agent) - TCPCLv4 implementation
