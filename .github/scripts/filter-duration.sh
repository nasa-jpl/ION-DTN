#!/bin/sh
# Usage: filter-duration-files.sh [--threshold <seconds>] [--output-dir <dir>]
# Filters .DURATION files modified in the current batch and stages them
# only if they changed by more than the specified threshold.

set -eu

# Default variables
THRESHOLD=5
OUTPUT_DIR="duration-staging"

# Parse command line arguments
while [ $# -gt 0 ]; do
    case $1 in
        --threshold)
            THRESHOLD="$2"
            shift 2
            ;;
        --output-dir)
            OUTPUT_DIR="$2"
            shift 2
            ;;
        *)
            echo "ERROR: Unknown argument $1" >&2
            echo "Usage: $0 [--threshold <seconds>] [--output-dir <dir>]" >&2
            exit 1
            ;;
    esac
done

echo "Filtering .DURATION files with a threshold of > ${THRESHOLD}s"
echo "Staging directory: $OUTPUT_DIR"

mkdir -p "$OUTPUT_DIR"

MODIFIED_FILES=$(git diff --name-only | grep '\.DURATION$' || true)

if [ -n "$MODIFIED_FILES" ]; then
    echo "Checking modified files..."
    echo "$MODIFIED_FILES" | while IFS= read -r file; do
        [ -z "$file" ] && continue

        # Get the original value from Git (default to 0 if it doesn't exist)
        old_val=$(git show HEAD:"$file" 2>/dev/null || echo "0")
        new_val=$(cat "$file")

        # Calculate absolute difference
        diff=$(( new_val - old_val ))
        abs_diff=${diff#-}

        if [ "$abs_diff" -gt "$THRESHOLD" ]; then
            echo "  Keeping $file (Changed by ${abs_diff}s)"
            mkdir -p "$OUTPUT_DIR/$(dirname "$file")"
            cp "$file" "$OUTPUT_DIR/$file"
        else
            echo "  Ignoring $file (Changed by ${abs_diff}s - below ${THRESHOLD}s threshold)"
        fi
    done
else
    echo "No modified .DURATION files found."
fi

# Process completely new .DURATION files (untracked by Git)
UNTRACKED_FILES=$(git ls-files --others --exclude-standard | grep '\.DURATION$' || true)

if [ -n "$UNTRACKED_FILES" ]; then
    echo "Checking new/untracked files..."
    echo "$UNTRACKED_FILES" | while IFS= read -r file; do
        [ -z "$file" ] && continue

        echo "  Keeping $file (New file)"
        mkdir -p "$OUTPUT_DIR/$(dirname "$file")"
        cp "$file" "$OUTPUT_DIR/$file"
    done
else
    echo "No new/untracked .DURATION files found."
fi

echo "Filtering complete."
