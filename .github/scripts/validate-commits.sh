#!/bin/sh

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <COMMIT_LIST>"
    exit 1
fi

COMMIT_LIST="$1"

# Default to 'make' if MAKE_CMD is not set (Linux), but allow override (Solaris)
MAKE_CMD=${MAKE_CMD:-make}

echo "COMMITS: $COMMIT_LIST"
echo "Make Command: $MAKE_CMD"

mkdir -p commit-logs
REPORT_FILE="commit-logs/validation-report.md"
OSNAME=$(uname -o)

{
    echo "### ${OSNAME} Commit Validation Report"
    echo "| Commit SHA | Configuration | Build | Quick-Test |"
    echo "|------------|---------------|-------|------------|"
} >"$REPORT_FILE"

# handle base..head or base ^head
COMMITS=$(printf "%s" "$COMMIT_LIST" | xargs git rev-list --reverse)
for COMMIT in $COMMITS; do
    # Handle potential empty lines
    [ -z "$COMMIT" ] && continue

    echo "=================================================="
    echo "Validating commit $COMMIT"
    echo "=================================================="

    # Hard reset and clean to ensure no lingering artifacts break the next build
    git reset --hard "$COMMIT"
    git clean -fdx -e commit-logs/

    if ! { autoreconf -fi && ./configure; } >"commit-logs/${COMMIT}-config.log" 2>&1; then
        echo "| \`$COMMIT\` | ❌ Fail | ⚠️ Skipped | ⚠️ Skipped |" >>"$REPORT_FILE"
        exit 1
    fi

    echo "Running $MAKE_CMD..."
    {
        $MAKE_CMD -j"$(nproc)" 2>&1
        echo "$?" >"commit-logs/${COMMIT}-make.status"
    } | tee "commit-logs/${COMMIT}-make.log"

    read -r BUILD_STATUS <"commit-logs/${COMMIT}-make.status"
    if [ "$BUILD_STATUS" -ne 0 ]; then
        echo "Build failed on commit $COMMIT" >&2
        echo "| \`$COMMIT\` | ✅ Pass | ❌ Fail | ⚠️ Skipped |" >>"$REPORT_FILE"
        exit 1
    fi

    echo "Running $MAKE_CMD test..."
    {
        $MAKE_CMD test 2>&1
        echo "$?" >"commit-logs/${COMMIT}-test.status"
    } | tee "commit-logs/${COMMIT}-quick-test.log"

    read -r TEST_STATUS <"commit-logs/${COMMIT}-test.status"
    if [ "$TEST_STATUS" -ne 0 ]; then
        echo "Quick-test failed on commit $COMMIT" >&2
        echo "| \`$COMMIT\` | ✅ Pass | ✅ Pass | ❌ Fail |" >>"$REPORT_FILE"
        exit 1
    fi

    echo "| \`$COMMIT\` | ✅ Pass | ✅ Pass | ✅ Pass |" >>"$REPORT_FILE"

done

echo "All commits validated successfully."
exit 0
