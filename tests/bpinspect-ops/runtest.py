"""
Automated regression test for bpinspect utility operations using bespokebpv7.

Nate Richard JPL
2026-07-22
"""

import argparse
import socket
import subprocess
import sys
import time
from pathlib import Path

from bespokebpv7.block_enum import BlockType, CRCType
from bespokebpv7.bpv7 import BPv7
from bespokebpv7.ext_functions import BPQExt

MAINDIR = Path.cwd()
NODE2DIR = MAINDIR.joinpath("node2")
ION_INIT_ERRORS = (
    "No node has been initialized in this directory",
    "Failed checking node list parms",
    "BP can't attach to ION",
    "Can't attach to BP",
)


class IonInitializationError(Exception):
    """Custom exception raised when ION/BP initialization errors are detected in stdout."""


class BPInspectTester:
    def __init__(self, induct_port=1113, induct_ip="127.0.0.1"):
        self.induct_addr = (induct_ip, induct_port)
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.passed_tests = 0
        self.failed_tests = 0

        self.bundle_seq_num = 0

    def send_bundle(
        self,
        dest_eid: str,
        payload_str: str,
        src_node: str = "ipn:1.1",
        ttl_sec: int = 7200,
        priority: int | None = None,
    ):
        """Generates a BPv7 bundle and sends it via UDP."""
        bundle = BPv7()
        bundle.primary_block.route.source_eid = src_node
        bundle.primary_block.route.dest_eid = dest_eid
        bundle.primary_block.life.lifetime = ttl_sec * 1000
        bundle.add_payload_block(payload_str.encode("utf-8"))
        bundle.primary_block.crc_type = CRCType.CRC16
        bundle.primary_block.set_creation(seq=self.bundle_seq_num)
        bundle.primary_block.no_fragment = True
        bundle.primary_block.update_crc()

        if priority is not None:
            bundle.blocks[BlockType.QOS] = BPQExt(
                block_number=2, class_of_service=priority
            )

        bundle_bytes = bytes(bundle)
        self.bundle_seq_num += 1

        self.sock.sendto(bundle_bytes, self.induct_addr)

    def run_bpinspect(self, args: list) -> str:
        """Executes bpinspect with the given arguments and returns stdout."""
        cmd = ["bpinspect"] + args
        result = subprocess.run(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            cwd=NODE2DIR,
            text=True,
            check=False,
        )

        try:
            self.check_for_ion_errors(result.stdout, "bpinspect")
        except IonInitializationError as e:
            print(e)
            sys.exit(1)

        return result.stdout

    def wait_for_bundles(
        self, expected_count: int, timeout_sec: int = 10, poll_interval: float = 0.5
    ) -> bool:
        """
        Polls the 'bplist' command line utility to check if the expected
        number of bundles have arrived in the network.

        Args:
            expected_count: The total number of bundles we are waiting for.
            timeout_sec: Maximum time in seconds to wait before giving up.
            poll_interval: How often to poll the bplist command in seconds.

        Returns:
            True if the expected number of bundles are found, False if we timeout.

        """
        start_time = time.time()
        current_count = 0

        while time.time() - start_time < timeout_sec:
            result = subprocess.run(
                ["bplist"],
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                cwd=NODE2DIR,
                text=True,
                check=False,
            )

            try:
                self.check_for_ion_errors(result.stdout, "bplist")
            except IonInitializationError as e:
                print(e)
                sys.exit(1)

            # Based on the bplist output format, we can count the occurrences
            # of "**** Bundle" to reliably determine how many bundles exist.
            current_count = result.stdout.count("**** Bundle")

            if current_count >= expected_count:
                return True

            time.sleep(poll_interval)

        print(
            f"  WARNING: Timeout reached. Expected {expected_count} bundles, but found {current_count}."
        )
        return False

    def check_for_ion_errors(self, output: str, cmd_name: str):
        """Helper to scan merged subprocess output for known ION initialization failures."""
        for error_msg in ION_INIT_ERRORS:
            if error_msg in output:
                raise IonInitializationError(
                    f"ERROR: {cmd_name} failed. ION/BP initialization issue detected:\n"
                    f"  -> '{error_msg}'\n"
                    f"  Make sure the ION node is running and initialized in: {NODE2DIR}"
                )

    def verify_count(self, args: list, search_term: str, expected_count: int) -> bool:
        """Helper to run bpinspect, count occurrences of a term, and verify."""
        output = self.run_bpinspect(args)
        actual_count = output.count(search_term)
        if actual_count == expected_count:
            print(
                f"  OK: {expected_count} bundles queued as expected for '{search_term}'"
            )
            return True
        print(
            f"  FAIL: Expected {expected_count} bundles for '{search_term}', found {actual_count}"
        )
        return False

    def report_result(self, test_name: str, success: bool):
        if success:
            print(f"  PASS: {test_name} successful\n")
            self.passed_tests += 1
        else:
            print(f"  FAIL: {test_name} failed\n")
            self.failed_tests += 1

    def test_1_basic_cancel(self):
        print("Test 1: Basic bundle cancellation")
        print("  Sending 3 bundles to ipn:2.1...")
        self.send_bundle("ipn:2.1", "test data 1")
        self.send_bundle("ipn:2.1", "test data 2")
        self.send_bundle("ipn:2.1", "test data 3")

        if not self.wait_for_bundles(expected_count=3):
            self.report_result("Basic bundle cancellation", False)
            return

        queued_ok = self.verify_count(["-t", "ipn:2.1", "-l"], "ipn:2.1", 3)
        print("  Canceling bundles...")
        self.run_bpinspect(["-t", "ipn:2.1", "-c", "-n"])
        time.sleep(1)
        canceled_ok = self.verify_count(["-t", "ipn:2.1", "-l"], "ipn:2.1", 0)

        self.report_result("Basic bundle cancellation", queued_ok and canceled_ok)

    def test_2_filter_cancel(self):
        print("Test 2: Filter + cancel (destination filter)")
        print("  Sending bundles to different destinations (ipn:2.2 and ipn:2.3)...")
        self.send_bundle("ipn:2.2", "to endpoint 2.2")
        self.send_bundle("ipn:2.3", "to endpoint 2.3")

        if not self.wait_for_bundles(expected_count=2):
            self.report_result("Filtered cancel", False)
            return

        ok_queued = self.verify_count(["-l"], "ipn:2.", 2)
        print("  Canceling only bundles to ipn:2.2...")
        self.run_bpinspect(["-t", "ipn:2.2", "-c", "-n"])
        time.sleep(1)

        ok_22 = self.verify_count(["-l"], "ipn:2.2", 0)
        ok_23 = self.verify_count(["-l"], "ipn:2.3", 1)

        self.run_bpinspect(["-t", "ipn:2.3", "-c", "-n"])
        self.report_result("Filtered cancel", ok_queued and ok_22 and ok_23)

    def test_3_dry_run(self):
        print("Test 3: Dry-run mode (no actual cancel)")
        print("  Sending bundle to ipn:2.3...")
        self.send_bundle("ipn:2.3", "dry-run test")

        if not self.wait_for_bundles(expected_count=1):
            self.report_result("Dry-run mode", False)
            return

        queued_ok = self.verify_count(["-t", "ipn:2.3", "-l"], "ipn:2.3", 1)

        print("  Running dry-run cancel (-D flag)...")
        output = self.run_bpinspect(["-t", "ipn:2.3", "-D", "-c", "-n"])

        dry_run_worked = "Would cancel" in output or "Dry-run" in output
        still_queued_ok = self.verify_count(["-t", "ipn:2.3", "-l"], "ipn:2.3", 1)

        self.run_bpinspect(["-t", "ipn:2.3", "-c", "-n"])
        self.report_result(
            "Dry-run mode", queued_ok and dry_run_worked and still_queued_ok
        )

    def test_4_multiple_cancel(self):
        print("Test 4: Multiple bundle cancel")
        print("  Sending multiple bundles to ipn:2.1...")
        self.send_bundle("ipn:2.1", "bundle 1")
        self.send_bundle("ipn:2.1", "bundle 2")

        if not self.wait_for_bundles(expected_count=2):
            self.report_result("Multiple bundle cancel", False)
            return

        queued_ok = self.verify_count(["-t", "ipn:2.1", "-l"], "ipn:2.1", 2)
        print("  Canceling all bundles to ipn:2.1...")
        self.run_bpinspect(["-t", "ipn:2.1", "-c", "-n"])
        time.sleep(1)
        canceled_ok = self.verify_count(["-t", "ipn:2.1", "-l"], "ipn:2.1", 0)

        self.report_result("Multiple bundle cancel", queued_ok and canceled_ok)

    def test_5_priority_filter_cancel(self):
        print("Test 5: Priority filter + cancel")
        print("  Sending bulk (pri=0) and standard (pri=1) bundles to ipn:1.2...")
        self.send_bundle("ipn:1.2", "bulk priority", src_node="dtn:none", priority=0)
        self.send_bundle(
            "ipn:1.2", "standard priority", src_node="dtn:none", priority=1
        )

        if not self.wait_for_bundles(expected_count=2):
            self.report_result("Priority filter cancel", False)
            return

        ok_queued = self.verify_count(["-t", "ipn:1.2", "-l"], "ipn:1.2", 2)

        print("  Canceling only bulk priority bundles (-p 0)...")
        self.run_bpinspect(["-t", "ipn:1.2", "-p", "0", "-c", "-n"])
        time.sleep(1)

        ok_remaining = self.verify_count(["-t", "ipn:1.2", "-l"], "ipn:1.2", 1)

        self.run_bpinspect(["-t", "ipn:1.2", "-c", "-n"])
        self.report_result("Priority filter cancel", ok_queued and ok_remaining)

    def test_6_suspend(self):
        print("Test 6: Basic bundle suspension")
        print("  Sending 2 bundles to ipn:1.4...")
        self.send_bundle("ipn:1.4", "suspend 1", src_node="dtn:none")
        self.send_bundle("ipn:1.4", "suspend 2")

        if not self.wait_for_bundles(expected_count=2):
            self.report_result("Basic bundle suspension", False)
            return

        queued_ok = self.verify_count(["-t", "ipn:1.4", "-l"], "ipn:1.4", 2)

        print("  Suspending bundles...")
        self.run_bpinspect(["-t", "ipn:1.4", "-u", "-n"])
        time.sleep(1)

        suspended_ok = self.verify_count(
            ["-q", "limbo", "-t", "ipn:1.4", "-l"], "ipn:1.4", 2
        )

        self.report_result("Basic bundle suspension", queued_ok and suspended_ok)

    def test_7_resume(self):
        print("Test 7: Resume suspended bundles")
        print("  Resuming bundles from limbo...")
        self.run_bpinspect(["-q", "limbo", "-t", "ipn:1.4", "-R", "-n"])
        time.sleep(1)

        resumed_ok = self.verify_count(
            ["-q", "limbo", "-t", "ipn:1.4", "-l"], "ipn:1.4", 0
        )
        queue_ok = self.verify_count(["-t", "ipn:1.4", "-l"], "ipn:1.4", 2)

        self.run_bpinspect(["-t", "ipn:1.4", "-c", "-n"])
        self.report_result("Resume suspended bundles", resumed_ok and queue_ok)

    def test_8_filter_suspend(self):
        print("Test 8: Filter + suspend (destination filter)")
        print("  Sending bundles to different destinations (ipn:1.5 and ipn:1.6)...")
        self.send_bundle("ipn:1.5", "to 1.5", src_node="dtn:none")
        self.send_bundle("ipn:1.6", "to 1.6", src_node="dtn:none")

        if not self.wait_for_bundles(expected_count=2):
            self.report_result("Filter suspend", False)
            return

        print("  Suspending only bundles to ipn:1.5...")
        self.run_bpinspect(["-t", "ipn:1.5", "-u", "-n"])
        time.sleep(1)

        limbo_15_ok = self.verify_count(
            ["-q", "limbo", "-t", "ipn:1.5", "-l"], "ipn:1.5", 1
        )
        limbo_16_ok = self.verify_count(
            ["-q", "limbo", "-t", "ipn:1.6", "-l"], "ipn:1.6", 0
        )

        self.run_bpinspect(["-q", "limbo", "-t", "ipn:1.5", "-c", "-n"])
        self.run_bpinspect(["-t", "ipn:1.6", "-c", "-n"])
        self.report_result("Filter suspend", limbo_15_ok and limbo_16_ok)

    def test_9_dry_run_suspend_resume(self):
        print("Test 9: Dry-run mode (suspend and resume)")
        print("  Sending bundle to ipn:1.7...")
        self.send_bundle("ipn:1.7", "dry-run suspend test", src_node="dtn:none")

        if not self.wait_for_bundles(expected_count=1):
            self.report_result("Dry-run suspend/resume", False)
            return

        print("  Running dry-run suspend (-D flag)...")
        self.run_bpinspect(["-t", "ipn:1.7", "-D", "-u", "-n"])
        time.sleep(1)

        limbo_empty = self.verify_count(
            ["-q", "limbo", "-t", "ipn:1.7", "-l"], "ipn:1.7", 0
        )

        # Actual suspend for the resume test
        self.run_bpinspect(["-t", "ipn:1.7", "-u", "-n"])
        time.sleep(1)

        print("  Running dry-run resume (-D flag)...")
        self.run_bpinspect(["-q", "limbo", "-t", "ipn:1.7", "-D", "-R", "-n"])
        time.sleep(1)

        limbo_full = self.verify_count(
            ["-q", "limbo", "-t", "ipn:1.7", "-l"], "ipn:1.7", 1
        )

        self.run_bpinspect(["-q", "limbo", "-t", "ipn:1.7", "-c", "-n"])
        self.report_result("Dry-run suspend/resume", limbo_empty and limbo_full)

    def run_all(self):
        self.test_1_basic_cancel()
        self.test_2_filter_cancel()
        self.test_3_dry_run()
        self.test_4_multiple_cancel()
        self.test_5_priority_filter_cancel()
        self.test_6_suspend()
        self.test_7_resume()
        self.test_8_filter_suspend()
        self.test_9_dry_run_suspend_resume()

        print("=========================================")
        if self.failed_tests == 0:
            print("PASS: All tests passed")
            sys.exit(0)
        else:
            print(f"FAIL: {self.failed_tests} tests failed")
            sys.exit(1)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="bpinspect Operations Test via bespokebpv7"
    )
    parser.add_argument(
        "--induct-port", type=int, default=1113, help="ION Node 1 UDP Induct Port"
    )
    args = parser.parse_args()

    tester = BPInspectTester(induct_port=args.induct_port)
    tester.run_all()
