#!/bin/sh
set -eu

if [ "$#" -ne 1 ]; then
	echo "usage: $0 /absolute/path/to/ion-build" >&2
	exit 2
fi

BUILD_DIR=$1
TEST_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
SOURCE_DIR=$(CDPATH= cd -- "$TEST_DIR/../.." && pwd)
WORK_DIR=$(mktemp -d /tmp/ion-subsecond-utcdelta.XXXXXX)
NODE_LIST_DIR=$(mktemp -d /tmp/ion-subsecond-utcdelta-nodes.XXXXXX)

cleanup()
{
	(
		cd "$WORK_DIR"
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
cp "$TEST_DIR/runtime-node.ionconfig" "$WORK_DIR/"
cd "$WORK_DIR"
printf '1 93 runtime-node.ionconfig\ns\nq\n' > start.ionrc
printf 'x\nq\n' > stop.ionrc

cc -std=iso9899:2018 -Wall -Wextra -Werror \
	-I"$SOURCE_DIR/ici/include" -I"$SOURCE_DIR/ici/library" \
	-I"$SOURCE_DIR/bpv7/include" -I"$SOURCE_DIR/bpv7/library" \
	-I"$BUILD_DIR" "$TEST_DIR/utcdelta-probe.c" \
	-L"$BUILD_DIR/.libs" -Wl,-rpath,"$BUILD_DIR/.libs" \
	-o utcdelta-probe -lbp -lici -lpthread -lm

env ION_NODE_LIST_DIR="$NODE_LIST_DIR" PATH="$BUILD_DIR:$PATH" \
	LD_LIBRARY_PATH="$BUILD_DIR/.libs" \
	"$BUILD_DIR/ionadmin" start.ionrc >/dev/null

check_delta()
{
	text=$1
	expected_ms=$2
	printf 'm utcdelta %s\nq\n' "$text" | \
		env ION_NODE_LIST_DIR="$NODE_LIST_DIR" PATH="$BUILD_DIR:$PATH" \
			LD_LIBRARY_PATH="$BUILD_DIR/.libs" \
			"$BUILD_DIR/ionadmin" >/dev/null
	env ION_NODE_LIST_DIR="$NODE_LIST_DIR" PATH="$BUILD_DIR:$PATH" \
		LD_LIBRARY_PATH="$BUILD_DIR/.libs" \
		./utcdelta-probe "$expected_ms" 20
}

check_delta 0.250 250
check_delta -0.125 -125
check_delta +2 2000
check_delta -2.375 -2375

invalid_output=$WORK_DIR/invalid.out
printf 'm utcdelta 0.1234\nm utcdelta bad\nq\n' | \
	env ION_NODE_LIST_DIR="$NODE_LIST_DIR" PATH="$BUILD_DIR:$PATH" \
		LD_LIBRARY_PATH="$BUILD_DIR/.libs" \
		"$BUILD_DIR/ionadmin" >"$invalid_output" 2>&1
if [ "$(grep -c 'Invalid UTC delta' "$invalid_output")" -ne 2 ]; then
	cat "$invalid_output" >&2
	echo "FAIL: malformed fractional UTC deltas were not rejected" >&2
	exit 1
fi

echo "PASS: signed millisecond UTC delta parsing, storage, ctime, and BP DTN time"
