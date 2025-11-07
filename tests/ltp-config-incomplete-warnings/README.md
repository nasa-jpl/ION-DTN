# LTP Incomplete Configuration Warnings Test

## Purpose

This regression test verifies that when LTP spans are created without explicit manage commands, the system:

1. Automatically initializes spans with global default values
2. Issues clear informational messages showing the applied defaults
3. Shows specific default values (maxRetries=5, maxSegmentLossRate=0.01)
4. Remains functional with the auto-applied defaults
5. Uses explicit configuration mode (not legacy maxBER mode)

## Test Scenario

The test configures a single ION node (node 1) with an LTP span to a remote engine (engine 2). The LTP configuration file contains **no manage commands**:

- **No `m maxretries` command**
- **No `m maxseglossrate` command**
- **Expected:** Span uses global defaults (5 retries, 0.01 loss rate)

This tests the automatic default initialization implemented in `libltpP.c`.

## Expected Behavior

### Initialization Messages
The test expects to see the following informational message in `ion.log`:

```
[i] Span 2 initialized with global unified mode defaults: maxRetries=5, maxSegmentLossRate=0.0100
```

### Configuration Applied
After applying defaults:
- `maxRetries = 5` (global default)
- `maxSegmentLossRate = 0.01` (global default)
- `maxTimeouts = 5` (computed as maxRetries × SIGNAL_REDUNDANCY)
- Explicit configuration mode enabled
- Unified mode (not split mode)

### System Functionality
- ION starts successfully without errors
- LTP engine daemons (ltpclock, ltpmeter) run correctly
- Span to engine 2 is raised successfully
- No session failures or premature cancellations

## Running the Test

```bash
cd tests/ltp-config-incomplete-warnings
./dotest
```

## Expected Output

```
Starting LTP automatic default configuration test...
...
=== Checking for default configuration initialization ===
✓ PASS: Span initialization with defaults message detected
✓ PASS: Default maxRetries=5 shown in initialization message
✓ PASS: Default maxSegmentLossRate=0.0100 shown in initialization message

=== Checking for correct configuration application ===
✓ PASS: Explicit configuration mode confirmed
✓ PASS: Max timeouts = 5 confirmed (5 retries × 1)
✓ PASS: Segment loss rate = 0.01 (default) confirmed

=== Checking system functionality ===
✓ PASS: No errors in ION startup
✓ PASS: LTP engine daemons are running
✓ PASS: Span to engine 2 raised successfully

=== Test Summary ===
✓✓✓ ALL TESTS PASSED ✓✓✓
```

## Test Validation Checks

The test performs 9 validation checks:

1. Span initialization with defaults message is issued
2. Default value maxRetries=5 is shown in message
3. Default value maxSegmentLossRate=0.0100 is shown in message
4. Explicit configuration mode is active
5. Computed max timeouts value is correct (5)
6. Segment loss rate matches default (0.01)
7. No errors in ION startup
8. LTP engine daemons are running
9. Span to engine 2 is raised successfully

## Cleanup

```bash
./cleanup
```

Or the test will automatically clean up after completion.

## Related Code

This test validates the changes made to:
- `ltp/library/libltpP.c`: Function `raiseSpan()` (lines ~625-745)
  - Auto-applies global defaults to incomplete configurations
  - Issues specific warnings for each missing parameter
  - Provides clear guidance messages

## Test Variations

Future enhancements could test:
- **Split mode incomplete**: Only some of the 4 split-mode parameters configured
- **Multiple missing parameters**: Both unified mode parameters missing
- **Per-span incomplete**: Span-specific overrides with missing parameters
