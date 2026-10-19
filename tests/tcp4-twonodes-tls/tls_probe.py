#!/usr/bin/env python3
"""TLS-negative probes against a live TLS-required tcpclv4_agent.

Case 1: peer that does not offer CAN_TLS.  The agent is configured
with TLS, so RFC 9174 CAN_TLS negotiation fails and the session must
be refused (connection closed without any TCPCL session).

Case 2: peer offers CAN_TLS but presents no client certificate.  The
agent verifies peers (SSL_VERIFY_FAIL_IF_NO_PEER_CERT), so the TLS
handshake must fail.

Both cases must leave the agent alive; the caller re-verifies the
primary session afterward.

Usage: tls_probe.py <host> <port>
"""

import socket
import ssl
import sys

CONTACT_PLAIN = bytes([0x64, 0x74, 0x6E, 0x21, 0x04, 0x00])  # no CAN_TLS
CONTACT_TLS = bytes([0x64, 0x74, 0x6E, 0x21, 0x04, 0x01])  # CAN_TLS

failures = []


def check(name, ok, detail=""):
    tag = "ok" if ok else "FAIL"
    print(f"tls-probe {name}: {tag} {detail}")
    if not ok:
        failures.append(name)


def read_until_close(sock, timeout, limit=64):
    sock.settimeout(timeout)
    data = b""
    try:
        while len(data) < limit:
            chunk = sock.recv(64)
            if not chunk:
                return data, True
            data += chunk
    except TimeoutError:
        return data, False
    except (ConnectionResetError, BrokenPipeError):
        return data, True
    return data, False


def probe_no_can_tls(host, port):
    s = socket.create_connection((host, port), timeout=10)
    s.sendall(CONTACT_PLAIN)
    data, closed = read_until_close(s, 12)
    s.close()
    # Expect the agent's contact header (6 bytes, CAN_TLS set) and then
    # a close: TLS is configured, we refused it, so no session.
    check(
        "no-can-tls",
        closed and len(data) >= 6,
        f"(got {len(data)} bytes, closed={closed})",
    )


def probe_no_client_cert(host, port):
    s = socket.create_connection((host, port), timeout=10)
    s.sendall(CONTACT_TLS)
    hdr = b""
    s.settimeout(10)
    try:
        while len(hdr) < 6:
            chunk = s.recv(6 - len(hdr))
            if not chunk:
                break
            hdr += chunk
    except TimeoutError:
        pass

    if len(hdr) < 6 or not (hdr[5] & 0x01):
        s.close()
        check("no-client-cert", False, "(peer did not offer CAN_TLS)")
        return

    ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
    ctx.minimum_version = ssl.TLSVersion.TLSv1_2  # don't negotiate down to legacy TLS
    ctx.check_hostname = False
    ctx.verify_mode = ssl.CERT_NONE  # we don't check the server...
    handshake_failed = False
    try:
        tls = ctx.wrap_socket(s, server_hostname=host)
        # ...but the server checks us, and we present no certificate.
        # Some stacks fail wrap_socket; TLS 1.3 servers may not report
        # the failure until the first read.
        tls.settimeout(10)
        got = tls.recv(64)
        if got == b"":
            handshake_failed = True  # closed without any TCPCL data
        tls.close()
    except (ssl.SSLError, ConnectionResetError, TimeoutError, OSError):
        handshake_failed = True

    try:
        s.close()
    except OSError:
        pass

    check(
        "no-client-cert",
        handshake_failed,
        "(handshake rejected as expected)"
        if handshake_failed
        else "(server accepted an uncertified peer!)",
    )


def main():
    host = sys.argv[1]
    port = int(sys.argv[2])
    probe_no_can_tls(host, port)
    probe_no_client_cert(host, port)
    if failures:
        print("FAILED tls probes:", ", ".join(failures))
        return 1
    print("All TLS-negative probes behaved as expected.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
