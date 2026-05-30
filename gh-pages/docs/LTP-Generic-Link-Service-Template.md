# Benchmarking ION LTP Over a Custom Lower Layer

### Analysis, integration guide, and design of a generic link-service emulation template

*Grounded against `nasa-jpl/ion-dtn`, `integration` branch, commit `87c34fb` (2026-05-29).*

---

## 1. Why this document exists

When ION flies, LTP does not run over UDP. The LTP link service talks to a bus, a serial line, a UART, a SpaceWire interface, or an FPGA — a byte-oriented lower layer with its own framing, rate, and error behavior. For bench testing on a single host we want to measure ION's own LTP engine (segmentation, reassembly, ZCO handling, report/checkpoint timers, span throughput) **without** the confounding behavior of the host IP stack.

UDP-on-loopback is a poor substrate for that measurement, and ION's actual link-service contract is small enough that a better one is easy to build. This document covers three things:

1. **Analysis** — what an LTP link service actually is in ION, and why UDP loopback distorts a benchmark.
2. **Integration guide** — what a user must implement to carry LTP over their own lower layer.
3. **Design** — a generic, transport-agnostic template (`xlso`/`xlsi` plus a transport "seam") with a reference shared-memory emulator and a byte-stream hardware-adaptation example.

The source files live under [`ltp/xlsa/`](https://github.com/nasa-jpl/ion-dtn/tree/integration/ltp/xlsa) in the ION tree; this document explains them. See also the companion [LTP Underlying Communications API](LTP-UComm-API.md) page for the engine-side contract this template builds on.

### Terminology and layering

ION's stack uses these terms precisely, and this document follows them:

```
   BP  ──CLA──▶  CL (e.g. LTP)  ──LS──▶  UCS / radio
```

- **CLA** — *Convergence Layer Adapter*: the interface between BP and a convergence layer. In ION the LTP CLA is `ltpcli` (input) / `ltpclo` (output). **This document is not about the CLA.**
- **CL** — *Convergence Layer*: LTP itself (also BSSP, STCP, TCPCLv4, etc.).
- **LS** — *Link Service* (a.k.a. *UCS*, Underlying Communication Service): the layer **below** a CL that actually moves bytes. Its two daemons are the **LSO** (Link Service Output) and **LSI** (Link Service Input). The UDP examples are `udplso`/`udplsi`.
- **UCS / radio** — the physical underlying service the LS drives (UDP socket in the stock build; a bus, serial line, or radio in flight).

The template here implements a **link service** (the `xlso`/`xlsi` daemons and a swappable transport backend), sitting under LTP. It does not touch the CLA. When this document says "link service," it means the LSO/LSI layer below LTP — not the BP↔CL adapter.

---

## 2. Analysis: what an LTP link service is

### 2.1 The contract

In ION an LTP link service is nothing more than **two daemons bound to a span**. The engine is wholly indifferent to how bytes physically move. The entire interface, confirmed in `ltp/library/ltpP.h`, is two functions:

```c
extern int  ltpDequeueOutboundSegment(LtpVspan *vspan, char **buf);
extern int  ltpHandleInboundSegment(char *buf, int length);
```

**Output daemon (LSO).** Modeled on `ltp/udp/udplso.c`. Once at startup it resolves its span:

```c
findSpan(remoteEngineId, &vspan, &vspanElt);
```

then loops:

```c
segmentLength = ltpDequeueOutboundSegment(vspan, &segment);  /* blocks */
/* ship segment[0..segmentLength) to the peer somehow */
```

**Input daemon (LSI).** Modeled on `ltp/udp/udplsi.c` plus its receiver thread in `ltp/udp/libudplsa.c`. It loops:

```c
segmentLength = <receive one segment from the transport>;
ltpHandleInboundSegment(buffer, segmentLength);
```

That is the whole surface. Everything else in the UDP driver — DNS resolution, IPv6 dual-stack handling, the token-bucket rate limiter, `sendmmsg`/`recvmmsg` batching, `EAGAIN`/`ENOBUFS` retry logic — is UDP-specific scaffolding, *not* part of the engine contract.

### 2.2 Three details that matter for a correct replacement

Reading the engine internals (`ltp/library/libltpP.c`) surfaces three properties any replacement transport must respect:

**(a) LTP is datagram-oriented — boundaries are sacred.** `ltpHandleInboundSegment()` expects *exactly one complete segment per call*. It does **not** reassemble a byte stream. A datagram transport (UDP, a shared-memory slot ring) preserves boundaries for free. A byte-stream transport (TCP, serial, UART, FIFO) does not — you **must** frame on send and deframe on receive. This is the single most important correctness property, and the original reason UDP was chosen.

**(b) The dequeue buffer is engine-owned.** `ltpDequeueOutboundSegment()` sets `*buf` to point at `vspan->segmentBuffer`, which the engine allocates at exactly `span.maxSegmentSize` (see `libltpP.c`: `vspan->segmentBuffer = psm_malloc(ltpwm, span.maxSegmentSize)`). The LSO must copy or transmit the bytes before the next dequeue; it must not free or over-write past `maxSegmentSize`. Your transport's maximum transfer unit must therefore be at least the span's `maxSegmentSize`.

**(c) Dequeue blocks on the segment semaphore *and* on rate.** The dequeue loop waits while `elt == 0 || vspan->localXmitRate == 0`. Two consequences: a return value of `0` means "interrupted or stopped" (check `sm_SemEnded(vspan->segSemaphore)` to tell which), and **if the contact plan gives the span a zero transmit rate, the LSO will block forever**. ION uses this same mechanism in flight to suspend transmission cleanly during off-contact periods, so the behavior is correct — but a benchmark must declare a contact with a non-zero `localXmitRate` for any segments to flow.

### 2.3 Why UDP loopback distorts the benchmark

Running two ION instances over UDP on `127.0.0.1` drags the following into your measurement:

- **The IP/UDP stack.** Checksums, the loopback MTU, and the per-segment `IPHDR_SIZE` accounting the LSO performs.
- **Socket-buffer backpressure.** `SO_SNDBUF`/`SO_RCVBUF` limits surface as `ENOBUFS`/`EAGAIN`. The current `udplso.c` reacts with retry loops (`UDPLSO_EAGAIN_RETRIES`, `microsnooze`) and, in `sendBatch`, treats a range of errnos as silent packet loss. All of this perturbs timing in ways unrelated to LTP.
- **An LSO-side token bucket.** `udplso.c` carries `applyTokenBucket()` with a `NANOSLEEP_FLOOR_USEC` floor that deliberately oversleeps and carries a deficit forward. That is link pacing happening *inside the link service*, mixed into any throughput number you collect.
- **Kernel scheduling of the loopback softirq path**, independent of ION.

The net effect: you end up benchmarking the Linux network stack and the UDP driver's compensation logic, not ION's LTP engine. For flight-representative numbers you want a substrate that (1) preserves segment boundaries, (2) involves no kernel networking, and (3) lets you model link delay/rate/loss **deterministically and in one place**.

---

## 3. Flow control across BP → LTP → LSO → radio

Before benchmarking, it is worth understanding that rate control in ION is not a single mechanism in one place. Several layers each pace themselves against a *notional* expected rate so they do not wildly over-produce, while the radio enforces the *actual* rate. The queues between layers absorb the difference. Getting the notional rates to agree with the radio is what keeps the system in steady state.

The chain, top to bottom, as implemented in the `integration` branch:

### 3.1 BP layer — `bpclm` meters against a Throttle

Each egress plan has a `bpclm` daemon (`bpv7/daemon/bpclm.c`). Before forwarding a bundle to its convergence layer it consults `applicableThrottle(vplan)` and **blocks** when rate control is in force but exhausted:

```c
throttle = applicableThrottle(vplan);
... if (throttle->nominalRate > 0 && throttle->capacity <= 0) { /* wait on vplan->semaphore */ }
... throttle->capacity -= computeECCC(guessBundleSize(&bundle));   /* after forwarding */
```

The `Throttle` (in `ici/include/ion.h`) is simply:

```c
typedef struct { double nominalRate; double capacity; } Throttle;   /* bytes/sec, bytes */
```

`nominalRate` is sourced from `neighbor->xmitRate` — i.e. the **contact plan**. Capacity is refilled once per second by `bpclock` (`bpv7/daemon/bpclock.c`), which adds `nominalRate` (clamped to `nominalRate`) and signals the plan's `bpclm` semaphore when capacity rises from `<= 0` to `> 0`. **Effect:** BP paces its handoff into the convergence layer to the notional rate, so it does not over-produce bundles into the duct.

### 3.2 LTP layer — `ltpmeter` gates on aggregation, then feeds segments

`ltpmeter` (`ltp/daemon/ltpmeter.c`) is the LTP-side producer. It blocks on the span's `bufClosedSemaphore` until the aggregation buffer closes — either `lengthOfBufferedBlock >= aggrSizeLimit` or the aggregation time limit fires:

```c
if (span.lengthOfBufferedBlock < span.aggrSizeLimit) { /* wait on bufClosedSemaphore */ }
...
/* segment the closed block, "giving the span's segSemaphore once per segment" */
```

So LTP throttles by **block aggregation** (`aggrSizeLimit` / `aggrTimeLimit`), smoothing bundle arrivals into LTP-sized blocks and posting `segSemaphore` once per segment as it fills the SDR segment queue. `ltpmeter` does not itself enforce a byte rate.

### 3.3 LSO — the actual dequeue pacing authority

`ltpDequeueOutboundSegment()` (`ltp/library/libltpP.c`) is where the configured rate finally bites. It blocks while:

```c
while (elt == 0 || vspan->localXmitRate == 0) { sm_SemTake(vspan->segSemaphore); ... }
```

The LSO pulls one segment per `segSemaphore` post, and `localXmitRate` (again from the contact plan) gates whether it pulls at all. This is the point the integrator's `xport_send` is driven from.

### 3.4 The radio and the link service rate buffer

In flight the radio sets the true rate. The link service holds a finite rate buffer, accepts segments from the LSO at full speed until that buffer fills, then blocks — and that block backpressures up through the LSO (it stops dequeuing) into the SDR segment queue. In the emulator this is `xport_shm.c`: the ring is the rate buffer, `XLSA_RATE_BPS` is the radio rate, and ring-full is the backpressure (see §5.5).

### 3.5 Three buffers, three rates

The key insight for anyone running this rig: there are **three buffers** absorbing rate fluctuation —

1. the BP forwarding queue (paced by the `bpclm` Throttle),
2. the LTP block-aggregation buffer plus the SDR segment queue (paced by `ltpmeter` + `localXmitRate`),
3. the link service's rate buffer in front of the radio (the `xport_shm` ring),

— and **three rate parameters** that must agree for steady state:

| Parameter | Where set | Role |
|---|---|---|
| `bpclm` Throttle `nominalRate` | contact plan (`neighbor->xmitRate`, via `ionadmin`) | paces BP production into the duct |
| LTP `localXmitRate` | same contact-plan source | gates LSO dequeue; calibrates LTP timers |
| `XLSA_RATE_BPS` | emulator (env var) | the actual radio drain rate |

The first two are the *same* contact-plan number seen by two subsystems; the third is the physical link. In flight all three track the planned radio rate. In the benchmark, set the ION contact rate equal to `XLSA_RATE_BPS` for steady-state runs. The deliberate mismatches are the interesting experiments:

| Scenario | Contact rate vs. `XLSA_RATE_BPS` | What it exercises |
|---|---|---|
| **Matched** | equal | Steady state; buffers stay near-constant. Baseline throughput/latency. |
| **Over-provisioned** | contact rate **>** radio | BP/LTP over-produce; SDR segment queue and LS ring grow; backpressure reaches the LSO via `xport_send` blocking. LTP timers (calibrated to the too-high rate) may fire **spurious retransmissions** because they expect reports sooner than the slow radio delivers. |
| **Under-provisioned** | contact rate **<** radio | BP Throttle starves the pipe; link under-utilized though the radio could go faster. Measures how much goodput is lost to a conservative contact plan. |

This three-way consistency requirement — not just the two rates noted in §5.5 — is what an integrator must get right, and is the reason a benchmark should record all three values for every run.

---

## 4. Integration guide: carrying LTP over your own lower layer

This section is for a user who wants ION's LTP to run over their actual system interface. The good news from §2.1 is that you implement a *very* small seam and reuse ION's daemon skeleton verbatim.

### 4.1 What you implement

Exactly five functions, declared in `xlsa.h`:

| Function | Responsibility |
|---|---|
| `xport_open(spec, isSender, remoteEngineId)` | Acquire/open the link; return an opaque handle. |
| `xport_send(link, segment, length)` | Transmit **one** segment. Return bytes accepted, or −1 fatal. |
| `xport_recv(link, buf, maxlen)` | Block until **one** segment arrives; copy it out; return its length (`1` = shutdown sentinel, `0` = interrupted, `−1` = fatal). |
| `xport_wake(link)` | Unblock a parked `xport_recv()` so the daemon can stop. |
| `xport_close(link)` | Release the link. |

You do **not** modify `xlso.c`, `xlsi.c`, or `libxlsa.c`. They contain the engine coupling (span lookup, the dequeue loop, the receiver thread, shutdown semaphore handling) and are copied directly from the structure of the UDP driver.

### 4.2 The rules your backend must obey

1. **One segment per `xport_recv()` call.** If your medium is a byte stream, add a length-prefixed frame on send and a deframer on receive that buffers partial reads until a whole segment is present. See `xport_serial.c` for a worked minimal framer (add a sync word and CRC for any real link).
2. **MTU ≥ span `maxSegmentSize`.** If your medium has a smaller native frame, fragment/reassemble *below* the seam, inside your backend; never hand LTP a partial segment.
3. **Treat transient link conditions as loss, not error.** A momentarily full FIFO or a busy bus should be retried internally or reported as accepted (`return length`) so LTP's own retransmission recovers it. Reserve `−1` for unrecoverable faults (bad descriptor, config error). This mirrors how `udplso.c` treats `ENETUNREACH`/`ENOBUFS` as packet loss so the daemon survives link flaps.
4. **Make `xport_recv()` interruptible.** `xport_wake()` must reliably unblock it; otherwise `xlsi` cannot shut down cleanly. The reference backend enqueues a one-byte sentinel, mirroring the UDP driver's self-addressed 1-byte datagram trick.
5. **Rate/impairment belongs in the backend, modeled as the radio.** In a real flight system the radio sets the data rate; the link service holds a finite rate buffer and drains to the radio at the radio's rate. When that buffer fills, your `xport_send` blocks, which backpressures the LSO — it stops calling `ltpDequeueOutboundSegment`, and segments accumulate in the SDR span queue. **This emergent backpressure, not a token bucket, is the correct flow-control mechanism.** Do *not* replicate `udplso`'s `applyTokenBucket()` in a flight link service; pacing on the producer side fights the radio and distorts timing.

### 4.3 Wiring it into ION

The span and seat are declared through `ltpadmin` exactly as for UDP — only the command strings change. A minimal two-node, same-host config:

**Node 1 (`node1.ltprc`), engine id 1, peer engine id 2:**

```
1 32                      # ltpInit: total session buffer (est. max export sessions)
a span 2 10 10 64000 65536 1 'xlso shm:ring_1to2 2'
s 'xlsi shm:ring_2to1'
```

**Node 2 (`node2.ltprc`), engine id 2, peer engine id 1:**

```
1 32
a span 1 10 10 64000 65536 1 'xlso shm:ring_2to1 1'
s 'xlsi shm:ring_1to2'
```

The `a span` fields are: peer engine id, max export sessions, max import sessions, max segment size, aggregation size limit, aggregation time limit, then the quoted `'<LSO command>'`. `ltpadmin` also accepts an optional final **queuing latency** field after the LSO command (defaults are fine for the bench rig; check `ltpadmin` if you want to tune it). Note the **mirrored endpoint names**: node 1's transmit ring `ring_1to2` is node 2's receive ring, and vice versa — both sides reference the *same* ring name for that direction so they map the same underlying shared-memory segment (see §5.5).

You also need a contact plan (`ionadmin`) giving each direction a **non-zero transmit rate** (see §2.2c), and the usual `bpadmin`/`ipnadmin` setup to put bundles through the spans.

### 4.4 Retargeting to real hardware

Replace the backend file with one that drives your device, keeping the `xport_*` signatures:

- **Serial/UART:** open the tty, configure raw-mode termios and baud, then `read()`/`write()`. Use the framing in `xport_serial.c`.
- **FPGA/DMA FIFO:** `xport_send` writes a descriptor into the TX FIFO; `xport_recv` blocks on an RX interrupt/poll and copies one frame out. Apply the same length-prefix framing if the FIFO is byte-granular.
- **Shared bus:** map the bus window in `xport_open`; gate access with whatever arbitration the bus requires.

Because the seam is five functions, a new link service is typically a single new `xport_*.c` file plus a one-line `Makefile` change (`BACKEND=...`).

---

## 5. Design: the emulation/template system

### 5.1 Goals

- **Accurate** — measure the LTP engine, not the host network stack.
- **Generic** — one daemon skeleton, swappable transport backends.
- **A template** — the path of least resistance for a user building a real link service is to copy this and replace one file.

### 5.2 Architecture

```
        ION instance A                         ION instance B
   ┌────────────────────┐                 ┌────────────────────┐
   │   LTP engine        │                 │   LTP engine        │
   │ ltpDequeueOutbound  │                 │ ltpHandleInbound    │
   └─────────┬──────────┘                 └──────────▲─────────┘
             │ (engine-owned segmentBuffer)          │
        ┌────▼─────┐   xport_send        xport_recv  ┌┴─────────┐
        │  xlso    │──────────►  TRANSPORT  ─────────►│  xlsi    │
        │ (skeleton)│           SEAM (xlsa.h)        │(+receiver │
        └──────────┘   one segment per call          │  thread)  │
                                                       └──────────┘
                         ▲ backend implements the seam ▲
        ┌────────────────┴─────────────┐  ┌────────────┴───────────────┐
        │ xport_shm.c                   │  │ xport_serial.c              │
        │ shared-memory datagram ring   │  │ framed byte stream          │
        │ delay / jitter / drop / rate  │  │ (hardware adaptation stub)  │
        │  → BENCHMARK SUBSTRATE        │  │  → REALISM CHECK            │
        └───────────────────────────────┘  └─────────────────────────────┘
```

The **seam** (`xlsa.h`) is the only thing a backend author touches. The **skeleton** (`xlso.c`, `xlsi.c`, `libxlsa.c`) is fixed. Two **backends** ship: the shared-memory ring for clean benchmarking, and the serial framer as the hardware template.

### 5.3 The seam (`xlsa.h`)

Five functions (§4.1) plus an opaque `XportLink` handle and a receiver-thread parameter block (`xlsa_ReceiverThreadParms`) that mirrors `udp_ReceiverThreadParms` but with the socket/address fields removed — the transport is reached only through the handle. `XLSA_BUFSZ` defines the maximum transfer unit and must be ≥ the span's `maxSegmentSize`.

### 5.4 The skeleton

`xlso.c` is `udplso.c` with the UDP/DNS/IPv6/token-bucket code stripped and `sendSegmentByUDP` replaced by `xport_send`. The retained logic — `findSpan`, the `ltpDequeueOutboundSegment` loop, `xlsoSemaphore`/`shutDownLso` arming `segSemaphore`, the `segmentLength == 0` interrupted/stopped distinction — is exactly what must not be changed.

`xlsi.c` + `libxlsa.c` are `udplsi.c` + the single-recv branch of `libudplsa.c`: spawn a receiver thread that loops `xport_recv()` → `ltpHandleInboundSegment()`, with the buffer freed through a `pthread_cleanup` handler so a cancelled thread does not leak ION working memory. The `case 1`/`case 0`/`case -1` handling of the receive return value is preserved verbatim from the UDP driver.

### 5.5 Reference backend: shared-memory datagram ring (`xport_shm.c`)

This is the recommended benchmark substrate.

- **Boundary-preserving.** Each ring slot holds one segment plus its length, so `xport_recv` returns one segment per call with no framing.
- **No kernel networking.** A POSIX `shm_open`/`mmap` region with a process-shared mutex + condition variables. No IP, no sockets, no MTU, no socket-buffer backpressure.
- **One shared segment per direction.** The endpoint name (e.g. `ring_1to2`) names a single shm object (`/xlsa.ring_1to2`). Node A's `xlso` writes it; node B's `xlsi` reads it. There is **no `.tx`/`.rx` suffixing** — both sides open the same object — and both sides may start in either order; the create-race is resolved internally (see "startup ordering" below).
- **Impairments modeled in one place**, configured by environment variable so a run needs no recompile:

| Variable | Effect |
|---|---|
| `XLSA_DELAY_US` | One-way propagation delay per segment (µs) — emulate light time. |
| `XLSA_JITTER_US` | Uniform ± jitter added to delay. |
| `XLSA_DROP_PPM` | Drop probability (parts per million) — exercises LTP retransmission. |
| `XLSA_RATE_BPS` | Emulated radio rate (bytes/s); `0` = unlimited. |
| `XLSA_SLOTS` | Rate-buffer depth in segments (default 1024) — the link service's buffering capacity; smaller makes backpressure bite sooner. |

Delay/jitter are implemented by stamping each slot with an earliest-delivery time that the receiver honors via `pthread_cond_timedwait`. **Bandwidth is modeled the way a flight link actually behaves**: the emulated radio is the rate authority, the ring is the link service's finite rate buffer (`XLSA_SLOTS` = its depth in segments), and the link rate is enforced by the *spacing of slot release times* on the receive side — a serialization clock advanced by each segment's transmit time. `xport_send` itself never sleeps; it only blocks when the ring is full, and that blocking is the backpressure that throttles the LSO (which then stops dequeuing, so segments pile up in the SDR). The LSO carries no pacing logic, so throughput reflects engine + modeled radio only. Drop discards before enqueue and reports success so LTP — not the emulator — recovers the loss.

A subtlety worth stating explicitly: there are **two** rates in play. `XLSA_RATE_BPS` is the *actual* link rate the emulator enforces. ION's contact-plan `localXmitRate` (set via `ionadmin`) is separate — it gates the dequeue loop (must be > 0) and calibrates LTP's retransmission timers (§2.2c). In flight these match the planned radio rate, so set the contact rate equal to `XLSA_RATE_BPS` for representative timer behavior. Deliberately mismatching them is itself a valuable test: it emulates the radio degrading below the rate ION was told to expect, and exposes how LTP's timers react when its rate model diverges from reality.

**Startup ordering and the create-race.** Either side may start first, but the two ION instances must not access the in-ring mutex/condvars until the creator has finished initializing them. `xport_open` handles this without external synchronization:

1. Both sides race on `shm_open(O_CREAT | O_EXCL)`. The winner becomes the *creator*; the loser becomes the *opener*.
2. The creator `ftruncate`s the segment to `mapLen`, `mmap`s it, zeroes it, initializes the process-shared mutex/condvars, then issues a full memory barrier and publishes the `slots` field **last**. `slots` is the init-complete sentinel.
3. The opener polls `fstat` until the segment has been truncated to `mapLen`, then `mmap`s and polls the `slots` field until non-zero. After that the mutex and condvars are guaranteed initialized.
4. If the opener arrives in the narrow window between the creator's `O_EXCL` succeeding and the file actually being created (or after an `shm_unlink` race), `shm_open` returns `ENOENT` and the opener backs off and retries the whole sequence.

Both polls have a ~1-second cap; exceeding it is reported as a fatal init error. The opener also verifies that the creator's `slots` matches its own `XLSA_SLOTS` and aborts on mismatch — disagreeing slot counts produce mismatched `mapLen` values and step out of bounds. **All processes on a ring must use the same `XLSA_SLOTS`.**

Stale segments from a previous (crashed) run live across reboots' `tmpfs` lifetimes. If a fresh run hangs in init, `rm /dev/shm/xlsa.*` (Linux) or restart to clear them.

### 5.6 Realism check backend: framed byte stream (`xport_serial.c`)

The shared-memory ring is optimistic in one respect: it never exercises framing/deframing or partial reads, which a real serial/FPGA link *will*. `xport_serial.c` is the antidote and the hardware template: a minimal 4-byte big-endian length frame on send, and a deframer on receive that accumulates bytes across partial `read()`s until a whole segment is present. Swap the `read_bytes`/`write_bytes` stubs for your device I/O and it becomes a working link service skeleton. Add a sync word and CRC before trusting it on a real link.

**Local wake via self-pipe.** `xport_wake` does **not** write to the device fd — that would inject bytes onto a real wire, which a flight link service must never do. Instead `xport_open` creates a non-blocking `pipe()` and `xport_recv` `poll()`s the device fd and the pipe's read end together. `xport_wake` writes one byte to the pipe's write end; `xport_recv` notices on the next `poll`, drains the pipe, and returns the shutdown sentinel. The mechanism is purely local — the peer never sees it.

**Loop testing against a `socat` pty pair.** Because wake-up is local and the device fd is bidirectional, two daemons can talk over a virtual serial link without hardware. Run

```
socat -d -d pty,raw,echo=0,b115200 pty,raw,echo=0,b115200
```

and `socat` prints two pty paths (e.g. `/dev/pts/3` and `/dev/pts/4`); point one ION instance's `xlso`/`xlsi` at one path and the other instance's at the other:

```
# node 1: 'xlso serial:/dev/pts/3 2'   's xlsi serial:/dev/pts/3'
# node 2: 'xlso serial:/dev/pts/4 1'   's xlsi serial:/dev/pts/4'
```

(or use distinct devices per direction if your topology runs full-duplex). This exercises the framing/deframing path end-to-end and is the substrate for §5.7's realism-check step.

### 5.7 Recommended benchmark methodology

1. Use `xport_shm` as the **primary** substrate for clean, repeatable engine numbers.
2. Set the three rates consistently first (§3.5): make the ION contact rate equal `XLSA_RATE_BPS` for a matched-rate baseline, and record all three values (Throttle `nominalRate`, LTP `localXmitRate`, `XLSA_RATE_BPS`) for every run.
3. Sweep one impairment at a time (`XLSA_RATE_BPS`, then `XLSA_DELAY_US`, then `XLSA_DROP_PPM`) and watch LTP behavior: goodput vs. offered rate, report/checkpoint timer activity, retransmission counts under loss.
4. Run the rate-mismatch matrix from §3.5 (matched / over-provisioned / under-provisioned) to characterize buffer growth, backpressure onset, and spurious retransmissions when ION's rate model diverges from the radio.
5. Re-run a subset over `xport_serial` against a `socat` pty pair (see §5.6) as a **realism check** to capture framing/partial-read cost.
6. Keep the ION config identical across substrates so only the backend varies; this isolates the transport's contribution.

---

## 6. File manifest

| File | Role |
|---|---|
| `xlsa.h` | The transport seam: five `xport_*` functions + handle/types. |
| `xlso.c` | Generic LTP output daemon (skeleton; do not modify). |
| `xlsi.c` | Generic LTP input daemon (skeleton; do not modify). |
| `libxlsa.c` | Receiver thread; `xport_recv` → `ltpHandleInboundSegment`. |
| `xport_shm.c` | Reference shared-memory ring backend with impairment knobs. |
| `xport_serial.c` | Framed byte-stream backend (hardware-adaptation example; loop-testable over a `socat` pty pair). |
| `Makefile` | Build `xlso`/`xlsi`; select backend with `BACKEND=shm|serial`. |
| `README.md` | Quick-start: build, configure, and run a matched-rate benchmark or the realism check. |

---

## 7. Caveats

- The reference `xport_shm.c` is written for clarity, not maximum throughput; it uses a single mutex/condvar per ring. For very high-rate runs you may want a lock-free SPSC ring, but verify that change does not itself become the bottleneck you are trying to measure.
- The shared-memory backend assumes a trusted same-host benchmark; it performs no validation of ring contents. Do not use it as-is across a trust boundary.
- The serial framer is deliberately minimal (length prefix only). A real link needs a sync marker for resynchronization after a bit error and a CRC to reject corrupted frames before they reach `ltpHandleInboundSegment`.
- `XLSA_BUFSZ` is `(256 * 256) - 1 = 65535` bytes — enough headroom for a 64 000-byte `maxSegmentSize` plus a small framing header. If you push the span's `maxSegmentSize` higher, bump `XLSA_BUFSZ` accordingly *and* re-check the byte-stream framer's accumulator size, which is dimensioned from the same constant.
- These templates were structured against the `integration` branch at commit `87c34fb`. The LSO/LSI contract has been stable for a long time, but re-check `ltpP.h` and the `ltpadmin` span syntax against the branch you build on.
