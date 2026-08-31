# Subsecond contact-plan validation

This report records the validation performed for commit
`078d2610bbabe0a8183b282baf9e4ded5044ad3b` against the stock
`ion-open-source-4.2.0-b` source at
`aa056ff51f12a4cc82bb4bf6e6652a5e3804b6c2`.

The review commits were subsequently replayed onto upstream
`integration` at `8742f615b83ac9a1263b3adbe99b02823bf3c87b`. The resulting
tree at `1490f85ba4` built successfully and the complete
`tests/subsecond-contact/dotest` gate passed on that base as well.

## Result summary

| Gate | Patched fork | Stock ION control | Interpretation |
|---|---:|---:|---|
| Subsecond parser, storage, revise/delete, and export | PASS | N/A | Fractional contact timestamps are preserved. |
| Live fractional contact start/stop | PASS | N/A | Timeline events are dispatched at millisecond boundaries. |
| Fractional `clockerr` (`0.125 s`) | PASS | N/A | Clock-error-adjusted boundaries retain milliseconds. |
| Legacy whole-second contact behavior | PASS | N/A | Existing syntax and wrapper APIs remain usable. |
| Fractional CGR boundary selection | PASS | N/A | Route selection changes at the fractional contact boundary. |
| Fractional signed `utcdelta` | PASS | N/A | Positive and negative subsecond UTC offsets are applied. |
| Full upstream `make test-all` | 159 PASS, 29 SKIP, 2 FAIL, 1 TIMEOUT | See isolated controls | All 191 entries completed; the three non-passing entries were investigated below. |
| `../demos/bench-tcpv6`, isolated | PASS | PASS | The full-suite timeout is not reproducible in isolation. |
| `issue-253-tcpcl-keepalive-v6`, isolated | PASS | PASS | The full-suite failure is not reproducible in isolation. |
| `req-0033-prob-CGR`, isolated | FAIL | FAIL | The failure reproduces on unmodified upstream and is not introduced by this branch. |

## Timing observations

The live scheduler tests used the default `10 ms` `rfxclock` polling period.
Observed absolute boundary errors were:

| Mode | Start error | Stop error |
|---|---:|---:|
| Fractional contact, `clockerr = 0` | 4 ms | 10 ms |
| Fractional contact, `clockerr = 0.125 s` | 14 ms | 9 ms |
| Legacy whole-second contact | 2 ms | 1 ms |

These values are smoke-test observations, not a statistical latency guarantee.
Scheduler precision and CPU cost depend on host load and on
`ION_RFXCLOCK_POLL_MSEC`, whose supported range is `1..1000 ms` and whose
default is `10 ms`.

## Full-suite triage

The patched full-suite run completed all 191 entries. It reported one timeout
and two failures:

1. `../demos/bench-tcpv6` timed out in the ordered full suite, but passed in a
   clean isolated container for both stock ION and the patched fork.
2. `issue-253-tcpcl-keepalive-v6` failed in the ordered full suite, but passed
   in a clean isolated container for both builds.
3. `req-0033-prob-CGR` failed both on the patched fork and on a clean build of
   stock `ion-open-source-4.2.0-b`.

The evidence therefore does not identify a regression caused by the
subsecond changes. The two TCPv6 results indicate order-dependent test
environment or IPC-state contamination in the monolithic run. The
probabilistic-CGR result is an upstream baseline failure at the tested tag.

## Reproduction

Build and install the branch into an isolated prefix, then run:

```sh
tests/subsecond-contact/dotest
```

The individual gates can also be executed as documented in
`SUBSECOND_CONTACT.md`. A fresh SDR/node database is required because the
persistent contact, range, event, and database structures contain additional
millisecond fields and are not binary compatible with stock databases.

Archived raw logs for this validation are stored outside the source tree in:

```text
/home/yasin/ion-subsecond-test-results/full-suite-078d2610b-20260831/
/home/yasin/ion-subsecond-test-results/fork-tcpv6-isolated-078d2610b-20260831/
/home/yasin/ion-subsecond-test-results/upstream-tcpv6-control-20260831/
/home/yasin/ion-subsecond-test-results/targeted-final-20260831/
/home/yasin/ion-subsecond-test-results/upstream-control-20260831/
/home/yasin/ion-subsecond-test-results/integration-latest-1490f85ba4-20260831/
```

## Scope boundaries

- UDPCL is the validated convergence layer for this milestone.
- LTP timers and retransmission calculations remain whole-second.
- CPS and NM control schemas remain whole-second; fractional values are
  administered locally through `ionadmin`.
- Existing whole-second contact-plan syntax remains supported.
- A clean node database is mandatory when switching between stock and patched
  binaries.
