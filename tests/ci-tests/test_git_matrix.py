#!/usr/bin/env python3
"""
Unit tests for git_matrix.py script.

Tests validate the following functionality:
- Matrix generation with various runner counts
- Subset test selection
- Load balancing behavior
- JSON output formatting
"""

import importlib.util
import io
import sys
import unittest
from pathlib import Path
from typing import Any
from unittest.mock import patch

current_file = Path(__file__).resolve()
script_path = (
    current_file.parent.parent.parent / ".github" / "scripts" / "git_matrix.py"
)
spec = importlib.util.spec_from_file_location("git_matrix", script_path)

if spec is not None and spec.loader is not None:
    git_matrix: Any = importlib.util.module_from_spec(spec)
    sys.modules["git_matrix"] = git_matrix
    spec.loader.exec_module(git_matrix)
else:
    raise ImportError(f"Could not load script from {script_path}")


class TestGitMatrix(unittest.TestCase):
    """Test cases for git_matrix.py functionality."""

    @patch("git_matrix.list_tests")
    @patch("git_matrix.balance_load")
    @patch("git_matrix.get_folder_durations")
    @patch("json.dumps")
    def test_main_with_default_runners(
        self, mock_json_dumps, mock_get_durations, mock_balance, mock_list_tests
    ):
        """Test main function with default runner count (7)."""
        mock_list_tests.return_value = [
            Path("test1"),
            Path("test2"),
            Path("test3"),
        ]
        mock_get_durations.return_value = [
            {"path": Path("test1"), "duration": 10.0},
            {"path": Path("test2"), "duration": 20.0},
            {"path": Path("test3"), "duration": 15.0},
        ]
        mock_balance.return_value = [
            {
                "id": 1,
                "total_time": 45.0,
                "folders": [Path("test1"), Path("test2"), Path("test3")],
            }
        ]
        mock_json_dumps.return_value = '["test1 test2 test3"]'

        saved_stdout = sys.stdout
        try:
            output = io.StringIO()
            sys.stdout = output
            git_matrix.main()
            mock_list_tests.assert_called_once()
            mock_get_durations.assert_called_once_with(
                [
                    Path("test1"),
                    Path("test2"),
                    Path("test3"),
                ]
            )
            mock_balance.assert_called_once_with(mock_get_durations.return_value, 7)
            mock_json_dumps.assert_called_once()
            self.assertEqual(output.getvalue().strip(), '["test1 test2 test3"]')
        finally:
            sys.stdout = saved_stdout

    @patch("git_matrix.list_tests")
    @patch("git_matrix.balance_load")
    @patch("git_matrix.get_folder_durations")
    @patch("json.dumps")
    def test_main_with_custom_runners(
        self, mock_json_dumps, mock_get_durations, mock_balance, mock_list_tests
    ):
        """Test main function with custom runner count (3)."""
        mock_list_tests.return_value = [
            Path("test1"),
            Path("test2"),
            Path("test3"),
        ]
        mock_get_durations.return_value = [
            {"path": Path("test1"), "duration": 10.0},
            {"path": Path("test2"), "duration": 20.0},
            {"path": Path("test3"), "duration": 15.0},
        ]
        mock_balance.return_value = [
            {"id": 1, "total_time": 20.0, "folders": [Path("test2")]},
            {"id": 2, "total_time": 15.0, "folders": [Path("test3")]},
            {"id": 3, "total_time": 10.0, "folders": [Path("test1")]},
        ]
        mock_json_dumps.return_value = '["test2", "test3", "test1"]'

        saved_stdout = sys.stdout
        try:
            output = io.StringIO()
            sys.stdout = output
            git_matrix.main(runners=3)
            mock_list_tests.assert_called_once()
            mock_get_durations.assert_called_once_with(
                [
                    Path("test1"),
                    Path("test2"),
                    Path("test3"),
                ]
            )
            mock_balance.assert_called_once_with(mock_get_durations.return_value, 3)
            mock_json_dumps.assert_called_once()
            self.assertEqual(output.getvalue().strip(), '["test2", "test3", "test1"]')
        finally:
            sys.stdout = saved_stdout

    @patch("git_matrix.get_folder_durations")
    @patch("json.dumps")
    def test_main_with_subset(self, mock_json_dumps, mock_get_durations):
        """Test main function with subset of tests."""
        subset = [Path("specific_test")]
        mock_get_durations.return_value = [
            {"path": Path("specific_test"), "duration": 30.0},
        ]
        mock_json_dumps.return_value = '["specific_test"]'

        saved_stdout = sys.stdout
        try:
            output = io.StringIO()
            sys.stdout = output
            git_matrix.main(runners=1, subset=subset)
            mock_get_durations.assert_called_once_with(subset)
            mock_json_dumps.assert_called_once()
            self.assertEqual(output.getvalue().strip(), '["specific_test"]')
        finally:
            sys.stdout = saved_stdout

    def test_balance_load(self):
        """Test the load balancing algorithm."""
        tasks: list[dict[str, Any]] = [
            {"path": Path("test1"), "duration": 20.0},
            {"path": Path("test2"), "duration": 15.0},
            {"path": Path("test3"), "duration": 10.0},
            {"path": Path("test4"), "duration": 5.0},
        ]

        runners = git_matrix.balance_load(tasks, 2)
        self.assertEqual(len(runners), 2)

        total_duration = runners[0]["total_time"] + runners[1]["total_time"]
        self.assertEqual(total_duration, 50.0)

        self.assertGreaterEqual(len(runners[0]["folders"]), 1)
        self.assertGreaterEqual(len(runners[1]["folders"]), 1)

        all_tasks = []
        for runner in runners:
            all_tasks.extend(runner["folders"])
        self.assertEqual(len(all_tasks), 4)

    def test_balance_load_with_empty_tasks(self):
        """Test load balancing with empty task list."""
        runners = git_matrix.balance_load([], 3)
        self.assertEqual(len(runners), 3)
        for runner in runners:
            self.assertEqual(runner["total_time"], 0.0)
            self.assertEqual(len(runner["folders"]), 0)

    @patch("git_matrix.find_tests")
    @patch("git_matrix.is_excluded", return_value=False)
    def test_list_tests_handles_demos(self, mock_excluded, mock_find_tests):
        """list_tests() handles demos paths without raising TypeError."""
        mock_find_tests.return_value = [Path("demos/demo1")]

        with (
            patch("os.access", return_value=True),
            patch("pathlib.Path.exists", return_value=True),
        ):
            result = git_matrix.list_tests()
            self.assertEqual(len(result), 1)
            self.assertIsInstance(result[0], Path)
            self.assertEqual(result[0], Path("../demos/demo1"))

    @patch("pathlib.Path.exists")
    @patch("pathlib.Path.is_file")
    @patch("pathlib.Path.read_text")
    def test_get_folder_durations(self, mock_read_text, mock_is_file, mock_exists):
        """Test reading duration files."""
        mock_exists.return_value = True
        mock_is_file.return_value = True
        mock_read_text.side_effect = ["10.5", "20.75", "15.0"]

        folder_list = [Path("test1"), Path("test2"), Path("test3")]
        tasks = git_matrix.get_folder_durations(folder_list)

        self.assertEqual(len(tasks), 3)
        self.assertEqual(tasks[0]["path"], Path("test1"))
        self.assertEqual(tasks[0]["duration"], 10.5)
        self.assertEqual(tasks[1]["path"], Path("test2"))
        self.assertEqual(tasks[1]["duration"], 20.75)
        self.assertEqual(tasks[2]["path"], Path("test3"))
        self.assertEqual(tasks[2]["duration"], 15.0)

    @patch("pathlib.Path.exists", autospec=True)
    @patch("pathlib.Path.is_file", autospec=True)
    @patch("pathlib.Path.read_text", autospec=True)
    def test_get_folder_durations_with_errors(
        self, mock_read_text, mock_is_file, mock_exists
    ):
        """Test handling errors when reading duration files.

        Verifies that missing files are skipped and unparsable duration
        files default to 0.0.
        """
        folder_list = [Path("valid"), Path("missing"), Path("corrupt")]

        def mock_exists_side_effect(path_obj):
            return "missing" not in str(path_obj)

        def mock_is_file_side_effect(path_obj):
            return "missing" not in str(path_obj)

        def mock_read_text_side_effect(path_obj, *args, **kwargs):
            if "corrupt" in str(path_obj):
                return "not-a-number"
            return "10.0"

        mock_exists.side_effect = mock_exists_side_effect
        mock_is_file.side_effect = mock_is_file_side_effect
        mock_read_text.side_effect = mock_read_text_side_effect

        tasks = git_matrix.get_folder_durations(folder_list)

        self.assertEqual(len(tasks), 2)
        self.assertEqual(tasks[0]["path"], Path("valid"))
        self.assertEqual(tasks[0]["duration"], 10.0)
        self.assertEqual(tasks[1]["path"], Path("corrupt"))
        self.assertEqual(tasks[1]["duration"], 0.0)

    @patch("pathlib.Path.exists")
    @patch("pathlib.Path.is_file")
    @patch("pathlib.Path.read_text")
    def test_get_folder_durations_uses_path_prefix(
        self, mock_read_text, mock_is_file, mock_exists
    ):
        """get_folder_durations correctly builds path using Path('tests') prefix."""
        mock_exists.return_value = True
        mock_is_file.return_value = True
        mock_read_text.return_value = "10.0"

        result = git_matrix.get_folder_durations([Path("test1")])

        self.assertEqual(len(result), 1)
        self.assertEqual(result[0]["path"], Path("test1"))
        self.assertEqual(result[0]["duration"], 10.0)

    @patch("json.dumps")
    def test_print_schedule(self, mock_json_dumps):
        """Test JSON output formatting."""
        runners: list[dict[str, Any]] = [
            {
                "id": 1,
                "total_time": 35.0,
                "folders": [Path("tests/test2"), Path("tests/test1")],
            },
            {"id": 2, "total_time": 25.0, "folders": [Path("tests/test3")]},
        ]

        expected_json = '["tests/test1 tests/test2", "tests/test3"]'
        mock_json_dumps.return_value = expected_json

        saved_stdout = sys.stdout
        try:
            output = io.StringIO()
            sys.stdout = output
            git_matrix.print_schedule(runners)
            mock_json_dumps.assert_called_once()
            self.assertEqual(output.getvalue().strip(), expected_json)
        finally:
            sys.stdout = saved_stdout

    def test_no_dead_setUp_tearDown_save_restore(self):
        """Verify obsolete setUp and tearDown methods have been removed.

        Such methods would perform Path save/restore operations that never
        replace the originals with mocks, so they are redundant. Assert they
        are absent.
        """
        self.assertNotIn("setUp", TestGitMatrix.__dict__)
        self.assertNotIn("tearDown", TestGitMatrix.__dict__)

    def test_git_matrix_duration_path_uses_path_prefix(self):
        """Verify duration file paths are built from Path("tests").

        Behavior tests cannot distinguish a string left operand from a Path
        left operand, because Path.__rtruediv__ makes both forms work. This
        check inspects the source text of git_matrix.py for the idiomatic
        form and confirms the non-idiomatic form is absent.
        """
        source_text = script_path.read_text(encoding="utf-8")
        path_construction_line = 'Path("tests") / folder'
        self.assertIn(path_construction_line, source_text)
        self.assertNotIn('"tests" / folder', source_text)


if __name__ == "__main__":
    unittest.main()
