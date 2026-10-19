# TCP4 CLA - TCPCLv4 Convergence Layer Adapter

## Overview

The TCP4 CLA provides TCPCLv4 (RFC 9174) support for ION through a bridge architecture:
- **tcp4cli/tcp4clo**: ION daemons using shared memory for IPC
- **Python bridge**: Connects shared memory to external TCPCLv4 agent
- **dtn-demo-agent**: Python implementation of TCPCLv4 protocol

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

### Bridge Connection Issues

If tcp4clo reports "waiting for agent":

```bash
# Verify bridge process
pgrep -f bridge_adapter

# Check bridge log
cat bridge.log

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
Python Bridge (reads from pool)
    ↓
dtn-demo-agent (TCPCLv4 XFER_SEGMENT messages)
    ↓
Network (TCP socket)
```

### Fragmentation vs Segmentation

**BP Fragmentation** (bpclm):
- Creates multiple bundles from one bundle
- Each fragment has BP headers
- Configured via maxPayloadLen

**CL Segmentation** (TCPCLv4):
- Splits one bundle's transmission
- XFER_SEGMENT messages on wire
- Transparent to ION, handled by bridge

**Both occur independently**: A 5MB fragment can be sent via many 1KB XFER_SEGMENTS.

## Current Status

**Date: 2026-03-25**

### Implementation Notes

This branch (`feature-4.2.x-tcpclv4`) implements TCPCLv4 support for ION
through a three-layer bridge architecture that delegates actual TCPCLv4
protocol handling to an external implementation
([dtn-demo-agent](https://github.com/BrianSipos/dtn-demo-agent)).

The architecture uses three IPC mechanisms to interface ION with the
external TCPCLv4 agent:

1. **POSIX Shared Memory Pools (Data Plane)** -- Named regions in
   `/dev/shm/` with lock-free atomic slot state machines
   (`FREE -> WRITING -> READY -> READING -> FREE`). Bundle data is
   copied once from SDR into shared memory via `zco_transmit()`. The
   Python bridge attaches to these pools via ctypes calls into
   `libici.so`, ensuring correct atomic operations without
   reimplementing lock-free logic in Python.

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
   | `CTRL_SLOT_RELEASE` | 0x06 | ION -> Agent | RX slot consumed, now free |
   | `CTRL_SESSION_UP` | 0x10 | Agent -> ION | TCPCL session established |
   | `CTRL_SESSION_DOWN` | 0x11 | Agent -> ION | TCPCL session ended |
   | `CTRL_KEEPALIVE` | 0xFF | Bidirectional | Liveness check (15s interval / 45s timeout) |

**Key design properties of this prototype:**

- **Asynchronous disposition**: `tcp4clo` uses `bpDequeue()` with
  `stewardship=1` and reports xmit success/failure asynchronously from a
  dedicated response thread when the external agent confirms transfer
  outcome.
- **Agent crash recovery**: Both `tcp4cli` and `tcp4clo` detect agent
  disconnection via keepalive timeouts and control socket errors, fail
  all in-flight transfers, reset pool slots, and wait for agent
  reconnect.
- **BP fragmentation for large bundles**: `tcp4clo` auto-sets
  `maxPayloadLen` so bpclm fragments at the BP layer before the CLA
  sees them, avoiding slot-size limits.

### Phase 2 C Libraries (Built, Not Yet Deployed)

Two C libraries implement TCPCLv4 wire format and TLS natively:

- **`tcpclv4_msg.c/h`** (`libtcpclv4_msg.so`) -- Stateless encode/decode
  for all RFC 9174 message types (contact header, SESS_INIT, SESS_TERM,
  XFER_SEGMENT, XFER_ACK, XFER_REFUSE, KEEPALIVE, MSG_REJECT).
- **`tcpclv4_tls.c/h`** (`libtcpclv4_tls.so`) -- OpenSSL-based TLS
  session management (connect, accept, message-framed send/recv,
  non-blocking mode).

These libraries are compiled and have unit tests (`tests/test_msg_roundtrip`,
`tests/test_tls_loopback`) but are **not yet integrated** into the bridge
path. The current prototype still uses the Python/dtn-demo-agent path for
all TCPCLv4 protocol operations.

### Project Direction

This prototype may represent the end-state architecture for demonstrating
ION interfacing with an external TCPCLv4 implementation. The shared-memory
pool and control-channel IPC boundary cleanly separates ION's bundle
processing from TCPCLv4 session management, allowing the external agent to
be developed, tested, and updated independently.

A potential follow-on phase would convert the Python bridge layer to C
(leveraging the Phase 2 `libtcpclv4_msg` and `libtcpclv4_tls` libraries)
for improved performance, while retaining TCPCLv4 as an external module
that communicates with ION through the same shared-memory / control-channel
interface.

## Files

| File | Purpose |
|------|---------|
| tcp4cla.h | Common header, defines slot size |
| tcp4cli.c | Induct daemon (RX) |
| tcp4clo.c | Outduct daemon (TX) |
| libtcp4cla.c | Shared functions |
| bridge/ | Python bridge adapter |

## References

- [RFC 9174](https://www.rfc-editor.org/rfc/rfc9174.html) - TCPCLv4 specification
- [RFC 9171](https://www.rfc-editor.org/rfc/rfc9171.html) - Bundle Protocol v7
- [TCPCLv4 Guide](../../gh-pages/docs/TCPCLv4-Guide.md) - User documentation
- [dtn-demo-agent](https://github.com/BrianSipos/dtn-demo-agent) - TCPCLv4 implementation
