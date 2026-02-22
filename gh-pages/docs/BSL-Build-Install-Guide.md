# BSL Build and Install Guide

!!! note "Feature Branch"
    BSL integration is available on the `feature-4.1.4-bsl` branch. This guide provides instructions for building the Bundle Protocol Security Library (BSL) and integrating it with ION.

## What is BSL?

BSL (Bundle Protocol Security Library) is an external BPSec implementation that ION can link to as an alternative to ION's native BPSec. BSL is integrated as a git submodule at `external/BSL/`.

## Prerequisites and Dependencies

### System Requirements

- **Operating System**: Linux (Ubuntu 22.04+, RHEL 9, or compatible)
- **Compiler**: GCC 11+ (GCC 13+ recommended for BSL unit tests; GCC 11 builds all libraries but test code triggers `-Werror` on unused variables)
- **Build Tools**: CMake 3.20+, Make/Ninja
- **Git**: For submodule management

### Required System Packages

#### Ubuntu/Debian

```bash
sudo apt install -y \
    build-essential \
    cmake \
    pkg-config \
    git \
    libssl-dev \
    libjansson-dev \
    valgrind \
    ruby \
    ninja-build
```

#### RHEL/Fedora/CentOS

```bash
sudo dnf install -y \
    gcc \
    gcc-c++ \
    cmake \
    pkg-config \
    git \
    openssl-devel \
    jansson-devel \
    valgrind \
    ruby \
    ninja-build
```

### Dependency Summary

| Package | Purpose | Version |
|---------|---------|---------|
| `gcc` or `clang` | C/C++ compiler | 11.0+ (13.0+ for tests) |
| `cmake` | Build system generator | 3.20+ |
| `pkg-config` | Library discovery for CMake | any |
| `libssl-dev` / `openssl-devel` | Cryptographic functions (AES, HMAC) | 3.0+ |
| `libjansson-dev` / `jansson-devel` | JSON parsing for policy/keys | 2.13+ |
| `valgrind` | Memory leak detection in tests | 3.18+ |
| `ruby` | Unity test framework code generation | 3.0+ |
| `ninja-build` | Fast parallel build (optional, but `build.sh` may default to Ninja) | 1.10+ |

## Part 1: Building BSL

### Step 1: Navigate to BSL Directory

The BSL repository is defined as a git submodule at `external/BSL/` in `.gitmodules`.

**Important:** The submodule entry may exist in `.gitmodules` but not yet be registered in the git index. If `git submodule update --init external/BSL` fails with `error: pathspec 'external/BSL' did not match any file(s) known to git`, clone BSL manually:

```bash
cd /path/to/ion-ios-dev
mkdir -p external
git clone https://github.com/iondev33/BSL.git external/BSL
cd external/BSL
```

The iondev33/BSL repository is a fork of the original BSL with some modifications for ION. If the submodule is properly registered, the standard approach works:

```bash
cd /path/to/ion-ios-dev
git submodule update --init external/BSL
cd external/BSL
```

### Step 2: Initialize BSL Submodules

BSL depends on several third-party libraries as submodules:

- **QCBOR**: CBOR encoding/decoding library
- **mlib**: M*LIB container library for C
- **Unity**: Unit testing framework

Initialize these submodules (run from within the `external/BSL` directory):

```bash
git submodule update --init --recursive
```

**Expected output:**
```
Submodule 'deps/QCBOR' (https://github.com/laurencelundblade/QCBOR.git) registered
Submodule 'deps/mlib' (https://github.com/P-p-H-d/mlib.git) registered
Submodule 'deps/unity' (https://github.com/ThrowTheSwitch/Unity.git) registered
Cloning into 'deps/QCBOR'...
Cloning into 'deps/mlib'...
Cloning into 'deps/unity'...
```

**Verify submodules:**
```bash
ls -la deps/
# Should show: QCBOR/, mlib/, unity/
```

### Step 3: Build BSL Dependencies

It's good to clean up everything before building:
```bash
./build.sh clean
```
Use `sudo` if previously built with root permissions. This will remove the build directory and installed files in `testroot/usr/` to ensure a clean state for the new build.

BSL provides a `build.sh` script that wraps CMake commands. First, build the dependencies:

```bash
./build.sh deps
```

This will:

1. Build QCBOR as a shared library
2. Install mlib headers (header-only library)
3. Build Unity test framework as a static library

All dependencies are installed to `testroot/usr/` directory within the BSL tree.

**Expected output:**
```
Building QCBOR...
[100%] Built target qcbor
Installing QCBOR to testroot/usr/

Building MLIB...
Installing MLIB headers to testroot/usr/include/m-lib/

Building Unity...
[100%] Built target unity
Installing Unity to testroot/usr/
```

### Step 4: Configure BSL Build

Configure BSL for Release build (optimized for production):

```bash
./build.sh prep -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$PWD/testroot/usr
```

OR set the PREFIX to /usr/local for system-wide install:
```bash
./build.sh prep -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local
```

To skip building tests (if using GCC 11/12):
```bash
./build.sh prep -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$PWD/testroot/usr -DBUILD_TESTING=OFF
```

**Build configuration options:**

- `-DCMAKE_BUILD_TYPE=Release`: Optimized build (use `Debug` for development)
- `-DCMAKE_INSTALL_PREFIX=$PWD/testroot/usr`: Install location

!!! note "Build System"
    `build.sh prep` may default to generating Ninja build files (not Makefiles). Check the output for `-- Build files have been written to: build/default`. If Ninja files are generated, use `ninja` instead of `make` in subsequent steps.

**Expected output:**
```
-- Using version marking 1.0.0 - 8.ga99ed4a
-- Using valgrind memcheck for tests: /usr/bin/valgrind
-- Searching for Unity tools in deps/unity
-- Found unity at testroot/usr/lib/cmake/unity
-- Adding unit test test_MockBPA_text_util
-- Adding unit test test_CryptoInterface
-- Adding unit test test_DefaultSecurityContext
...
-- Configuring done
-- Generating done
-- Build files have been written to: build/default
```

### Step 5: Build BSL

Check `build/default/` for `build.ninja` (Ninja) or `Makefile` to determine which build tool to use:

```bash
# If build.ninja exists (default):
cd build/default
ninja -j$(nproc)

# If Makefile exists:
cd build/default
make -j$(nproc)
```

Or use the build script wrapper (auto-detects build system):
```bash
cd ../../  # back to BSL root
./build.sh  # equivalent to: cd build/default && ninja/make
```

!!! warning "GCC 11/12 Note"
    BSL libraries will build successfully, but test code (`test/bsl_test_utils.c`) will fail with `-Werror` due to unused variables/parameters. This is expected with GCC < 13. The library targets still complete—only test binaries are affected.

To build only the libraries (skipping test targets):
```bash
cd build/default
ninja bsl_front bsl_dynamic bsl_crypto bsl_default_sc bsl_sample_pp bsl_mock_bpa
```

### Quick Build Summary

For GCC 11/12 users who want to skip tests and build just the libraries:

```bash
cd external/BSL
./build.sh deps
./build.sh prep -DBUILD_TESTING=OFF  # add type and prefix options if needed
./build.sh
./build.sh install
```

### Step 6: Run BSL Unit Tests (Optional, requires GCC 13+)

Verify BSL works correctly by running unit tests:

```bash
./build.sh check
```

Or manually:
```bash
cd build/default
ctest --output-on-failure
```

!!! note
    BSL unit tests require GCC 13+ to compile due to strict `-Werror` settings. With GCC 11, the test utility library fails to build. The core BSL libraries are unaffected.

**Expected output (GCC 13+):**
```
Test project /path/to/BSL/build/default
    Start  1: test_MockBPA_text_util
1/11 Test  #1: test_MockBPA_text_util ............   Passed    0.05 sec
    Start  2: test_MockBPA_EID
2/11 Test  #2: test_MockBPA_EID ..................   Passed    0.03 sec
    Start  3: test_CryptoInterface
3/11 Test  #3: test_CryptoInterface ..............   Passed    0.08 sec
...
100% tests passed, 0 tests failed out of 11
```

### Step 7: Install BSL

Install BSL libraries and headers to the configured prefix. Use `cmake --install` which installs already-built targets without attempting to rebuild failed test code:

```bash
cd build/default
cmake --install . --prefix $PWD/../../testroot/usr
```

OR use /usr/local for system-wide install:
```bash
cd build/default
cmake --install . --prefix /usr/local
```

**Important Notes:**

1. **GCC < 13 Compatibility:** The `ninja install` or `make install` commands will attempt to build ALL targets first, including test utilities. With GCC < 13, test code (`test/bsl_test_utils.c`) fails with `-Werror` due to unused variables. Using `cmake --install` directly bypasses the build step and installs only the already-compiled targets.

2. **Expected Error:** The install will report an error at the end:
   ```
   CMake Error: file INSTALL cannot find "libbsl_test_utils.so.1.0.0": No such file or directory
   ```
   This is expected with GCC < 13 and can be ignored. All production libraries and headers install successfully despite this error.

3. **What Gets Installed:** The `cmake --install` command successfully installs:
   - All 6 BSL production libraries (libbsl_*.so)
   - All BSL headers including generated BSLConfig.h
   - QCBOR library (libqcbor.so)
   - Unity static library (libunity.a)

If all targets built successfully (GCC 13+):
```bash
cd ../..  # back to BSL root
./build.sh install
```

**Installed files in `testroot/usr/`:**

**Libraries** (`lib/` on Ubuntu/Debian, `lib64/` on RHEL/Fedora):
```
libbsl_crypto.so          - Cryptographic interface (AES-GCM, HMAC-SHA2)
libbsl_default_sc.so      - RFC 9173 default security contexts
libbsl_dynamic.so         - Dynamic backend implementation
libbsl_front.so           - BSL front-end API
libbsl_mock_bpa.so        - Mock BPA for testing
libbsl_sample_pp.so       - Sample policy provider
libqcbor.so               - CBOR encoding/decoding
libunity.a                - Unity test framework
```

**Headers** (`include/bsl/`):
```
BPSecLib_Public.h         - Public BSL API
BPSecLib_Private.h        - Private BSL API (also in src/ source directory)
BSLConfig.h               - Generated config header (build-time generated)
CryptoInterface.h         - Crypto interface definitions
Data.h                    - Data structure utilities
policy_provider/          - Policy provider headers
security_context/         - Security context headers
backend/                  - Backend implementation headers
mock_bpa/                 - Mock BPA headers
```

**Verify installation:**
```bash
# Check library directory (lib/ on Ubuntu, lib64/ on RHEL)
ls -la testroot/usr/lib/libbsl_*.so    # Ubuntu/Debian
ls -la testroot/usr/lib64/libbsl_*.so  # RHEL/Fedora

# Verify headers are installed
ls -la testroot/usr/include/bsl/BPSecLib_Public.h
ls -la testroot/usr/include/bsl/BPSecLib_Private.h
ls -la testroot/usr/include/bsl/BSLConfig.h
```

### Runtime Library Configuration

If you install BSL to the default prefix (`testroot/usr`) or a non-system-wide location, you need to configure the dynamic linker to find the BSL shared libraries at runtime:

**Option 1: Set LD_LIBRARY_PATH**
```bash
export LD_LIBRARY_PATH=/path/to/external/BSL/testroot/usr/lib:$LD_LIBRARY_PATH
```
Add to `.bashrc` for persistence.

**Option 2: Configure system linker (persistent)**
```bash
echo "/path/to/external/BSL/testroot/usr/lib" | sudo tee /etc/ld.so.conf.d/bsl.conf
sudo ldconfig
```

### Uninstalling BSL

BSL library has no "uninstall" target. To remove a previous installation, use the `./uninstall_bsl.sh` script in the ION root directory, or manually delete the installed files from the prefix directory (e.g., `testroot/usr/` or `/usr/local/`).

## Part 2: Building ION with BSL

### Step 1: Update ION Submodule Reference (if needed)

If you cloned BSL manually, you may need to update the submodule reference in ION's `.gitmodules` and git index to point to the correct repository and commit. This ensures that future `git submodule update` commands work correctly for other developers.

### Step 2: Configure ION Build to Use BSL

The only way to build ION with BSL is to set `BSL_HOME` in the Makefile.dev files to point to the BSL installation prefix (e.g., `testroot/usr/` or `/usr/local/`). This allows ION's build system to find the BSL headers and libraries during compilation and linking.

By default, `BSL_HOME` is set to `external/BSL/testroot/usr/` in the Makefile.dev files, which matches the default install prefix used in the BSL build instructions.

If you installed BSL to a different location, pass the correct path to ION's build system:

```bash
make BSL_HOME=/path/to/your/bsl/installation
```

### Step 3: Build ION

Build ION using the `.dev` Makefile system:

```bash
cd bpv7

# Build using the .dev Makefile
gmake -f Makefile.dev clean
gmake -f Makefile.dev all

# Install
sudo gmake -f Makefile.dev install ROOT=/usr/local
```

The `-DUSING_BSL=1` flag is already set in the Makefile, which enables BSL code paths.

## Configuring BSL in ION

### Create Key Files

BSL uses JSON Web Key (JWK) format for key storage. Create a key file:

**Example**: `keys.json`
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

Where `k` is the base64-encoded key material.

### Create Policy Files

BSL uses JSON for policy configuration. Create a policy file:

**Example**: `policy.json`
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
            {
              "id": "key_name",
              "value": "myHmacKey"
            },
            {
              "id": "sha_variant",
              "value": "5"
            },
            {
              "id": "scope_flags",
              "value": "7"
            },
            {
              "id": "key_wrap",
              "value": "0"
            }
          ]
        },
        "_temp_not_ion_spec_policy_action_on_fail": "delete_bundle"
      }
    }
  ]
}
```

### Configure BSL in bprc

In your `bprc` configuration file, add the BSL configuration command:

```
# Initialize BSL with security EID, key file, and policy file
m bsl 'ipn:2.0' '/path/to/keys.json' '/path/to/policy.json'
```

**Parameters:**

1. Security source EID (e.g., `ipn:2.0`)
2. Path to key registry JSON file
3. Path to policy configuration JSON file

### Example bprc

```bash
# Standard BP configuration
1
a scheme ipn 'ipnfw' 'ipnadminep'
a endpoint ipn:2.0 x
a endpoint ipn:2.1 x
a protocol ltp 1400 100
a induct ltp 2 ltpcli
a outduct ltp 3 ltpclo

# Configure BSL
m bsl 'ipn:2.0' '/etc/ion/bsl_keys.json' '/etc/ion/bsl_policy.json'

# Start IPN admin
r 'ipnadmin ipn.ipnrc'
w 1
s
```

## Verifying the Installation

### Check BSL Libraries

Verify BSL libraries are installed:

```bash
ls -la /usr/local/lib64/libbsl_*
# Should show:
# libbsl_crypto.so
# libbsl_default_sc.so
# libbsl_dynamic.so
# libbsl_front.so
# libbsl_mock_bpa.so
# libbsl_sample_pp.so
```

### Check BSL Headers

Verify BSL headers are installed:

```bash
ls -la /usr/local/include/bsl/
# Should show:
# BPSecLib_Private.h
# BPSecLib_Public.h
# CryptoInterface.h
# (and subdirectories: policy_provider, security_context, ION_integration)
```

### Test ION with BSL

Run the BSL test suite included with ION:

```bash
cd tests/bpsec/bpsec-all-multinode-test.bsl
./dotest
```

## Troubleshooting

### Library Not Found Errors

If you get errors like `libbsl_front.so: cannot open shared object file`:

```bash
# Add BSL library path to LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/usr/local/lib64:$LD_LIBRARY_PATH

# Or add to /etc/ld.so.conf.d/
echo "/usr/local/lib64" | sudo tee /etc/ld.so.conf.d/bsl.conf
sudo ldconfig
```

### Compilation Errors

If BSL headers are not found during ION compilation:

1. Verify BSL is installed: `ls /usr/local/include/bsl/BPSecLib_Public.h`
2. Check Makefile paths in `bpv7/x86_64-linux/Makefile.dev`
3. Ensure `BSL_INCLUDE` points to the correct location

### BSL Initialization Fails

If ION fails with `[?] BSL init can't find SDR`:

1. Ensure `ionadmin` is run before `bpadmin`
2. Check that key and policy files exist and are readable
3. Verify JSON syntax in key and policy files

### Key/Policy File Errors

Validate JSON files:

```bash
# Check JSON syntax
python3 -m json.tool keys.json
python3 -m json.tool policy.json
```

## Cipher Suites Supported

### BIB (Block Integrity Block) - Security Context 1

- **HMAC-SHA256** (variant 5)
- **HMAC-SHA384** (variant 6)
- **HMAC-SHA512** (variant 7)

### BCB (Block Confidentiality Block) - Security Context 2

- **AES-128-GCM** (variant 1)
- **AES-256-GCM** (variant 3)

## BSL vs Native BPSec

| Feature | BSL | Native BPSec |
|---------|-----|--------------|
| Configuration | JSON files | bpsecadmin commands |
| Key Storage | JSON Web Key (JWK) | ionsecadmin database |
| Crypto Library | OpenSSL | mbedTLS |
| Branch | feature-4.1.4-bsl | integration/main |

## Additional Resources

- **BSL Repository**: https://github.com/iondev33/BSL
- **RFC 9172**: Bundle Protocol Security (BPSec)
- **RFC 9173**: Default Security Contexts for BPSec
