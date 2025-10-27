#!/bin/bash
#
# Quick test to verify adjacent contacts are accepted and configured correctly
# Based on demos/bench-ltp pattern
#

set -e

TESTDIR=$(cd "$(dirname "$0")" && pwd)

echo "Quick Adjacent Contacts Test"
echo "============================="
echo ""

# Cleanup function
cleanup() {
    echo ""
    echo "Cleaning up..."
    cd "$TESTDIR/1.adjacent" 2>/dev/null && killm 2>/dev/null || killm 2>/dev/null || true
    sleep 1
}

trap cleanup EXIT INT TERM

# Cleanup any existing ION instance
killm 2>/dev/null || true
sleep 2

echo "Starting ION with adjacent contacts configuration..."
echo ""

# Set ION_NODE_LIST_DIR following bench-ltp pattern
export ION_NODE_LIST_DIR="$TESTDIR"
rm -f "$TESTDIR/ion_nodes"

# Start ION node
cd "$TESTDIR/1.adjacent"
./ionstart > ionadmin_output.txt 2>&1

# Give ION time to initialize
sleep 3

# Check if ION started
if ! pgrep -x "rfxclock" > /dev/null; then
    echo "ERROR: rfxclock did not start"
    echo ""
    echo "Startup output:"
    cat ionadmin_output.txt
    exit 1
fi

echo "ION started successfully"
echo ""

# Use ionadmin to list contacts and verify they were accepted
echo "Listing configured contacts..."
echo "l contact" | ionadmin 2>&1 | tee contact_list.txt

# Check if both adjacent contacts were accepted
CONTACT_COUNT=$(grep -E "1\s*->\s*2" contact_list.txt | wc -l)

echo ""
if [ "$CONTACT_COUNT" -ge 2 ]; then
    echo "SUCCESS: Found $CONTACT_COUNT contacts for pair 1->2"
    echo "Adjacent contacts were accepted (no overlap error)."
    echo ""
    echo "Contact details:"
    grep -E "1\s*->\s*2" contact_list.txt || echo "(No contacts found)"
else
    echo "WARNING: Only found $CONTACT_COUNT contacts for pair 1->2"
    echo "Expected at least 2 adjacent contacts."
    echo ""
    echo "All contacts:"
    cat contact_list.txt
fi

# Show any error messages from ion.log
if [ -f ion.log ]; then
    echo ""
    echo "Checking for errors in ion.log:"
    if grep -i "overlap\|error\|fail" ion.log; then
        echo ""
        echo "Errors found in log!"
    else
        echo "(No errors found)"
    fi
fi

echo ""
echo "Test complete."
# Cleanup will be handled by trap
exit 0
