# UDP Network Integration Status - ION RTEMS 6.1 ARM64 Port

**Date:** November 7, 2025
**Status:** Network stack initialized successfully, LTP transmission debugging in progress

## Summary

Successfully integrated RTEMS libbsd networking stack with ION. The BSD network stack initializes correctly, UDP services bind to loopback interface, and all ION services start successfully. Bundle sourcing is verified, but LTP segment transmission through UDP requires further investigation.

## Achievements

### 1. RTEMS libbsd Integration
- **Network Stack Initialization**: `rtems_bsd_initialize()` successful
- **Loopback Interface**: `rtems_bsd_ifconfig_lo0()` configured, link state UP
- **Socket API**: Full BSD socket support available

### 2. Build System Updates
- **Conditional Compilation**: Added `USING_RTEMS_LIBBSD` define to exclude socket stubs
- **Socket Stubs**: Properly excluded when linking with libbsd via `#ifndef USING_RTEMS_LIBBSD`
- **Library Linking**: Correct link order: `rtemscpu`, `rtemsbsp`, `bsd`, `m`
- **Clean Build**: No compilation or linking errors

### 3. RTEMS Configuration
- **Unlimited Objects**: `CONFIGURE_UNLIMITED_OBJECTS` enabled
- **Allocation Size**: `CONFIGURE_UNLIMITED_ALLOCATION_SIZE 32`
- **Unified Work Areas**: `CONFIGURE_UNIFIED_WORK_AREAS` for libbsd compatibility
- **User Extensions**: `CONFIGURE_MAXIMUM_USER_EXTENSIONS 5` for libbsd

### 4. ION Memory Configuration
- **Working Memory**: Increased to 500KB (from 200KB)
- **Heap Words**: Increased to 500,000 words (from 150,000)
- **SDR Working Memory**: Increased to 500KB (from 200KB)
- **Rationale**: Support UDP buffer allocations (65KB per buffer)

### 5. UDP Services Running
```
[1988/01/01-00:00:00] [i] udplso is running, local=0.0.0.0:26171 (IPv4), peer=127.0.0.1:1113 (IPv4), rengine=19.
[1988/01/01-00:00:00] [i] udplsi is running, listening on 127.0.0.1:1113 (IPv4).
```
- UDP Link Service Output (udplso) bound successfully
- UDP Link Service Input (udplsi) listening on 127.0.0.1:1113
- No socket initialization errors

### 6. Network Interface Status
```
info: lo0: link state changed to UP
```
- Loopback interface operational
- Network device created: `nexus0: <RTEMS Nexus device>`

## Current Issue

### Bundle Transmission Not Completing

**Observation:**
- Bundle sourced successfully: `src: (1) 1 13` (1 bundle, 13 bytes)
- No LTP segments transmitted: `Output segments: popped=0 bytes=0`
- Export session created but not completed: `Sessions: export=1 import=0 completed=0`
- No transmission counters incremented: `xmt: (0) 0 0`, `rcv: (0) 0 0`

**Statistics from Test:**
```
Bundle Statistics:
  src from 1970/01/01-00:00:00 to 1988/01/01-00:00:08: (0) 0 0 (1) 1 13 (2) 0 0 (+) 1 13
  xmt from 1970/01/01-00:00:00 to 1988/01/01-00:00:08: (0) 0 0 (1) 0 0 (2) 0 0 (+) 0 0
  rcv from 1970/01/01-00:00:00 to 1988/01/01-00:00:08: (0) 0 0 (1) 0 0 (2) 0 0 (+) 0 0
  dlv from 1970/01/01-00:00:00 to 1988/01/01-00:00:08: (0) 0 0 (1) 0 0 (2) 0 0 (+) 0 0

LTP Statistics:
  Output segments: popped=0 bytes=0
  Checkpoints transmitted: 0
  Sessions: export=1 import=0 completed=0
```

**Possible Causes:**
1. **ION Loopback Optimization**: Same-node bundles might bypass convergence layer
2. **Routing Issue**: Bundle may not be routed to LTP outduct
3. **LTP State Machine**: Export session not transitioning to transmission state
4. **Contact/Range Timing**: Contact window or range configuration issue
5. **BP Transit Daemon**: Not pulling bundles from queue for transmission

## Test Configuration

### ION Configuration (ionrtems.c)
```c
// Node configuration
uvast nodenbr = 19;

// ION memory parameters
parms.wmSize = 500000;
parms.sdrWmSize = 500000;
parms.heapWords = 500000;

// Contact and range
ion_add_contact(now + 1, now + 7200, nodenbr, nodenbr, 100000, 1.0);
ion_add_range(now + 1, now + 7200, nodenbr, nodenbr, 1);

// LTP span configuration
add_span(19, 100, 100, 1400, 10000, 1, "udplso 127.0.0.1:1113", 1, 0);
add_seat("udplsi 127.0.0.1:1113");

// BP routing
add_protocol("ltp", 0);
add_induct("ltp", "19", "ltpcli");
add_outduct("ltp", "19", "ltpclo", 0);
add_plan("ipn:19.0", 0);
add_planduct("ipn:19.0", "ltp", "19");
```

### Test Procedure
```c
// Start bpsink to receive bundles
bpsink ipn:19.1

// Send bundle to local endpoint
bpsource ipn:19.1 'Hello, world.'

// Wait 5 seconds and check statistics
bpstats
```

## File Changes Summary

### Modified Files

1. **[wscript](wscript:57)**
   - Added `'USING_RTEMS_LIBBSD'` to iondefines
   - Changed UDP sources from pmql to udp
   - Updated library link order

2. **[rtems_stubs.c](rtems_stubs.c:88-262)**
   - Added `#ifndef USING_RTEMS_LIBBSD` guard around socket functions
   - Stubs excluded when linking with libbsd

3. **[ionrtems.c](ionrtems.c:35-51)**
   - Added `initNetwork()` function with BSD stack initialization
   - Increased ION memory allocations
   - Changed LTP span from pmqlso/pmqlsi to udplso/udplsi

4. **[ionrtems.c](ionrtems.c:490-493)**
   - Added `CONFIGURE_UNLIMITED_OBJECTS`
   - Added `CONFIGURE_UNLIMITED_ALLOCATION_SIZE 32`
   - Added `CONFIGURE_UNIFIED_WORK_AREAS`
   - Added `CONFIGURE_MAXIMUM_USER_EXTENSIONS 5`

5. **[mysymtab.c](mysymtab.c:24-25,85-86)**
   - Added udplsi and udplso function exports
   - Removed pmqlsi and pmqlso function exports

### New Files

- **ion-qemu-output-udp.txt**: Current test output with UDP networking

## Known Warnings (Non-Critical)

1. **`[zone: unpcb] kern.ipc.maxsockets limit reached`**
   - libbsd socket limit warning during initialization
   - Does not prevent operation
   - Can be addressed with libbsd configuration if needed

2. **`/ion directory warning`**
   - Expected in RTEMS (no filesystem required)
   - SDR operates in DRAM mode

3. **`cpsd function not in symbol table`**
   - Contact plan synchronization daemon
   - Non-critical for single-node testing
   - Multiple "Can't spawn task" warnings related to this

## Next Steps for Investigation

### 1. Verify Contact Timing
- Check if contact window is active during bundle transmission
- Verify contact and range are properly configured

### 2. Check BP Routing
- Verify bundle is queued for transmission to LTP outduct
- Inspect BP transit daemon activity
- Check if bundle is reaching ltpclo (LTP convergence layer output)

### 3. Investigate LTP Export Session
- Determine why export session is created but segments not transmitted
- Check LTP state machine transitions
- Verify ltpclo is processing queued data

### 4. Test with Explicit Routing
- Try sending to different destination to rule out loopback optimization
- Test with different priority levels
- Add debug logging to LTP layer

### 5. Memory and Resource Check
- Verify no resource exhaustion in LTP or BP layers
- Check for blocked tasks or semaphores
- Monitor ION memory usage during test

## References

### Documentation
- [RTEMS libbsd documentation](https://gitlab.rtems.org/rtems/pkg/rtems-libbsd/-/blob/6-freebsd-12/README.rst)
- [ION Admin API documentation](../README-admin-api.md)
- [LTP specification](https://tools.ietf.org/html/rfc5326)

### Test Output Files
- `ion-qemu-output-udp.txt` - Current UDP networking test
- `ion-qemu-output.txt` - Previous PMQL test (for comparison)

### Key Code Locations
- Network initialization: [ionrtems.c:35-51](ionrtems.c#L35-L51)
- ION configuration: [ionrtems.c:53-312](ionrtems.c#L53-L312)
- LTP span setup: [ionrtems.c:153-166](ionrtems.c#L153-L166)
- BP routing setup: [ionrtems.c:250-281](ionrtems.c#L250-L281)
- Test procedure: [ionrtems.c:352-372](ionrtems.c#L352-L372)

## Success Criteria

For complete UDP networking verification, we need:

1. [DONE] Network stack initialized
2. [DONE] UDP services bound and listening
3. [DONE] Bundle sourced successfully
4. [TODO] LTP segments transmitted (export session completes)
5. [TODO] LTP segments received (import session created)
6. [TODO] Bundle delivered to local application
7. [TODO] Statistics show non-zero xmt/rcv counts

**Current Progress: 3/7 criteria met (43%)**

---

*This document will be updated as debugging progresses.*
