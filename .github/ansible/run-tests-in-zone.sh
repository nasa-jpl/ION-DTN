#!/bin/env bash
# Usage: ./run-tests-in-zone.sh <run_id> <job_index> <test_list> [env_vars]

# Exit immediately if a command exits with a non-zero status
set -e

# Validate required arguments
if [ $# -lt 3 ]; then
    echo "Error: Missing required arguments" >&2
    echo "" >&2
    echo "Usage: $0 <run_id> <job_index> <test_list> [env_vars]" >&2
    echo "" >&2
    echo "Arguments:" >&2
    echo "  run_id      GitHub Actions run ID (required)" >&2
    echo "  job_index   Matrix job index (required)" >&2
    echo "  test_list   Space-separated list of tests to run (required)" >&2
    echo "  env_vars    Environment variables as KEY=VALUE pairs (optional)" >&2
    echo "" >&2
    echo "Example:" >&2
    echo "  $0 123456789 0 'bping ltp-green' 'DEBUG=1 VERBOSE=true'" >&2
    exit 1
fi

RUN_ID=$1
JOB_INDEX=$2
TEST_LIST=$3
ENV_VARS=$4

# Validate individual parameters
if [ -z "$RUN_ID" ]; then
    echo "Error: run_id cannot be empty" >&2
    exit 1
fi

if [ -z "$JOB_INDEX" ]; then
    echo "Error: job_index cannot be empty" >&2
    exit 1
fi

if [ -z "$TEST_LIST" ]; then
    echo "Error: test_list cannot be empty" >&2
    exit 1
fi

# Validate run_id is numeric
if ! [[ "$RUN_ID" =~ ^[0-9]+$ ]]; then
    echo "Error: run_id must be numeric (got: '$RUN_ID')" >&2
    exit 1
fi

# Validate job_index is numeric
if ! [[ "$JOB_INDEX" =~ ^[0-9]+$ ]]; then
    echo "Error: job_index must be numeric (got: '$JOB_INDEX')" >&2
    exit 1
fi

ZONE_NAME="ci-zone-${RUN_ID}-${JOB_INDEX}"
# Use explicit path since this script runs with sudo (HOME=/root)
ARTIFACT_TARBALL="/home/github-runner/ci-artifacts/${RUN_ID}/ion-build-${RUN_ID}.tar.gz"
RESULTS_DIR="/home/github-runner/ci-results/${RUN_ID}-${JOB_INDEX}"

echo "Gathering zone information for $ZONE_NAME..."
# Dynamically get the zone's root path using zoneadm
IFS=: read _ _ _ ZONE_PATH _ <<< "$(zoneadm -z "$ZONE_NAME" list -p)"

if [ -z "$ZONE_PATH" ]; then
    echo "Error: Could not determine zone path for $ZONE_NAME"
    exit 1
fi

echo "Deploying artifact to zone..."
cp "$ARTIFACT_TARBALL" "${ZONE_PATH}/root/tmp/ion-build.tar.gz"

echo "Extracting and installing ION in zone..."
zlogin "$ZONE_NAME" "export PATH=/usr/local/bin:/usr/bin:/usr/sbin:\$PATH && \
  mkdir -p /root/ion-build && \
  gtar -xzf /tmp/ion-build.tar.gz -C /root/ion-build && \
  cd /root/ion-build && \
  find . -name '*.la' -exec gsed -i 's|/home/github-runner/[^/]*/ion-ios-dev|/root/ion-build|g' {} + && \
  gsed -i 's|/home/github-runner/[^/]*/ion-ios-dev|/root/ion-build|g' libtool && \
  find . -name '*.pc' -exec gsed -i 's|/home/github-runner/[^/]*/ion-ios-dev|/root/ion-build|g' {} + && \
  gmake install"

echo "Verifying zone hostname..."
ZONE_HOSTNAME=$(zlogin "$ZONE_NAME" hostname)
echo "=== Zone Hostname Check ==="
echo "Expected zone: $ZONE_NAME"
echo "Actual hostname: $ZONE_HOSTNAME"
if [ "$ZONE_HOSTNAME" = "unknown" ]; then
    echo "WARNING: Zone hostname is 'unknown'!"
else
    echo "Zone hostname is correctly set."
fi
echo "============================"

echo "Running tests in zone..."
# Turn OFF exit-on-error temporarily so we can capture the test failure code
set +e

# Validate and prepare environment variables (prevent command injection)
ENV_EXPORT_CMD=""
if [ -n "$ENV_VARS" ]; then
    echo "Processing environment variables: $ENV_VARS"

    # Security check: reject dangerous shell metacharacters
    if echo "$ENV_VARS" | grep -qE '[;|&$()<>`\\]'; then
        echo "Error: ENV_VARS contains invalid/dangerous characters (;|&\$()<>\`\\)"
        echo "Only KEY=VALUE pairs with alphanumeric values are allowed"
        exit 1
    fi

    # Validate format: KEY=VALUE pairs separated by spaces
    # Keys must start with letter/underscore, contain only alphanumeric/underscore
    if ! echo "$ENV_VARS" | grep -qE '^[A-Za-z_][A-Za-z0-9_]*=[^ ]+([ ]+[A-Za-z_][A-Za-z0-9_]*=[^ ]+)*$'; then
        echo "Error: ENV_VARS must be space-separated KEY=VALUE pairs"
        echo "Keys must start with letter/underscore and contain only alphanumeric/underscore"
        echo "Example: FOO=bar BAZ=qux"
        exit 1
    fi

    # Build export commands safely - each variable is exported individually
    for var in $ENV_VARS; do
        ENV_EXPORT_CMD="${ENV_EXPORT_CMD}export ${var} && "
    done
fi

# Execute tests with PATH explicitly set for the Python venv and ION binaries
zlogin "$ZONE_NAME" "export PATH=/root/ion_dev/bin:/usr/local/bin:/usr/bin:/usr/sbin:\$PATH && \
  export VIRTUAL_ENV=/root/ion_dev && \
  cd /root/ion-build/tests && \
  ${ENV_EXPORT_CMD}./runtests $TEST_LIST"
TEST_EXIT_CODE=$?

# Turn exit-on-error back ON for the cleanup phase
set -e

echo "Collecting results to $RESULTS_DIR..."
mkdir -p "$RESULTS_DIR"

# Collect progress file
if [ -f "${ZONE_PATH}/root/root/ion-build/tests/progress" ]; then
    cp "${ZONE_PATH}/root/root/ion-build/tests/progress" "${RESULTS_DIR}/progress-job-${JOB_INDEX}.txt"
fi

# Collect ION log files (preserving directory structure)
LOGS_DIR="${RESULTS_DIR}/ion-logs-job-${JOB_INDEX}"
mkdir -p "$LOGS_DIR"

# Find all ion.log files from the entire ion-build directory
find "${ZONE_PATH}/root/root/ion-build" -name "ion.log" -type f 2>/dev/null | while read -r log_file; do
    rel_path=${log_file//"${ZONE_PATH}/root/root/ion-build/"/}
    target_dir="$LOGS_DIR/$(dirname "$rel_path")"
    mkdir -p "$target_dir"
    cp "$log_file" "$target_dir/"
done

# Collect ion-system.log diagnostic files (with flattened naming)
DIAGNOSTICS_DIR="${RESULTS_DIR}/diagnostics-job-${JOB_INDEX}"
mkdir -p "$DIAGNOSTICS_DIR"

# Find ion-system.log files in entire directory
find "${ZONE_PATH}/root/root/ion-build" -name "ion-system.log" -type f 2>/dev/null | while read -r log_file; do
    rel_path=${log_file//"${ZONE_PATH}/root/root/ion-build/"/}
    rel_path="${rel_path%/ion-system.log}"
    unique_name=$(echo "$rel_path" | tr '/' '-')
    cp "$log_file" "$DIAGNOSTICS_DIR/${unique_name}-ion-system.log"
done

echo "Test phase complete. Exit code: $TEST_EXIT_CODE"
exit $TEST_EXIT_CODE
