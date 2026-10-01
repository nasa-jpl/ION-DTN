# LTP Performance and Tuning

Consolidated reference for measured ION LTP throughput **and practical guidance
for tuning it**. It pulls together results that were previously scattered across
the Deployment Guide (ION 4.1.2 and 4.2-alpha, LTP over UDP) and the
`bench-ltp-xlsa` demo (LTP over the shared-memory link service), organized by the
three dimensions that all of them sweep: **bundle size, LTP segment size, and
platform**. If you came here to tune a node, jump to
[Tuning for throughput](#tuning-for-throughput); the measured results are the
evidence behind that advice.

All numbers are illustrative of ION's software pipeline on the hardware noted;
they bound the *shape* of LTP performance, not a portable ceiling. For
link-specific (long-delay, lossy space-link) configuration, use the **LTP
Configuration Tool** spreadsheet shipped with each ION release rather than these
loopback/benchmark numbers — but see the note under *See also* on its
dependence on the now-deprecated `maxber` parameter.

## The mental model: two regimes

Every dataset below tells the same story, so it is worth stating once:

- **Bundle-bound (small bundles).** Per-bundle bookkeeping — BP source/sink,
  ZCO plumbing, SDR transactions, LTP session setup/teardown — dominates total
  time. The LTP **segment size barely matters** here; throughput is limited by
  how fast ION can shepherd bundles through the BP/LTP path.
- **Segment-bound (large bundles).** Per-bundle overhead is amortized across
  more payload, and per-segment framing (UDP `sendto()`/`sendmmsg()`, ACK
  accounting, or — for xlsa — ring mutex traffic) takes over. **Segment size
  dominates** here.

Practical consequence: **pick the segment size that matches the link, then size
bundles accordingly.** Tuning segment size in isolation moves the result very
little when the workload is bundle-bound.

## LTP over UDP (ION 4.2 alpha)

Two-node loopback (`demos/bench-ltp`), contact rate capped at 16 Gbps so the
measurement floor is software speed, SDR in DRAM, kernel UDP buffers raised to
16 MB/8 MB. Hardware: Intel Xeon w7-3465X (Sapphire Rapids, 28C/56T), Linux 6.6
(WSL2), loopback only.

> **Kernel UDP buffers must be raised first.** At the Linux factory default
> (`net.core.rmem_max ≈ 200 KB`) a 2 Gbps target drops enough UDP segments that
> LTP spends each window retransmitting; benchmarking the processing ceiling is
> impractical until the buffers are enlarged. `demos/bench-ltp/dotest` enforces a
> minimum and warns otherwise.

Throughput vs. bundle size at two segment sizes — 64,000 B (space-link) and
1,400 B (sub-Ethernet-MTU, the terrestrial ceiling):

| Bundle Size (B) | 64 KB segments (Mbps) | 1,400 B segments (Mbps) | 64 KB advantage |
|----------------:|----------------------:|------------------------:|----------------:|
| 5,000,000 | 2,373.5 | 819.3 | 2.90× |
| 1,000,000 | 2,175.4 | 801.1 | 2.72× |
| 500,000   | 2,129.6 | 765.1 | 2.78× |
| 200,000   | 1,490.5 | 639.3 | 2.33× |
| 63,000    | 1,153.6 | 576.2 | 2.00× |
| 20,000    |   650.9 | 456.3 | 1.43× |
| 10,000    |   430.1 | 332.3 | 1.29× |
| 5,000     |   255.0 | 214.9 | 1.19× |
| 1,000     |    59.5 |  56.9 | 1.05× |

The 64 KB curve flattens around 2.1–2.4 Gbps once bundles exceed ~500 KB; the
1,400 B curve plateaus near 800 Mbps for any bundle ≥ 1 MB (syscall-bound at
that segment size). The 1.05× gap at 1,000 B bundles is the bundle-bound floor;
the 2.90× gap at 5 MB bundles is the segment-bound ceiling.

## LTP over xlsa (shared-memory link service)

`xlsa` is ION's shared-memory LTP link service adapter (`xport_shm`): the LTP
segments cross a process-shared ring buffer instead of the UDP/kernel path,
which is the relevant substrate for same-host inter-process LTP (see
`ltp/xlsa/doc/DESIGN.md`). Measured on bare-metal arm64 macOS (Apple M4 Max),
comparing xlsa at 64 KB and 1 MB segments against unmodified UDP loopback:

| Bundle | xlsa 64 KB seg (Mbps) | xlsa 1 MB seg (Mbps) | UDP loopback 64 KB (Mbps) |
|---:|---:|---:|---:|
| 5 MB   | 2148 | **2628** | TIMEOUT (>120 s) |
| 1 MB   | 2016 | **2537** | 924 |
| 500 KB | 1794 | **2003** | 1938 |
| 200 KB | 1239 | **1985** | 1430 |
| 63 KB  |  986 | **1175** | 1043 |
| 20 KB  |  569 |   627 | 600 |
| 10 KB  |  329 |   345 | 340 |
| 5 KB   |  185 |   183 | 187 |
| 1 KB   |   41 |    37 |  41 |

Key findings (full analysis in `demos/bench-ltp-xlsa/doc/RESULTS.md`):

- **At bundles ≥ 63 KB, xlsa with 1 MB segments wins** (+10 % to +60 % over
  64 KB-segment xlsa). The +60 % spike at 200 KB is the cleanest view of
  per-segment coordination cost: 64 KB segments split a 200 KB bundle into 4
  segments and pay 4× the ring mutex/condvar overhead; a 1 MB segment is one.
- **The 5 MB row is the substrate-class divider.** xlsa sustains 2.1–2.6 Gbps;
  UDP loopback cannot complete the workload within the 120 s watchdog
  (`sendmmsg`/`ENOBUFS` retry plus kernel socket-buffer limits). This is the
  headline if the question is "largest single-host LTP workload you can push."
- **At bundles ≤ 20 KB, segment size stops mattering** — all three substrates
  converge in the bundle-bound regime.
- **The xlsa ceiling is mutex-bound, not memory-bound.** `xport_shm`'s ring uses
  a single process-shared mutex/condvar pair; the ~2 Gbps wall at 64 KB segments
  is serialized lock traffic (memcpy on this host is 30+ GB/s). The next ceiling
  (~2.6 Gbps with 1 MB segments) is most likely the LTP engine itself
  (per-segment SDR transactions, ZCO acquire/release, `ltpmeter`).

## Cross-platform (xlsa, 1 MB bundle / 1 MB segment)

Same workload (the 1 MB/1 MB row) across substrates, illustrating the platform
dimension:

| Platform | Throughput (Mbps) |
|---|---:|
| Bare-metal arm64 macOS (Apple M4 Max) | 2537 |
| Bare-metal Linux aarch64 (Raspberry Pi 5, Cortex-A76) | 1522 |
| Solaris 11.4 SPARC (sun4v) | 1149 |
| x86-64 Linux container, fastest tier (RHEL 9) | 991 |
| Apple-Silicon linuxkit container (M4) | 601 |
| x86-64 Linux container, slow tier (OL/Ubuntu) | ~580–600 |

The dominant lesson is that **virtualization, not CPU class, sets the tier**:
the bare-metal Pi 5 beats a hypervised M4 linuxkit container (601 Mbps) by 2.5×
despite older/slower cores, and also beats every x86-64 container tier. Solaris
SPARC outperforms all x86-64 container tiers. Bare-metal arm64 macOS stands
alone at the top (~1.7× the Pi 5). See `RESULTS.md` for the full per-platform
tables, the impairment (delay/drop/rate) sweeps, and reproduction steps.

## Historical baseline (ION 4.1.2, condensed)

Earlier LTP-over-UDP measurements (full detail in the ION 4.1.2 Deployment
Guide history) established the durable trends that still hold:

- **Hardware range:** ~60 Mbps one-way on stock Raspberry Pi 4B (unoptimized,
  low end) up to ~3.7 Gbps between 2012-era Xeon servers over a 10 Gbps
  Ethernet link — the latter CPU/single-stream-bound, not link-bound (iperf
  reached 9.9 Gbps on the same NIC).
- **SDR configuration has a real throughput cost.** Object-boundary checks
  (`SDR_BOUNDED`) cost ~11 %; **reversibility is expensive** (≈ 130 Mbps vs
  ≈ 900 Mbps with vs. without, file-backed transaction log). Enable
  reversibility only where crash-consistency requires it.
- **Larger bundles raise throughput, and LTP block aggregation cushions mixed
  traffic** — a 32× increase in per-bundle overhead produced only ~10× lower
  throughput because aggregation keeps the LTP session/handshake count low.
- **Kernel UDP buffers matter** (8 MB unblocked higher rates on the 10 Gbps
  study) — the same lesson the 4.2 UDP results make mandatory.

## Tuning for throughput

The sections above are the "why"; this is the "what to do," biggest-impact
first. None of it changes correctness — it changes how close a node gets to the
ceilings shown above.

### It runs on one core

ION processes each node's traffic through a single shared data store, and only
one thread can be inside that store at a time, so a node's **LTP data path is
effectively single-threaded** — the "mutex-bound" / per-segment-SDR-transaction
ceiling seen in the xlsa results above, and the "single-stream-bound" limit in
the historical 10 Gbps result.

- **More cores do not make one flow faster.** A single sender-to-receiver
  transfer is bound by how fast *one* core runs the engine; prefer a high
  single-core clock over many slower cores.
- **Give each node its own CPU.** Running a sender and a receiver on the same
  host makes them share it — fine for testing, not for peak throughput.
- **Do not pin ION to a core** on a general-purpose host; let the OS place the
  processes. Manual pinning rarely helps and can hurt by stopping the sender,
  receiver, and link-service helpers from running in parallel. (A dedicated
  single-purpose node is the exception — measure before relying on it.)

### Keep the CPU at full clock

LTP works in short bursts — send a round of segments, wait for a report, repeat
— so Linux's default `powersave` CPU governor can read the gaps as idle and drop
the clock, cutting throughput substantially. On a throughput-sensitive node, set
the `performance` governor on **both** ends:

```sh
sudo cpupower frequency-set -g performance
```

It is one of the easiest wins and costs only power.

### Build ION with optimization on

Compiler optimization has a large effect on throughput — an unoptimized build can
run on the order of a third slower. ION compiles with `-O2` by default, **but only
when you do not set your own `CFLAGS`**. If you pass `CFLAGS` (for example to add
`-D` defines), it replaces that default, and unless you include an optimization
level the compiler silently falls back to unoptimized `-O0`. So either leave
`CFLAGS` unset, or include a level when you set it — `CFLAGS="-O2 …"`.

### Match the SDR store to the node's job

ION's SDR (its transaction store) can keep an undo log so transactions roll back
cleanly after a crash — *reversibility* — which is valuable operationally but
costly on the throughput path (see the reversibility cost noted under the
historical baseline above). Two settings help:

- Where crash-consistency is not required (test nodes, replaceable relays), run
  the SDR non-reversible for best throughput.
- Where reversibility is needed, keep the heap in memory and give the undo log a
  non-zero in-memory size (`logSize`) so it lives in shared memory rather than a
  file. A file-mode log makes a write system call on every update — several times
  slower — **even when the file sits on a RAM filesystem** like `/dev/shm`.

### Choose the link service

- **UDP** (`udplso`/`udplsi`) is fine up to a point, but at high datagram rates
  the kernel receive buffer overflows and throughput collapses into
  retransmission. **Raise the kernel socket buffers before pushing UDP hard** (and
  consider multisend) — see
  [LTP UDP multisend tuning](ltp-udp-multisend-tuning.md); the UDP table above was
  taken with buffers raised to 16 MB/8 MB.
- **Shared memory** (`xlsa`) avoids the UDP socket path entirely and is the better
  substrate for a high-rate link between nodes on the same host (the xlsa table
  above).
- **In flight**, use the mission's link service; the guidance here applies to the
  engine running above it.

### Size the span parameters

Match **segment size** to the link — around the Ethernet MTU for terrestrial
links, large CCSDS frames for space links. Per the two-regime model, a larger
segment helps most when the node is CPU-limited and only a little on a fast link
at full clock, so there is no need to chase very large segments. **Aggregation**
and **export sessions** are sized from the link's bandwidth-delay product; the
full procedure, worked examples, and a calculator are in
[LTP UDP multisend tuning](ltp-udp-multisend-tuning.md#configuring-ltp-for-high-throughput)
and [Using the LTP Configuration Tool](../using-ltp-config-tool.md). The exact
`ltpadmin` command syntax is in the Deployment Guide.

### Reliability costs time, not throughput

On a lossy link, bound how long LTP keeps retrying a block with the repair-round
budget described next; it trades worst-case delivery time against giving up
early and does not change steady-state throughput on a clean link.

### Quick checklist

- Built with optimization on (a custom `CFLAGS` has not dropped `-O`).
- `performance` CPU governor on sender and receiver.
- One ION node per CPU for peak throughput.
- SDR run non-reversible where crash-consistency is not needed (or, if it is, an
  in-memory undo log rather than a file one).
- Segment size matched to the link MTU.
- Kernel socket buffers raised before pushing UDP.
- Aggregation and export sessions sized for the bandwidth-delay product.
- Do not expect a single flow to scale with core count.

## Bounding session repair

> For the background — what this replaced, and why the budget is now stated
> rather than derived — see
> [Bounding LTP Session Repair](ltp-repair-rounds.md).

When an LTP session loses segments, it repairs them in **rounds**: one exchange
of reception report and retransmission. `m maxrepairrounds` sets how many
rounds a session may consume before the block is abandoned and left to BP to
re-forward. It is set globally, or per span with
`m span <engine ID> maxrepairrounds`, must be between 1 and 256, and **defaults
to 8**.

The parameter is worth thinking of as a *time* budget rather than a retry
count, because that is what it costs:

```
worst-case time to abandon a block  =  maxrepairrounds x acknowledgement deadline
```

and the acknowledgement deadline is dominated by round-trip light time. At the
default of 8 rounds that is roughly 16 s on a LEO link, 40 s lunar, and 5.3 h
at Mars distance. It also bounds how long an export session slot is held, which
matters if `max_export_sessions` is provisioned tightly — a long-haul span is
the case for a per-span override.

**Why 8.** The default is calibrated against a design ceiling of **5% segment
loss**, on the reasoning that a space link losing more than that is too degraded
to carry useful traffic and is better abandoned than repaired. At that ceiling,
8 rounds leave an expected residual below three undelivered segments in a
million even for a 100 MiB block, and below three in a hundred million for
1 MiB. Raising the value costs nothing for sessions that complete without loss,
because the limit is a ceiling rather than a schedule — only a session that
would otherwise have been abandoned consumes the extra allowance.

**Relationship to `m maxseglossrate`.** The loss rate no longer determines how
many rounds a session gets; it determines how many reception *reports* each
round is allowed. A report can carry only a limited number of reception claims,
so a large block with many gaps needs several reports to describe them — a
100 MiB block at a 20% loss rate needs some 955 report segments, but only across
a handful of rounds. Reports are cheap; rounds are what consume time. This is
why the tunable is on rounds.

**Inspecting the result.** `l span` in `ltpadmin` reports each span's resolved
retransmission configuration and the session budget it produces for a nominal
64 KiB block, so the effect of a change is visible immediately and two engines'
settings can be diffed against each other.

**Peer interaction.** LTP negotiates none of this. A receiving engine allows
itself extra rounds beyond its own configured limit, so in normal operation the
*sending* engine's limit governs and it is sufficient to configure each engine
for the traffic it sends. That allowance depends on both engines running a
release that implements `maxrepairrounds`; where one end is older, that end's
budget will govern in the direction in which it is receiving.

## See also

- `demos/bench-ltp-xlsa/doc/RESULTS.md` — raw xlsa benchmark data, impairment
  sweeps, caveats, and how to reproduce.
- `ltp/xlsa/doc/DESIGN.md` — the xlsa shared-memory link service design.
- [LTP UDP multisend tuning](ltp-udp-multisend-tuning.md) — `UDP_MULTISEND`
  configuration that underlies the UDP numbers above.
- The **LTP Configuration Tool** spreadsheet
  (`doc/ION-LTP-configuration_tool.xlsm`, shipped with each ION release) — for
  computing LTP settings for a specific link's delay/bandwidth/error rate.
  **Note:** this tool is built around the `m maxber` (maximum bit-error-rate)
  parameter, which ION 4.2 has **deprecated** in favor of setting the segment
  loss rate and retry count directly (`m maxseglossrate` / `m maxretries`;
  unified-mode defaults are `maxSegmentLossRate = 0.01` and `maxRetries = 5`).
  Until the spreadsheet is updated, use its computed **segment error rate**
  (from the *Link* worksheet) as the `m maxseglossrate` value — that is the
  quantity the tool derives internally before it converts to maxBER — rather
  than feeding the maxBER output to the deprecated `m maxber` command. Note also
  that with `m maxseglossrate` the loss rate is set *directly* and no longer
  tracks the segment size automatically (as it did under maxber), so it must be
  recomputed if the segment size changes. The spreadsheet also predates
  `m maxrepairrounds` (see [Bounding session repair](#bounding-session-repair)),
  which is what now determines how many repair rounds a session may consume;
  the loss rate it computes still governs how many reception reports each round
  is allowed.
