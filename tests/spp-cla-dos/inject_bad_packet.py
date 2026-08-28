#!/usr/bin/env python3
#
# Inject a single malformed SPP loopback frame into the provider's FIFO.
#
# The spp_loopback stub provider frames each packet as a 4-byte big-endian
# total-length header followed by that many bytes.  packet_indication()
# reads the length header and, if it exceeds MAX_PACKET_SIZE (65536),
# returns 0 ("error" in the CLA provider contract) WITHOUT reading any
# data -- so a lone oversized length header is a complete malformed frame
# that leaves the FIFO byte-aligned for the next real packet.
#
# Before the fix, sppcli mapped that 0 return onto ionKillMainThread(),
# so this single frame from any writer killed the induct.
#
import os
import sys

fifo = sys.argv[1] if len(sys.argv) > 1 else "/tmp/spp_loopback_fifo"

if not os.path.exists(fifo):
    sys.stderr.write("inject_bad_packet: FIFO %s does not exist\n" % fifo)
    sys.exit(1)

# 0x00080000 = 524288 bytes, comfortably above MAX_PACKET_SIZE (65536).
frame = bytes([0x00, 0x08, 0x00, 0x00])

# Open O_RDWR so the open never blocks even if the reader is momentarily
# not waiting; the stub opens its own ends O_RDWR for the same reason.
fd = os.open(fifo, os.O_RDWR)
try:
    os.write(fd, frame)
finally:
    os.close(fd)

sys.stderr.write("inject_bad_packet: wrote oversized SPP frame header "
                 "(declared length 524288)\n")
