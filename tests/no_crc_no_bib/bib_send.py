#!/usr/bin/env python3
"""Sends bundle without primary block CRC or BIB to check it is rejected."""
import binascii
import socket

INAUTHBUN = "9f88071844008202820301820100820100821b000000b5998c982b011a000493e08506021000458202820200850704010042183485010101004454455354ff"

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.sendto(binascii.unhexlify(INAUTHBUN), ("127.0.0.1", 3113))
