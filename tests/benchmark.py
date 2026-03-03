#!/usr/bin/env python3

import argparse
import os
import subprocess
import sys
import time
from pathlib import Path
from statistics import mean

from git_matrix import list_tests


def _check_cached_duration(duration_file: Path, script_dir: Path) -> int | None:
    """Check if a cached duration exists and is valid.

    Returns:
        Cached duration in seconds, or None if cache is invalid/missing

    """
    if not duration_file.exists():
        return None

    try:
        content = duration_file.read_text(encoding="utf-8").strip()
        cached_val = int(content.split()[0])
        print(f"Skipping {script_dir}: Found cached duration ({cached_val}s)")
        return cached_val
    except (ValueError, IndexError, OSError):
        print(f"Cache file {duration_file} is corrupt. Re-benchmarking...")
        return None


def _setup_environment() -> dict[str, str]:
    """Set up environment variables for test execution.

    Returns:
        Environment dictionary with ION-specific variables

    """
    env = os.environ.copy()
    ion_dir = Path.cwd()
    configs_root = (ion_dir / "configs").resolve()
    env["CONFIGSROOT"] = str(configs_root)
    env["IONDIR"] = str(ion_dir)

    current_path = env.get("PATH", "")
    if "/usr/local/bin" not in current_path:
        env["PATH"] = f"/usr/local/bin:{current_path}"

    return env


def _get_cleanup(script_dir: Path) -> str:
    """Checks if there is a ./cleanup.

    Returns:
        ./cleanup if it exists otherwise killm

    """
    cleanup_path = script_dir / "cleanup"
    if cleanup_path.exists():
        return "./cleanup"
    return "killm"


def _run_cleanup(script_dir: Path, log_handle, env: dict[str, str]) -> None:
    """Execute cleanup script (killm) after test run."""
    cleanup_cmd = _get_cleanup(script_dir)
    try:
        subprocess.run(
            [cleanup_cmd],
            cwd=script_dir,
            stdout=log_handle,
            stderr=subprocess.STDOUT,
            text=True,
            env=env,
            check=True,
        )
    except OSError as cleanup_err:
        log_handle.write(f"\n[Warning] Failed to execute killm: {cleanup_err}\n")


def _run_single_iteration(
    script_dir: Path, iteration_num: int, log_handle, env: dict[str, str]
) -> float | None:
    """Run a single test iteration.

    Returns:
        Elapsed time in seconds, or None if the test failed

    """
    log_handle.write(f"--- Run {iteration_num + 1} ---\n")
    log_handle.flush()

    start_time = time.perf_counter()

    try:
        process = subprocess.run(
            ["./dotest"],
            cwd=script_dir,
            check=True,
            stdout=log_handle,
            stderr=subprocess.STDOUT,
            text=True,
            env=env,
            shell=True,
        )

        elapsed = time.perf_counter() - start_time
        _run_cleanup(script_dir, log_handle, env)

        if process.returncode != 0:
            log_handle.write(f"\n!!! FAILED ON ITERATION {iteration_num + 1} !!!\n")
            return None

        return elapsed

    except (subprocess.CalledProcessError, OSError) as e:
        error_msg = (
            f"Execution failed: {e}" if isinstance(e, OSError) else "Test failed"
        )
        log_handle.write(
            f"\n!!! FAILED ON ITERATION {iteration_num + 1}: {error_msg} !!!\n"
        )
        return None


def _save_duration(duration_file: Path, avg_seconds: int) -> None:
    """Save benchmark duration to cache file."""
    try:
        duration_file.write_text(f"{avg_seconds}\n", encoding="utf-8")
    except OSError as e:
        print(f"  Could not write to {duration_file}: {e}")


def benchmark_and_log(script_dir: Path, iterations: int = 5) -> int | str:
    """Runs a script up to the number of iterations (default: 5) with an environment context.
    - Captures stdout/stderr to {directory_name}.stdout.
    - Ensures standard paths (like /usr/local/bin) are visible to the subprocess.

    Returns:
        Average execution time or FAILED if exection failed at all

    """
    duration_file = script_dir / ".DURATION"
    output_file = script_dir / f"{script_dir.name}.stdout"

    # Check for cached duration
    cached_duration = _check_cached_duration(duration_file, script_dir)
    if cached_duration is not None:
        return cached_duration

    # Set up environment
    env = _setup_environment()
    print(f"Benchmarking {script_dir.name}...")

    # Run iterations and collect timings
    run_times: list[float] = []

    try:
        with open(output_file, "w", encoding="utf-8") as log_handle:
            for i in range(iterations):
                elapsed = _run_single_iteration(script_dir, i, log_handle, env)

                if elapsed is None:
                    print(f"  Iteration {i + 1} FAILED. See {output_file}")
                    return "FAILED"

                run_times.append(elapsed)

    except OSError as e:
        print(f"  Could not open log file {output_file}: {e}")
        return "FAILED"

    if not run_times:
        return 0

    # Calculate and save result
    avg_seconds = round(mean(run_times))
    _save_duration(duration_file, avg_seconds)

    return avg_seconds


def proc_results(results: list, display: bool = True) -> None:
    """Either print results to STDOUT or save to a file for later reference."""
    results.sort(key=lambda tup: tup[1], reverse=True)
    if display:
        print("\n" + "=" * 65)
        print(f"{'Script Location':<50} | {'Avg Time (s)':<12}")
        print("-" * 65)
        for path, res in results:
            print(f"{path:<50} | {res:<12}")
    else:
        with open("result.txt", "w", encoding="utf-8") as file:
            file.write("\n" + "=" * 65)
            file.write(f"\n{'Script Location':<50} | {'Avg Time (s)':<12}\n")
            file.write("-" * 65 + "\n")
            for path, res in results:
                file.write(f"{path:<50} | {res:<12}\n")


def main() -> None:
    """Orchestrates the search, directory context switching, and benchmarking."""
    parser = argparse.ArgumentParser(description="Benchmark ./dotest scripts.")

    parser.add_argument(
        "--target-path",
        "-p",
        nargs="?",
        type=Path,
        help="Optional path to a specific directory or dotest file.",
    )
    parser.add_argument(
        "--save-results",
        "-s",
        help="Save results to result.txt, otherwise print to screen.",
        action="store_false",
    )

    args = parser.parse_args()
    target_scripts: list[Path] = []

    if args.target_path:
        target: Path = args.target_path

        if target.is_file() and target.name == "dotest":
            target_scripts = [target]
        elif target.is_dir() and (target / "dotest").exists():
            target_scripts = [target / "dotest"]
        else:
            print(f"Error: Could not find a valid './dotest' script at {target}")
            sys.exit(1)

    else:
        target_scripts = list_tests()

    if not target_scripts:
        print("No 'dotest' scripts found.")
        sys.exit(1)

    results: list[tuple[str, int | str]] = []
    has_failures = False  # Track if any benchmark failed

    for script in target_scripts:
        result = benchmark_and_log("tests" / script)
        if result == "FAILED":
            has_failures = True
        results.append((str(script), result))

    proc_results(results, args.save_results)

    if has_failures:
        print("\n[!] One or more benchmarks failed. Exiting with error code 1.")
        sys.exit(1)


if __name__ == "__main__":
    main()
