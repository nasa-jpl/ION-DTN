# Pull Request Testing & Label Guide

This document explains how pull request labels control test selection and execution in the [`ci-run-PR-tests`](ci-run-PR-tests.yml) GitHub Actions workflow.

## Workflow Logic & Priority

1. Skipping Tests
   * Draft Status: If a pull request is marked as a draft, test runs are automatically skipped regardless of labels.
   * `no-test` Override: Applying the `no-test` label forces `needs_tests` to `false`.
   * Bypassing Status Checks: When tests are skipped (via draft state or `no-test`), the `skip-tests` job sends `success` status checks to GitHub API for required check contexts (`ci/solaris`, `ci/arc-runner-set-*`) so branch protection rules pass.

2. Test Subset Determination
    * `workflow_dispatch` Input: Manual triggers override label-based filtering and execute the exact space-separated test string provided in `tests_to_run`.
    * `test-mods` Processing: Queries the GitHub API for PR file changes, isolates path directories under `tests/` or `demos/`, and runs only those detected subfolders.
    * `quick-test` / `CI/CD` Processing: Sets the test execution parameter to the fixed list of core tests.
    * Full Suite: If no specific test-selection label is present, `tests_to_run` remains empty, triggering the full test suite in downstream workflows.

---

## Automatic Label Assignment

The workflow runs `actions/labeler` on PR updates to apply labels based on file modification rules:

* **`no-test`**: Automatically assigned if *all* modified files match non-code paths such as markdown files (`**/*.md`, `**/*.pod`, `**/README*`), site documentation (`site-docs/**`), or infrastructure directories (`charts/**`, `images/**`).

* **`test-mods`**: Automatically assigned if *all* modified files reside inside `tests/**` or `demos/**`.

* **`CI/CD`**: Automatically assigned if any changed file resides in `.github/**`.

* **Component Labels**: Subsystem labels (e.g., `AMS`, `BIBE`, `BPSec`, `BSL`, `CFDP`, `IMC`, `LTP`, `nm/adm/amp`) are automatically assigned based on matching directory paths.

---

## Test Controlling Labels

| Label | Purpose / Action | Impact on Test Suite |
| --- | --- | --- |
| `no-test` | Forces tests to skip completely. Marks required status checks as passed. | No tests are executed. |
| `test-mods` | Restricts test execution to modified directories under `tests/` or `demos/`. | Dynamically selects modified test/demo folders. |
| `quick-test` | Restricts execution to a pre-defined set of key integration tests. | Runs `bping`, `cfdpv1`, `ams-sana`, `ltp-cancel-ack-regression`, `tcpcl-dos`, `bibect`, and `conformance_test`. |
| `CI/CD` | Triggers the same quick-test suite when workflow files change. | Runs `bping`, `cfdpv1`, `ams-sana`, `ltp-cancel-ack-regression`, `tcpcl-dos`, `bibect`, and `conformance_test`. |
| `retest` | Passes `ENABLE_RETEST=1` as an environment variable to runner workflows. | Inherits test selection, enables retesting behavior. |
| *(None)* | Default fallback behavior when no selection labels are applied. | Runs the entire test suite. |

---

## Label Co-Occurrence Matrix

| Controlling Label | Co-Occurring Label(s) | Trigger / Scenario |
| --- | --- | --- |
| `no-test` | `documentation` | Automatically applied together when changed files are strictly limited to `site-docs/**` or Markdown/POD documentation. |
| `no-test` | `Infrastructure` | Automatically applied together when changed files are strictly limited to `charts/**` or `images/**`. |
| `no-test` | `CI/CD` | Automatically applied together if modified files are exclusively Markdown/documentation files within the `.github/**` directory. |
| `CI/CD` | Subsystem Labels (`AMS`, `CFDP`, `LTP`, etc.) | Automatically assigned together when `.github/**` workflow files and subsystem code are modified in the same PR. |
| `retest` | `quick-test` / `test-mods` / Component Labels | Manually assigned alongside any test execution or subsystem label to force environment flag `ENABLE_RETEST=1`. |
