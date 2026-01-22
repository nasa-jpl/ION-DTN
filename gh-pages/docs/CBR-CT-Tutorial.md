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

Use the `cbrcustodytest` utility to monitor custody transfer status:

```bash
# List all bundles currently in custody
cbrcustodytest -l
```

This displays:
- Bundles currently held in custody
- CBR/CT statistics including CCS signals sent/received

Example output:
```
cbrcustodytest: Listing bundles in custody...
  [0] destEid=ipn:2.1 seqId=0 seqNum=1
       custodyAccepted=1706400000 lastTransmit=1706400000 retransmitCount=0
cbrcustodytest: 1 bundle(s) in custody.

CBR/CT Statistics:
  CCS Accept Sent:     5
  CCS Refuse Sent:     0
  CCS Accept Received: 3
  CCS Refuse Received: 0
  Custody Originated:  5
  Custody Accepted:    2
  Custody Released:    3
  CRS Signals Sent:    0
  CRS Signals Recv:    0
```

### Understanding the Statistics

- **CCS Accept Sent**: Number of custody acceptance signals sent to prior custodians
- **CCS Refuse Sent**: Number of custody refusal signals sent
- **CCS Accept Received**: Number of custody acceptances received from next-hop custodians
- **CCS Refuse Received**: Number of custody refusals received
- **Custody Originated**: Bundles originated at this node with custody transfer
- **Custody Accepted**: Bundles received from other nodes with custody accepted
- **Custody Released**: Bundles released after receiving CCS acceptance
- **CRS Signals Sent/Recv**: Compressed Reporting Signals sent and received

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
   # On Node 1 (should show custody released after CCS received)
   cbrcustodytest -l

   # On Node 2 (should show intermediate custody handling)
   cbrcustodytest -l
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

## Manual Retransmission

The current implementation uses manual (command-based) retransmission. If custody signals are not received within expected timeframes, you can trigger retransmission:

```bash
# Test mode with retransmission enabled
cbrcustodytest ipn:2.1 -r -t30
```

This sends a test bundle and, if custody is not released within 30 seconds, triggers manual retransmission of all bundles in custody.

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
2. Check for pending signals with `cbrcustodytest -l`

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

- [bprc(5)](./Man-Page-Introduction.md) - Bundle Protocol configuration commands
- [cbrcustodytest(1)](./Man-Page-Introduction.md) - Custody transfer test utility
- [bptrace(1)](./Man-Page-Introduction.md) - Bundle Protocol trace utility
- CCSDS Orange Book "Compressed Bundle Status Reporting and Custody Signaling" specification

## Summary

CBR/CT provides efficient status reporting and reliable custody-based delivery for DTN networks. Key configuration steps:

1. Enable custody mode: `m custodymode orangebook`
2. Configure aggregation: `m cbraggr <crs_limit> <ccs_limit> <timeout>`
3. Monitor with: `cbrcustodytest -l`
4. Test with: `bptrace` (for CRS) or `cbrcustodytest` (for custody transfer)

For questions or issues, see the [ION GitHub repository](https://github.com/nasa-jpl/ION-DTN).
