# Compressed Bundle Reporting and Custody Transfer Tutorial

This tutorial introduces the Compressed Bundle Reporting (CBR) and Custody Transfer (CT) feature in ION, which implements the CCSDS Orange Book specification "Compressed Bundle Status Reporting and Custody Signaling."

## Overview

CBR/CT provides two key capabilities for DTN networks:

1. **Compressed Reporting Signals (CRS)**: A more efficient alternative to traditional BPv7 status reports, allowing multiple bundle status events to be aggregated into a single administrative message.

2. **Custody Transfer with Compressed Custody Signals (CCS)**: A mechanism for reliable bundle delivery through hop-by-hop custody transfer, where each node explicitly accepts responsibility for bundle delivery.

### Why Use CBR/CT?

- **Reduced Overhead**: CRS aggregates multiple status reports into a single message, reducing administrative traffic
- **Reliable Delivery**: Custody transfer ensures bundles are retransmitted if they don't reach the next custodian
- **Interoperability**: Follows the CCSDS Orange Book specification for compatibility with other implementations
- **Efficient Encoding**: Uses CBOR (Concise Binary Object Representation) for compact wire format

## Prerequisites

Before using CBR/CT, ensure you have:

1. ION built and installed (version 4.2 or later with CBR/CT support)
2. Basic familiarity with ION configuration files (see [Basic Configuration Tutorial](./Basic-Configuration-File-Tutorial.md))
3. At least two ION nodes configured for communication

## Configuration

### Enabling CBR/CT Mode

Add the following command to your node's `bprc` file to enable Orange Book custody mode:

```
m custodymode orangebook
```

Available custody modes are:
- `none` - No custody transfer (default)
- `bibe` - Bundle-In-Bundle Encapsulation mode (reserved for future use)
- `orangebook` - CCSDS Orange Book CBR/CT mode

### Configuring Signal Aggregation

CBR/CT can aggregate multiple signals into a single administrative bundle to reduce overhead. Configure aggregation with the `m cbraggr` command in your `bprc` file:

```
m cbraggr <crs_limit> <ccs_limit> <timeout_seconds>
```

Parameters:
- `crs_limit` - Maximum bundle sequence entries per CRS before sending
- `ccs_limit` - Maximum bundle sequence entries per CCS before sending
- `timeout_seconds` - Maximum seconds to wait before flushing pending signals

**Example configurations:**

For testing (immediate signal transmission):
```
m cbraggr 1 1 2
```

For production (aggregate up to 100 bundles):
```
m cbraggr 100 50 30
```

### Complete Node Configuration Example

Here's a complete example `node.bprc` configuration file with CBR/CT enabled:

```
## Bundle Protocol configuration with CBR/CT enabled
1
a scheme ipn 'ipnfw' 'ipnadminep'
a endpoint ipn:1.0 x
a endpoint ipn:1.1 x
a endpoint ipn:1.2 x
a protocol udp 1400 100
a induct udp 127.0.0.1:4556 udpcli
a outduct udp 127.0.0.1:4557 udpclo
r 'ipnadmin node.ipnrc'
m custodymode orangebook
m cbraggr 1 1 2
s
```

## Using CBR/CT Features

### Sending Bundles with Custody Transfer

When `custodymode orangebook` is enabled, bundles can request custody transfer using the standard BP flags. For example, using `bpsource`:

```bash
# Send a bundle with custody transfer requested
bpsource ipn:2.1 "Test message with custody"
```

When an application sends a bundle with custody transfer requested, ION automatically:
1. Adds a CTEB (Custody Transfer Extension Block) to the bundle
2. Tracks the bundle in local custody storage
3. Waits for a CCS from the next custodian
4. Releases custody upon receiving CCS acceptance

### Monitoring Custody Status

Custody and reporting state is inspected directly through `bpadmin`, so no
extra tooling is required. Pipe the list/info commands into `bpadmin`:

```bash
# List every bundle this node is currently holding in custody
echo 'l custodybundle' | bpadmin
```

Each entry shows the sequence identifier (`seqId`), sequence number
(`seqNum`), the next-custodian destination EID, the time custody was
accepted, the time of the last retransmission (or `(never)`), and the
retransmit count:

```
: bundle seqId=0 seqNum=1 dest=ipn:2.1 accepted=1706400000 lastRetx=(never) retxCount=0
```

To check one specific bundle:

```bash
# Reports "pending" (held, awaiting next-hop CCS) or "not-found"
echo 'i custodybundle ipn:1.2 0 1' | bpadmin
```

`l custodybundle` is most informative when `m cbrretx none` is set, since
refused bundles then remain in tracking; with `m cbrretx signal` they are
re-forwarded on refusal and may drain before you look.

To review received Compressed Reporting Signals:

```bash
# Newest first; add an EID argument to filter by sender
echo 'l crslog' | bpadmin
```

> The legacy `cbrcustodytest` utility can also list custody bundles and print
> aggregate statistics, but it is a developer test tool; the `bpadmin`
> commands above are the supported operational interface.

### Using bptrace with CRS

The `bptrace` utility automatically handles both traditional status reports and Compressed Reporting Signals:

```bash
# Send a trace bundle with status reporting
bptrace -flags rcv,fwd,dlv ipn:1.1 ipn:2.1 ipn:1.2 -ttl 300 -msg "Trace test"
```

When nodes are configured with `custodymode orangebook`, they generate CRS instead of traditional status reports. `bptrace` seamlessly processes both formats and displays status information in a unified format.

## Multi-Node Custody Transfer Example

### Network Topology

This example demonstrates custody transfer across three nodes:

```
Node 1 (Source) --> Node 2 (Intermediate) --> Node 3 (Destination)
  ipn:1.x             ipn:2.x                  ipn:3.x
```

### Node 1 Configuration (Source)

**node1.bprc:**
```
1
a scheme ipn 'ipnfw' 'ipnadminep'
a endpoint ipn:1.0 x
a endpoint ipn:1.1 x
a protocol udp 1400 100
a induct udp 10.0.0.1:4556 udpcli
a outduct udp 10.0.0.2:4556 udpclo
r 'ipnadmin node1.ipnrc'
m custodymode orangebook
m cbraggr 1 1 2
s
```

### Node 2 Configuration (Intermediate Custodian)

**node2.bprc:**
```
1
a scheme ipn 'ipnfw' 'ipnadminep'
a endpoint ipn:2.0 x
a endpoint ipn:2.1 x
a protocol udp 1400 100
a induct udp 10.0.0.2:4556 udpcli
a outduct udp 10.0.0.1:4556 udpclo
a outduct udp 10.0.0.3:4556 udpclo
r 'ipnadmin node2.ipnrc'
m custodymode orangebook
m cbraggr 1 1 2
s
```

### Node 3 Configuration (Destination)

**node3.bprc:**
```
1
a scheme ipn 'ipnfw' 'ipnadminep'
a endpoint ipn:3.0 x
a endpoint ipn:3.1 x
a protocol udp 1400 100
a induct udp 10.0.0.3:4556 udpcli
a outduct udp 10.0.0.2:4556 udpclo
r 'ipnadmin node3.ipnrc'
m custodymode orangebook
m cbraggr 1 1 2
s
```

### Testing the Custody Chain

1. Start a receiver on Node 3:
   ```bash
   bpsink ipn:3.1
   ```

2. On Node 1, send a custody-enabled bundle:
   ```bash
   bpsource ipn:3.1 "Hello with custody transfer"
   ```

3. Verify custody transfer on each node:
   ```bash
   # On Node 1 (custody is released once Node 2's CCS acceptance arrives)
   echo 'l custodybundle' | bpadmin

   # On Node 2 (shows intermediate custody handling)
   echo 'l custodybundle' | bpadmin
   ```

## Heterogeneous Networks

CBR/CT is designed to work in networks where not all nodes support the Orange Book specification. The CTEB and CREB extension blocks use processing flags that enable transparent forwarding:

- Non-supporting nodes will forward bundles with these blocks intact
- Custody transfer still works between supporting nodes
- Status reporting falls back to traditional BPv7 reports on non-supporting nodes

### Configuration for Mixed Networks

For a node that should NOT participate in CBR/CT but should forward bundles transparently:

```
# Leave custodymode at default (none)
# Do NOT add: m custodymode orangebook
```

The node will still forward bundles with CTEB/CREB blocks to the next hop.

## Retransmission Strategy

When a next-hop custodian does not acknowledge custody, the node's behavior
is governed by `m cbrretx`:

```
m cbrretx { none | timer | signal } <interval_seconds> <max_retransmissions>
```

- **none** (default) — no automatic retransmission. Bundles stay in custody
  tracking and can be inspected with `l custodybundle`.
- **timer** — the `bpclock` daemon re-forwards a bundle after
  `interval_seconds` if no CCS acceptance has arrived.
- **signal** — a bundle is re-forwarded immediately when a CCS *refusal* is
  received for it.

`max_retransmissions` caps attempts per bundle (0 = unlimited). For example,
retransmit every 30 s up to 5 times:

```
m cbrretx timer 30 5
```

## Command Reference

All CBR/CT options are `bpadmin` commands: `m` (manage/set), `l` (list/show),
`a` (add), `d` (delete), and `i` (info). They may be placed in a node's
`bprc` file (processed at startup) or piped into a running node with
`echo '<command>' | bpadmin`. Unless noted otherwise, every command below
requires `m custodymode orangebook` to be in effect.

> **Startup vs. runtime:** Most commands take effect immediately. The two
> exceptions are noted in their entries: `m crebreportto` and
> `m cbrcounterwidth` are read by long-running daemons from a cached snapshot
> and are best set in the `bprc` file (a runtime change is reliably picked up
> only after the BP daemons restart). `m cbraggr` *is* honored at runtime.

### Mode and reporting

| Command | Description |
|---------|-------------|
| `m custodymode { none \| bibe \| orangebook }` | Selects the custody transfer mode. `orangebook` enables CBR/CT; `none` (default) disables it; `bibe` is reserved. This is the master switch for everything below. |
| `m srmode { traditional \| compressed \| both }` | Selects the status-report format generated by this node: classic BPv7 status reports, Compressed Reporting Signals (CRS), or both. CRS-based reporting (`compressed` or `both`) is required for the CREB commands. |

### Signal aggregation

`m cbraggr <crs_limit> <ccs_limit> <timeout_seconds>`

Aggregates multiple status/custody events into one administrative bundle. A
signal is transmitted when its bundle-entry count reaches the limit, or when
`timeout_seconds` elapses since aggregation began — whichever comes first. A
limit of 0 means "send immediately". Defaults: 10, 10, 5. **Effective at
runtime.**

```
m cbraggr 1 1 2       # testing: send almost immediately
m cbraggr 50 50 15    # production: batch up to 50 entries, flush every 15 s
```

`l cbraggr` — display the current CRS limit, CCS limit, and timeout.

### Retransmission

`m cbrretx { none | timer | signal } <interval_seconds> <max_retransmissions>`

Controls re-forwarding when custody is not acknowledged. See
[Retransmission Strategy](#retransmission-strategy) above. Defaults: `none`,
60, 3.

### CREB (Compressed Reporting Extension Block) options

| Command | Description |
|---------|-------------|
| `m crebexpliciteid { 0 \| 1 }` | When `1`, the source EID is written explicitly into CREB blocks instead of being implied from the primary block. Default `0`. Set `1` only for peers that require the explicit field. |
| `m crebreportto <eid> \| -` | Redirects every CRS generated for this node's bundles to `<eid>` (carried as CREB element 4) instead of each bundle's `reportTo`. Use `-` to clear. Useful for a dedicated monitoring station. Requires `m srmode compressed` or `both`. Best set in `bprc` (see startup-vs-runtime note). |
| `l crebreportto` | Show the current report-to override (or "none"). |
| `m cbrcounterwidth { 16 \| 32 \| 64 }` | Sets the wraparound width (bits) of new bundle sequence counters, for interoperability with constrained peers (Orange Book §3.2.9). Applies to counters created afterward; existing counters keep their width. Default 64. Best set in `bprc`. |
| `l cbrcounterwidth` | Show the current counter width and its maximum value. |

### Custody-acceptance whitelist

Controls which incoming custody requests this node will accept. The two
dimensions are checked independently; an empty list for a dimension means
"accept any". If a non-empty list does not match, custody is refused with a
CCS refusal.

| Command | Description |
|---------|-------------|
| `a cbraccept { custodian \| source } <eid>` | Add an EID to the whitelist. `custodian` matches the node currently holding custody (CTEB custodian EID); `source` matches the bundle's original source EID. |
| `d cbraccept { custodian \| source } <eid>` | Remove a whitelist entry. |
| `l cbraccept` | List both whitelists; `(all)` means that dimension accepts everyone. |

```
a cbraccept custodian ipn:3.0   # only take custody from node 3
a cbraccept source ipn:5.1      # only for bundles sourced at ipn:5.1
```

### Auto custody-request policy

Requests custody transfer automatically for bundles sent to listed
destinations, even when the sending application did not ask for it.

| Command | Description |
|---------|-------------|
| `a custodyreq <eid>` | Add a destination EID to the auto custody-request list (exact match). |
| `d custodyreq <eid>` | Remove a destination EID. |
| `l custodyreq` | List all auto-request destinations, or `(none)`. |

### Monitoring and history

| Command | Description |
|---------|-------------|
| `l custodybundle` | List all bundles currently held in custody tracking (seqId, seqNum, next-custodian EID, accept time, last-retransmit time, retransmit count). |
| `i custodybundle <source_eid> <seqId> <seqNum>` | Report one bundle's custody status: `pending` or `not-found`. |
| `m crsmaxlog <count>` | Set the max number of received-CRS records kept in the persistent history log; `0` = unbounded. Default 100. |
| `l crslog [<eid>]` | Print received CRS records, newest first (timestamp, sender EID, status code [1=received, 2=forwarded, 3=delivered, 4=deleted], bundle count). Optional `<eid>` filters by sender. |

## Troubleshooting

### Bundles Not Appearing in Custody

1. Verify custody mode is enabled:
   ```bash
   # Check ion.log for custody mode initialization
   grep -i custody ion.log
   ```

2. Ensure the application requests custody transfer when sending

3. Check that both source and destination nodes have matching custody mode configuration

### CCS Signals Not Received

1. Verify network connectivity between nodes
2. Check aggregation timeout isn't too long
3. Review ion.log for CCS-related messages:
   ```bash
   grep -i "CCS" ion.log
   ```

### Signal Aggregation Issues

If signals seem delayed:
1. Reduce aggregation limits for testing: `m cbraggr 1 1 2`
2. Check current limits with `echo 'l cbraggr' | bpadmin`
3. Check for pending custody bundles with `echo 'l custodybundle' | bpadmin`

## Performance Considerations

- **Aggregation Limits**: Higher limits reduce overhead but increase latency
- **Timeout Values**: Balance between signal timeliness and aggregation efficiency
- **Custody Storage**: Monitor custody bundle counts to avoid memory issues

Recommended production settings:
```
m cbraggr 50 50 15
```

This aggregates up to 50 bundle entries per signal and flushes every 15 seconds.

## Extension Blocks Reference

CBR/CT uses two extension blocks:

| Block Type | Name | Purpose |
|------------|------|---------|
| 13 | CTEB (Custody Transfer Extension Block) | Carries custody sequence information |
| 14 | CREB (Compressed Reporting Extension Block) | Carries status reporting sequence information |

And two administrative record types:

| Admin Record Type | Name | Purpose |
|-------------------|------|---------|
| 13 | CCS (Compressed Custody Signal) | Custody acceptance/refusal notifications |
| 14 | CRS (Compressed Reporting Signal) | Aggregated status reports |

## See Also

- [bprc(5)](./Man-Page-Introduction.md) - Bundle Protocol configuration commands (full CBR/CT command syntax)
- [bptrace(1)](./Man-Page-Introduction.md) - Bundle Protocol trace utility
- [cbrcustodytest(1)](./Man-Page-Introduction.md) - Developer test utility for custody transfer
- CCSDS Orange Book "Compressed Bundle Status Reporting and Custody Signaling" specification

## Summary

CBR/CT provides efficient status reporting and reliable custody-based delivery for DTN networks. Key configuration steps:

1. Enable custody mode: `m custodymode orangebook`
2. Choose report format: `m srmode compressed`
3. Configure aggregation: `m cbraggr <crs_limit> <ccs_limit> <timeout>`
4. Monitor with: `echo 'l custodybundle' | bpadmin` and `echo 'l crslog' | bpadmin`
5. Test with: `bptrace` (delivery/CRS) and `bpsource`/`bpsink`

See the [Command Reference](#command-reference) above for every option.

For questions or issues, see the [ION GitHub repository](https://github.com/nasa-jpl/ION-DTN).
