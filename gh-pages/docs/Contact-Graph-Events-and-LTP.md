# Contact Graph Events and LTP Interaction

## Overview

This document describes how ION's contact graph system manages communication opportunities and how these events interact with the Licklider Transmission Protocol (LTP) layer. Understanding this interaction is critical for implementing advanced features like adjacent contacts and dynamic rate management.

The contact graph serves two distinct roles. This page covers **rate control** — how contacts drive transmission rate through `rfxclock` into LTP. Its other role is **routing**: using the contact graph to select each bundle's next-hop neighbor. Routing is a separate concern that sits *above* rate control — see [Bundle Forwarding Architecture](./Bundle-Forwarding-Architecture.md).

## Fundamental Assumptions

The ION contact graph operates under two fundamental assumptions:

### 1. Synchronized Clocks with Bounded Error

All nodes in the network are assumed to have synchronized clocks with only small errors. The `maxClockError` parameter (default 1 second) represents the maximum expected clock synchronization error between any two nodes. This assumption allows:

- Predictable event timing across distributed nodes
- Coordinated transmission and reception windows
- Deterministic contact plan execution

Without reasonable clock synchronization, contact events may not align properly, leading to missed transmissions or rejected segments.

### 2. Transmitter Time Reference Frame

**Contact start and end times are defined in the transmitter's time reference frame, not the receiver's.**

For a contact from Node 1 to Node 2 with:
- Start time: t1
- End time: t2
- One-Way Light Time (OWLT): owlt

The actual signal arrival times at the receiver are:
- First signal arrives at receiver: t1 + owlt
- Last signal arrives at receiver: t2 + owlt

**Example:**
```
Contact: Node 1 -> Node 2
  fromTime = 100 seconds
  toTime = 200 seconds
  OWLT = 5 seconds

Transmitter (Node 1):
  Begins transmission at t=100
  Ends transmission at t=200

Receiver (Node 2):
  First signal arrives at t=105 (100 + 5)
  Last signal arrives at t=205 (200 + 5)
  Reception window: [105, 205]
```

This definition is critical for:
- **Reception window calculation**: The receiver must account for propagation delay when scheduling IonStartRecv/IonStopRecv events
- **Adjacent contact planning**: When planning adjacent contacts, the boundary time refers to the transmitter's timeline
- **Timer management**: LTP timers must account for round-trip light time when expecting acknowledgments

The RFX system automatically calculates appropriate reception times based on OWLT, ensuring the receiver accepts segments during the correct time window.

## Contact Graph Architecture

### Contact Representation

A contact in ION represents a communication opportunity between two nodes. Each contact is stored in two forms:

1. **Persistent storage (SDR)**: The authoritative contact definition including start time, end time, transmission rate, and node identifiers.

2. **Volatile index (IonCXref)**: A cached representation in shared memory for fast access by the rate/flow control (RFX) system.

Key IonCXref fields:
- `fromTime`, `toTime`: Contact duration
- `xmitRate`: Configured transmission rate (bytes/second)
- `startXmit`, `stopXmit`: Actual transmission start/stop times (may differ from fromTime/toTime due to maxClockError)
- `startFire`, `stopFire`: Timer management times for receiving node
- `startRecv`, `stopRecv`: Reception window times
- `purgeTime`: When to clean up the contact structure

### Timeline Events

The RFX system maintains a timeline (red-black tree) of events scheduled at specific times:

- **IonStartXmit**: Begin transmission on this contact (sets neighbor->xmitRate)
- **IonStopXmit**: End transmission (sets neighbor->xmitRate to 0)
- **IonStartFire**: Remote node can begin firing timers (sets neighbor->fireRate)
- **IonStopFire**: Remote node should suspend timers (sets neighbor->fireRate to 0)
- **IonStartRecv**: Begin accepting segments (sets neighbor->recvRate)
- **IonStopRecv**: Stop accepting segments (sets neighbor->recvRate to 0)
- **IonPurgeContact**: Clean up contact structure after use

These events are processed by rfxclock daemon, which dispatches them when their scheduled time arrives.

### Clock Error Compensation (maxClockError)

ION applies a safety margin (maxClockError, default 1 second) to account for clock synchronization uncertainty between nodes:

**Transmission side:**
- Start transmitting later: `startXmit = fromTime + maxClockError`
- Stop transmitting earlier: `stopXmit = toTime - maxClockError`

**Reception side:**
- Start accepting earlier: `startRecv = startFire - maxClockError`
- Stop accepting later: `stopRecv = stopFire + maxClockError`

This ensures segments arrive only when the remote node expects them, even with imperfect clock synchronization.

## LTP Integration

### LTP Volatile Span (LtpVspan)

LTP maintains per-neighbor state in the LtpVspan structure:

- `localXmitRate`: Current transmission rate for this span
- `remoteXmitRate`: Remote node's transmission rate (for timer calculations)
- `receptionRate`: Rate at which to accept incoming segments

### Rate Management in ltpclock

The ltpclock daemon periodically reads the RFX neighbor state and updates LTP span rates:

```c
// Simplified ltpclock logic
if (neighbor->xmitRate == 0) {
    if (vspan->localXmitRate > 0) {
        vspan->localXmitRate = 0;
        ltpStopXmit(vspan);  // Flush sessions, stop transmitter
    }
} else {
    if (vspan->localXmitRate == 0) {
        vspan->localXmitRate = neighbor->xmitRate;
        ltpStartXmit(vspan);  // Wake transmitter, give semaphores
    } else if (vspan->localXmitRate != neighbor->xmitRate) {
        // Rate changed (non-zero to different non-zero)
        vspan->localXmitRate = neighbor->xmitRate;
    }
}
```

Key operations:
- `ltpStartXmit()`: Wakes transmitter threads by giving semaphores
- `ltpStopXmit()`: Cancels in-progress export sessions if configured
- `ltpSuspendTimers()`: Pauses retransmission timers when remote rate goes to zero
- `ltpResumeTimers()`: Resumes timers when remote rate becomes non-zero

### LTP Screening and Purging Based on Contact Graph

LTP uses contact graph information to manage segment lifecycle and prevent resource exhaustion:

#### Segment Screening (Reception)

When an LTP segment arrives, it undergoes screening to determine if it should be accepted or rejected. The screening process checks:

1. **Reception window timing**: Is the current time within the contact's reception window (startRecv to stopRecv)?
2. **Span configuration**: Does an LTP span exist for the sending node?
3. **Session limits**: Are there available session resources?

**Contact graph integration:**
- The `neighbor->recvRate` (set by IonStartRecv/IonStopRecv events) determines if segments should be accepted
- When `recvRate = 0`, incoming segments are typically rejected or queued
- The reception window is calculated based on contact times plus OWLT and maxClockError

**Configuration (in ltprc):**
```
# LTP span configuration
a span <engine_id> <max_export_sessions> <max_import_sessions> ...

# Screening is automatic based on contact graph
# No explicit screening configuration needed
```

#### Segment Purging (Cleanup)

LTP periodically purges old segments and sessions to reclaim memory and prevent resource leaks. Purging is coordinated with contact graph events:

**Export session purging:**
- When `ltpStopXmit()` is called (contact ends), in-progress export sessions may be canceled
- Segments awaiting transmission are either:
  - Sent immediately if close to completion
  - Canceled and returned to bundle protocol for reforwarding
- Purge behavior depends on span configuration (purge flag)

**Import session purging:**
- Incomplete import sessions that exceed timeout are purged
- Sessions are retained as long as acknowledgments are expected (based on contact timing)
- When a contact ends (IonStopRecv), sessions without expected segments may be purged

**Checkpoint purging:**
- After a session completes and is acknowledged, its checkpoint data is purged
- Purge timing considers round-trip light time to ensure acknowledgments are received

**Configuration (in ltprc):**
```
a span <engine_id> <max_export> <max_import> <max_segment_size> \
      <aggregation_size_limit> <aggregation_time_limit> \
      '<LSO_command>' <max_timeouts> <LSI_command> [purge]

# The optional 'purge' flag at the end controls export session cancellation:
# - If 'purge' is specified: Cancel sessions when contact ends (ltpStopXmit)
# - If omitted: Allow sessions to complete even after contact ends
```

**Purge timing and contact graph:**

The contact graph's IonPurgeContact event (scheduled at purgeTime) triggers cleanup of the contact structure itself, but LTP purging is driven by:

1. **Timer expirations**: Sessions that exceed retransmission timeout limits
2. **Contact end events**: IonStopXmit/IonStopFire trigger session evaluation
3. **Resource pressure**: When session limits are reached, oldest sessions may be purged

**Best practices:**
- Set appropriate `max_export_sessions` and `max_import_sessions` based on contact duration and traffic volume
- Use the `purge` flag for short contacts where sessions are unlikely to complete
- Omit `purge` for long contacts to allow session completion after contact ends
- Ensure `aggregation_time_limit` is shorter than contact duration to prevent unsent data

**Impact on adjacent contacts:**

For adjacent contacts, proper purging configuration prevents:
- Resource exhaustion when transitioning between contacts
- Session conflicts between old and new contacts to the same neighbor
- Segment rejections due to exceeded session limits

When using adjacent contacts with the `purge` flag:
- Sessions from Contact 1 are canceled when it ends
- Contact 2 starts with a clean slate of available session resources
- No session ID conflicts between adjacent contacts

When using adjacent contacts without `purge`:
- Sessions from Contact 1 may continue during Contact 2
- Session resources must be sufficient for overlapping sessions
- Useful when data volume exceeds single contact capacity

## Adjacent Contacts

### Definition

Adjacent contacts are contacts where one contact's end time exactly equals the next contact's start time:

```
Contact A: [fromTime=10, toTime=60]
Contact B: [fromTime=60, toTime=110]  // Adjacent: A.toTime == B.fromTime
```

### Challenges

Without special handling, adjacent contacts cause transmission interruption:

1. IonStopXmit fires at time 60, sets neighbor->xmitRate to 0
2. ltpclock sees xmitRate=0, calls ltpStopXmit()
3. IonStartXmit fires at time 60, sets neighbor->xmitRate to new value
4. But LTP already stopped transmission, causing gap

Additional complexity:
- IonPurgeContact for Contact A also fires at time 60
- deleteContact() might zero neighbor rates, overwriting Contact B's rates

### Solution Components

#### 1. Timeline Event Management (rfx.c)

When inserting a contact, detect if it's adjacent to an existing contact:

```c
adjacentContact = findAdjacentContact(..., &isPreceding, &adjacentAddr);

if (adjacentContact && isPreceding) {
    // This contact starts where another ends
    cxref->startXmit = cxref->fromTime;  // No maxClockError

    // Update preceding contact's Stop events
    updateAdjacentStopEvents(vdb, adjacentContact, adjacentCxaddr,
                             cxref->fromTime);
}
```

`updateAdjacentStopEvents()` performs:
- Delete old IonStopXmit/IonStopFire events from timeline (had maxClockError applied)
- Update cxref's stopXmit/stopFire fields to exact boundary time
- Create new Stop events at exact boundary time

This ensures Stop and Start events occur at the same timestamp.

#### 2. Event Dispatcher Lookahead (rfxclock.c)

When processing Stop events, check if adjacent Start event exists:

```c
case IonStopXmit:
    if (findAdjacentStartEvent(vdb, event->time, IonStartXmit,
                                cxref->fromFqnn, cxref->toFqnn)) {
        // Adjacent Start found at same time, skip zeroing rate
        sm_rbt_delete(ionwm, vdb->timeline, rfx_order_events,
                      event, rfx_erase_data, NULL);
        return 0;
    }

    // No adjacent contact, proceed with normal stop
    neighbor->xmitRate = 0;
    // ...
```

The lookahead prevents rate from being set to zero when an adjacent Start event will immediately set it to a non-zero value.

#### 3. Contact Deletion Protection (rfx.c)

Modified deleteContact() to avoid interfering with adjacent contacts:

```c
// Only zero rates if contact terminated EARLY
if (currentTime >= cxref->startXmit && currentTime < cxref->stopXmit) {
    neighbor->xmitRate = 0;  // Early termination, zero rate
}
// If currentTime == stopXmit (normal termination), don't zero
// Let IonStop events handle rate transitions
```

This change distinguishes between:
- **Early termination**: Contact manually deleted before natural end (zero rates immediately)
- **Normal termination**: Contact ends at scheduled time (let events handle it)

#### 4. LTP Rate Updates (ltpclock.c)

Enhanced ltpclock to handle rate changes between non-zero values:

```c
else if (vspan->localXmitRate != neighbor->xmitRate) {
    // Rate changed from one non-zero value to another
    vspan->localXmitRate = neighbor->xmitRate;
}
```

Previously, LTP only handled 0-to-non-zero transitions. Adjacent contacts with different rates require non-zero-to-different-non-zero transitions.

### Event Sequence for Adjacent Contacts

Example: Two adjacent contacts at time T=60, Contact A (1 MB/s) ending, Contact B (2 MB/s) starting.

**At time T=60:**

1. **IonStopXmit for Contact A**: Lookahead finds IonStartXmit at same time, skips zeroing, deletes event
2. **IonStopFire for Contact A**: Lookahead finds IonStartFire, skips zeroing, deletes event
3. **IonStartXmit for Contact B**: Sets neighbor->xmitRate = 2000000
4. **IonStartFire for Contact B**: Sets neighbor->fireRate = 2000000, schedules IonStartRecv/IonStopRecv
5. **IonPurgeContact for Contact A**: Calls deleteContact(), sees currentTime == stopXmit, skips zeroing rates

**ltpclock sees:**
- neighbor->xmitRate changed from 1000000 to 2000000
- Condition: `vspan->localXmitRate != neighbor->xmitRate` triggers
- Updates: `vspan->localXmitRate = 2000000`
- LTP transmitter continues seamlessly at new rate

## Timing Considerations

### Event Ordering

Events at the same timestamp are processed in the order they appear in the timeline red-black tree. The tree uses `rfx_order_events()` comparator which orders by:
1. Time (primary)
2. Event type (secondary, if time equal)
3. Contact reference (tertiary)

For adjacent contacts, the exact order of Stop/Start events at the boundary may vary, which is why the lookahead mechanism is necessary.

### Race Conditions

The ltpclock daemon runs independently from rfxclock. There's a potential race where ltpclock reads neighbor state between Stop and Start events:

**Without lookahead:**
```
T=60.000: IonStopXmit sets neighbor->xmitRate = 0
T=60.001: ltpclock reads neighbor->xmitRate = 0, calls ltpStopXmit()
T=60.002: IonStartXmit sets neighbor->xmitRate = 2000000
Result: LTP stopped, transmission interrupted
```

**With lookahead:**
```
T=60.000: IonStopXmit skipped (adjacent Start detected)
T=60.001: IonStartXmit sets neighbor->xmitRate = 2000000
T=60.002: ltpclock reads neighbor->xmitRate = 2000000, updates vspan
Result: Seamless continuation
```

### Purge Event Timing

IonPurgeContact fires after a contact naturally ends:
- For local transmission: `purgeTime = stopXmit` (or later)
- For non-local: `purgeTime = toTime`

The purge event should occur after all Start/Stop events are processed. By checking `currentTime < stopXmit` instead of `currentTime <= stopXmit`, we avoid zeroing rates at the exact boundary when adjacent contacts may be active.

## Implementation Details

### Finding Adjacent Contacts

`findAdjacentContact()` searches the contact index for contacts to the same node pair:

```c
// Iterate all contacts in index
for (cxelt = sm_rbt_first(ionwm, vdb->contactIndex); cxelt; ...) {
    cxref = (IonCXref *) psp(ionwm, sm_rbt_data(ionwm, cxelt));

    // Check if existing contact ends when new one begins
    if (cxref->toTime == fromTime) {
        *isPreceding = 1;
        return cxref;  // Found preceding adjacent contact
    }

    // Check if existing contact begins when new one ends
    if (cxref->fromTime == toTime) {
        *isPreceding = 0;
        return cxref;  // Found following adjacent contact
    }
}
```

### Updating Timeline Events

`updateAdjacentStopEvents()` must delete and recreate events because:
1. Events in the timeline are indexed by time
2. Simply changing the time field would corrupt the red-black tree structure
3. Must delete old event, update cxref, create new event

```c
// Delete old event
oldEvent.time = precedingCxref->stopXmit;
oldEvent.type = IonStopXmit;
sm_rbt_delete(ionwm, vdb->timeline, rfx_order_events, &oldEvent, ...);

// Update cxref field
precedingCxref->stopXmit = boundaryTime;

// Create new event
newEvent->time = boundaryTime;
newEvent->type = IonStopXmit;
sm_rbt_insert(ionwm, vdb->timeline, addr, rfx_order_events, newEvent);
```

### Lookahead Implementation

`findAdjacentStartEvent()` searches the timeline for Start events at the same time:

```c
for (elt = sm_rbt_first(ionwm, vdb->timeline); elt; elt = sm_rbt_next(...)) {
    candidateEvent = (IonEvent *) psp(ionwm, addr);

    if (candidateEvent->time > eventTime) break;  // Past boundary
    if (candidateEvent->time < eventTime) continue;  // Before boundary

    // At exact boundary time
    if (candidateEvent->type == startType) {
        candidateCxref = (IonCXref *) psp(ionwm, candidateEvent->ref);
        if (candidateCxref->fromFqnn == fromFqnn &&
            candidateCxref->toFqnn == toFqnn) {
            return 1;  // Adjacent Start found
        }
    }
}
```

## Use Cases

### Sequential Coverage

Maintain continuous communication through sequential contacts:

```
Contact 1: Ground Station A, 100-200 seconds, 1 MB/s
Contact 2: Ground Station B, 200-300 seconds, 2 MB/s (adjacent)
Contact 3: Ground Station C, 300-400 seconds, 1.5 MB/s (adjacent)
```

Benefits:
- No transmission gaps between contacts
- Automatic rate adaptation as link quality changes
- Smooth handoffs between ground stations

### Rate Adaptation

Change transmission rates without interrupting sessions:

```
Contact 1: 500 KB/s (distant, low signal)
Contact 2: 2 MB/s (adjacent, closer, better signal)
```

LTP sessions continue across the boundary with updated transmission rates.

### Link Handoff

Seamlessly switch between different communication links:

```
Contact 1: UHF link, 128 KB/s
Contact 2: S-band link, 1 MB/s (adjacent, better antenna pointing)
```

## Configuration

### Enabling Adjacent Contacts

Set maxClockError to 0 in ionrc to use exact contact boundaries:

```
m clockerr 0
```

This disables the safety margin, allowing contacts to be truly adjacent.

### Contact Plan Example

```
# Two adjacent contacts with different rates
a contact +10 +60 1 2 1000000
a contact +60 +2000 1 2 2000000

# Range for both contacts
a range +0 +2000 1 2 1
```

### Debugging

Enable DEBUG_RFX at compile time to see detailed event processing:

```bash
./configure CFLAGS="-DDEBUG_RFX"
make
```

Debug output shows:
- Adjacent contact detection during insertion
- Timeline event updates for adjacent boundaries
- Stop event skipping when adjacent Start found
- LTP rate changes
- Contact deletion decisions

## Performance Considerations

### Memory Impact

Adjacent contact support adds minimal memory overhead:
- One additional pointer in `findAdjacentContact()` return
- No additional persistent storage

### CPU Impact

Additional processing during contact insertion:
- O(n) search through contact index to find adjacent contacts
- O(log n) timeline event deletion and recreation for preceding contact
- Negligible compared to overall contact management cost

### Timeline Event Processing

Lookahead adds O(n) search through timeline events at the same timestamp. In practice:
- Few events occur at exactly the same time
- Search terminates quickly (typically 2-5 events checked)
- Only performed for Stop events, not Start events

## Limitations and Considerations

### Clock Synchronization

Adjacent contacts work best with accurate clock synchronization between nodes. Without synchronization:
- Events may not occur at exact boundary time
- Small gaps or overlaps may occur
- Consider using maxClockError > 0 for imperfect synchronization

### Contact Planning

When planning adjacent contacts:
- Ensure toTime of one contact exactly equals fromTime of next
- Use consistent time references across all nodes
- Account for propagation delays in contact definitions

### Same-Rate Adjacent Contacts

Adjacent contacts with identical rates (e.g., 1 MB/s to 1 MB/s) are supported but provide limited benefit since the transmission rate doesn't change. The seamless transition still prevents brief gaps.

### Multiple Adjacent Chains

Multiple contacts can be chained:

```
Contact A: [10, 60]
Contact B: [60, 110]  // Adjacent to A
Contact C: [110, 160] // Adjacent to B
```

Each adjacency is handled independently during contact insertion.

## Future Enhancements

Potential improvements to the adjacent contact system:

1. **Automatic adjacency detection in ionadmin**: Parse contact plans and automatically adjust maxClockError for detected adjacent contacts

2. **Per-contact maxClockError**: Allow different clock error margins for different contacts based on link characteristics

3. **Graceful degradation**: If adjacent Start event doesn't fire as expected, automatically zero rates after timeout

4. **Statistics**: Track successful adjacent transitions vs. gaps for monitoring contact plan quality

## Summary

ION's contact graph and LTP interaction provides robust management of communication opportunities. The adjacent contact feature extends this capability to support seamless transitions between sequential contacts, enabling:

- Continuous transmission across changing link conditions
- Dynamic rate adaptation without session interruption
- Efficient use of sequential communication opportunities

The implementation carefully coordinates between the contact graph event system (rfx.c, rfxclock.c) and the LTP rate management system (ltpclock.c) to ensure consistent state and prevent timing-related race conditions.
