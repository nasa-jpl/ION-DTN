"""------------------------------------
     JET PROPULSION LABORATORY
------------------------------------
         ___  _______  ___
        |   ||       ||   |
        |   ||    _  ||   |
        |   ||   |_| ||   |
     ___|   ||    ___||   |___
    |       ||   |    |       |
    |_______||___|    |_______|

------------------------------------
 CALIFORNIA INSTITUTE OF TECHNOLOGY
------------------------------------

*****************************************************************************
 Title: Status Report Test
 Author: Nate Richard
 Modified: 01/29/2026
 Company: JPL
 Date:   01/29/2026

 File: runtest
 Description:
           Script to verify status reports are generated
           Python 3.12.11

Copyright 2025, by the California Institute of Technology. United States
Government sponsorship acknowledged. Any rights or license to commercial use
must be negotiated with the Office of Technology Transfer at the California
Institute of Technology.

This software may be subject to U.S. export control laws and regulations. By
accepting this software, the user agrees to comply with all applicable U.S.
export laws and regulations. The user has the responsibility to obtain export
licenses, or other export authority as may be required before exporting the
software to foreign countries or providing access to foreign persons.
*****************************************************************************
"""

import argparse
import json
import socket
import sys
import threading
import time
from pathlib import Path
from queue import Empty, Queue

try:
    from bespokebpv7.admin_records import (
        BundleStatusReport,  # type: ignore[import-untyped]
    )
    from bespokebpv7.block_enum import (  # type: ignore[import-untyped]
        BlockType,
        CRCType,
    )
    from bespokebpv7.bpv7 import BPv7  # type: ignore[import-untyped]
except ImportError:
    print("SKIP: bespokebpv7 module not found. Please install bespokebpv7.")
    sys.exit(2)

report_queue: Queue[bytes] = Queue()


def packet_receiver(
    listen_addr: tuple[str, int], stop_event: threading.Event, active: threading.Event
) -> None:
    """
    Background thread to catch all incoming UDP bundles and put
    valid Status Reports into the queue.

    Args:
        listen_addr: UDP address to tuple listen for reports
        stop_event: threading Event to stop thread once finished
        active: Event to signal main thread that receiver is ready

    """
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
    sock.bind(listen_addr)
    sock.settimeout(1)

    print(f"[*] Receiver thread started on {listen_addr}")
    active.set()
    while not stop_event.is_set():
        try:
            data = sock.recv(4096)
        except (TimeoutError, ValueError):
            continue
        except OSError as e:
            print(f"[!] Receiver error: {e}")
            break

        report_queue.put(data)

    sock.close()
    print("[*] Receiver thread shut down.")


def create_test_bundle(
    dest_eid: str, requested_reports: list[str] | None = None
) -> BPv7:
    """Configure a BPv7 bundle with requested status report flags.

    Args:
        dest_eid: Destination for bundles
        requested_reports: list of requested status flags

    Returns:
        Bundle with requested status report flags set

    """
    bundle = BPv7()
    bundle.primary_block.route.source_eid = "ipn:2.1"
    bundle.primary_block.route.dest_eid = dest_eid
    bundle.primary_block.route.report_to = "ipn:5.0"
    bundle.primary_block.status_time = True
    bundle.primary_block.crc_type = CRCType.CRC16
    bundle.primary_block.life.lifetime = 8 * 1000  # time in ms

    # Map string args to BPv7 Primary Block flag properties
    report_map = {
        "rcv": "recv_report",
        "fwd": "fwd_report",
        "dlv": "deliv_report",
        "del": "del_report",
    }

    if requested_reports:
        for report in requested_reports:
            # Familarizing myself with walrus operator
            # Combines if not None with default value of None
            if attr := report_map.get(report):
                setattr(bundle.primary_block, attr, True)

    bundle.add_payload_block(b"Simplified multi-threaded status report test.")
    bundle.primary_block.set_creation()
    bundle.primary_block.update_crc()
    return bundle


def check_status(data: bytes) -> BundleStatusReport | None:
    """Parse bundles for status reports.

    Args:
        data: potential bundle as bytes

    Returns:
        Status report if found

    """
    bundle = BPv7(data)
    if bundle.primary_block.adu_is_admin:
        payload = bundle.get_block_by_type(BlockType.PAYLOAD_BLOCK)
        if isinstance(payload, BundleStatusReport):
            return payload
    return None


def collect_reports(requested_count: int) -> set[str]:
    """Poll the queue and extracts verified status types from incoming reports.

    Args:
        requested_count: number of status report flags to expect

    Returns:
        Set of received status reports

    """
    timeout = time.time() + 30
    received_types: set[str] = set()

    while len(received_types) < requested_count and time.time() < timeout:
        try:
            data = report_queue.get(timeout=1.0)
            print("Got status report, processing...")
        except Empty:
            continue

        if report_payload := check_status(data):
            status = report_payload.base_status.status_info

            if status.recv_bundle.status_indicator:
                received_types.add("rcv")
            if status.fwd_bundle.status_indicator:
                received_types.add("fwd")
            if status.deliv_bundle.status_indicator:
                received_types.add("dlv")
            if status.del_bundle.status_indicator:
                received_types.add("del")

            print(f"[+] Found report! Progress: {received_types}")
    return received_types


def verify_network_aliveness(node_addr: tuple[str, int], dest_eid: str) -> bool:
    """Send a bundle and waits for a response from bpecho.

    Returns:
        success or failure of network aliveness

    """
    print(f"[*] Verifying network aliveness via {dest_eid}...")
    ping = create_test_bundle(dest_eid)

    timeout = time.time() + 5.0
    while time.time() < timeout:
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
            s.sendto(bytes(ping), node_addr)
        try:
            data = report_queue.get(timeout=1.0)
        except Empty:
            continue
        bundle = BPv7(data)
        # Check if we got our payload back
        payload = bundle.get_block_by_type(BlockType.PAYLOAD_BLOCK)
        if payload and b"Simplified multi-threaded status report test." in payload.data:
            print("[+] Network alive! Bundle received.")
            return True
    return False


def run_status_report_test(
    parameter_dict: dict[str, list[str]], rport: int = 5115, sport: int = 3113
) -> int:
    """Run the status report test.

    Args:
        parameter_dict: Dictionary of status flag parameters to test
        rport: Port to receive status reports
        sport: Port to send bundles

    Returns:
        0 on success and 1 on failure

    """
    node3_addr = ("127.0.0.1", sport)
    listen_addr = ("127.0.0.1", rport)
    stop_event = threading.Event()
    active = threading.Event()

    receiver_thread = threading.Thread(
        target=packet_receiver, args=(listen_addr, stop_event, active)
    )
    receiver_thread.start()

    # Wait until receiver thread is ready
    active.wait()

    if not verify_network_aliveness(node3_addr, "ipn:5.1"):
        print("FAILURE: Network check failed. Is network up?")
        stop_event.set()
        receiver_thread.join()
        return 1

    successes = []
    print("Setup complete, sending bundles and waiting for reports...")
    for dnode, reports in parameter_dict.items():
        bundle = create_test_bundle(dnode, reports)
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as send_sock:
            send_sock.sendto(bytes(bundle), node3_addr)
        print(f"[+] Sent bundle. Waiting for: {reports}")

        try:
            received = collect_reports(len(reports))
        except KeyboardInterrupt:
            print("Caught keyboard interrupt.")
            stop_event.set()
            receiver_thread.join()
            return 1
        successes.append(set(reports).issubset(received))

    if all(successes):
        print("SUCCESS: All requested reports verified.")
        stop_event.set()
        receiver_thread.join()
        return 0

    print("FAILURE: Missing reports")
    stop_event.set()
    receiver_thread.join()
    return 1


if __name__ == "__main__":
    print("\nTest parameter setup.")
    parser = argparse.ArgumentParser(description="bespokebpv7 Status Report Tester.")
    parser.add_argument(
        "--parm-dict",
        "-p",
        type=str,
        required=True,
        help="JSON file defining destination EID and their status report flags.",
    )
    parser.add_argument(
        "--recv-port",
        "-r",
        type=int,
        help="Port to receive bundle status reports.",
    )
    parser.add_argument(
        "--src-port",
        "-s",
        type=int,
        help="Port to send bundle to, generating status reports.",
    )

    args = parser.parse_args()

    print("\nReading parameter file...")
    with Path(args.parm_dict).open(encoding="utf-8") as file:
        parm_dict = json.load(file)
    print("\nSetting up test..")
    sys.exit(run_status_report_test(parm_dict, args.recv_port, args.src_port))
