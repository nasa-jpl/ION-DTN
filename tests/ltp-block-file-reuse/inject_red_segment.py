#!/usr/bin/env python3
#
# inject_red_segment.py HOST PORT ENGINE SESSION OFFSET LENGTH FILL
#
# Send one LTP red data segment to the udplsi UDP port at HOST:PORT.  The
# segment claims to come from session SESSION of engine ENGINE and carries
# LENGTH bytes of the character FILL at block offset OFFSET.  It is not a
# checkpoint, so the receiver sends no report in response.
#
# LTP data-segment wire format (RFC 5326), all multi-byte fields SDNV:
#   control byte = (version << 4) | segment type code   (version 0, red = 0x00)
#   session originator (engine ID) SDNV
#   session number SDNV
#   extension counts byte = (header count << 4) | trailer count
#   client service ID SDNV   (1 = BpLtpClientId, registered by ltpcli)
#   offset SDNV
#   length SDNV
#   ... data ...

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
    if len(sys.argv) != 8:
        sys.stderr.write("usage: inject_red_segment.py HOST PORT ENGINE "
                         "SESSION OFFSET LENGTH FILL\n")
        return 1

    host = sys.argv[1]
    port = int(sys.argv[2])
    engine, session, offset, length = (int(a) for a in sys.argv[3:7])
    fill = sys.argv[7].encode("ascii")[:1]

    segment = (
        bytes([0x00])          # control: version 0, red data, not a checkpoint
        + sdnv(engine)         # session originator engine ID
        + sdnv(session)        # session number
        + bytes([0x00])        # extension counts: none
        + sdnv(1)              # client service ID = 1 (BpLtpClientId)
        + sdnv(offset)
        + sdnv(length)
        + fill * length
    )

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        sock.sendto(segment, (host, port))
    finally:
        sock.close()

    return 0


if __name__ == "__main__":
    sys.exit(main())
