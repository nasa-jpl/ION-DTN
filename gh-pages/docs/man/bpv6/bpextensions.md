# NAME

bpextensions - interface for adding extensions to Bundle Protocol

# SYNOPSIS

    #include "bpextensions.c"

# DESCRIPTION

ION's interface for extending the Bundle Protocol enables the definition
of external functions that insert _extension_ blocks into outbound bundles
(either before or after the payload block), parse and record extension
blocks in inbound bundles, and modify extension blocks at key points in
bundle processing.  All extension-block handling is statically linked into
ION at build time, but the addition of an extension never requires that
any standard ION source code be modified.

Standard structures for recording extension blocks -- both in transient
storage \[memory\] during bundle acquisition (AcqExtBlock) and in persistent
storage \[the ION database\] during subsequent bundle processing (ExtensionBlock)
\-- are defined in the _bei.h_ header file.  In each case, the extension
block structure comprises a block _type_ code, block processing _flags_,
possibly a list of _EID references_, an array of _bytes_ (the
serialized form of the block, for transmission), the _length_ of that
array, optionally an extension-specific opaque _object_ whose structure
is designed to characterize the block in a manner that's convenient for
the extension processing functions, and the _size_ of that object.

The definition of each extension is asserted in an ExtensionDef structure,
also as defined in the _bei.h_ header file.  Each ExtensionDef must supply:

> The name of the extension.  (Used in some diagnostic messages.)
>
> The extension's block type code.
>
> A pointer to an **offer** function.
>
> A pointer to a function to be called when **forwarding** a bundle containing
> this sort of block.
>
> A pointer to a function to be called when **taking custody** of a bundle
> containing this sort of block.
>
> A pointer to a function to be called when **enqueuing** for transmission a
> bundle containing this sort of block.
>
> A pointer to a function to be called when a convergence-layer adapter
> **dequeues** a bundle containing this sort of block, before serializing it.
>
> A pointer to a function to be called immediately before a convergence-layer
> adapter **transmits** a bundle containing this sort of block, after the
> bundle has been serialized.
>
> A pointer to a **release** function.
>
> A pointer to a **copy** function.
>
> A pointer to an **acquire** function.
>
> A pointer to a **decrypt** function.
>
> A pointer to a **parse** function.
>
> A pointer to a **check** function.
>
> A pointer to a **record** function.
>
> A pointer to a **clear** function.

All extension definitions must be coded into an array of ExtensionDef
structures named _extensionDefs_.

An array of ExtensionSpec structures named _extensionSpecs_ is
also required.  Each ExtensionSpec provides the specification for
producing an outbound extension block: block definition (identified by
block type number), three discriminator tags whose semantics are
block-type-specific, and a list index value indicating whether the
extension block is to be inserted before or after the Payload block.
The order of appearance of extension specifications in the extensionSpecs
array determines the order in which extension blocks will be inserted
into locally sourced bundles.

The standard extensionDefs array -- which is empty -- is in the
_noextensions.c_ prototype source file.  The procedure for extending
the Bundle Protocol in ION is as follows:

1\.  Specify -DBP\_EXTENDED in the Makefile's compiler command line when
building the libbpP.c library module.

2\.  Create a copy of the prototype extensions file, named "bpextensions.c",
in a directory that is made visible to the Makefile's libbpP.c compilation
command line (by a -I parameter).

3\.  In the "external function declarations" area of "bpextensions.c", add
"extern" function declarations identifying the functions that will implement
your extension (or extensions).

4\.  Add one or more ExtensionDef structure initialization lines to the
extensionDefs array, referencing those declared functions.

5\.  Add one or more ExtensionSpec structure initialization lines to the
extensionSpecs array, referencing those extension definitions.

6\.  Develop the implementations of the extension implementation functions
in one or more new source code files.

7\.  Add the object file or files for the new extension implementation source
file (or files) to the Makefile's command line for linking libbpP.so.

The function pointers supplied in each ExtensionDef must conform to the
following specifications.  NOTE that any function that modifies the _bytes_
member of an ExtensionBlock or AckExtBlock **must** set the corresponding
_length_ to the new length of the _bytes_ array, if changed.

- int (\*BpExtBlkOfferFn)(ExtensionBlock \*blk, Bundle \*bundle)

    Populates all fields of the indicated ExtensionBlock structure
    for inclusion in the indicated outbound bundle.  This function is
    automatically called when a new bundle is locally sourced or
    upon acquisition of a remotely sourced bundle that does not
    contain an extension block of this type.  The values of the extension
    block are typically expected to be a function of the state of the bundle,
    but this is extension-specific.  If it is not appropriate to offer an
    extension block of this type as part of this bundle, then the _size_,
    _length_, _object_, and _bytes_ members of _blk_ must all be set to
    zero.  If it is appropriate to offer such a block but no internal object
    representing the state of the block is needed, the _object_ and _size_
    members of _blk_ must be set to zero.  The _type_, _blkProcFlags_, and
    _dataLength_ members of _blk_ must be populated by the implementation of
    the "offer" function, but the _length_ and _bytes_ members are typically
    populated by calling the BP library function serializeExtBlk(), which must
    be passed the block to be serialized (with _type_, _blkProcFlags_ and
    _dataLength_ already set), a Lyst of EID references (two list elements
    \-- offsets -- per EID reference, if applicable; otherwise NULL), and a
    pointer to the extension-specific block data.  The block's _bytes_ array
    and _object_ (if present) must occupy space allocated from the ION database
    heap.  Return zero on success, -1 on any system failure.

- int (\*BpExtBlkProcessFn)(ExtensionBlock \*blk, Bundle \*bundle, void \*context)

    Performs some extension-specific transformation of the data encapsulated in
    _blk_ based on the state of _bundle_.  The transformation to be performed
    will typically vary depending on whether the identified function is the one
    that is automatically invoked upon forwarding the bundle, upon taking custody
    of the bundle, upon enqueuing the bundle for transmission, upon removing
    the bundle from the transmission queue, or upon transmitting the serialized
    bundle.  The _context_ argument may supply useful supplemental information;
    in particular, the context provided to the ON\_DEQUEUE function will comprise
    the name of the protocol for the duct from which the bundle has been dequeued,
    together with the EID of the neighboring node endpoint to which the bundle
    will be directly transmitted when serialized.  The block-specific data in _blk_
    is located within _bytes_ immediately after the header of the extension
    block; the length of the block's header is the difference between _length_
    and _dataLength_.  Whenever the block's _blkProcFlags_, EID extensions,
    and/or block-specific data are altered, the serializeExtBlk() function should
    be called again to recalculate the size of the extension block and rebuild
    the _bytes_ array.  Return zero on success, -1 on any system failure.

- void (\*BpExtBlkReleaseFn)(ExtensionBlock \*blk)

    Releases all ION database space occupied by the _object_ member of _blk_.
    This function is automatically called when a bundle is destroyed.  Note that
    incorrect implementation of this function may result in a database space leak.

- int (\*BpExtBlkCopyFn)(ExtensionBlock \*newblk, ExtensionBlock \*oldblk)

    Copies the _object_ member of _oldblk_ to ION database heap space and
    places the address of that new non-volatile object in the _object_ member of
    _newblk_, also sets _size_ in _newblk_.  This function is automatically
    called when two copies of a bundle are needed, e.g., in the event that it
    must both be delivered to a local client and also fowarded to another node.
    Return zero on success, -1 on any system failure.

- int (\*BpAcqExtBlkAcquireFn)(AcqExtBlock \*acqblk, AcqWorkArea \*work)

    Populates the indicated AcqExtBlock structure with _size_ and _object_
    for retention as part of the indicated inbound bundle.  (The _type_,
    _blkProcFlags_, EID references (if any), _dataLength_, _length_, and
    _bytes_ values of the structure are pre-populated with data as extracted
    from the serialized bundle.)  This function is only to be provided for
    extension blocks that are never encrypted; a extension block that may be
    encrypted should have a BpAcqExtBlkParseFn callback instead.  The function
    is automatically called
    when an extension block of this type is encountered in the course of
    parsing and acquiring a bundle for local delivery and/or forwarding.  If
    no internal object representing the state of the block is needed, the
    _object_ member of _acqblk_ must be set to NULL and the _size_
    member must be set to zero.  If an _object_ is needed for this block,
    it must occupy space that is allocated from ION working memory using
    **MTAKE** and its _size_ must be indicated in _blk_.  Return zero if
    the block is malformed (this will cause the bundle to be discarded), 1 if
    the block is successfully parsed, -1 on any system failure.

- int (\*BpAcqExtBlkDecryptFn)(AcqExtBlock \*acqblk, AcqWorkArea \*work)

    Decrypts some other extension block that has been acquired but not yet
    parsed, nominally using encapsulated ciphersuite information.  Return
    zero if the block is malformed (this will cause the bundle to be discarded),
    1 if no error in decryption was encountered, -1 on any system failure.

- int (\*BpAcqExtBlkParseFn)(AcqExtBlock \*acqblk, AcqWorkArea \*work)

    Populates the indicated AcqExtBlock structure with _size_ and _object_
    for retention as part of the indicated inbound bundle.  (The _type_,
    _blkProcFlags_, EID references (if any), _dataLength_, _length_, and
    _bytes_ values of the structure are pre-populated with data as extracted
    from the serialized bundle.)  This function is provided for extension
    blocks that may be encrypted; a extension block that can never be
    encrypted should have a BpAcqExtBlkAcquireFn callback instead.  The function
    is automatically called
    when an extension block of this type is encountered in the course of
    parsing and acquiring a bundle for local delivery and/or forwarding.  If
    no internal object representing the state of the block is needed, the
    _object_ member of _acqblk_ must be set to NULL and the _size_
    member must be set to zero.  If an _object_ is needed for this block,
    it must occupy space that is allocated from ION working memory using
    **MTAKE** and its _size_ must be indicated in _blk_.  Return zero if
    the block is malformed (this will cause the bundle to be discarded), 1 if
    the block is successfully parsed, -1 on any system failure.

- int (\*BpAcqExtBlkCheckFn)(AcqExtBlock \*acqblk, AcqWorkArea \*work)

    Examines the bundle in _work_ to determine whether or not it is authentic,
    in the context of the indicated extension block.  Return 1 if the block is
    determined to be inauthentic (this will cause the bundle to be discarded),
    zero if no inauthenticity is detected, -1 on any system failure.

- int (\*BpExtBlkRecordFn)(ExtensionBlock \*blk, AcqExtBlock \*acqblk)

    Copies the _object_ member of _acqblk_ to ION database heap space and
    places the address of that non-volatile object in the _object_ member of
    _blk_; also sets _size_ in _blk_.  This function is automatically called
    when an acquired bundle is accepted for forwarding and/or delivery.  Return
    zero on success, -1 on any system failure.

- void (\*BpAcqExtBlkClearFn)(AcqExtBlock \*acqblk)

    Uses **MRELEASE** to release all ION working memory occupied by the _object_
    member of _acqblk_.  This function is automatically called when acquisition
    of a bundle is completed, whether or not the bundle is accepted.  Note that
    incorrect implementation of this function may result in a working memory leak.

## UTILITY FUNCTIONS FOR EXTENSION PROCESSING

- void discardExtensionBlock(AcqExtBlock \*blk)

    Deletes this block from the bundle acquisition work area prior to the
    recording of the bundle in the ION database.

- void scratchExtensionBlock(ExtensionBlock \*blk)

    Deletes this block from the bundle after the bundle has been recorded in the
    ION database.

- Object findExtensionBlock(Bundle \*bundle, unsigned int type, unsigned int listIdx)

    On success, returns the address of the ExtensionBlock in _bundle_ for the
    indicated _type_ and _listIdx_.  If no such extension block exists, returns
    zero.

- int serializeExtBlk(ExtensionBlock \*blk, Lyst eidReferences, char \*blockData)

    Constructs an RFC5050-conformant serialized representation of this extension
    block in blk->bytes.  Returns 0 on success, -1 on an unrecoverable system
    error.

- void suppressExtensionBlock(ExtensionBlock \*blk)

    Causes _blk_ to be omitted when the bundle to which it is attached is
    serialized for transmission.  This suppression remains in effect until
    it is reversed by restoreExtensionBlock();

- void restoreExtensionBlock(ExtensionBlock \*blk)

    Reverses the effect of suppressExtensionBlock(), enabling the block to
    be included when the bundle to which it is attached is serialized.

# SEE ALSO

bp(3)
