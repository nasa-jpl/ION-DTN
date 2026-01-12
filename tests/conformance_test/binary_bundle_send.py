#!/usr/bin/env python3
"""
Sends string hexadecimal representations of full bundle(s) to a local node
running the UDPCLA. Accepts them either passed via the command line or a file.

Nate Richard 2025/12/19 JPL
"""
import argparse
import binascii
import socket
import sys
import time


def bundle_send(str_bundle: str, port: int = 3113) -> None:
    """
    Sends a string representation of a bundle to a local node.

    :param binary_bundle: Hex string representation of a bundle
    :type binary_bundle: str
    """
    try:
        binary_bundle = binascii.unhexlify(str_bundle)
    except binascii.Error:
        print("Invalid bundle format, skipping")
        return

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.sendto(binary_bundle, ("127.0.0.1", port))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Sends bundle(s) that are as hexadecimal string(s) via command line or a file."
    )
    parser.add_argument(
        "-p", "--port", help="UDP port to send bundle, default: 3113", default=3113
    )
    parser.add_argument(
        "-b",
        "--bundle",
        help="Hexadecimal string representation of a full bundle",
        action="append",
    )
    parser.add_argument(
        "-f",
        "--file",
        help="Path to file with a hexadecimal string representation of a full bundle on each line.",
    )
    args = parser.parse_args()

    if args.file:
        with open(args.file, "r", encoding="utf-8") as hfile:
            init_bundle_list = hfile.readlines()
        # Make sure there are no newlines
        bundle_list = [item.strip() for item in init_bundle_list if "#" not in item]
    elif args.bundle:
        bundle_list = args.bundle
    else:
        print("No bundles, can't send anything.")
        sys.exit(1)

    for bundle in bundle_list:
        bundle_send(bundle, args.port)
        time.sleep(1)
