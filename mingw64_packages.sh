#!/bin/bash

# Ensure this runs in an MSYS2 MINGW64 environment (e.g., mingw64.exe)

# Update package database and system
pacman -Syu --noconfirm || {
    echo "Error: Failed to update system. Check network or permissions."
    exit 1
}

# List of packages to install (MINGW64-focused, consolidated)
packages=(
    # Core development tools
    "mingw-w64-x86_64-gcc"          # GCC compiler (includes gcc-libs)
    "mingw-w64-x86_64-binutils"     # Assembler, linker, etc.
    "mingw-w64-x86_64-make"         # GNU Make
    "mingw-w64-x86_64-libtool"      # Libtool for building libraries
    "mingw-w64-x86_64-gettext"      # Internationalization
    "mingw-w64-x86_64-expat"        # XML parsing
    "mingw-w64-x86_64-libiconv"     # Character encoding conversion
    "mingw-w64-x86_64-zlib"         # Compression library
    "mingw-w64-x86_64-zstd"         # Zstandard compression

    # Math libraries (for GCC)
    "mingw-w64-x86_64-gmp"          # Arbitrary precision arithmetic
    "mingw-w64-x86_64-mpfr"         # Floating-point arithmetic
    "mingw-w64-x86_64-mpc"          # Complex numbers
    "mingw-w64-x86_64-isl"          # Integer set library

    # Build automation
    "autoconf"                      # Latest autoconf
    "automake"                      # Latest automake
    "m4"                            # Macro processor

    # GitHub CLI
    "mingw-w64-x86_64-github-cli"   # gh CLI for GitHub
    "git"    			    # git

    # Optional: SQLite and diffutils
    "libsqlite"                     # SQLite library
    "diffutils"                     # File comparison tools

    # Perl (if needed for scripts)
    "perl"                          # Base Perl
    "perl-HTTP-Message"             # HTTP handling (example, add others as needed)
)

# Install packages, exit on failure
pacman -S --needed --noconfirm "${packages[@]}" || {
    echo "Error: Failed to install packages."
    exit 1
}

echo "Installation completed successfully!"