#!/usr/bin/env python3
"""Inject one hex-encoded bundle into a local node's UDP convergence layer.

usage: send_bundle.py <hex-bundle> <port>
"""
import binascii
import socket
import sys


def main() -> None:
    if len(sys.argv) != 3:
        raise SystemExit("usage: send_bundle.py <hex-bundle> <port>")

    try:
        bundle = binascii.unhexlify(sys.argv[1])
    except binascii.Error as exc:
        raise SystemExit("invalid bundle hex: %s" % exc)

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        sock.sendto(bundle, ("127.0.0.1", int(sys.argv[2])))
    finally:
        sock.close()


if __name__ == "__main__":
    main()
