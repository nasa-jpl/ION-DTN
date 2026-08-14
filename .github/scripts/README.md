# Workflow Scripts

## [build-runner-matrix.sh](build-runner-matrix.sh)

**Purpose:** Builds a JSON array of enabled Arc runners
for GitHub Actions matrix strategy.

**Usage:**

```bash
build-runner-matrix.sh --ubuntu_22=<bool> \
  --ubuntu_24=<bool> \
  --oracle_linux_8=<bool> \
  --oracle_linux_9=<bool> \
  --rhel_8=<bool> \
  --rhel_9=<bool>
```

**Parameters:**

- `--ubuntu_22=<bool>` (optional, default: false):
  Include Ubuntu 22.04 runner (u22)
- `--ubuntu_24=<bool>` (optional, default: false):
  Include Ubuntu 24.04 runner (u24)
- `--oracle_linux_8=<bool>` (optional, default: false):
  Include Oracle Linux 8 runner (ol8)
- `--oracle_linux_9=<bool>` (optional, default: false):
  Include Oracle Linux 9 runner (ol9)
- `--rhel_8=<bool>` (optional, default: false):
  Include RHEL 8 runner (rhel8)
- `--rhel_9=<bool>` (optional, default: false):
  Include RHEL 9 runner (rhel9)

**Output (to stdout):**

- JSON array of runner short names (e.g., `["u22","u24","ol9"]`)

**Exit Codes:**

- `0` - Success, JSON array printed to stdout
- `1` - Unknown argument or no runners selected

**Used By:**

- [`.github/workflows/ci-workflow-arc.yml`](../workflows/ci-workflow-arc.yml)
  (setup job, runner matrix generation)

**Example:**

```bash
# Enable Ubuntu 22 and Oracle Linux 9 runners
build-runner-matrix.sh --ubuntu_22=true --oracle_linux_9=true
# Output: ["u22","ol9"]
```

---

## [cleanup-zones.sh](cleanup-zones.sh)

**Purpose:** Manual cleanup script for Solaris zones created by CI.
Removes all zones matching `ci-zone-*` pattern
and their associated ZFS datasets.

**Usage:**

```bash
cleanup-zones.sh
```

**Parameters:**
None. Takes no inputs and cleans ALL ci-zone-* zones unconditionally.

**Behavior:**

1. Connects to Solaris hosts (dsoc3, dsoc4) via SSH
2. Finds all zones matching pattern `ci-zone-*` in any state
   (configured, installed, incomplete)
3. For each zone: halts,
   uninstalls (removes ZFS datasets),
   deletes zone config
4. Cleans up any remaining orphaned ZFS datasets in `rpool/zones/ci-zone-*`
5. Prints summary of remaining zones/datasets per host

**Exit Codes:**

- `0` - Cleanup completed (check summary for any remaining items)

**Used By:**
Manual recovery only. Not called by CI workflows.

**Notes:**

- Requires SSH access to github-runner@dsoc3 and github-runner@dsoc4
- Requires sudo privileges on Solaris hosts
- Uses retry logic for uninstall operations
  (3 attempts with 1s delay)
- Destructive operation - removes zones and data without confirmation
- Use for manual recovery when CI fails to clean up zones properly

---

## [compile-and-buildcheck.sh](compile-and-buildcheck.sh)

**Purpose:** Standard build script that compiles ION
and runs buildcheck validation.

**Usage:**

```bash
compile-and-buildcheck.sh
```

**Parameters:**
None (uses environment variables for configuration)

**Environment Variables:**

- `MAKE_CMD` (optional, default: "make"): Make command to use
  (set to "gmake" for Solaris)
- `EXTRA_CONFIGURE_FLAGS` (optional): Additional flags passed to ./configure
- `ION_RUN_EXPERT` (set to "yes"): Enables expert mode
- `PRESERVE_TEST_LOGS` (set to "1"): Preserves test logs

**Behavior:**

1. Runs `autoreconf -fi` to generate configure script
2. Runs `./configure --enable-crypto-mbedtls --enable-bpsec-debugging $EXTRA_CONFIGURE_FLAGS`
3. Runs `$MAKE_CMD -j$(nproc) all` to compile ION
4. Runs `$MAKE_CMD buildcheck` to validate build integrity

**Exit Codes:**

- `0` - Build and buildcheck completed successfully
- `1` - Build or buildcheck failed

**Used By:**

- [`.github/workflows/ci-workflow-arc.yml`](../workflows/ci-workflow-arc.yml)
  (build job)
- [`.github/workflows/ci-workflow-solaris.yml`](../workflows/ci-workflow-solaris.yml)
  (build job)

**Example:**

```bash
# Standard Linux build
compile-and-buildcheck.sh

# Solaris build with gmake
export MAKE_CMD=gmake
compile-and-buildcheck.sh

# Arc workflow with atomic tier CFLAGS
export EXTRA_CONFIGURE_FLAGS="-std=c99 -DION_TEST_FORCE_FALLBACK"
compile-and-buildcheck.sh
```

---

## [compute-atomic-tiers.sh](compute-atomic-tiers.sh)

**Purpose:** Computes which atomic tiers (native, __atomic,__sync)
to test based on tier filter and platform inputs.

**Usage:**

```bash
compute-atomic-tiers.sh --tier-filter=<filter> --platforms=<platforms>
```

**Parameters:**

- `--tier-filter=<value>` (required): Tier selection
  - `all` - Test all relevant tiers
    (x86: __atomic,__sync; arm: native, __atomic,__sync)
  - `fallbacks` - Test fallback tiers only (both: __atomic,__sync)
  - `native` - Test native tier only (arm only)
  - `__atomic` - Test __atomic tier only
  - `__sync` - Test __sync tier only
- `--platforms=<value>` (required): Platform selection
  - `both` - Test both x86_64 and arm64
  - `x86_64` - Test x86_64 only (zeroes out arm tiers)
  - `arm64` - Test arm64 only (zeroes out x86 tiers)

**Output (to $GITHUB_OUTPUT):**

- `x86_tiers` - JSON array of tiers for x86_64 platform
- `arm_tiers` - JSON array of tiers for arm64 platform

**Exit Codes:**

- `0` - Success, tier lists computed
- `1` - Invalid tier_filter, invalid platforms,
  or combination produces no jobs

**Used By:**

- [`.github/workflows/ci-workflow-atomic-tiers.yml`](../workflows/ci-workflow-atomic-tiers.yml)
  (setup job, tier matrix computation)

**Example:**

```bash
# Compute fallback tiers for both platforms
compute-atomic-tiers.sh --tier-filter=fallbacks --platforms=both
# Output: x86_tiers=["__atomic","__sync"], arm_tiers=["__atomic","__sync"]
```

## [compute-tier-cflags.sh](compute-tier-cflags.sh)

**Purpose:** Maps atomic tier name to corresponding CFLAGS
and configure settings for testing fallback atomics tiers.

**Usage:**

```bash
compute-tier-cflags.sh --tier=<tier>
```

**Parameters:**

- `--tier=<value>` (required): Atomic tier name
  - `native` - Test with native C11 atomics
  - `__atomic` - Test with GCC __atomic builtins
    (C99 mode, forced fallback)
  - `__sync` - Test with legacy __sync builtins
    (C99 mode, forced to Tier 3)

**Output (to $GITHUB_OUTPUT):**

- `cflags` - CFLAGS string for this tier
  (includes VALGRIND, C99, force macros as appropriate)
- `ac_cv_c11` - Whether C11 is available/used
  (`yes` for native, `no` for fallbacks)

**Exit Codes:**

- `0` - Success, CFLAGS computed
- `1` - Unknown tier

**Used By:**

- [`.github/workflows/ci-workflow-atomic-tiers.yml`](../workflows/ci-workflow-atomic-tiers.yml)
  (build job, CFLAGS computation per matrix tier)

**Example:**

```bash
# Compute CFLAGS for __atomic tier
compute-tier-cflags.sh --tier=__atomic
# Output: cflags=-DHAVE_VALGRIND_VALGRIND_H -std=c99 -DION_TEST_FORCE_FALLBACK
#         ac_cv_c11=no
```

**Notes:**
Fallback tiers require both language mode (C99 via -std=c99 and ac_cv_c11=no)
and tier dispatch (ION_TEST_FORCE_FALLBACK macros).
See the coding-guide.md ["Testing the Fallback Tiers"](../../site-docs/docs/coding-guide.md#testing-the-fallback-tiers)
for details.

---

## [generate-test-summary.sh](generate-test-summary.sh)

**Purpose:** Parses test results from multiple jobs
to generate a GitHub Actions step summary table
and determine overall test status.

**Usage:**

```bash
generate-test-summary.sh --platform <arc|solaris> [--artifact-pattern <pattern>]
```

**Parameters:**

- `--platform <value>` (optional, default: "solaris"): Platform type
  - `arc` - Generate summary for Arc workflow (per-runner status with batches)
  - `solaris` - Generate summary for Solaris workflow (per-job status)
- `--artifact-pattern <pattern>` (optional, default: "all-artifacts/test-results-*"):
  Glob pattern for artifact directories

**Output (to $GITHUB_OUTPUT):**

- `status` - Overall test status ("success" or "failure")
- `status_payload` - (Arc only) JSON array of per-runner status objects
  for set-pr-status action

**Output (to $GITHUB_STEP_SUMMARY):**

- Markdown table showing test results per job (Solaris)
  or per runner/batch (Arc)
- Columns: Result,
  Failed Tests,
  Skipped Tests
- Visual indicators: ✅ success,
  ❌ failed,
  ❌ no results

**Exit Codes:**

- `0` - Summary generated successfully (check status output for pass/fail)

**Used By:**

- [`.github/workflows/ci-workflow-arc.yml`](../workflows/ci-workflow-arc.yml)
  (aggregate-results job)
- [`.github/workflows/ci-workflow-solaris.yml`](../workflows/ci-workflow-solaris.yml)
  (aggregate-results job)

**Example:**

```bash
# Generate Arc workflow summary
generate-test-summary.sh --platform arc \
  --artifact-pattern "all-artifacts/test-results-*"

# Generate Solaris workflow summary (default)
generate-test-summary.sh --platform solaris
```

**Status Payload Format (Arc only):**

```json
[
  {
    "runner": "u22",
    "context": "ci/arc-runner-set-u22",
    "state": "success",
    "description": "All tests passed",
    "target_url": "https://github.com/owner/repo/actions/runs/123"
  },
  {
    "runner": "ol9",
    "context": "ci/arc-runner-set-ol9",
    "state": "failure",
    "description": "Tests failed: test1, test2",
    "target_url": "https://github.com/owner/repo/actions/runs/123"
  }
]
```

**Notes:**

- Parses progress files from test artifact directories
- Sets overall status to "failure" if any test fails or no results found
- Arc platform generates per-runner status for fine-grained PR status checks
- Solaris platform generates single overall status
- Falls back gracefully when run outside GitHub Actions (for local testing)

---

## [rtems-verify-qemu-output.sh](rtems-verify-qemu-output.sh)

**Purpose:** Verifies RTEMS QEMU test output contains
expected ION operational indicators.

**Usage:**

```bash
rtems-verify-qemu-output.sh --qemu-output=<file> --report-output=<file>
```

**Parameters:**

- `--qemu-output=<file>` (required): Path to QEMU output file to verify
- `--report-output=<file>` (required): Path to write verification report

**Verification Checks:**

1. Bundle payload delivered
2. IPN forwarder daemon (ipnfw) running
3. UDP services (udplso, udplsi) running
4. LTP segment transmission (popped=1)
5. LTP segment reception (count=1)
6. LTP session completion
7. Absence of daemon spawn errors
8. System clock initialized
9. Bundle transmission statistics (1 bundle, 13 bytes)
10. Bundle reception statistics (1 bundle, 13 bytes)

**Output:**

- Writes detailed verification report to specified output file
- Prints summary to stdout
- Each check logged as [PASS] or [FAIL]

**Exit Codes:**

- `0` - All verification checks passed
- `1` - One or more checks failed, or missing input file

**Used By:**

- [`.github/workflows/ci-rtems61-aarch64-libbsd.yml`](../workflows/ci-rtems61-aarch64-libbsd.yml)
  (verify step, QEMU output verification)

**Example:**

```bash
# Verify QEMU output
rtems-verify-qemu-output.sh \
  --qemu-output=rtems-test-output/qemu-output.txt \
  --report-output=rtems-test-output/verification-report.txt
# Exit 0 if all checks pass, 1 if any fail
```
