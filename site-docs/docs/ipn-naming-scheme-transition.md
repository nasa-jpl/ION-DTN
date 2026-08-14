# IPN Naming Scheme Transition Guide

## Table of Contents
1. [Introduction](#introduction)
2. [Overview of 3-Part IPN Naming](#overview-of-3-part-ipn-naming)
3. [Structure Field Name Changes](#structure-field-name-changes)
4. [Function Parameter Changes](#function-parameter-changes)
5. [Code Migration Examples](#code-migration-examples)
6. [FQNN Representation in Logs and Internal State](#fqnn-representation-in-logs-and-internal-state)

---

## Introduction

Starting with ION version 4.1.4-a.2, ION introduced support for a 3-part IPN naming scheme using **Fully Qualified Node Numbers (FQNN)** instead of simple node numbers. This change enables hierarchical node addressing with an optional allocator component, which is essential for large-scale DTN deployments where node number management needs to be delegated across multiple administrative domains.

This document describes the changes developers need to make when updating code that references the older Node-based structures to the new FQNN-based structures.

---

## Overview of 3-Part IPN Naming

### Old 2-Part IPN Format
```
ipn:<node>.<service>
Example: ipn:5.23
```

### New 3-Part IPN Format
```
ipn:[allocator.]<node>.<service>
Examples:
  ipn:5.23        (node 5, service 23 - backwards compatible)
  ipn:1.5.23      (allocator 1, node 5, service 23)
```

### FQNN Encoding

The FQNN is a 64-bit unsigned integer (`uvast`) that encodes both the allocator and node number:

```
FQNN (64 bits) = [32-bit allocator ID][32-bit node number]
```

For backwards compatibility, when no allocator is specified, the allocator portion is zero, and the FQNN equals the node number.

---

## Structure Field Name Changes

The following table summarizes all field name changes across ION structures:

### IonCXref (Contact Cross-Reference)

| Old Field | New Field | Type | Description |
|-----------|-----------|------|-------------|
| `fromNode` | `fromFqnn` | `uvast` | Source node FQNN |
| `toNode` | `toFqnn` | `uvast` | Destination node FQNN |

### IonRXref (Range Cross-Reference)

| Old Field | New Field | Type | Description |
|-----------|-----------|------|-------------|
| `fromNode` | `fromFqnn` | `uvast` | Source node FQNN |
| `toNode` | `toFqnn` | `uvast` | Destination node FQNN |

### IonContact

| Old Field | New Field | Type | Description |
|-----------|-----------|------|-------------|
| `fromNode` | `fromFqnn` | `uvast` | Source node FQNN |
| `toNode` | `toFqnn` | `uvast` | Destination node FQNN |

### IonRange

| Old Field | New Field | Type | Description |
|-----------|-----------|------|-------------|
| `fromNode` | `fromFqnn` | `uvast` | Source node FQNN |
| `toNode` | `toFqnn` | `uvast` | Destination node FQNN |

### IonNode

| Old Field | New Field | Type | Description |
|-----------|-----------|------|-------------|
| `nodeNbr` | `fqnn` | `uvast` | Node's fully qualified node number |

### IonNeighbor

| Old Field | New Field | Type | Description |
|-----------|-----------|------|-------------|
| `nodeNbr` | `fqnn` | `uvast` | Neighbor's fully qualified node number |

### IonProbe

| Old Field | New Field | Type | Description |
|-----------|-----------|------|-------------|
| `destNodeNbr` | `destFqnn` | `uvast` | Destination FQNN |
| `neighborNodeNbr` | `neighborFqnn` | `uvast` | Neighbor FQNN |

### RegionMember

| Old Field | New Field | Type | Description |
|-----------|-----------|------|-------------|
| `nodeNbr` | `fqnn` | `uvast` | Member's fully qualified node number |

### IonDB

| Old Field | New Field | Type | Description |
|-----------|-----------|------|-------------|
| `ownNodeNbr` | `ownFqnn` | `uvast` | This node's fully qualified node number |

---

## Function Parameter Changes

### RFX API Functions (rfx.h)

| Old Signature | New Signature |
|---------------|---------------|
| `rfx_insert_contact(..., uvast fromNode, uvast toNode, ...)` | `rfx_insert_contact(..., uvast fromFqnn, uvast toFqnn, ...)` |
| `rfx_insert_range(..., uvast fromNode, uvast toNode, ...)` | `rfx_insert_range(..., uvast fromFqnn, uvast toFqnn, ...)` |
| `rfx_remove_contact(..., uvast fromNode, uvast toNode, ...)` | `rfx_remove_contact(..., uvast fromFqnn, uvast toFqnn, ...)` |
| `rfx_remove_range(..., uvast fromNode, uvast toNode, ...)` | `rfx_remove_range(..., uvast fromFqnn, uvast toFqnn, ...)` |
| `rfx_contact_state(uvast nodeNbr, ...)` | `rfx_contact_state(uvast fqnn, ...)` |
| `findNeighbor(IonVdb *vdb, uvast nodeNbr, ...)` | `findNeighbor(IonVdb *vdb, uvast fqnn, ...)` |
| `findNode(IonVdb *vdb, uvast nodeNbr, ...)` | `findNode(IonVdb *vdb, uvast fqnn, ...)` |
| `addEmbargo(IonNode *node, uvast neighborNodeNbr)` | `addEmbargo(IonNode *node, uvast neighborFqnn)` |
| `removeEmbargo(IonNode *node, uvast neighborNodeNbr)` | `removeEmbargo(IonNode *node, uvast neighborFqnn)` |

### Utility Functions

| Old Function | New Function |
|--------------|--------------|
| `getOwnNodeNbr()` | `getOwnFqnn()` |

---

## Code Migration Examples

### Example 1: Accessing Contact Information

**Old Code:**
```c
if (cxref->fromNode == getOwnNodeNbr())
{
    neighbor = getNeighbor(vdb, cxref->toNode);
    CHKERR(neighbor);
    neighbor->xmitRate = 0;
}
```

**New Code:**
```c
if (cxref->fromFqnn == getOwnFqnn())
{
    neighbor = getNeighbor(vdb, cxref->toFqnn);
    CHKERR(neighbor);
    neighbor->xmitRate = 0;
}
```

### Example 2: Initializing Contact Search Arguments

**Old Code:**
```c
IonCXref arg;

memset((char *) &arg, 0, sizeof(IonCXref));
arg.fromNode = sourceNode;
arg.toNode = destNode;
```

**New Code:**
```c
IonCXref arg;

memset((char *) &arg, 0, sizeof(IonCXref));
arg.fromFqnn = sourceFqnn;
arg.toFqnn = destFqnn;
```

### Example 3: Inserting Contacts

**Old Code:**
```c
rfx_insert_contact(_regionNbr(NULL), MAX_POSIX_TIME, MAX_POSIX_TIME,
        getOwnNodeNbr(), getOwnNodeNbr(), 0, 1.0, &xaddr, _announce(NULL));
```

**New Code:**
```c
rfx_insert_contact(_regionNbr(NULL), MAX_POSIX_TIME, MAX_POSIX_TIME,
        getOwnFqnn(), getOwnFqnn(), 0, 1.0, &xaddr, _announce(NULL));
```

### Example 4: Comparing Node Identifiers

**Old Code:**
```c
if (contact->fromNode > argContact->fromNode) return 1;
if (contact->toNode < argContact->toNode) return -1;
```

**New Code:**
```c
if (contact->fromFqnn > argContact->fromFqnn) return 1;
if (contact->toFqnn < argContact->toFqnn) return -1;
```

---

## FQNN Representation in Logs and Internal State

When examining ION's internal state variables or log reports, you may encounter FQNN values displayed in different formats. It is important to understand that these different representations refer to the same underlying node.

### 64-Bit Integer Representation

Internal state variables and some log messages display the FQNN as a single 64-bit integer:

```
Contact from 4294967301 to 4294967302
```

In this example:
- `4294967301` = allocator 1, node 5 (encoded as `(1 << 32) + 5`)
- `4294967302` = allocator 1, node 6 (encoded as `(1 << 32) + 6`)

### 3-Part Dotted Representation

User-facing commands and configuration files use the human-readable 3-part format:

```
a contact +0 +3600 1.5 1.6 100000
```

This represents a contact from node `1.5` (allocator 1, node 5) to node `1.6` (allocator 1, node 6).

### Converting Between Representations

To interpret a 64-bit FQNN value:

```c
uvast fqnn = 4294967301;  // Example FQNN value

uint32_t allocator = (uint32_t)(fqnn >> 32);        // Upper 32 bits
uint32_t nodeNbr = (uint32_t)(fqnn & 0xFFFFFFFF);   // Lower 32 bits

// Result: allocator = 1, nodeNbr = 5
// Displayed as: 1.5
```

To construct an FQNN from allocator and node number:

```c
uint32_t allocator = 1;
uint32_t nodeNbr = 5;

uvast fqnn = ((uvast)allocator << 32) | nodeNbr;
// Result: fqnn = 4294967301
```

### Backwards Compatibility

For nodes without an allocator (allocator = 0), the FQNN equals the node number:

| Node Number | Allocator | FQNN (64-bit) | Display Format |
|-------------|-----------|---------------|----------------|
| 5 | 0 | 5 | `5` or `0.5` |
| 5 | 1 | 4294967301 | `1.5` |
| 100 | 2 | 8589934692 | `2.100` |

### Practical Tips

1. **Log Analysis**: When you see large integers (> 4 billion) in contact or range logs, they likely represent 3-part FQNNs with a non-zero allocator.

2. **Debugging**: Use the conversion formulas above to decode FQNN values when debugging.

3. **Configuration**: Always use the dotted notation in configuration files for clarity.

4. **Comparison**: Two representations are equivalent if they encode the same allocator and node number, regardless of display format.

---

**Document Version:** 1.0
**Last Updated:** 2025-12-02
**Applies to:** ION 4.1.4-a.2 and later
