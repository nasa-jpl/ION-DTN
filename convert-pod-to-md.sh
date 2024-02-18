#!/bin/bash

set -e  # Exit immediately if a command exits with a non-zero status.

# Predefined list of directory pairs
# Format: "search_directory link_directory"
DIR_PAIRS=(
  "../../ams man/ams"
  "../../bpv6 man/bpv6"
  "../../bpv7 man/bpv7"
  "../../bss man/bss"
  "../../bssp man/bssp"
  "../../cfdp man/cfdp"
  "../../dgr man/dgr"
  "../../dtpc man/dtpc"
  "../../ici man/ici"
  "../../ltp man/ltp"
  "../../nm man/nm"
  "../../restart man/restart"
  "../../tc man/tc"
)

# Function to convert .pod files to Markdown in each directory pair
convert_pod_to_md() {
  SEARCH_DIR=$1
  LINK_DIR=$2

  echo "Processing .pod files in $SEARCH_DIR to store in $LINK_DIR..."

  # Create the link directory if it doesn't exist
  mkdir -p "$LINK_DIR" || { echo "Failed to create directory $LINK_DIR"; exit 1; }

  # Convert .pod files to Markdown and store them in LINK_DIR
  find "$SEARCH_DIR" -name '*.pod' -exec sh -c '
    for file do
      # Convert to Markdown and store in LINK_DIR
      pod2markdown "$file" > "'$LINK_DIR'/$(basename "${file%.pod}.md")" || { echo "Failed to convert file $file"; exit 1; }
    done
  ' sh {} + || { echo "Conversion failed in directory $SEARCH_DIR"; exit 1; }

  # Create an index.md file with links to all Markdown files except itself
  echo "Creating index.md in $LINK_DIR..."
  echo "# Index of Man Pages" > "$LINK_DIR/index.md" || { echo "Failed to create index.md in $LINK_DIR"; exit 1; }
  
  for mdfile in "$LINK_DIR"/*.md; do
    filename=$(basename "$mdfile")
    if [ "$filename" != "index.md" ]; then
      echo "- [${filename%.md}]($filename)" >> "$LINK_DIR/index.md" || { echo "Failed to write to index.md in $LINK_DIR"; exit 1; }
    fi
  done
}

# Loop through each directory pair and process them
for pair in "${DIR_PAIRS[@]}"; do
  # Split the pair into SEARCH_DIR and LINK_DIR
  read -r SEARCH_DIR LINK_DIR <<< "$pair"
  convert_pod_to_md "$SEARCH_DIR" "$LINK_DIR" || { echo "Error processing $SEARCH_DIR and $LINK_DIR"; exit 1; }
done
