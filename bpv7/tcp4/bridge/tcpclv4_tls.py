'''
tcpclv4_tls.py: Python ctypes wrapper for libtcpclv4_tls.so

Provides Pythonic interface to C TCPCLv4 TLS session management.

Author: iondev33

Copyright (c) 2026, California Institute of Technology.
ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
'''

import ctypes
import ctypes.util
from ctypes import (
    CDLL, Structure, POINTER,
    c_void_p, c_char_p, c_int, c_uint8, c_size_t
)

# Load library
_lib_path = ctypes.util.find_library('tcpclv4_tls')
if not _lib_path:
    import os
    _lib_path = os.path.join(os.path.dirname(__file__), '..', 'libtcpclv4_tls.so')

_lib = CDLL(_lib_path)

# TLS configuration structure

class TLSConfig(Structure):
    _fields_ = [
        ('tls_cert_file', c_char_p),
        ('tls_key_file', c_char_p),
        ('tls_ca_file', c_char_p),
        ('tls_verify_peer', c_int),
    ]


# Initialize/cleanup
_lib.tcpclv4_tls_init.argtypes = []
_lib.tcpclv4_tls_init.restype = None

_lib.tcpclv4_tls_cleanup.argtypes = []
_lib.tcpclv4_tls_cleanup.restype = None

# Session establishment
_lib.tcpclv4_tls_connect.argtypes = [c_char_p, c_int, c_char_p, POINTER(TLSConfig)]
_lib.tcpclv4_tls_connect.restype = c_void_p

_lib.tcpclv4_tls_accept.argtypes = [c_int, c_char_p, POINTER(TLSConfig)]
_lib.tcpclv4_tls_accept.restype = c_void_p

# Message I/O
_lib.tcpclv4_tls_send_msg.argtypes = [c_void_p, POINTER(c_uint8), c_size_t]
_lib.tcpclv4_tls_send_msg.restype = c_int

_lib.tcpclv4_tls_recv_msg.argtypes = [c_void_p, POINTER(c_uint8), c_size_t, POINTER(c_size_t)]
_lib.tcpclv4_tls_recv_msg.restype = c_int

# Session control
_lib.tcpclv4_tls_get_fd.argtypes = [c_void_p]
_lib.tcpclv4_tls_get_fd.restype = c_int

_lib.tcpclv4_tls_set_nonblocking.argtypes = [c_void_p, c_int]
_lib.tcpclv4_tls_set_nonblocking.restype = c_int

_lib.tcpclv4_tls_get_peer_nodeid.argtypes = [c_void_p]
_lib.tcpclv4_tls_get_peer_nodeid.restype = c_char_p

_lib.tcpclv4_tls_is_secure.argtypes = [c_void_p]
_lib.tcpclv4_tls_is_secure.restype = c_int

# Session teardown
_lib.tcpclv4_tls_close.argtypes = [c_void_p]
_lib.tcpclv4_tls_close.restype = None

# Error handling
_lib.tcpclv4_tls_get_error.argtypes = [c_void_p]
_lib.tcpclv4_tls_get_error.restype = c_char_p

_lib.tcpclv4_tls_clear_error.argtypes = [c_void_p]
_lib.tcpclv4_tls_clear_error.restype = None


# Python-friendly wrapper class

class TLSSession:
    """Python wrapper for TCPCLv4 TLS session."""

    def __init__(self, host=None, port=None, node_id=None, tls_config=None,
                 client_fd=None):
        """Create TLS session.

        Client mode (active open):
            TLSSession(host='127.0.0.1', port=4556, node_id='ipn:2.0',
                       tls_config={'cert': ..., 'key': ..., 'ca': ...})

        Server mode (passive open):
            TLSSession(client_fd=sock_fd, node_id='ipn:3.0',
                       tls_config={...})

        :param host: Hostname/IP for client mode
        :param port: Port for client mode
        :param node_id: Local node ID
        :param tls_config: TLS configuration dict (or None for plain TCP)
        :param client_fd: Connected socket FD for server mode
        """
        _lib.tcpclv4_tls_init()

        # Prepare TLS config
        cfg = None
        if tls_config:
            cfg = TLSConfig()
            cfg.tls_cert_file = tls_config.get('cert', '').encode('utf-8') if tls_config.get('cert') else None
            cfg.tls_key_file = tls_config.get('key', '').encode('utf-8') if tls_config.get('key') else None
            cfg.tls_ca_file = tls_config.get('ca', '').encode('utf-8') if tls_config.get('ca') else None
            cfg.tls_verify_peer = tls_config.get('verify', 1)

        # Create session
        if client_fd is not None:
            # Server mode
            self._sess = _lib.tcpclv4_tls_accept(
                client_fd,
                node_id.encode('utf-8'),
                ctypes.byref(cfg) if cfg else None
            )
        else:
            # Client mode
            self._sess = _lib.tcpclv4_tls_connect(
                host.encode('utf-8'),
                port,
                node_id.encode('utf-8'),
                ctypes.byref(cfg) if cfg else None
            )

        if not self._sess:
            raise ConnectionError(f"Failed to establish TLS session")

    def send_msg(self, data: bytes) -> int:
        """Send TCPCLv4 message.

        :param data: Message bytes to send
        :return: Number of bytes sent, or 0 if would block, -1 on error
        """
        buf = (c_uint8 * len(data)).from_buffer_copy(data)
        ret = _lib.tcpclv4_tls_send_msg(self._sess, buf, len(data))
        return ret

    def recv_msg(self, max_len=65536) -> bytes:
        """Receive TCPCLv4 message.

        :param max_len: Maximum message size
        :return: Message bytes, or None if would block/error
        """
        buf = (c_uint8 * max_len)()
        out_len = c_size_t()
        ret = _lib.tcpclv4_tls_recv_msg(self._sess, buf, max_len,
                                         ctypes.byref(out_len))
        if ret == 1:
            return bytes(buf[:out_len.value])
        elif ret == 0:
            return None  # Would block
        else:
            return None  # Error

    def fileno(self) -> int:
        """Get socket file descriptor for select/poll/epoll.

        :return: Socket FD
        """
        return _lib.tcpclv4_tls_get_fd(self._sess)

    def set_nonblocking(self, nonblock: bool = True):
        """Set socket to non-blocking mode.

        :param nonblock: True for non-blocking, False for blocking
        """
        ret = _lib.tcpclv4_tls_set_nonblocking(self._sess, 1 if nonblock else 0)
        if ret < 0:
            raise OSError("Failed to set non-blocking mode")

    def get_peer_nodeid(self) -> str:
        """Get peer node ID from contact header exchange.

        :return: Peer node ID string, or None if not available
        """
        nodeid = _lib.tcpclv4_tls_get_peer_nodeid(self._sess)
        return nodeid.decode('utf-8') if nodeid else None

    def is_secure(self) -> bool:
        """Check if TLS is enabled.

        :return: True if TLS enabled, False if plain TCP
        """
        return bool(_lib.tcpclv4_tls_is_secure(self._sess))

    def get_error(self) -> str:
        """Get last error message.

        :return: Error string, or None if no error
        """
        err = _lib.tcpclv4_tls_get_error(self._sess)
        return err.decode('utf-8') if err else None

    def clear_error(self):
        """Clear error state."""
        _lib.tcpclv4_tls_clear_error(self._sess)

    def close(self):
        """Close TLS session gracefully."""
        if self._sess:
            _lib.tcpclv4_tls_close(self._sess)
            self._sess = None

    def __enter__(self):
        """Context manager support."""
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context manager cleanup."""
        self.close()

    def __del__(self):
        """Cleanup on garbage collection."""
        self.close()


def init():
    """Initialize OpenSSL library.

    Safe to call multiple times.
    """
    _lib.tcpclv4_tls_init()


def cleanup():
    """Cleanup OpenSSL library.

    Call at program exit. Safe to call multiple times.
    """
    _lib.tcpclv4_tls_cleanup()
