#!/usr/bin/env bash
#
# rtems-verify-qemu-output.sh - Verify RTEMS QEMU test output
#
# Checks QEMU output for expected ION operational indicators. Used by
# ci-rtems61-aarch64-libbsd.yml verify step.
#
# Usage:
#   rtems-verify-qemu-output.sh --qemu-output=<file> --report-output=<file>
#
# Parameters:
#   --qemu-output=<file>   QEMU output file to verify
#   --report-output=<file> Verification report output path
#
# Exit codes:
#   0 - All verification checks passed
#   1 - One or more checks failed, or missing input file

set -euo pipefail

# Parse command-line arguments
QEMU_OUTPUT=""
REPORT_OUTPUT=""

for arg in "$@"; do
    case $arg in
        --qemu-output=*)
            QEMU_OUTPUT="${arg#*=}"
            ;;
        --report-output=*)
            REPORT_OUTPUT="${arg#*=}"
            ;;
        *)
            echo "Unknown argument: $arg" >&2
            exit 1
            ;;
    esac
done

# Validate required arguments
if [ -z "$QEMU_OUTPUT" ] || [ -z "$REPORT_OUTPUT" ]; then
    echo "Error: Missing required arguments" >&2
    echo "Usage: rtems-verify-qemu-output.sh --qemu-output=<file> --report-output=<file>" >&2
    exit 1
fi

# Verify QEMU output file exists
if [ ! -f "$QEMU_OUTPUT" ]; then
    echo "Error: QEMU output file not found: $QEMU_OUTPUT" >&2
    exit 1
fi

echo "Verifying ION execution results..."

# Create verification report file
{
    echo "RTEMS 6.1 ARM64 ION Verification Report"
    echo "========================================"
    echo "Date: $(date)"
    echo ""
} > "$REPORT_OUTPUT"

VERIFICATION_FAILED=false

# Check for critical success indicators in output
{
    echo "Verification Checks:"
    echo ""
} >> "$REPORT_OUTPUT"

# Check 1: Payload delivery
if grep -q "ION event: Payload delivered" "$QEMU_OUTPUT"; then
    echo "✓ Bundle payload delivered successfully"
    echo "[PASS] Bundle payload delivered successfully" >> "$REPORT_OUTPUT"
else
    echo "✗ Bundle payload delivery not found"
    echo "[FAIL] Bundle payload delivery not found" >> "$REPORT_OUTPUT"
  VERIFICATION_FAILED=true
fi

# Check 2: IPN forwarder daemon
if grep -q "ipnfw is running" "$QEMU_OUTPUT"; then
    echo "✓ IPN forwarder daemon running"
    echo "[PASS] IPN forwarder daemon running" >> "$REPORT_OUTPUT"
else
    echo "✗ IPN forwarder daemon not found"
    echo "[FAIL] IPN forwarder daemon not found" >> "$REPORT_OUTPUT"
    VERIFICATION_FAILED=true
fi

# Check 3: UDP services
if grep -q "udplso is running" "$QEMU_OUTPUT" && grep -q "udplsi is running" "$QEMU_OUTPUT"; then
    echo "✓ UDP services running"
    echo "[PASS] UDP services running (udplso and udplsi)" >> "$REPORT_OUTPUT"
else
    echo "✗ UDP services not found"
    echo "[FAIL] UDP services not found (udplso or udplsi missing)" >> "$REPORT_OUTPUT"
    VERIFICATION_FAILED=true
fi

# Check 4: LTP transmission
if grep -q "Output segments: popped=1" "$QEMU_OUTPUT"; then
    echo "✓ LTP segment transmitted"
    echo "[PASS] LTP segment transmitted (popped=1)" >> "$REPORT_OUTPUT"
else
    echo "✗ LTP segment transmission not found"
    echo "[FAIL] LTP segment transmission not found" >> "$REPORT_OUTPUT"
    VERIFICATION_FAILED=true
fi

# Check 5: LTP reception
if grep -q "Input segments (red): count=1" "$QEMU_OUTPUT"; then
    echo "✓ LTP segment received"
    echo "[PASS] LTP segment received (count=1)" >> "$REPORT_OUTPUT"
else
    echo "✗ LTP segment reception not found"
    echo "[FAIL] LTP segment reception not found" >> "$REPORT_OUTPUT"
    VERIFICATION_FAILED=true
fi

# Check 6: Session completion
if grep -q "Sessions:.*completed=1" "$QEMU_OUTPUT"; then
    echo "✓ LTP session completed"
    echo "[PASS] LTP session completed" >> "$REPORT_OUTPUT"
else
    echo "✗ LTP session completion not found"
    echo "[FAIL] LTP session did not complete" >> "$REPORT_OUTPUT"
    VERIFICATION_FAILED=true
fi

# Check 7: Absence of spawn errors
if grep -q "Can't spawn task: no parms cleared yet" "$QEMU_OUTPUT"; then
    echo "✗ Daemon spawn errors detected"
    echo "[FAIL] Daemon spawn errors detected" >> "$REPORT_OUTPUT"

    # Extract which daemons failed to spawn
    {
        echo ""
        echo "Failed daemon spawns:"
        grep "Can't spawn task: no parms cleared yet" "$QEMU_OUTPUT" | \
        sed 's/.*(\(.*\))/  - \1/'
    } >> "$REPORT_OUTPUT"

    VERIFICATION_FAILED=true
else
    echo "✓ No daemon spawn errors"
    echo "[PASS] No daemon spawn errors" >> "$REPORT_OUTPUT"
fi

# Check 8: System clock initialization
if grep -q "System clock initialized" "$QEMU_OUTPUT"; then
    echo "✓ System clock initialized"
    echo "[PASS] System clock initialized" >> "$REPORT_OUTPUT"
else
    echo "✗ System clock initialization not found"
    echo "[FAIL] System clock initialization not found" >> "$REPORT_OUTPUT"
    VERIFICATION_FAILED=true
fi

# Check bundle statistics
if grep -q "xmt from.*: .*(1) [1-9]" "$QEMU_OUTPUT"; then
    echo "✓ Bundle transmission statistics present"
    echo "[PASS] Bundle transmission statistics present" >> "$REPORT_OUTPUT"
else
    echo "✗ Bundle transmission statistics not found"
    echo "[FAIL] Bundle transmission statistics incorrect" >> "$REPORT_OUTPUT"
    VERIFICATION_FAILED=true
fi

if grep -q "rcv from.*: .*(1) [1-9]" "$QEMU_OUTPUT"; then
    echo "✓ Bundle reception statistics present"
    echo "[PASS] Bundle reception statistics present" >> "$REPORT_OUTPUT"
else
    echo "✗ Bundle reception statistics not found"
    echo "[FAIL] Bundle reception statistics incorrect" >> "$REPORT_OUTPUT"
    VERIFICATION_FAILED=true
fi

# Multi-test scenarios: the RTEMS test now exercises TCP/TCPCL, CFDP, and
# AMS in addition to the baseline UDP/LTP loopback.  Verify each of the
# added scenarios so the externalized verifier covers the full set.

# Check: TCP/TCPCL loopback scenario exercised
if grep -q "Starting TCP loopback test." "$QEMU_OUTPUT"; then
    echo "✓ TCP/TCPCL loopback scenario ran"
    echo "[PASS] TCP/TCPCL loopback scenario ran" >> "$REPORT_OUTPUT"
else
    echo "✗ TCP/TCPCL loopback scenario not found"
    echo "[FAIL] TCP/TCPCL loopback scenario not found" >> "$REPORT_OUTPUT"
    VERIFICATION_FAILED=true
fi

# Check: CFDP loopback delivered
if grep -q "CFDP delivered:" "$QEMU_OUTPUT"; then
    echo "✓ CFDP loopback delivered"
    echo "[PASS] CFDP loopback delivered" >> "$REPORT_OUTPUT"
else
    echo "✗ CFDP loopback delivery not found"
    echo "[FAIL] CFDP loopback delivery not found" >> "$REPORT_OUTPUT"
    VERIFICATION_FAILED=true
fi

# Check: AMS loopback delivered
if grep -q "AMS delivered:" "$QEMU_OUTPUT"; then
    echo "✓ AMS loopback delivered"
    echo "[PASS] AMS loopback delivered" >> "$REPORT_OUTPUT"
else
    echo "✗ AMS loopback delivery not found"
    echo "[FAIL] AMS loopback delivery not found" >> "$REPORT_OUTPUT"
    VERIFICATION_FAILED=true
fi

# Write summary and exit
{
    echo ""
    echo "========================================"
} >> "$REPORT_OUTPUT"

if [ "$VERIFICATION_FAILED" = "true" ]; then
    {
        echo ""
        echo "OVERALL RESULT: FAILED"
        echo ""
        echo "One or more verification checks failed."
        echo "See details above for specific failures."
        echo ""
    } >> "$REPORT_OUTPUT"
    echo "Verification failed - see verification-report.txt for details"
    cat "$REPORT_OUTPUT"
    exit 1
else
    {
        echo ""
        echo "OVERALL RESULT: PASSED"
        echo ""
        echo "All verification checks passed successfully!"
        echo ""
    } >> "$REPORT_OUTPUT"
    echo "All verification checks passed!"
    cat "$REPORT_OUTPUT"
    exit 0
fi
