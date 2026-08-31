# GitHub Actions

This directory contains composite actions used by GitHub Actions workflows
for ION testing.

## Actions

### generate-test-matrix

**Purpose:** Generates a JSON matrix for parallel test execution,
distributing tests across runners/batches.

**Location:** [`.github/actions/generate-test-matrix/action.yml`](generate-test-matrix/action.yml)

**Inputs:**

- `tests_to_run` (optional): Space-separated list of specific tests to run.
  Leave blank for all tests.
- `num_runners` (required): Number of runners/batches to split tests into.
- `is_solaris` (optional, default: "false"): Set to "true"
  to include Solaris VM hostnames (dsoc3/dsoc4) in output.

**Outputs:**

- `matrix`: JSON array of test batch objects
- `count`: Total number of batches generated

**Output Format (Arc):**

```json
[
  {"batch": 1, "tests": "test1 test2", "job_index": 0},
  {"batch": 2, "tests": "test3 test4", "job_index": 1}
]
```

**Output Format (Solaris):**

```json
[
  {
    "batch": 1,
    "tests": "test1 test2",
    "job_index": 0,
    "vm_hostname": "dsoc3",
    "vm_index": "1"
  }
]
```

**Example:**

```yaml
- name: Generate test matrix
  id: matrix
  uses: ./.github/actions/generate-test-matrix
  with:
    num_runners: 4
    is_solaris: "false"

- name: Use matrix
  strategy:
    matrix:
      test_batch: ${{ fromJSON(steps.matrix.outputs.matrix) }}
```

**Used by:**

- [`.github/workflows/ci-workflow-arc.yml`](../workflows/ci-workflow-arc.yml)
  (setup job)
- [`.github/workflows/ci-workflow-solaris.yml`](../workflows/ci-workflow-solaris.yml)
  (generate-matrix job)

**Implementation:** Uses [`git_matrix.py`](../../.github/scripts/git_matrix.py) script
to distribute tests based on git history.

---

### set-pr-status

**Purpose:** Sets commit statuses on a PR for one or multiple contexts.

**Location:** [`.github/actions/set-pr-status/action.yml`](set-pr-status/action.yml)

**Inputs:**

- `context` (optional): Status context name (e.g., "ci/arc-runner-set-u22").
  Use this OR status_payload, not both.
- `state` (optional): Status state (success, failure, error, pending).
  Required when context is provided.
- `status_payload` (optional): JSON mapping of contexts to states.
  Use this OR context/state, not both.
- `pr_number` (optional): PR number.
  If omitted, auto-detects from branch.

**Status Payload Format:**

```json
{
  "ci/arc-runner-set-u22": "success",
  "ci/arc-runner-set-ol8": "failure"
}
```

**Example (single status):**

```yaml
- name: Set PR status
  uses: ./.github/actions/set-pr-status
  with:
    pr_number: ${{ github.event.pull_request.number }}
    context: "ci/solaris"
    state: "success"
```

**Example (multiple statuses):**

```yaml
- name: Set PR statuses
  uses: ./.github/actions/set-pr-status
  with:
    pr_number: ${{ github.event.pull_request.number }}
    status_payload: ${{ steps.summary.outputs.status_payload }}
```

**Behavior:**

- Auto-detects PR if `pr_number` not provided
- Skips gracefully if no PR found (not an error)
- Sets commit status via GitHub API for each context

**Used by:**

- [`.github/workflows/ci-workflow-arc.yml`](../workflows/ci-workflow-arc.yml)
  (aggregate-results job)
- [`.github/workflows/ci-workflow-solaris.yml`](../workflows/ci-workflow-solaris.yml)
  (aggregate-results job)

---

### setup-solaris-ssh

**Purpose:** Configures SSH access to Solaris VMs (dsoc3/dsoc4).

**Location:** [`.github/actions/setup-solaris-ssh/action.yml`](setup-solaris-ssh/action.yml)

**Note:** Solaris-specific, not modified by arc workflow modernization.
See action file for documentation.

**Used by:** [`.github/workflows/ci-workflow-solaris.yml`](../workflows/ci-workflow-solaris.yml)
(multiple jobs)

---

## Using Actions

All actions in this directory are composite actions
using `using: "composite"` in their action.yml.

**Standard usage pattern:**

```yaml
- name: Action Name
  uses: ./.github/actions/action-name
  with:
    input1: value1
    input2: value2
```

**Outputs usage:**

```yaml
- name: Action Name
  id: action-step
  uses: ./.github/actions/action-name
  with:
    input: value

- name: Use output
  run: echo "${{ steps.action-step.outputs.output_name }}"
```

## Testing Actions Locally

Actions can be tested by creating a minimal workflow
that calls them with test inputs.

## Common Patterns

**Composite actions:** All actions use composite format for inline execution.

**GitHub API:** Actions use `actions/github-script` for GitHub API access.

**Error handling:** Actions use `continue-on-error: true` where appropriate
to allow workflow continuation.
