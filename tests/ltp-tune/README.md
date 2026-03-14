# LTP Performance Tuning Test

**Developer tool** for parametric throughput evaluation of ION's LTP (Licklider
Transmission Protocol) over UDP loopback. Not a regression test — this is for
interactive experimentation and performance analysis. Tests run two ION nodes
on the same host using `localhost` UDP to isolate ION's BP/LTP processing
speed from network effects.

## Quick Start

```bash
cd tests/ltp-tune
./dotest                              # Run with defaults
BUNDLE_SIZE=1000000 ./dotest          # Override a single parameter
```

Override any parameter via environment variables (see full list below):

```bash
BUNDLE_SIZE=5000000 \
MAX_EXPORT=8 \
MAX_IMPORT=8 \
AGG_SIZE=128000 \
CONTACT_RATE=2000000000 \
LTP_INIT=20 \
TOTAL_BYTES=5000000000 \
SENDER_HEAP=200000000 \
RECEIVER_HEAP=100000000 \
./dotest
```

## Tunable Parameters

| Variable | Default | Description |
|----------|---------|-------------|
| `BUNDLE_SIZE` | 1000000 | Bundle payload size (bytes) |
| `TOTAL_BYTES` | 200000000 | Total data to transfer |
| `CONTACT_RATE` | 100000000 | Contact plan rate (bytes/sec) |
| `LTP_INIT` | 50 | LTP estimated max export sessions |
| `MAX_EXPORT` | 10 | Max concurrent export sessions |
| `MAX_IMPORT` | 10 | Max concurrent import sessions |
| `MAX_SEG` | 64000 | LTP max segment size (bytes) |
| `AGG_SIZE` | 100000 | Aggregation size threshold (bytes) |
| `AGG_TIME` | 1 | Aggregation time limit (seconds) |
| `SENDER_HEAP` | 50000000 | Sender SDR heapWords |
| `RECEIVER_HEAP` | 50000000 | Receiver SDR heapWords |
| `SENDER_WM` | 150000000 | Sender SDR wmSize |
| `RECEIVER_WM` | 150000000 | Receiver SDR wmSize |
| `RX_MAX_IMPORT` | 200 | Receiver max import sessions |
| `RX_MAX_EXPORT` | 200 | Receiver max export sessions |
| `PROTO_PAYLOAD` | 1400 | BP protocol estimated payload/frame |
| `PROTO_OVERHEAD` | 100 | BP protocol estimated overhead/frame |
| `MEASURE_PCT` | 90 | Measure throughput at this % of bundles |
| `MAXSECONDS` | 120 | Timeout per test (seconds) |

## How to Run a Parameter Sweep

```bash
# Contact rate sweep
for RATE in 100000000 500000000 1000000000 2000000000; do
  echo "=== CONTACT_RATE=$RATE ==="
  BUNDLE_SIZE=1000000 CONTACT_RATE=$RATE TOTAL_BYTES=1000000000 ./dotest 2>&1 \
    | grep -E '(Throughput|Transfer|TIMEOUT)'
done

# Bundle size sweep
for BS in 63000 500000 1000000 5000000; do
  echo "=== BUNDLE_SIZE=$BS ==="
  BUNDLE_SIZE=$BS CONTACT_RATE=2000000000 TOTAL_BYTES=1000000000 ./dotest 2>&1 \
    | grep -E '(Throughput|Transfer|TIMEOUT)'
done
```

For reliable steady-state measurement, set `TOTAL_BYTES` high enough for at
least several hundred bundles (e.g., `TOTAL_BYTES=$((BUNDLE_SIZE * 1000))`).
Throughput is measured at 90% completion to exclude startup and tail effects.

## Performance Characteristics (ION 4.1.4)

Test system: Intel Xeon w7-3465X (28 cores/56 threads), SDR in DRAM
(`configFlags 1`), POSIX named semaphores, UDP loopback (`localhost`).
Kernel UDP buffers: `rmem_max=16MB`, `wmem_max=16MB`,
`rmem_default=8MB`, `wmem_default=8MB`.

### 1. Bundle Size (Most Impactful Parameter)

Bundle size has the strongest effect on throughput because it determines
per-bundle BP processing overhead relative to payload.

Fixed config: 8 sessions, 128KB aggregation, 64K segments, 2 GB/s contact rate.

**Large bundles (100KB+):**

| Bundle Size | Throughput (Mbps) | Notes |
|------------|-------------------|-------|
| 100 KB | 1,044 | |
| 200 KB | 1,119 | |
| 500 KB | 1,458 | Practical sweet spot |
| 1 MB | 1,550 | |
| 2 MB | 1,602 | Diminishing returns above here |
| 5 MB | 1,643 | Near ceiling |
| 10 MB | 1,642 | No further gain |

**Small bundles (1KB-63KB):**

Fixed config: 8 sessions, 100KB aggregation, 64K segments, 2 GB/s contact rate,
100MB total data for steady-state measurement.

| Bundle Size | Throughput (Mbps) | Per-bundle time |
|------------|-------------------|-----------------|
| 1 KB | 54 | ~148 us |
| 2 KB | 102 | ~157 us |
| 3 KB | 148 | ~162 us |
| 5 KB | 218 | ~184 us |
| 7 KB | 286 | ~196 us |
| 10 KB | 354 | ~226 us |
| 15 KB | 458 | ~262 us |
| 20 KB | 556 | ~288 us |
| 30 KB | 706 | ~340 us |
| 40 KB | 803 | ~399 us |
| 50 KB | 796 | ~503 us |
| 63 KB | 936 | ~539 us |

**Trend:** Throughput scales roughly linearly with bundle size in the small
bundle region. Each bundle incurs a fixed ~150 microsecond BP processing cost
(SDR transactions, forwarding decisions, ECCC computation, plan queue
operations) regardless of payload size. At 1KB, the payload is tiny relative to
this fixed cost. Aggregation does not help because the bottleneck is per-bundle
BP processing, not per-session LTP overhead.

For large bundles (500KB+), throughput scales as `log(bundle_size)` and
asymptotically approaches the CPU processing ceiling (~1.6 Gbps on this
system).

### 2. Contact Plan Rate (Second Most Impactful)

The contact rate directly controls two rate limiters:
1. **bpclock throttle** — BP daemon adds `nominalRate` bytes of capacity per
   second (derived from contact rate and protocol frame estimates)
2. **udplso token bucket** — refills at `xmitRate` bytes/sec (set from
   neighbor's contact rate)

If contact rate is below ION's processing capacity, it becomes the bottleneck.
If too high (>3 GB/s in our tests), the sender overwhelms the receiver's UDP
buffer, causing silent drops and retransmissions.

Fixed config: 1MB bundles, 8 sessions, 128KB aggregation, 64K segments.

| Contact Rate | Throughput (Mbps) | Status |
|-------------|-------------------|--------|
| 100 MB/s | 641 | Rate-limited |
| 150 MB/s | 826 | Rate-limited |
| 200 MB/s | 958 | Rate-limited |
| 300 MB/s | 1,157 | Rate-limited |
| 500 MB/s | 1,388 | Approaching CPU limit |
| 1 GB/s | 1,550 | Near CPU limit |
| 2 GB/s | 1,571 | At CPU limit |
| 3 GB/s | 1,598 | Peak |

**Trend:** Throughput follows the contact rate linearly until ~500 MB/s, then
asymptotically approaches the CPU processing ceiling (~1.6 Gbps). Beyond
3 GB/s, performance degrades due to UDP buffer overflow at the receiver.

**Recommendation:** Set contact rate 1.5-2x higher than expected throughput.
For loopback testing, 2 GB/s is a safe default for high-throughput tests.

#### Why Contact Rate Must Exceed Processing Speed

ION enforces the contact rate at two layers independently:

1. **bpclock throttle** (BP layer): adds `nominalRate` bytes of capacity once
   per second; bpclm deducts `computeECCC(guessBundleSize())` per bundle,
   which adds a 6.25% overhead surcharge plus 29 bytes per bundle.
2. **udplso token bucket** (LTP/UDP layer): refills tokens based on elapsed
   microseconds and deducts actual bytes sent (including 28 bytes IP+UDP
   header per segment).

The udplso token bucket calls `nanosleep()` to pace transmission. On Linux,
`nanosleep()` has a minimum actual sleep of ~70-250 microseconds (depending on
system load) regardless of the requested duration. When the per-segment pacing
delay drops below this floor, the kernel oversleeps, wasting link time.

A `NANOSLEEP_FLOOR_USEC` fix in `udplso.c` skips sleeps below the floor and
lets the deficit carry forward, compensated on the next refill via elapsed-time
measurement. This reduces the contact rate inflation needed from 2-4x to ~1.5x
at moderate rates:

| Contact Rate | Before fix (Mbps) | After fix (Mbps) | Improvement |
|-------------|-------------------|------------------|-------------|
| 150 MB/s | 777 | 826 | +6% |
| 200 MB/s | 920 | 958 | +4% |
| 500 MB/s | 1,349 | 1,388 | +3% |
| 1 GB/s | 1,542 | 1,550 | +1% |
| 2 GB/s | 1,601 | 1,571 | (noise) |

### 3. LTP Segment Size

Segment size determines the number of UDP datagrams per LTP block.
Constrained to <=64KB for UDP transport.

Fixed config: 5MB bundles, 8 sessions, 128KB aggregation, 2 GB/s contact rate.

| Segment Size | Throughput (Mbps) | Segments per 5MB block |
|-------------|-------------------|----------------------|
| 1,400 | 785 | ~3,571 |
| 10,000 | 1,189 | ~500 |
| 32,000 | 1,559 | ~156 |
| 64,000 | 1,632 | ~78 |

**Trend:** Throughput scales strongly with segment size because each segment
incurs fixed per-segment processing (SDR operations, UDP send/receive).
Always use the maximum segment size allowed by the transport (64,000 for UDP).

### 4. Max Export Sessions

Controls LTP pipeline depth — how many sessions (blocks) can be in-flight
simultaneously.

Fixed config: 1MB bundles, 128KB aggregation, 2 GB/s contact rate.

| Sessions | Throughput (Mbps) | Notes |
|----------|-------------------|-------|
| 1 | 1,480 | Single session pipeline |
| 2 | 1,498 | |
| 4 | 1,535 | |
| 8 | 1,566 | Optimal for 1MB+ bundles |
| 16 | 1,539 | Slight overhead increase |
| 32 | 397 | Severe degradation |

**Trend:** For large bundles (>= aggregation threshold), each bundle gets its
own LTP session. Few concurrent sessions (4-8) are sufficient because each
session already carries substantial data. Higher session counts add management
overhead without benefit. Very high counts (32+) cause catastrophic degradation
from resource contention.

For small bundles (< aggregation threshold), the aggregation mechanism batches
multiple bundles per session, so session count matters less.

### 5. Aggregation Size Threshold

Controls how much data LTP accumulates before creating a block/session.
Only matters when bundles are smaller than the threshold.

**With 1MB bundles (larger than any tested threshold):**

| Aggregation | Throughput (Mbps) |
|------------|-------------------|
| 64 KB | 1,557 |
| 128 KB | 1,596 |
| 500 KB | 1,580 |
| 1 MB | 1,565 |
| 2 MB | 1,580 |

Minimal impact — each bundle exceeds the threshold regardless.

**With 100KB bundles (smaller than threshold):**

| Aggregation | Throughput (Mbps) | Bundles per block |
|------------|-------------------|-------------------|
| 128 KB | 890 | ~1 |
| 500 KB | 1,197 | ~5 |
| 1 MB | 1,291 | ~10 |
| 5 MB | 1,336 | ~50 |

Large aggregation significantly helps small bundles by amortizing per-session
overhead across more data.

**With 1KB bundles (steady-state, 100MB total):**

| Aggregation | Throughput (Mbps) |
|------------|-------------------|
| 100 KB | 54 |
| 500 KB | 57 |
| 1 MB | 57 |
| 5 MB | 56 |

No impact — at 1KB, the bottleneck is per-bundle BP processing (~150 us per
bundle), not LTP session overhead. Aggregation helps at the LTP layer (fewer
sessions) but each bundle still goes through the full BP pipeline individually.

**Recommendation:** Set aggregation at or slightly above the expected bundle
size. For mixed traffic, 128KB is a reasonable default.

### 6. LTP Init Parameter & Protocol Frame Estimates

These have minimal impact on throughput (< 2% variation across tested ranges).

**LTP Init** (`1 N`): Tested 5-100, all within 1,600-1,641 Mbps.
Use `N` >= `MAX_EXPORT` for consistency.

**Protocol frame estimates** (`a protocol ltp P O`): Tested 1400/100 vs
64000/100 vs 64000/28 — negligible difference when contact rate is high.

### 7. Heap and Working Memory

Tested `heapWords` from 50M to 500M and `wmSize` from 150M to 300M.
No measurable throughput difference. These only matter if they're too small
to hold the in-flight data (which causes congestion/backpressure).

**Rule of thumb:** `heapWords >= MAX_EXPORT * max_block_size * 4` (account for
both send and receive side SDR objects). For high-throughput with 5MB bundles
and 8 sessions, `heapWords=200000000` is sufficient.

## Optimal Configurations

### High Throughput (Gbps+)

Target: maximize throughput with large bundles.

```bash
BUNDLE_SIZE=5000000 \
MAX_EXPORT=8 \
MAX_IMPORT=8 \
AGG_SIZE=128000 \
MAX_SEG=64000 \
CONTACT_RATE=2000000000 \
LTP_INIT=20 \
TOTAL_BYTES=5000000000 \
SENDER_HEAP=200000000 \
RECEIVER_HEAP=100000000 \
./dotest
```

Expected: ~1.6 Gbps (system-dependent).

### Moderate Throughput (practical bundle sizes)

Target: good throughput with typical 63KB bundles.

```bash
BUNDLE_SIZE=63000 \
MAX_EXPORT=4 \
MAX_IMPORT=4 \
AGG_SIZE=100000 \
MAX_SEG=64000 \
CONTACT_RATE=200000000 \
LTP_INIT=50 \
TOTAL_BYTES=1000000000 \
SENDER_HEAP=50000000 \
RECEIVER_HEAP=50000000 \
./dotest
```

Expected: ~860 Mbps (system-dependent).

## Key Takeaways

1. **Bundle size is king.** Moving from 63KB to 1MB bundles nearly doubles
   throughput. This is purely BP-layer overhead — LTP processes the same
   number of segments regardless.

2. **Small bundles are BP-bound.** Below ~63KB, throughput scales linearly with
   bundle size at roughly 54 Mbps per KB. Each bundle costs ~150 us of fixed
   BP processing. Neither aggregation, session count, nor contact rate can
   overcome this per-bundle cost.

3. **Contact rate must exceed processing speed.** If the contact rate caps
   throughput below the CPU ceiling, larger bundles won't help. Set it 1.5-2x
   above expected throughput (with the `NANOSLEEP_FLOOR_USEC` fix in
   `udplso.c`; without the fix, 2-4x is needed).

4. **Contact rate has a ceiling.** Setting it far above processing speed
   (>3 GB/s on our system) causes UDP buffer overflow and silent drops,
   reducing throughput. The sweet spot is 1.5-2x the achieved throughput.

5. **Session count: less is more (for large bundles).** 4-8 sessions is
   optimal for MB-sized bundles. More sessions add overhead without benefit
   since each session already carries substantial data.

6. **64KB segments are optimal** for UDP transport (maximum before
   fragmentation). Smaller segments drastically reduce throughput.

7. **Aggregation helps medium bundles.** If using bundles in the 10-500KB range
   (smaller than the aggregation threshold), increasing the threshold batches
   more data per session and improves throughput. Has no effect on very small
   bundles (1-5KB) where per-bundle BP overhead dominates.

8. **SDR memory** (`heapWords`, `wmSize`) only matters if it's too small. Once
   sufficient, adding more memory has no effect.

9. **LTP init, protocol frame estimates** have negligible impact when other
   parameters are well-tuned.
