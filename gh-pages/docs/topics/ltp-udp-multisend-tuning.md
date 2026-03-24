# LTP UDP Multisend and Performance Tuning

This document explains the `UDP_MULTISEND` feature in ION's LTP link service,
how it interacts with the token bucket rate control algorithm, how to build
with or without it, and how to tune Linux kernel parameters for optimal LTP
throughput.

## Overview

ION's `udplso` (LTP UDP Link Service Output) daemon transmits LTP segments as
UDP datagrams. There are two code paths for sending segments:

| Feature | Single-Send | Multisend (`UDP_MULTISEND`) |
|---|---|---|
| System call | One `sendto()` per segment | One `sendmmsg()` per batch |
| Default on Linux | Yes | No (opt-in) |
| Rate control | Token bucket | Token bucket |
| Best for | Small aggregation limits | Large aggregation limits |

`UDP_MULTISEND` is opt-in via `./configure --enable-ltp-udp-multisend` or by
compiling with `-DUDP_MULTISEND`. Both code paths use the same token bucket
rate control algorithm, which replaced the older per-segment sleep model.

## When Multisend Helps

The `sendmmsg()` system call sends multiple UDP datagrams in a single kernel
transition. The performance benefit comes from amortizing system call overhead
across many segments.

The **batch size** is determined by the LTP span configuration:

```
batchLimit = aggrSizeLimit / maxSegmentSize
```

For example, with `aggrSizeLimit = 1000000` and `maxSegmentSize = 1400`:

```
batchLimit = 1000000 / 1400 = 714 segments per batch
```

This means 714 segments are sent with a single `sendmmsg()` call instead of
714 individual `sendto()` calls.

### When Multisend Has No Advantage

When `batchLimit` computes to 1, each batch contains a single segment. There
is no batching benefit — performance is equivalent to the single-send path.

Example configuration that produces `batchLimit = 1`:

```
a span 3 1000 1000 64000 100000 1 'udplso 10.0.0.2:1113'
#                        ^^^^^  ^^^^^^
#                        maxSeg  aggrSize
#                        64000   100000  →  100000/64000 = 1
```

To get a batching benefit, increase `aggrSizeLimit` relative to
`maxSegmentSize`:

```
a span 3 1000 1000 1400 1000000 1 'udplso 10.0.0.2:1113'
#                  ^^^^  ^^^^^^^
#                  1400  1000000  →  1000000/1400 = 714
```

### Choosing Aggregation Parameters

| Parameter | Effect |
|---|---|
| `maxSegmentSize` | Maximum bytes per LTP segment. Smaller values reduce IP fragmentation but increase per-segment overhead. 1400 avoids fragmentation on typical networks. |
| `aggrSizeLimit` | Maximum bytes aggregated into one LTP block before transmission. Larger values increase batch size (more segments per `sendmmsg`). |
| `aggrTimeLimit` | Maximum seconds to wait before sending a partial block. Set to 1 for interactive traffic. |

A good starting point for high-throughput links:

```
a span <engineId> 1000 1000 1400 1000000 1 'udplso <peer>:<port>'
```

This produces `batchLimit = 714`, giving significant system call reduction.

## Token Bucket Rate Control

Both code paths use a token bucket algorithm for rate control. This replaced
the older per-segment sleep model that was bottlenecked by the OS `nanosleep()`
minimum granularity (typically 50-100+ microseconds on Linux).

**How it works:**

- The bucket holds "tokens" representing bytes the sender is allowed to
  transmit
- Tokens are refilled based on elapsed time and the configured `xmitRate`
  (from the contact plan)
- Sending a batch of segments deducts the byte count from the bucket
- If the bucket is empty (tokens < 0), the sender sleeps for exactly the
  deficit duration
- If the bucket has tokens remaining, the sender continues immediately with
  no sleep

**Key properties:**

- Integer-only arithmetic (no floating point)
- At high rates (e.g. 10 Gbps), the bucket refills faster than segments are
  sent, so the sender never sleeps
- At low rates, the sender sleeps precisely the amount needed to match the
  configured rate
- Burst tolerance is controlled by the bucket size (`UDPLSA_BUFSZ * 2` =
  ~128 KB)

## Building With and Without Multisend

### Default Build (Multisend Disabled)

```bash
./configure
make
sudo make install && sudo ldconfig
```

The default build uses the single-send (`sendto()`) path. No multisend
flags are defined.

### Enabling Multisend

To enable the `sendmmsg()` batching path on Linux:

```bash
./configure --enable-ltp-udp-multisend
make
sudo make install && sudo ldconfig
```

For users building with the development Makefiles (`.dev`) instead of
autotools, pass the flag directly:

```bash
make CFLAGS="-DUDP_MULTISEND"
```

### Segment Size Override

When `UDP_MULTISEND` is enabled, LTP automatically caps `maxSegmentSize`
to `MULTISEND_SEGMENT_SIZE` (default 1450 bytes) if the span's configured
value exceeds it. A memo is logged:

```
[i] Note max segment size reduced to work with UDP sendmmsg()
```

This ensures segments fit within the standard Ethernet MTU (1500 bytes)
without IP fragmentation, since `sendmmsg()` sends many datagrams in one
call and fragmentation of any one would degrade performance.

The span's `.ltprc` `maxSegmentSize` parameter is effectively ignored
when it exceeds 1450. This also affects the batch size calculation:

```
batchLimit = aggrSizeLimit / maxSegmentSize
```

where `maxSegmentSize` is the capped value (1450), not the span config
value.

Users on networks with larger MTU (e.g., jumbo frames) can override the
cap at compile time:

```bash
./configure --enable-ltp-udp-multisend CFLAGS="-DMULTISEND_SEGMENT_SIZE=8800"
```

### Non-Linux Platforms

On non-Linux platforms (macOS, FreeBSD, Solaris),
`UDP_MULTISEND` is automatically disabled regardless of flags, because
`sendmmsg()` is a Linux-specific API. The single-send path with token bucket
rate control is used.

## Linux Kernel Tuning for LTP Performance

When LTP sends data at high rates — especially with the multisend path — the
sender can produce UDP datagrams faster than the receiver application drains
its socket buffer. When the receive buffer fills, the kernel drops incoming
packets silently. LTP detects the loss and retransmits, but retransmission
recovery is much slower than initial transmission.

### Diagnosing UDP Buffer Overflow

Symptoms:

- An initial burst of bundles is delivered quickly, then throughput drops
  dramatically
- `ion.log` shows LTP retransmission activity
- `netstat -su` or `cat /proc/net/snmp` shows increasing UDP receive errors

Check current UDP receive errors:

```bash
cat /proc/net/snmp | grep Udp:
# Look at RcvbufErrors column — nonzero means packets were dropped
```

### Recommended sysctl Settings

The following settings increase the UDP socket buffer limits. Apply them on
**both the sending and receiving nodes**:

```bash
# Set temporarily (lost on reboot):
sudo sysctl -w net.core.rmem_max=16777216
sudo sysctl -w net.core.wmem_max=16777216
sudo sysctl -w net.core.rmem_default=8388608
sudo sysctl -w net.core.wmem_default=8388608
```

To make the settings persistent across reboots, add them to
`/etc/sysctl.conf` or a file in `/etc/sysctl.d/`:

```bash
# /etc/sysctl.d/90-ion-ltp.conf
net.core.rmem_max=16777216
net.core.wmem_max=16777216
net.core.rmem_default=8388608
net.core.wmem_default=8388608
```

Then apply:

```bash
sudo sysctl -p /etc/sysctl.d/90-ion-ltp.conf
```

A convenience script is available in the ION repository for applying these settings:

`ltp-sysctl-tuning.sh`

Please review the script before running it, and adjust the values if you have specific requirements or constraints.

### What Each Parameter Does

| Parameter | Default | Recommended | Purpose |
|---|---|---|---|
| `net.core.rmem_max` | 212992 | 16777216 (16 MB) | Maximum receive buffer size a socket can request |
| `net.core.wmem_max` | 212992 | 16777216 (16 MB) | Maximum send buffer size a socket can request |
| `net.core.rmem_default` | 212992 | 8388608 (8 MB) | Default receive buffer size for new sockets |
| `net.core.wmem_default` | 212992 | 8388608 (8 MB) | Default send buffer size for new sockets |

The `_max` values set the upper bound; the `_default` values set the size
assigned to sockets that don't explicitly request a larger buffer. ION's
`udplsi` does not currently set `SO_RCVBUF`, so it uses `rmem_default`.

### Sizing the Buffers

The receive buffer must be large enough to absorb a burst of segments while
the receiver processes them. A rough guideline:

```
buffer_size >= aggrSizeLimit * max_concurrent_blocks
```

For example, with `aggrSizeLimit = 1000000` and up to 8 blocks in flight:

```
buffer_size >= 1000000 * 8 = 8 MB
```

The 16 MB `rmem_max` / 8 MB `rmem_default` recommendations above provide
headroom for most configurations.

### Verifying the Settings

After applying the sysctl values, verify they took effect:

```bash
sysctl net.core.rmem_max net.core.wmem_max \
       net.core.rmem_default net.core.wmem_default
```

To check the actual buffer size of a running `udplsi` socket:

```bash
# Find udplsi's socket
ss -unlp | grep udplsi
# The Recv-Q column shows current usage; check SO_RCVBUF with:
cat /proc/$(pidof udplsi)/fdinfo/<fd_number> | grep rcvbuf
```

## Performance Comparison

The following results are from the `bench.ltpv6` test (10,000 bundles of
1 KB each over IPv6 loopback) on a Linux system:

| Configuration | Time | Throughput | Notes |
|---|---|---|---|
| Old `applyRateControl` (pre-4.1.4) | ~45-60s | ~1-2 Mbps | Per-segment `nanosleep` bottleneck |
| Token bucket, default sysctl | ~23s | ~3.5 Mbps | Fast burst, then retransmission recovery |
| Token bucket, tuned sysctl | ~1.7s | ~48 Mbps | No packet loss, full-speed delivery |

The tuned sysctl configuration eliminates UDP buffer overflow, allowing the
token bucket and `sendmmsg()` to operate at their full potential.

## Configuring LTP for High Throughput

This section provides guidance on choosing LTP span parameters for
high-throughput links. The principles apply regardless of whether
multisend is enabled or not. See also the
[LTP Configuration Tool](../Using-LTP-Config-Tool.md) documentation and
[Contact Graph Events and LTP](../Contact-Graph-Events-and-LTP.md) for
additional background.

### LTP Span Parameter Reference

An LTP span is configured in `.ltprc` with the `a span` command:

```
a span <engineId> <maxExportSessions> <maxImportSessions> \
       <maxSegmentSize> <aggrSizeLimit> <aggrTimeLimit> \
       '<LSO_command>'
```

The parameters most relevant to throughput are:

| Parameter | Description |
|---|---|
| `maxExportSessions` | Maximum concurrent outbound LTP sessions (blocks in flight). Controls flow: no new data can transmit until a session slot is available. |
| `maxImportSessions` | Maximum concurrent inbound sessions. Set to the remote engine's `maxExportSessions`. |
| `maxSegmentSize` | Maximum bytes per LTP segment. Determines how blocks are divided for transmission. |
| `aggrSizeLimit` | Byte threshold for block aggregation. LTP concatenates service data units (bundles) into a block until this limit is exceeded, then segments and transmits the block. |
| `aggrTimeLimit` | Seconds to wait before transmitting a partially filled block. Prevents data from waiting indefinitely when traffic is light. |

### How Aggregation Works

LTP aggregates incoming service data units (bundles) into a single block.
Aggregation ends when either condition is met:

1. The accumulated data exceeds `aggrSizeLimit` (size-limited)
2. The oldest data has waited longer than `aggrTimeLimit` seconds
   (time-limited)

**Size-limited aggregation** is the desired mode for high throughput. It
produces consistently sized blocks, which means a predictable number of
segments per block and predictable session behavior.

**Time-limited aggregation** occurs when the data arrival rate is too low
to fill a block before the time limit expires. The resulting blocks are
smaller than `aggrSizeLimit`, which increases the block rate (more
sessions per second) and protocol overhead.

To check which mode will dominate, compare the data rate to the
aggregation threshold rate:

```
threshold_rate = aggrSizeLimit / aggrTimeLimit
```

If the actual data rate exceeds `threshold_rate`, aggregation will be
size-limited. If not, it will be time-limited and you should consider
reducing `aggrSizeLimit` or accepting the overhead.

### Sizing maxExportSessions (Bandwidth-Delay Product)

`maxExportSessions` is the most important parameter for link utilization.
It controls how many LTP blocks can be in flight simultaneously. If this
value is too low, the sender runs out of session slots and idles while
waiting for acknowledgments from the remote engine.

The minimum value to fully utilize the link is derived from the
**bandwidth-delay product**:

```
max_data_in_transit = xmitRate * 2 * OWLT
maxExportSessions >= max_data_in_transit / mean_block_size
```

Where:

- `xmitRate` is the transmission rate (bytes/sec) from the contact plan
- `OWLT` is the one-way light time in seconds
- `mean_block_size` approximates `aggrSizeLimit` when size-limited

**Example — terrestrial link (OWLT = 0.01s, 10 Mbps):**

```
max_data_in_transit = 1,250,000 * 2 * 0.01 = 25,000 bytes
mean_block_size = 1,000,000
maxExportSessions >= 25,000 / 1,000,000 = 1
```

For low-latency terrestrial links, even 1 session provides full
utilization. Use a modest value (e.g., 100) for headroom.

**Example — deep-space link (OWLT = 600s, 1 Mbps):**

```
max_data_in_transit = 125,000 * 2 * 600 = 150,000,000 bytes
mean_block_size = 1,000,000
maxExportSessions >= 150,000,000 / 1,000,000 = 150
```

Here, 150 export sessions are needed to keep the link saturated. Setting
it lower means the link sits idle while the sender waits for
acknowledgments to free session slots.

**Tradeoff:** Each open export session consumes memory (SDR heap space
for block data, segment metadata, and retransmission state). Higher
values improve utilization but require more `heapWords` in `.ionconfig`.

### Choosing maxSegmentSize

`maxSegmentSize` determines how each LTP block is divided into segments
for transmission. Each segment becomes one UDP datagram (when using
`udplso`).

**Avoiding IP fragmentation** is the primary concern. If a UDP datagram
exceeds the path MTU, IP fragmentation occurs, which increases loss
probability (losing any fragment loses the entire datagram) and wastes
bandwidth on retransmission.

| Network | Typical MTU | Recommended maxSegmentSize |
|---|---|---|
| Ethernet | 1500 bytes | 1400 (leaves room for IP+UDP headers) |
| Jumbo frames | 9000 bytes | 8800 |
| Space link (CCSDS frames) | Frame-dependent | Match to frame payload size |

**Impact on multisend batch size:**

```
batchLimit = aggrSizeLimit / maxSegmentSize
```

Smaller `maxSegmentSize` produces more segments per block and larger
batches, increasing the benefit of `sendmmsg()`. However, each segment
carries fixed overhead (LTP header, UDP header, IP header), so very small
segments reduce payload efficiency.

A `maxSegmentSize` of 1400 is a good default for standard Ethernet.

### Choosing aggrSizeLimit

`aggrSizeLimit` controls the nominal LTP block size. Larger blocks mean:

- Fewer blocks per second (lower session overhead)
- More segments per block (larger multisend batches)
- Less report segment traffic on the return channel
- Higher risk of retransmission per block (a single lost segment
  triggers a report for the entire block)

Smaller blocks mean:

- More blocks per second (higher session overhead)
- More report traffic on the return channel
- Better delivery efficiency (smaller blocks are less likely to
  contain errors)
- Less data retransmitted when errors occur

**Finding the optimal block size:** There is a tradeoff between overhead
and retransmission cost. Very small blocks increase per-block overhead
(more sessions, more report traffic), reducing throughput. Very large
blocks increase the probability that at least one segment is lost,
triggering retransmission of potentially large amounts of data and
adding delay. The optimal `aggrSizeLimit` balances these two effects,
minimizing the combined impact of overhead and retransmission on
throughput and latency. This balance depends on the link's error rate:
lossy links favor smaller blocks, while clean links can afford larger
ones.

**Return channel constraint:** Each block generates at least one report
segment from the receiver. The receiver's return link must support the
resulting report traffic:

```
blocks_per_second = xmitRate / aggrSizeLimit
report_bytes_per_second = blocks_per_second * mean_report_size
```

Where `mean_report_size` is approximately 25 bytes for error-free
reception. If `report_bytes_per_second` exceeds the return channel
capacity, increase `aggrSizeLimit` to reduce the block rate.

This is especially important on **asymmetric links** (high forward rate,
low return rate), which are common in space communications.

**Recommended values:**

| Scenario | aggrSizeLimit | Rationale |
|---|---|---|
| High-throughput terrestrial | 1,000,000 (1 MB) | Good balance of batch size and retransmission risk |
| High-throughput with reliable link | 10,000,000 (10 MB) | Minimizes session overhead on clean links |
| Asymmetric space link | Compute from return channel capacity | Avoid overwhelming the return channel |
| Low-throughput or lossy link | 100,000 (100 KB) | Limits retransmission cost per block |

### Choosing aggrTimeLimit

Set `aggrTimeLimit` to ensure blocks are transmitted promptly even during
low-traffic periods. The value represents the maximum additional latency
LTP adds before transmitting a partially filled block.

- **1 second** is appropriate for most scenarios. It ensures data is
  transmitted within 1 second even if the aggregation size limit is
  never reached.
- **Higher values** (e.g., 5-10 seconds) reduce the number of
  undersized blocks during bursty traffic but increase worst-case
  latency.

For high-throughput links where the data rate consistently exceeds
`aggrSizeLimit / aggrTimeLimit`, the time limit rarely triggers and has
no effect on throughput.

### Contact Plan xmitRate

The token bucket rate control in `udplso` reads the `xmitRate` from the
contact plan (via `IonNeighbor`). This rate determines how fast the token
bucket refills and therefore the maximum transmission rate.

```
# In global.ionrc or .ionrc:
a contact +0 +3600 1 2 10000000
#                       ^^^^^^^^
#                       xmitRate in bytes/sec (10 MB/s = 80 Mbps)
```

**Important considerations:**

- Set `xmitRate` to the actual link capacity or the desired rate cap.
  The token bucket will pace transmission to this rate.
- If `xmitRate` is higher than the physical link can handle, packets
  will queue in the OS network stack and may be dropped.
- If `xmitRate` is lower than the link capacity, the token bucket
  throttles transmission accordingly.
- When no contact is active (`xmitRate = 0`), the token bucket allows
  all pending data to pass without throttling. Rate control only
  engages when a non-zero `xmitRate` is configured.

### Putting It Together: Configuration Examples

#### Example 1: High-Throughput Terrestrial (1 Gbps, low latency)

```
# global.ionrc
a contact +0 +3600 1 2 125000000    # 1 Gbps = 125 MB/s
a range   +0 +3600 1 2 1            # OWLT ~0 (LAN)

# .ltprc
a span 2 100 100 1400 1000000 1 'udplso 10.0.0.2:1113'
#        ^^^ ^^^ ^^^^ ^^^^^^^ ^
#        |   |   |    |       aggrTimeLimit=1s
#        |   |   |    aggrSizeLimit=1MB
#        |   |   maxSegmentSize=1400
#        |   maxImportSessions=100
#        maxExportSessions=100
```

With OWLT near zero, even a few export sessions fully utilize the link.
100 sessions provides ample headroom. Each 1 MB block produces ~714
segments, giving effective multisend batching.

**Sysctl tuning is critical** at this rate — see the kernel tuning
section above.

#### Example 2: Moderate Space Link (1 Mbps, 5-second OWLT)

```
# global.ionrc
a contact +0 +3600 1 2 125000       # 1 Mbps = 125 KB/s
a range   +0 +3600 1 2 5            # 5-second OWLT

# .ltprc
a span 2 10 10 1400 100000 1 'udplso 10.0.0.2:1113'
```

```
max_data_in_transit = 125,000 * 10 = 1,250,000
maxExportSessions >= 1,250,000 / 100,000 = 13 → use 10-15
```

Smaller `aggrSizeLimit` (100 KB) reduces retransmission cost on a link
where errors are more likely. 10 export sessions is close to the
bandwidth-delay product requirement.

#### Example 3: Deep-Space Link (256 Kbps, 10-minute OWLT)

```
# global.ionrc
a contact +0 +7200 1 2 32000        # 256 Kbps = 32 KB/s
a range   +0 +7200 1 2 600          # 10-minute OWLT

# .ltprc
a span 2 400 400 1400 100000 1 'udplso 10.0.0.2:1113'
```

```
max_data_in_transit = 32,000 * 1200 = 38,400,000
maxExportSessions >= 38,400,000 / 100,000 = 384 → use 400
```

The long round-trip time requires many export sessions to keep the link
busy. The `heapWords` in `.ionconfig` must be sized accordingly — see
the [LTP Configuration Tool](../Using-LTP-Config-Tool.md) for heap
sizing guidance.

### Checklist for High-Throughput LTP

1. **Set `maxSegmentSize` to avoid IP fragmentation** — 1400 for
   standard Ethernet
2. **Set `aggrSizeLimit` for your throughput/overhead tradeoff** —
   larger blocks mean fewer sessions and less report traffic, but more
   data at risk per retransmission
3. **Compute `maxExportSessions` from bandwidth-delay product** —
   `(xmitRate * 2 * OWLT) / aggrSizeLimit`, round up
4. **Set `maxImportSessions`** to the remote engine's
   `maxExportSessions`
5. **Set `aggrTimeLimit` to 1** unless you have a specific latency
   tolerance
6. **Set contact plan `xmitRate`** to match your link capacity
7. **Tune sysctl buffers** on both sender and receiver (see kernel
   tuning section above)
8. **Size `heapWords`** in `.ionconfig` to support the number of
   concurrent sessions and their block data — use the LTP configuration
   spreadsheet for guidance
9. **Verify with `bpstats` and `ion.log`** — check for retransmission
   activity and delivery statistics after initial testing
