#!/usr/bin/env bash
set -e

# Usage: run-benchmark-batch.sh <batch_num> <test1> [test2] [...]
if [ "$#" -lt 1 ]; then
    echo "Usage: $0 <batch_num> <test1> [test2] [...]" >&2
    exit 1
fi

BATCH_NUM="$1"
shift
TESTS=( "$@" )

echo "Starting benchmark for batch $BATCH_NUM at $(date)"
export ION_RUN_EXPERT="yes"
export LD_LIBRARY_PATH="/usr/local/lib:$LD_LIBRARY_PATH"

for test in "${TESTS[@]}"; do
    if [[ "$test" == ../* ]]; then
        target_path="${test#../}"
    else
        target_path="tests/$test"
    fi

    echo "Benchmarking $test (resolved to $target_path)..."
    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    python3 "$SCRIPT_DIR/benchmark.py" --force-update --target-path "$target_path" || {
        TEST_STATUS=$?
        echo "::error::Benchmark failed for $test with status $TEST_STATUS"
        exit $TEST_STATUS
    }
done

echo "Batch $BATCH_NUM complete at $(date)"
