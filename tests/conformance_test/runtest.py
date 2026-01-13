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


def bptrace() -> None:
    """Run bptrace as if it were in node 2 directory."""
    trace_cmd = "bptrace ipn:2.1 ipn:3.1 dtn:none 300 0.1 'Hello!'"
    subprocess.run(shlex.split(trace_cmd), cwd=NODE2DIR, check=True)


def bpstats() -> None:
    """Run bpstats as if it were in node 3 directory."""
    nodedir = Path.cwd().joinpath("3.ipn.udp")
    subprocess.run(["bpstats"], cwd=nodedir, check=True)


def bpstats_check(bundle_count: int) -> int:
    """Check that bpstats increased correctly"""
    actual_count, actual_size = ion_parse.bpstats_parse("rcv", "+", str(NODE3LOG))

    return ion_parse.bpstats_compare(
        bundle_count, (7 * bundle_count), actual_count, actual_size
    )


def main(bundles: list[str], mapping: dict[int, dict]) -> None:
    """Run tests of conformance suite."""
    total_bundles = 0
    for num, bundle in enumerate(bundles, 1):
        print(f"Running test of bundle {num}...")
        bundle_send(bundle)
        time.sleep(1)
        bptrace()
        time.sleep(1)
        bpstats()
        time.sleep(1)

        if mapping[num]:
            log_result = ion_parse.ion_log_parse(mapping[num]["msg"], str(NODE3LOG))
        else:
            log_result = 1
        total_bundles += mapping[num]["bundle_thru"]
        stats_result = bpstats_check(total_bundles)
        if log_result != 0 and stats_result == 0:
            print(f"Bundle {num} success")
        else:
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
    }

    main(bundle_list, expected_msgs)
