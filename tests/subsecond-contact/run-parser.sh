#!/bin/sh
set -eu

if [ "$#" -ne 1 ]; then
	echo "usage: $0 /absolute/path/to/ion-build" >&2
	exit 2
fi

BUILD_DIR=$1
TEST_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
WORK_DIR=$(mktemp -d /tmp/ion-subsecond-contact.XXXXXX)
NODE_LIST_DIR=$(mktemp -d /tmp/ion-subsecond-contact-nodes.XXXXXX)
OUTPUT_FILE="$WORK_DIR/ionadmin.out"

cleanup()
{
	(
		cd "$WORK_DIR"
		env ION_NODE_LIST_DIR="$NODE_LIST_DIR" \
			LD_LIBRARY_PATH="$BUILD_DIR/.libs" \
			"$BUILD_DIR/ionexit" >/dev/null 2>&1 || true
	)
	rm -rf "$WORK_DIR" "$NODE_LIST_DIR"
}

trap cleanup EXIT INT TERM
cp "$TEST_DIR/node.ionconfig" "$TEST_DIR/decimal-contact.ionrc" "$WORK_DIR/"
cd "$WORK_DIR"

env ION_NODE_LIST_DIR="$NODE_LIST_DIR" \
	LD_LIBRARY_PATH="$BUILD_DIR/.libs" \
	"$BUILD_DIR/ionadmin" decimal-contact.ionrc >"$OUTPUT_FILE" 2>&1

BRIEF_FILE="$WORK_DIR/contacts.1.ionrc"
if [ ! -f "$BRIEF_FILE" ]; then
	cat "$OUTPUT_FILE" >&2
	echo "FAIL: contact briefing export was not created" >&2
	exit 1
fi

if ! grep -q '2030/01/01-00:00:00.500 2030/01/01-00:00:01.200' "$BRIEF_FILE"; then
	cat "$BRIEF_FILE" >&2
	echo "FAIL: contact parser/storage/export did not preserve .500/.200 milliseconds" >&2
	exit 1
fi

INFO_OUTPUT="$WORK_DIR/info.out"
printf 'l contact\ni contact 2030/01/01-00:00:00.500 93 94\nq\n' | \
	env ION_NODE_LIST_DIR="$NODE_LIST_DIR" \
		LD_LIBRARY_PATH="$BUILD_DIR/.libs" \
		"$BUILD_DIR/ionadmin" >"$INFO_OUTPUT" 2>&1
if [ "$(grep -c '00:00:00.500' "$INFO_OUTPUT")" -lt 2 ]; then
	cat "$INFO_OUTPUT" >&2
	echo "FAIL: fractional contact could not be queried by exact key" >&2
	exit 1
fi

CHANGE_OUTPUT="$WORK_DIR/change.out"
DELETE_OUTPUT="$WORK_DIR/delete.out"
printf 'c contact 2030/01/01-00:00:00.500 93 94 123456\nl contact\nq\n' | \
	env ION_NODE_LIST_DIR="$NODE_LIST_DIR" \
		LD_LIBRARY_PATH="$BUILD_DIR/.libs" \
		"$BUILD_DIR/ionadmin" >"$CHANGE_OUTPUT" 2>&1
if ! grep -q '123456 bytes/sec' "$CHANGE_OUTPUT"; then
	cat "$CHANGE_OUTPUT" >&2
	echo "FAIL: fractional contact could not be revised by exact key" >&2
	exit 1
fi

printf 'd contact 2030/01/01-00:00:00.500 93 94\nl contact\nq\n' | \
	env ION_NODE_LIST_DIR="$NODE_LIST_DIR" \
		LD_LIBRARY_PATH="$BUILD_DIR/.libs" \
		"$BUILD_DIR/ionadmin" >"$DELETE_OUTPUT" 2>&1
if grep -q '00:00:00.500' "$DELETE_OUTPUT"; then
	cat "$DELETE_OUTPUT" >&2
	echo "FAIL: fractional contact could not be deleted by exact key" >&2
	exit 1
fi

echo "PASS: subsecond contact parser, storage, and export"
