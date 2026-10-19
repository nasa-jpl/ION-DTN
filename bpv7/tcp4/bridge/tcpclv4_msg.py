'''
tcpclv4_msg.py: Python ctypes wrapper for libtcpclv4_msg.so

Provides Pythonic interface to C TCPCLv4 message encode/decode functions.

Author: iondev33

Copyright (c) 2026, California Institute of Technology.
ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
'''

import ctypes
import ctypes.util
from ctypes import (
    CDLL, Structure, POINTER,
    c_uint8, c_uint16, c_uint32, c_uint64, c_size_t, c_int, c_char_p
)

# Load library (search in standard paths + current directory)
_lib_path = ctypes.util.find_library('tcpclv4_msg')
if not _lib_path:
    # Try relative path for development
    import os
    _lib_path = os.path.join(os.path.dirname(__file__), '..', 'libtcpclv4_msg.so')

_lib = CDLL(_lib_path)

# Message type constants (from tcpclv4_msg.h)
MSG_SESS_INIT = 0x07
MSG_SESS_TERM = 0x05
MSG_XFER_SEGMENT = 0x01
MSG_XFER_ACK = 0x02
MSG_XFER_REFUSE = 0x03
MSG_KEEPALIVE = 0x04
MSG_MSG_REJECT = 0x06

# Contact header flags
CONTACT_FLAG_CAN_TLS = 0x01

# XFER_SEGMENT flags
XFER_FLAG_END = 0x01
XFER_FLAG_START = 0x02

# SESS_TERM reason codes
TERM_UNKNOWN = 0x00
TERM_IDLE_TIMEOUT = 0x01
TERM_VERSION_MISMATCH = 0x02
TERM_BUSY = 0x03
TERM_CONTACT_FAILURE = 0x04
TERM_RESOURCE_EXHAUSTION = 0x05

# XFER_REFUSE reason codes
REFUSE_UNKNOWN = 0x00
REFUSE_COMPLETED = 0x01
REFUSE_NO_RESOURCES = 0x02
REFUSE_RETRANSMIT = 0x03
REFUSE_NOT_ACCEPTABLE = 0x04
REFUSE_EXTENSION_FAILURE = 0x05

# Message structures (mirrors C structs)

class ContactHeader(Structure):
    _fields_ = [
        ('magic', c_uint32),
        ('flags', c_uint8),
    ]


class SessionInit(Structure):
    _fields_ = [
        ('keepalive_interval', c_uint16),
        ('segment_mru', c_uint64),
        ('transfer_mru', c_uint64),
        ('node_id', c_char_p),
        ('node_id_len', c_uint16),
        ('extensions', POINTER(c_uint8)),
        ('extensions_len', c_uint16),
    ]


class SessionTerm(Structure):
    _fields_ = [
        ('flags', c_uint8),
        ('reason_code', c_uint8),
    ]


class XferSegment(Structure):
    _fields_ = [
        ('flags', c_uint8),
        ('transfer_id', c_uint64),
        ('data_offset', c_uint64),
        ('extensions', POINTER(c_uint8)),
        ('extensions_len', c_uint16),
        ('data', POINTER(c_uint8)),
        ('data_len', c_uint64),
    ]


class XferAck(Structure):
    _fields_ = [
        ('flags', c_uint8),
        ('transfer_id', c_uint64),
        ('ack_len', c_uint64),
    ]


class XferRefuse(Structure):
    _fields_ = [
        ('reason_code', c_uint8),
        ('transfer_id', c_uint64),
    ]


class MsgReject(Structure):
    _fields_ = [
        ('reason_code', c_uint8),
        ('rejected_msg_header', c_uint8),
    ]


# Configure function signatures

# Decode functions
_lib.tcpclv4_decode_contact_header.argtypes = [POINTER(c_uint8), c_size_t, POINTER(ContactHeader)]
_lib.tcpclv4_decode_contact_header.restype = c_int

_lib.tcpclv4_decode_sess_init.argtypes = [POINTER(c_uint8), c_size_t, POINTER(SessionInit)]
_lib.tcpclv4_decode_sess_init.restype = c_int

_lib.tcpclv4_decode_sess_term.argtypes = [POINTER(c_uint8), c_size_t, POINTER(SessionTerm)]
_lib.tcpclv4_decode_sess_term.restype = c_int

_lib.tcpclv4_decode_xfer_segment.argtypes = [POINTER(c_uint8), c_size_t, POINTER(XferSegment)]
_lib.tcpclv4_decode_xfer_segment.restype = c_int

_lib.tcpclv4_decode_xfer_ack.argtypes = [POINTER(c_uint8), c_size_t, POINTER(XferAck)]
_lib.tcpclv4_decode_xfer_ack.restype = c_int

_lib.tcpclv4_decode_xfer_refuse.argtypes = [POINTER(c_uint8), c_size_t, POINTER(XferRefuse)]
_lib.tcpclv4_decode_xfer_refuse.restype = c_int

_lib.tcpclv4_decode_msg_reject.argtypes = [POINTER(c_uint8), c_size_t, POINTER(MsgReject)]
_lib.tcpclv4_decode_msg_reject.restype = c_int

# Encode functions
_lib.tcpclv4_encode_contact_header.argtypes = [POINTER(ContactHeader), POINTER(c_uint8), c_size_t, POINTER(c_size_t)]
_lib.tcpclv4_encode_contact_header.restype = c_int

_lib.tcpclv4_encode_sess_init.argtypes = [POINTER(SessionInit), POINTER(c_uint8), c_size_t, POINTER(c_size_t)]
_lib.tcpclv4_encode_sess_init.restype = c_int

_lib.tcpclv4_encode_sess_term.argtypes = [POINTER(SessionTerm), POINTER(c_uint8), c_size_t, POINTER(c_size_t)]
_lib.tcpclv4_encode_sess_term.restype = c_int

_lib.tcpclv4_encode_xfer_segment.argtypes = [POINTER(XferSegment), POINTER(c_uint8), c_size_t, POINTER(c_size_t)]
_lib.tcpclv4_encode_xfer_segment.restype = c_int

_lib.tcpclv4_encode_xfer_ack.argtypes = [POINTER(XferAck), POINTER(c_uint8), c_size_t, POINTER(c_size_t)]
_lib.tcpclv4_encode_xfer_ack.restype = c_int

_lib.tcpclv4_encode_xfer_refuse.argtypes = [POINTER(XferRefuse), POINTER(c_uint8), c_size_t, POINTER(c_size_t)]
_lib.tcpclv4_encode_xfer_refuse.restype = c_int

_lib.tcpclv4_encode_keepalive.argtypes = [POINTER(c_uint8), c_size_t, POINTER(c_size_t)]
_lib.tcpclv4_encode_keepalive.restype = c_int

_lib.tcpclv4_encode_msg_reject.argtypes = [POINTER(MsgReject), POINTER(c_uint8), c_size_t, POINTER(c_size_t)]
_lib.tcpclv4_encode_msg_reject.restype = c_int

# Memory management
_lib.tcpclv4_sess_init_free.argtypes = [POINTER(SessionInit)]
_lib.tcpclv4_sess_init_free.restype = None

_lib.tcpclv4_xfer_segment_free.argtypes = [POINTER(XferSegment)]
_lib.tcpclv4_xfer_segment_free.restype = None

# Utilities
_lib.tcpclv4_get_msg_type.argtypes = [POINTER(c_uint8), c_size_t]
_lib.tcpclv4_get_msg_type.restype = c_int

_lib.tcpclv4_msg_type_name.argtypes = [c_uint8]
_lib.tcpclv4_msg_type_name.restype = c_char_p


# Python-friendly wrapper functions

def decode_contact_header(data: bytes) -> ContactHeader:
    """Decode contact header from wire bytes."""
    msg = ContactHeader()
    buf = (c_uint8 * len(data)).from_buffer_copy(data)
    ret = _lib.tcpclv4_decode_contact_header(buf, len(data), ctypes.byref(msg))
    if ret < 0:
        raise ValueError("Failed to decode contact header")
    return msg


def decode_sess_init(data: bytes) -> SessionInit:
    """Decode SESS_INIT from wire bytes."""
    msg = SessionInit()
    buf = (c_uint8 * len(data)).from_buffer_copy(data)
    ret = _lib.tcpclv4_decode_sess_init(buf, len(data), ctypes.byref(msg))
    if ret < 0:
        raise ValueError("Failed to decode SESS_INIT")
    return msg


def decode_sess_term(data: bytes) -> SessionTerm:
    """Decode SESS_TERM from wire bytes."""
    msg = SessionTerm()
    buf = (c_uint8 * len(data)).from_buffer_copy(data)
    ret = _lib.tcpclv4_decode_sess_term(buf, len(data), ctypes.byref(msg))
    if ret < 0:
        raise ValueError("Failed to decode SESS_TERM")
    return msg


def decode_xfer_segment(data: bytes) -> XferSegment:
    """Decode XFER_SEGMENT from wire bytes."""
    msg = XferSegment()
    buf = (c_uint8 * len(data)).from_buffer_copy(data)
    ret = _lib.tcpclv4_decode_xfer_segment(buf, len(data), ctypes.byref(msg))
    if ret < 0:
        raise ValueError("Failed to decode XFER_SEGMENT")
    return msg


def decode_xfer_ack(data: bytes) -> XferAck:
    """Decode XFER_ACK from wire bytes."""
    msg = XferAck()
    buf = (c_uint8 * len(data)).from_buffer_copy(data)
    ret = _lib.tcpclv4_decode_xfer_ack(buf, len(data), ctypes.byref(msg))
    if ret < 0:
        raise ValueError("Failed to decode XFER_ACK")
    return msg


def decode_xfer_refuse(data: bytes) -> XferRefuse:
    """Decode XFER_REFUSE from wire bytes."""
    msg = XferRefuse()
    buf = (c_uint8 * len(data)).from_buffer_copy(data)
    ret = _lib.tcpclv4_decode_xfer_refuse(buf, len(data), ctypes.byref(msg))
    if ret < 0:
        raise ValueError("Failed to decode XFER_REFUSE")
    return msg


def encode_sess_init(keepalive_interval: int, segment_mru: int,
                      transfer_mru: int, node_id: str) -> bytes:
    """Encode SESS_INIT to wire bytes."""
    msg = SessionInit()
    msg.keepalive_interval = keepalive_interval
    msg.segment_mru = segment_mru
    msg.transfer_mru = transfer_mru
    msg.node_id = node_id.encode('utf-8')
    msg.node_id_len = len(node_id)
    msg.extensions = None
    msg.extensions_len = 0

    buf = (c_uint8 * 4096)()
    out_len = c_size_t()
    ret = _lib.tcpclv4_encode_sess_init(ctypes.byref(msg), buf, 4096,
                                         ctypes.byref(out_len))
    if ret < 0:
        raise ValueError("Failed to encode SESS_INIT")

    return bytes(buf[:out_len.value])


def encode_xfer_ack(transfer_id: int, ack_len: int) -> bytes:
    """Encode XFER_ACK to wire bytes."""
    msg = XferAck()
    msg.flags = 0
    msg.transfer_id = transfer_id
    msg.ack_len = ack_len

    buf = (c_uint8 * 32)()
    out_len = c_size_t()
    ret = _lib.tcpclv4_encode_xfer_ack(ctypes.byref(msg), buf, 32,
                                        ctypes.byref(out_len))
    if ret < 0:
        raise ValueError("Failed to encode XFER_ACK")

    return bytes(buf[:out_len.value])


def encode_keepalive() -> bytes:
    """Encode KEEPALIVE to wire bytes."""
    buf = (c_uint8 * 1)()
    out_len = c_size_t()
    ret = _lib.tcpclv4_encode_keepalive(buf, 1, ctypes.byref(out_len))
    if ret < 0:
        raise ValueError("Failed to encode KEEPALIVE")

    return bytes(buf[:out_len.value])


def get_msg_type(data: bytes) -> int:
    """Get message type from first byte of wire data."""
    buf = (c_uint8 * len(data)).from_buffer_copy(data)
    return _lib.tcpclv4_get_msg_type(buf, len(data))


def msg_type_name(msg_type: int) -> str:
    """Get human-readable name for message type."""
    name = _lib.tcpclv4_msg_type_name(msg_type)
    return name.decode('utf-8') if name else "UNKNOWN"


# Context managers for automatic memory cleanup

class SessionInitContext:
    """Context manager for SessionInit with automatic cleanup."""
    def __init__(self, data: bytes):
        self.msg = decode_sess_init(data)

    def __enter__(self) -> SessionInit:
        return self.msg

    def __exit__(self, exc_type, exc_val, exc_tb):
        _lib.tcpclv4_sess_init_free(ctypes.byref(self.msg))


class XferSegmentContext:
    """Context manager for XferSegment with automatic cleanup."""
    def __init__(self, data: bytes):
        self.msg = decode_xfer_segment(data)

    def __enter__(self) -> XferSegment:
        return self.msg

    def __exit__(self, exc_type, exc_val, exc_tb):
        _lib.tcpclv4_xfer_segment_free(ctypes.byref(self.msg))
