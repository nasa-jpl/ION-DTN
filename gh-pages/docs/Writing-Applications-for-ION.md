# Using ION with pkg-config

When using the automake build system, ION provides a `pkg-config` file (`ion.pc`) that contains all the necessary compiler and linker flags for building external applications against ION libraries.

## Checking ION Installation

First, verify ION's pkg-config information is available:

```bash
# Check if ION is found by pkg-config
pkg-config --exists ion && echo "ION found" || echo "ION not found"

# Display ION version
pkg-config --modversion ion

# Show all ION package information
pkg-config --print-provides ion
```

## Getting Compiler and Linker Flags

```bash
# Get compiler flags (includes platform-specific macros)
pkg-config --cflags ion

# Get linker flags and libraries
pkg-config --libs ion

# Get both compiler and linker flags
pkg-config --cflags --libs ion
```

## Example Usage in Application Development

### Simple Compilation

```bash
# Compile a simple ION application
gcc $(pkg-config --cflags ion) -o myapp myapp.c $(pkg-config --libs ion)
```

## Troubleshooting

If `pkg-config` cannot find ION:

1. **Check installation path**:
   ```bash
   find /usr -name "ion.pc" 2>/dev/null
   find /usr/local -name "ion.pc" 2>/dev/null
   ```

2. **Update PKG_CONFIG_PATH** if ION is installed in a non-standard location:
   ```bash
   export PKG_CONFIG_PATH="/path/to/ion/lib/pkgconfig:$PKG_CONFIG_PATH"
   ```

3. **Verify ION installation** completed successfully and installed the `.pc` file.

## Benefits of Using pkg-config

- **Automatic platform detection**: Gets the correct `-Dunix`, `-Dlinux`, etc. flags
- **Version compatibility**: Ensures you get flags matching your ION installation  
- **Crypto backend awareness**: Automatically includes MbedTLS libraries if ION was built with crypto support
- **Maintainability**: No need to manually track ION's build configuration changes