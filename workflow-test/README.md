# Workflow Test Suite

This directory contains tests specifically designed to validate CI/CD workflow behavior.
These tests are **NOT** part of the normal regression test suite and should only be run
manually for workflow validation purposes.

## Available Tests

### test-failed-ion-log
Tests the workflow's handling of failed tests and ion.log collection.
- **Purpose**: Validates that failed tests are properly detected and their ion.log files are collected with unique names
- **Expected Result**: Test fails and generates an ion.log that gets uploaded as an artifact
- **Usage**: `./runtests ../workflow-test/test-failed-ion-log`

### test-successful
Tests the workflow's handling of successful tests and ion.log collection.
- **Purpose**: Validates that successful tests have their ion.log files collected and uploaded
- **Expected Result**: Test passes and generates an ion.log that gets uploaded as an artifact
- **Usage**: `./runtests ../workflow-test/test-successful`

### test-timeout
Tests the workflow's timeout mechanism and ion.log collection from timed-out tests.
- **Purpose**: Validates that the workflow enforces timeouts and still collects ion.log files
- **Expected Result**: Test is killed by timeout, ion.log is still collected and uploaded
- **Usage**: Set `timeout_minutes` to a small value (1-2 minutes) and run `../workflow-test/test-timeout`
- **WARNING**: This test runs for 30 minutes if not killed by timeout!

## Running Workflow Tests

### Locally from the tests directory:
```bash
./runtests ../workflow-test/test-failed-ion-log
./runtests ../workflow-test/test-successful
# NOT recommended for timeout test without killm ready
```

### Via GitHub Actions workflow_dispatch:
Set the `tests_to_run` input parameter to one of:
```
../workflow-test/test-failed-ion-log
../workflow-test/test-successful
../workflow-test/test-timeout
```

For timeout testing, also set `timeout_minutes` to a small value like `2`.

## Creating New Workflow Tests

Follow the same structure as existing tests:
1. Create a subdirectory under `workflow-test/`
2. Add a `dotest` script (main test executable with `#!/usr/bin/env bash`)
3. Add any required ION configuration files
4. Add a README.md explaining the test purpose and expected behavior
5. Make `dotest` executable: `chmod +x dotest`
