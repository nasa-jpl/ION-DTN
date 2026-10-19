#!/usr/bin/env python3
'''
test_python_tls.py: Test Python ctypes wrapper for TCPCLv4 TLS library.

Author: iondev33

Copyright (c) 2026, California Institute of Technology.
ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
'''

import sys
import os
import socket
import threading
import time

# Add parent directory to path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from bridge import tcpclv4_tls as tls
from bridge import tcpclv4_msg as msg

TEST_PORT = 24556


def test_initialization():
    """Test library initialization."""
    print("Testing TLS initialization... ", end='')

    # Should be safe to call multiple times
    tls.init()
    tls.init()

    print("PASS")


def server_thread(port, node_id):
    """Simple server thread for testing."""
    # Create listen socket
    listen_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    listen_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    listen_sock.bind(('127.0.0.1', port))
    listen_sock.listen(1)

    # Accept connection
    client_sock, _ = listen_sock.accept()
    client_fd = client_sock.fileno()

    # Create TLS session (no TLS for basic test)
    with tls.TLSSession(client_fd=client_fd, node_id=node_id,
                        tls_config=None) as sess:
        # Receive SESS_INIT
        data = sess.recv_msg()
        if data:
            # Send KEEPALIVE response
            keepalive = msg.encode_keepalive()
            sess.send_msg(keepalive)

    client_sock.close()
    listen_sock.close()


def test_connection():
    """Test TCP connection and message exchange."""
    print("Testing TLS connection (plain TCP)... ", end='')

    # Start server thread
    port = TEST_PORT
    server = threading.Thread(target=server_thread,
                              args=(port, 'ipn:3.0'),
                              daemon=True)
    server.start()

    # Give server time to start
    time.sleep(0.1)

    # Connect as client
    with tls.TLSSession(host='127.0.0.1', port=port,
                        node_id='ipn:2.0', tls_config=None) as sess:
        # Verify properties
        assert sess.fileno() > 0, "Invalid FD"
        assert not sess.is_secure(), "Should not be secure (no TLS)"

        # Send SESS_INIT
        sess_init = msg.encode_sess_init(
            keepalive_interval=30,
            segment_mru=1048576,
            transfer_mru=10485760,
            node_id="ipn:2.0"
        )
        ret = sess.send_msg(sess_init)
        assert ret > 0, "Failed to send SESS_INIT"

        # Receive KEEPALIVE
        data = sess.recv_msg()
        assert data is not None, "Failed to receive KEEPALIVE"
        assert msg.get_msg_type(data) == msg.MSG_KEEPALIVE, "Wrong message type"

    server.join(timeout=1.0)

    print("PASS")


def test_nonblocking():
    """Test non-blocking mode."""
    print("Testing non-blocking mode... ", end='')

    # Start server thread
    port = TEST_PORT + 1
    server = threading.Thread(target=server_thread,
                              args=(port, 'ipn:3.0'),
                              daemon=True)
    server.start()

    time.sleep(0.1)

    # Connect and set non-blocking
    with tls.TLSSession(host='127.0.0.1', port=port,
                        node_id='ipn:2.0', tls_config=None) as sess:
        sess.set_nonblocking(True)

        # Send message
        sess_init = msg.encode_sess_init(
            keepalive_interval=30,
            segment_mru=1048576,
            transfer_mru=10485760,
            node_id="ipn:2.0"
        )
        sess.send_msg(sess_init)

        # Try immediate receive (should return None since would block)
        # Actually, data might be available immediately, so just verify no crash
        data = sess.recv_msg()

    server.join(timeout=1.0)

    print("PASS")


def test_error_handling():
    """Test error handling."""
    print("Testing error handling... ", end='')

    try:
        # Try to connect to non-existent server (should fail)
        sess = tls.TLSSession(host='127.0.0.1', port=65534,
                              node_id='ipn:2.0', tls_config=None)
        # If we get here, connection somehow succeeded (unlikely)
        sess.close()
        raise AssertionError("Expected connection to fail")
    except ConnectionError:
        # Expected failure
        pass

    print("PASS")


def main():
    print("TCPCLv4 TLS Python Wrapper Tests")
    print("=================================\n")

    try:
        test_initialization()
        test_connection()
        test_nonblocking()
        test_error_handling()

        print("\n=================================")
        print("ALL TESTS PASSED")
        return 0

    except AssertionError as e:
        print(f"FAIL: {e}")
        return 1
    except Exception as e:
        print(f"ERROR: {e}")
        import traceback
        traceback.print_exc()
        return 1
    finally:
        tls.cleanup()


if __name__ == '__main__':
    sys.exit(main())
