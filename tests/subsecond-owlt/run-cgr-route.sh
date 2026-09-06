#!/bin/sh
set -eu

if [ "$#" -ne 1 ]; then
	echo "usage: $0 /absolute/path/to/ion-build" >&2
	exit 2
fi

BUILD_DIR=$1
TEST_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
FIXTURE_DIR="$TEST_DIR/cgr"

for tool in bpadmin cgrfetch ionadmin ionexit
do
	if [ ! -x "$BUILD_DIR/$tool" ]; then
		echo "FAIL: expected executable $BUILD_DIR/$tool" >&2
		exit 2
	fi
done

if [ ! -d "$BUILD_DIR/.libs" ]; then
	echo "FAIL: expected in-tree library directory $BUILD_DIR/.libs" >&2
	exit 2
fi

WORK_DIR=$(mktemp -d /tmp/ion-subsecond-cgr.XXXXXX)
NODE_LIST_DIR=$(mktemp -d /tmp/ion-subsecond-cgr-nodes.XXXXXX)

cleanup()
{
	(
		cd "$WORK_DIR"
		env ION_NODE_LIST_DIR="$NODE_LIST_DIR" \
			PATH="$BUILD_DIR:$PATH" \
			LD_LIBRARY_PATH="$BUILD_DIR/.libs" \
			"$BUILD_DIR/bpadmin" stop.bprc >/dev/null 2>&1 || true
		env ION_NODE_LIST_DIR="$NODE_LIST_DIR" \
			PATH="$BUILD_DIR:$PATH" \
			LD_LIBRARY_PATH="$BUILD_DIR/.libs" \
			"$BUILD_DIR/ionadmin" stop.ionrc >/dev/null 2>&1 || true
		env ION_NODE_LIST_DIR="$NODE_LIST_DIR" \
			PATH="$BUILD_DIR:$PATH" \
			LD_LIBRARY_PATH="$BUILD_DIR/.libs" \
			"$BUILD_DIR/ionexit" >/dev/null 2>&1 || true
	)
	rm -rf "$WORK_DIR" "$NODE_LIST_DIR"
}

trap cleanup EXIT INT TERM
cp "$FIXTURE_DIR"/* "$WORK_DIR"/

run_case()
{
	plan=$1
	expected_first_hop=$2
	trace="$WORK_DIR/$plan.trace"

	cd "$WORK_DIR"
	env ION_NODE_LIST_DIR="$NODE_LIST_DIR" \
		PATH="$BUILD_DIR:$PATH" \
		LD_LIBRARY_PATH="$BUILD_DIR/.libs" \
		"$BUILD_DIR/ionadmin" node.ionrc >/dev/null
	env ION_NODE_LIST_DIR="$NODE_LIST_DIR" \
		PATH="$BUILD_DIR:$PATH" \
		LD_LIBRARY_PATH="$BUILD_DIR/.libs" \
		"$BUILD_DIR/ionadmin" "$plan.ionrc" >/dev/null
	env ION_NODE_LIST_DIR="$NODE_LIST_DIR" \
		PATH="$BUILD_DIR:$PATH" \
		LD_LIBRARY_PATH="$BUILD_DIR/.libs" \
		"$BUILD_DIR/bpadmin" node.bprc >/dev/null
	sleep 2

	env ION_NODE_LIST_DIR="$NODE_LIST_DIR" \
		PATH="$BUILD_DIR:$PATH" \
		LD_LIBRARY_PATH="$BUILD_DIR/.libs" \
		"$BUILD_DIR/cgrfetch" -j -d udp:127.0.0.1:4556 4 2>"$trace"

	first_hop=$(sed -n 's/.*PROPOSE firstHop to:\([0-9][0-9]*\).*/\1/p' \
		"$trace" | head -n 1)
	if [ "$first_hop" != "$expected_first_hop" ]; then
		cat "$trace" >&2
		echo "FAIL: $plan selected first hop ${first_hop:-NONE}; expected $expected_first_hop" >&2
		exit 1
	fi

	env ION_NODE_LIST_DIR="$NODE_LIST_DIR" \
		PATH="$BUILD_DIR:$PATH" \
		LD_LIBRARY_PATH="$BUILD_DIR/.libs" \
		"$BUILD_DIR/bpadmin" stop.bprc >/dev/null 2>&1 || true
	env ION_NODE_LIST_DIR="$NODE_LIST_DIR" \
		PATH="$BUILD_DIR:$PATH" \
		LD_LIBRARY_PATH="$BUILD_DIR/.libs" \
		"$BUILD_DIR/ionadmin" stop.ionrc >/dev/null 2>&1 || true
	env ION_NODE_LIST_DIR="$NODE_LIST_DIR" \
		PATH="$BUILD_DIR:$PATH" \
		LD_LIBRARY_PATH="$BUILD_DIR/.libs" \
		"$BUILD_DIR/ionexit" >/dev/null 2>&1 || true
	sleep 1
}

run_case fast-via-3 3
run_case fast-via-2 2

echo "PASS: CGR route choice follows millisecond OWLT (6 ms versus 9 ms)"
