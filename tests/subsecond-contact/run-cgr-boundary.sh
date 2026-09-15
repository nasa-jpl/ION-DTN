#!/bin/sh
set -eu

if [ "$#" -ne 1 ]; then
	echo "usage: $0 /absolute/path/to/ion-build" >&2
	exit 2
fi

BUILD_DIR=$1
TEST_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
FIXTURE_DIR="$TEST_DIR/cgr-boundary"
WORK_DIR=$(mktemp -d /tmp/ion-subsecond-contact-cgr.XXXXXX)
NODE_LIST_DIR=$(mktemp -d /tmp/ion-subsecond-contact-cgr-nodes.XXXXXX)
TRACE_FILE="$WORK_DIR/cgr.trace"

cleanup()
{
	(
		cd "$WORK_DIR"
		env ION_NODE_LIST_DIR="$NODE_LIST_DIR" PATH="$BUILD_DIR:$PATH" \
			LD_LIBRARY_PATH="$BUILD_DIR/.libs" \
			"$BUILD_DIR/bpadmin" stop.bprc >/dev/null 2>&1 || true
		env ION_NODE_LIST_DIR="$NODE_LIST_DIR" PATH="$BUILD_DIR:$PATH" \
			LD_LIBRARY_PATH="$BUILD_DIR/.libs" \
			"$BUILD_DIR/ionadmin" stop.ionrc >/dev/null 2>&1 || true
		env ION_NODE_LIST_DIR="$NODE_LIST_DIR" PATH="$BUILD_DIR:$PATH" \
			LD_LIBRARY_PATH="$BUILD_DIR/.libs" \
			"$BUILD_DIR/ionexit" >/dev/null 2>&1 || true
	)
	rm -rf "$WORK_DIR" "$NODE_LIST_DIR"
}

trap cleanup EXIT INT TERM
cp "$FIXTURE_DIR"/* "$WORK_DIR"/
cd "$WORK_DIR"
env ION_NODE_LIST_DIR="$NODE_LIST_DIR" PATH="$BUILD_DIR:$PATH" \
	LD_LIBRARY_PATH="$BUILD_DIR/.libs" \
	"$BUILD_DIR/ionadmin" node.ionrc >/dev/null
env ION_NODE_LIST_DIR="$NODE_LIST_DIR" PATH="$BUILD_DIR:$PATH" \
	LD_LIBRARY_PATH="$BUILD_DIR/.libs" \
	"$BUILD_DIR/ionadmin" fractional-boundary.ionrc >/dev/null
env ION_NODE_LIST_DIR="$NODE_LIST_DIR" PATH="$BUILD_DIR:$PATH" \
	LD_LIBRARY_PATH="$BUILD_DIR/.libs" \
	"$BUILD_DIR/bpadmin" node.bprc >/dev/null
sleep 2

env ION_NODE_LIST_DIR="$NODE_LIST_DIR" PATH="$BUILD_DIR:$PATH" \
	LD_LIBRARY_PATH="$BUILD_DIR/.libs" \
	"$BUILD_DIR/cgrfetch" -j -d udp:127.0.0.1:4556 4 2>"$TRACE_FILE"
first_hop=$(sed -n 's/.*PROPOSE firstHop to:\([0-9][0-9]*\).*/\1/p' \
	"$TRACE_FILE" | head -n 1)
if [ "$first_hop" != 3 ]; then
	cat "$TRACE_FILE" >&2
	echo "FAIL: fractional contact boundaries selected ${first_hop:-NONE}; expected 3" >&2
	exit 1
fi

echo "PASS: CGR route choice honors fractional contact boundaries"
