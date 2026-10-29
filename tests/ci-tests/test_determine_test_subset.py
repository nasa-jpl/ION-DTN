"""
Unit tests for determine_test_subset.py script.

Tests validate the following functionality:
- Parses changed file paths returned by the `gh` CLI into a list
- Nested test directory discovery preserves full relative paths
- test-mods output is space-separated (not comma-separated)
- process_changed_files() is importable and callable with mock data
- compute_bsl_flag() correctly computes BSL flag from labels and inputs
"""

import contextlib
import importlib.util
import os
import subprocess
import sys
import unittest
from io import StringIO
from pathlib import Path
from typing import Any
from unittest.mock import MagicMock, patch

current_file = Path(__file__).resolve()
script_path = (
    current_file.parent.parent.parent
    / ".github"
    / "scripts"
    / "determine_test_subset.py"
)
spec = importlib.util.spec_from_file_location("determine_test_subset", script_path)

if spec is not None and spec.loader is not None:
    determine_test_subset: Any = importlib.util.module_from_spec(spec)
    sys.modules["determine_test_subset"] = determine_test_subset
    spec.loader.exec_module(determine_test_subset)
else:
    raise ImportError(f"Could not load script from {script_path}")


class TestDetermineTestSubset(unittest.TestCase):
    """Test cases for determine_test_subset.py functionality."""

    @patch("subprocess.run")
    def test_get_changed_files_parses_output(self, mock_run):
        """get_changed_files returns one path per line of the gh CLI output."""
        mock_run.return_value.returncode = 0
        mock_run.return_value.stdout = (
            "tests/foo/dotest\ntests/bar/sub/test_b.c\ndemos/baz/run.sh\n"
        )

        result = determine_test_subset.get_changed_files(123)

        self.assertEqual(
            result,
            ["tests/foo/dotest", "tests/bar/sub/test_b.c", "demos/baz/run.sh"],
        )

    @patch("subprocess.run")
    def test_get_changed_files_returns_empty_on_failure(self, mock_run):
        """get_changed_files() returns [] and emits a warning on gh CLI failure.

        When gh CLI raises CalledProcessError, the function must return
        an empty list and emit a diagnostic annotation to stderr.
        """
        mock_run.side_effect = subprocess.CalledProcessError(
            returncode=1, cmd="gh pr diff 123 --name-only"
        )

        stderr_capture = StringIO()
        with contextlib.redirect_stderr(stderr_capture):
            result = determine_test_subset.get_changed_files(123)

        self.assertEqual(result, [])
        self.assertIn("::warning::", stderr_capture.getvalue())

    def test_test_mods_discovers_nested_tests(self):
        """process_changed_files preserves full nested relative paths."""
        changed_files = [
            "tests/cbr-ct-orange-book/cbr-aggr-runtime/dotest",
            "tests/cbr-ct-orange-book/cbr-aggr-runtime/foo.c",
            "tests/ci-tests/test_git_matrix.py",
            "demos/demo1/run.sh",
        ]
        result = determine_test_subset.process_changed_files(changed_files)

        self.assertIn("cbr-ct-orange-book/cbr-aggr-runtime", result)
        self.assertIn("ci-tests", result)
        self.assertIn("demo1", result)
        # Do not truncate to top-level for nested paths
        self.assertNotIn("cbr-ct-orange-book", result)

    def test_output_is_space_separated(self):
        """main() produces space-separated tests_to_run with no commas.

        Uses the test-mods branch: mocks subprocess.run to return test file
        paths, then verifies the output is truly space-separated.
        """
        mock_run = MagicMock()
        mock_run.return_value.returncode = 0
        mock_run.return_value.stdout = "tests/foo/test_a.c\ntests/bar/test_b.c\n"

        with (
            patch.dict(
                os.environ,
                {
                    "PR_LABELS": "test-mods",
                    "PR_NUMBER": "123",
                    "TESTS_TO_RUN": "",
                    "INPUTS_WITH_BSL": "false",
                },
            ),
            patch("subprocess.run", mock_run),
        ):
            output = StringIO()
            with contextlib.redirect_stdout(output):
                determine_test_subset.main()

        output_text = output.getvalue()
        # Extract the tests_to_run line
        tests_to_run_line = next(
            line
            for line in output_text.splitlines()
            if line.startswith("tests_to_run=")
        )
        tests_to_run_value = tests_to_run_line.split("=", 1)[1]

        # Must be space-separated, not comma-separated
        self.assertNotIn(",", tests_to_run_value)
        self.assertIn(" ", tests_to_run_value)

        # Verify with_bsl_effective is also in output
        self.assertIn("with_bsl_effective=false", output_text)

    def test_determine_test_subset_importable(self):
        """determine_test_subset.py is importable and process_changed_files works."""
        self.assertTrue(hasattr(determine_test_subset, "process_changed_files"))
        self.assertTrue(hasattr(determine_test_subset, "get_changed_files"))
        self.assertTrue(hasattr(determine_test_subset, "main"))

        result = determine_test_subset.process_changed_files(
            ["tests/foo/test_a.c", "tests/bar/sub/test_b.c", "demos/baz/run.sh"]
        )
        self.assertIn("foo", result)
        self.assertIn("bar/sub", result)
        self.assertIn("baz", result)

    def test_process_changed_files_filters_non_test_files(self):
        """process_changed_files only includes tests/ and demos/ paths."""
        changed_files = [
            "README.md",
            "ci/setup.sh",
            "tests/foo/dotest",
            "demos/baz/run.sh",
        ]
        result = determine_test_subset.process_changed_files(changed_files)
        self.assertIn("foo", result)
        self.assertIn("baz", result)
        self.assertNotIn("ci", result)
        self.assertNotIn("README", result)

    def test_process_changed_files_empty_input(self):
        """process_changed_files handles empty input gracefully."""
        result = determine_test_subset.process_changed_files([])
        self.assertEqual(result, set())

    # --- compute_bsl_flag tests ---

    def test_bsl_label_forces_with_bsl_true(self):
        """bsl label present forces with_bsl_effective to true."""
        self.assertEqual(
            determine_test_subset.compute_bsl_flag({"bsl", "ci/cd"}, "false"),
            "true",
        )

    def test_no_bsl_label_and_input_false(self):
        """No bsl label and inputs.with_bsl=false returns false."""
        self.assertEqual(
            determine_test_subset.compute_bsl_flag({"ci/cd", "quick-test"}, "false"),
            "false",
        )

    def test_no_bsl_label_and_input_true(self):
        """No bsl label but inputs.with_bsl=true returns true."""
        self.assertEqual(
            determine_test_subset.compute_bsl_flag({"ci/cd"}, "true"),
            "true",
        )

    def test_mixed_case_bsl_label(self):
        """Mixed-case BSL label is normalized through main() to force true.

        compute_bsl_flag() is case-sensitive; it does NOT lowercase
        internally. main() normalizes labels before calling compute_bsl_flag(),
        so a mixed-case "BSL" label should still produce with_bsl_effective=true.
        """
        mock_changed_files = MagicMock(return_value=[])
        with (
            patch.dict(
                os.environ,
                {
                    "PR_LABELS": "BSL, CI/CD",
                    "INPUTS_WITH_BSL": "false",
                    "PR_NUMBER": "123",
                    "TESTS_TO_RUN": "",
                },
            ),
            patch.object(
                determine_test_subset, "get_changed_files", mock_changed_files
            ),
        ):
            output = StringIO()
            with contextlib.redirect_stdout(output):
                determine_test_subset.main()

        self.assertIn("with_bsl_effective=true", output.getvalue())

    def test_bsl_label_not_lowercased_directly(self):
        """compute_bsl_flag does NOT lowercase labels internally.

        Passing {"BSL"} (uppercase) directly must return "false", proving
        the function relies on the caller for normalization. main() is
        responsible for label normalization before calling this function.
        """
        self.assertEqual(
            determine_test_subset.compute_bsl_flag({"BSL"}, "false"),
            "false",
        )

    def test_bsl_tools_label_not_whole_word(self):
        """'bsl-tools' label should NOT match (whole word only)."""
        self.assertEqual(
            determine_test_subset.compute_bsl_flag({"bsl-tools", "ci/cd"}, "false"),
            "false",
        )

    def test_bsl_label_and_no_test_label(self):
        """bsl and no-test together still returns true."""
        self.assertEqual(
            determine_test_subset.compute_bsl_flag({"bsl", "no-test"}, "false"),
            "true",
        )

    def test_empty_labels_and_input_false(self):
        """Empty labels and inputs.with_bsl=false returns false."""
        self.assertEqual(
            determine_test_subset.compute_bsl_flag(set(), "false"),
            "false",
        )

    def test_empty_labels_and_input_true(self):
        """Empty labels but inputs.with_bsl=true returns true."""
        self.assertEqual(
            determine_test_subset.compute_bsl_flag(set(), "true"),
            "true",
        )

    def test_empty_labels_and_input_empty(self):
        """Empty labels and inputs.with_bsl empty string returns false."""
        self.assertEqual(
            determine_test_subset.compute_bsl_flag(set(), ""),
            "false",
        )

    def test_with_bsl_effective_output_true(self):
        """main() emits with_bsl_effective=true when BSL label is set."""
        mock_changed_files = MagicMock(return_value=[])
        with (
            patch.dict(
                os.environ,
                {
                    "PR_LABELS": "bsl",
                    "INPUTS_WITH_BSL": "false",
                    "PR_NUMBER": "",
                    "TESTS_TO_RUN": "",
                },
            ),
            patch.object(
                determine_test_subset, "get_changed_files", mock_changed_files
            ),
        ):
            output = StringIO()
            with contextlib.redirect_stdout(output):
                determine_test_subset.main()

        self.assertIn("with_bsl_effective=true", output.getvalue())

    def test_with_bsl_effective_output_false(self):
        """main() emits with_bsl_effective=false when no BSL label or input."""
        mock_changed_files = MagicMock(return_value=[])
        with (
            patch.dict(
                os.environ,
                {
                    "PR_LABELS": "ci/cd",
                    "INPUTS_WITH_BSL": "false",
                    "PR_NUMBER": "",
                    "TESTS_TO_RUN": "",
                },
            ),
            patch.object(
                determine_test_subset, "get_changed_files", mock_changed_files
            ),
        ):
            output = StringIO()
            with contextlib.redirect_stdout(output):
                determine_test_subset.main()

        self.assertIn("with_bsl_effective=false", output.getvalue())

    def test_bsl_label_only_and_input_empty(self):
        """bsl label only, inputs.with_bsl empty returns true."""
        self.assertEqual(
            determine_test_subset.compute_bsl_flag({"bsl"}, ""),
            "true",
        )

    def test_no_test_label_only_no_bsl(self):
        """no-test label only (no bsl), inputs.with_bsl=false returns false."""
        self.assertEqual(
            determine_test_subset.compute_bsl_flag({"no-test"}, "false"),
            "false",
        )

    def test_bslools_label_not_whole_word(self):
        """'bslools' label should NOT match (not exact 'bsl')."""
        self.assertEqual(
            determine_test_subset.compute_bsl_flag({"bslools"}, "false"),
            "false",
        )

    def test_none_inputs_with_bsl(self):
        """inputs_with_bsl=None (env var not set) works correctly."""
        self.assertEqual(
            determine_test_subset.compute_bsl_flag({"ci/cd"}, None),
            "false",
        )

    def test_compute_bsl_flag_is_pure_function(self):
        """compute_bsl_flag is a pure function callable with mock data."""
        self.assertTrue(hasattr(determine_test_subset, "compute_bsl_flag"))

        result = determine_test_subset.compute_bsl_flag({"bsl"}, "false")
        self.assertIsInstance(result, str)
        self.assertIn(result, ("true", "false"))


if __name__ == "__main__":
    unittest.main()
