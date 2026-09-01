# SDR Transaction Reversibility Tests

## What reversibility is — and is not

SDR transaction *reversibility* is transaction reversal.
When a modified transaction is cancelled,
`reverseTransaction()` in `ici/sdr/sdrxn.c` rolls the SDR heap
back to its state at `sdr_begin_xn()`,
undoing every write, malloc, and free made inside the transaction.
`terminateXn()` orchestrates that rollback on cancel.

Reversibility is scoped to exactly that guarantee.
It is **not** a durability or crash-recovery mechanism,
and it does **not** guarantee SDR correctness after a process
crash or a hard power reset.
Surviving those events is a separate concern —
it depends on the platform, the storage medium, and the restart
machinery, not on transaction reversal —
and it is deliberately out of scope for these tests.

## Test

### `reversibilityCorrectness` — correctness, in-process

Standalone test that does not start ION.
It loads a private reversible SDR,
pre-populates it with known small and large objects,
snapshots the whole dataspace,
runs a deliberately destructive transaction
(partial and whole-object overwrites, a free, and mixed
malloc/write/free), cancels it,
and asserts the heap is restored byte-for-byte.
It also exercises the read-only skip path
(`sdr->modified == false`, no reversal work performed)
and a follow-up destructive transaction,
confirming reversal still works after the skip case.

Driver binary: `reversibility_correctness_test`
(`ici/test/reversibility_correctness_test.c`).
Deterministic, runs in well under one second,
no daemons in the loop.

## Why there is no live-ION recovery test

An earlier `reversibilityRecovery` test drove a live node into a
cancel-with-modifications, waited for the `ionrestart` that
cancel triggered, and then pushed a post-recovery bundle
round-trip.
That conflated transaction reversal with crash recovery:
its distinctive assertions were about the node restarting and
resuming traffic, not about the reversal itself —
which `reversibilityCorrectness` already verifies directly and
deterministically.
Manufacturing and surviving a restart is inherently
timing-dependent — it needed a 300-second ceiling for the Solaris
CI runner, send retries, and daemon re-raises —
so the test was flaky without adding any reversal coverage,
and it was retired.
The predecessor `reversibilityCheck1`–`4` suite,
which forced crashes through heap exhaustion,
was retired earlier for the same reason.

## Layout

```
tests/req-0022-reversibility/
├── README.md                     this file
└── reversibilityCorrectness/     correctness test (in-process)
    ├── cleanup
    └── dotest
```

## Supporting source (outside this directory)

- `ici/test/reversibility_correctness_test.c` — the test driver
- build wiring in `Makefile.am` and
  `ici/x86_64-linux/Makefile.dev`

## Running

After building ION:

```
cd tests/req-0022-reversibility/reversibilityCorrectness
./dotest
```

The test is deterministic and needs no running ION node.
