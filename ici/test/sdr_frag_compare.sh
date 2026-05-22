#!/usr/bin/env bash
#
# sdr_frag_compare.sh -- sweep sdr_frag_stress across searchLimit and
# random seeds, aggregate fail-rate / fragmentation, and print a table.
#
# Usage:
#   sdr_frag_compare.sh [STRESS_BINARY]
#
# If STRESS_BINARY is omitted the script looks up sdr_frag_stress on
# PATH or in the standard build output directory.
#
# Workload knobs are read from the environment; defaults below.  Set
# any of them before invoking to change the experiment.  ION must NOT
# already be running -- the script wipes IPC state between runs.
#
# Output: CSV header on stdout, one row per (searchLimit, seed), then
# a one-row-per-searchLimit summary with mean/stddev.  Per-run logs
# land in $LOG_DIR (default ./frag_logs).

set -u

STRESS_BIN="${1:-}"
if [ -z "$STRESS_BIN" ]; then
    if command -v sdr_frag_stress >/dev/null 2>&1; then
        STRESS_BIN="$(command -v sdr_frag_stress)"
    elif [ -x /home/jgao/iondev/ion-ios-dev/ici/x86_64-linux/sdr_frag_stress ]; then
        STRESS_BIN=/home/jgao/iondev/ion-ios-dev/ici/x86_64-linux/sdr_frag_stress
    else
        echo "ERROR: sdr_frag_stress not found; pass path as arg 1" >&2
        exit 1
    fi
fi

# Sweep configuration.
SEARCH_LIMITS="${SDR_FRAG_SEARCH_LIMITS:-0 1 4 8 16}"
SEEDS="${SDR_FRAG_SEEDS:-1 2 3 4 5}"

# Workload knobs (also overridable from env).
export SDR_STRESS_ITERS="${SDR_STRESS_ITERS:-20000}"
export SDR_STRESS_SIZE_MIN="${SDR_STRESS_SIZE_MIN:-64}"
export SDR_STRESS_SIZE_MAX="${SDR_STRESS_SIZE_MAX:-16384}"
export SDR_STRESS_FREE_PROB_PCT="${SDR_STRESS_FREE_PROB_PCT:-48}"
export SDR_STRESS_HEAP_WORDS="${SDR_STRESS_HEAP_WORDS:-500000}"
export SDR_STRESS_SNAPSHOT_EVERY="${SDR_STRESS_SNAPSHOT_EVERY:-2000}"

LOG_DIR="${LOG_DIR:-$PWD/frag_logs}"
# Resolve to absolute so subsequent cd does not move it.
case "$LOG_DIR" in /*) ;; *) LOG_DIR="$PWD/$LOG_DIR" ;; esac
mkdir -p "$LOG_DIR"

WORK_DIR="$(mktemp -d -t sdr_frag.XXXXXX)"
trap 'killm f >/dev/null 2>&1; rm -rf "$WORK_DIR"' EXIT
cd "$WORK_DIR" || exit 1
export ION_NODE_LIST_DIR="$WORK_DIR"

# Print sweep summary.
echo "# binary: $STRESS_BIN"
echo "# iters=$SDR_STRESS_ITERS size=[$SDR_STRESS_SIZE_MIN,$SDR_STRESS_SIZE_MAX]" \
     "free_pct=$SDR_STRESS_FREE_PROB_PCT heap_words=$SDR_STRESS_HEAP_WORDS"
echo "# search_limits: $SEARCH_LIMITS"
echo "# seeds: $SEEDS"
echo
echo "search_limit,seed,allocs,fails,large_free,largest_free,large_frag_pct"

declare -A FAILS_SUM FAILS_SQ N_SAMPLES FRAG_SUM FRAG_SQ

for SL in $SEARCH_LIMITS; do
    FAILS_SUM[$SL]=0
    FAILS_SQ[$SL]=0
    FRAG_SUM[$SL]=0
    FRAG_SQ[$SL]=0
    N_SAMPLES[$SL]=0
    for SEED in $SEEDS; do
        killm f >/dev/null 2>&1
        rm -f ion_nodes
        SDR_STRESS_SEARCH_LIMIT="$SL" \
        SDR_STRESS_SEED="$SEED" \
        "$STRESS_BIN" > "$LOG_DIR/run_sl${SL}_seed${SEED}.csv" 2>/dev/null
        LINE=$(grep ^REPORT "$LOG_DIR/run_sl${SL}_seed${SEED}.csv" | head -1)
        # Parse the REPORT line into shell vars.
        ALLOCS=$(  echo "$LINE" | sed -n 's/.*allocs=\([0-9]*\).*/\1/p')
        FAILS=$(   echo "$LINE" | sed -n 's/.*fails=\([0-9]*\).*/\1/p')
        LARGE_FREE=$(echo "$LINE" | sed -n 's/.*large_free=\([0-9]*\).*/\1/p')
        LARGEST=$( echo "$LINE" | sed -n 's/.*largest_free=\([0-9]*\).*/\1/p')
        LFRAG=$(   echo "$LINE" | sed -n 's/.*large_frag_pct=\([0-9]*\).*/\1/p')
        echo "$SL,$SEED,$ALLOCS,$FAILS,$LARGE_FREE,$LARGEST,$LFRAG"
        FAILS_SUM[$SL]=$((FAILS_SUM[$SL] + FAILS))
        FAILS_SQ[$SL]=$((FAILS_SQ[$SL] + FAILS * FAILS))
        FRAG_SUM[$SL]=$((FRAG_SUM[$SL] + LFRAG))
        FRAG_SQ[$SL]=$((FRAG_SQ[$SL] + LFRAG * LFRAG))
        N_SAMPLES[$SL]=$((N_SAMPLES[$SL] + 1))
    done
done

echo
echo "# Summary (mean over seeds):"
echo "search_limit,n,fails_mean,fails_stddev,large_frag_mean,large_frag_stddev"
for SL in $SEARCH_LIMITS; do
    N=${N_SAMPLES[$SL]}
    if [ "$N" -lt 2 ]; then
        FAILS_MEAN=$((FAILS_SUM[$SL] / N))
        echo "$SL,$N,$FAILS_MEAN,-,${FRAG_SUM[$SL]},-"
        continue
    fi
    FAILS_MEAN=$((FAILS_SUM[$SL] / N))
    FRAG_MEAN=$((FRAG_SUM[$SL] / N))
    # Population stddev via integer math, rounded.
    FAILS_VAR=$(( (FAILS_SQ[$SL] - N * FAILS_MEAN * FAILS_MEAN) / N ))
    [ $FAILS_VAR -lt 0 ] && FAILS_VAR=0
    FAILS_SD=$(awk -v v=$FAILS_VAR 'BEGIN{printf "%.1f", sqrt(v)}')
    FRAG_VAR=$(( (FRAG_SQ[$SL] - N * FRAG_MEAN * FRAG_MEAN) / N ))
    [ $FRAG_VAR -lt 0 ] && FRAG_VAR=0
    FRAG_SD=$(awk -v v=$FRAG_VAR 'BEGIN{printf "%.1f", sqrt(v)}')
    echo "$SL,$N,$FAILS_MEAN,$FAILS_SD,$FRAG_MEAN,$FRAG_SD"
done
