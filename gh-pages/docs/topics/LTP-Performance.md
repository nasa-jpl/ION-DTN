# LTP Performance

Consolidated reference for measured ION LTP throughput. It pulls together
results that were previously scattered across the Deployment Guide (ION 4.1.2
and 4.2-alpha, LTP over UDP) and the `bench-ltp-xlsa` demo (LTP over the
shared-memory link service), organized by the three dimensions that all of
them sweep: **bundle size, LTP segment size, and platform**.

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
  recomputed if the segment size changes.
