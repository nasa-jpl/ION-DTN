#!/usr/bin/env bash
#
# verify_sdr_recovery.sh - Common verification logic for SDR recovery tests
#
# This script provides reusable functions for verifying SDR transaction
# reversibility and recovery in ION 4.1.3+.
#
# Usage: source this script in your test, then call verification functions
#

# Check if transaction abort occurred
# Returns 0 if "Transaction aborted" found, 1 otherwise
check_transaction_aborted() {
    local LOG_FILE="${1:-ion.log}"

    if grep -q "Transaction aborted" "$LOG_FILE"; then
        echo "PASS: Transaction aborted message found"
        return 0
    else
        echo "FAIL: No transaction aborted message found"
        return 1
    fi
}

# Check if transaction reversal was attempted (ION 4.1.3+ with modifications)
# Returns 0 if reversal message found, 1 otherwise
check_reversal_attempted() {
    local LOG_FILE="${1:-ion.log}"

    if grep -q "Attempting transaction reversal" "$LOG_FILE"; then
        echo "PASS: Transaction reversal was attempted"
        return 0
    else
        echo "FAIL: Transaction reversal was not attempted"
        return 1
    fi
}

# Check if transaction reversal was skipped (ION 4.1.3+ without modifications)
# Returns 0 if skip message found, 1 otherwise
check_reversal_skipped() {
    local LOG_FILE="${1:-ion.log}"

    if grep -q "Transaction reversal not necessary" "$LOG_FILE"; then
        echo "PASS: Transaction reversal was skipped (no modifications)"
        return 0
    else
        echo "FAIL: Expected reversal skip message not found"
        return 1
    fi
}

# Check if unrecoverable SDR error occurred (should NOT happen in successful tests)
# Returns 0 if NO unrecoverable error, 1 if error found
check_no_unrecoverable_error() {
    local LOG_FILE="${1:-ion.log}"

    if grep -q "Unrecoverable SDR error" "$LOG_FILE"; then
        echo "FAIL: Unrecoverable SDR error detected"
        return 1
    else
        echo "PASS: No unrecoverable SDR errors"
        return 0
    fi
}

# Check if ionrestart executed
# Returns 0 if ionrestart activity found, 1 otherwise
check_ionrestart_executed() {
    local LOG_FILE="${1:-ion.log}"

    # Look for ionrestart-specific messages
    # Use multiple greps for Solaris compatibility (no -E option)
    if grep -q "ionrestart" "$LOG_FILE" 2>/dev/null || \
       grep -q "Stopping daemons" "$LOG_FILE" 2>/dev/null || \
       grep -q "Restarting ION" "$LOG_FILE" 2>/dev/null; then
        echo "PASS: ionrestart executed"
        return 0
    else
        echo "FAIL: ionrestart did not execute"
        return 1
    fi
}

# Verify basic SDR operations work after recovery
# Returns 0 if SDR is operational, 1 otherwise
check_sdr_operational() {
    # Try a simple ionadmin command - just attach and quit
    # Using '#' as a comment command to avoid any prompts
    if ionadmin <<EOF 2>/dev/null
# SDR operational check
q
EOF
    then
        echo "PASS: SDR is operational"
        return 0
    else
        echo "FAIL: SDR is not operational"
        return 1
    fi
}

# Count specific messages in log file
# Usage: count_messages "pattern" "log_file"
count_messages() {
    local PATTERN="$1"
    local LOG_FILE="${2:-ion.log}"

    local COUNT=$(grep -c "$PATTERN" "$LOG_FILE" 2>/dev/null || echo "0")
    echo "$COUNT"
}

# Verify complete recovery with reversal (for modified transaction tests)
# This is the full check for tests like reversibilityCheck1, reversibilityCheck2
verify_recovery_with_reversal() {
    local LOG_FILE="${1:-ion.log}"
    local RESULT=0

    echo "=== Verifying Recovery with Reversal ==="

    # Check 1: Transaction was aborted
    check_transaction_aborted "$LOG_FILE" || RESULT=1

    # Check 2: Reversal was attempted
    check_reversal_attempted "$LOG_FILE" || RESULT=1

    # Check 3: No unrecoverable errors
    check_no_unrecoverable_error "$LOG_FILE" || RESULT=1

    # Check 4: ionrestart executed
    check_ionrestart_executed "$LOG_FILE" || RESULT=1

    # Check 5: SDR is operational
    check_sdr_operational || RESULT=1

    if [ $RESULT -eq 0 ]; then
        echo "=== OVERALL: PASS ==="
    else
        echo "=== OVERALL: FAIL ==="
    fi

    return $RESULT
}

# Verify no-reversal path (for unmodified transaction tests)
# This is for tests like reversibilityCheck3
verify_recovery_without_reversal() {
    local LOG_FILE="${1:-ion.log}"
    local RESULT=0

    echo "=== Verifying No-Reversal Path ==="

    # Check 1: Transaction was aborted
    check_transaction_aborted "$LOG_FILE" || RESULT=1

    # Check 2: Reversal was skipped
    check_reversal_skipped "$LOG_FILE" || RESULT=1

    # Check 3: No unrecoverable errors
    check_no_unrecoverable_error "$LOG_FILE" || RESULT=1

    # Check 4: SDR is operational
    check_sdr_operational || RESULT=1

    if [ $RESULT -eq 0 ]; then
        echo "=== OVERALL: PASS ==="
    else
        echo "=== OVERALL: FAIL ==="
    fi

    return $RESULT
}

# Print log excerpt around a pattern (for debugging)
# Usage: print_log_context "pattern" "log_file" [lines_before] [lines_after]
print_log_context() {
    local PATTERN="$1"
    local LOG_FILE="${2:-ion.log}"
    local BEFORE="${3:-5}"
    local AFTER="${4:-5}"

    echo "=== Log context for: $PATTERN ==="
    # Use awk for cross-platform compatibility (no grep -B/-A on Solaris)
    # This version avoids GNU awk extensions like length() on arrays
    awk -v pattern="$PATTERN" -v before="$BEFORE" -v after="$AFTER" '
    BEGIN { found = 0; match_count = 0 }
    {
        lines[NR] = $0
    }
    $0 ~ pattern {
        match_lines[match_count++] = NR
        found = 1
    }
    END {
        if (found == 0) {
            print "Pattern not found"
            exit
        }
        for (j = 0; j < match_count; j++) {
            m = match_lines[j]
            start = m - before
            if (start < 1) start = 1
            end = m + after
            if (end > NR) end = NR
            for (i = start; i <= end; i++) {
                if (!printed[i]) {
                    print lines[i]
                    printed[i] = 1
                }
            }
            print "--"
        }
    }
    ' "$LOG_FILE" 2>/dev/null || echo "Pattern not found"
    echo "=== End context ==="
}

# Wait for log message with timeout
# Usage: wait_for_message "pattern" "log_file" timeout_seconds
wait_for_message() {
    local PATTERN="$1"
    local LOG_FILE="${2:-ion.log}"
    local TIMEOUT="${3:-60}"
    local ELAPSED=0

    echo "Waiting for message: $PATTERN (timeout: ${TIMEOUT}s)"

    while [ $ELAPSED -lt $TIMEOUT ]; do
        if grep -q "$PATTERN" "$LOG_FILE" 2>/dev/null; then
            echo "Message found after ${ELAPSED}s"
            return 0
        fi
        sleep 1
        ELAPSED=$((ELAPSED + 1))
    done

    echo "Timeout waiting for message after ${TIMEOUT}s"
    return 1
}

# Clean up ion.log before test (optional)
clean_ion_log() {
    local LOG_FILE="${1:-ion.log}"

    if [ -f "$LOG_FILE" ]; then
        # Back up existing log
        mv "$LOG_FILE" "${LOG_FILE}.backup.$$"
        echo "Backed up existing ion.log"
    fi
}

# Display summary of key messages
display_log_summary() {
    local LOG_FILE="${1:-ion.log}"

    echo "=== Log Summary ==="
    echo "Transaction aborted: $(count_messages 'Transaction aborted' "$LOG_FILE")"
    echo "Reversal attempted: $(count_messages 'Attempting transaction reversal' "$LOG_FILE")"
    echo "Reversal skipped: $(count_messages 'Transaction reversal not necessary' "$LOG_FILE")"
    echo "Unrecoverable errors: $(count_messages 'Unrecoverable SDR error' "$LOG_FILE")"
    echo "==================="
}

# Note: Functions are available to sourcing scripts automatically.
# No 'export -f' needed since this script is sourced, not executed as a subprocess.
# (export -f is bash-specific and not portable to other shells anyway)
