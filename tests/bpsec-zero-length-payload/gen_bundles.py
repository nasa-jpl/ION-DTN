#!/usr/bin/env python3
"""Generate the two fixture bundles used by this test.

Both bundles travel ipn:2.1 -> ipn:3.1 and carry a CRC-16 primary block and a
CRC-16 payload block.  They differ in exactly one respect: the length of the
payload block's block-type-specific data.

  zero     payload data is b"" -- encodes as h'' (0x40), the empty
           definite-length CBOR byte string.  This is the case under test:
           canonicalizing a zero-length payload for a BIB passed
           payload.length straight to zco_clone(), whose CHKZERO(length > 0)
           precondition terminated the signing or verifying process.

  control  payload data is a short non-empty string.  Identical in every other
           way, so it isolates payload length as the only variable: if the
           control is not delivered, the node configuration or the BPSec
           policy is wrong rather than the zero-length case being broken.

Bundles are built with bespokebpv7 so the structure is expressed structurally
rather than as an opaque hex blob.  This script only prints hex; the values are
committed to bundles.txt, which dotest injects.  The test therefore needs no
bespokebpv7 on the CI runner.  Re-run this script if the fixtures ever need
regenerating, then paste the output into bundles.txt.

Usage:
    PYTHONPATH=<path-to>/bespokebpv7/src ./gen_bundles.py

Deterministic inputs (fixed creation time, ~100 year lifetime) keep the emitted
hex stable across regenerations and ensure neither bundle is ever treated as
expired during acquisition.
"""
from bespokebpv7.block_enum import BlockType, CRCType
from bespokebpv7.bpv7 import BPv7

# 2026-07-01T00:00:00Z expressed as milliseconds since the DTN epoch
# (2000-01-01T00:00:00Z).  Matches tests/conformance_test/gen_dos_bundle.py.
CREATION_MS = 836179200000
# ~100 years; guarantees the bundle is not expired when acquired.
LIFETIME_MS = 3155760000000

CONTROL_PAYLOAD = b"control"


def build_bundle(payload: bytes, sequence: int) -> BPv7:
    """Return a bundle ipn:2.1 -> ipn:3.1 carrying the given payload bytes."""
    bundle = BPv7()
    bundle.add_payload_block(payload, crc_type=CRCType.CRC16)

    bundle.primary_block.crc_type = CRCType.CRC16
    bundle.primary_block.route.source_eid = "ipn:2.1"
    bundle.primary_block.route.dest_eid = "ipn:3.1"
    bundle.primary_block.route.report_to = "ipn:2.1"
    bundle.primary_block.life.timestamp_ms = CREATION_MS
    # Distinct sequence numbers keep the two bundles from colliding in the
    # receiver's duplicate-suppression state.
    bundle.primary_block.life.sequence = sequence
    bundle.primary_block.life.lifetime = LIFETIME_MS
    bundle.primary_block.update_crc()
    return bundle


def payload_length(bundle: BPv7) -> int:
    """Return the length of the bundle's payload block data."""
    payload = bundle.get_block_by_type(BlockType.PAYLOAD_BLOCK)
    if payload is None:
        raise SystemExit("bundle has no payload block; refusing to emit")

    return len(payload.data)


def main() -> None:
    """Print both fixture bundles as hex, for pasting into bundles.txt."""
    control = build_bundle(CONTROL_PAYLOAD, sequence=0)
    zero = build_bundle(b"", sequence=1)

    if payload_length(control) != len(CONTROL_PAYLOAD):
        raise SystemExit("control payload length is wrong; refusing to emit")

    if payload_length(zero) != 0:
        raise SystemExit("payload block is not zero-length; refusing to emit")

    print("# control: non-empty payload, otherwise identical to the zero case")
    print(bytes(control).hex())
    print("# zero: empty CBOR byte string h'' payload -- the case under test")
    print(bytes(zero).hex())


if __name__ == "__main__":
    main()
