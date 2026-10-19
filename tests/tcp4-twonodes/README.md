# TCP4 Two-Node Test

## Purpose

Tests TCPCLv4 functionality between two ION nodes using the Python bridge adapter.

## Test Cases

### Test Case 1: Small Bundle
- Sends a text message using bpsource
- Verifies delivery via bpsink
- Confirms XFER_SUCCESS feedback and SDR cleanup

### Test Case 2: 6MB Bundle with BP Fragmentation
- Creates a 6MB random data file
- Sends via **bpsendfile** to test large bundle handling with fragmentation
- Receives via **bprecvfile** which handles large bundles
- Verifies:
  - BP fragmentation occurred (2 fragments transmitted)
  - Complete bundle received with correct size and checksum (MD5)
  - No "exceeds slot capacity" errors
  - SDR properly cleaned up on source node
  - Auto-configuration of maxPayloadLen with BP header margin
  - Automatic fragment reassembly by ION

## Expected Results

### With 5MB Slots (5MB - 1KB margin):
- **6MB bundle** → **2 fragments** (~5MB + ~1MB)
- Each fragment transmitted independently via TCPCLv4
- ION automatically reassembles fragments at receiver
- MD5 checksum matches source file
- No manual reassembly required

### Verification Points:

1. **Auto-Configuration:**
   ```
   [i] tcp4clo: Auto-set maxPayloadLen=5241856 (5 MB - 1024 B margin)
       to enable BP fragmentation for bundles > slot size.
   ```

2. **Fragmentation Confirmation:**
   ```
   SUCCESS: Fragmentation count matches expected!
   Fragments transmitted: 2
   ```

3. **No Slot Errors:**
   ```
   SUCCESS: No slot capacity errors (BP fragmentation working correctly)
   ```

4. **Correct Reception:**
   ```
   SUCCESS: 6MB bundle received correctly!
     Sent:     6291456 bytes, MD5=<hash>
     Received: 6291456 bytes, MD5=<hash>
     Files:    1 (fragments reassembled by ION)
   ```

5. **SDR Cleanup:**
   ```
   SUCCESS: Source SDR is clear after 6MB bundle transmission.
   ```

## Configuration

**Compile-Time (./configure):**
- Slot size: 5MB (configurable via `--with-tcp4-slot-size=5`)
- Num slots: 8 (configurable via `--with-tcp4-num-slots=8`)

**Runtime (.bprc):**
- **maxPayloadLen is automatically configured** by tcp4clo at startup
- Auto-configuration sets: `maxPayloadLen = slot_size - 1024 bytes`
- The 1KB margin accounts for BP bundle headers (~90-300 bytes per fragment)
- Example .bprc (auto-configuration):
  ```
  a outduct tcp4 peer3 'tcp4clo peer3'
  ```
- Manual override (optional):
  ```
  a outduct tcp4 peer3 'tcp4clo peer3' 5241856
  ```
- **Auto-configured values by slot size:**
  - 1MB slots: `1047552` (1MB - 1KB)
  - 5MB slots: `5241856` (5MB - 1KB, default)
  - 10MB slots: `10484736` (10MB - 1KB)

**ION Memory (.ionconfig):**
- wmSize: 262144000 (250MB)
- sdrWmSize: 262144000 (250MB)
- heapWords: 32768000 (250MB / 8 bytes per word)
- **Note**: Large bundles require sufficient heap space

**Contact Plan (global.ionrc):**
- Contact rate: 10000000 bytes/sec (10 MB/s)
- **Note**: Low contact rates will throttle transmission of large bundles

## Running the Test

```bash
cd tests/tcp4-twonodes
./dotest
```

**Expected output:**
```
Test Case 1: Small Bundle
SUCCESS: Bundle received correctly via TCP4+TCPCLv4!
SUCCESS: Source SDR is clear

Test Case 2: 6MB Bundle (BP Fragmentation)
Test file created: 6291456 bytes, MD5=<hash>
Starting bprecvfile on Node 3...

Checking tcp4clo configuration...
Looking for slot size and maxPayloadLen in ion.log:
[i] tcp4clo: Auto-set maxPayloadLen=5241856 (5 MB - 1024 B margin)
    to enable BP fragmentation for bundles > slot size.

Sending 6MB bundle from Node 2 to Node 3...
Expected: 2 fragments with 5MB slot size (5MB + 1MB)
Waiting for bundle transmission and reassembly...
Checking received 6MB bundle...
SUCCESS: 6MB bundle received correctly!
  Sent:     6291456 bytes, MD5=<hash>
  Received: 6291456 bytes, MD5=<hash>
  Files:    1 (fragments reassembled by ION)

BP fragmentation analysis:
  Fragments transmitted: 2
  Fragments reassembled: 1 file(s)
  Expected fragments: 2
SUCCESS: Fragmentation count matches expected!

SUCCESS: Source SDR is clear after 6MB bundle transmission.
SUCCESS: No slot capacity errors (BP fragmentation working correctly)
```

## Troubleshooting

### Bundle Size Mismatch

If received size doesn't match sent size:
- Check ion.log for transmission errors
- Verify sufficient time for transmission (increase sleep)
- Check bridge logs for errors

### No Fragmentation

If fragmentation count shows 1 instead of 2:
- Check ion.log for "Auto-set maxPayloadLen" message with margin
- Should show: `Auto-set maxPayloadLen=5241856 (5 MB - 1024 B margin)`
- Check for "exceeds slot capacity" errors
- Verify bundle size is larger than maxPayloadLen

### Slot Capacity Errors

If you see "exceeds slot capacity":
- **Cause**: maxPayloadLen was manually set equal to slot_size without margin
- **Solution**: Remove manual maxPayloadLen from .bprc to use auto-configuration
- Auto-configuration automatically includes 1KB margin for BP headers
- Verify: `grep "Auto-set maxPayloadLen" ion.log`

### Slow Transmission

If bundles take a very long time to transmit:
- Check contact plan rate in global.ionrc
- Should be at least 10000000 (10 MB/s) for testing
- Low rates will throttle bpclm transmission

## Files

| File | Purpose |
|------|---------|
| dotest | Main test script |
| cleanup | Cleanup script |
| global.ionrc | Shared contact plan (10 MB/s rate) |
| 2.ipn.tcp4/ | Node 2 configuration (sender) |
| 3.ipn.tcp4/ | Node 3 configuration (receiver) |
| 2.ipn.tcp4/amroc.ionconfig | Memory configuration (250MB) |
| 2.ipn.tcp4/amroc.bprc | BP configuration (auto maxPayloadLen) |
| 3.ipn.tcp4/amroc.ionconfig | Memory configuration (250MB) |
| 3.ipn.tcp4/amroc.bprc | BP configuration (auto maxPayloadLen) |
| testfile_6mb | Generated 6MB test file |
| testfile1 | Received bundle (reassembled by ION) |
