# BSL Build and Install Guide

## What is BSL?

BSL (Bundle Protocol Security Library) is an external BPSec implementation that ION can use as an alternative to ION's built-in BPSec. BSL v1.1 implements RFC 9172/9173 security contexts, uses OpenSSL for cryptography, and is configured through JSON files rather than `bpsecadmin` commands.

BSL is integrated as a git submodule at `external/BSL/`. When enabled, it replaces ION's native BPSec policy engine and security processing.

BSL v1.1 uses runtime dynamic memory callbacks instead of compile-time integration, so BSL builds as a standalone library with no ION dependencies. ION's `ionpatch.c` registers ION memory allocation callbacks at runtime.

## Prerequisites

### Required System Packages

**Ubuntu/Debian:**
```bash
sudo apt install -y build-essential automake autoconf libtool m4 \
    cmake pkg-config git libssl-dev libjansson-dev ninja-build
```

**RHEL/Fedora/CentOS:**
```bash
sudo dnf install -y gcc gcc-c++ automake autoconf libtool m4 \
    cmake pkg-config git openssl-devel jansson-devel ninja-build
```

See [Appendix A](#appendix-a-detailed-prerequisites) for complete dependency information.

## Build Instructions

BSL v1.1 builds as a standalone library (no ION dependency), then ION links against BSL. The automake build system handles this automatically.

### Step 1: Initialize the BSL Submodule

```bash
cd /path/to/ion-ios-dev

# Clone BSL submodule
git submodule update --init external/BSL

# Initialize BSL's own dependencies (QCBOR, mlib, Unity)
cd external/BSL
git submodule update --init --recursive
cd ../..
```

### Step 2: Build BSL

BSL must be built **before** running `./configure --enable-bsl`, because configure resolves `BSL_HOME` to an absolute path and verifies headers exist.

```bash
./build-bsl-for-ion.sh
```

Verify it completed:
```bash
ls external/BSL/testroot/usr/include/m-lib/m-bstring.h
ls external/BSL/testroot/usr/lib/libbsl_front.so
```

**When to rebuild BSL:**

```bash
./build-bsl-for-ion.sh clean
./build-bsl-for-ion.sh
```

Rebuild BSL when:
- The BSL submodule is updated (`git submodule update external/BSL`)
- BSL's dependencies change (QCBOR, mlib, Unity versions bumped)
- Switching platforms or compilers (e.g., different GCC version)
- The `testroot/` directory is missing or corrupted (headers or libraries absent)
- You see build errors referencing missing BSL headers (`m-bstring.h`, `bsl_front.h`, `qcbor.h`)

You do **not** need to rebuild BSL when:
- Only ION source files change (rerun `make` only)
- Reconfiguring ION with different options unrelated to BSL
- Running tests

### Step 3: Build and Install ION with BSL

```bash
# Generate configure script (if not already present)
autoreconf -fi

# Configure with BSL enabled (BSL_HOME must already exist)
./configure --enable-bsl

# Build
make -j$(nproc)

# Install
sudo make install
sudo ldconfig
```

The `--enable-bsl` flag:
- Sets `-DUSING_BSL=1` for the entire build
- Looks for the BSL submodule at `external/BSL/`
- Defaults `BSL_HOME` to `external/BSL/testroot/usr`
- Adds BSL libraries (`libbsl_front`, `libbsl_crypto`, etc.) to the link

### Verify the Build

```bash
# Check BSL libraries are linked
ldd /usr/local/lib/libbp.so | grep bsl

# Check bpadmin has BSL support
strings /usr/local/bin/bpadmin | grep "m bsl"
```

### Custom BSL_HOME

If BSL is installed somewhere other than the default submodule location:

```bash
./configure --enable-bsl BSL_HOME=/custom/path/to/bsl
```

## Configuration

### Runtime Setup

BSL uses JSON files for keys and policies. Configure them in your `.bprc` file:

```
m bsl 'ipn:2.0' '/path/to/keys.json' '/path/to/policy.json'
```

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

### EID Pattern Matching and Wildcards

BSL policy files support wildcard patterns for matching source and destination EIDs. Understanding how 2-part and 3-part IPN formats interact with wildcards is essential for writing correct security policies.

#### IPN Format

ION supports two IPN endpoint naming formats:

- **2-part (legacy)**: `ipn:node.service` (e.g., `ipn:2.1`)
  - Assumes allocator = 0
- **3-part (modern)**: `ipn:allocator.node.service` (e.g., `ipn:0.2.1`)
  - Explicitly specifies allocator

ION automatically converts 2-part to 3-part internally, so `ipn:2.1` becomes `ipn:0.2.1`. Existing 2-part policies continue to work.

#### Wildcard Behavior

The `*` wildcard matches any value in its position:

| Pattern | Interpretation | Matches |
|---------|----------------|---------|
| `ipn:2.*` | allocator=0, node=2, service=any | `ipn:0.2.1`, `ipn:0.2.5` |
| `ipn:0.2.*` | allocator=0, node=2, service=any | Same as above |
| `ipn:2.*.*` | allocator=2, node=any, service=any | `ipn:2.1.1`, `ipn:2.3.5` |
| `ipn:0.*.*` | allocator=0, node=any, service=any | All default-allocator EIDs |
| `ipn:*.*.*` | any | All IPN EIDs |

2-part `ipn:2.*` is automatically expanded to 3-part `ipn:0.2.*` — the `*` applies to the service field only. In 3-part format, each `*` matches exactly the field in its position.

## Supported Cipher Suites

### BIB (Block Integrity Block) - Security Context 1

- HMAC-SHA256 (variant 5)
- HMAC-SHA384 (variant 6)
- HMAC-SHA512 (variant 7)

### BCB (Block Confidentiality Block) - Security Context 2

- AES-128-GCM (variant 1)
- AES-256-GCM (variant 3)

## BIB and BCB Policy Design (RFC 9172 Guidelines)

When writing BSL policy files, the relationship between BIB (integrity) and BCB (confidentiality) blocks must follow RFC 9172 rules.

### Same Source, Same Target: Use a Single BCB

Per RFC 9172 Section 4.7, if a single security source applies **both** integrity and confidentiality to the **same target block**, it **must** use a single BCB whose security context provides integrity as an additional result — not separate BIB and BCB blocks.

Applying separate BIB and BCB to the same target from the same node creates an ordering problem: the BIB computes its HMAC over plaintext, then the BCB encrypts the payload. A verifier in transit cannot verify the BIB because the payload is ciphertext.

### Different Targets: BIB on Primary, BCB on Payload

When the same node needs both integrity and confidentiality, apply each to a **different** target block:

- **BIB** targets the **primary block** (`tgt: 0`)
- **BCB** targets the **payload block** (`tgt: 1`)

```json
{
  "policyrule_set": [
    {
      "policyrule": {
        "desc": "BIB on primary block",
        "filter": { "role": "s", "src": "ipn:2.*", "dest": "ipn:3.*", "tgt": 0, "sc_id": 1 },
        "spec": {
          "sc_id": 1,
          "sc_parms": [
            {"id": "key_name", "value": "9100"},
            {"id": "sha_variant", "value": "7"},
            {"id": "scope_flags", "value": "0"},
            {"id": "key_wrap", "value": "0"}
          ]
        }
      }
    },
    {
      "policyrule": {
        "desc": "BCB on payload block",
        "filter": { "role": "s", "src": "ipn:2.*", "dest": "ipn:3.*", "tgt": 1, "sc_id": 2 },
        "spec": {
          "sc_id": 2,
          "sc_parms": [
            {"id": "key_name", "value": "9103"},
            {"id": "aes_variant", "value": "1"},
            {"id": "aad_scope", "value": "0"},
            {"id": "key_wrap", "value": "1"}
          ]
        }
      }
    }
  ]
}
```

### Different Sources: BIB and BCB on Same Target Is Allowed

RFC 9172 permits separate BIB and BCB on the same target when applied by **different nodes** (e.g., Node A adds a BIB, Node B adds a BCB on the same payload). This is valid because the nodes act as independent security sources.

### Security Roles

| Role | Code | Location | Description |
|------|------|----------|-------------|
| **Source** | `s` | `appin` | Creates the security block |
| **Verifier** | `v` | `clin` | Checks the block in transit without consuming it |
| **Acceptor** | `a` | `appout` | Processes and removes the block at delivery |

A BIB verifier cannot operate at CLIN when a BCB is present on the same target, because the BIB was computed over plaintext but the payload is still encrypted. When BIB and BCB target different blocks (the recommended approach), this does not apply.

### Multinode Policy Example

```
Node 2 (source) → Node 3 (relay) → Node 4 (destination)

Node 2: BIB source on primary (tgt 0), BCB source on payload (tgt 1)
Node 3: BCB verifier at CLIN for payload (validates ASB, no decrypt)
Node 4: BCB verifier at CLIN, BIB acceptor at APPOUT, BCB acceptor at APPOUT
```

See `tests/bpsec/bpsec-all-multinode-test.bsl/` for complete working policy files.

## Testing

```bash
cd /path/to/ion-ios-dev/tests/bpsec/bpsec-all-multinode-test.bsl

killm
./dotest
```

Tests 1-5 cover basic BPSec operations. Check `ion.log` for `BSL initialization succeeded`.

## BSL vs Native BPSec

| Feature | BSL | Native BPSec |
|---------|-----|--------------|
| Configuration | JSON files (`m bsl` in bprc) | `bpsecadmin` commands |
| Key Storage | JSON Web Key (JWK) | `ionsecadmin` database |
| Crypto Library | OpenSSL | mbedTLS |
| Build Flag | `--enable-bsl` | Default (no flag) |

Switching between BSL and native BPSec requires a full rebuild:
```bash
make clean
./configure          # native BPSec (default)
./configure --enable-bsl  # BSL
make -j$(nproc)
```

## Building on Solaris

BSL can be built on Solaris 11 with some additional setup. The `build-bsl-for-ion.sh` script handles most Solaris-specific adjustments automatically, but the environment must be prepared first.

### Prerequisites (Solaris 11)

```bash
sudo pkg install developer/versioning/git developer/gcc \
    developer/build/automake developer/build/gnu-make \
    developer/build/cmake developer/build/libtool \
    library/python/pip developer/build/autoconf \
    developer/build/ninja runtime/ruby developer/build/pkg-config
```

### Environment Setup

Solaris requires several environment adjustments before building BSL:

```bash
# GNU Make must be the default 'make' — Solaris native make doesn't support GNU extensions
export PATH="/usr/gnu/bin:$PATH"

# Solaris doesn't have 'cc' by default; set compiler explicitly
export CC=gcc
export CXX=g++
```

Optionally, create a persistent `~/bin` directory for symlinks:

```bash
mkdir -p ~/bin
ln -sf /usr/bin/gcc ~/bin/cc
ln -sf /usr/bin/gmake ~/bin/make
export PATH="$HOME/bin:$PATH"
```

### Solaris-Specific Issues Handled by `build-bsl-for-ion.sh`

The following issues are automatically handled when `uname` reports `SunOS`:

| Issue | Cause | Fix Applied |
|-------|-------|-------------|
| `__EXTENSIONS__` needed | Solaris requires this define to expose POSIX/XPG interfaces | `CFLAGS="$CFLAGS -D__EXTENSIONS__"` |
| Ninja not found | BSL defaults to `-G Ninja` but Solaris CMake may not find it | Overridden to `-G "Unix Makefiles"` |
| `jansson.h` not found | Solaris installs jansson headers under `/usr/include/jansson/` instead of `/usr/include/` | Auto-detected and passed via `-DCMAKE_INCLUDE_PATH` |
| Valgrind unavailable | Solaris does not ship valgrind | `-DTEST_MEMCHECK=OFF` |

### Solaris-Specific Issues Handled by `configure.ac`

When running `./configure --enable-bsl` on Solaris, the following adjustments are made automatically:

| Issue | Cause | Fix Applied |
|-------|-------|-------------|
| `cp -a` unsupported | Solaris `cp` doesn't support `-a` | Uses `cp -pPR` instead |
| `-Wl,-rpath-link` unsupported | Solaris linker uses different syntax | Uses `-R${BSL_HOME}/lib` instead |
| Jansson header detection | `AC_CHECK_HEADER` needs the include path | Temporarily adds `AM_CFLAGS` to `CFLAGS` for header checks |
| `BSL_HOME` must be absolute | Libtool requires absolute paths | Resolved via `cd "$BSL_HOME" && pwd` |

### Build Steps on Solaris

```bash
cd /path/to/ion-ios-dev

# 1. Set up environment
export PATH="/usr/gnu/bin:$PATH"
export CC=gcc
export CXX=g++

# 2. Initialize submodules
git submodule update --init --recursive external/BSL

# 3. Build BSL (Solaris adjustments are automatic)
./build-bsl-for-ion.sh

# 4. Build ION with BSL
autoreconf -fi
./configure --enable-bsl
gmake -j$(nproc)
sudo gmake install
sudo ldconfig
```

### `pkg-config` Path on Solaris

Solaris does not include `/usr/local/lib/pkgconfig` in the default `pkg-config` search path. After installing ION, the test runner (`runtests`) will fail to detect BSL mode and skip all `.bsl` tests unless this is set:

```bash
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig
```

Add this to `~/.profile` or `~/.bashrc` for persistence. Without it, `pkg-config --cflags ion` will fail to find `ion.pc` and the test runner will treat the build as native BPSec, excluding all BSL-specific tests.

### Note on `localhost` Resolution on Solaris

ION supports both IPv4 and IPv6. The `.bsl` test configs use `127.0.0.1` instead of `localhost` for clarity, but `localhost` works correctly as long as `/etc/hosts` is properly ordered.

**Important:** On Solaris, `getent hosts` and `getaddrinfo()` may return inconsistent results because they use different NSS databases:
- `getent hosts` → queries the `hosts` database (IPv4 only)
- `getaddrinfo()` (used by ION) → queries the `ipnodes` database (IPv4 and IPv6)

Users should verify what `getaddrinfo()` actually resolves by checking:
```bash
getent ipnodes localhost
```

If this returns `::1` first (because `/etc/hosts` lists `::1 localhost` before `127.0.0.1 localhost`), ION will use IPv6. This is not a problem as long as all ION processes resolve consistently and IPv6 loopback is plumbed (`ipadm show-addr lo0/v6`).

If you see connection failures between nodes using `localhost`, check the order in `/etc/hosts` and ensure IPv4 comes first:
```
127.0.0.1 myhost localhost loghost
::1 myhost localhost
```

Do not rely on `getent hosts` to diagnose address resolution issues — always use `getent ipnodes` to see what ION will actually resolve.

## Troubleshooting

### `undefined symbol: BSLX_BCB_Execute`

ION was not rebuilt with BSL enabled. Rebuild:
```bash
make clean && ./configure --enable-bsl && make -j$(nproc) && sudo make install
```

### `cannot open shared object file: libbsl_front.so`

BSL libraries not in the linker path:
```bash
export LD_LIBRARY_PATH=/path/to/external/BSL/testroot/usr/lib:$LD_LIBRARY_PATH
sudo ldconfig
```

### `bpadmin` does not recognize `m bsl`

`bpadmin` was built without BSL. Rebuild with `--enable-bsl`.

### `cannot find -lbsl_crypto`

BSL not built or `BSL_HOME` incorrect:
```bash
ls external/BSL/testroot/usr/lib/libbsl_*.so
# If missing, rebuild BSL submodule
./build-bsl-for-ion.sh
```

---

## Appendices

### Appendix A: Detailed Prerequisites

| Package | Purpose | Version |
|---------|---------|---------|
| `gcc` or `clang` | C/C++ compiler | 11.0+ (13.0+ for BSL unit tests) |
| `automake`, `autoconf`, `libtool`, `m4` | ION build system | any |
| `cmake` | BSL build system | 3.20+ |
| `pkg-config` | Library discovery | any |
| `libssl-dev` / `openssl-devel` | Cryptographic functions | 3.0+ |
| `libjansson-dev` / `jansson-devel` | JSON parsing for policy/keys | 2.13+ |
| `ninja-build` | Fast parallel build for BSL (optional) | 1.10+ |

### Appendix B: Manual BSL Build

BSL v1.1 builds as a standalone library with no ION dependencies. You can build BSL first, then build ION linking against it.

```bash
# 1. Build BSL
git submodule update --init --recursive external/BSL
./build-bsl-for-ion.sh

# 2. Build ION with BSL
autoreconf -fi
./configure --enable-bsl
make -j$(nproc)
sudo make install && sudo ldconfig
```

For finer-grained control over BSL's CMake build:

```bash
cd external/BSL
./build.sh clean
./build.sh deps
./build.sh prep \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=$PWD/testroot/usr \
    -DBUILD_TESTING=OFF
./build.sh
cd build/default
cmake --install . --prefix $PWD/../../testroot/usr
```

### Appendix C: System-Wide BSL Installation

```bash
cd external/BSL
./build.sh prep \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DBUILD_TESTING=OFF
./build.sh
cd build/default
sudo cmake --install . --prefix /usr/local
sudo ldconfig
```

No `LD_LIBRARY_PATH` configuration needed with a system-wide install.

### Appendix D: Running BSL Unit Tests

BSL unit tests require GCC 13+ to compile.

```bash
cd external/BSL
./build.sh prep -DBUILD_TESTING=ON
./build.sh
./build.sh check
```

### Appendix E: Key and Policy File Reference

#### Key File Format

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

- `kty`: Key type (`oct` for symmetric)
- `kid`: Key identifier (referenced in policy `key_name`)
- `k`: Base64-encoded key material

#### Policy Parameters

| Parameter | Values | Description |
|-----------|--------|-------------|
| `role` | `s`, `v`, `a` | Source, verifier, or acceptor |
| `src` / `dest` | EID pattern | Wildcard patterns supported |
| `tgt` | `0`, `1` | Target block (0=primary, 1=payload) |
| `loc` | `appin`, `clin`, `appout` | Processing location |
| `sc_id` | `1`, `2` | Security context (1=BIB, 2=BCB) |

**BIB parameters:** `key_name`, `sha_variant` (5/6/7), `scope_flags`, `key_wrap` (0/1)
**BCB parameters:** `key_name`, `aes_variant` (1/3), `scope_flags`, `key_wrap` (0/1)

#### Validating JSON Files

```bash
python3 -m json.tool keys.json
python3 -m json.tool policy.json
```

### Appendix F: Uninstalling BSL

**Local (testroot):**
```bash
cd external/BSL
rm -rf testroot/usr build
```

**System-wide:**
```bash
sudo rm -f /usr/local/lib/libbsl_*.so* /usr/local/lib/libqcbor.so*
sudo rm -rf /usr/local/include/bsl
sudo ldconfig
```

## Additional Resources

- **BSL Repository**: https://github.com/NASA-AMMOS/BSL
- **RFC 9172**: Bundle Protocol Security (BPSec)
- **RFC 9173**: Default Security Contexts for BPSec
- **ION Documentation**: https://ion-dtn.readthedocs.io
