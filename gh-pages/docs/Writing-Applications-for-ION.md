# Using ION with pkg-config

When using the automake build system, ION provides a `pkg-config` file (`ion.pc`) that contains all the necessary compiler and linker flags for building external applications against ION libraries.

> **WARNING - ABI Compatibility**: You **MUST** use `pkg-config --cflags ion` or manually specify the exact same platform flags that ION was compiled with. ION's public structures (such as `IonParms` in `ion.h`) contain members whose sizes depend on platform-specific preprocessor definitions. For example, `IonParms.pathName` is sized using `MAXPATHLEN`, which is only correctly defined when platform flags like `-Dlinux` are present. Without these flags, your application will use different struct sizes than the ION library, causing buffer overruns and segmentation faults when calling functions like `ionInitialize()`.

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

## Writing Shutdown-Aware Applications

**IMPORTANT**: When writing BP applications, you must design them to detect and respond to ION shutdown gracefully. This is critical for operational environments where ION operators and application users may be different people.

### Why This Matters

When ION shuts down via `ionstop` or `bpadmin .`, it signals all BP applications to terminate by calling `sm_SemEnd()` on endpoint semaphores. Applications that properly check for `BpEndpointStopped` will automatically exit cleanly, allowing ION to shut down without hanging.

**If your application does NOT check for `BpEndpointStopped`:**
- ION shutdown (`bpadmin .`) may hang indefinitely waiting for your application to exit
- Operators will need to manually terminate your application before shutting down ION
- This creates coordination problems in multi-user environments

### Required Pattern for All BP Applications

Every BP application that receives bundles **MUST** check for `BpEndpointStopped` and exit gracefully:

```c
while (running)
{
    if (bp_receive(sap, &dlv, BP_BLOCKING) < 0)
    {
        running = 0;
        continue;
    }

    // REQUIRED: Check for ION shutdown
    if (dlv.result == BpEndpointStopped)
    {
        writeMemo("ION is shutting down - exiting gracefully.");
        running = 0;  // Exit your main loop
    }
    else if (dlv.result == BpPayloadPresent)
    {
        // Process bundle
    }

    bp_release_delivery(&dlv, 1);
}

// Clean up before exiting
bp_close(sap);
bp_detach();
```

### Additional Shutdown Best Practices

1. **Handle SIGTERM** - Implement signal handlers so operators can manually terminate your application:
   ```c
   signal(SIGTERM, sigterm_handler);
   signal(SIGINT, sigint_handler);
   ```

2. **Avoid long operations** - If your application performs lengthy processing between `bp_receive()` calls, consider checking shutdown status periodically

3. **Clean up resources** - Always call `bp_close()` and `bp_detach()` before exiting

4. **Log shutdown events** - Use `writeMemo()` to log when your application detects shutdown for debugging

### Complete Example

See the full shutdown-aware application pattern with signal handling in the [BP Service API Guide, ION Shutdown Order section](./BP-Service-API.md#critical-resource-management-warnings).

### For More Information

For detailed information about ION shutdown behavior, application shutdown detection, and complete code examples, see:
- [BP Service API - ION Shutdown Order and Application Graceful Termination](./BP-Service-API.md#critical-resource-management-warnings)
