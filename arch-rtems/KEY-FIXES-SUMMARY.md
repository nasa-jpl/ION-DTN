# Key Adjustments Required for ION RTEMS 6.1 ARM64 UDP Networking

**Document Purpose:** Quick reference guide for critical fixes needed to achieve full UDP networking functionality

**Date:** November 7, 2025

**Status:** All issues resolved - Port fully operational

---

## Critical Fixes Summary

### 1. System Clock Initialization (REQUIRED)

**Issue:** RTEMS boots with default time of January 1, 1988, causing bundle timestamp corruption that prevents bundle forwarding.

**Symptoms:**
- Bundle timestamps show corrupted values (e.g., 18446744641703454)
- Bundles stuck in "Queued for proximate destination selection"
- ipnfw forwarder cannot process bundles with invalid timestamps

**Fix:** Initialize RTEMS system clock before starting ION

**Code Location:** [ionrtems.c:54-80](ionrtems.c#L54-L80)

```c
static void initClock()
{
    rtems_time_of_day tod;
    rtems_status_code sc;

    // Set to current date (adjust as needed for your application)
    tod.year   = 2025;
    tod.month  = 11;
    tod.day    = 7;
    tod.hour   = 0;
    tod.minute = 0;
    tod.second = 0;
    tod.ticks  = 0;

    sc = rtems_clock_set(&tod);
    assert(sc == RTEMS_SUCCESSFUL);
}
```

**Call from Init():** [ionrtems.c:455](ionrtems.c#L455)
```c
rtems_task Init(rtems_task_argument ignored)
{
    initClock();      // MUST be called BEFORE ION starts
    initNetwork();    // Then initialize networking
    startDTN();       // Finally start ION
}
```

---

### 2. Increase MAX_SPAWNS Limit (REQUIRED)

**Issue:** ION requires 15+ concurrent daemon tasks, but RTEMS platform default MAX_SPAWNS=8 allows only 8 concurrent spawns.

**Symptoms:**
- Error: "Can't spawn task: no parms cleared yet."
- Critical daemons fail to start:
  - bptransit (bundle transit)
  - ipnfw (IPN forwarder - CRITICAL for bundle forwarding)
  - ipnadminep (IPN administrative endpoint)
  - bpclm (convergence layer manager)
  - ltpcli (LTP input adapter)
  - ltpclo (LTP output adapter)
  - lgagent (logging agent)
- Only 8 daemons start, remaining 7+ fail
- Bundles cannot be forwarded (ipnfw not running)

**Fix:** Define MAX_SPAWNS=32 in build configuration

**Code Location:** [wscript:58](wscript#L58)

```python
iondefines=[
    'RTEMS',
    'BP_EXTENDED',
    # ... other defines ...
    'USING_RTEMS_LIBBSD',
    'MAX_SPAWNS=32'         # CRITICAL: Increase from default 8
]
```

**Verification:** After fix, all 15 daemons should start:
```
[i] rfxclock is running.
[i] ltpclock is running.
[i] ltpdeliv is running.
[i] ltpmeter is running.
[i] udplso is running.
[i] udplsi is running.
[i] bpclock is running.
[i] cpsd (stops if no multicast)
[i] bptransit is running.
[i] ipnfw is running.           <-- CRITICAL
[i] ipnadminep is running.
[i] bpclm is running.
[i] ltpcli is running.
[i] ltpclo is running.
[i] lgagent is running.
```

---

### 3. Add cpsd to Symbol Table (REQUIRED)

**Issue:** Contact Plan Synchronization Daemon (cpsd) not in RTEMS private symbol table.

**Symptoms:**
- Error: "Can't spawn task; function not in private symbol table; must be added to mysymtab.c. (cpsd)"
- Blocks other daemon spawns

**Fix:** Add cpsd extern declaration and symbol table entry

**Code Location:** [mysymtab.c:33,97](mysymtab.c#L33)

```c
// Add extern declaration at top of file
extern int cpsd(int, int, int, int, int, int, int, int, int, int);

// Add to symbol table array
static SymTabEntry symbols[] =
{
    // ... other entries ...
    { "cpsd",      (FUNCPTR) cpsd,       ICI_PRIORITY,  32768 },
    // ... more entries ...
};
```

---

### 4. RTEMS libbsd Network Initialization (REQUIRED for UDP)

**Issue:** UDP sockets require RTEMS libbsd networking stack to be initialized.

**Symptoms:**
- Error: "getaddrinfo failed for 127.0.0.1:1113 - Address resolution not supported"
- UDP services fail to bind

**Fix:** Initialize BSD networking stack before ION

**Code Location:** [ionrtems.c:36-52](ionrtems.c#L36-L52)

```c
static void initNetwork()
{
    rtems_status_code sc;
    int exit_code;

    sc = rtems_bsd_initialize();
    assert(sc == RTEMS_SUCCESSFUL);

    exit_code = rtems_bsd_ifconfig_lo0();
    assert(exit_code == EX_OK);
}
```

**Required Build Configuration:** [wscript:57](wscript#L57)
```python
iondefines=[
    # ...
    'USING_RTEMS_LIBBSD',   # Exclude socket stubs
    # ...
]
```

**Required Library Link Order:** [wscript:537](wscript#L537)
```python
lib=['rtemscpu', 'rtemsbsp', 'bsd', 'm']  # Order matters!
```

**Socket Stub Exclusion:** [rtems_stubs.c:88,262](rtems_stubs.c#L88)
```c
#ifndef USING_RTEMS_LIBBSD
// Socket stubs only when NOT using libbsd
int socket(...) { ... }
// ... other socket functions ...
#endif
```

---

### 5. RTEMS Configuration for libbsd (REQUIRED)

**Issue:** libbsd requires specific RTEMS resource configuration.

**Fix:** Add required RTEMS configuration macros

**Code Location:** [ionrtems.c:524-528](ionrtems.c#L524-L528)

```c
#define CONFIGURE_UNLIMITED_OBJECTS
#define CONFIGURE_UNLIMITED_ALLOCATION_SIZE     32
#define CONFIGURE_UNIFIED_WORK_AREAS
#define CONFIGURE_MAXIMUM_USER_EXTENSIONS       5
```

---

### 6. Increase ION Memory Allocations (REQUIRED for UDP)

**Issue:** UDP layer allocates 65KB buffers, original 200KB insufficient.

**Symptoms:**
- Error: "Not enough available memory. (65536)"
- Error: "udplsi can't get UDP buffer."

**Fix:** Increase memory allocations to 500KB

**Code Location:** [ionrtems.c:68-73](ionrtems.c#L68-L73)

```c
IonParms parms;
memset(&parms, 0, sizeof parms);

parms.wmSize = 500000;      // Increased from 200000
parms.sdrWmSize = 500000;   // Increased from 200000
parms.heapWords = 500000;   // Increased from 150000
```

---

## Quick Checklist for New Ports

When adapting this port to your BSP, ensure:

- [ ] System clock initialized to valid date before ION starts
- [ ] MAX_SPAWNS=32 (or higher) defined in wscript
- [ ] cpsd added to mysymtab.c
- [ ] USING_RTEMS_LIBBSD defined in wscript
- [ ] Library link order: rtemscpu, rtemsbsp, bsd, m
- [ ] Socket stubs conditionally excluded with `#ifndef USING_RTEMS_LIBBSD`
- [ ] RTEMS unlimited objects and unified work areas configured
- [ ] ION memory increased to 500KB (wmSize, sdrWmSize, heapWords)
- [ ] Network initialization called before ION starts
- [ ] BSP built with rtems-libbsd support

---

## Verification

**Expected Success Indicators:**

1. All 15 daemons start successfully (especially ipnfw)
2. No "Can't spawn task" errors
3. Timestamps show correct date (not 1988)
4. Bundle statistics show:
   - src: 1 bundle sourced
   - fwd: 1 bundle forwarded
   - xmt: 1 bundle transmitted
   - rcv: 1 bundle received
5. LTP statistics show:
   - Output segments: popped=1
   - Input segments: count=1
   - Checkpoints: transmitted=1, received=1
   - Sessions: completed=1
6. Console message: "ION event: Payload delivered."

---

## Debug Tips

**If bundles stuck in queue:**
- Check system clock is initialized (not 1988)
- Verify ipnfw daemon is running
- Check bundle timestamps are valid

**If daemons fail to start:**
- Increase MAX_SPAWNS beyond 32 if needed
- Check all required daemons are in mysymtab.c
- Use enhanced error messages to identify failing daemons

**If UDP services fail:**
- Verify libbsd initialized before ION
- Check USING_RTEMS_LIBBSD defined
- Ensure socket stubs excluded
- Verify library link order

---

## References

- [UDP-NETWORK-STATUS.md](UDP-NETWORK-STATUS.md) - Complete technical details
- [README](README) - Full port documentation
- [ion-qemu-output-udp-success.txt](ion-qemu-output-udp-success.txt) - Successful test output

---

**Document Version:** 1.0
**Last Updated:** November 7, 2025
**Applies To:** ION 4.1.5 RTEMS 6.1 ARM64 Port
