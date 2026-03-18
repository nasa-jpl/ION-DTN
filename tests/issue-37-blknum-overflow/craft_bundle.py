#!/usr/bin/env python3
"""
Crafts a BPv7 bundle with an extension block numbered 255, then sends it
via UDP to a local ION node.  The bundle intentionally omits standard
extension blocks (PreviousNode, BundleAge, QoS) so that the forwarding
node must add them through patchExtensionBlocks -> selectBlkNumber.

This exercises the fix for GitHub issue #37: if block number 255 is
already present, selectBlkNumber must pick a lower available number
instead of overflowing to 0.

Nate Richard / Jay Gao  JPL  2026
"""
import socket
import struct
import time


# ---------------------------------------------------------------------------
#  CRC-16-X.25 (ISO/IEC 13239) used by BPv7 (RFC 9171 Section 4.2.1)
# ---------------------------------------------------------------------------

def crc16_x25(data: bytes) -> int:
    """Compute CRC-16/X.25 over *data* and return a 16-bit integer."""
    crc = 0xFFFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            if crc & 1:
                crc = (crc >> 1) ^ 0x8408   # reflected polynomial
            else:
                crc >>= 1
    return crc ^ 0xFFFF


# ---------------------------------------------------------------------------
#  Minimal CBOR encoding helpers (deterministic, no external deps)
# ---------------------------------------------------------------------------

def _cbor_uint(major: int, value: int) -> bytes:
    """Encode a CBOR unsigned integer with the given major type (0-7)."""
    mt = major << 5
    if value < 24:
        return bytes([mt | value])
    if value < 0x100:
        return bytes([mt | 24, value])
    if value < 0x10000:
        return bytes([mt | 25]) + struct.pack(">H", value)
    if value < 0x100000000:
        return bytes([mt | 26]) + struct.pack(">I", value)
    return bytes([mt | 27]) + struct.pack(">Q", value)


def cbor_uint(value: int) -> bytes:
    """CBOR unsigned integer (major type 0)."""
    return _cbor_uint(0, value)


def cbor_bstr(data: bytes) -> bytes:
    """CBOR byte string (major type 2)."""
    return _cbor_uint(2, len(data)) + data


def cbor_tstr(text: str) -> bytes:
    """CBOR text string (major type 3)."""
    encoded = text.encode("utf-8")
    return _cbor_uint(3, len(encoded)) + encoded


def cbor_array(items: list[bytes]) -> bytes:
    """CBOR definite-length array (major type 4)."""
    header = _cbor_uint(4, len(items))
    return header + b"".join(items)


# ---------------------------------------------------------------------------
#  BPv7 bundle construction
# ---------------------------------------------------------------------------

def build_ipn_eid(node: int, service: int) -> bytes:
    """Encode an IPN EID as CBOR: [2, [node, service]]."""
    return cbor_array([cbor_uint(2), cbor_array([cbor_uint(node), cbor_uint(service)])])


def build_dtn_none() -> bytes:
    """Encode dtn:none EID as CBOR: [1, 0]."""
    return cbor_array([cbor_uint(1), cbor_uint(0)])


def build_primary_block() -> bytes:
    """Build the BPv7 primary block with CRC-16."""
    # Creation timestamp: milliseconds since DTN epoch (2000-01-01 00:00:00 UTC)
    # Current time in DTN epoch
    dtn_epoch = 946684800  # Unix timestamp of 2000-01-01 00:00:00 UTC
    dtn_time_ms = int((time.time() - dtn_epoch) * 1000)

    # Primary block fields (9 items = has CRC)
    version = cbor_uint(7)
    bundle_flags = cbor_uint(0)          # no special flags
    crc_type = cbor_uint(1)              # CRC-16
    dest_eid = build_ipn_eid(3, 1)       # ipn:3.1
    src_eid = build_ipn_eid(2, 1)        # ipn:2.1
    report_to = build_dtn_none()         # dtn:none
    creation_ts = cbor_array([cbor_uint(dtn_time_ms), cbor_uint(0)])
    lifetime = cbor_uint(86400000)       # 1 day in ms

    # Build the block with CRC placeholder (2 zero bytes)
    crc_placeholder = cbor_bstr(b"\x00\x00")
    block_with_zeros = cbor_array([
        version, bundle_flags, crc_type,
        dest_eid, src_eid, report_to,
        creation_ts, lifetime,
        crc_placeholder,
    ])

    # Compute CRC-16-X.25 over the block with zeroed CRC field
    crc_value = crc16_x25(block_with_zeros)
    crc_bytes = struct.pack(">H", crc_value)

    # Replace the placeholder with the real CRC
    real_crc = cbor_bstr(crc_bytes)
    block_final = cbor_array([
        version, bundle_flags, crc_type,
        dest_eid, src_eid, report_to,
        creation_ts, lifetime,
        real_crc,
    ])

    return block_final


def build_extension_block_255() -> bytes:
    """Build an extension block with block number 255 (unknown type 200)."""
    block_type = cbor_uint(200)      # unknown / private-use block type
    block_number = cbor_uint(255)    # THE key value for this test
    proc_flags = cbor_uint(0)        # no special flags (keep block even if unknown)
    crc_type = cbor_uint(0)          # no CRC
    block_data = cbor_bstr(b"\x00")  # 1 byte of dummy data

    return cbor_array([block_type, block_number, proc_flags, crc_type, block_data])


def build_payload_block() -> bytes:
    """Build the payload block (block type 1, number 1)."""
    block_type = cbor_uint(1)
    block_number = cbor_uint(1)
    proc_flags = cbor_uint(0)
    crc_type = cbor_uint(0)
    block_data = cbor_bstr(b"Hello")

    return cbor_array([block_type, block_number, proc_flags, crc_type, block_data])


def build_bundle() -> bytes:
    """Assemble a complete BPv7 bundle as a CBOR indefinite-length array."""
    primary = build_primary_block()
    ext_255 = build_extension_block_255()
    payload = build_payload_block()

    # Indefinite-length array: 0x9F ... 0xFF
    return b"\x9f" + primary + ext_255 + payload + b"\xff"


# ---------------------------------------------------------------------------
#  Send via UDP
# ---------------------------------------------------------------------------

def main() -> None:
    bundle = build_bundle()
    print(f"Bundle size: {len(bundle)} bytes")
    print(f"Bundle hex:  {bundle.hex()}")

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.sendto(bundle, ("127.0.0.1", 2113))
    print("Bundle sent to 127.0.0.1:2113 (node 2 induct)")


if __name__ == "__main__":
    main()
