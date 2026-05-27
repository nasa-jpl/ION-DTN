# sdr_frag_stress

Developer micro-benchmark for the SDR large-pool allocator's fragmentation
behavior. Drives `sdr_malloc` / `sdr_free` in a configurable random-size
mixed workload, snapshots `sdr_usage` periodically, and writes CSV to
stdout. Companion script `sdr_frag_compare.sh` sweeps the search-limit
knob across seeds and prints mean/stddev so single-run noise does not
dominate.

This is a tuning tool, not a regression test. It is meant to inform
allocator-default decisions (Issues 2 and 3 from `ion-sdr-analysis.md`)
with measurements rather than intuition.

## Build

The standard build wires it up automatically. After `make` in the
top-level or in `ici/x86_64-linux/`, the binary appears in
`ici/x86_64-linux/sdr_frag_stress` (or wherever your build output
lives). It links against the same `libici` everything else uses.

## Run

```bash
# Default workload, one run, CSV to stdout
./sdr_frag_stress

# Tighter heap, longer run, search limit raised to 8
SDR_STRESS_HEAP_WORDS=500000 \
SDR_STRESS_ITERS=20000 \
SDR_STRESS_SEARCH_LIMIT=8 \
./sdr_frag_stress > run_sl8.csv
```

The benchmark sets up its own SDR profile named `sfrag` in the current
directory, so run it in a scratch directory and `killm f` between runs
to avoid carrying over state. The companion script does this for you.

## Knobs

All knobs are environment variables. Defaults are tuned to fragment
a small heap noticeably in a few seconds of wall time.

| Variable | Default | Purpose |
|---|---|---|
| `SDR_STRESS_ITERS` | 5000 | Total alloc/free decisions to make. |
| `SDR_STRESS_SIZE_MIN` | 64 | Smallest allocation, in bytes. |
| `SDR_STRESS_SIZE_MAX` | 4096 | Largest allocation, in bytes. |
| `SDR_STRESS_FREE_PROB_PCT` | 45 | Probability (0-100) that each iteration frees rather than allocates. Below 50 the live set grows over time. |
| `SDR_STRESS_SEARCH_LIMIT` | 0 | Value passed to `sdr_set_search_limit`. 0 keeps the current ION default. |
| `SDR_STRESS_SNAPSHOT_EVERY` | 100 | How often to emit a CSV row. |
| `SDR_STRESS_HEAP_WORDS` | 5000000 | SDR heap size in words (~40MB on 64-bit). Smaller = faster fragmentation pressure. |
| `SDR_STRESS_SEED` | 42 | Seed for the size/free-decision RNG. |
| `SDR_STRESS_MAX_LIVE` | 100000 | Cap on tracked live allocations. Reached only with pathological growth. |

## Output

CSV header followed by one row per snapshot:

```
iter,allocs,frees,alloc_fails,in_use,small_free,large_free,unused,largest_free,frag_pct,large_frag_pct
```

`frag_pct` is the overall-heap fragmentation that `sdrwatch` reports
(includes the unassigned gap as one span). `large_frag_pct` ignores
the gap and isolates the large-pool allocator's internal fragmentation,
which is the metric tuning decisions care about.

A `REPORT,...` line at the end summarises the run's final state with
the same fields, prefixed `REPORT` so the script driver can grep it
out.

## Sweep with `sdr_frag_compare.sh`

```bash
./ici/test/sdr_frag_compare.sh \
    /path/to/sdr_frag_stress

# Default sweep: search limits {0,1,4,8,16} x seeds {1,2,3,4,5}
# Override either via SDR_FRAG_SEARCH_LIMITS / SDR_FRAG_SEEDS.
```

The script prints per-run rows then a summary with mean and stddev for
`fails` and `large_frag_pct` per search limit. Use it as the primary
entry point when comparing allocator tunings; do not rely on a single
run.

## Interpreting results

The data this benchmark produces is workload-specific. Synthetic
uniform-random size distributions over-state fragmentation pressure
compared to BP forwarding, where bundle sizes cluster around a few
modes. Before changing an in-tree default based on these numbers,
re-run against a representative real workload (e.g. driving the
SDR via `bping` and `loopback-file` for tens of thousands of bundles).

The `largestFreeBlock` field on `SdrUsageSummary`, which this
benchmark exercises heavily, is also exposed to operators via
`sdrwatch -t` as `largest free blk`. The deployment guide for
fragmentation diagnostics is in
`gh-pages/docs/ION-Monitoring-Guide.md`.
