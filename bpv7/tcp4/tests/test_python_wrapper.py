#!/usr/bin/env python3
'''
test_python_wrapper.py: Test Python ctypes wrapper for TCPCLv4 message library.

Author: iondev33

Copyright (c) 2026, California Institute of Technology.
ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
'''

import sys
import os

# Add parent directory to path for imports
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from bridge import tcpclv4_msg as msg

def test_sess_init():
    """Test SESS_INIT encode/decode round-trip."""
    print("Testing SESS_INIT encode/decode... ", end='')

    # Encode
    wire = msg.encode_sess_init(
        keepalive_interval=30,
        segment_mru=1048576,
        transfer_mru=10485760,
        node_id="ipn:2.0"
    )

    assert len(wire) > 0, "Encode produced empty data"

    # Decode
    with msg.SessionInitContext(wire) as decoded:
        assert decoded.keepalive_interval == 30, "keepalive_interval mismatch"
        assert decoded.segment_mru == 1048576, "segment_mru mismatch"
        assert decoded.transfer_mru == 10485760, "transfer_mru mismatch"
        assert decoded.node_id == b"ipn:2.0", "node_id mismatch"

    print("PASS")


def test_xfer_ack():
    """Test XFER_ACK encode."""
    print("Testing XFER_ACK encode/decode... ", end='')

    # Encode
    wire = msg.encode_xfer_ack(transfer_id=12345, ack_len=1024)

    assert len(wire) == 18, f"Expected 18 bytes, got {len(wire)}"

    # Decode
    decoded = msg.decode_xfer_ack(wire)
    assert decoded.transfer_id == 12345, "transfer_id mismatch"
    assert decoded.ack_len == 1024, "ack_len mismatch"

    print("PASS")


def test_keepalive():
    """Test KEEPALIVE encode."""
    print("Testing KEEPALIVE encode... ", end='')

    wire = msg.encode_keepalive()

    assert len(wire) == 1, f"Expected 1 byte, got {len(wire)}"
    assert wire[0] == msg.MSG_KEEPALIVE, "Wrong message type byte"

    print("PASS")


def test_msg_type_detection():
    """Test message type detection."""
    print("Testing message type detection... ", end='')

    # Create different message types
    keepalive_wire = msg.encode_keepalive()
    ack_wire = msg.encode_xfer_ack(123, 456)

    assert msg.get_msg_type(keepalive_wire) == msg.MSG_KEEPALIVE
    assert msg.get_msg_type(ack_wire) == msg.MSG_XFER_ACK

    # Test name lookup
    assert msg.msg_type_name(msg.MSG_KEEPALIVE) == "KEEPALIVE"
    assert msg.msg_type_name(msg.MSG_SESS_INIT) == "SESS_INIT"
    assert msg.msg_type_name(0xFF) == "UNKNOWN"

    print("PASS")


def main():
    print("TCPCLv4 Message Python Wrapper Tests")
    print("=====================================\n")

    try:
        test_sess_init()
        test_xfer_ack()
        test_keepalive()
        test_msg_type_detection()

        print("\n=====================================")
        print("ALL TESTS PASSED")
        return 0

    except AssertionError as e:
        print(f"FAIL: {e}")
        return 1
    except Exception as e:
        print(f"ERROR: {e}")
        import traceback
        traceback.print_exc()
        return 1


if __name__ == '__main__':
    sys.exit(main())
