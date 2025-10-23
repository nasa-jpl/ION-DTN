# Workflow Test: Successful Test with ion.log

## Purpose
This test succeeds and generates a mock ion.log file to validate that the CI/CD workflow correctly:
1. Detects test success
2. Collects ion.log files from successful tests
3. Uploads them with unique names and timestamps
4. Displays success information in the workflow output

**NOTE:** This test does NOT require ION to be installed. It creates a mock ion.log file for workflow validation only.

## Usage
To test a specific workflow's success handling:

```bash
# From the tests directory
./runtests ../workflow-test/test-successful
```

Or trigger via workflow_dispatch:
- Set `tests_to_run` input to: `../workflow-test/test-successful`

## Expected Behavior
- Test should appear in progress file as PASSED (or not listed if only failures are shown)
- ion.log should be created with mock diagnostic messages
- Workflow should upload ion.log as: `workflow-test-test-successful-ion.log`
- Build should succeed

## Files
- `dotest` - Main test script (executable, creates mock ion.log and exits with code 0)
- `README.md` - This file
