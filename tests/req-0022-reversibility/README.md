# SDR Transaction Reversibility Tests

This directory contains the regression tests that exercise SDR
transaction reversibility -- the mechanism that lets ION roll back
in-progress modifications to the SDR heap when a transaction is
cancelled, implemented in `reverseTransaction()` at
`ici/sdr/sdrxn.c:647` and orchestrated from `terminateXn()` at
`ici/sdr/sdrxn.c:808`.

The previous suite (`reversibilityCheck1` through `4`) was retired in
favour of the two tests below.

## Tests

### `reversibilityCorrectness` -- correctness, in-process

Standalone test that does not start ION.  It loads a private SDR,
pre-populates it with known objects, snapshots the dataspace, runs a
deliberately destructive transaction, cancels it, and asserts the
heap is restored byte-for-byte.  Also covers the
`sdr->modified == false` skip path and a follow-up modified
transaction to confirm reversibility itself stays functional after
the skip case.

Driver binary: `reversibility_correctness_test`
(`ici/test/reversibility_correctness_test.c`).  Runs in well under
one second, deterministic, no daemons in the loop.

### `reversibilityRecovery` -- live-ION integration

Starts ION on a single LTP-loopback node with reversibility enabled
and triggers a deterministic cancel-with-modifications via the
`sdrcancel` utility.  Asserts that the cancel reaches the reversal
code path (`"Attempting transaction reversal..."`), does not escalate
to an unrecoverable error, that `ionrestart` finishes
(`"ionrestart: finished restarting ION"`), and that a post-recovery
`bpsource` -> `bpsink` round-trip delivers bundles.

Driver binaries: `sdrcancel` (`ici/test/sdrcancel.c`).  Shared log
helper: `check_recovery.sh`.

## Layout

```
tests/req-0022-reversibility/
├── README.md                       this file
├── check_recovery.sh               shared log-checking helpers for Test B
├── reversibilityCorrectness/       Test A (correctness, in-process)
│   ├── cleanup
│   └── dotest
└── reversibilityRecovery/          Test B (live-ION integration)
    ├── cleanup
    ├── configs/
    │   ├── config.ionconfig        configFlags 13 (REVERSIBLE)
    │   ├── ionstart
    │   ├── loopback.bprc
    │   ├── loopback.ionrc
    │   ├── loopback.ionsecrc
    │   ├── loopback.ipnrc
    │   └── loopback.ltprc
    └── dotest
```

## Source code touched outside this directory

```
ici/test/reversibility_correctness_test.c   (new) Test A driver
ici/test/sdrcancel.c                        (new) Test B trigger
ici/x86_64-linux/Makefile.dev               (mod) build wiring
Makefile.am                                 (mod) build wiring
ici/test/sdr_test_util.c                    (del) superseded utility
```

## Running

After building ION:

```
cd tests/req-0022-reversibility/reversibilityCorrectness
./dotest

cd tests/req-0022-reversibility/reversibilityRecovery
./dotest
```

Test A is deterministic.  Test B's Phase 4 (waiting for ionrestart)
uses a 300-second ceiling to accommodate the Solaris LDOM CI runner,
where LTP teardown alone can take 80+ seconds; on Linux it typically
converges in well under 30 s.
