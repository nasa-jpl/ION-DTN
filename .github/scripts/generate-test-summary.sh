#!/bin/bash
# Usage: ./.github/scripts/generate-test-summary.sh
# Parses test results to generate a GitHub Actions step summary and determine overall test status.

echo "Generating test summary..."

# Fallbacks for local testing (if running outside of GitHub Actions)
if [ -z "$GITHUB_STEP_SUMMARY" ]; then
  GITHUB_STEP_SUMMARY="/dev/stdout"
fi
if [ -z "$GITHUB_OUTPUT" ]; then
  GITHUB_OUTPUT="/dev/stdout"
fi

# Initialize the markdown table
echo "## 📊 Solaris Test Overview" >> "$GITHUB_STEP_SUMMARY"
echo "| Job | Result | Failed Tests | Skipped Tests |" >> "$GITHUB_STEP_SUMMARY"
echo "|-----|--------|--------------|---------------|" >> "$GITHUB_STEP_SUMMARY"

OVERALL_STATUS="success"
FOUND_ANY_RESULTS=false

# Loop through all job result directories
for dir in all-artifacts/test-results-*-job-*; do
  if [ -d "$dir" ]; then
    FOUND_ANY_RESULTS=true
    JOB_INDEX=$(basename "$dir" | sed 's/.*-job-//')
    PROGRESS_FILE="$dir/progress-job-${JOB_INDEX}.txt"

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
      # Missing progress file indicates a job-level failure (e.g., timeout or crash)
      FAILED="-"
      SKIPPED="-"
      RESULT="❌ no results"
      OVERALL_STATUS="failure"
    fi

    # Append row to table
    echo "| Job $JOB_INDEX | $RESULT | $FAILED | $SKIPPED |" >> "$GITHUB_STEP_SUMMARY"
  fi
done

# Handle edge case where no test results were downloaded
if [ "$FOUND_ANY_RESULTS" = "false" ]; then
  echo "ERROR: No test results found - all jobs may have failed"
  OVERALL_STATUS="failure"
  echo "| N/A | ❌ no results | - | - |" >> "$GITHUB_STEP_SUMMARY"
fi

# Output the final status for the workflow to use
echo "status=$OVERALL_STATUS" >> "$GITHUB_OUTPUT"

if [ "$OVERALL_STATUS" = "failure" ]; then
  echo "Tests FAILED"
else
  echo "All tests PASSED"
fi
