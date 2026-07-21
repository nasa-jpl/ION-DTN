# BP Extension Interface

ION offers software developers a set of standard interface for adding extensions to Bundle Protocol without modifying the core BP source code. This capability can be used to implement both standardized bundle extension blocks or user-specific extension blocks.

ION's interface for extending the Bundle Protocol enables the definition of external functions that insert extension blocks into outbound bundles (either before or after the payload block), parse and record extension blocks in inbound bundles, and modify extension blocks at key points in bundle processing. All extension-block handling is statically linked into ION at build time, but the addition of an extension never requires that any standard ION source code be modified.

Standard structures for recording extension blocks -- both in transient storage memory during bundle acquisition (AcqExtBlock) and in persistent storage [the ION database] during subsequent bundle processing (ExtensionBlock) -- are defined in the bei.h header file. In each case, the extension block structure comprises a block type code, block processing flags, possibly a list of EID references, an array of bytes (the serialized form of the block, for transmission), the length of that array, optionally an extension-specific opaque object whose structure is designed to characterize the block in a manner that's convenient for the extension processing functions, and the size of that object.

## Extension Definition: `ExtensionDef` & `extensionDefs`

The definition of each extension is asserted in an ExtensionDef structure, also as defined in the `bei.h` header file.

```c
/**
 *  \struct ExtensionDef
 *  \brief Defines the callbacks used to process extension blocks.
 *
 * ExtensionDef defines the callbacks for production and acquisition
 * of a single type of extension block, identified by block type name
 * and number.
 */
typedef struct
{
	char			name[32];	/** Name of extension	*/
	BpBlockType		type;		/** Block type		*/

	/*	Production callbacks.					*/

	BpExtBlkOfferFn		offer;		/** Offer 		*/
	BpExtBlkSerializeFn	serialize;	/** Serialize		*/
	BpExtBlkProcessFn	process[5];	/** Process		*/
	BpExtBlkReleaseFn	release;	/** Release 		*/
	BpExtBlkCopyFn		copy;		/** Copy		*/

	/*	Acquisition callbacks.					*/

	BpAcqExtBlkAcquireFn	acquire;	/** Acquire 		*/
	BpAcqExtReviewFn	review;		/** Review		*/
	BpAcqExtBlkDecryptFn	decrypt;	/** Decrypt 		*/
	BpAcqExtBlkParseFn	parse;		/** Parse 		*/
	BpAcqExtBlkCheckFn	check;		/** Check 		*/
	BpExtBlkRecordFn	record;		/** Record 		*/
	BpAcqExtBlkClearFn	clear;		/** Clear 		*/
} ExtensionDef;
```

Each ExtensionDef must supply:

* The name of the extension. (Used in some diagnostic messages.)

* The extension's block type code.

* A pointer to an offer function.

* A pointer to a function to be called when forwarding a bundle containing this sort of block.

* A pointer to a function to be called when taking custody of a bundle containing this sort of block.

* A pointer to a function to be called when enqueuing for transmission a bundle containing this sort of block.

* A pointer to a function to be called when a convergence-layer adapter dequeues a bundle containing this sort of block, before serializing it.

* A pointer to a function to be called immediately before a convergence-layer adapter transmits a bundle containing this sort of block, after the bundle has been serialized.

* A pointer to a release function.

* A pointer to a copy function.

* A pointer to an acquire function.

* A pointer to a review function.

* A pointer to a decrypt function.

* A pointer to a parse function.

* A pointer to a check function.

* A pointer to a record function.

* A pointer to a clear function.

All extension definitions must be coded into an array of ExtensionDef structures named extensionDefs.

## ExtensionSpec - specification for producing an extension block

```c
/*	ExtensionSpec provides the specification for producing an
 *	outbound extension block: block definition (identified by
 *	block type number), a formulation tag whose semantics are
 *	block-type-specific, and applicable CRC type.			*/

typedef struct
{
	BpBlockType	type;		/*	Block type		*/
	unsigned char	tag;		/*	Extension-specific	*/
	BpCrcType	crcType;	/*	Type of CRC on block	*/
} ExtensionSpec;
```

An array of ExtensionSpec structures named extensionSpecs is also required. Each ExtensionSpec provides the specification for producing an outbound extension block:

1. block definition (identified by block type number),
2. a discriminator tag whose semantics are block-type-specific, and
3. CRC type indicating what type of CRC must be used to protect this extension block.

The order of appearance of extension specifications in the extensionSpecs array determines the order in which extension blocks will be inserted into locally sourced bundles.

## Procedure to Extend the Bundle Protocol

The standard extensionDefs array -- which is empty -- is in the `noextensions.c` prototype source file. The procedure for extending the Bundle Protocol in ION is as follows:

1. Specify -DBP_EXTENDED in the Makefile's compiler command line when building the libbpP.c library module.

2. Create a copy of the prototype extensions file, named "bpextensions.c", in a directory that is made visible to the Makefile's libbpP.c compilation command line (by a -I parameter).

3. In the "external function declarations" area of "bpextensions.c", add "extern" function declarations identifying the functions that will implement your extension (or extensions).

4. Add one or more ExtensionDef structure initialization lines to the extensionDefs array, referencing those declared functions.

5. Add one or more ExtensionSpec structure initialization lines to the extensionSpecs array, referencing those extension definitions.

6. Develop the implementations of the extension implementation functions in one or more new source code files.

7. Add the object file or files for the new extension implementation source file (or files) to the Makefile's command line for linking libbpP.so.

## Extension Implementation Functions

The function pointers supplied in each ExtensionDef must conform to the following specifications.

NOTE that any function that modifies the bytes member of an ExtensionBlock or AcqExtBlock must set the corresponding length to the new length of the bytes array, if changed.

```c
int (*BpExtBlkOfferFn)(ExtensionBlock *blk, Bundle *bundle)
```

Populates all fields of the indicated ExtensionBlock structure for inclusion in the indicated outbound bundle. This function is automatically called when a new bundle is locally sourced or upon acquisition of a remotely sourced bundle that does not contain an extension block of this type. The values of the extension block are typically expected to be a function of the state of the bundle, but this is extension-specific. If it is not appropriate to offer an extension block of this type as part of this bundle, then the size, length, object, and bytes members of blk must all be set to zero. If it is appropriate to offer such a block but no internal object representing the state of the block is needed, the object and size members of blk must be set to zero. The type, blkProcFlags, and dataLength members of blk must be populated by the implementation of the "offer" function, but the length and bytes members are typically populated by calling the BP library function serializeExtBlk(), which must be passed the block to be serialized (with type, blkProcFlags and dataLength already set), a Lyst of EID references (two list elements -- offsets -- per EID reference, if applicable; otherwise NULL), and a pointer to the extension-specific block data. The block's bytes array and object (if present) must occupy space allocated from the ION database heap. Return zero on success, -1 on any system failure.

```c
int (*BpExtBlkProcessFn)(ExtensionBlock *blk, Bundle *bundle, void *context)
```

Performs some extension-specific transformation of the data encapsulated in blk based on the state of bundle. The transformation to be performed will typically vary depending on whether the identified function is the one that is automatically invoked upon forwarding the bundle, upon taking custody of the bundle, upon enqueuing the bundle for transmission, upon removing the bundle from the transmission queue, or upon transmitting the serialized bundle. The context argument may supply useful supplemental information; in particular, the context provided to the ON_DEQUEUE function will comprise the name of the protocol for the duct from which the bundle has been dequeued, together with the EID of the neighboring node endpoint to which the bundle will be directly transmitted when serialized. The block-specific data in blk is located within bytes immediately after the header of the extension block; the length of the block's header is the difference between length and dataLength. Whenever the block's blkProcFlags, EID extensions, and/or block-specific data are altered, the serializeExtBlk() function should be called again to recalculate the size of the extension block and rebuild the bytes array. Return zero on success, -1 on any system failure.

```c
void (*BpExtBlkReleaseFn)(ExtensionBlock *blk)
```

Releases all ION database space occupied by the object member of blk. This function is automatically called when a bundle is destroyed. Note that incorrect implementation of this function may result in a database space leak.

```c
int (*BpExtBlkCopyFn)(ExtensionBlock *newblk, ExtensionBlock *oldblk)
```

Copies the object member of oldblk to ION database heap space and places the address of that new non-volatile object in the object member of newblk, also sets size in newblk. This function is automatically called when two copies of a bundle are needed, e.g., in the event that it must both be delivered to a local client and also forwarded to another node. Return zero on success, -1 on any system failure.

```c
int (*BpAcqExtBlkAcquireFn)(AcqExtBlock *acqblk, AcqWorkArea *work)
```

Populates the indicated AcqExtBlock structure with size and object for retention as part of the indicated inbound bundle. (The type, blkProcFlags, EID references (if any), dataLength, length, and bytes values of the structure are pre-populated with data as extracted from the serialized bundle.) This function is only to be provided for extension blocks that are never encrypted; an extension block that may be encrypted should have a BpAcqExtBlkParseFn callback instead. The function is automatically called when an extension block of this type is encountered in the course of parsing and acquiring a bundle for local delivery and/or forwarding. If no internal object representing the state of the block is needed, the object member of acqblk must be set to NULL and the size member must be set to zero. If an object is needed for this block, it must occupy space that is allocated from ION working memory using MTAKE and its size must be indicated in blk. Return zero if the block is malformed (this will cause the bundle to be discarded), 1 if the block is successfully parsed, -1 on any system failure.

```c
int (*BpAcqExtBlkReviewFn)(AcqWorkArea *work)
```

Reviews the extension blocks that have been acquired for this bundle, checking to make sure that all blocks of this type that are required by policy are present. Returns 0 if any blocks are missing, 1 if all required blocks are present, -1 on any system failure.

```c
int (*BpAcqExtBlkDecryptFn)(AcqExtBlock *acqblk, AcqWorkArea *work)
```

Decrypts some other extension block that has been acquired but not yet parsed, nominally using encapsulated ciphersuite information. Return zero if the block is malformed (this will cause the bundle to be discarded), 1 if no error in decryption was encountered, -1 on any system failure.

```c
int (*BpAcqExtBlkParseFn)(AcqExtBlock *acqblk, AcqWorkArea *work)
```

Populates the indicated AcqExtBlock structure with size and object for retention as part of the indicated inbound bundle. (The type, blkProcFlags, EID references (if any), dataLength, length, and bytes values of the structure are pre-populated with data as extracted from the serialized bundle.) This function is provided for extension blocks that may be encrypted; an extension block that can never be encrypted should have a BpAcqExtBlkAcquireFn callback instead. The function is automatically called when an extension block of this type is encountered in the course of parsing and acquiring a bundle for local delivery and/or forwarding. If no internal object representing the state of the block is needed, the object member of acqblk must be set to NULL and the size member must be set to zero. If an object is needed for this block, it must occupy space that is allocated from ION working memory using MTAKE and its size must be indicated in blk. Return zero if the block is malformed (this will cause the bundle to be discarded), 1 if the block is successfully parsed, -1 on any system failure.

```c
int (*BpAcqExtBlkCheckFn)(AcqExtBlock *acqblk, AcqWorkArea *work)
```

Examines the bundle in work to determine whether or not it is authentic, in the context of the indicated extension block. Return 1 if the block is determined to be inauthentic (this will cause the bundle to be discarded), zero if no inauthenticity is detected, -1 on any system failure.

```c
int (*BpExtBlkRecordFn)(ExtensionBlock *blk, AcqExtBlock *acqblk)
```

Copies the object member of acqblk to ION database heap space and places the address of that non-volatile object in the object member of blk; also sets size in blk. This function is automatically called when an acquired bundle is accepted for forwarding and/or delivery. Return zero on success, -1 on any system failure.

```c
void (*BpAcqExtBlkClearFn)(AcqExtBlock *acqblk)
```

Uses MRELEASE to release all ION working memory occupied by the object member of acqblk. This function is automatically called when acquisition of a bundle is completed, whether or not the bundle is accepted. Note that incorrect implementation of this function may result in a working memory leak.

## Utility Functions for Extension Processing

```c
void discardExtensionBlock(AcqExtBlock *blk)
```

Deletes this block from the bundle acquisition work area prior to the recording of the bundle in the ION database.

```c
void scratchExtensionBlock(ExtensionBlock *blk)
```

Deletes this block from the bundle after the bundle has been recorded in the ION database.

```c
SdrObject findExtensionBlock(Bundle *bundle, unsigned int type,
                unsigned char tag1, unsigned char tag2, unsigned char tag3)
```

On success, returns the address of the ExtensionBlock in bundle for the indicated type and tag values. If no such extension block exists, returns zero.

```c
int serializeExtBlk(ExtensionBlock *blk, char *blockData)
```

Constructs a BPv7-conformant serialized representation of this extension block in blk->bytes. Returns 0 on success, -1 on an unrecoverable system error.

```c
void suppressExtensionBlock(ExtensionBlock *blk)
```

Causes blk to be omitted when the bundle to which it is attached is serialized for transmission. This suppression remains in effect until it is reversed by restoreExtensionBlock();

```c
void restoreExtensionBlock(ExtensionBlock *blk)
```

Reverses the effect of suppressExtensionBlock(), enabling the block to be included when the bundle to which it is attached is serialized.

---

## Extension Block Lifecycle

Understanding the lifecycle of extension blocks is critical for proper implementation:

### Outbound Bundle Processing

1. **Bundle Creation**: When a bundle is created locally, the `offer()` function is called for each registered extension type
2. **Serialization**: The `serialize()` function converts the block structure to wire format (typically via `serializeExtBlk()`)
3. **Forwarding**: `process[BP_PROCESS_ON_FORWARD]()` is called when the bundle is forwarded to another node
4. **Custody**: `process[BP_PROCESS_ON_TAKE_CUSTODY]()` is called when custody is taken (if applicable)
5. **Enqueue**: `process[BP_PROCESS_ON_ENQUEUE]()` is called when the bundle is queued for transmission
6. **Dequeue**: `process[BP_PROCESS_ON_DEQUEUE]()` is called when removed from transmission queue
7. **Transmission**: `process[BP_PROCESS_ON_TRANSMIT]()` is called after serialization, before actual transmission
8. **Duplication**: If the bundle needs to be duplicated (e.g., local delivery + forwarding), `copy()` is called
9. **Destruction**: When the bundle is destroyed, `release()` is called to free database heap memory

### Inbound Bundle Processing

1. **Reception**: Bundle arrives and is parsed by BP core
2. **Acquisition**: For non-encrypted blocks, `acquire()` extracts the block into temporary (working) memory
   - For potentially encrypted blocks, use `parse()` instead of `acquire()`
3. **Decryption**: If applicable, `decrypt()` processes encrypted blocks
4. **Validation**: `check()` verifies block authenticity
5. **Review**: `review()` checks that all required blocks are present
6. **Recording**: If the bundle is accepted, `record()` converts from temporary to persistent (database) storage
7. **Cleanup**: `clear()` releases working memory allocated during acquisition

---

## Memory Management: Critical Concepts

### Database Heap vs. Working Memory

ION uses two distinct memory regions:

- **Database Heap (SDR)**: Persistent storage that survives process crashes. Use `sdr_malloc()` for allocation.
  - For `ExtensionBlock` objects (persistent storage)
  - Memory allocated in `offer()`, `copy()`, and `record()`
  - Must be freed in `release()`

- **Working Memory**: Temporary storage for bundle acquisition. Use `MTAKE()` for allocation.
  - For `AcqExtBlock` objects (temporary storage during reception)
  - Memory allocated in `acquire()` or `parse()`
  - Must be freed in `clear()`

### Common Memory Management Errors

**Error 1: Using wrong memory type**
```c
// WRONG: Using working memory for persistent storage
int myOffer(ExtensionBlock *blk, Bundle *bundle) {
    MyData *data = (MyData *)MTAKE(sizeof(MyData));  // WRONG!
    blk->object = (SdrObject)data;
    return 0;
}

// CORRECT: Use database heap for persistent storage
int myOffer(ExtensionBlock *blk, Bundle *bundle) {
    Sdr sdr = getIonsdr();
    SdrObject dataObj = sdr_malloc(sdr, sizeof(MyData));
    if (dataObj == 0) return -1;

    MyData data;
    // ... populate data ...
    sdr_write(sdr, dataObj, (char *)&data, sizeof(MyData));

    blk->object = dataObj;
    blk->size = sizeof(MyData);
    return 0;
}
```

**Error 2: Not freeing memory**
```c
// WRONG: Memory leak - not freeing in release()
void myRelease(ExtensionBlock *blk) {
    // Nothing - MEMORY LEAK!
}

// CORRECT: Free database memory
void myRelease(ExtensionBlock *blk) {
    Sdr sdr = getIonsdr();
    if (blk->object) {
        sdr_free(sdr, blk->object);
    }
}
```

---

## Complete Working Example: Timestamp Extension

This example implements a simple extension block that adds a timestamp to bundles.

### Step 1: Define the Extension Data Structure

```c
// File: timestamp_ext.h
#ifndef TIMESTAMP_EXT_H
#define TIMESTAMP_EXT_H

#include "bei.h"

#define TIMESTAMP_EXT_TYPE  192  // Use private block type number

typedef struct {
    time_t  timestamp;
    uvast   sequenceNum;
} TimestampData;

#endif
```

### Step 2: Implement Callback Functions

```c
// File: timestamp_ext.c
#include "timestamp_ext.h"

static uvast g_sequenceNum = 0;

// Offer function: Create block for outbound bundle
int timestampOffer(ExtensionBlock *blk, Bundle *bundle)
{
    Sdr sdr = getIonsdr();
    SdrObject dataObj;
    TimestampData data;

    // Populate timestamp data
    data.timestamp = time(NULL);
    data.sequenceNum = g_sequenceNum++;

    // Allocate in database heap
    dataObj = sdr_malloc(sdr, sizeof(TimestampData));
    if (dataObj == 0) {
        return -1;
    }

    // Write data to SDR
    sdr_write(sdr, dataObj, (char *)&data, sizeof(TimestampData));

    // Set block metadata
    blk->type = TIMESTAMP_EXT_TYPE;
    blk->blkProcFlags = 0;  // No special processing flags
    blk->dataLength = sizeof(TimestampData);
    blk->object = dataObj;
    blk->size = sizeof(TimestampData);

    // Serialize the block
    if (serializeExtBlk(blk, (char *)&data) < 0) {
        sdr_free(sdr, dataObj);
        return -1;
    }

    return 0;
}

// Release function: Free database memory
void timestampRelease(ExtensionBlock *blk)
{
    Sdr sdr = getIonsdr();

    if (blk->object) {
        sdr_free(sdr, blk->object);
        blk->object = 0;
    }
}

// Copy function: Duplicate for bundle copying
int timestampCopy(ExtensionBlock *newBlk, ExtensionBlock *oldBlk)
{
    Sdr sdr = getIonsdr();
    SdrObject newObj;

    if (oldBlk->object == 0) {
        newBlk->object = 0;
        newBlk->size = 0;
        return 0;
    }

    // Allocate new database memory
    newObj = sdr_malloc(sdr, oldBlk->size);
    if (newObj == 0) {
        return -1;
    }

    // Copy data from old to new
    char buffer[sizeof(TimestampData)];
    sdr_read(sdr, buffer, oldBlk->object, oldBlk->size);
    sdr_write(sdr, newObj, buffer, oldBlk->size);

    newBlk->object = newObj;
    newBlk->size = oldBlk->size;

    return 0;
}

// Acquire function: Parse inbound block
int timestampAcquire(AcqExtBlock *acqBlk, AcqWorkArea *work)
{
    TimestampData *data;

    // Allocate working memory
    data = (TimestampData *)MTAKE(sizeof(TimestampData));
    if (data == NULL) {
        return -1;
    }

    // Extract from serialized bytes
    if (acqBlk->length < sizeof(TimestampData)) {
        MRELEASE(data);
        return 0;  // Malformed block
    }

    memcpy(data, acqBlk->bytes + (acqBlk->length - acqBlk->dataLength),
           sizeof(TimestampData));

    acqBlk->object = (SdrObject)data;
    acqBlk->size = sizeof(TimestampData);

    return 1;  // Successfully parsed
}

// Clear function: Free working memory
void timestampClear(AcqExtBlock *acqBlk)
{
    if (acqBlk->object) {
        MRELEASE((char *)acqBlk->object);
        acqBlk->object = 0;
    }
}

// Record function: Convert from working to database memory
int timestampRecord(ExtensionBlock *blk, AcqExtBlock *acqBlk)
{
    Sdr sdr = getIonsdr();
    SdrObject dataObj;

    if (acqBlk->object == 0) {
        blk->object = 0;
        blk->size = 0;
        return 0;
    }

    // Allocate database heap
    dataObj = sdr_malloc(sdr, acqBlk->size);
    if (dataObj == 0) {
        return -1;
    }

    // Copy from working memory to database
    sdr_write(sdr, dataObj, (char *)acqBlk->object, acqBlk->size);

    blk->object = dataObj;
    blk->size = acqBlk->size;

    return 0;
}
```

### Step 3: Register the Extension

```c
// File: bpextensions.c
#include "bei.h"
#include "timestamp_ext.h"

// External function declarations
extern int  timestampOffer(ExtensionBlock *, Bundle *);
extern void timestampRelease(ExtensionBlock *);
extern int  timestampCopy(ExtensionBlock *, ExtensionBlock *);
extern int  timestampAcquire(AcqExtBlock *, AcqWorkArea *);
extern void timestampClear(AcqExtBlock *);
extern int  timestampRecord(ExtensionBlock *, AcqExtBlock *);

// Extension definitions array
ExtensionDef extensionDefs[] = {
    {
        "timestamp",              // name
        TIMESTAMP_EXT_TYPE,       // type
        timestampOffer,           // offer
        NULL,                     // serialize (not needed, using serializeExtBlk)
        {NULL, NULL, NULL, NULL, NULL},  // process callbacks (not needed for this example)
        timestampRelease,         // release
        timestampCopy,            // copy
        timestampAcquire,         // acquire
        NULL,                     // review (not needed)
        NULL,                     // decrypt (not applicable)
        NULL,                     // parse (using acquire instead)
        NULL,                     // check (not needed)
        timestampRecord,          // record
        timestampClear            // clear
    },
    {"", 0, NULL, NULL, {NULL, NULL, NULL, NULL, NULL}, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL}  // Terminator
};

// Extension specifications array
ExtensionSpec extensionSpecs[] = {
    {TIMESTAMP_EXT_TYPE, 0, NoCRC},
    {0, 0, NoCRC}  // Terminator
};
```

### Step 4: Build Integration

Add to the Makefile for linking `libbpP.so`:

```makefile
# Add timestamp_ext.o to the link command
libbpP.so: ... timestamp_ext.o
    $(CC) -shared -o libbpP.so ... timestamp_ext.o
```

Compile with the `-DBP_EXTENDED` flag when building `libbpP.c`.

---

## Common Pitfalls and Best Practices

### Return Value Conventions

The return values are **not** consistent across all callbacks:

- **offer(), copy(), record()**: Return 0 on success, -1 on system error
- **acquire(), parse()**: Return 1 on success, 0 if malformed (discard bundle), -1 on system error
- **review()**: Return 1 if all required blocks present, 0 if missing, -1 on system error
- **decrypt()**: Return 1 on success, 0 if malformed, -1 on system error
- **check()**: Return 1 if **inauthentic** (discard), 0 if authentic, -1 on system error

### Callback Function Requirements

Not all callbacks are required. Minimal implementations need:

**For basic non-encrypted extension**:
- `offer()` - create block for outbound bundles
- `release()` - free database memory
- `copy()` - duplicate blocks
- `acquire()` - parse inbound blocks
- `record()` - convert to persistent storage
- `clear()` - free working memory

**Optional callbacks**:
- `process[]` - for modifying blocks at specific points
- `review()` - for policy enforcement
- `decrypt()` - for encrypted blocks
- `parse()` - alternative to `acquire()` for potentially encrypted blocks
- `check()` - for authentication verification

### Transaction Management

Always use SDR transactions when manipulating database memory:

```c
int myOffer(ExtensionBlock *blk, Bundle *bundle)
{
    Sdr sdr = getIonsdr();
    SdrObject obj;

    CHKZERO(sdr_begin_xn(sdr));

    obj = sdr_malloc(sdr, sizeof(MyData));
    if (obj == 0) {
        sdr_cancel_xn(sdr);
        return -1;
    }

    // ... populate object ...

    if (sdr_end_xn(sdr) < 0) {
        return -1;
    }

    blk->object = obj;
    return 0;
}
```

### Testing Your Extension

1. **Unit Testing**: Test each callback independently
2. **Integration Testing**: Use `bpsource` and `bpsink` to send bundles with your extension
3. **Logging**: Use ION's logging to trace execution:
   ```c
   writeMemo("[i] Timestamp extension offered");
   ```
4. **Memory Verification**: Run ION with memory leak detection enabled
5. **Multi-hop Testing**: Verify blocks survive forwarding through intermediate nodes

---

## Debugging Guide

### Common Error Messages

**"Extension block type X not recognized"**
- Verify the extension is registered in `extensionDefs`
- Check that `-DBP_EXTENDED` was used during compilation
- Ensure `bpextensions.c` is in the include path

**"SDR transaction already in progress"**
- You may be starting a nested transaction
- Check if calling code already has a transaction open

**Memory leak detected on shutdown**
- Verify `release()` frees all allocated database memory
- Verify `clear()` frees all allocated working memory
- Use `sdr_free()` for database objects, `MRELEASE()` for working memory

### Using ION Logging

Enable detailed logging to trace extension execution:

```c
#include "platform.h"

int timestampOffer(ExtensionBlock *blk, Bundle *bundle)
{
    char buf[256];

    isprintf(buf, sizeof(buf), "[i] timestamp:offer called for bundle");
    writeMemo(buf);

    // ... implementation ...

    writeMemo("[i] timestamp:offer completed successfully");
    return 0;
}
```

Check `ion.log` for these messages during operation.

---

## ION Built-in Extension Blocks

ION includes several built-in extension block implementations. Each block is
registered in `extensionDefs[]` (enabling parsing of inbound blocks) and may
also be listed in `extensionSpecs[]` (enabling automatic attachment to
outbound bundles). A block in `extensionDefs[]` but not in `extensionSpecs[]`
can still be received and processed, but will not be generated locally.

The `extensionSpecs[]` array in `bpv7/library/ext/bpextensions.c` controls
which blocks are attached to locally-sourced bundles. Under `ION_CORE_BUILD`,
individual blocks can be enabled via compile-time flags (e.g., `PNB_EXT`,
`BAE_EXT`). In the standard build, all blocks listed in `extensionSpecs[]`
are enabled.

### Summary

| Block | Type | Standard | In `extensionSpecs[]` | Behavior |
|-------|------|----------|-----------------------|----------|
| Previous Node (PNB) | 6 | RFC 9171 | Yes | Always attached; sole source of sender ID for routing |
| Bundle Age (BAE) | 7 | RFC 9171 | Yes | Always attached; required for unsynced nodes |
| Metadata (MEB) | 8 | -- | No | Conditional offer; only when metadata is set |
| Hop Count (HCB) | 10 | RFC 9171 | No | Not attached; appropriate for loop-free topologies |
| IMC Destinations | 11 | ION | Yes | Placeholder; zero wire overhead when unused |
| SNW Permits | 12 | ION | Yes | Placeholder; zero wire overhead when unused |
| Custody Transfer (CTEB) | 13 | ION | Yes | Conditional offer; only when custody mode is active |
| Compressed Reporting (CREB) | 14 | ION | Yes | Conditional offer; only when CBR mode and SRR flags are set |
| Quality of Service (QOS) | 254 | ION | Yes | **Non-standard ION extension**; always attached; conveys class-of-service, ordinal, flow label |

### Previous Node Block (PNB) - Block Type 6

**Source:** `bpv7/library/ext/pnb/pnb.c`

The Previous Node Block identifies the node that most recently forwarded the
bundle. It is defined in RFC 9171 as an optional extension block.

**Default behavior:** Always attached. At offer time, a placeholder is
created. At dequeue time, the block is populated with the local node's admin
EID matching the proximate node's scheme, or suppressed if the EID cannot be
resolved (which in practice does not occur on a properly configured node).

**Rationale:** PNB is the sole source of sender identification in ION. No
convergence layer adapter provides sender EID out-of-band (all pass `NULL`
to `bpBeginAcq()`). The parsed sender EID flows into
`bundle->clDossier.senderFqnn`, which is used by:

- `ipnfw.c` to exclude the sender from CGR next-hop candidates (loop
  prevention)
- `imcfw.c` to exclude the sender from multicast forwarding (duplicate
  prevention)
- `libbpP.c` to detect multicast self-delivery

Removing PNB would eliminate sender-exclusion routing, increasing the risk
of routing loops and multicast duplication.

### Bundle Age Block (BAE) - Block Type 7

**Source:** `bpv7/library/ext/bae/bae.c`

The Bundle Age Block records how long a bundle has been in transit. RFC 9171
Section 4.3.3 requires BAE when the creation timestamp is zero, but ION
attaches it unconditionally.

**Default behavior:** Always attached. At offer time, `bundle->age` is
initialized to 0. At dequeue time, the age is computed from creation time
(if the clock is synchronized and creation time is known) or accumulated
from the arrival time.

**Rationale:** ION supports mixed networks where nodes with synchronized
clocks send bundles through nodes without synchronized clocks. An
unsynchronized node (`clocksync 0`) sets creation time to zero and cannot
compute expiration from another node's creation timestamp alone -- it
relies on the Bundle Age block. Without BAE, bundles from synchronized nodes
would never expire on unsynchronized forwarding nodes. The
`req-0002-bundle-age` regression test explicitly validates this mixed
scenario.

### Quality of Service Block (QOS) - Block Type 254

**Source:** `bpv7/library/ext/bpq/bpq.c`

The QOS block is an **ION-specific, non-standard extension** (not defined in
RFC 9171) that conveys class-of-service, ordinal, and flow label values between
ION nodes. It is patterned after the IETF draft-burleigh-dtn-ecos-00 Extended
Class of Service (ECOS) specification, which was proposed for but not included
in BPv7. ION maintains this extension for compatibility with BPv6-era QoS
semantics and to provide fine-grained priority control within expedited traffic.

**Wire Format:** ION's QoS block (type 254) uses a different wire format than
the IETF ECOS draft:
- **ION format:** CBOR array with 4 elements: [flags, classOfService, ordinal, dataLabel]
  - classOfService and ordinal are separate CBOR integers
  - ordinal is full 8-bit (0-254, with 255 reserved)
- **IETF draft format:** CBOR array with 5 elements with priority and ordinal
  bit-packed into a single byte (2 bits + 6 bits)

**Interoperability:** This block will not interoperate with implementations
following the IETF ECOS draft specification. It is intended for use only
between ION nodes.

**Default behavior:** Always attached. At offer time, a placeholder is
created. At dequeue time, the block is serialized with the bundle's QoS
values.

**Rationale:** QoS support is required for the target deployment. The block
conveys priority and scheduling information to ION peer nodes. Non-ION
implementations will process it as an unknown block type per RFC 9171 block
processing rules.

### Hop Count Block (HCB) - Block Type 10

**Source:** `bpv7/library/ext/hcb/hcb.c`

The Hop Count Block provides loop prevention by limiting the number of times
a bundle can be forwarded. It is defined in RFC 9171 as an optional
extension block.

**Default behavior:** Not in `extensionSpecs[]`. Locally-sourced bundles do
not carry HCB. Incoming bundles with HCB are still parsed and processed
correctly (the block definition remains in `extensionDefs[]`).

**Rationale:** The target deployment uses point-to-point, loop-free
topologies where hop count limiting is unnecessary. To re-enable, add
`{ HopCountBlk, 0, NoCRC }` to the `extensionSpecs[]` array in
`bpv7/library/ext/bpextensions.c`.

### SNW Permits Block - Block Type 12

**Source:** `bpv7/library/ext/snw/snw.c`

The SNW (Spray and Wait) block is an ION-specific extension that limits
bundle flooding on opportunistic (discovered) links.

**Default behavior:** Always attached as a placeholder. The placeholder has
zero wire overhead -- it is only serialized when `bundle->permits > 0`.
Permits are assigned during forwarding by `ipnfw.c` when a discovered
contact is selected, after the offer has already run.

**Rationale:** The placeholder must be present at offer time because the
data that determines whether SNW is needed (`bundle->permits`) does not
exist until forwarding. Removing the placeholder would require restructuring
the forwarding layer.

### IMC Destinations Block - Block Type 11

**Source:** `bpv7/library/ext/imc/imc.c`

The IMC (IPN Multicast) block is an ION-specific extension that carries the
list of multicast destination nodes.

**Default behavior:** Always attached as a placeholder. The placeholder has
zero wire overhead -- it is only serialized when multicast destinations
exist. The destination list is populated during multicast forwarding, after
the offer has already run.

**Rationale:** Same pattern as SNW. The placeholder must be present at offer
time because multicast destinations are assigned later. Removing the
placeholder would break multicast functionality.

### Metadata Extension Block (MEB) - Block Type 8

The Metadata Extension Block (block type 8) was originally defined for BPv6 in RFC 6258. In BPv7, block type 8 is not standardized and may be used differently by other implementations.

**ION's Behavior:**

ION registers block type 8 as the Metadata Extension Block, but handles non-conformant data gracefully:

- If an inbound bundle contains a block type 8 that conforms to MEB format (a CBOR array with metadata type and content), ION parses and processes it as an MEB.
- If the block data does not conform to MEB format, ION treats the block as opaque data and retains it without modification, allowing the bundle to be processed normally.

This approach ensures interoperability with other BPv7 implementations that may use block type 8 for different purposes, while preserving MEB functionality for ION-to-ION communication.

**Note:** The MEB implementation code is retained to allow ION to adapt should a Metadata Extension Block be standardized for BPv7 in the future.

### Custody Transfer Block (CTEB) - Block Type 13

**Source:** `bpv7/library/ext/cteb/cteb.c`

The CTEB is an ION-specific extension that supports Orange Book custody
transfer.

**Default behavior:** Conditional. The offer function checks whether Orange
Book custody mode is active and custody transfer was requested for the
bundle. The block is only attached when both conditions are met.

### Compressed Reporting Block (CREB) - Block Type 14

**Source:** `bpv7/library/ext/creb/creb.c`

The CREB is an ION-specific extension that supports Compressed Bundle
Reporting (CBR).

**Default behavior:** Conditional. The offer function checks whether CBR
mode is not set to TRADITIONAL and status report request (SRR) flags are
non-zero. The block is only attached when both conditions are met.

---

## Additional Resources

For more detailed information about ION's internal APIs used in extension development:

- **ICI API Documentation**: See [ICI-API.md](./ICI-API.md) for SDR management functions
- **BP Service API**: See [BP-Service-API.md](./BP-Service-API.md) for bundle protocol details
- **ION Manual Pages**: Consult `man bei`, `man sdr`, `man ion` for comprehensive API references
- **Example Extensions**: See ION source code in `bp/extensions/` for production examples like BPSec blocks
