#!/bin/sh
#
# Advisory lint for ION regression-test teardown conventions.
#
# Checks ONLY the files passed as command-line arguments. This script
# NEVER exits non-zero: findings are emitted as GitHub warning annotations
# and a step-summary table so they surface on the PR without blocking merge.
#
# Conventions checked (see gh-pages/docs/ION-TestSet-Readme.md):
#   A. A cleanup script that stops ION should use `killm f`, not bare
#      `killm`.  In a multi-node environment bare `killm` only stops the
#      current node (or refuses from a non-node directory) and leaves
#      shared IPC behind.
#   B. A multi-node cleanup must `export ION_NODE_LIST_DIR=$PWD`.  Cleanup
#      runs as a separate subprocess from dotest and does not inherit it;
#      without it `killm f` cannot find ion_nodes to do the per-node
#      graceful shutdown.
#   C. An ION dotest should `trap 'killm f' EXIT` so ION is torn down on
#      every exit path.
#
# Usage: lint-test-conventions.sh [file ...]

set -u +e

DOC="gh-pages/docs/ION-TestSet-Readme.md"
FINDINGS=0
SUMMARY=""

# Exit early if no files were passed
if [ "$#" -eq 0 ]; then
    echo "No files provided."
    if [ -n "${GITHUB_STEP_SUMMARY:-}" ]; then
        {
            echo "## Test teardown convention check (advisory)"
            echo ""
            echo "No changed cleanup/dotest scripts. :white_check_mark:"
        } >> "$GITHUB_STEP_SUMMARY"
    fi
    exit 0
fi

note() {
    _file="$1"
    _line="$2"
    _msg="$3"

    if [ -n "${GITHUB_ACTIONS:-}" ]; then
        if [ -n "$_line" ]; then
            echo "::warning file=${_file},line=${_line}::${_msg}"
        else
            echo "::warning file=${_file}::${_msg}"
        fi
    fi
    echo "  ${_file}${_line:+:${_line}}: ${_msg}"

    # POSIX safe newline accumulation
    SUMMARY="${SUMMARY}| \`${_file}\` | ${_line:--} | ${_msg} |
"
    FINDINGS=$((FINDINGS + 1))
}

_killm_lines() {
    grep -nE '(^|[^[:alnum:]_])killm([^[:alnum:]_]|$)' "$1" 2>/dev/null \
        | grep -vE '^[[:space:]]*[0-9]+:[[:space:]]*#'
}

check_cleanup() {
    _f="$1"
    _dir=$(dirname "$_f")
    _sibling="${_dir}/dotest"

    if ! _killm_lines "$_f" >/dev/null; then
        return
    fi

    _bad_line=$(_killm_lines "$_f" \
        | grep -vE '(^|[^[:alnum:]_])killm[[:space:]]+f([^[:alnum:]_]|$)' \
        | head -n 1 | cut -d: -f1)

    if [ -n "$_bad_line" ]; then
        note "$_f" "$_bad_line" \
            "cleanup calls bare 'killm'; use 'killm f' for a full forced cleanup. See ${DOC}."
    fi

    if [ -f "$_sibling" ] && grep -qE '(^|[^[:alnum:]_])ION_NODE_LIST_DIR' "$_sibling"; then
        if ! grep -qE 'export[[:space:]]+ION_NODE_LIST_DIR' "$_f"; then
            _km_line=$(_killm_lines "$_f" | head -n 1 | cut -d: -f1)
            note "$_f" "$_km_line" \
                "multi-node test, but cleanup does not 'export ION_NODE_LIST_DIR=\$PWD'; killm f cannot locate ion_nodes. See ${DOC}."
        fi
    fi
}

check_dotest() {
    _f="$1"
    if ! grep -qE '(^|[^[:alnum:]_])(killm|ionstart)([^[:alnum:]_]|$)' "$_f"; then
        return
    fi
    if ! grep -qE "trap[[:space:]]+.*killm[[:space:]]+f.*EXIT" "$_f"; then
        note "$_f" "" \
            "dotest should \"trap 'killm f' EXIT\" near the top so ION is cleaned up on every exit path. See ${DOC}."
    fi
}

printf 'Checking %d changed script(s):\n' "$#"
printf '  %s\n' "$@"

for f in "$@"; do
    [ -f "$f" ] || continue
    case "$(basename "$f")" in
        cleanup) check_cleanup "$f" ;;
        dotest)  check_dotest "$f" ;;
    esac
done

if [ -n "${GITHUB_STEP_SUMMARY:-}" ]; then
    {
        echo "## Test teardown convention check (advisory)"
        echo ""
        if [ "$FINDINGS" -eq 0 ]; then
            echo "No teardown-convention issues in the changed test scripts. :white_check_mark:"
        else
            echo "Found **${FINDINGS}** advisory finding(s). This check is **non-blocking** and does not prevent merge; address them or merge over them with a reason. See \`${DOC}\`."
            echo ""
            echo "| File | Line | Finding |"
            echo "|------|------|---------|"
            printf '%s' "$SUMMARY"
        fi
    } >> "$GITHUB_STEP_SUMMARY"
fi

if [ "$FINDINGS" -eq 0 ]; then
    echo "Test teardown convention check: no issues."
else
    echo "Test teardown convention check: ${FINDINGS} advisory finding(s) (non-blocking)."
fi

exit 0
