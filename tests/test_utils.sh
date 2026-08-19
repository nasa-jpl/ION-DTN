#!/bin/sh
#
# test_utils - Shared utility functions for ION test scripts.
#
# Source this file from dotest scripts:
#   . ../../test_utils.sh  (or adjust path as needed)

# The platforms we care about support sleep(1) with milliseconds.
# Older platforms that don't, probably have Perl.
# Stuff that doesn't have either falls back to seconds.
#
# $1: string, decimal value, seconds to sleep for (e.g., "0.1")
# $2: integer, value of $1 as milliseconds (e.g., 100)
# _tu_slept_ms is updated to the amount of milliseconds slept.
# This can be different than $2 if we have to fall back to seconds.
if sleep 0.001 2>/dev/null; then
    _tu_sleep() {
        sleep "$1"
        _tu_slept_ms="$2"
    }
elif command -v perl >/dev/null 2>&1; then
    _tu_sleep() {
        perl -e "select(undef, undef, undef, $1)"
        _tu_slept_ms="$2"
    }
else
    _tu_sleep() {
        _tu_sleep_val="$1"
        _tu_sleep_whole="${_tu_sleep_val%.*}"
        _tu_sleep_whole="${_tu_sleep_whole:-0}"

        if [ "$_tu_sleep_whole" -gt 0 ]; then
            sleep "$_tu_sleep_whole"
            _tu_slept_ms=$((_tu_sleep_whole * 1000))
        else
            sleep 1
            _tu_slept_ms=1000
        fi
    }
fi

# waitForLog PATTERN LOGFILE [TIMEOUT_MS] [POLL_INTERVAL_MS]
#
# Block until PATTERN appears in LOGFILE (grep -q).
# Returns 0 on match, 1 on timeout.
waitForLog() {
    waitForFileContent "$@"
}

# waitForPidReady PID [TIMEOUT_MS] [POLL_INTERVAL_MS]
#
# Block until process PID is running (kill -0).
# Returns 0 if process is alive, 1 on timeout.
waitForPidReady() {
    _wfpr_pid="$1"
    _wfpr_timeout="${2:-10000}"
    _wfpr_interval="${3:-100}"
    _wfpr_sleep_arg=$(printf "%d.%03d" \
        $((_wfpr_interval / 1000)) $((_wfpr_interval % 1000)))
    _wfpr_elapsed=0

    while :; do
        if kill -0 "$_wfpr_pid" 2>/dev/null; then
            return 0
        fi
        if [ "$_wfpr_elapsed" -ge "$_wfpr_timeout" ]; then
            echo "waitForPidReady: timeout after" \
                "$((_wfpr_timeout / 1000))s waiting for pid $_wfpr_pid" >&2
            return 1
        fi
        _tu_sleep "$_wfpr_sleep_arg" "$_wfpr_interval"
        _wfpr_elapsed=$((_wfpr_elapsed + _tu_slept_ms))
    done
}

# waitForFileContent PATTERN FILE [TIMEOUT_MS] [POLL_INTERVAL_MS]
#
# Block until PATTERN appears in FILE.
# Returns 0 on match, 1 on timeout.
waitForFileContent() {
    _wffc_pattern="$1"
    _wffc_file="$2"
    _wffc_timeout="${3:-30000}"
    _wffc_interval="${4:-100}"
    _wffc_sleep_arg=$(printf "%d.%03d" \
        $((_wffc_interval / 1000)) $((_wffc_interval % 1000)))
    _wffc_elapsed=0

    while :; do
        if grep -q "$_wffc_pattern" "$_wffc_file" 2>/dev/null; then
            return 0
        fi
        if [ "$_wffc_elapsed" -ge "$_wffc_timeout" ]; then
            echo "waitForFileContent: timeout after" \
                "$((_wffc_timeout / 1000))s waiting for" \
                "'$_wffc_pattern' in $_wffc_file" >&2
            return 1
        fi
        _tu_sleep "$_wffc_sleep_arg" "$_wffc_interval"
        _wffc_elapsed=$((_wffc_elapsed + _tu_slept_ms))
    done
}

# killAndWaitForPidToDie PID [SIGNAL] [TIMEOUT_MS] [POLL_INTERVAL_MS]
#
# Send SIGNAL (default SIGINT) to PID, then wait for it to exit.
# If it doesn't exit within TIMEOUT_MS, send SIGKILL.
# Returns 0 if process exited, 1 if SIGKILL was needed.
killAndWaitForPidToDie() {
    _kwpd_pid="$1"
    _kwpd_sig="${2:-2}"
    _kwpd_timeout="${3:-5000}"
    _kwpd_interval="${4:-100}"

    kill -0 "$_kwpd_pid" 2>/dev/null || return 0 # already dead

    kill -"$_kwpd_sig" "$_kwpd_pid" 2>/dev/null

    _kwpd_sleep_arg=$(printf "%d.%03d" \
        $((_kwpd_interval / 1000)) $((_kwpd_interval % 1000)))
    _kwpd_elapsed=0

    while :; do
        if ! kill -0 "$_kwpd_pid" 2>/dev/null; then
            return 0
        fi
        if [ "$_kwpd_elapsed" -ge "$_kwpd_timeout" ]; then
            kill -9 "$_kwpd_pid" 2>/dev/null
            return 1
        fi
        _tu_sleep "$_kwpd_sleep_arg" "$_kwpd_interval"
        _kwpd_elapsed=$((_kwpd_elapsed + _tu_slept_ms))
    done
}

# lastBpTally TALLY [IONLOG]
#
# Print the cumulative "(+)" count from the most recent bpstats snapshot in
# IONLOG (default ./ion.log), or 0 if no snapshot has recorded that tally yet.
# TALLY is a bpstats category: src, fwd, xmt, rcv, dlv, rfw or exp.
#
# bpstats appends one snapshot per invocation, so a caller polling for traffic
# to settle should run bpstats and then read the newest snapshot back:
#
#     bpstats
#     sleep 2
#     fwd=$(lastBpTally fwd)
#
# Allow for lag between the traffic and the tally.  Most BP tallies are batched
# into atomic deltas and written to the SDR by bpclock only once a second, so a
# snapshot taken immediately after a bundle moves can still read zero, and two
# tallies on different nodes become visible independently.  Poll for every
# tally an assertion depends on, not just one of them.
#
# The snapshot line ends with "(+) <count> <bytes>", so the count is the
# second-to-last field.  Do the whole match and extract in one awk pass:
# GNU-only options are a recurring portability trap here -- Solaris rejects
# both "grep -a" and "sed -E" -- and a discarded stderr turns the resulting
# empty read into a silent zero that looks like a protocol failure rather than
# a broken script.  awk gets no -v either, since that is not in the original
# awk still shipped as /usr/bin/awk on some platforms.
lastBpTally() {
    _lbt_tally="$1"
    _lbt_log="${2:-ion.log}"

    awk '/\[x\] '"$_lbt_tally"' / { _lbt_v = $(NF - 1) }
         END { print _lbt_v + 0 }' "$_lbt_log"
}
