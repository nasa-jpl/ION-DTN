#!/usr/bin/env python3
"""Script that provides functionality to parse the ION log for certain information."""
import argparse
import re
import sys


def bpstats_parse(bundle_type: str, flow: str, logfile: str) -> tuple[int, int]:
    """Looks for the first instance where bpstats is called in ion.log and
    returns the number of bundles and total size. This will search based on the
    network type and which traffic flow."""
    bpstats = False
    stats_lines = []

    with open(logfile, "r", encoding="utf-8") as file:
        for line in file.readlines():
            if "Start of statistics snapshot" in line:
                bpstats = True
            if bpstats and bundle_type in line:
                stats_lines.append(line)

    if not stats_lines:
        return 0, 0

    if flow == "+":
        flow = r"\+"
    result = re.search(rf"{flow}\) \d+ \d+", stats_lines[-1])

    if result is not None:
        paired_values = result.group(0).split(")")[1].strip()
        bundle_results = paired_values.split(" ")
        return int(bundle_results[0]), int(bundle_results[1])

    return 0, 0


def ion_log_parse(msg: str, logfile: str) -> int:
    """Simple parser to determine whether the desired log message is in ion.log.
    Returns int so it can be used with bash script."""
    with open(logfile, "r", encoding="utf-8") as file:
        for num, line in enumerate(file):
            if msg in line:
                return num + 1

    return 0


def bpstats_compare(
    expected_count: int, expected_size: int, actual_count: int, actual_size: int
) -> int:
    """Checks values returned from bpstats meet expected results."""
    if expected_count == actual_count and expected_size == actual_size:
        return 0
    return 1


def main() -> None:
    """Main function to handle argument parsing for the different ion.log
    parsing needs."""
    parser = argparse.ArgumentParser(
        description="Python script to check for different bits of info in the ION log file."
    )
    parser.add_argument(
        "--bundle-type",
        "-b",
        help="Which type of bundle traffic .to find in bpstats.",
        choices=["src", "fwd", "xmt", "rcv", "dlv", "ctr", "rfw", "exp"],
    )
    parser.add_argument(
        "--traffic-flow",
        "-f",
        help="Which traffic flow to check or use + for total",
        choices=["0", "1", "2", "+"],
    )
    parser.add_argument(
        "--expected-count", "-c", help="Expected number of bundles in bpstats", type=int
    )
    parser.add_argument(
        "--expected-size", "-s", help="Expected total bundle size", type=int
    )
    parser.add_argument(
        "--log-str", "-l", help="String to check if it exists in logfile."
    )
    parser.add_argument("logfile", help="Path to ion.log file.")

    args = parser.parse_args()

    if args.bundle_type:
        actual_count, actual_size = bpstats_parse(
            args.bundle_type, args.traffic_flow, args.logfile
        )
        print(f"Bundle(s) sent: {actual_count}\nTotal size of bundle(s): {actual_size}")
        sys.exit(
            bpstats_compare(
                args.expected_count, args.expected_size, actual_count, actual_size
            )
        )

    if args.log_str:
        line_num = ion_log_parse(args.log_str, args.logfile)
        if line_num == 0:
            print("No line found")
            sys.exit(1)
        print(f"{line_num}")
        sys.exit(0)

    print("Additional arguments required for proper parsing")
    sys.exit(1)


if __name__ == "__main__":
    main()
