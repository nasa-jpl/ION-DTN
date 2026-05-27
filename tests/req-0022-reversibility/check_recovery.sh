#!/usr/bin/env bash
#
# check_recovery.sh - shared log-checking helpers for the
# reversibilityRecovery test.
#
# Replaces the much larger verify_sdr_recovery.sh used by the
# superseded reversibilityCheck1-4 tests.  Only the checks Test B
# actually needs are exposed here.
#
# Each function returns 0 on the expected condition, 1 otherwise.
# Functions print a single diagnostic line on failure.
#

# check_reversal_attempted <log-file>
# Confirms terminateXn() saw a modified transaction and entered the
# reversal path (sdrxn.c:848).
check_reversal_attempted() {
    local log="$1"
    if grep -q "Attempting transaction reversal" "$log"; then
        return 0
    fi
    echo "check_reversal_attempted: 'Attempting transaction reversal' not found in $log"
    return 1
}

# check_ionrestart_finished <log-file>
# Confirms ionrestart was launched by terminateXn (sdrxn.c:884) and
# completed its restart sequence (restart/utils/ionrestart.c:516).
check_ionrestart_finished() {
    local log="$1"
    if grep -q "ionrestart: finished restarting ION" "$log"; then
        return 0
    fi
    echo "check_ionrestart_finished: 'ionrestart: finished restarting ION' not found in $log"
    return 1
}

# check_no_unrecoverable_error <log-file>
# Confirms reversal did not fall through to handleUnrecoverableError
# (sdrxn.c:836, sdrxn.c:855).
check_no_unrecoverable_error() {
    local log="$1"
    if grep -q "Unrecoverable SDR error" "$log"; then
        echo "check_no_unrecoverable_error: 'Unrecoverable SDR error' found in $log"
        return 1
    fi
    return 0
}
