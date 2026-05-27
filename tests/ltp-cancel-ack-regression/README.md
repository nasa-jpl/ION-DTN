# LTP Cancel Segment Acknowledgment Regression Test

## Purpose

This test verifies that the LTP engine properly acknowledges Cancel Segments regardless of session state, addressing the fix for issue where Cancel Segments received after sessions were completed were being ignored.

## Problem Background

Previously, when a session completed (either successfully or unsuccessfully), later received Cancel Segments would be ignored, causing:

- The peer to keep retransmitting Cancel Segments
- Unnecessary consumption of finite session resources
- Potential blocking of new sessions from being created

## Solution Tested

The fix ensures that the LTP engine always acknowledges Cancel Segments. Since Cancel-acknowledgment segments are smaller than Cancel segments, this prevents amplification attacks while allowing senders to tear down their session state immediately.

## Test Scenarios

1. **CS (Cancel by Source) for unknown session**: Verify CAS acknowledgment is sent
2. **CR (Cancel by Receiver) for unknown session**: Verify CAR acknowledgment is sent
3. **CS after data transfer**: Test acknowledgment for potentially completed session
4. **CR after data transfer**: Test acknowledgment for potentially completed session

## Test Components

- `dotest`: Main test script
- `runtest.py`: Python script that provides Cancel Segment generation, ACK reception, and bundle data generation
- `cleanup`: Test cleanup script
- Node configurations: 2-node LTP network setup

## Expected Results

- All Cancel Segments should receive immediate acknowledgments (< 1 second)
- No retransmission cycles should occur for Cancel Segments
- Normal LTP operations should remain unaffected

## Usage

```bash
cd tests/ltp-cancel-ack-regression
./dotest
```

## Success Criteria

The test passes if all injected Cancel Segments receive proper acknowledgments, regardless of whether the referenced sessions exist or have been completed.
