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
 Title: bpdriver TTL Test
 Author: Nate Richard
 Modified: 01/29/2026
 Company: JPL
 Date:   01/29/2026

 File: runtest
 Description:
           Script to verify TTL option in bpdriver bundles are set to the
           expected value
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
import shlex
import socket
import subprocess
import sys
from pathlib import Path

try:
    from bespokebpv7.bpv7 import BPv7  # type: ignore[import-untyped]
except ImportError:
    print("SKIP: bespokebpv7 module not found. Please install bespokebpv7.")
    sys.exit(2)

MAINDIR = Path.cwd()
NODE1DIR = MAINDIR.joinpath("1.ipn.bp.udp")
DEFAULTTTL = 300


def call_bpdriver(ttl_sec: int | None = None) -> bool:
    """
    Execute the bpdriver command to send a single bundle.
    Waits for completion before returning.

    Args:
        ttl_sec: bpdriver TTL argument

    Returns:
        Success or failure of running bpdriver

    """
    cmd = "bpdriver 1 ipn:1.1 ipn:1.2 -100"
    if ttl_sec is not None:
        cmd += f" t{ttl_sec}"

    try:
        subprocess.run(
            shlex.split(cmd),
            cwd=NODE1DIR,
            check=True,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.STDOUT,
        )
    except subprocess.CalledProcessError as e:
        print(f"ERROR: bpdriver failed with exit code {e.returncode}")
        return False
    except FileNotFoundError:
        print("ERROR: bpdriver command not found. Is ION installed and in your PATH?")
        return False
    return True


def run_test(expected_ttl_sec: int, listen_port: int = 4556) -> int:
    """Send bpdriver bundle and verify TTL.

    Args:
        expected_ttl_sec: Expected TTL
        listen_port: UDP Port to receive bundles

    Returns:
        0 for success and 1 for failure

    """
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(("127.0.0.1", listen_port))
    sock.settimeout(5.0)

    bpdriver_ttl = None
    if expected_ttl_sec != DEFAULTTTL:
        bpdriver_ttl = expected_ttl_sec

    if not call_bpdriver(bpdriver_ttl):
        return 1

    try:
        data, _ = sock.recvfrom(4096)

    except TimeoutError:
        print("FAILURE: Timed out waiting for bundle.")
        return 1
    finally:
        sock.close()

    bundle = BPv7(data)

    actual_ttl_ms = bundle.primary_block.life.lifetime
    expected_ttl_ms = expected_ttl_sec * 1000

    print(f"Result -> Expected: {expected_ttl_ms}ms | Actual: {actual_ttl_ms}ms")

    if actual_ttl_ms == expected_ttl_ms:
        print("SUCCESS: TTL verified.")
        return 0
    print("FAILURE: TTL values do not match.")
    return 1


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Verify bpdriver TTL option using bespokebpv7 parser."
    )
    parser.add_argument(
        "ttl", type=int, help="The expected Time To Live (TTL) in seconds."
    )
    parser.add_argument("--port", "-p", type=int, help="Bundle receive port.")

    args = parser.parse_args()

    sys.exit(run_test(args.ttl, args.port))
