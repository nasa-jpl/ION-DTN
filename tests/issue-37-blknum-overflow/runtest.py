#!/usr/bin/env python3
"""
Test script for GitHub issue #37: block number overflow handling.

Creates a BPv7 bundle with extension block number 255 using bespokebpv7,
sends it to ION node 2, and receives/verifies it (acting as node 3).

Nate Richard / Jay Gao  JPL  2026
"""

import argparse
import socket
import sys
import threading
import time

try:
    from bespokebpv7.block_enum import (  # type: ignore[import-untyped]
        BlockFlags,
        BlockType,
        CRCType,
    )
    from bespokebpv7.blocks import CanonicalBlock  # type: ignore[import-untyped]
    from bespokebpv7.bpv7 import BPv7  # type: ignore[import-untyped]
except ImportError:
    print("SKIP: bespokebpv7 module not found. Please install bespokebpv7.")
    sys.exit(2)


# Monkeypatch BlockType to accept arbitrary integer values
# This allows creating extension blocks with unknown type codes for V&V testing
def _blocktype_missing(cls, value):
    """Accept any integer value as a valid block type for testing purposes."""
    if isinstance(value, int):
        # Create a pseudo-member that acts like an enum member
        pseudo_member = int.__new__(cls, value)
        pseudo_member._name_ = f"UNKNOWN_{value}"
        pseudo_member._value_ = value
        return pseudo_member
    raise ValueError(f"{value} is not a valid {cls.__name__}")


BlockType._missing_ = classmethod(_blocktype_missing)  # pyright: ignore[reportAttributeAccessIssue]


def create_test_bundle() -> bytes:
    """
    Create a BPv7 bundle with extension block number 255.

    The bundle intentionally omits standard extension blocks (PreviousNode,
    BundleAge) so that the forwarding node must add them through
    patchExtensionBlocks -> selectBlkNumber.
    """
    # Create bundle with basic routing
    bundle = BPv7()
    bundle.primary_block.route.source_eid = "ipn:2.1"
    bundle.primary_block.route.dest_eid = "ipn:3.1"
    bundle.primary_block.route.report_to = "dtn:none"
    bundle.primary_block.crc_type = CRCType.CRC16
    bundle.primary_block.life.lifetime = 86400000  # 1 day in ms
    bundle.primary_block.set_creation()

    # Add payload block (block number 1)
    bundle.add_payload_block(b"Hello")

    # Create extension block with number 255 (unknown type 200)
    ext_block = CanonicalBlock(
        flags=BlockFlags(0),
        block_type=BlockType(200),
        block_number=255,
        crc_type=CRCType.NONE,
        data=bytes.fromhex("00"),
    )

    # Add to bundle's blocks dictionary
    bundle.blocks[BlockType(200)] = ext_block

    # Update CRCs
    bundle.primary_block.update_crc()

    # Serialize to bytes
    return bytes(bundle)


def receive_bundle(bind_port: int, timeout: int, result_holder: dict) -> None:
    """
    Receive bundle on specified UDP port and verify structure.
    Acts as node 3 endpoint.

    Args:
        bind_port: UDP port to bind and listen on
        timeout: Timeout in seconds
        result_holder: Dict to store results (success, message)
    """
    sock = None
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            sock.settimeout(timeout)
            sock.bind(("127.0.0.1", bind_port))

            print(f"[Receiver] Listening on 127.0.0.1:{bind_port} (acting as node 3)")

            data, addr = sock.recvfrom(8192)
    except TimeoutError:
        result_holder["success"] = False
        result_holder["message"] = (
            f"✗ Timeout waiting for bundle after {timeout} seconds"
        )
        print(f"[Receiver] {result_holder['message']}")
        return

    except OSError as e:
        result_holder["success"] = False
        result_holder["message"] = f"✗ Error receiving/parsing bundle: {e}"
        print(f"[Receiver] {result_holder['message']}")
        import traceback

        traceback.print_exc()
        return

    received_bundle = BPv7(data)
    print(f"[Receiver] Received {len(data)} bytes from {addr}")
    print(f"{received_bundle}")

    # Collect all block numbers for validation
    block_numbers = []
    for block in received_bundle.blocks.values():
        block_numbers.append(block.block_number)
        print(f"[Receiver] Found block type {block.block_type} with block_number {block.block_number}")

    # Check for block number 0 (overflow indicator)
    if 0 in block_numbers:
        result_holder["success"] = False
        result_holder["message"] = "✗ Block with number 0 found (overflow occurred)"
        print(f"[Receiver] {result_holder['message']}")
        return

    # Check for duplicate block number 1 (should only be payload)
    if block_numbers.count(1) > 1:
        result_holder["success"] = False
        result_holder["message"] = f"✗ Duplicate block number 1 found ({block_numbers.count(1)} occurrences)"
        print(f"[Receiver] {result_holder['message']}")
        return

    # Verify block 255 is present
    if BlockType(200) in received_bundle.blocks:
        ext_block = received_bundle.blocks[BlockType(200)]
        if ext_block.block_number == 255:
            result_holder["success"] = True
            result_holder["message"] = "✓ Block 255 found, no block 0, no duplicate block 1"
            print(f"[Receiver] {result_holder['message']}")
        else:
            result_holder["success"] = False
            result_holder["message"] = (
                f"✗ Block found but number is {ext_block.block_number}, expected 255"
            )
            print(f"[Receiver] {result_holder['message']}")
    else:
        result_holder["success"] = False
        result_holder["message"] = (
            "✗ Extension block type 200 not found in received bundle"
        )
        print(f"[Receiver] {result_holder['message']}")
        print(f"[Receiver] Available blocks: {list(received_bundle.blocks.keys())}")


def send_bundle(bundle_bytes: bytes, send_port: int) -> bool:
    """
    Send bundle via UDP to ION node.

    Args:
        bundle_bytes: Serialized bundle data
        send_port: UDP port to send to (node 2 induct)

    Returns:
        True if sent successfully, False otherwise
    """
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
            sock.sendto(bundle_bytes, ("127.0.0.1", send_port))
    except OSError as e:
        print(f"✗ Error sending bundle: {e}")
        return False
    print(f"[Sender] Bundle sent to 127.0.0.1:{send_port}")
    print(f"{BPv7(bundle_bytes)}")
    return True


def main() -> int:
    """
    Main test execution.

    Returns:
        Exit code: 0=success, 1=failure, 2=skip
    """
    parser = argparse.ArgumentParser(
        description="Test ION block number overflow handling (issue #37)"
    )
    parser.add_argument(
        "--send-port",
        type=int,
        default=2113,
        help="UDP port to send bundle to (node 2 induct)",
    )
    parser.add_argument(
        "--recv-port",
        type=int,
        default=3113,
        help="UDP port to receive bundle on (acting as node 3)",
    )
    parser.add_argument(
        "--timeout",
        type=int,
        default=15,
        help="Timeout in seconds for bundle reception",
    )

    args = parser.parse_args()

    print("=" * 70)
    print("ION Issue #37 Test: Block Number Overflow Handling")
    print("=" * 70)

    # Create test bundle
    print("\n[Setup] Creating bundle with extension block 255...")
    try:
        bundle_bytes = create_test_bundle()
    except ValueError as e:
        print(f"✗ Failed to create bundle: {e}")
        import traceback

        traceback.print_exc()
        return 1

    print("[Setup] ✓ Bundle created successfully")

    # Start receiver thread (acting as node 3)
    print(f"\n[Setup] Starting receiver on port {args.recv_port}...")
    result_holder = {"success": False, "message": ""}
    receiver_thread = threading.Thread(
        target=receive_bundle,
        args=(args.recv_port, args.timeout, result_holder),
        daemon=True,
    )
    receiver_thread.start()

    # Give receiver time to bind
    time.sleep(1)

    # Send bundle
    print(f"\n[Test] Sending bundle to node 2 on port {args.send_port}...")
    if not send_bundle(bundle_bytes, args.send_port):
        return 1

    # Wait for receiver to complete
    print(f"\n[Test] Waiting for bundle reception (timeout: {args.timeout}s)...")
    receiver_thread.join(timeout=args.timeout + 2)

    # Check results
    print("\n" + "=" * 70)
    print("Test Results")
    print("=" * 70)
    print(result_holder.get("message", "✗ No result from receiver"))

    if result_holder["success"]:
        print("\n✓ SUCCESS: Bundle with block 255 sent and verified")
        return 0
    else:
        print("\n✗ FAILURE: Bundle verification failed")
        return 1


if __name__ == "__main__":
    sys.exit(main())
