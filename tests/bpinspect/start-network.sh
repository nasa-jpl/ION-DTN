#!/usr/bin/env bash

echo "Starting bpinspect test network..."
echo

# Check if ION is already running
if killm 2>&1 | grep -q "OK"; then
    echo "ERROR: ION is already running. Run ./cleanup first."
    exit 1
fi

# Setup environment
export ION_NODE_LIST_DIR=$PWD
rm -f ./ion_nodes

echo "Starting node 1 (hub)..."
(cd node1; ./ionstart) &
sleep 2

echo "Starting node 2 (immediate link)..."
(cd node2; ./ionstart) &
sleep 2

echo "Starting node 3 (delayed link)..."
(cd node3; ./ionstart) &
sleep 2

echo
echo "Waiting for initialization..."
sleep 3

echo
echo "Verifying nodes..."
if (cd node1; bpadmin <<< "i") >/dev/null 2>&1; then
    echo "  ✓ Node 1 running"
else
    echo "  ✗ Node 1 failed"
fi

if (cd node2; bpadmin <<< "i") >/dev/null 2>&1; then
    echo "  ✓ Node 2 running"
else
    echo "  ✗ Node 2 failed"
fi

if (cd node3; bpadmin <<< "i") >/dev/null 2>&1; then
    echo "  ✓ Node 3 running"
else
    echo "  ✗ Node 3 failed"
fi

echo
echo "Network started successfully!"
echo
echo "Test environment ready:"
echo "  - Node 1 (ipn:1.0): Hub node"
echo "  - Node 2 (ipn:2.0): Connected, immediate contact at 5 KB/s"
echo "  - Node 3 (ipn:3.0): Connected, delayed 24h at 50 KB/s"
echo
echo "Bundles sent to ipn:3.* will queue in node 1 for bpinspect testing."
echo
echo "To interact with nodes, cd into node1/, node2/, or node3/ subdirectories."
echo
