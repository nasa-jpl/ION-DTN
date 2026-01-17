#!/usr/bin/env bash
# generate-ps-rules.sh  <srcdir>
# Emits explicit rules for every *.1, *.3, *.5 → *.1.ps, *.3.ps, *.5.ps

set -e
[ $# -eq 1 ] || { echo "Usage: $0 <srcdir>" >&2; exit 1; }
srcdir=$1

cd "$srcdir" || exit 1

# find all man pages, strip leading "./", sort
find . -type f \( -name '*.1' -o -name '*.3' -o -name '*.5' \) \
    | sed 's|^\./||' \
    | sort \
    | while read man; do
          echo "${man}.ps: ${man}"
          printf '%b\n\n' '\tgroff -mandoc $^ > $@'
          echo
      done
