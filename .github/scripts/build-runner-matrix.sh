#!/usr/bin/env bash
# Usage: build-runner-matrix.sh --ubuntu_20=true --ubuntu_22=false ...
# Builds JSON array of enabled arc runners for GitHub Actions matrix

set -euo pipefail

# Initialize flags (default all false)
ubuntu_20=false
ubuntu_22=false
oracle_linux_8=false
oracle_linux_9=false
rhel_8=false
rhel_9=false

# Parse command line arguments
for arg in "$@"; do
    case $arg in
        --ubuntu_20=*)
            ubuntu_20="${arg#*=}"
            ;;
        --ubuntu_22=*)
            ubuntu_22="${arg#*=}"
            ;;
        --oracle_linux_8=*)
            oracle_linux_8="${arg#*=}"
            ;;
        --oracle_linux_9=*)
            oracle_linux_9="${arg#*=}"
            ;;
        --rhel_8=*)
            rhel_8="${arg#*=}"
            ;;
        --rhel_9=*)
            rhel_9="${arg#*=}"
            ;;
        *)
            echo "ERROR: Unknown argument $arg" >&2
            exit 1
            ;;
    esac
done

# Build JSON array of enabled runners
runners=()

[[ "$ubuntu_20" == "true" ]] && runners+=("u20")
[[ "$ubuntu_22" == "true" ]] && runners+=("u22")
[[ "$oracle_linux_8" == "true" ]] && runners+=("ol8")
[[ "$oracle_linux_9" == "true" ]] && runners+=("ol9")
[[ "$rhel_8" == "true" ]] && runners+=("rhel8")
[[ "$rhel_9" == "true" ]] && runners+=("rhel9")

# Validate at least one runner selected
if [ ${#runners[@]} -eq 0 ]; then
    echo "ERROR: No runners selected. At least one runner must be enabled." >&2
    exit 1
fi

# Output JSON array
jq -c -n '$ARGS.positional' --args -- "${runners[@]}"
