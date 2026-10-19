'''
bridge_ctrl.py: Pure Python control channel for ION TCP4 CLA.

Implements the TLV-framed control channel protocol over Unix domain
sockets.  All multi-byte fields use network byte order (big-endian).

Frame format (14-byte header + 0-256 payload):
    Type(1) | Reserved(1) | SlotIndex(2) | TransferID(8) | PayloadLen(2) | Payload

Author: iondev33

Copyright (c) 2026, California Institute of Technology.
ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
'''

import logging
import socket
import struct
import time
from dataclasses import dataclass, field

LOGGER = logging.getLogger(__name__)

# Control message types (must match ici/include/ion_ctrl_channel.h)
CTRL_SLOT_READY = 0x01
CTRL_BUNDLE_RECV = 0x02
CTRL_XFER_SUCCESS = 0x03
CTRL_XFER_FAILURE = 0x04
CTRL_XFER_REFUSED = 0x05
CTRL_SLOT_RELEASE = 0x06
CTRL_SESSION_UP = 0x10
CTRL_SESSION_DOWN = 0x11
CTRL_CMD_CONNECT = 0x20
CTRL_CMD_LISTEN = 0x21
CTRL_KEEPALIVE = 0xFF

CTRL_HDR_SIZE = 14
CTRL_MAX_PAYLOAD = 256

# Network byte order: Type(B) Reserved(B) SlotIndex(H) TransferID(Q) PayloadLen(H)
_HDR_FMT = '>BBHQH'

# Human-readable names for logging
_MSG_NAMES = {
    CTRL_SLOT_READY: 'SLOT_READY',
    CTRL_BUNDLE_RECV: 'BUNDLE_RECV',
    CTRL_XFER_SUCCESS: 'XFER_SUCCESS',
    CTRL_XFER_FAILURE: 'XFER_FAILURE',
    CTRL_XFER_REFUSED: 'XFER_REFUSED',
    CTRL_SLOT_RELEASE: 'SLOT_RELEASE',
    CTRL_SESSION_UP: 'SESSION_UP',
    CTRL_SESSION_DOWN: 'SESSION_DOWN',
    CTRL_CMD_CONNECT: 'CMD_CONNECT',
    CTRL_CMD_LISTEN: 'CMD_LISTEN',
    CTRL_KEEPALIVE: 'KEEPALIVE',
}


@dataclass
class CtrlMsg:
    '''Parsed control message in host byte order.'''
    type: int
    slot_index: int = 0
    xfer_id: int = 0
    payload: bytes = field(default_factory=bytes)

    def __repr__(self):
        name = _MSG_NAMES.get(self.type, f'0x{self.type:02x}')
        return (f'CtrlMsg({name}, slot={self.slot_index}, '
                f'xfer={self.xfer_id}, plen={len(self.payload)})')


def _recv_exactly(sock, n):
    '''Read exactly n bytes from a socket.

    :raises ConnectionError: If the peer disconnects.
    '''
    buf = bytearray()
    while len(buf) < n:
        chunk = sock.recv(n - len(buf))
        if not chunk:
            raise ConnectionError('Control channel disconnected')
        buf.extend(chunk)
    return bytes(buf)


class CtrlChannel:
    '''Unix domain socket control channel client.

    Connects to the server socket created by tcp4clo or tcp4cli.
    '''

    def __init__(self, path, max_retries=30):
        '''Connect to a control channel socket.

        :param path: Unix socket path (e.g., "/tmp/ion_tcp4/tx_loopback.sock")
        :param max_retries: Number of connection attempts (1s between retries)
        :raises RuntimeError: If connection fails after max_retries
        '''
        self._path = path
        self._sock = None
        self.last_recv_time = time.time()
        self.last_send_time = time.time()

        for attempt in range(max_retries):
            try:
                sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
                sock.connect(path)
                self._sock = sock
                LOGGER.info('Connected to ctrl channel %s', path)
                return
            except (ConnectionRefusedError, FileNotFoundError):
                if attempt < max_retries - 1:
                    LOGGER.debug('Ctrl %s not ready, retry %d/%d',
                                 path, attempt + 1, max_retries)
                    time.sleep(1)

        raise RuntimeError(f'Cannot connect to ctrl channel {path} '
                           f'after {max_retries} retries')

    def fileno(self):
        '''Return the socket file descriptor (for GLib.io_add_watch).'''
        return self._sock.fileno()

    def send(self, msg):
        '''Send a control message.

        Serializes to network byte order and sends atomically.

        :param msg: The message to send.
        :type msg: CtrlMsg
        '''
        payload = msg.payload if msg.payload else b''
        hdr = struct.pack(_HDR_FMT,
                          msg.type,
                          0,  # reserved
                          msg.slot_index,
                          msg.xfer_id,
                          len(payload))
        self._sock.sendall(hdr + payload)
        self.last_send_time = time.time()
        LOGGER.debug('Sent %s', msg)

    def recv(self):
        '''Receive a control message (blocking).

        :return: Parsed control message.
        :rtype: CtrlMsg
        :raises ConnectionError: If the peer disconnects.
        '''
        hdr = _recv_exactly(self._sock, CTRL_HDR_SIZE)
        msg_type, _, slot_index, xfer_id, payload_len = struct.unpack(
            _HDR_FMT, hdr)

        payload = b''
        if payload_len > 0:
            payload = _recv_exactly(self._sock, payload_len)

        msg = CtrlMsg(type=msg_type, slot_index=slot_index,
                      xfer_id=xfer_id, payload=payload)
        self.last_recv_time = time.time()
        LOGGER.debug('Recv %s', msg)
        return msg

    def send_keepalive(self):
        '''Send a KEEPALIVE message.'''
        self.send(CtrlMsg(type=CTRL_KEEPALIVE))

    def check_timeout(self, timeout_s):
        '''Check if the keepalive timeout has elapsed.

        :return: True if timed out (peer presumed dead).
        '''
        return time.time() - self.last_recv_time > timeout_s

    def keepalive_due(self, interval_s):
        '''Check if it is time to send a keepalive.

        :return: True if interval has elapsed since last send.
        '''
        return time.time() - self.last_send_time > interval_s

    def set_nonblocking(self, nonblocking=True):
        '''Set the socket to non-blocking mode.'''
        self._sock.setblocking(not nonblocking)

    def close(self):
        '''Close the control channel connection.'''
        if self._sock:
            try:
                self._sock.close()
            except OSError:
                pass
            self._sock = None
            LOGGER.info('Disconnected from ctrl channel %s', self._path)
