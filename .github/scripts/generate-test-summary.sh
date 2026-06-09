#!/usr/bin/env bash
# Usage: generate-test-summary.sh --platform <arc|solaris> [--artifact-pattern <pattern>]
# Parses test results to generate a GitHub Actions step summary and determine overall test status.

set -euo pipefail

# Parse command line arguments
PLATFORM="solaris"  # default for backward compatibility
ARTIFACT_PATTERN="all-artifacts/test-results-*"

while [[ $# -gt 0 ]]; do
    case $1 in
        --platform)
            PLATFORM="$2"
            shift 2
            ;;
        --artifact-pattern)
            ARTIFACT_PATTERN="$2"
            shift 2
            ;;
        *)
            echo "ERROR: Unknown argument $1" >&2
            echo "Usage: $0 --platform <arc|solaris> [--artifact-pattern <pattern>]" >&2
            exit 1
            ;;
    esac
done

# Validate platform
if [[ "$PLATFORM" != "arc" && "$PLATFORM" != "solaris" ]]; then
    echo "ERROR: Invalid platform '$PLATFORM'. Must be 'arc' or 'solaris'." >&2
    exit 1
fi

echo "Generating test summary for platform: $PLATFORM"

# Fallbacks for local testing (if running outside of GitHub Actions)
if [ -z "${GITHUB_STEP_SUMMARY:-}" ]; then
    GITHUB_STEP_SUMMARY="/dev/stdout"
fi
if [ -z "${GITHUB_OUTPUT:-}" ]; then
    GITHUB_OUTPUT="/dev/stdout"
fi
if [ -z "${GITHUB_SERVER_URL:-}" ]; then
    GITHUB_SERVER_URL="https://github.com"
fi
if [ -z "${GITHUB_REPOSITORY:-}" ]; then
    GITHUB_REPOSITORY="unknown/repo"
fi
if [ -z "${GITHUB_RUN_ID:-}" ]; then
    GITHUB_RUN_ID="0"
fi

# Build URL back to workflow run
WORKFLOW_URL="${GITHUB_SERVER_URL}/${GITHUB_REPOSITORY}/actions/runs/${GITHUB_RUN_ID}"

# Initialize based on platform
if [[ "$PLATFORM" == "solaris" ]]; then
    {
        echo "## 📊 Solaris Test Overview"
        echo "| Batch | Result | Failed Tests | Skipped Tests |"
        echo "|-------|--------|--------------|---------------|"
    } >> "$GITHUB_STEP_SUMMARY"
else
    {
        echo "## 📊 Arc Test Overview"
        echo "| Runner | Batch | Result | Failed Tests | Skipped Tests |"
        echo "|--------|-------|--------|--------------|---------------|"
    } >> "$GITHUB_STEP_SUMMARY"
fi

OVERALL_STATUS="success"
FOUND_ANY_RESULTS=false

# For arc: collect per-runner status
declare -A runner_status  # runner -> success/failure
declare -A runner_failed  # runner -> comma-separated failed tests

# Loop through all artifact directories
for dir in ${ARTIFACT_PATTERN}; do
    if [ -d "$dir" ]; then
        FOUND_ANY_RESULTS=true

        if [[ "$PLATFORM" == "solaris" ]]; then
            # Solaris: extract batch number (1-indexed)
            BATCH=$(basename "$dir" | sed 's/.*-batch-//')
            PROGRESS_FILE="$dir/progress-batch-${BATCH}.txt"

            if [ -f "$PROGRESS_FILE" ]; then
                # Extract Failed Tests list
                FAILED=$(awk '/^failed: /{flag=1; next} /^$/{flag=0} flag {print $1}' "$PROGRESS_FILE" | tr -d '\r' | xargs | sed 's/ /, /g')

                # Extract Skipped Tests list
                SKIPPED=$(awk '/^skipped: /{flag=1; next} /^$/{flag=0} flag {print $1}' "$PROGRESS_FILE" | tr -d '\r' | xargs | sed 's/ /, /g')

                # Determine row result and update overall status if failure found
                if [ -n "$FAILED" ]; then
                    RESULT="❌ failed"
                    OVERALL_STATUS="failure"
                else
                    RESULT="✅ success"
                    FAILED="-"
                fi

                [ -z "$SKIPPED" ] && SKIPPED="-"
            else
                # Missing progress file indicates a batch-level failure (e.g., timeout or crash)
                FAILED="-"
                SKIPPED="-"
                RESULT="❌ no results"
                OVERALL_STATUS="failure"
            fi

            # Append row to table
            echo "| Batch $BATCH | $RESULT | $FAILED | $SKIPPED |" >> "$GITHUB_STEP_SUMMARY"

        else
            # Arc: extract runner and batch from artifact name
            # Expected pattern: test-results-arc-runner-set-{os_name}-{timestamp}-batch-{N}
            # where {timestamp} is YYYYMMDD-HHMMSS and {os_name} is u22, u24, ol8, etc.
            BASENAME=$(basename "$dir")

            # Extract runner OS name (strip "arc-runner-set-" prefix to get just the OS name)
            RUNNER=$(echo "$BASENAME" | sed -E 's/test-results-arc-runner-set-(.+)-[0-9]{8}-[0-9]{6}-batch-.*/\1/')

            # Extract batch number (after "batch-")
            BATCH=$(echo "$BASENAME" | sed -E 's/.*-batch-([0-9]+)/\1/')

            # Find progress file (may have different naming patterns)
            # Arc uses "progress" (no extension), Solaris uses "progress-batch-N.txt"
            PROGRESS_FILE=$(find "$dir" -name "progress*" -type f | head -n 1)

            if [ -n "$PROGRESS_FILE" ] && [ -f "$PROGRESS_FILE" ]; then
                # Extract Failed Tests list
                FAILED=$(awk '/^failed: /{flag=1; next} /^$/{flag=0} flag {print $1}' "$PROGRESS_FILE" | tr -d '\r' | xargs | sed 's/ /, /g')

                # Extract Skipped Tests list
                SKIPPED=$(awk '/^skipped: /{flag=1; next} /^$/{flag=0} flag {print $1}' "$PROGRESS_FILE" | tr -d '\r' | xargs | sed 's/ /, /g')

                # Determine row result
                if [ -n "$FAILED" ]; then
                    RESULT="❌ failed"
                    runner_status[$RUNNER]="failure"
                    # Accumulate failed tests for this runner
                    if [ -n "${runner_failed[$RUNNER]:-}" ]; then
                        runner_failed[$RUNNER]="${runner_failed[$RUNNER]}, ${FAILED}"
                    else
                        runner_failed[$RUNNER]="$FAILED"
                    fi
                    OVERALL_STATUS="failure"
                else
                    RESULT="✅ success"
                    # Only set to success if not already failed
                    [ -z "${runner_status[$RUNNER]:-}" ] && runner_status[$RUNNER]="success"
                    FAILED="-"
                fi

                [ -z "$SKIPPED" ] && SKIPPED="-"
            else
                # Missing progress file
                runner_status[$RUNNER]="failure"
                FAILED="-"
                SKIPPED="-"
                RESULT="❌ no results"
                OVERALL_STATUS="failure"
            fi

            # Append row to table
            echo "| $RUNNER | batch-$BATCH | $RESULT | $FAILED | $SKIPPED |" >> "$GITHUB_STEP_SUMMARY"
        fi
    fi
done

# Handle edge case where no test results were downloaded
if [ "$FOUND_ANY_RESULTS" = "false" ]; then
    echo "ERROR: No test results found - all jobs may have failed"
    OVERALL_STATUS="failure"

    if [[ "$PLATFORM" == "solaris" ]]; then
        echo "| N/A | ❌ no results | - | - |" >> "$GITHUB_STEP_SUMMARY"
    else
        echo "| N/A | N/A | ❌ no results | - | - |" >> "$GITHUB_STEP_SUMMARY"
    fi
fi

# Output status based on platform
if [[ "$PLATFORM" == "solaris" ]]; then
    # Solaris: single overall status
    echo "status=$OVERALL_STATUS" >> "$GITHUB_OUTPUT"

    if [ "$OVERALL_STATUS" = "failure" ]; then
        echo "Tests FAILED"
    else
        echo "All tests PASSED"
    fi

else
    # Arc: per-runner status payload
    # Build JSON array of status objects for set-pr-status action
    STATUS_PAYLOAD="["
    FIRST=true

    for runner in "${!runner_status[@]}"; do
        state="${runner_status[$runner]}"

        # Determine description
        if [ "$state" = "failure" ]; then
            if [ -n "${runner_failed[$runner]:-}" ]; then
                description="Tests failed: ${runner_failed[$runner]}"
            else
                description="Tests failed or no results"
            fi
        else
            description="All tests passed"
        fi

        # Build status context (e.g., "ci/arc-runner-set-u22")
        context="ci/arc-runner-set-${runner}"

        # Add to JSON array
        if [ "$FIRST" = true ]; then
            FIRST=false
        else
            STATUS_PAYLOAD+=","
        fi

        STATUS_PAYLOAD+=$(jq -nc \
            --arg runner "$runner" \
            --arg context "$context" \
            --arg state "$state" \
            --arg description "$description" \
            --arg url "$WORKFLOW_URL" \
            '{runner: $runner, context: $context, state: $state, description: $description, target_url: $url}')
    done

    STATUS_PAYLOAD+="]"

    # Output both overall status and per-runner payload
    {
        echo "status=$OVERALL_STATUS"
        echo "status_payload=$STATUS_PAYLOAD"
    } >> "$GITHUB_OUTPUT"

    if [ "$OVERALL_STATUS" = "failure" ]; then
        echo "Tests FAILED (see per-runner status)"
    else
        echo "All tests PASSED"
    fi
fi
