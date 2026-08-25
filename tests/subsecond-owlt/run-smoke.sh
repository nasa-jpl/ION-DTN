#!/bin/sh
set -eu

if [ "$#" -ne 1 ]; then
	echo "usage: $0 /absolute/path/to/ion-build" >&2
	exit 2
fi

BUILD_DIR=$1
TEST_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
NODE_LIST_DIR=$(mktemp -d /tmp/ion-subsecond-node-list.XXXXXX)
OUTPUT_FILE=$(mktemp /tmp/ion-subsecond-output.XXXXXX)

cleanup()
{
	(
		cd "$TEST_DIR"
		env ION_NODE_LIST_DIR="$NODE_LIST_DIR" \
			LD_LIBRARY_PATH="$BUILD_DIR/.libs" \
			"$BUILD_DIR/ionexit" >/dev/null 2>&1 || true
	)
	rm -f "$OUTPUT_FILE"
	rm -rf "$NODE_LIST_DIR"
}

trap cleanup EXIT INT TERM
cd "$TEST_DIR"
rm -f ranges.ionrc
env ION_NODE_LIST_DIR="$NODE_LIST_DIR" \
	LD_LIBRARY_PATH="$BUILD_DIR/.libs" \
	"$BUILD_DIR/ionadmin" decimal-range.ionrc >"$OUTPUT_FILE" 2>&1

PRINTED_COUNT=$(grep -c "is 0.006 seconds" "$OUTPUT_FILE" || true)
if [ "$PRINTED_COUNT" -ne 2 ]; then
	cat "$OUTPUT_FILE" >&2
	echo "FAIL: asserted and imputed ranges did not retain 6 ms" >&2
	exit 1
fi

if ! grep -q "91 92 0.006" ranges.ionrc; then
	cat ranges.ionrc >&2
	echo "FAIL: briefing export lost the 6 ms OWLT" >&2
	exit 1
fi

if ! grep -Eq "91 92 1$" ranges.ionrc; then
	cat ranges.ionrc >&2
	echo "FAIL: legacy integer OWLT output changed" >&2
	exit 1
fi

if ! grep -q "Invalid OWLT" "$OUTPUT_FILE"; then
	cat "$OUTPUT_FILE" >&2
	echo "FAIL: unsupported sub-millisecond input was not rejected" >&2
	exit 1
fi

echo "PASS: subsecond OWLT parser, storage, symmetry, and export"
