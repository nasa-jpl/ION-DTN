#!/usr/bin/env bash
#
# repeat_test.sh - Repeatedly run dotest until failure is detected
#
# Usage: ./repeat_test.sh [max_iterations]
#
# This script runs ./dotest repeatedly and checks ion.log for the
# "Unrecoverable SDR error" or "sdr_exit_xn with modified=1" messages.
# It stops when either message is found or max iterations reached.
#

MAX_ITERATIONS=${1:-100}
ITERATION=0
FAILURE_FOUND=0

echo "========================================"
echo "Starting repeated test runs"
echo "Max iterations: $MAX_ITERATIONS"
echo "Looking for: 'Unrecoverable SDR error' or 'sdr_exit_xn with modified=1'"
echo "========================================"
echo ""

while [ $ITERATION -lt $MAX_ITERATIONS ]; do
    ITERATION=$((ITERATION + 1))
    echo "========================================"
    echo "=== ITERATION $ITERATION of $MAX_ITERATIONS ==="
    echo "========================================"

    # Clean up from previous run
    killm 2>/dev/null
    ./cleanup 2>/dev/null
    rm -f ion.log 2>/dev/null

    # Run the test
    ./dotest
    TEST_EXIT_CODE=$?

    # Check for failure messages in ion.log
    if [ -f ion.log ]; then
        if grep -q "Unrecoverable SDR error" ion.log; then
            echo ""
            echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
            echo "!!! FAILURE DETECTED: Unrecoverable SDR error !!!"
            echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
            echo "Iteration: $ITERATION"
            echo ""
            echo "=== Relevant log entries ==="
            # Use awk for Solaris compatibility (no grep -B/-A)
            awk '/Unrecoverable SDR error/ {
                for(i=NR-5; i<=NR+5; i++) if(i in a) print a[i]
            }
            {a[NR]=$0; if(NR>10) delete a[NR-10]}' ion.log
            echo ""
            echo "=== DIAG entries ==="
            grep "DIAG" ion.log | tail -50
            FAILURE_FOUND=1
            break
        fi

        if grep -q "sdr_exit_xn with modified=1" ion.log; then
            echo ""
            echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
            echo "!!! FAILURE DETECTED: sdr_exit_xn with modified=1 !!!"
            echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
            echo "Iteration: $ITERATION"
            echo ""
            echo "=== Relevant log entries ==="
            # Use awk for Solaris compatibility (no grep -B/-A)
            awk '/sdr_exit_xn with modified=1/ {
                for(i=NR-5; i<=NR+5; i++) if(i in a) print a[i]
            }
            {a[NR]=$0; if(NR>10) delete a[NR-10]}' ion.log
            echo ""
            echo "=== DIAG entries ==="
            grep "DIAG" ion.log | tail -50
            FAILURE_FOUND=1
            break
        fi
    fi

    # Check test exit code
    if [ $TEST_EXIT_CODE -ne 0 ]; then
        echo ""
        echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
        echo "!!! TEST EXITED WITH ERROR CODE: $TEST_EXIT_CODE !!!"
        echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
        echo "Iteration: $ITERATION"
        echo ""
        if [ -f ion.log ]; then
            echo "=== Last 100 lines of ion.log ==="
            tail -100 ion.log
            echo ""
            echo "=== DIAG entries ==="
            grep "DIAG" ion.log | tail -50
        fi
        FAILURE_FOUND=1
        break
    fi

    echo ""
    echo "Iteration $ITERATION PASSED"
    echo ""

    # Brief pause between iterations
    sleep 2
done

echo ""
echo "========================================"
if [ $FAILURE_FOUND -eq 1 ]; then
    echo "STOPPED: Failure detected at iteration $ITERATION"
    echo "ion.log preserved for analysis"
else
    echo "COMPLETED: All $MAX_ITERATIONS iterations passed"
fi
echo "========================================"

# Clean up
killm 2>/dev/null

exit $FAILURE_FOUND
