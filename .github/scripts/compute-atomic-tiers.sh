#!/usr/bin/env bash
#
# compute-atomic-tiers.sh - Compute atomic tier test matrix
#
# Determines which atomic tiers (native, __atomic, __sync) to test based on
# tier filter and platform inputs. Used by ci-workflow-atomic-tiers.yml.
#
# Usage:
#   compute-atomic-tiers.sh --tier-filter=<filter> --platforms=<platforms>
#
# Parameters:
#   --tier-filter=<value>  Tier selection: all, fallbacks, native, __atomic, __sync
#   --platforms=<value>    Platform selection: both, x86_64, arm64
#
# Output (to $GITHUB_OUTPUT):
#   x86_tiers=<JSON array>  Tiers for x86_64 platform
#   arm_tiers=<JSON array>  Tiers for arm64 platform
#
# Exit codes:
#   0 - Success
#   1 - Invalid tier_filter, invalid platforms, or no jobs to run

set -euo pipefail

# Parse command-line arguments
TIER_FILTER=""
PLATFORMS=""

for arg in "$@"; do
    case $arg in
        --tier-filter=*)
            TIER_FILTER="${arg#*=}"
            ;;
        --platforms=*)
            PLATFORMS="${arg#*=}"
            ;;
        *)
            echo "Unknown argument: $arg" >&2
            exit 1
            ;;
    esac
done

# Validate required arguments
if [ -z "$TIER_FILTER" ] || [ -z "$PLATFORMS" ]; then
    echo "Error: Missing required arguments" >&2
    echo "Usage: compute-atomic-tiers.sh --tier-filter=<filter> --platforms=<platforms>" >&2
    exit 1
fi

# Compute tier lists based on tier_filter
# x86_64: exclude 'native' because Tier 1 is already covered by ci-workflow-arc.yml
case "$TIER_FILTER" in
    all)
        X86='["__atomic","__sync"]'
        ARM='["native","__atomic","__sync"]'
        ;;
    fallbacks)
        X86='["__atomic","__sync"]'
        ARM='["__atomic","__sync"]'
        ;;
    native)
        X86='[]'
        ARM='["native"]'
        ;;
    __atomic)
        X86='["__atomic"]'
        ARM='["__atomic"]'
        ;;
    __sync)
        X86='["__sync"]'
        ARM='["__sync"]'
        ;;
    *)
        echo "Unknown tier_filter: $TIER_FILTER" >&2
        exit 1
        ;;
esac

# Apply platform filter: zero out the tier list for any excluded platform.
# Downstream jobs already gate on `tier_list != '[]'`, so this cleanly
# skips the excluded platform's build/test/arm64 jobs.
case "$PLATFORMS" in
    both)
        # No filtering - use both platforms
        ;;
    x86_64)
        ARM='[]'
        ;;
    arm64)
        X86='[]'
        ;;
    *)
        echo "Unknown platforms: $PLATFORMS" >&2
        exit 1
        ;;
esac

# Verify at least one platform has jobs
if [ "$X86" = '[]' ] && [ "$ARM" = '[]' ]; then
    echo "Error: tier_filter='$TIER_FILTER' combined with platforms='$PLATFORMS' leaves no jobs to run." >&2
    exit 1
fi

# Output to GitHub Actions
{
    echo "x86_tiers=$X86"
    echo "arm_tiers=$ARM"
} >> "$GITHUB_OUTPUT"

# Log for workflow visibility
echo "x86 tiers: $X86"
echo "arm tiers: $ARM"
