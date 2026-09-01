# Optional subsecond contact planning

ION contact and range records normally use whole-second values. This optional
extension retains the legacy syntax and public wrappers while allowing
millisecond precision where short contacts or short one-way light times need
it.

## Configuration syntax

Absolute contact timestamps may contain one to three fractional digits:

```text
a contact 2026/03/25-14:30:00.500 2026/03/25-14:30:01.200 1 2 100000 1.0
```

Range OWLT, `clockerr`, and signed `utcdelta` values may also contain up to
three fractional digits:

```text
a range +0 +300 1 2 0.006
m clockerr 0.050
m utcdelta -0.125
```

Omitted fractions are zero. Inputs with more than three fractional digits are
rejected rather than rounded silently.

## Representation and runtime behavior

Fractional values are stored as normalized whole-second and millisecond
components. The millisecond component is carried through persistent and
volatile contact/range records, timeline events, contact revise/delete/list
operations, CGR contact boundaries, and CGR OWLT calculations. Existing
whole-second C entry points remain wrappers around the millisecond-aware
entry points.

`rfxclock` preserves its legacy 1000 ms polling interval by default. A node
that uses fractional boundaries can opt into a shorter interval:

```sh
export ION_RFXCLOCK_POLL_MSEC=10
```

The accepted range is 1 through 1000 ms. The configured interval bounds event
dispatch resolution; it is not a scheduling-latency guarantee. Legacy
housekeeping remains limited to once per second even when the contact
timeline is polled more frequently.

## Compatibility and scope

- Existing whole-second contact plans remain accepted.
- Persistent contact, range, event, and database layouts changed. Initialize
  a fresh SDR/node database when switching between stock and subsecond builds;
  there is no in-place migration.
- UDPCL is the convergence layer validated by this milestone.
- LTP timers and retransmission calculations remain whole-second.
- CPS and NM control schemas remain whole-second. Fractional administration is
  local through `ionadmin`.
- Linux/NetEm remains responsible for applying packet propagation delay. The
  fractional range makes CGR use the corresponding OWLT in route-time
  calculations.
- Smaller polling intervals increase process wakeups. Operators should select
  the coarsest interval that satisfies the required boundary precision.

## Regression tests

The source tree provides two independently discoverable regression tests:

```sh
cd tests
./runtests subsecond-contact
./runtests subsecond-owlt
```

They cover parsing, persistence, export, exact revise/delete, live contact
start/stop, fractional clock error, signed UTC delta, CGR contact-boundary and
OWLT route selection, and legacy whole-second behavior. Every test creates a
fresh temporary node database and removes it on exit.
