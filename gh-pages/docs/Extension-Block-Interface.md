# BP Extension Interface

ION offers software developer a set of standard interface for adding extensions to Bundle Protocol without modifying the core BP source code. This capability can be used to implement both standardized bundle extension blocks or user-specific extension blocks.

ION's interface for extending the Bundle Protocol enables the definition of external functions that insert extension blocks into outbound bundles (either before or after the payload block), parse and record extension blocks in inbound bundles, and modify extension blocks at key points in bundle processing. All extension-block handling is statically linked into ION at build time, but the addition of an extension never requires that any standard ION source code be modified.

Standard structures for recording extension blocks -- both in transient storage [memory] during bundle acquisition (AcqExtBlock) and in persistent storage [the ION database] during subsequent bundle processing (ExtensionBlock) -- are defined in the bei.h header file. In each case, the extension block structure comprises a block type code, block processing flags, possibly a list of EID references, an array of bytes (the serialized form of the block, for transmission), the length of that array, optionally an extension-specific opaque object whose structure is designed to characterize the block in a manner that's convenient for the extension processing functions, and the size of that object.

The definition of each extension is asserted in an ExtensionDef structure, also as defined in the bei.h header file. Each ExtensionDef must supply:

... add material from man page and put in same format as previous API pages.