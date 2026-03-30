#!/usr/bin/env python3
"""
Finds executable demos and tests that run on Linux and has been benchmarked.
Then balances the tests across a number of runners between 1 & 7 to ensure each
runner has approximately the same execution time. The result is returned as a
JSON array.

NOTE: Any message print statements need to be to stderr as stdout is used to
return the batches of tests.

Nate Richard 2026/03/24 JPL
"""

import argparse
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


def is_excluded(test_dir: Path) -> bool:
    """Check whether a test directory has any exclusion markers.

    Returns:
        True if the test should be skipped.

    """
    for marker in (
        ".exclude_bpv7",
        ".exclude_expert",
        ".exclude_linux",
        ".exclude_arc",
        ".exclude_all",
    ):
        if (test_dir / marker).exists():
            return True
    return False


def list_tests() -> list[Path]:
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

        if is_excluded(test_dir):
            continue

        # runtests wrapper is run from within the tests directory so need paths relative to that
        # can handle up to 2 levels of sub-folders
        if "demos" in str(test_dir):
            test_path = ".." / test_dir
        # means there is one sub-test folder
        elif len(test_dir.parents) == 3:
            test_path = Path(test_dir.parent.name) / Path(test_dir.name)
        # means there is sub-folder within the sub-folder
        elif len(test_dir.parents) == 4:
            test_path = (
                Path(test_dir.parent.parent.name)
                / Path(test_dir.parent.name)
                / Path(test_dir.name)
            )
        else:
            test_path = Path(test_dir.name)

        valid_tests.append(test_path)

    return valid_tests


def main(runners: int = 7, subset: list[Path] | None = None) -> None:
    """Check list of existing tests and return JSON array for batching to GitHub
    Actions Runner Set."""
    if subset:
        valid_tests = subset
    else:
        valid_tests = list_tests()
    if not valid_tests:
        sys.exit(1)

    task_data = get_folder_durations(valid_tests)
    schedule = balance_load(task_data, runners)
    print_schedule(schedule)


def get_folder_durations(folder_list: list[Path]) -> list[Task]:
    """Reads the .DURATION file in each folder.

    Returns:
        list of Task dictionaries.

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
    """
    output_groups: list[str] = []

    for idx, runner in enumerate(runners):
        print(f"Index {idx} duration: {runner['total_time']}", file=sys.stderr)
        sorted_folders = sorted(str(p) for p in runner["folders"])
        group_string = " ".join(sorted_folders)
        if group_string:
            output_groups.append(group_string)

    print(json.dumps(output_groups))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Batching ION tests across runners.")
    parser.add_argument(
        "--runners",
        "-r",
        help="Number of runners to use, default: 7.",
        type=int,
        default=7,
    )
    parser.add_argument(
        "--tests", "-t", type=str, nargs="+", help="List of tests to distribute."
    )

    args = parser.parse_args()

    test_subset = []
    if args.tests:
        for test in args.tests:
            test_path = Path(test)
            # If the path is a directory with dotest, use it directly.
            # Otherwise expand it: find all dotest scripts underneath.
            tests_dir = Path("tests") / test_path
            if (tests_dir / "dotest").exists():
                test_subset.append(test_path)
            elif tests_dir.is_dir():
                for dotest_file in sorted(tests_dir.rglob("dotest")):
                    if ".libs" not in dotest_file.parts:
                        test_subset.append(
                            dotest_file.parent.relative_to(Path("tests"))
                        )
            else:
                test_subset.append(test_path)

    # Don't allow 0 runners and we cannot exceed 7 runners
    if args.runners < 1:
        RUNNER_COUNT = 1
    elif args.runners > 7:
        RUNNER_COUNT = 7
    else:
        RUNNER_COUNT = args.runners

    main(RUNNER_COUNT, test_subset)
