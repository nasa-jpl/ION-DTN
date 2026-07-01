#!/usr/bin/env python3
"""
Script to pass conformance testing bundles through ION and verify they
pass/fail correctly.

Nate Richard JPL
2026-01-12
"""
import argparse
import shlex
import subprocess
import sys
import time
from pathlib import Path

import ion_parse
from binary_bundle_send import bundle_send

MAINDIR = Path.cwd()
NODE2DIR = MAINDIR.joinpath("2.ipn.udp")
NODE3DIR = MAINDIR.joinpath("3.ipn.udp")
NODE3LOG = NODE3DIR.joinpath("ion.log")

# Upper bound on how long to wait for node 3 to finish acquiring the injected
# bundle and the relayed bptrace bundle, and to flush ion.log/bpstats, before
# declaring a bundle's verification failed.  ION processes and logs these
# asynchronously from separate daemons, so we poll rather than sampling once
# after a fixed delay (the single-sample approach raced on slower CI runners).
VERIFY_TIMEOUT = 20


def bptrace() -> None:
    """Run bptrace as if it were in node 2 directory."""
    trace_cmd = "bptrace ipn:2.1 ipn:3.1 dtn:none 300 0.1 'Hello!'"
    subprocess.run(shlex.split(trace_cmd), cwd=NODE2DIR, check=True)


def bpstats() -> None:
    """Run bpstats as if it were in node 3 directory."""
    nodedir = Path.cwd().joinpath("3.ipn.udp")
    subprocess.run(["bpstats"], cwd=nodedir, check=True)


# Every bundle that reaches node 3 (the bptrace probe and the pass-through
# conformance bundles) carries a 7-byte payload, so the received byte total is
# normally 7 per bundle.  A bundle may override this via "bytes_thru" (e.g. the
# zero-length-payload bundle contributes 0 bytes), otherwise it defaults here.
BYTES_PER_BUNDLE = 7


def bpstats_check(expected_count: int, expected_size: int) -> int:
    """Check that bpstats increased to the expected received count and size."""
    actual_count, actual_size = ion_parse.bpstats_parse("rcv", "+", str(NODE3LOG))

    return ion_parse.bpstats_compare(
        expected_count, expected_size, actual_count, actual_size
    )


def main(bundles: list[str], mapping: dict[int, dict]) -> None:
    """Run tests of conformance suite."""
    total_bundles = 0
    total_bytes = 0
    for num, bundle in enumerate(bundles, 1):
        print(f"Running test of bundle {num}...")
        bundle_send(bundle)
        time.sleep(1)
        # bptrace is sent exactly once per bundle; re-sending it would skew the
        # delivered-bundle counts that bpstats_check verifies.
        bptrace()
        time.sleep(1)
        total_bundles += mapping[num]["bundle_thru"]
        total_bytes += mapping[num].get(
            "bytes_thru", BYTES_PER_BUNDLE * mapping[num]["bundle_thru"]
        )

        # Poll for the expected outcome instead of sampling once after a fixed
        # delay.  bpstats() freezes a snapshot when it runs, so we must re-run
        # it each iteration to observe a newer count; the log message likewise
        # may not be flushed yet on a busy runner.
        deadline = time.monotonic() + VERIFY_TIMEOUT
        while True:
            bpstats()
            time.sleep(1)
            if mapping[num]["msg"]:
                log_result = ion_parse.ion_log_parse(
                    mapping[num]["msg"], str(NODE3LOG)
                )
            else:
                log_result = 1
            stats_result = bpstats_check(total_bundles, total_bytes)
            if log_result != 0 and stats_result == 0:
                print(f"Bundle {num} success")
                break
            if time.monotonic() >= deadline:
                print(f"Bundle {num} failure")
                sys.exit(1)
    sys.exit(0)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Sends bundle(s) that are as hexadecimal string(s) via command line or a file."
    )
    parser.add_argument(
        "bundle_list",
        help="list of bundles to send",
    )

    args = parser.parse_args()
    with open(args.bundle_list, "r", encoding="utf-8") as hfile:
        init_bundle_list = hfile.readlines()
    # Make sure there are no newlines
    bundle_list = [item.strip() for item in init_bundle_list if "#" not in item]
    expected_msgs = {
        1: {
            "msg": "No primary block BIB or primary block CRC",
            "bundle_thru": 1,
        },  # Inauthentic
        2: {
            "msg": "Malformed extension block",
            "bundle_thru": 1,
        },  # Malformed Ext. Block
        3: {"msg": "", "bundle_thru": 2},  # No BAE, but DTN Time. Should pass through
        4: {
            "msg": "",
            "bundle_thru": 2,
        },  # Non-standard Ext. block overlap, should pass through
        5: {
            "msg": "Block number exceeds maximum supported value",
            "bundle_thru": 1,
        },  # Large block number
        6: {
            "msg": "",
            "bundle_thru": 2,
            "bytes_thru": 7,
        },  # Zero-length payload (GHSA-27wg-h3xq-4p9g): a legal empty payload
        # must be acquired and delivered, not crash acquisition.  Both the
        # injected bundle and the bptrace probe are received (count += 2), but
        # only the probe carries bytes, so bytes_thru is 7 (not 14).  A
        # vulnerable node aborts on the ZCO length assertion or discards the
        # bundle as malformed, so the injected bundle never reaches node 3 and
        # the received count fails to advance to 2.
    }

    main(bundle_list, expected_msgs)
