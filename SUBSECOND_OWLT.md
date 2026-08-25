# Subsecond OWLT fork

This branch is an experimental, backward-syntax-compatible extension of
ION Open Source 4.2.0-b.  It addresses OWLT precision without changing contact
start and stop timestamps.

## Supported milestone

- `ionadmin` accepts integer OWLT values exactly as stock ION does.
- `ionadmin` additionally accepts fixed-point values with one to three
  fractional digits, such as `0.006` seconds.
- The persistent `IonRange`, volatile `IonRXref`, symmetric imputed range, and
  `IonNeighbor` path retain the millisecond component.
- The public C API offers `ion_add_range_ms()` and `rfx_insert_range_ms()` while
  retaining the original whole-second functions as wrappers.
- CGR earliest-arrival and projected-bundle-arrival calculations use the
  fractional OWLT.
- Range listing and briefing export preserve the fixed-point value.

## Deliberate limits

- Contact start and stop timestamps remain `time_t` values.
- Fractional range announcements through CPS are rejected rather than silently
  losing precision.
- LTP's event timeline and retransmission timers remain whole-second.  UDPCL is
  the supported convergence layer for this milestone.
- Existing stock SDR databases are not binary compatible because persistent
  range structures gained a field.  Initialize a fresh node database when
  switching between stock and subsecond builds.

## Time model

The range is represented as a fixed-point pair:

```text
OWLT = whole seconds + fractional milliseconds / 1000
```

For a 6 ms link:

```text
whole seconds           = 0
fractional milliseconds = 6
CGR OWLT                 = 0.006 seconds
```

Linux/NetEm remains responsible for applying the physical packet delay.  This
fork makes CGR's route-time model use the same OWLT rather than zero or one
second.

## Verification

Build `libici`, `ionadmin`, `rfxclock`, `libcgr`, and the BP applications, then
run:

```sh
tests/subsecond-owlt/run-smoke.sh /absolute/path/to/ion-build
```

The test passes only if both the asserted and symmetric imputed ranges print as
`0.006 seconds` and the briefing export retains `0.006`.
