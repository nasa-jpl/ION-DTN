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

PREREQUISITES
-------------
ION built with the xlsa backend:

    cd <ion source>
    ./configure --with-xlsa-backend=shm
    make
    sudo make install

`xlso`, `xlsi`, `bpdriver`, and `bpcounter` must all be on PATH.  If you
don't want to install system-wide, prepend the source-tree root to PATH
so the libtool-wrapped binaries are found:

    PATH=/path/to/ion-source:$PATH ./dotest

The pretest-script verifies binary availability before running.

QUICK START
-----------
    cd demos/bench-ltp-xlsa
    ./dotest                  # baseline scenario only (~30s)

Each scenario prints a RESULT line; the harness collects every result
into a SUMMARY table at the end of the run.

SCENARIOS
---------
    baseline    matched-rate run at XLSA_RATE_BPS = contact rate (default)
    delay       sweep XLSA_DELAY_US over 0 / 10 ms / 100 ms / 1 s
    drop        sweep XLSA_DROP_PPM over 0 / 100 / 1000 / 10000 ppm
    rate        sweep XLSA_RATE_BPS at matched contact rates
    matrix      rate-mismatch matrix: matched / under-provisioned /
                over-provisioned (varies the contact rate against a
                fixed XLSA_RATE_BPS = 2 GB/s)
    all         every scenario above

Run any subset:

    ./dotest delay drop       # delay sweep then drop sweep
    ./dotest all              # the full battery

OVERRIDING DEFAULTS
-------------------
The matched-rate baseline sets:

    XLSA_RATE_BPS  = 2000000000     (= the contact-plan rate in global.ionrc)
    XLSA_SLOTS     = 1024
    XLSA_DELAY_US  = 0
    XLSA_JITTER_US = 0
    XLSA_DROP_PPM  = 0

The sweep scenarios overwrite the relevant variables internally, but the
baseline can be tuned by exporting them before invoking dotest:

    XLSA_DELAY_US=50000 XLSA_DROP_PPM=1000 ./dotest

CAUTION: outside of the `matrix` scenario, dotest keeps XLSA_RATE_BPS
equal to the contact-plan rate so every result is a matched-rate run.
Deliberately mismatching them is what the `matrix` scenario does on
purpose -- see DESIGN.md §3.5 for the rationale.

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
