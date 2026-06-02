# AGENTS.md

Last verified: 2026-06-05 (Review quarterly or after major architectural changes)

## Project Overview

NASA/JPL's Interplanetary Overlay Network (ION) implements Delay/Disruption Tolerant Networking (DTN) for reliable data transmission in high-latency, intermittent-connectivity environments (space communications, planetary networks).

## Tech Stack

- Language: C (C99/C11)
- Build System: GNU Autotools (primary) / Development Makefiles (alternative)
- Version: ION 4.1.4+
- Protocol: Bundle Protocol v7 (BPv7 only - BPv6 removed as of 4.1.4-a.2)

### Dependencies

- **Required**: Standard C library, POSIX-compliant system
- **Optional**:
  - Mbed TLS (for ION's native BPSec cryptographic operations)
  - BSL (Bundle Security Library) - external library providing an alternative BPSec implementation
- **Build tools**: autoconf, automake, libtool (for Autotools build)

### Security Implementations

ION provides two BPSec implementations:

1. **Native BPSec** (`bpv7/bpsec/`): Built-in implementation using Mbed TLS
2. **BSL (Bundle Security Library)**: External library with separate build options

## Repository Layout

- `ici/` - Interplanetary Communication Infrastructure (core foundation)
- `ltp/` - Licklider Transmission Protocol implementation
- `bpv7/` - Bundle Protocol v7 implementation
- `cfdp/`, `ams/`, `dtpc/`, `bss/`, `bssp/`, `dgr/`, `nm/`, `tc/` - Higher-level services
- `tests/` - Comprehensive test suite with self-contained test scenarios
- `configs/` - Example ION configuration files (.rc files)
- `demos/` - Demonstration scenarios
- `tools/` - Development tools (gdslogger, gdswatcher for logging/monitoring)

## Project Architecture

ION uses a **multi-process architecture** with layered services where components communicate via shared memory (PSM) and persistent storage (SDR), providing isolation and crash recovery.

### Core Layer (ICI - Interplanetary Communication Infrastructure)

`ici/` - Foundation shared by all ION components:

- **SDR (Simple Data Recorder)**: Persistent storage using memory-mapped files with transactional guarantees for atomic updates
- **PSM (Personal Space Management)**: Inter-process communication via shared memory
- **ZCO (Zero-Copy Objects)**: Reference-counted efficient data handling supporting file and SDR backing
- **Platform abstraction**: Cross-platform mutex, semaphore, shared memory primitives
- **ionShred()**: ReqTicket resource cleanup function - must call on error paths to prevent leaks

### Protocol Layers

- `ltp/` - **LTP (Licklider Transmission Protocol)**: Reliable data transmission designed for space links with asymmetric data rates and long delays. Supports sessions, blocks, segments, red/green data.

- `bpv7/` - **Bundle Protocol v7**: Application-layer DTN protocol. Contains:
  - `cgr/` - Contact Graph Routing (dynamic routing for time-varying topology)
  - `bpsec/` - Bundle Protocol Security extensions (ION's native BPSec implementation)
  - `daemon/` - Core daemons (bpclock, bptransit, etc.)
  - Convergence layer adapters: `tcp/`, `udp/`, `stcp/`, `ltp/`, `file/`, etc.

### Higher-Level Services

- `cfdp/` - CCSDS File Delivery Protocol (reliable file transfers over DTN)
- `ams/` - Asynchronous Message Service (pub/sub messaging)
- `dtpc/` - Delay-Tolerant Payload Conditioning (TCP-like retransmission)
- `bss/` - Bundle Streaming Service
- `bssp/` - Bundle Streaming Service Protocol
- `dgr/` - Datagram Retransmission (UDP-like with reliability)
- `nm/` - Network Management
- `tc/` - Trusted Collective / DTKA (Delay-Tolerant Key Administration)

## Key Concepts

### ION Configuration Files (.rc files)

ION nodes are configured via `.rc` (run control) files specifying:

- Node number/EID
- Contacts (communication opportunities with other nodes)
- Ranges (one-way light times)
- Protocol configuration (LTP spans, BP endpoints, convergence layers)

Example configs in `configs/`. Start with: `ionstart -I <config.rc>`

### Lifecycle Management

- `ionstart` - Initialize ION node from configuration file
- `ionrun` - Simplified alternative for single-node testing
- `ionstop` - Graceful shutdown
- `killm` - Force kill (use with caution - may leave inconsistent state)
- `ion-diagnostics` - Collect diagnostic information

### Troubleshooting

For debugging and monitoring, see [SOP-for-ION.md](SOP-for-ION.md), [ION-Monitoring-Guide.md](ION-Monitoring-Guide.md), and [ION-Watch-Characters.md](ION-Watch-Characters.md) for standard operating procedures, monitoring techniques, and watch character interpretation.

### SDR Transactions

All SDR modifications must occur within a transaction block for atomic updates:

```c
Sdr sdr = getIonsdr();
CHKERR(sdr_begin_xn(sdr));
// ... operations that modify SDR ...
if (sdr_end_xn(sdr) < 0) {
    putErrmsg("Transaction failed", NULL);
}
```

### Memory Management

- SDR allocations persist across restarts (memory-mapped files)
- ZCO objects are reference-counted - call zco_destroy_reference() to avoid leaks
- Call ionShred() on error paths when holding ReqTicket resources
- Check for NULL/negative returns from allocation functions
- PSM provides inter-process communication

## Commands

**Platform notes:** Some commands are platform-specific. Use `gmake` on Solaris where GNU Make is not the default. The `sudo ldconfig` command is Linux-specific and may not be needed on other platforms.

### Building (Autotools - Recommended)

```bash
# Standard build
./configure
make
sudo make install
sudo ldconfig

# Generate configure if missing
autoreconf -fi

# Build with Mbed TLS (for native BPSec)
./configure --enable-crypto-mbedtls
make
sudo make install

# Clean
make clean

# Uninstall
sudo make uninstall
```

### Building (Development Makefiles - Alternative)

```bash
# Build all
gmake -f Makefile.dev all

# Build with BSL (Bundle Security Library - external BPSec implementation)
gmake -f Makefile.dev all USING_BSL=1

# Install
gmake -f Makefile.dev install

# Clean
gmake -f Makefile.dev clean
```

### Testing

```bash
# Build test programs
make test

# Run normal test suite
cd tests && ./runtestset normaltests

# Run all tests
cd tests && ./runtestset alltests

# Run specific test
cd tests && ./runtests <test-name>

# Example: run loopback test
cd tests && ./runtests 1000.loopback

# Rerun failed tests
cd tests && ./runtestset retest
```

### Configuration Options

```bash
# View all configure options
./configure -h

# Common options:
--enable-crypto-mbedtls       # Enable Mbed TLS cipher suite (for native BPSec)
--enable-bpsec-debugging      # Enable BPSec debug logging (native implementation)
--enable-ewchar               # Enhanced watch character support
--enable-commandline-history  # Command history (ground systems only)
--disable-dgr                 # Disable DGR convergence layer
--disable-bssp                # Disable BSSP convergence layer

# Custom compiler flags
./configure CFLAGS="-DGDSWATCHER -Itools/gdswatcher"
```

## Testing Framework

**Important:** Read [ION-TestSet-Readme.md](ION-TestSet-Readme.md) for test creation instructions.

Tests in `tests/` subdirectories:

- Each test has executable `dotest` script that starts/stops ION
- Exit codes: 0=success, 1=failure, 2=skip
- Environment: `$IONDIR` points to source root, binaries in `$PATH`
- `.optional` files: run but don't affect pass/fail
- `.exclude_*` files: conditionally skip tests

To add a test: create `tests/my-test/dotest` executable.

## Conventions

### Code Style

- Follow [ION-Coding-Guide](ION-Coding-Guide.md) for coding guidance
- Function naming: camelCase for public APIs, snake_case less common
- Error handling: Return -1/NULL on error, putErrmsg() for error messages
- Logging: writeErrMemo() for critical errors, writeMemo() for info
- Headers: Public APIs in `include/`, private headers end with `P.h`
- Man pages: POD format in `doc/pod1/` (commands), `pod3/` (APIs), `pod5/` (file formats)
- Avoid issue/PR/line number references in commits/comments unless reference has full URL (ION develops in private repos; references become stale)
- Commits must be atomic - build and pass tests after each commit for reliable `git bisect`

## Important Notes

- **WSL1 not supported** - WSL2 works fine
- **BPv6 removed** - All development uses BPv7 as of ION 4.1.4-a.2
- **Thread safety**: Most ION code uses processes, not threads. Shared memory synchronization via semaphores.
- **Endianness**: Network byte order used for protocol encoding
- **Flight vs Ground**: Some features (command history, logging hooks) are ground-only
- **Multi-node Operation**: If you need to operate multiple ION nodes on the same host machine, you must read [ION-Deployment-Guide.md](ION-Deployment-Guide.md) for the required configuration instructions, such as setting the `ION_NODE_LIST_DIR` environment variable.

## CI/CD

GitHub Actions workflows in `.github/workflows/`:

### Platform Support Tiers

**Tier 1 (Full Support)**: Ubuntu 20.04/22.04, RHEL 8/9, OracleLinux 8/9, Solaris 11

- Comprehensive test suites via ARC runners ([`ci-workflow-arc.yml`](../../.github/workflows/ci-workflow-arc.yml)) and Solaris workflow ([`ci-workflow-solaris.yml`](../../.github/workflows/ci-workflow-solaris.yml))
- Full regression testing with parallel execution

**Tier 2 (Best Effort)**: RTEMS, macOS ARM64, FreeBSD 14

- Compilation verification and best-effort test runs

**Tier 3 (Legacy/Minimal)**: Raspberry Pi OS, VxWorks

- Previously tested, no longer supported

### Test Execution Strategy

- **Atomic test tiers**: Parallel execution across multiple runners
- **Benchmark-based distribution**: Tests distributed targeting equal execution time
- **Matrix-based parallelization**: [`git_matrix.py`](../../tests/git_matrix.py) generates concurrent test batches for ARC/Solaris runners
