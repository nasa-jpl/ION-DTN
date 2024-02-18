#!/bin/bash

# Function to display help text
show_help() {
    echo "Usage: $0 -f <file-path> -b <base-branch> -s <branch-filter>"
    echo "  -f  Relative path to the file you want to check across branches"
    echo "  -b  Base branch name to compare other branches against"
    echo "  -s  String to filter branch names"
    echo "Example: $0 -f src/main.c -b master -s feature"
    echo " "
    echo "NOTE: it is recommended to add this script to /usr/local/bin "
    echo "so it can be run from anywhere"
}

# Initialize variables
FILE_PATH=""
BASE_BRANCH=""
BRANCH_FILTER=""

# Parse command-line options
while getopts 'hf:b:s:' flag; do
    case "${flag}" in
        f) FILE_PATH="${OPTARG}" ;;
        b) BASE_BRANCH="${OPTARG}" ;;
        s) BRANCH_FILTER="${OPTARG}" ;;
        h) show_help
           exit 0 ;;
        *) show_help
           exit 1 ;;
    esac
done

# Check if required options are provided
if [ -z "$FILE_PATH" ] || [ -z "$BASE_BRANCH" ] || [ -z "$BRANCH_FILTER" ]; then
    echo "Error: Missing required arguments"
    show_help
    exit 1
fi

# Fetch all remote branches
git fetch --all

# List all branches and loop through them
for branch in $(git branch -r | grep -v HEAD); do
    # Check if the branch name contains the filter string
    if [[ $branch == *"$BRANCH_FILTER"* ]]; then
        # Check if the file differs from the base branch
        if git diff --quiet $BASE_BRANCH..$branch -- $FILE_PATH; then
            echo "No changes in $branch for file $FILE_PATH"
        else
            echo "Changes in $branch for file $FILE_PATH"
        fi
    fi
done
