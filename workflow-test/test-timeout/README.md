# Workflow Test: Timeout with ion.log

## Purpose
This test runs for an extended period (30 minutes) to validate that the CI/CD workflow correctly:
1. Detects and enforces test timeouts
2. Collects ion.log files from timed-out tests
3. Uploads them with unique names and timestamps
4. Displays timeout information in the workflow output

**NOTE:** This test does NOT require ION to be installed. It creates a mock ion.log file and runs in a loop for workflow validation only.

## Usage
To test a specific workflow's timeout handling:

```bash
# From the tests directory (NOT RECOMMENDED - will run for 30 minutes)
./runtests ../workflow-test/test-timeout
```

**Recommended:** Trigger via workflow_dispatch with a short timeout:
- Set `tests_to_run` input to: `../workflow-test/test-timeout`
- Set `timeout_minutes` input to: `2` (or any small value for quick testing)

## Expected Behavior
- Test should be killed by the workflow timeout mechanism
- ion.log should be created with periodic status updates
- Workflow should upload ion.log as: `workflow-test-test-timeout-ion.log`
- Build should fail due to timeout
- The log file should show multiple "Still running" entries up until timeout

## Testing Strategy
1. Set a short timeout (1-2 minutes) in the workflow
2. Run this test
3. Verify the workflow kills the test after the specified timeout
4. Verify the ion.log is still collected and uploaded

## Files
- `dotest` - Main test script (executable, creates mock ion.log and loops for 30 minutes)
- `README.md` - This file

## Warning
Do NOT run this test without a timeout configured - it will run for 30 minutes!
