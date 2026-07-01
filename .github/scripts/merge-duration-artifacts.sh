#!/bin/sh
set -e

# Usage: merge-duration-artifacts.sh <source_dir> <target_dir>
if [ "$#" -ne 2 ]; then
  echo "Usage: $0 <source_dir> <target_dir>" >&2
  exit 1
fi

SOURCE_DIR="$1"
TARGET_DIR="$2"

if [ ! -d "$SOURCE_DIR" ]; then
  echo "Error: Source directory does not exist: $SOURCE_DIR" >&2
  exit 1
fi

if [ ! -d "$TARGET_DIR" ]; then
  echo "Error: Target directory does not exist: $TARGET_DIR" >&2
  exit 1
fi

echo "Merging .DURATION files from $SOURCE_DIR to $TARGET_DIR"

# Track if any files were updated
UPDATED=0

# Create temp file for find results
TMPFILE=$(mktemp)
find "$SOURCE_DIR" -type f -name '.DURATION' > "$TMPFILE"

# Process files (no subshell, uses input redirection)
while read -r src_file; do
  # Extract relative path from source directory
  rel_path=$(echo "$src_file" | sed "s|^$SOURCE_DIR/||")

  # Construct target path
  target_file="$TARGET_DIR/$rel_path"
  target_dir=$(dirname "$target_file")

  # Create target directory if needed
  mkdir -p "$target_dir"

  # Check for conflicts (file already exists from another batch)
  if [ -f "$target_file" ]; then
    src_val=$(cat "$src_file")
    target_val=$(cat "$target_file")

    if [ "$src_val" != "$target_val" ]; then
      echo "ERROR: Conflict detected for $rel_path" >&2
      echo "  Existing value: $target_val" >&2
      echo "  New value: $src_val" >&2
      echo "  This indicates a test partitioning bug - same test in multiple batches." >&2
      exit 3
    fi
    # Skip copy if file already exists with same value
    continue
  fi

  # Copy file
  cp "$src_file" "$target_file"
  echo "Updated: $rel_path"
  UPDATED=1
done < "$TMPFILE"

rm -f "$TMPFILE"

if [ "$UPDATED" -eq 1 ]; then
  echo "Merge complete: files updated"
  exit 0
else
  echo "No files updated"
  exit 1
fi
