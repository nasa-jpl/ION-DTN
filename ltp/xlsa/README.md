# Generic LTP Link Service Template (`xlso` / `xlsi`)

A transport-agnostic ION LTP link service — a drop-in replacement for
`udplso`/`udplsi` whose lower layer is one swappable file. Ships with two
backends:

- **`xport_shm`** — a shared-memory datagram ring with knobs for one-way
  delay, jitter, drop, and emulated radio rate. The recommended substrate
  for benchmarking ION's LTP engine on a single host without the
  Linux network stack contaminating the measurement.
- **`xport_serial`** — a length-framed byte-stream backend that runs over a
  real serial line (or a `socat` pty pair, for loop testing). This is the
  starting skeleton for adapting LTP to a UART, SpaceWire link, FPGA FIFO,
  or any other byte-oriented bus.

The full analysis and rationale — what an LTP link service is in ION, how
flow control flows from BP through LTP into the radio, why UDP loopback is
the wrong benchmark substrate, and what an integrator must implement to
carry LTP over a custom lower layer — is in [`doc/DESIGN.md`](doc/DESIGN.md).
This README covers build and quick-start only.

## What you actually implement

Five functions in [`xlsa.h`](xlsa.h): `xport_open`, `xport_send`,
`xport_recv`, `xport_wake`, `xport_close`. Nothing in `xlso.c`, `xlsi.c`, or
`libxlsa.c` needs to change for a new backend. See DESIGN.md §4 for the
contract each function must meet.

## Build

**Inside the ION tree (recommended, authoritative):**

```
./configure --with-xlsa-backend=shm       # benchmark substrate (default)
./configure --with-xlsa-backend=serial    # hardware / realism check
make xlso xlsi
```

**Standalone, outside the ION build (fast iteration on a backend):**

```
make -f Makefile.standalone BACKEND=shm    ION_ROOT=/path/to/ion
make -f Makefile.standalone BACKEND=serial ION_ROOT=/path/to/ion
```

`ION_ROOT` is wherever you `make install`ed ION. The standalone path is
provided for hacking on backends without rebuilding the rest of ION; the
autotools path above is the supported build.

## Quick-start: matched-rate shm benchmark, two nodes on one host

1. **Configure two ION instances** (engine IDs 1 and 2) with the `'xlso shm:...'`
   and `'s xlsi shm:...'` strings shown in DESIGN.md §4.3. The endpoint name
   identifies one direction of the ring; use mirrored names so node 1's TX
   ring is node 2's RX ring (and vice versa):

   ```
   # node1.ltprc, engine id 1
   1 32
   a span 2 10 10 64000 65536 1 'xlso shm:ring_1to2 2'
   s 'xlsi shm:ring_2to1'

   # node2.ltprc, engine id 2
   1 32
   a span 1 10 10 64000 65536 1 'xlso shm:ring_2to1 1'
   s 'xlsi shm:ring_1to2'
   ```

2. **Declare a non-zero contact rate** in `ionadmin` for each direction —
   without it the LSO blocks forever (DESIGN.md §2.2c). Set the contact's
   `xmitRate` equal to `XLSA_RATE_BPS` for a matched-rate baseline.

3. **Set the link rate** before starting either node:

   ```
   export XLSA_RATE_BPS=10000000   # 10 Mbit/s
   export XLSA_SLOTS=1024          # rate-buffer depth, segments
   ```

   `XLSA_SLOTS` must agree across both processes on a ring (see DESIGN.md §5.5).

4. **Start both nodes.** Either order is fine — the create-race on the shm
   segment is resolved internally.

5. **Sweep impairments** one at a time:

   | Variable | Default | Purpose |
   |---|---|---|
   | `XLSA_DELAY_US` | 0 | One-way propagation delay per segment, µs |
   | `XLSA_JITTER_US` | 0 | Uniform ± jitter on delay |
   | `XLSA_DROP_PPM` | 0 | Drop probability, parts per million |
   | `XLSA_RATE_BPS` | 0 (unlimited) | Emulated radio rate, B/s |
   | `XLSA_SLOTS` | 1024 | Rate-buffer depth in segments |

   Record the Throttle `nominalRate`, LTP `localXmitRate`, and `XLSA_RATE_BPS`
   for every run; DESIGN.md §3.5 explains why all three matter.

## Quick-start: realism check over a `socat` pty pair

1. Create a virtual point-to-point serial link:

   ```
   socat -d -d pty,raw,echo=0,b115200 pty,raw,echo=0,b115200
   ```

   `socat` prints two pty paths (e.g. `/dev/pts/3` and `/dev/pts/4`); leave it
   running.

2. **Build with `BACKEND=serial`** and configure your two ION instances to
   reference the ptys instead of shm rings:

   ```
   # node1.ltprc, engine id 1
   1 32
   a span 2 10 10 64000 65536 1 'xlso serial:/dev/pts/3 2'
   s 'xlsi serial:/dev/pts/3'

   # node2.ltprc, engine id 2
   1 32
   a span 1 10 10 64000 65536 1 'xlso serial:/dev/pts/4 1'
   s 'xlsi serial:/dev/pts/4'
   ```

3. **Same ION config otherwise.** Keep `bpadmin`/`ipnadmin`/`ionadmin`
   identical between the shm and serial runs so the only varying factor is
   the backend.

Use this to capture the cost of framing and partial reads relative to the
clean shm baseline. The serial backend does not model link delay/drop/rate
— those belong in the shm substrate for controlled sweeps.

## Adapting to real hardware

Replace `xport_serial.c` (or copy it to e.g. `xport_uart.c` and pass
`BACKEND=uart` to `Makefile.standalone`, or add the same name as a valid
`--with-xlsa-backend=` value in `configure.ac`) and supply your device
I/O for the two single-shot helpers `read_bytes`/`write_bytes`. The
deframer, self-pipe wake, and shutdown flow stay as-is. Before trusting
it on a real link, add a sync marker for resynchronization after bit
errors and a CRC over each frame.

## File map

| File | Role |
|---|---|
| [`xlsa.h`](xlsa.h) | The transport seam: five `xport_*` functions. |
| [`xlso.c`](xlso.c) | Output daemon (engine coupling — do not modify). |
| [`xlsi.c`](xlsi.c) | Input daemon (engine coupling — do not modify). |
| [`libxlsa.c`](libxlsa.c) | Receiver thread. |
| [`xport_shm.c`](xport_shm.c) | Shared-memory ring backend; benchmark substrate. |
| [`xport_serial.c`](xport_serial.c) | Length-framed byte-stream backend; hardware template. |
| [`Makefile.standalone`](Makefile.standalone) | Out-of-tree build: `make -f Makefile.standalone BACKEND=shm\|serial`. |
| [`doc/DESIGN.md`](doc/DESIGN.md) | Full analysis, integration guide, and design. |

## Caveats before you build on this

- Reference code, written for clarity over throughput. See DESIGN.md §7.
- The shm backend assumes a same-host, trusted benchmark; it does not
  validate ring contents.
- The serial framer is a length prefix only — add a sync word and CRC
  before any real link.
- Grounded against `nasa-jpl/ion-dtn`, `integration` branch, commit
  `87c34fb`. Re-check `ltpP.h` and the `ltpadmin` span syntax against the
  branch you build on.
