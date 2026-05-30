bench-ltp-xlsa: LTP throughput benchmark over the xlsa link service
====================================================================

PURPOSE
-------
Measure ION's LTP engine (segmentation, reassembly, ZCO handling,
report/checkpoint timers, span throughput) on a single host WITHOUT the
host UDP/IP stack confounding the measurement.

Two ION nodes (engine ids 2 and 3) exchange bundles over LTP carried by
the generic xlsa link service (xlso/xlsi) with the shared-memory backend
(xport_shm).  The topology mirrors demos/bench-ltp/ exactly so results
can be compared head-to-head; the only varying factor is the link
service.

STATUS
------
This is a sketch.  The matched-rate baseline scenario runs end-to-end;
the impairment sweeps and rate-mismatch matrix from
ltp/xlsa/doc/DESIGN.md §3.5 / §5.7 are listed as TODO at the bottom of
the dotest script.

PREREQUISITES
-------------
ION built with the xlsa backend:

    cd <ion source>
    ./configure --with-xlsa-backend=shm
    make
    sudo make install

`xlso`, `xlsi`, `bpdriver`, and `bpcounter` must all be on PATH.  The
pretest-script verifies this before running.

QUICK START
-----------
    cd demos/bench-ltp-xlsa
    ./dotest

Output ends with a single RESULT line:

    RESULT: <N> bytes in <T>s = <BPS> B/s goodput

OVERRIDING DEFAULTS
-------------------
The matched-rate baseline sets:

    XLSA_RATE_BPS  = 2000000000     (= the contact-plan rate in global.ionrc)
    XLSA_SLOTS     = 1024
    XLSA_DELAY_US  = 0
    XLSA_JITTER_US = 0
    XLSA_DROP_PPM  = 0

Export any of these before invoking dotest to override.  Example: a
50-ms one-way delay with 1000 ppm drop (≈ 0.1% loss):

    XLSA_DELAY_US=50000 XLSA_DROP_PPM=1000 ./dotest

CAUTION: keep XLSA_RATE_BPS = the contact-plan rate for a clean
matched-rate run.  Deliberately mismatching them is itself a valuable
experiment (it emulates a radio degrading below the rate ION was told to
expect), but it is no longer the baseline.  See DESIGN.md §3.5.

LAYOUT
------
    global.ionrc                contact plan (rate must match XLSA_RATE_BPS)
    2.bench.xlsa/               sender node, engine id 2
        bench.ltprc             xlso/xlsi span declarations (mirrored shm names)
        ...
    3.bench.xlsa/               receiver node, engine id 3
        ...
    pretest-script              path / config sanity checks
    cleanup                     remove ION state + leftover shm segments
    dotest                      orchestration (one matched-rate scenario)

COMPARING AGAINST UDP
---------------------
Run demos/bench-ltp/dotest with the same bundle size and total bytes,
then compare the goodput.  The difference is roughly the cost of the
host UDP/loopback path plus udplso's token-bucket pacing — which is
exactly what xlsa exists to remove from the measurement.

SEE ALSO
--------
    ltp/xlsa/doc/DESIGN.md      full analysis (also at gh-pages
                                ION APIs -> "Generic LTP Link Service
                                Template (xlso/xlsi)")
    ltp/xlsa/README.md          build & adaptation guide
    xlso(1), xlsi(1)            man pages
