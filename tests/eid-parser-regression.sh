#!/usr/bin/env bash
# Helpers for the two parser caller regressions. Run only in an isolated node.
set -euo pipefail

fail() { echo "FAIL: $*" >&2; exit 1; }
cleanup() {
    local status=$?
    if ! killm f; then
        echo "FAIL: ION cleanup did not complete" >&2
        [[ "$status" != 0 ]] || status=1
    fi
    rm -f test.rc admin.in admin.out endpoint.out
    exit "$status"
}
setup_loopback() {
    local config="${CONFIGSROOT:-../../configs}/loopback-udp/loopback.rc"
    if [[ "$1" == imc ]]; then
        for tool in imcfw imcadminep psmwatch; do
            if ! command -v "$tool" >/dev/null 2>&1; then
                echo "SKIP: missing $tool"
                exit 2
            fi
        done
        # Add the IMC scheme before BP starts its daemons.
        awk '/^a scheme ipn / {print; print "a scheme imc '''imcfw''' '''imcadminep'''"; next} {print}' \
            "$config" > test.rc
    else
        cp "$config" test.rc
    fi
    if ! killm f; then
        rm -f test.rc
        fail "Cannot clean the previous ION node"
    fi
    trap cleanup EXIT
    ionstart -I test.rc
}
admin() {
    bpadmin admin.in > admin.out
    if grep -E "\[\?\]|Can't|Unknown|Duplicate|[Ss]yntax error" admin.out; then
        fail "Endpoint administration failed"
    fi
}
drain() {
    local attempts count
    for ((attempts=0; attempts<20; attempts++)); do
        count=$(bplist count | sed -n 's/^Report count is \([0-9]*\)\.$/\1/p')
        [[ -n "$count" ]] || fail "Cannot read bundle count"
        [[ "$count" == 0 ]] && return
        sleep 1
    done
    fail "IMC administrative bundles did not drain"
}
memory() {
    psmwatch 0 1 | awk '/total unavbl:/ {sum += $NF; n++} END {if(n != 2) exit 1; print sum}'
}
cycles() {
    local i
    : > admin.in
    for ((i=0; i<50; i++)); do
        printf 'a endpoint imc:0.9 q\nd endpoint imc:0.9\n' >> admin.in
    done
    admin
    drain
    printf 'l endpoint\n' > admin.in
    admin
    if grep -q 'imc:0.9' admin.out; then
        fail "Temporary IMC endpoint was not deleted"
    fi
}

settled_memory() {
    # Take the minimum over several samples to avoid transient daemon scratch.
    local sample value minimum
    minimum=$(memory)
    for sample in 1 2; do
        sleep 1
        value=$(memory)
        [[ "$value" -ge "$minimum" ]] || minimum=$value
    done
    printf '%s\n' "$minimum"
}
