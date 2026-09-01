#!/bin/sh
set -eu

if [ "$#" -ne 1 ]; then
	echo "usage: $0 /absolute/path/to/ion-build" >&2
	exit 2
fi

BUILD_DIR=$1
TEST_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
SOURCE_DIR=$(CDPATH= cd -- "$TEST_DIR/../.." && pwd)
WORK_DIR=$(mktemp -d /tmp/ion-subsecond-contact-runtime.XXXXXX)
NODE_LIST_DIR=$(mktemp -d /tmp/ion-subsecond-contact-runtime-nodes.XXXXXX)
TRACE_FILE="$WORK_DIR/contact-state.csv"
CLOCK_ERROR_TEXT=${CLOCK_ERROR_TEXT:-0}
CLOCK_ERROR_MS=${CLOCK_ERROR_MS:-0}
CONTACT_MODE_VALUE=${CONTACT_MODE:-subsecond}
POLL_MSEC_EXPLICIT=1
if [ -n "${ION_RFXCLOCK_POLL_MSEC+x}" ]; then
	POLL_MSEC=$ION_RFXCLOCK_POLL_MSEC
elif [ "$CONTACT_MODE_VALUE" = legacy ]; then
	POLL_MSEC=1000
	POLL_MSEC_EXPLICIT=0
else
	POLL_MSEC=10
fi

case $CLOCK_ERROR_MS in
*[!0-9]*|'')
	echo "FAIL: CLOCK_ERROR_MS must be a non-negative integer" >&2
	exit 2
	;;
esac

case $POLL_MSEC in
*[!0-9]*|'')
	echo "FAIL: ION_RFXCLOCK_POLL_MSEC must be an integer from 1 through 1000" >&2
	exit 2
	;;
esac
if [ "$POLL_MSEC" -lt 1 ] || [ "$POLL_MSEC" -gt 1000 ]; then
	echo "FAIL: ION_RFXCLOCK_POLL_MSEC must be an integer from 1 through 1000" >&2
	exit 2
fi

cleanup()
{
	(
		cd "$WORK_DIR"
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
cp "$TEST_DIR/runtime-node.ionconfig" "$WORK_DIR/"
cd "$WORK_DIR"
printf '1 93 runtime-node.ionconfig\nm clockerr %s\ns\nq\n' \
	"$CLOCK_ERROR_TEXT" > start.ionrc
printf 'x\nq\n' > stop.ionrc

cc -std=iso9899:2018 -Wall -Wextra -Werror \
	-I"$SOURCE_DIR/ici/include" -I"$SOURCE_DIR/ici/library" \
	-I"$BUILD_DIR" "$TEST_DIR/contact-state-probe.c" \
	-L"$BUILD_DIR/.libs" -Wl,-rpath,"$BUILD_DIR/.libs" \
	-o contact-state-probe -lici -lpthread -lm

if [ "$POLL_MSEC_EXPLICIT" -eq 1 ]; then
	env ION_NODE_LIST_DIR="$NODE_LIST_DIR" \
		ION_RFXCLOCK_POLL_MSEC="$POLL_MSEC" \
		PATH="$BUILD_DIR:$PATH" \
		LD_LIBRARY_PATH="$BUILD_DIR/.libs" \
		"$BUILD_DIR/ionadmin" start.ionrc >/dev/null
else
	env ION_NODE_LIST_DIR="$NODE_LIST_DIR" \
		PATH="$BUILD_DIR:$PATH" \
		LD_LIBRARY_PATH="$BUILD_DIR/.libs" \
		"$BUILD_DIR/ionadmin" start.ionrc >/dev/null
fi

now_ms=$(date -u +%s%3N)
case $CONTACT_MODE_VALUE in
legacy)
	start_ms=$((((now_ms / 1000) + 3) * 1000))
	stop_ms=$((start_ms + 2000))
	probe_samples=1600
	;;
subsecond)
	start_ms=$((now_ms + 2000))
	stop_ms=$((start_ms + 700))
	probe_samples=1000
	;;
*)
	echo "FAIL: CONTACT_MODE must be subsecond or legacy" >&2
	exit 2
	;;
esac
start_sec=$((start_ms / 1000))
stop_sec=$((stop_ms / 1000))
start_frac=$((start_ms % 1000))
stop_frac=$((stop_ms % 1000))
start_stamp=$(date -u -d "@$start_sec" +%Y/%m/%d-%H:%M:%S)
stop_stamp=$(date -u -d "@$stop_sec" +%Y/%m/%d-%H:%M:%S)

{
	printf 'a contact %s.%03d %s.%03d 93 94 100000\n' \
		"$start_stamp" "$start_frac" "$stop_stamp" "$stop_frac"
	printf 'q\n'
} | env ION_NODE_LIST_DIR="$NODE_LIST_DIR" \
	PATH="$BUILD_DIR:$PATH" \
	LD_LIBRARY_PATH="$BUILD_DIR/.libs" \
	"$BUILD_DIR/ionadmin" >/dev/null

env ION_NODE_LIST_DIR="$NODE_LIST_DIR" \
	PATH="$BUILD_DIR:$PATH" \
	LD_LIBRARY_PATH="$BUILD_DIR/.libs" \
	./contact-state-probe 94 "$probe_samples" > "$TRACE_FILE"

expected_start_ms=$((start_ms + CLOCK_ERROR_MS))
expected_stop_ms=$((stop_ms - CLOCK_ERROR_MS))
awk -F, -v expected_start="$expected_start_ms" \
	-v expected_stop="$expected_stop_ms" \
	-v clock_error="$CLOCK_ERROR_TEXT" \
	-v max_error="$((POLL_MSEC + 100))" '
$2 == 100000 && observed_start == 0 { observed_start = $1 }
$2 == 0 && observed_start > 0 && $1 >= observed_start { observed_stop = $1; exit }
END {
	if (observed_start == 0 || observed_stop == 0) {
		print "FAIL: live xmitRate did not complete 0 -> 100000 -> 0" > "/dev/stderr"
		exit 1
	}
	start_error = observed_start - expected_start
	stop_error = observed_stop - expected_stop
	if (start_error < -20 || start_error > max_error || stop_error < -20 || stop_error > max_error) {
		printf "FAIL: dispatch errors are start=%d ms stop=%d ms\n", start_error, stop_error > "/dev/stderr"
		exit 1
	}
	printf "PASS: %s live contact dispatch poll=%d ms clockerr=%s s start error=%d ms, stop error=%d ms\n", ENVIRON["CONTACT_MODE"] ? ENVIRON["CONTACT_MODE"] : "subsecond", max_error - 100, clock_error, start_error, stop_error
}' "$TRACE_FILE"
