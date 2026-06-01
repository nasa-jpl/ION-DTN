#!/usr/bin/env bash
#
# compute-tier-cflags.sh - Compute CFLAGS for atomic tier testing
#
# Maps atomic tier name to corresponding CFLAGS and configure settings.
# Used by ci-workflow-atomic-tiers.yml build job.
#
# Usage:
#   compute-tier-cflags.sh --tier=<tier>
#
# Parameters:
#   --tier=<value>  Atomic tier: native, __atomic, __sync
#
# Output (to $GITHUB_OUTPUT):
#   cflags=<string>      CFLAGS for this tier
#   ac_cv_c11=<yes|no>   Whether C11 is available/used
#
# Exit codes:
#   0 - Success
#   1 - Unknown tier

set -euo pipefail

# Parse command-line arguments
TIER=""

for arg in "$@"; do
    case $arg in
        --tier=*)
            TIER="${arg#*=}"
            ;;
        *)
            echo "Unknown argument: $arg" >&2
            exit 1
            ;;
    esac
done

# Validate required argument
if [ -z "$TIER" ]; then
    echo "Error: Missing required argument" >&2
    echo "Usage: compute-tier-cflags.sh --tier=<tier>" >&2
    exit 1
fi

# Compute CFLAGS and ac_cv_c11 based on tier
#
# Each fallback tier needs BOTH levers set (see
# gh-pages/docs/ION-Coding-Guide.md "Testing the Fallback Tiers"):
#   (A) language mode — short-circuit configure.ac's C18 probe with
#       ac_cv_c11=no and pass -std=c99 in CFLAGS so AM_CFLAGS + user
#       CFLAGS resolve to C99.
#   (B) tier dispatch — ION_TEST_FORCE_FALLBACK moves Zone 1 to its
#       mutex fallback and Zone 2 down the tier ladder;
#       ION_TEST_FORCE_SYNC_FALLBACK forces Zone 2 all the way to
#       Tier 3 (__sync).
# Passing only the force macros leaves the compiler in C18 mode, which
# does NOT exercise the genuine C99 language environment ION would see
# on a pre-GCC-4.7 flight toolchain.
case "$TIER" in
    native)
        CFLAGS="-DHAVE_VALGRIND_VALGRIND_H"
        AC_CV_C11="yes"
        ;;
    __atomic)
        CFLAGS="-DHAVE_VALGRIND_VALGRIND_H -std=c99 -DION_TEST_FORCE_FALLBACK"
        AC_CV_C11="no"
        ;;
    __sync)
        CFLAGS="-DHAVE_VALGRIND_VALGRIND_H -std=c99 -DION_TEST_FORCE_FALLBACK -DION_TEST_FORCE_SYNC_FALLBACK"
        AC_CV_C11="no"
        ;;
    *)
        echo "Unknown tier: $TIER" >&2
        exit 1
        ;;
esac

# Output to GitHub Actions
{
    echo "cflags=$CFLAGS"
    echo "ac_cv_c11=$AC_CV_C11"
} >> "$GITHUB_OUTPUT"

# Log for workflow visibility
echo "Tier $TIER CFLAGS:     $CFLAGS"
echo "Tier $TIER ac_cv_c11: $AC_CV_C11"
