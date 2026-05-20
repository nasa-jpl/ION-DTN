# AGENTS.md

Last verified: 2026-05-27

## Project Overview

NASA/JPL's Interplanetary Overlay Network (ION) - an implementation of Delay/Disruption Tolerant Networking (DTN). ION enables reliable data transmission in environments with high latency, intermittent connectivity, and long signal propagation delays (space communications, planetary networks).

## Tech Stack

- Language: C (C99/C11)
- Build System: GNU Autotools (primary) / Development Makefiles (alternative)
- Version: ION 4.1.4+
- Protocol: Bundle Protocol v7 (BPv7 only - BPv6 removed as of 4.1.4-a.2)

## Commands

### Building (Autotools - Recommended)

```bash
# Standard build
./configure
make
sudo make install
sudo ldconfig

# If configure missing, generate it first
autoreconf -fi

# Build with MBEDTLS cipher suite for BPSec
./configure --enable-crypto-mbedtls
make
sudo make install

# Clean build artifacts
make clean

# Uninstall
sudo make uninstall
```

### Building (Development Makefiles - Alternative)

```bash
# Build all
gmake -f Makefile.dev all

# Build with BSL (Bundle Security Layer)
gmake -f Makefile.dev all USING_BSL=1

# Install
gmake -f Makefile.dev install

# Clean
gmake -f Makefile.dev clean
```

### Testing

```bash
# Build test programs (C-based tests)
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
--enable-crypto-mbedtls       # Enable MBEDTLS cipher suite
--enable-bpsec-debugging      # Enable BPSec debug logging
--enable-ewchar               # Enhanced watch character support
--enable-commandline-history  # Command history (ground systems only)
--disable-dgr                 # Disable DGR convergence layer
--disable-bssp                # Disable BSSP convergence layer

# Custom compiler flags
./configure CFLAGS="-DGDSWATCHER -Itools/gdswatcher"
```

## Project Architecture

ION is organized as a layered system with core services and protocol implementations:

### Core Layer (ICI - Inter-Component Interface)

`ici/` - Foundation shared by all ION components:

- **SDR (Space Data Router)**: Persistent storage system using memory-mapped files and heap management. Critical for reliability - all protocol state persists across restarts.
- **PSM (Partitioned Shared Memory)**: Inter-process communication mechanism.
- **ZCO (Zero-Copy Objects)**: Efficient data handling for large payloads - reference-counted, supports both file and SDR backing.
- **Platform abstraction**: Mutex, semaphore, shared memory primitives.
- **ionShred()**: Resource cleanup function for request tickets - must be called on error paths to prevent leaks.

### Protocol Layers

- `ltp/` - **LTP (Licklider Transmission Protocol)**: Reliable data transmission designed for space links with asymmetric data rates and long delays. Supports sessions, blocks, segments, red/green data.

- `bpv7/` - **Bundle Protocol v7**: Application-layer DTN protocol. Contains:
  - `cgr/` - Contact Graph Routing (dynamic routing for time-varying topology)
  - `bpsec/` - Bundle Protocol Security extensions
  - `daemon/` - Core daemons (bpclock, bptransit, etc.)
  - Convergence layer adapters: `tcp/`, `udp/`, `stcp/`, `dccp/`, `ltp/`, `file/`, etc.

### Higher-Level Services

- `cfdp/` - CCSDS File Delivery Protocol (reliable file transfers over DTN)
- `ams/` - Asynchronous Message Service (pub/sub messaging)
- `dtpc/` - Delay-Tolerant Payload Conditioning (TCP-like retransmission)
- `bss/` - Bundle Streaming Service
- `bssp/` - Bundle Streaming Service Protocol
- `dgr/` - Datagram Retransmission (UDP-like with reliability)
- `nm/` - Network Management
- `tc/` - Trusted Collective / DTKA (delay-tolerant key authentication)

### Important Directories

- `tests/` - Extensive test suite (137+ test scenarios). Each test is self-contained with a `dotest` script.
- `configs/` - Example ION configuration files (.rc files).
- `demos/` - Demonstration scenarios.
- `tools/` - Development tools (gdslogger, gdswatcher for logging/monitoring).

## Key Concepts

### ION Configuration Files (.rc files)

ION nodes are configured via `.rc` (run control) files that specify:

- Node number/EID
- Contacts (communication opportunities with other nodes)
- Ranges (one-way light times)
- Protocol configuration (LTP spans, BP endpoints, convergence layers)

Start ION with: `ionstart -I <config.rc>`

### Lifecycle Management

- `ionstart` - Initialize ION node from configuration
- `ionrun` - Quick setup tool (recommended for beginners)
- `ionstop` - Graceful shutdown
- `killm` - Force kill all ION processes (use with caution)
- `ion-diagnostics` - Collect diagnostic information for debugging

### SDR Transactions

Most ION operations require SDR transactions:

```c
Sdr sdr = getIonsdr();
CHKERR(sdr_begin_xn(sdr));
// ... operations ...
if (sdr_end_xn(sdr) < 0) {
    putErrmsg("Transaction failed", NULL);
}
```

### Memory Management

- SDR allocations persist across process restarts
- ZCO objects are reference-counted - must call zco_destroy_reference()
- Always call ionShred() on error paths when holding ReqTicket
- Check for NULL/negative returns from allocation functions

## Testing Framework

Tests live in `tests/` subdirectories:

- Each test has a `dotest` executable script
- Tests must start/stop ION themselves
- Exit codes: 0=success, 1=failure, 2=skip
- Environment: `$IONDIR` points to source root, binaries in `$PATH`
- Optional tests (`.optional` file): run but don't affect pass/fail
- Exclude files (`.exclude_*`): conditionally skip tests

To add a new test: create directory `tests/my-test/` with executable `dotest` script.

**Important:** If you are adding new features or fixing bugs, you must read [`ION-TestSet-Readme.md`](gh-pages/docs/ION-TestSet-Readme.md) for comprehensive instructions on how to create tests for ION.

>>>>>>>
## Conventions

- Function naming: camelCase for public APIs, snake_case less common
- Error handling: Return -1/NULL on error, putErrmsg() for error messages
- Logging: writeErrMemo() for critical errors, writeMemo() for info
- Header files: Public APIs in `include/`, private headers end with `P.h`
- Man pages: POD format in `doc/pod1/` (commands), `pod3/` (APIs), `pod5/` (file formats)

## Important Notes

- **WSL1 not supported** - WSL2 works fine
- **BPv6 removed** - All development uses BPv7 as of ION 4.1.4-a.2
- **Thread safety**: Most ION code uses processes, not threads. Shared memory synchronization via semaphores.
- **Endianness**: Network byte order used for protocol encoding
- **Flight vs Ground**: Some features (command history, logging hooks) are ground-only
- **Multi-node Operation**: If you need to operate multiple ION nodes on the same host machine, you must read [`ION-Deployment-Guide.md`](gh-pages/docs/ION-Deployment-Guide.md) for the required configuration instructions, such as setting the `ION_NODE_LIST_DIR` environment variable.

## CI/CD

GitHub Actions workflows in `.github/workflows/`:

- Multiple OS targets (Debian, RTEMS, macOS)
- Atomic test tiers for parallel execution
- Automatic test retries on failure
- Benchmark tests for performance tracking
