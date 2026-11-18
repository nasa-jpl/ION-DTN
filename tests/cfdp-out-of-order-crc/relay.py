#!/usr/bin/env python3
"""UDP relay between two BP/UDP nodes that delivers the first file data
PDU last: the first forward datagram larger than THRESHOLD is held for
HOLD seconds while later datagrams pass."""
import select
import socket
import sys
import time

THRESHOLD = 700
HOLD = 2.0


def main():
    fwd_in, fwd_out, rev_in, rev_out = (int(x) for x in sys.argv[1:5])
    fwd = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    fwd.bind(("127.0.0.1", fwd_in))
    rev = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    rev.bind(("127.0.0.1", rev_in))
    out = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    held = None
    done = False
    while True:
        ready, _, _ = select.select([fwd, rev], [], [], 0.1)
        for s in ready:
            data, _ = s.recvfrom(65535)
            if s is rev:
                out.sendto(data, ("127.0.0.1", rev_out))
            elif not done and len(data) > THRESHOLD:
                held = (time.time() + HOLD, data)
                done = True
                print("HOLD %d" % len(data), flush=True)
            else:
                out.sendto(data, ("127.0.0.1", fwd_out))
        if held and time.time() >= held[0]:
            out.sendto(held[1], ("127.0.0.1", fwd_out))
            print("RELEASE %d" % len(held[1]), flush=True)
            held = None


main()
