# BSL Build and Install Guide

!!! note "Feature Branch & Build System"
    - BSL integration is available on the `feature-4.1.4-bsl` branch
    - This guide covers **development Makefiles only** (Makefile.dev)
    - Automake build instructions are under development

## What is BSL?

BSL (Bundle Protocol Security Library) is an external BPSec implementation that ION can use as an alternative to ION's native BPSec. BSL implements RFC 9172/9173 security contexts and is integrated as a git submodule at `external/BSL/`.

**ION Integration Branch**: The `bsl-ion-integration` branch of BSL integrates BSL's memory allocation/deallocation with ION's SDR (Simple Data Recorder) memory management system. This creates a mutual dependency between BSL and ION that requires a specific 3-step build process.

## Prerequisites

### Required System Packages

**Ubuntu/Debian:**
```bash
sudo apt install -y build-essential cmake pkg-config git \
    libssl-dev libjansson-dev ninja-build
```

**RHEL/Fedora/CentOS:**
```bash
sudo dnf install -y gcc gcc-c++ cmake pkg-config git \
    openssl-devel jansson-devel ninja-build
```

See [Appendix A](#appendix-a-detailed-prerequisites) for complete dependency information.

## Quick Start Guide

### Overview

Building ION with BSL requires three steps in order:

1. **Build ION without BSL** - Creates base ION libraries
2. **Build and install BSL** - Uses `build-for-ion.sh` to compile BSL with ION integration
3. **Rebuild ION with BSL** - Recompiles ION to link against BSL libraries

### Step 1: Build ION Without BSL

```bash
cd /path/to/ion-ios-dev

# Enable development Makefiles
./enable_manual_build.sh --skip-clean

# Build ION (without BSL)
make clean
make -j4

# Install
sudo make install
```

!!! warning "Why This 3-Step Process?"
    The `bsl-ion-integration` branch creates a **mutual dependency** between BSL and ION:

    - **BSL depends on ION**: BSL's memory allocators (`ion_malloc`, `ion_free`) call ION's `allocFromIonMemory()` and `releaseToIonMemory()` functions. BSL's `bsl_ionpatch` library links against ION's `libici.so`.
    - **ION depends on BSL**: When built with `USING_BSL=1`, ION links against BSL's security libraries.

    This circular dependency requires the bootstrap process: Build ION first (creates `libici.so`) → Build BSL (links against `libici.so`) → Rebuild ION with BSL (links against BSL libraries).

### Step 2: Build and Install BSL

```bash
cd /path/to/ion-ios-dev

# Clone BSL submodule (if not already present)
# Note: The submodule is configured to use the bsl-ion-integration branch
git submodule update --init external/BSL
cd external/BSL

# Initialize BSL's dependencies (QCBOR, mlib, Unity)
git submodule update --init --recursive

# Build BSL with ION integration
./build-for-ion.sh
```

!!! info "BSL Branch"
    The BSL submodule is configured to track the **`bsl-ion-integration`** branch (see `.gitmodules`). This branch includes ION memory integration code in `src/ION_integration/ionpatch.c` that implements BSL's memory allocators using ION's `allocFromIonMemory()` and `releaseToIonMemory()` functions.

The `build-for-ion.sh` script:
- Cleans previous builds
- Builds dependencies (QCBOR, mlib, Unity)
- Configures BSL with `-DION_INTEGRATION=ON` and `-DION_ROOT`
- Finds and links against ION's `libici.so` (required for ION memory integration)
- Builds BSL libraries with ION memory allocators
- Installs to `external/BSL/testroot/usr/` (default BSL_HOME location)

**Expected output:**
```
Using ION_ROOT: /path/to/ion-ios-dev
...
BSL built successfully with ION memory allocators
Libraries installed to: /path/to/ion-ios-dev/external/BSL/testroot/usr/lib
```

**Configure library path:**
```bash
# Add BSL libraries to LD_LIBRARY_PATH
export LD_LIBRARY_PATH=$(pwd)/testroot/usr/lib:$LD_LIBRARY_PATH

# Make permanent (add to ~/.bashrc)
echo "export LD_LIBRARY_PATH=$(pwd)/testroot/usr/lib:\$LD_LIBRARY_PATH" >> ~/.bashrc
```

**Verify installation:**
```bash
ls -la testroot/usr/lib/libbsl_*.so
# Should show: libbsl_crypto.so, libbsl_default_sc.so, libbsl_dynamic.so,
#              libbsl_front.so, libbsl_mock_bpa.so, libbsl_sample_pp.so
```

### Step 3: Rebuild ION with BSL

```bash
cd /path/to/ion-ios-dev

# Clean previous build
make clean

# Build with BSL enabled
make -j4 USING_BSL=1

# Verify BSL linkage
ldd bpv7/x86_64-linux/lib/libbp.so | grep bsl
# Should show BSL libraries from external/BSL/testroot/usr/lib

# Install
sudo make install
```

**Verify installed binary has BSL:**
```bash
ldd /usr/local/lib/libbp.so | grep bsl
strings /usr/local/bin/bpadmin | grep "m bsl"
```

You should see:
- BSL library paths in `ldd` output
- "m bsl" command help text in `bpadmin`

## Configuration

### BSL_HOME Variable

By default, ION's development Makefiles expect BSL at:
```
BSL_HOME = external/BSL/testroot/usr
```

This matches the default install location used by `build-for-ion.sh`.

To use a different location:
```bash
make USING_BSL=1 BSL_HOME=/custom/path/to/bsl/installation
```

### Runtime Configuration

BSL uses JSON files for keys and policies:

**Key file** (`keys.json`):
```json
{
  "keys": [
    {
      "kty": "oct",
      "kid": "myHmacKey",
      "k": "cXdlcnR5dWlvcGFzZGZnaA=="
    }
  ]
}
```

**Policy file** (`policy.json`):
```json
{
  "policyrule_set": [
    {
      "policyrule": {
        "filter": {
          "rule_id": "1",
          "role": "s",
          "src": "ipn:2.*",
          "dest": "ipn:3.*",
          "tgt": 1,
          "loc": "appin",
          "sc_id": 1
        },
        "spec": {
          "sc_id": 1,
          "sc_parms": [
            {"id": "key_name", "value": "myHmacKey"},
            {"id": "sha_variant", "value": "5"},
            {"id": "scope_flags", "value": "7"}
          ]
        }
      }
    }
  ]
}
```

**Configure in bprc:**
```bash
# Initialize BSL with security EID, key file, and policy file
m bsl 'ipn:2.0' '/path/to/keys.json' '/path/to/policy.json'
```

### EID Pattern Matching and Wildcards

BSL policy files support wildcard patterns for matching source and destination EIDs. Understanding how 2-part and 3-part IPN formats interact with wildcards is essential for writing correct security policies.

#### IPN Format Evolution

The IPN addressing scheme has evolved to support allocator-based addressing:

- **Legacy 2-part format**: `ipn:node.service` (e.g., `ipn:2.1`)
  - Used before allocator support was added
  - Assumes allocator = 0 (default allocator)

- **Modern 3-part format**: `ipn:allocator.node.service` (e.g., `ipn:0.2.1`)
  - Explicitly specifies all three components
  - Required when using non-zero allocators

**Backward Compatibility:**
- ION automatically converts 2-part format to 3-part internally
- `ipn:2.1` is internally represented as `ipn:0.2.1`
- Existing policies using 2-part format continue to work

#### Wildcard Interpretation

The `*` wildcard matches any value in its position. The interpretation depends on the number of dot-separated components:

| Pattern | Token Count | Interpretation | Matches | Does NOT Match |
|---------|-------------|----------------|---------|----------------|
| `ipn:2.*` | 2 (legacy) | allocator=0, node=2, service=* | `ipn:0.2.1`<br>`ipn:0.2.5`<br>`ipn:0.2.99` | `ipn:1.2.1` (allocator≠0)<br>`ipn:0.3.1` (node≠2) |
| `ipn:0.2.*` | 3 (explicit) | allocator=0, node=2, service=* | `ipn:0.2.1`<br>`ipn:0.2.5`<br>`ipn:0.2.99` | `ipn:1.2.1` (allocator≠0)<br>`ipn:0.3.1` (node≠2) |
| `ipn:2.*.*` | 3 (modern) | allocator=2, node=*, service=* | `ipn:2.1.1`<br>`ipn:2.3.5`<br>`ipn:2.99.99` | `ipn:0.2.1` (allocator≠2)<br>`ipn:1.2.1` (allocator≠2) |
| `ipn:*.*.*` | 3 (universal) | allocator=*, node=*, service=* | All IPN EIDs | None |
| `ipn:0.*.*` | 3 (default allocator) | allocator=0, node=*, service=* | `ipn:0.1.1`<br>`ipn:0.2.5`<br>`ipn:0.99.99` | `ipn:1.2.1` (allocator≠0) |

**Key Principle**: The wildcard `*` matches **only the field in its position**, not multiple fields.

#### 2-Part Format Behavior

When ION sees a 2-part IPN pattern like `ipn:2.*`:

1. Tokenizes by `.` → `["2", "*"]` (2 tokens)
2. Recognizes as legacy format (tokenCount == 2)
3. Transforms to 3-part: `["0", "2", "*"]`
4. **Result**: Matches allocator=0, node=2, any service

**The `*` in 2-part format is interpreted as the SERVICE field only.**

#### 3-Part Format Behavior

When ION sees a 3-part IPN pattern like `ipn:2.*.*`:

1. Tokenizes by `.` → `["2", "*", "*"]` (3 tokens)
2. Recognizes as modern format (tokenCount == 3)
3. No transformation needed
4. **Result**: First `*` is node, second `*` is service

**Each `*` in 3-part format matches exactly one field in its position.**

#### Implementation Details

The format conversion happens in ION's EID parsing code (`bpv7/library/libbp.c`), not in BSL:

1. **BSL**: Reads policy JSON, extracts pattern strings like `"ipn:2.*"`
2. **BSL → ION**: Passes pattern string to ION's `loadEidPattern()` function
3. **ION**: Parses pattern, detects 2-part vs 3-part, performs transformation
4. **Result**: Internal representation always uses explicit 3-part format

This architectural separation ensures:
- BSL doesn't need to understand IPN allocator semantics
- Policy files can use either format
- Existing 2-part policies work indefinitely
- Format conversion is centralized in ION

#### Policy File Examples

**Example 1: Secure traffic from node 2 to node 3 (default allocator):**
```json
{
  "filter": {
    "src": "ipn:2.*",     // Matches ipn:0.2.1, ipn:0.2.5, etc.
    "dest": "ipn:3.*",    // Matches ipn:0.3.1, ipn:0.3.5, etc.
    "role": "s",
    "loc": "appin"
  }
}
```

**Example 2: Secure traffic within allocator 2 network:**
```json
{
  "filter": {
    "src": "ipn:2.*.*",   // Matches ipn:2.1.1, ipn:2.5.3, etc.
    "dest": "ipn:2.*.*",  // Matches ipn:2.3.1, ipn:2.10.5, etc.
    "role": "s",
    "loc": "appin"
  }
}
```

**Example 3: Secure all traffic from any node in default allocator:**
```json
{
  "filter": {
    "src": "ipn:0.*.*",   // Matches all nodes in allocator 0
    "dest": "ipn:3.*",    // Specific destination (allocator=0, node=3)
    "role": "v",
    "loc": "clin"
  }
}
```

**Example 4: Universal security policy (all traffic):**
```json
{
  "filter": {
    "src": "ipn:*.*.*",   // Matches any source EID
    "dest": "ipn:*.*.*",  // Matches any destination EID
    "role": "v",
    "loc": "clin"
  }
}
```

#### Migration Strategy

**Legacy systems using 2-part format:**
- Continue using `ipn:node.service` format in policies
- No changes required
- ION automatically treats as allocator=0

**New systems using allocators:**
- Use explicit 3-part format: `ipn:allocator.node.service`
- Clearly specify which allocator network policies apply to
- Use wildcards for flexible matching: `ipn:2.*.*` (all of allocator 2)

**Mixed environments:**
- Policy files can contain both formats
- Each pattern is independently parsed and interpreted
- `ipn:2.*` and `ipn:0.2.*` are equivalent (both match allocator=0, node=2)

## Testing

### Run BSL Test Suite

```bash
cd /path/to/ion-ios-dev/tests/bpsec/bpsec-all-multinode-test.bsl

# Clean up previous test runs
killm

# Run test
./dotest
```

**Expected results:**
- Tests 1-5 should pass (basic BPSec operations)
- Test 6-7 test multi-hop forwarding (may require additional configuration)

### Verify BSL Initialization

Check `ion.log` files in test directories:
```bash
grep -i "BSL init" */ion.log
```

Expected output:
```
[i] BSL initialization succeeded.
```

## Troubleshooting

### Symbol Errors: `undefined symbol: BSLX_BCB_Execute`

**Cause:** ION was not rebuilt after BSL installation, or `USING_BSL=1` was not specified.

**Solution:**
```bash
make clean
make -j4 USING_BSL=1
sudo make install
```

### Library Not Found: `cannot open shared object file: libbsl_front.so`

**Cause:** `LD_LIBRARY_PATH` not set correctly.

**Solution:**
```bash
export LD_LIBRARY_PATH=/path/to/external/BSL/testroot/usr/lib:$LD_LIBRARY_PATH
echo "export LD_LIBRARY_PATH=/path/to/external/BSL/testroot/usr/lib:\$LD_LIBRARY_PATH" >> ~/.bashrc
```

### Syntax Error at line 1746 of bpadmin.c

**Cause:** `bpadmin` was built without `USING_BSL=1`, so it doesn't recognize the `m bsl` command.

**Solution:** Rebuild ION with `USING_BSL=1` as shown in Step 3.

### Build Error: `cannot find -lbsl_crypto`

**Cause:** BSL libraries not installed or `BSL_HOME` incorrect.

**Solution:**
```bash
# Verify BSL is installed
ls external/BSL/testroot/usr/lib/libbsl_*.so

# If missing, rebuild BSL
cd external/BSL
./build-for-ion.sh
```

## Development Workflow

### Updating Makefiles

When modifying `Makefile.dev` files:

```bash
# Copy updated Makefile.dev to Makefile (skip clean to avoid permission issues)
./enable_manual_build.sh --skip-clean

# Rebuild
make -j4 USING_BSL=1
```

### Local Installation (No sudo)

For development, install to a local directory:

```bash
# Build and install locally
make -j4 USING_BSL=1
make install ROOT=$HOME/ion-local

# Update paths
export PATH=$HOME/ion-local/bin:$PATH
export LD_LIBRARY_PATH=$HOME/ion-local/lib:$LD_LIBRARY_PATH
```

Add to `~/.bashrc` for persistence.

## Supported Cipher Suites

### BIB (Block Integrity Block) - Security Context 1

- HMAC-SHA256 (variant 5)
- HMAC-SHA384 (variant 6)
- HMAC-SHA512 (variant 7)

### BCB (Block Confidentiality Block) - Security Context 2

- AES-128-GCM (variant 1)
- AES-256-GCM (variant 3)

## BSL vs Native BPSec

| Feature | BSL | Native BPSec |
|---------|-----|--------------|
| Configuration | JSON files | bpsecadmin commands |
| Key Storage | JSON Web Key (JWK) | ionsecadmin database |
| Crypto Library | OpenSSL | mbedTLS |
| Branch | feature-4.1.4-bsl | integration/main |
| Build Flag | USING_BSL=1 | USING_BSL=0 (default) |

### What Happens to Native BPSec When BSL Is Enabled

When you build ION with `USING_BSL=1` (or `--enable-bsl` with automake), the native BPSec policy framework and security processing code are **excluded from compilation**. The `-DUSING_BSL=1` preprocessor flag activates `#if USING_BSL` / `#if !USING_BSL` guards throughout the shared source code, and the build system conditionally excludes files that are not needed.

**What BSL replaces:**

| Operation | Native BPSec Function | BSL Replacement |
|-----------|----------------------|-----------------|
| Security initialization | `secAttach()` | `bslInitialize()` |
| Security shutdown | `secDetach()` | `bslCleanup()` |
| Outbound signing (BIB) | `bpsec_sign()` | `bslProcess()` at `BSL_POLICYLOCATION_APPIN` |
| Outbound encryption (BCB) | `bpsec_encrypt()` | `bslProcess()` at `BSL_POLICYLOCATION_APPOUT` |
| Inbound verification/decryption | `bpsec_verify()` / `bpsec_decrypt()` | `bslProcess()` at `BSL_POLICYLOCATION_CLIN` |
| ASB deserialization | Native deserializer in `bpextensions.c` | Set to NULL; BSL parses ASBs directly |

### BPSec Source Code Organization

The BPSec implementation is organized into layers. Some layers are shared infrastructure needed by both BSL and native BPSec, while others are native-only and excluded from BSL builds.

#### Shared infrastructure (compiled in both BSL and non-BSL builds)

These components provide the low-level BPSec data structures and extension block plumbing that both BSL and native BPSec rely on:

| Component | Source files | Purpose |
|-----------|-------------|---------|
| **ASB handling** | `bpsec_asb.c`, `bpsec_asb.h` | Abstract Security Block (ASB) creation, serialization, copy, and recording. Provides the extension block callbacks registered in `bpextensions.c` (`bpsec_asb_outboundAsbCopy`, `bpsec_asb_inboundAsbRecord`). |
| **Security utilities** | `bpsec_util.c`, `bpsec_util.h` | Extension block release/clear callbacks (`bpsec_util_outboundBlkRelease`, `bpsec_util_inboundBlkClear`), EID helpers, canonicalization, key retrieval, and block conversion functions used by security contexts. |
| **Security context interface (SCI)** | `sci.c`, `sci.h`, `sci_structs.h` | Security context registry and lookup. Defines the `sc_Def` structure that maps security context IDs to their implementations. |
| **SC value system** | `sc_value.c`, `sc_value.h`, `sci_valmap.c`, `sci_valmap.h` | Typed value containers for security parameters and results. Used by ASB serialization/deserialization and by security context implementations. |
| **SC utilities** | `sc_util.c`, `sc_util.h` | Shared helper functions used by security context implementations (key retrieval wrappers, parameter extraction). |
| **SC implementations** | `bcb_aes_gcm_sc.c`, `bib_hmac_sha2_sc.c`, `ion_test_sc.c`, `rfc9173_utils.c` | Concrete security context implementations (AES-GCM, HMAC-SHA2, test SC). Registered in the static `gScDefs[]` table in `sci.c`. |
| **Extension block registration** | `bpextensions.c` | BIB/BCB block type entries in the extension table. Deserialize callbacks are set to NULL when BSL is active; other callbacks (copy, record, release, clear) remain. |
| **BSL wrapper code** | `bsl.c`, `ionpatch.c` | BSL initialization, `bslProcess()` dispatch, and ION memory allocator bridge. Only compiled when BSL is enabled. |

> **Why SCI and the SC implementations remain:** `bpsec_asb.c` depends on `sc_value.c` for typed value serialization, `sc_value.c` depends on `sci_valmap.c` for value map lookups and `sci.c` for context definition lookups, and `sci.c` statically references function pointers from all three SC implementations. This dependency chain means the entire SCI layer must be compiled even in BSL builds. The SC implementation functions are not called at BSL runtime (BSL uses its own crypto via OpenSSL), but they compile cleanly and are linked as dead code.

#### Native BPSec only (excluded from BSL builds)

These components implement the native BPSec security processing pipeline and policy engine. They are not compiled when `USING_BSL=1`:

| Component | Source files | Purpose |
|-----------|-------------|---------|
| **Policy engine** | `bpsec_policy.c`, `bpsec_policy_rule.c`, `bpsec_policy_event.c`, `bpsec_policy_eventset.c` | ION's native policy rule matching, event handling, and event set management. BSL replaces this with its own JSON-based policy engine. |
| **BIB processing** | `bib.c`, `bib.h` | `bpsec_sign()` and `bpsec_verify()` — native integrity operations using the SCI/mbedTLS stack. |
| **BCB processing** | `bcb.c`, `bcb.h` | `bpsec_encrypt()` and `bpsec_decrypt()` — native confidentiality operations using the SCI/mbedTLS stack. |
| **Instrumentation** | `bpsec_instr.c` | BPSec statistics counters (`ADD_SRC_BYTES`, `ADD_RCV_BYTES`, etc.) only incremented by native BIB/BCB processing. |
| **`bpsecadmin` utility** | `bpsecadmin.c`, `bpsecadmin_config.c` | CLI for managing native policy rules and event sets. Not built when BSL is active — use `m bsl` in `bpadmin` instead. |

**Key source files involved in the build toggle:**

- `bpv7/library/bpP.h` — Includes `bsl.h` and adds BSL config fields to `BpDB` when `USING_BSL=1`
- `bpv7/library/libbpP.c` — Contains the main `#if USING_BSL` guards for init, shutdown, send-side and receive-side security processing
- `bpv7/bpsec/utils/bpsec_util.h` — Guards policy header includes and native-only function declarations
- `bpv7/library/ext/bpextensions.c` — Nulls out BIB/BCB ASB deserialization callbacks when BSL is active
- `ici/include/ionsec.h` — Policy fields in `SecDB`/`SecVdb` are conditionally compiled out
- `Makefile.am` / `Makefile.dev` — Conditionally exclude native BPSec sources and `bpsecadmin`

**Practical implications:**

- The resulting `libbp.so` is **smaller** with BSL enabled (native policy, instrumentation, and BIB/BCB processing code are excluded)
- `bpsecadmin` is **not built** when BSL is enabled — use BSL's JSON policy files configured via `m bsl` in `bpadmin` instead
- The `bpadmin` utility gains the `m bsl` command for BSL-specific configuration
- Switching between BSL and native BPSec requires a **full rebuild** (`make clean && make`) with the appropriate `USING_BSL` flag — it is not a runtime toggle

---

## Appendices

### Appendix A: Detailed Prerequisites

#### Dependency Summary

| Package | Purpose | Version |
|---------|---------|---------|
| `gcc` or `clang` | C/C++ compiler | 11.0+ (13.0+ for BSL tests) |
| `cmake` | Build system generator | 3.20+ |
| `pkg-config` | Library discovery for CMake | any |
| `libssl-dev` / `openssl-devel` | Cryptographic functions (AES, HMAC) | 3.0+ |
| `libjansson-dev` / `jansson-devel` | JSON parsing for policy/keys | 2.13+ |
| `valgrind` | Memory leak detection in tests (optional) | 3.18+ |
| `ruby` | Unity test framework code generation (optional) | 3.0+ |
| `ninja-build` | Fast parallel build (optional) | 1.10+ |

!!! note "GCC Version Notes"
    - **GCC 11/12**: Can build all BSL libraries but test code may fail with `-Werror`
    - **GCC 13+**: Required for building BSL unit tests
    - For production use, GCC 11+ is sufficient

### Appendix B: Manual BSL Build (Advanced)

If you need more control than `build-for-ion.sh` provides:

!!! warning "Prerequisite"
    ION must be built and installed **before** building BSL manually. BSL's `bsl_ionpatch` library links against ION's `libici.so`. See [Step 1](#step-1-build-ion-without-bsl) first.

#### Build Dependencies

```bash
cd external/BSL
./build.sh clean
./build.sh deps
```

This builds:
- **QCBOR**: Shared library for CBOR encoding/decoding
- **mlib**: Header-only container library
- **Unity**: Static library for unit testing

#### Configure BSL

```bash
./build.sh prep \
    -DION_INTEGRATION=ON \
    -DION_ROOT=/path/to/ion-ios-dev \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=$PWD/testroot/usr \
    -DBUILD_TESTING=OFF
```

**Configuration Options:**
- `-DION_INTEGRATION=ON` - Enable ION memory allocator integration
- `-DION_ROOT=/path/to/ion` - Path to ION source directory
- `-DCMAKE_BUILD_TYPE=Release` - Optimized build (use `Debug` for development)
- `-DCMAKE_INSTALL_PREFIX` - Installation directory
- `-DBUILD_TESTING=OFF` - Skip unit tests (recommended with GCC < 13)

#### Build BSL

```bash
./build.sh

# Or manually:
cd build/default
ninja -j$(nproc)  # or: make -j$(nproc)
```

#### Install BSL

```bash
cd build/default
cmake --install . --prefix $PWD/../../testroot/usr
```

!!! note "GCC < 13 Installation"
    With GCC 11/12, `ninja install` may fail trying to build test utilities. Use `cmake --install` instead, which installs already-built targets without rebuilding tests.

#### Verify Installation

```bash
cd ../..  # back to BSL root

# Check libraries
ls -la testroot/usr/lib/libbsl_*.so

# Check headers
ls -la testroot/usr/include/bsl/
```

### Appendix C: System-Wide BSL Installation

To install BSL system-wide instead of locally:

```bash
cd external/BSL

# Configure with system prefix
./build.sh prep \
    -DION_INTEGRATION=ON \
    -DION_ROOT=/path/to/ion-ios-dev \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DBUILD_TESTING=OFF

# Build
./build.sh

# Install (requires sudo)
cd build/default
sudo cmake --install . --prefix /usr/local
sudo ldconfig

# No LD_LIBRARY_PATH configuration needed
```

**Advantages:**
- No `LD_LIBRARY_PATH` configuration required
- Libraries available to all users
- Standard system location

**Disadvantages:**
- Requires sudo for installation
- System-wide changes
- Potential conflicts with other software

### Appendix D: Running BSL Unit Tests

BSL unit tests require GCC 13+ to compile.

```bash
cd external/BSL

# Configure with tests enabled
./build.sh prep \
    -DION_INTEGRATION=ON \
    -DBUILD_TESTING=ON

# Build
./build.sh

# Run tests
./build.sh check

# Or manually:
cd build/default
ctest --output-on-failure
```

**Expected output (GCC 13+):**
```
Test project /path/to/BSL/build/default
100% tests passed, 0 tests failed out of 11
```

### Appendix E: BSL Key and Policy File Examples

#### Complete Key File Example

```json
{
  "keys": [
    {
      "kty": "oct",
      "kid": "hmac-key-256",
      "k": "cXdlcnR5dWlvcGFzZGZnaGprbHp4Y3Zibm0="
    },
    {
      "kty": "oct",
      "kid": "aes-gcm-key-128",
      "k": "YWJjZGVmZ2hpamtsbW5vcA=="
    }
  ]
}
```

Where:
- `kty`: Key type (`oct` for symmetric keys)
- `kid`: Key identifier (referenced in policy)
- `k`: Base64-encoded key material

#### Complete Policy File Example

```json
{
  "policyrule_set": [
    {
      "policyrule": {
        "filter": {
          "rule_id": "1",
          "role": "s",
          "src": "ipn:2.*",
          "dest": "ipn:3.*",
          "tgt": 1,
          "loc": "appin",
          "sc_id": 1
        },
        "spec": {
          "sc_id": 1,
          "sc_parms": [
            {"id": "key_name", "value": "hmac-key-256"},
            {"id": "sha_variant", "value": "5"},
            {"id": "scope_flags", "value": "7"},
            {"id": "key_wrap", "value": "0"}
          ]
        },
        "_temp_not_ion_spec_policy_action_on_fail": "delete_bundle"
      }
    },
    {
      "policyrule": {
        "filter": {
          "rule_id": "2",
          "role": "s",
          "src": "ipn:2.*",
          "dest": "ipn:3.*",
          "tgt": 1,
          "loc": "appin",
          "sc_id": 2
        },
        "spec": {
          "sc_id": 2,
          "sc_parms": [
            {"id": "key_name", "value": "aes-gcm-key-128"},
            {"id": "aes_variant", "value": "1"},
            {"id": "scope_flags", "value": "7"},
            {"id": "key_wrap", "value": "0"}
          ]
        }
      }
    }
  ]
}
```

**Policy Parameters:**
- `rule_id`: Unique identifier for the rule
- `role`: `s` (source), `v` (verifier), or `a` (acceptor)
- `src`: Source EID pattern
- `dest`: Destination EID pattern
- `tgt`: Target block type (1 = payload)
- `loc`: Location (`appin`, `appout`, etc.)
- `sc_id`: Security context ID (1 = BIB, 2 = BCB)

**Security Context Parameters (BIB):**
- `key_name`: Key identifier from key file
- `sha_variant`: `5` (SHA256), `6` (SHA384), `7` (SHA512)
- `scope_flags`: Bitmask for protected bundle parts
- `key_wrap`: `0` (no wrap), `1` (wrap)

**Security Context Parameters (BCB):**
- `key_name`: Key identifier from key file
- `aes_variant`: `1` (AES-128-GCM), `3` (AES-256-GCM)
- `scope_flags`: Bitmask for protected bundle parts
- `key_wrap`: `0` (no wrap), `1` (wrap)

#### Validating JSON Files

```bash
# Check JSON syntax
python3 -m json.tool keys.json
python3 -m json.tool policy.json
```

### Appendix F: Uninstalling BSL

BSL does not provide an uninstall target. To remove:

**Local installation (testroot):**
```bash
cd external/BSL
rm -rf testroot/usr
rm -rf build
```

**System-wide installation:**
```bash
sudo rm -f /usr/local/lib/libbsl_*.so*
sudo rm -f /usr/local/lib/libqcbor.so*
sudo rm -rf /usr/local/include/bsl
sudo ldconfig
```

Or use ION's uninstall script:
```bash
cd /path/to/ion-ios-dev
./uninstall_bsl.sh
```

## Additional Resources

- **BSL Repository**: https://github.com/iondev33/BSL (branch: `bsl-ion-integration`)
- **RFC 9172**: Bundle Protocol Security (BPSec)
- **RFC 9173**: Default Security Contexts for BPSec
- **ION Documentation**: https://ion-dtn.readthedocs.io
