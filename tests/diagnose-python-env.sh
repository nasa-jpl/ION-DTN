#!/usr/bin/env bash
# Diagnostic script to trace Python environment issues in ephemeral pods
# This gathers evidence at each layer to identify where bespokebpv7 lookup fails

echo "=== Layer 1: Environment Variables ==="
echo "PYENV_ROOT: ${PYENV_ROOT:-UNSET}"
echo "PATH: $PATH"
echo "SHELL: $SHELL"
echo ""

echo "=== Layer 2: Python Resolution ==="
echo "which python3: $(which python3 || echo 'NOT FOUND')"
echo "python3 --version: $(python3 --version 2>&1 || echo 'FAILED')"
echo "python3 location: $(python3 -c 'import sys; print(sys.executable)' 2>&1 || echo 'FAILED')"
echo ""

echo "=== Layer 3: Pyenv Status ==="
if [ -n "$PYENV_ROOT" ] && [ -d "$PYENV_ROOT" ]; then
    echo "PYENV_ROOT exists: YES"
    echo "Pyenv global: $(${PYENV_ROOT}/bin/pyenv global 2>&1 || echo 'FAILED')"
    echo "Pyenv versions: $(${PYENV_ROOT}/bin/pyenv versions 2>&1 || echo 'FAILED')"
    echo "Pyenv python3: $(${PYENV_ROOT}/bin/pyenv which python3 2>&1 || echo 'FAILED')"
else
    echo "PYENV_ROOT: NOT SET OR DOES NOT EXIST"
fi
echo ""

echo "=== Layer 4: Pyenv Init Check ==="
if command -v pyenv >/dev/null 2>&1; then
    echo "pyenv command available: YES"
    echo "pyenv shims in PATH: $(echo $PATH | grep -q '.pyenv/shims' && echo 'YES' || echo 'NO')"
else
    echo "pyenv command available: NO"
fi
echo ""

echo "=== Layer 5: Python Packages ==="
echo "Checking bespokebpv7 with python3:"
python3 -c "import bespokebpv7; print('bespokebpv7 found:', bespokebpv7.__file__)" 2>&1 || echo "bespokebpv7: NOT FOUND in python3"
echo ""

if [ -n "$PYENV_ROOT" ] && [ -f "${PYENV_ROOT}/versions/3.11.15/bin/python3" ]; then
    echo "Checking bespokebpv7 with pyenv python directly:"
    ${PYENV_ROOT}/versions/3.11.15/bin/python3 -c "import bespokebpv7; print('bespokebpv7 found:', bespokebpv7.__file__)" 2>&1 || echo "bespokebpv7: NOT FOUND in pyenv python"
fi
echo ""

echo "=== Layer 6: Shell Profile Files ==="
echo "~/.bashrc contains pyenv init: $(grep -q 'pyenv init' ~/.bashrc 2>/dev/null && echo 'YES' || echo 'NO')"
echo "~/.profile contains pyenv init: $(grep -q 'pyenv init' ~/.profile 2>/dev/null && echo 'YES' || echo 'NO')"
echo ""

echo "=== Diagnosis Complete ==="
echo "If bespokebpv7 is found in pyenv python but NOT in python3,"
echo "then pyenv is not initialized in the shell environment."
