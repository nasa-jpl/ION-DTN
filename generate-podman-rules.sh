#!/usr/bin/env bash
# generate-podman-rules.sh  <submodule>
# Emits explicit rules such as
#   ici/doc/ionunlock.1: ici/doc/pod1/ionunlock.pod
#       pod2man -s 1 -c "ICI executables" $? $@

set -e
[ $# -eq 1 ] || { echo "Usage: $0 <submodule>" >&2; exit 1; }

sub=$1               # e.g. ici
doc="$sub/doc"       # e.g. ici/doc
modname=$(printf '%s' "$sub" | tr '[:lower:]' '[:upper:]')

for sec in 1 3 5; do
    case $sec in
        1) kind="executables" ;;
        3) kind="library functions" ;;
        5) kind="configuration files" ;;
    esac

    dir="$doc/pod$sec"
    [ -d "$dir" ] || continue
    for pod in "$dir"/*.pod; do
        [ -f "$pod" ] || continue
        base=$(basename "$pod" .pod)

        # IMPORTANT:  Generate target path WITHOUT "./"
        target_path="$doc/$base.$sec"
        printf '%s: %s/%s.pod\n' "$target_path" "$dir" "$base"
        printf '\tpod2man -s %s -c "%s %s" \$? $@\n' "$sec" "$modname" "$kind"
    done
done