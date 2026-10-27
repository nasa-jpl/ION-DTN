import os
import subprocess
import sys
from pathlib import Path


def get_env(var, default=None) -> str | None:
    """Retrieve environment variable or return default."""
    return os.environ.get(var, default)


def get_changed_files(pr_number) -> list:
    """Fetch changed files for a PR using gh CLI, returning a list of paths."""
    try:
        cmd = f"gh pr diff {pr_number} --name-only"
        result = subprocess.run(
            cmd, shell=True, capture_output=True, text=True, check=True
        )
        return result.stdout.strip().splitlines()
    except subprocess.CalledProcessError as e:
        print(
            f"::warning::gh CLI failed (exit {e.returncode}) - falling back to full test suite",
            file=sys.stderr,
        )
        return []


def process_changed_files(changed_files: list[str]) -> set[str]:
    """Extract test directory paths from a list of changed file paths.

    For files under ``tests/`` or ``demos/``, the full relative path from the
    ``tests/`` or ``demos/`` prefix to the file's parent directory is collected.
    This preserves nested directory structure (e.g.,
    ``tests/cbr-ct-orange-book/cbr-aggr-runtime/dotest`` →
    ``cbr-ct-orange-book/cbr-aggr-runtime``).

    Args:
        changed_files: List of file paths changed in the PR.

    Returns:
        Set of relative directory paths suitable for git_matrix.py.

    """
    changed_dirs: set[str] = set()
    for path in changed_files:
        # Only care about tests/ or demos/
        if path.startswith(("tests/", "demos/")):
            parts = path.split("/")
            if len(parts) > 2:
                # Keep nested dirs; truncating to the top level would merge distinct test suites.
                changed_dirs.add("/".join(parts[1:-1]))
            elif len(parts) == 2:
                # File directly in tests/ or demos/ (rare)
                changed_dirs.add(parts[1])
    return changed_dirs


def compute_bsl_flag(labels: set[str], inputs_with_bsl) -> str:
    """Compute the effective BSL flag from labels and inputs.with_bsl.

    Returns ``"true"`` if either the ``bsl`` label is present (case-sensitive,
    whole-word match; labels must be pre-lowercased by the caller) or
    ``inputs.with_bsl`` is ``true``. Returns ``"false"`` otherwise.

    Args:
        labels: Set of lowercased PR label strings.
        inputs_with_bsl: Value of the ``with_bsl`` workflow input (string or None).

    Returns:
        ``"true"`` or ``"false"``.

    """
    has_bsl_label = "bsl" in labels
    input_is_true = str(inputs_with_bsl).strip().lower() == "true"
    return "true" if (has_bsl_label or input_is_true) else "false"


def main() -> None:
    pr_number = get_env("PR_NUMBER")
    tests_to_run_input = get_env("TESTS_TO_RUN")
    pr_labels = (get_env("PR_LABELS") or "").split(",")
    inputs_with_bsl = get_env("INPUTS_WITH_BSL")

    labels = {l.strip().lower() for l in pr_labels if l.strip()}

    # Priority 1: Manual override from workflow_dispatch
    if tests_to_run_input:
        tests_to_run = tests_to_run_input

    # Priority 2: BSL Label
    elif "bsl" in labels:
        # Find directories under tests/ that end in .bsl
        tests_path = Path("tests")
        if tests_path.exists() and tests_path.is_dir():
            bsl_dirs = sorted(
                str(d.relative_to(tests_path))
                for d in tests_path.rglob("*.bsl")
                if d.is_dir()
            )
            tests_to_run = " ".join(bsl_dirs)
        else:
            tests_to_run = ""

    # Priority 3: test-mods Label
    elif "test-mods" in labels:
        if not pr_number:
            tests_to_run = ""
        else:
            # Get changed files using gh CLI
            changed_files = get_changed_files(pr_number)

            changed_dirs = process_changed_files(changed_files)

            tests_to_run = " ".join(sorted(changed_dirs))

    # Priority 4: quick-test or CI/CD labels
    elif "quick-test" in labels or "ci/cd" in labels:
        # Hardcoded subset for quick validation
        tests_to_run = "bping cfdpv1 ams-sana ltp-cancel-ack-regression tcpcl-dos bibect conformance_test"

    # Priority 5: Default (all tests)
    else:
        tests_to_run = ""

    with_bsl_effective = compute_bsl_flag(labels, inputs_with_bsl)

    # Retest logic
    enable_retest = "1" if "retest" in labels else "0"

    # Output for GITHUB_OUTPUT
    print(f"tests_to_run={tests_to_run}")
    print(f"with_bsl_effective={with_bsl_effective}")
    print(f"enable_retest_value=ENABLE_RETEST={enable_retest}")


if __name__ == "__main__":
    main()
