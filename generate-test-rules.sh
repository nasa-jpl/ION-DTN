#!/usr/bin/env bash
# generate-test-rules.sh  <tests-dir>
# Emits lines like:
#   .PHONY: test-normaltests
#   test-normaltests: buildcheck
#       cd $(srcdir)/tests && ./runtestset normaltests

set -e
[ $# -eq 1 ] || { echo "Usage: $0 <tests-dir>" >&2; exit 1; }
tests_dir=$1

# For each plain file in tests/ (skip directories and source files)
for file in "$tests_dir"/*; do
    [ -f "$file" ] || continue
    name=$(basename "$file")

    # Skip anything with a dot (.) or colon (:) in its name,
    # so we only get plain test-set files like "normaltests", "quicktests", etc.
    case "$name" in
        *.*|*:* ) continue ;;
    esac

    printf 'test-%s: buildcheck\n'  "$name"
    printf '\tcd $(srcdir)/tests && ./runtestset %s\n\n' "$name"
done
