#!/usr/bin/env python3
"""Regression test for issue 1174: hop-limit exceedance reporting.

Uses bespokebpv7 to build a bundle that already carries a Hop Count
extension block whose hop count equals its hop limit, and injects it
into the ION node's UDP induct.  When ION forwards the bundle, the
dequeue-time hop count increment exceeds the limit, so ION must delete
the bundle and (because the bundle requests deletion reporting) send a
deletion status report with reason code 9 (hop limit exceeded) to the
report-to endpoint.

The report-to endpoint is ipn:5.2; the ION node's egress plan for node
5 is a udp duct pointed at this script's listener socket, so the test
receives and decodes the actual status report.  The bundle itself is
destined for ipn:2.1, whose egress plan points at a second listener
socket; if the bundle arrives there, hop-limit enforcement failed.

Exit codes: 0 = pass, 1 = fail, 2 = skip (bespokebpv7 not installed).
"""

import socket
import sys
import threading
import time
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
    from bespokebpv7.utils import (
        bundle_converter,  # type: ignore[import-untyped]
    )
except ImportError:
    print("SKIP: bespokebpv7 module not found. Please install bespokebpv7.")
    sys.exit(2)

NODE_ADDR = ("127.0.0.1", 3213)     # ION node's udp induct
REPORT_ADDR = ("127.0.0.1", 5215)   # egress duct for node 5 (report-to)
FORWARD_ADDR = ("127.0.0.1", 8200)  # egress duct for node 2 (bundle dest)

HOP_LIMIT_EXCEEDED = 9

report_queue: Queue[bytes] = Queue()
forwarded_queue: Queue[bytes] = Queue()


def receiver(
    addr: tuple[str, int],
    queue: Queue[bytes],
    stop_event: threading.Event,
    active: threading.Event,
) -> None:
    """Catch UDP datagrams emitted by the ION node on one egress duct."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(addr)
    sock.settimeout(1)
    print(f"[*] Receiver started on {addr}")
    active.set()
    while not stop_event.is_set():
        try:
            queue.put(sock.recv(4096))
        except (TimeoutError, ValueError):
            continue
        except OSError as e:
            print(f"[!] Receiver error: {e}")
            break
    sock.close()


def build_bundle(
    dest_eid: str, hop_limit: int | None = None, hop_count: int = 0
) -> BPv7:
    """Build a bundle, optionally carrying a Hop Count extension block."""
    bundle = BPv7()
    bundle.primary_block.route.source_eid = "ipn:7.1"
    bundle.primary_block.route.dest_eid = dest_eid
    bundle.primary_block.route.report_to = "ipn:5.2"
    bundle.primary_block.crc_type = CRCType.CRC16
    bundle.primary_block.life.lifetime = 30 * 1000  # time in ms
    bundle.primary_block.status_time = True
    bundle.primary_block.del_report = True
    if hop_limit is not None:
        bundle.add_canonical_block(
            {"block_type": BlockType.HOP_COUNT},
            bundle_converter.dumps([hop_limit, hop_count]),
        )
    bundle.add_payload_block(b"issue-1174 hop limit test bundle")
    bundle.primary_block.set_creation()
    bundle.primary_block.update_crc()
    return bundle


def inject(bundle: BPv7) -> None:
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        sock.sendto(bytes(bundle), NODE_ADDR)


def verify_network_aliveness() -> bool:
    """Send a plain bundle destined for node 5 and wait for it to be
    forwarded to the report listener, proving the node is up."""
    print("[*] Verifying network aliveness via ipn:5.1...")
    timeout = time.time() + 20
    while time.time() < timeout:
        inject(build_bundle("ipn:5.1"))
        try:
            data = report_queue.get(timeout=1.0)
        except Empty:
            continue
        bundle = BPv7(data)
        payload = bundle.get_block_by_type(BlockType.PAYLOAD_BLOCK)
        if payload and b"issue-1174 hop limit test" in payload.data:
            print("[+] Network alive! Bundle forwarded.")
            return True
    return False


def wait_for_deletion_report() -> bool:
    """Wait for a deletion status report with reason code 9."""
    timeout = time.time() + 30
    while time.time() < timeout:
        try:
            data = report_queue.get(timeout=1.0)
        except Empty:
            continue

        bundle = BPv7(data)
        if not bundle.primary_block.adu_is_admin:
            continue
        payload = bundle.get_block_by_type(BlockType.PAYLOAD_BLOCK)
        if not isinstance(payload, BundleStatusReport):
            continue

        status = payload.base_status
        deleted = status.status_info.del_bundle.status_indicator
        reason = int(status.reason_code)
        print(f"[+] Status report: deleted={deleted} reason={reason}")
        if deleted and reason == HOP_LIMIT_EXCEEDED:
            return True
        print("[!] Report did not carry deleted status with reason 9.")
        return False
    print("[!] Timed out waiting for a status report.")
    return False


def main() -> int:
    stop_event = threading.Event()
    threads = []
    for addr, queue in ((REPORT_ADDR, report_queue),
                        (FORWARD_ADDR, forwarded_queue)):
        active = threading.Event()
        t = threading.Thread(
            target=receiver, args=(addr, queue, stop_event, active)
        )
        t.start()
        threads.append(t)
        active.wait()

    result = 1
    try:
        if not verify_network_aliveness():
            print("FAILURE: Network check failed. Is the node up?")
            return 1

        # Hop count already at the limit: the forwarding dequeue must
        # push it over and delete the bundle with reason code 9.
        print("[*] Injecting bundle with hop count at its hop limit...")
        inject(build_bundle("ipn:2.1", hop_limit=15, hop_count=15))

        if not wait_for_deletion_report():
            print("FAILURE: no 'hop limit exceeded' deletion report.")
            return 1

        # The bundle itself must NOT have been forwarded onward.
        try:
            forwarded_queue.get(timeout=2.0)
            print("FAILURE: bundle was forwarded despite exceeding its "
                  "hop limit.")
            return 1
        except Empty:
            pass

        print("SUCCESS: bundle deleted with 'hop limit exceeded' report.")
        result = 0
    finally:
        stop_event.set()
        for t in threads:
            t.join()
    return result


if __name__ == "__main__":
    sys.exit(main())
