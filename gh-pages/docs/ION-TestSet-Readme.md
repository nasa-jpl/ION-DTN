# Running the ION test set

## Directory layout

The `tests` directory under ION's root folder contains the test suite. Each test lives in its own subdirectory of this directory. Each test is conducted by a script `$TESTNAME/dotest`. Another directory that contains ION tests is the `demos` directory, which includes examples of ION configurations using different convergence layers. For this document, we focus on the usage of the `tests` directory.

## Exclude files

Exclude files are hidden files that allow for tests to be disabled based on certain conditions that may cause the test not to run correctly. If an exclude file exists, it should have a short message about why the test has been excluded.

Exclude files can exist in any of the following formats:

- `.exclude_OS-TYPE`: Disables a test for an operating system that it does not run successfully on. Acceptable values to fill in for OS-TYPE are "linux", "mac", and "solaris".

- `.exclude_BP-VERSION`: Disables a test for a version of the bundle protocol that it does not run correctly or does not make sense with. As of ION 4.0.0, the acceptable values to fill in for `BP-VERSION` are "bpv6" and "bpv7".

- `.exclude_all`: Disables a test for all platforms.

- `.exclude_expert`: Disables a test because of additional utilities that are required for the test. To work around this exclusion if you want to run an expert test, you can set `ION_RUN_EXPERT="yes"` in your shell environment to enable all ION tests classified as expert.

- `.exclude_cbased`: Disables a test that relies on compiling a C program to generate the dotest executable script. To exclude C-based tests, you need to define the environment variable `ION_EXCLUDE_CBASED`.

## Optional tests

Tests can be marked as optional by adding a `.optional` file to the test directory. Optional tests are run by default (unless explicitly excluded via an exclude file), but their results do not affect the overall pass/fail status of the test campaign.

### How to mark a test as optional

To mark a test as optional, create a file named `.optional` in the test directory:

```bash
touch tests/my-test/.optional
```

### Use cases for optional tests

Optional tests are useful for:

- Experimental or unstable tests that are not yet ready for CI enforcement
- Performance tests that may be sensitive to system load or timing
- Tests that depend on external resources that may be temporarily unavailable
- Long-running tests that provide valuable information but should not block the test suite
- Tests that are being developed or debugged

### Key behaviors of optional tests

- They are run during normal test execution
- They are clearly marked as "OPTIONAL TEST" in the console output
- Their pass/fail status is reported in a separate "Optional Tests" section at the end of the test run
- Failed optional tests do **NOT** cause the overall test campaign to fail (exit code 0)
- Failed optional tests are **NOT** included in the `retest` file
- Optional tests are **NOT** retested in the automatic retest phase

### Example output

When running tests that include optional tests, the output will clearly indicate when an optional test is being executed:

```
***
*** OPTIONAL TEST: experimental-feature
***
OPTIONAL TEST FAILED!
```

At the end of the test run, optional test results are shown in a dedicated section:

```
=== Optional Tests ===
optional tests passed: 2
    perf-test
    integration-test

optional tests failed: 1
    experimental-feature
```

The overall test campaign will still report success (exit 0) even if optional tests fail, as long as all non-optional tests pass.

## Running the tests

The tests are run by running `make test-all` in the top-level directory, or by running `runtests` in the `tests` directory.

An individual test can also be run: `./runtests <test_name>`

A file defining a set of tests can be run with `runtestset`. The arguments to `runtestset` are files that contain globs of tests to run, for example: `./runtestset quicktests`. Alternatively, you can pass glob patterns directly to `runtests`, such as `./runtests a*`, which will run all tests that match the `a*` name pattern.

In order to run BPSec-related regression tests and other tests marked as "expert", one should set the `ION_RUN_EXPERT` environment variable to a non-empty value (such as "1", "yes", or "YES"). This enables tests that are excluded with `.exclude_expert` files. Otherwise, those tests will be skipped.

## Expert Test Requirements

Expert tests (`.exclude_expert`) require additional system capabilities or dependencies beyond a standard ION build. The following table summarizes the current expert tests and their requirements:

| Test | Description | Requirements |
|------|-------------|--------------|
| `bpsec/python_tests` | BPSec cryptographic operations | Python >= 3.7, MbedTLS library, ION compiled with `--enable-crypto-mbedtls --enable-bpsec-debugging` |
| `ipaddr-caching-udpclo` | IPv4/IPv6 address caching for UDP CLO | Password-less sudo, ability to modify `/etc/hosts`, IPv6 support (optional) |
| `ipaddr-caching-udplso` | IPv4/IPv6 address caching for UDP/LTP | Password-less sudo, ability to modify `/etc/hosts` |
| `tcpcl-ack-resilience` | TCP convergence layer ACK resilience | Password-less sudo, iptables or nftables, TCP kernel parameter access (`/proc/sys/net/ipv4/`) |

### Detailed Requirements

#### BPSec Python Tests (`bpsec/python_tests`)

This test validates Bundle Protocol Security (BPSec) cryptographic operations. It requires:

- **Python 3.7 or later**: The test uses Python scripts for validation
- **MbedTLS library**: Version 2.28.9 recommended
- **Special compile flags**: ION must be built with:
  ```bash
  ./configure --enable-crypto-mbedtls --enable-bpsec-debugging
  ```

Without MbedTLS, ION uses NULL cipher suites and this test will fail.

#### IP Address Caching Tests (`ipaddr-caching-udpclo`, `ipaddr-caching-udplso`)

These tests validate DNS resolution caching and failover behavior in the UDP convergence layers. They require:

- **Password-less sudo**: The tests modify system files and need non-interactive sudo access
- **`/etc/hosts` modification**: Tests temporarily add/remove hostname entries
- **IPv6 support** (optional for `udpclo`): Tests IPv6 fallback when available

The tests verify that ION correctly handles:
- DNS resolution failures and bundle abandonment
- Address cache refresh cycles (65-second intervals)
- Automatic IPv4/IPv6 fallback

#### TCPCL ACK Resilience Test (`tcpcl-ack-resilience`)

This test validates TCP convergence layer behavior during network interruptions. It requires:

- **Password-less sudo**: Required for firewall and kernel parameter modifications
- **Packet filtering**: Either `iptables` (most Linux distributions) or `nftables` (RHEL/Oracle Linux)
- **TCP kernel parameters**: Access to modify `/proc/sys/net/ipv4/tcp_retries1`, `tcp_retries2`, and `tcp_keepalive_time`
- **Ports 4555 and 4556**: Must be available for the test

The test sends 10,000 bundles while temporarily blocking network traffic to verify ACK resilience and connection recovery.

### CI/CD Integration

Most CI workflows set `ION_RUN_EXPERT="yes"` to run the full test suite. Platforms in the CI matrix should have:

1. Password-less sudo configured for the runner user
2. Required tools installed (iptables/nftables, Python 3.7+)
3. MbedTLS library installed (for BPSec tests)
4. IPv6 enabled (optional, tests adapt when unavailable)

If a platform lacks certain capabilities (e.g., no MbedTLS), the corresponding expert tests will fail but other tests will continue to run.

## Writing new tests

### Test directory structure

Each test lives in its own subdirectory under `tests/`. A minimal test directory contains:

```
tests/my-test/
├── dotest          # Required. The test driver script (must be executable).
├── cleanup         # Required. Cleans up ION processes and test artifacts.
├── .description    # Optional. One-line description shown by runtests.
├── .optional       # Optional. Marks the test as optional (see above).
└── (config files)  # ION configuration files used by the test.
```

Multi-node tests typically use a subdirectory per node:

```
tests/my-multi-node-test/
├── dotest
├── cleanup
├── 2.ipn.ltp/          # Node 2 working directory and configs
│   ├── amroc.ionrc
│   ├── amroc.bprc
│   └── ...
├── 3.ipn.ltp/          # Node 3
│   └── ...
└── 5.ipn.ltp/          # Node 5
    └── ...
```

### The `cleanup` script

The `cleanup` script is responsible for two things:

1. **Stop all ION processes and release IPC resources** by calling `killm f`. The `f` (force) flag ensures a full cleanup of all ION instances, shared memory, and semaphores.

2. **Remove test-specific artifacts** such as log files, output files, and temporary data generated during the test.

#### When `runtests` calls cleanup

The `runtests` framework calls `./cleanup` at these points:

- **Before** running `./dotest` — unconditionally, to ensure a clean starting state.
- **After** `./dotest` exits with **0** (pass) — by default, cleanup runs to reclaim disk space. Set `PRESERVE_TEST_LOGS=1` to skip cleanup and keep logs from passing tests.
- **After** `./dotest` exits with **1** (fail) — **never**. Logs are always preserved on failure for debugging.
- **After** `./dotest` exits with **2** (skip) or any other value — cleanup is **not** called.

By default, `runtests` runs post-test cleanup after passing tests to prevent disk exhaustion during long CI batches. Pre-test cleanup always runs to guarantee a clean starting state. Failed test logs are always preserved regardless of settings.

When `PRESERVE_TEST_LOGS` is set to `1` (`export PRESERVE_TEST_LOGS=1`), `runtests` skips post-test cleanup after passing tests, preserving all logs for inspection. This is useful for local debugging but should not be used in CI environments with limited disk space.

#### Standalone cleanup mode (`./runtests cleanup`)

`./runtests cleanup [tests...]` runs cleanup without executing any test. It performs a full reset, intended to put the test tree back to a pristine state after an interrupted or hung campaign:

1. **`killm f` once upfront** — reaps any orphan ION daemons before per-test sweeps, so the artifact removal isn't racing live processes that hold the files open.
2. For each test in turn:
   - The test's own `./cleanup` script (if present) — handles named config artifacts that only the test knows about.
   - `cleanup_staging_files` — generic runtime cruft (`bpacq*`, `ltpacq*`, `*.sdr`, `bsspSegment*`, `xnref*`, `*.sdrlog`, `core`, `core.*`) under the test directory and `/tmp`.
   - `ion.log` and `ion-system.log` removal (depth 3, so multi-node `nodeN/ion.log` files are caught).
3. Finally, removes `tests/retest` and `tests/progress` so the campaign bookkeeping is also reset.

Unlike the per-test cleanup that happens during a normal run, standalone cleanup mode does **not** preserve `ion.log` or `ion-system.log` — the assumption is that if you asked for cleanup, you want a real reset. Investigate failures before invoking it.

If no test names are passed, cleanup runs against every test that `runtests` would otherwise discover.

#### Environment isolation

`runtests` executes `./dotest` and `./cleanup` as separate subprocesses. Environment variables exported inside `dotest` (such as `ION_NODE_LIST_DIR`) do **not** propagate to the cleanup subprocess. Each script is responsible for setting the environment variables it needs.

#### Multi-node cleanup and `ION_NODE_LIST_DIR`

`killm f` treats the run as multi-node when `ION_NODE_LIST_DIR` is set **and** `$ION_NODE_LIST_DIR/ion_nodes` exists and is non-empty.

In multi-node mode, `killm f` deliberately **skips** graceful `ionexit` and goes straight to SIGTERM → SIGKILL across all ION processes, followed by unconditional `ipcrm`/semaphore cleanup. It does **not** walk the node directories or run `ionexit` per node. If you need an orderly shutdown that keeps other nodes running, use per-node `ionexit k n` (see the [ION Shutdown Guide](ION-Shutdown-Guide.md)) instead of `killm f`.

That is the behavior a test harness wants: cleanup is after a clean slate, not an orderly handover. Only a `dotest` that stops one node while others keep running needs the graceful path.

Because of the subprocess isolation described above, a multi-node test's cleanup script must export `ION_NODE_LIST_DIR` itself — it cannot rely on the value set by `dotest`. Without it, `killm f` takes its single-node path and first attempts a bare `ionexit` from the cleanup directory, which is not a registered node directory; that attempt fails and can burn up to 15 seconds of timeout before the signal sweep runs. The end state is the same — the signal sweep and IPC cleanup still happen — but the run is slower and noisier.

#### Multi-node shutdown order

ION instances sharing a host also share the SDR working memory segment and the global `ion:GLOBAL:*` semaphores, so when nodes are stopped individually the order matters and `ionexit n` is **not** a substitute for `ionexit k n`. Tests that tear everything down at once with `killm f` do not need to care, because it kills every node's processes and clears the shared IPC in one pass. Tests that stop nodes individually do.

Those rules, the per-path breakdown of what `killm` does with and without `f`, and the reasoning behind them live in the [ION Shutdown Guide](ION-Shutdown-Guide.md); they are deliberately not duplicated here.

**Example cleanup script (single-node):**
```bash
#!/usr/bin/env bash
killm f
rm -f ion.log
```

**Example cleanup script (multi-node):**
```bash
#!/usr/bin/env bash
export ION_NODE_LIST_DIR=$PWD
killm f
rm -f ion_nodes
rm -f 2.ipn.ltp/ion.log 3.ipn.ltp/ion.log 5.ipn.ltp/ion.log
rm -f 5.ipn.ltp/testfile1 5.ipn.ltp/testfile2
```

### The `dotest` script

A test directory must contain an executable file named `dotest`. If a directory does not contain this, the test will be ignored. The `dotest` program should execute the test, possibly reporting runtime information on stdout and stderr, and indicate by its return value the result of the test as follows:

    0: Success
    1: Failure
    2: Skip this test

The test program starts without the ION stack running (cleanup has already been called). The test program is responsible for starting ION in the way that is appropriate for the test.

**Important conventions:**

- **EXIT trap recommended**: Every `dotest` script should include `trap 'killm f' EXIT` near the top of the file (after the shebang). This ensures ION processes and IPC resources are cleaned up on every exit path — normal exit, error exit, skip, and unexpected termination. The `runtests` harness only calls `killm f` directly on the timeout and interrupt paths — on a normal pass the post-test `killm f` comes from `./cleanup`, and on failure/skip nothing is killed — so a test that omits the trap must handle its own teardown on every exit path (e.g. an explicit `ionstop`/`killm f`); it must not rely on the harness for teardown.

- **Mid-test resets**: If your test runs multiple sub-scenarios that each require a fresh ION instance, call `killm f` between them to ensure full cleanup before restarting ION. The `f` flag is necessary because multi-node tests set `ION_NODE_LIST_DIR`, which causes bare `killm` to operate in node-only mode.

- **Error paths**: On error, simply `exit 1`. The EXIT trap handles cleanup automatically.

- **Merging with other traps**: If your script needs an EXIT trap for other purposes (e.g., removing temporary files, restoring terminal state), combine them into a single trap:
  ```bash
  trap 'rm -f "$TMPFILE"; killm f' EXIT
  ```

#### Why the EXIT trap is recommended — and its one trade-off

The trap is redundant on the paths the harness already handles: a passing test is reaped by `./cleanup`, and timeouts and Ctrl-C are reaped by `runtests` itself. Its value is concentrated on the paths where `runtests` runs **neither** `./cleanup` **nor** `killm f`:

- **Self-`exit 1` (failure), `exit 2` after starting ION (skip), and mid-script aborts / uncaught errors.** The failure branch deliberately preserves logs and never kills ION, so without the trap the daemons the test started stay alive after `dotest` returns — until the *next* test's pre-test cleanup (or indefinitely, if this was the last or only test).
- **Standalone `./dotest` runs — the trap's primary benefit.** Run by hand there is no harness wrapping the test with pre/post cleanup, so the trap is the *only* thing that reaps ION on exit.

In practice the standalone case is where the trap earns its keep: under `runtests` a leak from a self-`exit 1` is usually swept by the next test's pre-test cleanup, but a direct `./dotest` invocation — the common way to develop and debug a single test — has no such safety net. Without the trap, every hand-run of a test that starts ION would leave daemons and IPC resources behind.

This makes each `dotest` self-contained and leak-free: a failed or hand-run test does not orphan ION daemons that keep holding the SDR working-memory segment (key `0xFF00`), the named semaphores, and the wmKey — stale copies of which can collide with and corrupt the next ION start. It also gives a single guaranteed teardown point instead of a `killm f` on every `exit` branch.

**The one gap — missed live-daemon capture on a self-`exit 1`:** the trap's `killm f` fires *inside* `dotest` before `runtests` reaches its failure-path diagnostics, so `ion-diagnostics` can no longer attach to the now-dead daemons — live-daemon stack traces (`gdb -p`) and live statistics are lost for that case. Everything durable is still captured: crash **core files** (`killm f` does not delete them; they are gdb'd post-mortem), `ion.log`, and — on a **timeout/hang** — full live capture, because there `runtests` collects diagnostics from the still-running test *before* anything kills it. If a test specifically needs live-daemon state on failure, capture it *before* returning rather than relying on post-return collection.

**Example dotest structure:**
```bash
#!/usr/bin/env bash
trap 'killm f' EXIT

echo "########################################"
echo "NAME: my-test"
echo "PURPOSE: Verify that feature X works correctly."
echo "########################################"

RETVAL=0

# cleanup is called by runtests before dotest, so ION is not running.

echo "Starting ION..."
ionstart -I "config.rc"
sleep 1

# ... run test logic ...

if ! grep -q "expected output" results.txt; then
    echo "FAIL: Did not find expected output"
    RETVAL=1
fi

exit $RETVAL
```

## The test environment

The `dotest` scripts are run in their test directory.

> **The suite runs the installed ION.
> Run `make install` before running it.**
> `runtests` refuses to start if `ionadmin` is not on `PATH`.
> (`./runtests cleanup` is exempt.)

`runtests` exports two variables that point into the source tree,
for tests that reach source-tree resources
(configuration files, link graphs) through them:

- `IONDIR` — the root of the local ION source directory.

- `CONFIGSROOT` — `$IONDIR/configs`.

Before the first test runs,
`runtests` reports the resolved environment
to both stderr and the `progress` file,
so a completed run records which ION it exercised:

```
# IONDIR: /path/to/ion
# CONFIGSROOT: /path/to/ion/configs
# PATH: /usr/local/bin:...
# LD_LIBRARY_PATH: /usr/local/lib:...
# ionadmin resolves to: /usr/local/bin/ionadmin
# libici resolves to: /usr/local/lib/libici.so.0
```

### Positional argument: the platform string

`runtests` invokes `./dotest "$OS_VERSION"` and `./cleanup "$OS_VERSION"`, so the host platform is passed to both scripts as `$1`. `OS_VERSION` is derived from `uname -a` inside `runtests` and is one of:

| Value | Detected from `uname -a` containing |
|---|---|
| `raspberrypi` | `raspberrypi` |
| `linux` | `Linux` / `LINUX` |
| `mac` | `Mac` / `MAC` / `Darwin` |
| `solaris` | `Solaris` / `SunOS` |
| `freebsd` | `FreeBSD` / `OpenBSD` |

Most `dotest` scripts are OS-independent and can safely ignore the argument. Scripts that do need to branch on host platform (for example, the `ps` argument syntax differs slightly across OSes — bench-ltp uses `if [ "$1" == "windows" ]` for this) should read `$1` directly.

A script that uses positional arguments for its own scenarios (e.g. a benchmark harness with `./dotest baseline` / `./dotest compare`) **must shift the platform string off first**:

```bash
case "${1:-}" in
    raspberrypi|linux|mac|solaris|freebsd) shift ;;
esac

# remaining $@ now holds the scenario name(s) you passed on the command line
```

Without that shift, invoking `./dotest` interactively works fine, but the same script called by `runtests` receives `mac` (or `linux`, etc.) as `$1` and a naive scenario dispatcher will mistake it for a scenario name. See `demos/bench-ltp-xlsa/dotest` for a worked example.

## Test Progress Tracking (ION 4.1.3 and later)

Starting with ION version 4.1.3, the `runtests` script maintains a file called `tests/progress` that records the start time, finish time, and final result for each test.

If the environment variable `RUNTESTS_OUTPUTDIR` is set, as in `export RUNTESTS_OUTPUTDIR="/tmp"`, then the output from each test will be stored in individual files like `/tmp/results.testname` (e.g., `/tmp/results.1000.loopback`), which makes it much easier to find particular text or results when debugging.

Each test conclusion line in the `progress` file and on stderr reports both the actual elapsed time and (when a `.DURATION` file is present) the declared expected duration:

```
PASSED: bping (12s, declared 10s)
FAILED: foo (45s, declared 30s)
TIMEOUT: tc-dtka (2400s, declared 1033s, limit=2400s)
```

This makes it easy to spot tests drifting past their declared budget without cross-referencing `.DURATION` by hand.

## Per-test timeout

Each test is run under a watchdog so that a hung test cannot block the rest of the campaign. The per-test timeout is computed as:

- `4 × .DURATION` if a `.DURATION` file is present and contains a positive integer
- Capped at **2400 s** (40 minutes) and floored at **60 s**
- Defaulted to **1200 s** (20 minutes) when `.DURATION` is missing or unparseable
- Overridden entirely by `ION_TEST_TIMEOUT` (in seconds, uncapped — useful for interactive debugging of long-running tests)

When the watchdog fires, `runtests` collects diagnostics from the still-live test (see below), then kills the process group via SIGTERM/SIGKILL and `killm f`. The test is reported as `TIMEOUT` in the `progress` file with `limit=<seconds>` included.

## Diagnostics on failure or timeout

On any test failure or timeout, `runtests` invokes the `ion-diagnostics` script (located at the repository root) to capture forensic state into `ion-system.log` in the test directory. The same file is preserved alongside `ion.log` after the run, and is collected as a CI artifact by the Solaris workflow.

`ion-diagnostics` writes a sectioned report. Sections include:

1. Host information (uname, OS release, user)
2. ION process inventory (`ps`-style snapshot of every ION daemon)
3. **Stack traces**: live `gdb`/`eu-stack`/`mdb` backtraces of each ION process, plus per-core-file backtraces (see below)
4. Kernel messages (`dmesg`)
5. Memory usage (`free` on Linux, `prtconf`+`vmstat` on Solaris)
6. Disk usage (`df -h`)
7. SysV IPC resources (`ipcs`)
8. POSIX named semaphore files (`/dev/shm`, `/tmp/.LIBRT/SEM*`)
9. SDR and PSM snapshots (`sdrwatch`, `psmwatch`) per node
10. Open file descriptors (`/proc/PID/fd` on Linux, `pfiles` on Solaris, `lsof` fallback)
11. Per-node `ion.log` tails
12. Network sockets (`ss` on Linux, `pfiles` + `netstat -an -P tcp/udp` on Solaris)

Sections can be read back individually from a saved file with `ion-diagnostics read <number|name>`. Use `ion-diagnostics read` with no argument to list available sections.

### Core file capture

A crash that occurs before diagnostics runs (e.g., SIGSEGV in a test daemon) leaves no live process for the live-PID `gdb` path to attach to. The Stack Traces section also discovers core files:

- Linux: reads `/proc/sys/kernel/core_pattern`. If it begins with `|systemd-coredump`, uses `coredumpctl list/info/debug` to extract backtraces. Otherwise, scans the configured core directory plus the test directory.
- Solaris: queries `coreadm` for the configured core paths.
- Each fresh core (matched against `ION_DIAG_SINCE`, which `runtests` sets to the test start time, with a `-mmin -60` fallback) is fed to `gdb -batch -ex 'thread apply all bt full' BINARY CORE`. On Solaris, `mdb` is used if `gdb` is absent.

`runtests` runs `ulimit -c unlimited` at entry so that crashes actually produce a core in environments where the default core-file limit is 0. This is best-effort; restricted environments (containers with `RLIMIT_CORE` hard limit 0) silently leave the limit at 0 and `ion-diagnostics` simply reports no cores found.

Core files and acquisition staging files (`bpacq*`, `ltpacq*`, `*.sdr`, `bsspSegment*`, `xnref*`, `*.sdrlog`) are removed by `cleanup_staging_files` after diagnostics finishes, so disk usage doesn't grow across a long campaign while still preserving `ion.log` and `ion-system.log` for inspection.

Stale `ion-system.log` files from prior runs are swept out of each test directory before the test starts, so they cannot be mistakenly uploaded by CI as if they belonged to the current run.

## Test retries and the retest file

When tests fail during a test campaign, `runtests` writes the names of all failed non-optional tests to a file called `retest` in the `tests` directory. Optional tests (marked with `.optional`) are never included.

Automatic retesting is **on by default**: every failed test is re-run once at the end of the campaign, and a test that passes on retest is counted as passed. To disable the retest pass and see the true first-run failure rate (e.g. when investigating flaky tests), set `ENABLE_RETEST=0`:

```bash
ENABLE_RETEST=0 ./runtests
```

The `retest` file is generated regardless of `ENABLE_RETEST`, so failed tests can be replayed manually at any time:

```bash
./runtests retest
```

## Environment variables

| Variable | Default | Effect |
|----------|---------|--------|
| `RUNTESTS_OUTPUTDIR` | unset | If set to a directory, per-test output is written to `<dir>/results.<testname>` instead of the terminal |
| `PRESERVE_TEST_LOGS` | `0` | When `1`, logs from passing tests are kept (failed-test logs are always preserved) |
| `ENABLE_RETEST` | `1` | When `1`, automatically re-runs failed tests once after the initial pass; set to `0` to skip the retest pass (flaky-test investigations) |
| `ION_TEST_TIMEOUT` | unset | Overrides the computed per-test timeout (seconds, uncapped) |
| `ION_MIN_DISK_MB` | `500` | Minimum free disk space (MB) required before each test; lower values abort the test as `ABORT (disk full)` |
| `ION_RUN_EXPERT` | unset | When non-empty, enables tests marked with `.exclude_expert` (e.g., BPSec) |
| `ION_NODE_LIST_DIR` | unset | Multi-node test isolation: directory where the `ion_nodes` file lives so `killm` can scope to a single node |
| `ION_DIAG_SINCE` | (set by `runtests`) | Epoch seconds; bounds `ion-diagnostics`' core-file scan to cores produced after this time |
