#!/usr/bin/env python3
"""Generate the fixture bundles for the extension-block-count regression.

`parseExtensionBlocks()` recorded each parsed extension block's number in a
fixed 32-entry stack array indexed by a running count that was never bounded.
A bundle carrying more than 32 recognized extension blocks with distinct block
numbers drove the count past the array and wrote attacker-controlled block
numbers into adjacent stack memory, up to the saved return address.

This emits two bundles, both ipn:2.1 -> ipn:3.1:

  flood    a bundle carrying 40 Bundle Age extension blocks (block type 7) with
           distinct block numbers 2..41 -- well past the 32-entry array.  Each
           carries a valid Bundle Age (a CBOR uint), so every block is parsed
           and counted rather than discarded, and the distinct numbers keep the
           duplicate check from short-circuiting before the overflow.

  control   an ordinary single-payload bundle, otherwise identical, used to
           prove the receiving node is still alive after handling the flood.

Bundles are emitted as hex for pasting into bundles.txt, so the test needs no
CBOR library at run time.  BPv7 wraps a bundle in an indefinite-length array
(0x9f .. 0xff); cbor2 encodes each block definite and this concatenates them.
The primary and payload blocks carry a CRC-16 (ION rejects a primary block that
has neither a CRC nor a BIB); extension blocks are left CRC-less.

Deterministic inputs keep the emitted hex stable across regenerations.
"""
import cbor2

CREATION_MS = 836179200000
LIFETIME_MS = 3155760000000
N_EXT_BLOCKS = 40          # comfortably past the 32-entry array
BUNDLE_AGE_BLK = 7
PAYLOAD_BLK = 1
CRC16 = 1

DEST = [2, [3, 1]]         # ipn:3.1
SRC = [2, [2, 1]]          # ipn:2.1
REPORT_TO = [2, [2, 1]]    # ipn:2.1


def crc16_x25(data):
    """BPv7 CRC-16 (X.25): reflected, poly 0x1021, init/xorout 0xFFFF."""
    crc = 0xFFFF
    for b in data:
        crc ^= b
        for _ in range(8):
            crc = (crc >> 1) ^ 0x8408 if (crc & 1) else crc >> 1
    return crc ^ 0xFFFF


def with_crc(block_list):
    """Fill in the trailing CRC-16 field of a block encoded with a zero CRC."""
    block_list[-1] = b"\x00\x00"
    block_list[-1] = crc16_x25(cbor2.dumps(block_list)).to_bytes(2, "big")
    return cbor2.dumps(block_list)


def primary_block():
    return with_crc([7, 0, CRC16, DEST, SRC, REPORT_TO,
                     [CREATION_MS, 0], LIFETIME_MS, None])


def payload_block(data):
    return with_crc([PAYLOAD_BLK, 1, 0, CRC16, data, None])


def ext_block(number, data):
    """CRC-less canonical extension block: [type, number, flags, crc_type, data]."""
    return cbor2.dumps([BUNDLE_AGE_BLK, number, 0, 0, data])


def bundle_hex(ext_blocks, payload):
    out = b"\x9f" + primary_block()
    for b in ext_blocks:
        out += b
    out += payload_block(payload) + b"\xff"
    return out.hex()


def main():
    age = cbor2.dumps(0)   # Bundle Age block data: CBOR uint (age in ms)
    flood = [ext_block(n, age) for n in range(2, 2 + N_EXT_BLOCKS)]

    print("# control: ordinary single-payload bundle (node must survive + deliver)")
    print(bundle_hex([], b"control"))
    print(f"# flood: {N_EXT_BLOCKS} Bundle Age extension blocks, distinct numbers 2..{1 + N_EXT_BLOCKS}")
    print(bundle_hex(flood, b"flood"))


if __name__ == "__main__":
    main()
