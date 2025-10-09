#!/usr/bin/env python3
"""Sends corrupt bundle as described in CVE-2025-61910 to verify issue is fixed."""

import binascii
import socket

BAD_PAYLOAD = "9f88070000820282020182028201018202820100821b000100bb0e20b4ea001a000927c085070201005bbb0e20b4ea001a000927c085070201005b0086010100014d480086010100014d48656c6c6f2c756433746e2d686f54"

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.sendto(binascii.unhexlify(BAD_PAYLOAD), ("127.0.0.1", 3113))
