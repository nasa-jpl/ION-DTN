# Running the ION test set

## Directory layout

The `tests` directory under ION's root folder contains the test suite. Each test lives in its own subdirectory of this directory. Each test is conducted by a script `$TESTNAME/dotest`. Another directory that contains ION tests is the `demos` directory, which includes examples of ION configurations using different convergence layers. For this document, we focus on the usage of the `tests` directory.

## Exclude files

Exclude files are hidden files that allow for tests to be disabled based on certain conditions that may cause the test not to run correctly. If an exclude file exists, it should have a short message about why the test has been excluded.

Exclude files can exist in any of the following formats:

- `.exclude_OS-TYPE`: Disables a test for an operating system that it does not run successfully on. Acceptable values to fill in for OS-TYPE are "windows", "linux", "mac", and "solaris".

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

## Writing new tests

A test directory must contain an executable file named `dotest`.  If a directory does not contain this, the test will be ignored. The `dotest` program should execute the test, possibly reporting runtime information on stdout and stderr, and indicate by its return value the result of the test as follows:

    0: Success
    1: Failure
    2: Skip this test

The test program starts without the ION stack running. The test program is responsible for starting ION in the way that is appropriate for the test.

The test program *must* stop the ION protocol stack before returning.

## The test environment

The `dotest` scripts are run in their test directory. The following environment variables are set as part of the test environment:

- `IONDIR` is the root of the local ION source directory.

- `PATH` has `IONDIR` prepended to it, ensuring ION executables in the local build directory are found first.

## Test Progress Tracking (ION 4.1.3 and later)

Starting with ION version 4.1.3, the `runtests` script maintains a file called `tests/progress` that records the start time, finish time, and final result for each test.

If the environment variable `RUNTESTS_OUTPUTDIR` is set, as in `export RUNTESTS_OUTPUTDIR="/tmp"`, then the output from each test will be stored in individual files like `/tmp/results.testname` (e.g., `/tmp/results.1000.loopback`), which makes it much easier to find particular text or results when debugging.

## Test retries and the retest file

When tests fail during a test campaign, `runtests` automatically generates a file called `retest` in the `tests` directory. This file contains the names of all failed non-optional tests. After the initial test run completes, `runtests` will automatically attempt to re-run all tests listed in the `retest` file once more. This automatic retry mechanism helps identify intermittent failures and reduces false positives from timing-sensitive tests.

Note that optional tests (marked with `.optional`) are never included in the `retest` file and are not retested automatically, since their failure does not affect the overall test campaign status.
