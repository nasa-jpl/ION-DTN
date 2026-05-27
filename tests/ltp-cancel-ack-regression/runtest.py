"""Script to verify LTP Cancel Segment Acknowledgments without tcpdump.
Uses bespokebpv7 to parse the UDP payload as an LTP segment, generate
LTP payload data, and generate Cancel Segments.

Nate Richard JPL
2026-04-01
"""

import argparse
import socket
import sys
import time

try:
    from bespokebpv7.block_enum import CRCType  # type: ignore[import-untyped]
    from bespokebpv7.bpv7 import BPv7  # type: ignore[import-untyped]
    from bespokebpv7.ltp import LTP  # type: ignore[import-untyped]
    from bespokebpv7.segment_enum import (  # type: ignore[import-untyped]
        CancelReasonCode,
        LTPSegmentType,
    )
    from bespokebpv7.segments import (  # type: ignore[import-untyped]
        CancelSegment,
        DataSegment,
    )
except ImportError:
    print("SKIP: bespokebpv7 module not found. Please install bespokebpv7.")
    sys.exit(2)


def generate_data(dest_port, engine_id, session_id) -> None:
    """Generates a BPv7 bundle, wraps it in an LTP Data Segment,
    and sends it directly to the specified convergence layer UDP port.
    """
    print(
        f"Generating bundle and sending via LTP to port {dest_port} (Engine {engine_id}, Session {session_id})..."
    )

    bundle = BPv7()
    bundle.primary_block.route.source_eid = "ipn:2.1"
    bundle.primary_block.route.dest_eid = "ipn:3.1"
    bundle.primary_block.crc_type = CRCType.CRC16
    bundle.add_payload_block(b"Hello World!")
    bundle.primary_block.update_crc()
    bundle_bytes = bytes(bundle)

    data_seg = DataSegment()
    data_seg.segment_type = LTPSegmentType.DATA_RED_CP_EORP_EOB
    data_seg.session_originator = engine_id
    data_seg.session_number = session_id
    data_seg.client_service_id = 1
    data_seg.client_length = len(bundle_bytes)
    data_seg.data = bundle_bytes

    ltp_packet = LTP()
    ltp_packet.segment = data_seg
    ltp_packet.bpv7 = bundle

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.sendto(bytes(ltp_packet), ("127.0.0.1", dest_port))
    sock.close()
    print("Bundle sent.")


def send_cancel(cancel_type, dest_port, engine_id, session_id) -> None:
    """Generates and sends an LTP Cancel Segment."""
    print(
        f"Injecting {cancel_type} segment for engine {engine_id}, session {session_id} to port {dest_port}..."
    )

    cancel_seg = CancelSegment()
    cancel_seg.segment_type = (
        LTPSegmentType.CANCEL_SENDER
        if cancel_type == "CS"
        else LTPSegmentType.CANCEL_RECV
    )
    cancel_seg.session_originator = engine_id
    cancel_seg.session_number = session_id
    cancel_seg.reason_code = CancelReasonCode.CLIENT_CANCELED

    ltp_packet = LTP()
    ltp_packet.segment = cancel_seg

    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        sock.sendto(bytes(ltp_packet), ("127.0.0.1", dest_port))
    print("Cancel segment sent.")


def run_test(scenario, cancel_type, dest_port, ack_port, engine_id, session_id) -> int:
    """Injects a Cancel Segment and listens for the corresponding ACK.

    Returns:
        0 on success and 1 on failure

    """
    print(f"=== Test Scenario: {scenario} ===")

    expected_type = (
        LTPSegmentType.CANCEL_SENDER_ACK
        if cancel_type == "CS"
        else LTPSegmentType.CANCEL_RECV_ACK
    )

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(("127.0.0.1", ack_port))
    sock.settimeout(1.0)

    send_cancel(cancel_type, dest_port, engine_id, session_id)

    timeout_time = time.time() + 10.0
    while time.time() < timeout_time:
        try:
            data, _ = sock.recvfrom(4096)
            pkt = LTP(data)

            if (
                pkt.segment
                and pkt.segment.segment_type == expected_type
                and pkt.segment.session_number == session_id
            ):
                print(f"SUCCESS: Received {cancel_type} acknowledgment")
                sock.close()
                return 0
        except TimeoutError:
            continue

    print(f"FAILURE: No {cancel_type} acknowledgment found on port {ack_port}")
    sock.close()
    return 1


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="LTP Cancel Acknowledgment Test")
    parser.add_argument(
        "--generate-data",
        action="store_true",
        help="Generate and send data instead of testing",
    )
    parser.add_argument(
        "--send-cancel-only",
        action="store_true",
        help="Send a cancel segment without waiting for ACK",
    )
    parser.add_argument("--scenario", help="Name of the test scenario")
    parser.add_argument(
        "--cancel-type", choices=["CS", "CR"], help="Type of cancel (CS or CR)"
    )
    parser.add_argument("--dest-port", type=int, required=True, help="Destination port")
    parser.add_argument("--ack-port", type=int, help="Port to listen for the ACK")
    parser.add_argument("--engine-id", type=int, required=True, help="Engine ID")
    parser.add_argument("--session-id", type=int, required=True, help="Session ID")

    args = parser.parse_args()

    if args.generate_data:
        generate_data(args.dest_port, args.engine_id, args.session_id)
        sys.exit(0)
    elif args.send_cancel_only:
        send_cancel(args.cancel_type, args.dest_port, args.engine_id, args.session_id)
        sys.exit(0)
    else:
        if not all([args.scenario, args.cancel_type, args.ack_port]):
            print("Error: Missing arguments for test scenario.")
            sys.exit(1)

        sys.exit(
            run_test(
                args.scenario,
                args.cancel_type,
                args.dest_port,
                args.ack_port,
                args.engine_id,
                args.session_id,
            )
        )
