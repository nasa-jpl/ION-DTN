#!/usr/bin/env python3
"""Generate the malicious zero-length-payload bundle for the DoS regression.

Reproduces the bundle described in GitHub Security Advisory
GHSA-27wg-h3xq-4p9g: a syntactically valid BPv7 bundle whose payload block is
present but whose block-type-specific data is the empty definite-length CBOR
byte string h'' (0x40).  In vulnerable revisions, acquiring such a bundle drove
zco_clone(..., length=0) in acquireBundle(), tripping the CHKZERO(length > 0)
assertion in ici/library/zco.c and terminating the receiving process (remote,
unauthenticated denial of service).

The bundle is built with the bespokebpv7 package so the "malicious format" is
expressed structurally rather than as an opaque hex blob.  This script only
prints the hex; the value is committed as the last entry of bundles.txt, which
runtest.py injects like every other conformance bundle (so the test itself
needs no bespokebpv7 on the CI runner).  Re-run this script if the bundle ever
needs regenerating, then paste the hex into bundles.txt.

Deterministic inputs (fixed creation time + ~100 year lifetime) keep the
generated hex stable across regenerations and ensure the bundle is never
treated as expired during acquisition.
"""
from bespokebpv7.block_enum import BlockType, CRCType
from bespokebpv7.bpv7 import BPv7

# 2026-07-01T00:00:00Z expressed as milliseconds since the DTN epoch
# (2000-01-01T00:00:00Z).
CREATION_MS = 836179200000
# ~100 years, matching the advisory PoC; guarantees the bundle is not expired.
LIFETIME_MS = 3155760000000


def build_dos_bundle() -> BPv7:
    """Return a BPv7 bundle carrying an empty (zero-length) payload block."""
    bundle = BPv7()
    # Empty payload data -> the block-type-specific field encodes as 0x40, the
    # empty definite-length CBOR byte string.
    bundle.add_payload_block(b"", crc_type=CRCType.CRC16)

    bundle.primary_block.crc_type = CRCType.CRC16
    bundle.primary_block.route.source_eid = "ipn:2.1"
    bundle.primary_block.route.dest_eid = "ipn:3.1"
    bundle.primary_block.route.report_to = "ipn:2.1"
    bundle.primary_block.life.timestamp_ms = CREATION_MS
    bundle.primary_block.life.sequence = 0
    bundle.primary_block.life.lifetime = LIFETIME_MS
    bundle.primary_block.update_crc()
    return bundle


def main() -> None:
    """Print the zero-length-payload bundle hex for pasting into bundles.txt."""
    bundle = build_dos_bundle()

    payload = bundle.get_block_by_type(BlockType.PAYLOAD_BLOCK)
    if payload is None or len(payload.data) != 0:
        raise SystemExit("payload block is not zero-length; refusing to emit")

    print(bytes(bundle).hex())


if __name__ == "__main__":
    main()
