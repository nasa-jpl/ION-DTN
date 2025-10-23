# Workflow Test: Failed Test with ion.log

## Purpose
This test intentionally fails and generates a mock ion.log file to validate that the CI/CD workflow correctly:
1. Detects test failures
2. Collects ion.log files from failed tests
3. Uploads them with unique names and timestamps
4. Displays failure information in the workflow output

**NOTE:** This test does NOT require ION to be installed. It creates a mock ion.log file for workflow validation only.

## Usage
To test a specific workflow's error handling:

```bash
# From the tests directory
./runtests ../workflow-test/test-failed-ion-log
```

Or trigger via workflow_dispatch:
- Set `tests_to_run` input to: `../workflow-test/test-failed-ion-log`

## Expected Behavior
- Test should appear in progress file as FAILED
- ion.log should be created with mock diagnostic messages
- Workflow should upload ion.log as: `workflow-test-test-failed-ion-log-ion.log`
- Build should fail (as expected)

## Files
- `dotest` - Main test script (executable, creates mock ion.log and exits with code 1)
- `README.md` - This file
