# bench-ltp-xlsa: Measured Results

End-to-end goodput numbers for LTP carried over the xlsa shared-memory
backend, compared to LTP over UDP loopback (`demos/bench-ltp/`), across
the bundle-size dimension that bench-ltp sweeps.

These were the first runs that completed end-to-end after the in-tree
xlsa landing (commits `609d62c5a` through `be9108b50` on
`update-4.2.0-demo-ltp-xlsa`). They are **single-iteration** numbers on
one machine, intended to validate the substrate and establish the
performance shape — not statistically robust comparisons.

## Test environment

| Item | Value |
|---|---|
| Host | Apple Mac16,9 (Mac Studio) |
| CPU | Apple M4 Max (arm64) |
| OS | macOS / Darwin 25.4.0 |
| ION | `integration` branch at commit `87c34fb` + this branch's xlsa commits |
| Build | `./configure --with-xlsa-backend=shm`, `-O2 -Werror` |
| Workload tool | `bpdriver` (sender) → `bpcounter` (receiver, 90 % steady-state mark) |
| Both daemons | Same host; engines 2 → 3; mirrored ring/UDP-port topology |

Each scenario starts ION fresh, runs one bpdriver/bpcounter pair, tears
ION down, then advances. Both substrates use the same contact-plan rate
(2 GB/s) and the same per-bundle workload.

### Kernel UDP/socket tuning on this machine

This host has the socket-buffer sysctls elevated above macOS stock
defaults. That matters because the UDP-substrate numbers below are
*more* favorable to UDP than they would be on an out-of-the-box system.

| sysctl | This host | macOS stock | Ratio |
|---|---:|---:|---:|
| `net.inet.udp.maxdgram` | 65 536 | 65 536 | 1× |
| `net.inet.udp.recvspace` | **786 896** | ~42 080 | **~19×** |
| `kern.ipc.maxsockbuf` | **8 388 608** | 4 194 304 | **2×** |
| `net.inet.udp.checksum` | 1 | 1 | (on; honest test) |

A udplsi receive buffer of ~770 KB (vs. the stock ~41 KB) easily absorbs
a sender burst of ~12 segments × 64 KB ≈ 768 KB without `ENOBUFS`. On a
stock kernel that burst would trip udplso's `applyTokenBucket` retry
loop and visibly drag the numbers down. **Read the UDP column as an
upper bound for what UDP loopback can do on this kernel, not as a
representative ION-on-stock-macOS result.** On a stock host the gap to
xlsa would widen at the medium-bundle rows; the 5 MB / 1 GB TIMEOUT
would only get worse.

## Bundle-size sweep (bench-ltp's TESTS array, row-for-row)

Three substrate variants:

- **xlsa shm 64 KB seg** — `xport_shm` with `XLSA_BUFSZ = 65535`,
  `maxSegmentSize = 64000`. Matches bench-ltp's row parameters
  exactly (`max_export`, `max_import`, `agg_size` as listed by
  bench-ltp's `TESTS`).
- **xlsa shm 1 MB seg** — same as above but `XLSA_BUFSZ` bumped to
  1 MB and the sweep overrides each row's `maxSegmentSize` to
  1 000 000 (also bumping the aggregation buffer to fit at least one
  full segment). Invoked via `SEG_SIZE=1000000 ./dotest bundlesize`.
- **UDP loopback 64 KB seg** — `demos/bench-ltp/dotest`, unmodified.

Numbers are `bpcounter`-reported Mbps (precise; matches wall-clock at
the slow end but is more accurate sub-second).

| Bundle | Total | xlsa shm 64 KB seg | **xlsa shm 1 MB seg** | UDP loopback 64 KB seg |
|---:|---:|---:|---:|---:|
| 5 MB | 1 GB | 2148 | **2628** *(+22 %)* | TIMEOUT (>120 s) |
| 1 MB | 1 GB | 2016 | **2537** *(+26 %)* | 924 |
| 500 KB | 500 MB | 1794 | **2003** *(+12 %)* | 1938 |
| 200 KB | 200 MB | 1239 | **1985** *(+60 %)* | 1430 |
| 63 KB | 200 MB | 986 | **1175** *(+19 %)* | 1043 |
| 20 KB | 200 MB | 569 | 627 *(+10 %)* | 600 |
| 10 KB | 200 MB | 329 | 345 *(+5 %)* | 340 |
| 5 KB | 200 MB | 185 | 183 *(–1 %)* | 187 |
| 1 KB | 200 MB | 41 | 37 *(–9 %)* | 41 |

### Observations

**1. At bundles ≥ 63 KB, xlsa with 1 MB segments wins across the
board.** +10 % to +60 % over the 64 KB-segment xlsa, and +13 % to +∞ %
over UDP. The +60 % spike at the 200 KB row is the cleanest
demonstration of the per-segment coordination cost: at 64 KB segments
each 200 KB bundle splits into 4 segments and pays 4× the
mutex/condvar overhead; at 1 MB segments it's 1 segment and the
substrate's contention drops 4×.

**2. The 5 MB / 1 GB row is the substrate-class divider.** xlsa runs it
at 2.1 – 2.6 Gbps (depending on segment size). UDP loopback **cannot
complete** the workload within bench-ltp's 120 s watchdog — udplso's
`sendmmsg`/`ENOBUFS` retry loop and the kernel's UDP socket buffer
become the bottleneck under the burst of ~80 segments per bundle. If
the deliverable is "what's the largest single-host LTP workload you
can actually push," this row is the headline.

**3. At bundles ≤ 20 KB, segment size stops mattering.** All three
substrates converge — per-bundle BP/LTP cost dominates (bundle header
encoding, ZCO get/give, SDR transactions), the substrate is idle most
of the time, and bigger segments can't be filled anyway because
aggregation timing closes the LTP block before they reach segment
size.

**4. UDP loopback on Apple Silicon is more competitive than expected.**
At medium bundles (500 KB – 20 KB), unmodified UDP is within 5–15 %
of 64 KB-segment xlsa. macOS's loopback path is well-tuned (in-kernel
zero-copy where possible) and udplso's `sendmmsg` batching amortizes
syscall cost. On Linux x86 the gap would likely be wider — different
loopback MTU, different softirq scheduling, no equivalent optimization.

### Why the 1 MB-segment lift is real but bounded

`xport_shm`'s ring uses a **single process-shared mutex/condvar pair**
for producer/consumer coordination. At the 64 KB-segment regime the
2 Gbps ceiling corresponds to ~32 K segments/sec × 4 mutex acquires/sec
× small cond_signal cost — clearly serialized lock traffic, not memcpy
bandwidth (memcpy on Apple Silicon is 30+ GB/s).

Bumping `maxSegmentSize` to 1 MB cuts the per-byte mutex cost 16× at
large bundles, and throughput moves accordingly. At small bundles the
ratio of mutex ops to bytes is still high (the aggregation block can't
fill a 1 MB segment), so the lift evaporates.

The next ceiling, at ~2.6 Gbps with 1 MB segments, is **most likely the
LTP engine itself** — SDR transactions for each segment, ZCO
acquire/release, `ltpmeter` aggregation logic. To measure this directly
would require a microbenchmark that ping-pongs through `xport_send` /
`xport_recv` without LTP in the loop.

## Same workload in a Linux/aarch64 docker container

To check portability and confirm the demo runs on the targeted
deployment platform, the full compare set was re-run inside the
`ion-test-suite-ARM64` dev container (Oracle Linux 9.7 aarch64,
under docker on Apple Silicon via linuxkit) at two `/dev/shm` caps.

Two operating regimes:

| | Container A | Container B |
|---|---:|---:|
| `shm_size` | docker default 64 MB | 512 MB (recommended) |
| `XLSA_SLOTS` (auto-shrunk) | 16 | 64 |
| ring per direction | 16 MB | 64 MB |
| tmpfs reserved for ION sem files | 32 MB | 384 MB |

The 64 MB cap is what docker gives a container if `shm_size` is not
set. With it, ION + the xlsa rings together exhaust the tmpfs and
`bpadmin` fails to create its volatile-database sem files. The demo's
`dotest` auto-shrinks `XLSA_SLOTS` to keep its own footprint to
`shm / 4` per ring, which lets the run complete; the regression
wrapper `tests/bench-ltp-xlsa/dotest` pins `XLSA_SLOTS=16` directly
to avoid depending on the auto-shrink math at all. **For the demo
to run with the default 1024-slot rings, the orchestration that
spins the container up needs `shm_size: 512m` (compose) or
`--shm-size=512m` (`docker run`).** See `ION-dev-issue` repo `89f5656`
for the docker-compose template change.

Results (`bpcounter`-reported Mbps, 1 MB-seg pass only; 64 KB-seg
results follow the same shape):

| Bundle / Total | Container 16-slot | Container 64-slot | Mac native 1024-slot |
|---:|---:|---:|---:|
| 5 MB / 1 GB | 573 | 575 | 2628 |
| 1 MB / 1 GB | 608 | 601 | 2537 |
| 500 KB / 500 MB | 587 | 582 | 2003 |
| 200 KB / 200 MB | 506 | 498 | 1985 |
| 63 KB / 200 MB | 319 | 319 | 1175 |
| 20 KB / 200 MB | 160 | 158 | 627 |
| 10 KB / 200 MB | 92 | 92 | 345 |
| 5 KB / 200 MB | 53 | 52 | 183 |
| 1 KB / 200 MB | TIMEOUT | TIMEOUT | 37 |

**The 4× increase in ring depth bought zero throughput.** Both
container regimes report essentially the same numbers row for row
across both segment sizes. Container/Mac is a near-constant **~25 %**
across every bundle row, which strongly suggests the bottleneck is
**not** ring coordination cost (that would scale with ring depth)
but a flat overhead factor shared by every code path — most likely
**linuxkit's syscall and memory-translation cost on Apple Silicon**.
Every `memcpy` into the shm ring, every `pthread_cond_signal`, every
sem-related syscall crosses the hypervisor boundary at non-trivial
cost. To confirm, run the same compare set on bare-metal Linux
aarch64 (no hypervisor) — we expect throughput to climb back toward
the Mac numbers, leaving ring-depth-doesn't-matter as the true
finding.

The 1 KB row times out in both container regimes because per-bundle
BP/LTP overhead × 200 000 bundles × constant container slowdown
exceeds `bench-ltp`'s `MAXSECONDS=120` watchdog. On the Mac the same
workload completes in 39 seconds at 37 Mbps.

**Container takeaway for the bench-ltp-xlsa user:**
- Always set `shm_size: 512m` (or higher) on the container — without
  it the demo runs at reduced ring depth and the regression test
  pins itself defensively.
- Do not benchmark on a hypervised container; throughput numbers
  there are an overhead-bounded constant, not a substrate property.
- For "does xlsa work on Linux aarch64?" the container is a valid
  smoke-test environment; for "what throughput does the substrate
  deliver?" use a bare-metal host of the target platform.

## Cross-platform 1 MB bundle / 1 MB segment comparison

The same bench-ltp-xlsa workload — bundle size 1 MB, `maxSegmentSize`
1 MB, 1 GB total transferred — has now been measured on every test
environment that touched this PR. This single row is the cleanest
"what does the substrate's ceiling look like on platform X?" data
point, and it makes the platform/container/CPU contributions
separable.

| Platform | Class | bpcounter Mbps |
|---|---|---:|
| **Mac M4 Max** | macOS arm64, bare metal | **2537** |
| ARC RHEL 9 | x86-64 Linux CI container (GitHub Actions Runner Controller) | 991 |
| ARC RHEL 8 | x86-64 Linux CI container | 973 |
| **Linuxkit container** | aarch64 Linux container on M4 (hypervised, 512 MB shm) | 601 |
| ARC Oracle Linux 9 | x86-64 Linux CI container | 598 |
| ARC Oracle Linux 8 | x86-64 Linux CI container | 588 |
| ARC Ubuntu 22.04 | x86-64 Linux CI container | 570 |
| ARC Ubuntu 20.04 | x86-64 Linux CI container | 568 |

**The ARC pool clearly splits into two tiers.** RHEL-class hosts
deliver ~980 Mbps; Oracle Linux and Ubuntu hosts deliver ~580 Mbps.
The ~1.7× factor is consistent across every bundle row (not just
this one) and reflects different x86-64 SKUs in the ARC pool rather
than anything xlsa is doing differently. RHEL appears to be on
newer/faster nodes; the OL/Ubuntu pool is on older silicon.

**The Apple Silicon linuxkit container lands in the slow tier of
ARC hosts**, not below it. ~600 Mbps for the Linuxkit M4 container
sits right next to ARC OL/Ubuntu's ~580–600 Mbps. What we earlier
attributed primarily to "linuxkit syscall overhead on Apple Silicon"
is actually the typical figure for a moderately-provisioned Linux
container running this workload — the CPU class of the slow ARC
tier and the linuxkit-wrapped M4 are evidently comparable for this
workload.

**Bare-metal arm64 macOS remains alone at the top** at 2537 Mbps —
roughly 2.5× the fastest container measurement and 4.4× the slowest.
That gap is the combined cost of (a) container virtualization, (b)
slower CPU on most CI nodes, and (c) macOS's well-tuned scheduler
and pthread/futex paths that the substrate's mutex/condvar
coordination benefits from on bare metal. A bare-metal Linux box of
M4-class CPU performance is the missing data point that would let
us decompose those three contributions; for now they collapse into
one number.

## Other (impairment) sweeps

Earlier exploratory runs at 1000 bundles × 10 KB (10 MB total)
exercised the emulator's delay / drop / rate / matrix knobs. Results
are not reproduced here in full because the workload is too small for
those impairments to meaningfully bite, but the headline was:

- Delay sweep saw the expected LTP-timer collapse — 234 Mbps at 0 µs
  delay, 40 Mbps at 100 ms one-way, 4.4 Mbps at 1 s one-way.
- Drop sweep (0–10 000 ppm) was effectively flat at ~240 Mbps; the
  10 MB workload is too small to reveal retransmission cost at the
  loss rates tested.
- Rate sweeps and the matched/under/over-provisioned matrix all
  landed within a 220 – 260 Mbps band, dominated by the small-workload
  ceiling rather than the impairment knob.

A larger workload (e.g. 200 MB) with `ltpstats` retransmission counters
captured would make the drop sweep meaningful. Future work.

## Caveats

- **Single iteration per scenario.** Variance is real (the wall-clock
  `goodput_bps` column in dotest's SUMMARY rounds to integer seconds,
  so it disagrees with bpcounter's Mbps below ~1 s). Multi-iteration
  runs with confidence intervals would tighten the small-bundle rows.
- **One host.** All numbers are macOS arm64. The shape will differ on
  Linux x86 (UDP loopback path is structurally different).
- **xlsa shm assumes trusted same-host benchmark** — no validation of
  ring contents. The DESIGN.md §7 caveats stand.
- **Substrate ceiling is mutex-bound, not memcpy-bound.** A lock-free
  SPSC ring would shift the ceiling higher; we have not measured how
  much.

## How to reproduce

```sh
# Build & install ION with xlsa (xlsa is built by default)
./configure --with-xlsa-backend=shm   # shm is also the default
make
sudo make install
```

The xlsa demo's `./dotest` (with no arguments) runs the full
comparison set — `baseline` + bundlesize sweep at 64 KB segments +
bundlesize sweep at 1 MB segments — in one go:

```sh
cd demos/bench-ltp-xlsa
./dotest
```

Run the UDP side once for the comparison column:

```sh
cd ../bench-ltp
./dotest
```

If you want to run a subset by hand, the named scenarios still work:

```sh
./dotest baseline                       # the 10 MB sanity run only (~30 s)
./dotest bundlesize                     # 9-row sweep at 64 KB segs   (~12 min)
SEG_SIZE=1000000 ./dotest bundlesize    # same sweep at 1 MB segs     (~12 min)
./dotest delay drop rate matrix         # impairment sweeps
./dotest all                            # everything above
```

The SUMMARY tables at the end of each run contain the per-row goodput
(integer-second precision). For the precise per-row Mbps used in the
table above, grep `Throughput (Mbps):` in each run's stdout — those
lines come straight from `bpcounter`.
