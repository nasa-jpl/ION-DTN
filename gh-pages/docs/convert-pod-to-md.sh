#!/bin/bash

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
  mkdir -p "$LINK_DIR"

  # Convert .pod files to Markdown and store them in LINK_DIR
  find "$SEARCH_DIR" -name '*.pod' -exec sh -c '
    for file do
      # Convert to Markdown and store in LINK_DIR
      pod2markdown "$file" > "'$LINK_DIR'/$(basename "${file%.pod}.md")"
    done
  ' sh {} +

  # Create an index.md file with links to all Markdown files except itself
  echo "Creating index.md in $LINK_DIR..."
  echo "# Index of Man Pages" > "$LINK_DIR/index.md"
  for mdfile in "$LINK_DIR"/*.md; do
    filename=$(basename "$mdfile")
    if [ "$filename" != "index.md" ]; then
      echo "- [${filename%.md}]($filename)" >> "$LINK_DIR/index.md"
    fi
  done
}

# Loop through each directory pair and process them
for pair in "${DIR_PAIRS[@]}"; do
  # Split the pair into SEARCH_DIR and LINK_DIR
  read -r SEARCH_DIR LINK_DIR <<< "$pair"
  convert_pod_to_md "$SEARCH_DIR" "$LINK_DIR"
done