#!/usr/bin/env bash
# generate-podman-rules.sh  <srcdir> <submodule>
# Emits explicit rules such as
#   ici/doc/ionunlock.1: $(srcdir)/ici/doc/pod1/ionunlock.pod
#       pod2man -s 1 -c "ICI executables" $? $@
#
# Targets are build-relative and prerequisites are srcdir-relative, so the
# rules work whether the build is in the source tree or out of it.

set -e
[ $# -eq 2 ] || { echo "Usage: $0 <srcdir> <submodule>" >&2; exit 1; }

srcdir=$1             # e.g. "." or /path/to/ion
sub=$2                # e.g. ici
doc="$sub/doc"        # e.g. ici/doc, build-relative, used for targets
srcdoc="$srcdir/$doc" # where the .pod sources live
modname=$(printf '%s' "$sub" | tr '[:lower:]' '[:upper:]')

for sec in 1 3 5; do
    case $sec in
        1) kind="executables" ;;
        3) kind="library functions" ;;
        5) kind="configuration files" ;;
    esac

    dir="$srcdoc/pod$sec"
    [ -d "$dir" ] || continue
    for pod in "$dir"/*.pod; do
        [ -f "$pod" ] || continue
        base=$(basename "$pod" .pod)

        # IMPORTANT:  Generate target path WITHOUT "./"
        target_path="$doc/$base.$sec"
        printf '%s: $(srcdir)/%s/pod%s/%s.pod\n' \
            "$target_path" "$doc" "$sec" "$base"
        printf '\tpod2man -s %s -c "%s %s" \$? $@\n' "$sec" "$modname" "$kind"
    done
done
