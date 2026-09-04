#!/usr/bin/env python3
import binascii, socket, sys
b=binascii.unhexlify(sys.argv[1]); s=socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
s.sendto(b,("127.0.0.1",int(sys.argv[2]))); s.close()
