#!/usr/bin/env python3
#
# send_datagram.py HOST PORT NBYTES
#
# Send a single UDP datagram of exactly NBYTES bytes (each byte 0x01) to
# HOST:PORT, from an ordinary ephemeral source port.  Used by the
# udpcli-dos regression test to reproduce the two attack vectors of
# GHSA-5qr2-pgwp-vf8r:
#
#   NBYTES == 0 -> empty datagram (old code: bundleLength == 0 -> kill)
#   NBYTES == 1 -> one stray 0x01 byte (old code: bundleLength == 1 -> stop)
#
# The source port is an ephemeral port, never the induct's own port, so
# isUdpClaShutdownSignal() correctly rejects it as a non-shutdown packet;
# the loopback-verified shutdown path is therefore not exercised here.
#
# Note: a genuine 0-byte UDP datagram cannot be produced by most shells
# (bash's /dev/udp and many nc builds suppress it), which is why this
# test relies on Python's sendto().

import socket
import sys


def main():
    if len(sys.argv) != 4:
        sys.stderr.write("usage: send_datagram.py HOST PORT NBYTES\n")
        return 1

    host = sys.argv[1]
    port = int(sys.argv[2])
    nbytes = int(sys.argv[3])

    payload = b"\x01" * nbytes

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        sent = sock.sendto(payload, (host, port))
        # For a 0-byte payload sendto() returns 0, which is success here.
        if sent != nbytes:
            sys.stderr.write(
                "short send: asked %d, sent %d\n" % (nbytes, sent))
            return 1
    finally:
        sock.close()

    return 0


if __name__ == "__main__":
    sys.exit(main())
