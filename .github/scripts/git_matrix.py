#!/usr/bin/env python3
"""
Finds executable demos and tests that run on Linux and has been benchmarked.
Then balances the tests across a number of runners between 1 & 7 to ensure each
runner has approximately the same execution time. The result is returned as a
JSON array.

NOTE: Any message print statements need to be to stderr as stdout is used to
return the batches of tests.

Nate Richard 2026/04/07 JPL
"""

import argparse
import glob
import heapq
import json
import os
import sys
from pathlib import Path
from typing import TypedDict


# --- Type Definitions ---
class Task(TypedDict):
    """Test with duration time"""

    path: Path
    duration: float


class Runner(TypedDict):
    """Runner with total execution time for tests with the folders"""

    id: int
    total_time: float
    folders: list[Path]


def find_tests() -> list[Path]:
    """Find a available tests.

    Returns:
        List of test Paths

    """
    # Find all test directories containing dotest files
    # Searches in both ./tests and ../demos (relative to tests directory)
    search_paths = [Path("./tests"), Path("./demos")]
    test_dirs: list[Path] = []

    print("Discovering tests...", file=sys.stderr)
    for search_path in search_paths:
        if not search_path.exists():
            continue

        # Find all dotest files, excluding those in .libs directories
        for dotest_file in search_path.rglob("dotest"):
            if ".libs" in dotest_file.parts:
                continue
            test_dirs.append(dotest_file.parent)

    test_dirs.sort()
    return test_dirs


def is_excluded(
    test_dir: Path,
    solaris_run: bool = False,
    mode: str = "native_bpsec",
    exclude_optional: bool = False,
) -> bool:
    """Check whether a test directory has any exclusion markers.

    Returns:
        True if the test should be skipped.
    """
    exclude_set = [
        ".exclude_bpv7",
        ".exclude_expert",
        ".exclude_all",
    ]

    if solaris_run:
        exclude_set.append(".exclude_solaris")
    else:
        exclude_set.extend([".exclude_linux", ".exclude_arc"])

    # Add the specific bpsec marker to the list based on the mode
    if mode == "bsl":
        exclude_set.append(".exclude_bsl")
    elif mode == "native_bpsec":
        exclude_set.append(".exclude_native_bpsec")

    # Add optional marker if requested
    if exclude_optional:
        exclude_set.append(".optional")

    # Check if ANY of the markers in our list exist in the directory
    return any((test_dir / marker).exists() for marker in exclude_set)


def list_tests(
    solaris_run: bool = False, bpsec: str = "native_bpsec", exclude_optional: bool = False
) -> list[Path]:
    """Generate list of tests.

    Returns:
        List of valid test for BPv7 & under expert mode

    """
    test_dirs = find_tests()
    valid_tests: list[Path] = []

    # Filter tests based on enabled status and exclusions
    for test_dir in test_dirs:
        dotest_path = test_dir / "dotest"

        # Check if dotest is executable (test is enabled)
        if not dotest_path.exists():
            continue

        if not os.access(dotest_path, os.X_OK):
            continue

        if is_excluded(test_dir, solaris_run, bpsec, exclude_optional):
            continue

        # Convert absolute test paths to relative paths for runtests wrapper.
        # The runtests script executes from within the tests/ directory, so all paths
        # must be relative to that location. Handle different nesting levels:
        #
        # Path structure analysis uses parent count to determine depth:
        # - Parent count 2: tests/foo (top-level test)
        # - Parent count 3: tests/subdir/foo (one level deep)
        # - Parent count 4: tests/subdir/subdir2/foo (two levels deep)

        if "demos" in str(test_dir):
            # Demo tests are siblings of tests/, so need ../ to go up from tests/
            test_path = ".." / test_dir
        elif len(test_dir.parents) == 3:
            # One subfolder deep: tests/subdir/testname -> subdir/testname
            test_path = Path(test_dir.parent.name) / Path(test_dir.name)
        elif len(test_dir.parents) == 4:
            # Two subfolders deep: tests/sub1/sub2/testname -> sub1/sub2/testname
            test_path = (
                Path(test_dir.parent.parent.name)
                / Path(test_dir.parent.name)
                / Path(test_dir.name)
            )
        else:
            # Top-level test: tests/testname -> testname
            test_path = Path(test_dir.name)

        valid_tests.append(test_path)

    return valid_tests


def main(
    runners: int = 7,
    subset: list[Path] | None = None,
    solaris_run: bool = False,
    bpsec: str = "native_bpsec",
    exclude_optional: bool = False,
) -> None:
    """Check list of existing tests and return JSON array for batching to GitHub
    Actions Runner Set.

    Args:
        runners: Number of test runners to distribute tests across
        subset: Optional list of specific tests to run, if None run all tests
    """
    if subset:
        valid_tests = subset
        # Filter out optional tests from the subset if requested
        if exclude_optional:
            filtered_tests = []
            for test in valid_tests:
                # Check both relative (to tests/) and absolute paths
                test_path = test if test.is_absolute() else (Path("tests") / test)
                if not (test_path / ".optional").exists():
                    filtered_tests.append(test)
            valid_tests = filtered_tests
    else:
        valid_tests = list_tests(solaris_run, bpsec, exclude_optional)
    if not valid_tests:
        sys.exit(1)

    task_data = get_folder_durations(valid_tests)
    schedule = balance_load(task_data, runners)
    print_schedule(schedule)


def get_folder_durations(folder_list: list[Path]) -> list[Task]:
    """Reads the .DURATION file in each folder to get test execution times.

    Args:
        folder_list: List of test folder paths to read duration files from

    Returns:
        List of Task dictionaries containing path and duration for each test
    """
    tasks: list[Task] = []

    for folder in folder_list:
        # add tests for file took up
        file_path = "tests" / folder / ".DURATION"
        duration = 0.0

        if file_path.exists() and file_path.is_file():
            try:
                content = file_path.read_text(encoding="utf-8").strip()
                if content:
                    duration = float(content)
            except ValueError:
                print(
                    f"[Warning] Could not parse number in: {file_path}. Defaulting to 0s.",
                    file=sys.stderr,
                )
            except OSError as e:
                print(f"[Warning] Error reading {file_path}: {e}", file=sys.stderr)
        else:
            print(f"[Warning] {file_path} not benchmarked, skipping.", file=sys.stderr)
            continue

        tasks.append({"path": folder, "duration": duration})
    return tasks


def balance_load(tasks: list[Task], num_runners: int) -> list[Runner]:
    """Distributes tasks to runners using the LPT (Longest Processing Time) greedy algorithm.

    Returns:
        List of runner typed dicts

    """
    sorted_tasks = sorted(tasks, key=lambda x: x["duration"], reverse=True)

    runners: list[Runner] = [
        {"id": i + 1, "total_time": 0.0, "folders": []} for i in range(num_runners)
    ]

    # Heap stores tuples: (current_total_time, runner_index)
    runner_heap: list[tuple[float, int]] = [(0.0, i) for i in range(num_runners)]
    heapq.heapify(runner_heap)

    for task in sorted_tasks:
        _, runner_idx = heapq.heappop(runner_heap)

        runners[runner_idx]["folders"].append(task["path"])
        runners[runner_idx]["total_time"] += task["duration"]

        new_load = runners[runner_idx]["total_time"]
        heapq.heappush(runner_heap, (new_load, runner_idx))

    return runners


def print_schedule(runners: list[Runner]) -> None:
    """Prints a JSON array where each element is a space-separated string
    of folder paths for a specific runner.

    Args:
        runners: List of runner dictionaries containing test assignments
    """
    output_groups: list[str] = []

    for idx, runner in enumerate(runners):
        print(f"Index {idx} duration: {runner['total_time']}", file=sys.stderr)
        sorted_folders = sorted(str(p) for p in runner["folders"])
        group_string = " ".join(sorted_folders)
        if group_string:
            output_groups.append(group_string)

    print(json.dumps(output_groups))


def resolve_test_list(tests: list[str]) -> list[Path]:
    """Resolve paths to subset of ION tests and/or demos.

    Returns:

    """
    test_subset = []
    for test_pattern in tests:
        matches = []

        # If the user explicitly provided the folder prefix, trust it
        if test_pattern.startswith(("tests/", "demos/")):
            matches.extend(glob.glob(test_pattern, recursive=True))
        else:
            # Prioritize searching inside tests/ and demos/ first
            matches.extend(glob.glob(f"tests/{test_pattern}", recursive=True))
            matches.extend(glob.glob(f"demos/{test_pattern}", recursive=True))

        # Fallback to the raw pattern if nothing was found in the subdirectories
        if not matches:
            matches.extend(glob.glob(test_pattern, recursive=True))

        if not matches:
            matches = [test_pattern]

        for match in matches:
            match_path = Path(match)

            # Extract valid test directories containing 'dotest'
            target_dirs = []
            if (match_path / "dotest").exists():
                target_dirs.append(match_path)
            elif match_path.is_dir():
                for dotest_file in sorted(match_path.rglob("dotest")):
                    if ".libs" not in dotest_file.parts:
                        target_dirs.append(dotest_file.parent)
            else:
                target_dirs.append(match_path)

            # Normalize directories so they work relative to `tests/`
            for test_dir in target_dirs:
                if "demos" in test_dir.parts:
                    idx = test_dir.parts.index("demos")
                    rel_path = Path("..") / Path(*test_dir.parts[idx:])
                elif "tests" in test_dir.parts:
                    idx = test_dir.parts.index("tests")
                    rel_path = Path(*test_dir.parts[idx + 1 :])
                else:
                    rel_path = test_dir

                if rel_path not in test_subset:
                    test_subset.append(rel_path)

    return test_subset


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Batching ION tests across runners.")
    parser.add_argument(
        "--runners",
        "-r",
        help="Number of runners to use, default: 7.",
        type=lambda v: int(float(v)),
        default=7,
    )
    parser.add_argument(
        "--tests",
        "-t",
        type=str,
        nargs="+",
        help="List of tests to distribute. If wildcarding, pass argument in double quotes.",
    )
    parser.add_argument(
        "--solaris",
        "-s",
        action="store_true",
        help="If passed, exclude any .exclude_solaris tests",
    )

    parser.add_argument(
        "--bsl",
        "-b",
        action="store_true",
        help="If passed, exclude native BPSec test, using BSL.",
    )

    parser.add_argument(
        "--exclude-optional",
        action="store_true",
        help="If passed, exclude tests marked with .optional files",
    )

    args = parser.parse_args()

    test_subset = []
    if args.tests:
        test_subset = resolve_test_list(args.tests)

    if args.bsl:
        current_bpsec_mode = "bsl"
    else:
        current_bpsec_mode = "native_bpsec"

    # Don't allow 0 runners and we cannot exceed 7 runners
    if args.runners < 1:
        RUNNER_COUNT = 1
    else:
        RUNNER_COUNT = args.runners

    main(RUNNER_COUNT, test_subset, args.solaris, current_bpsec_mode, args.exclude_optional)
