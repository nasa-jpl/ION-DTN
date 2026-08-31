# Subsecond contact-plan prototype

This branch contains an experimental millisecond-resolution extension of the
ION contact timeline.  It builds on the separately committed millisecond OWLT
work described in `SUBSECOND_OWLT.md` and retains the original whole-second C
APIs as wrappers.

## Supported scope

- `ionadmin` accepts absolute contact timestamps with one to three fractional
  digits, for example `2026/03/25-14:30:00.500`.
- Persistent contacts, volatile contact cross-references, timeline events,
  clock-error-adjusted boundaries, and CGR calculations retain milliseconds.
- Contact list and briefing output preserve the fractional timestamp.
- Exact fractional contact keys can be revised and removed.
- `rfxclock` dispatches timeline events using a configurable millisecond poll
  interval.  `ION_RFXCLOCK_POLL_MSEC` defaults to `10` and accepts `1` through
  `1000`.
- `m clockerr` accepts non-negative seconds with up to three fractional digits.
- `m utcdelta` accepts signed seconds with up to three fractional digits,
  including values such as `-0.125`.
- Existing whole-second contact plans and the original whole-second public APIs
  remain supported.

## Fixed-point representation

Contact timestamps use normalized pairs:

```text
timestamp = whole Unix seconds + milliseconds / 1000
milliseconds = 0..999
```

UTC delta uses a signed remainder so that values between `-1` and `0` seconds
are representable:

```text
UTC delta = whole seconds + signed milliseconds / 1000
signed milliseconds = -999..999
```

For example, `-0.125` is stored as whole seconds `0` and signed milliseconds
`-125`.

## Deliberate limits

- LTP timers and retransmission calculations remain whole-second and are not
  part of this milestone.  UDPCL is the validated convergence layer.
- CPS cannot announce fractional contact timestamps with its current notice
  structure.  Fractional local plans must therefore be administered locally.
- NM management controls retain their legacy whole-second schema.  Fractional
  `clockerr` and `utcdelta` are administered through `ionadmin`.
- The millisecond scheduler increases `rfxclock` wakeups.  The default 10 ms
  polling interval trades dispatch precision for bounded CPU overhead.
- For a non-adjacent local contact, clock-error protection yields the usable
  interval `[start + clockerr, end - clockerr]`.  The planned duration must
  therefore exceed twice `clockerr`; short contacts should configure a
  measured fractional value such as `m clockerr 0.050` rather than retaining
  the one-second default.
- Persistent structure layouts changed.  Stock and subsecond SDR databases are
  not binary compatible.  Always initialize a fresh node database when moving
  between builds; no in-place migration is provided.

## Verification

All tests create isolated node-list and working directories under `/tmp`, so
they validate fresh SDR initialization without touching an installed node.

```sh
tests/subsecond-contact/run-parser.sh /absolute/path/to/ion-build
tests/subsecond-contact/run-runtime.sh /absolute/path/to/ion-build
CLOCK_ERROR_TEXT=0.050 CLOCK_ERROR_MS=50 \
  tests/subsecond-contact/run-runtime.sh /absolute/path/to/ion-build
CONTACT_MODE=legacy \
  tests/subsecond-contact/run-runtime.sh /absolute/path/to/ion-build
tests/subsecond-contact/run-cgr-boundary.sh /absolute/path/to/ion-build
tests/subsecond-contact/run-utcdelta.sh /absolute/path/to/ion-build
```

The gates cover parsing, persistent and volatile storage, briefing output,
exact revise/delete, live start/stop dispatch, fractional clock error, signed
UTC delta application, CGR boundary choice, and whole-second compatibility.
