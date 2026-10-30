#!/usr/bin/env python3
"""TLS-negative probes against a live TLS-required tcpclv4_agent.

Case 1: peer that does not offer CAN_TLS.  The agent is configured
with TLS, so RFC 9174 CAN_TLS negotiation fails and the session must
be refused (connection closed without any TCPCL session).

Cases 2-4: peer offers CAN_TLS but presents no client certificate, a
certificate from an untrusted CA, or an expired certificate.  The agent
verifies peers (SSL_VERIFY_FAIL_IF_NO_PEER_CERT), so the TLS handshake
must fail.

Case 5: peer presents a valid certificate.  The handshake must succeed
and the agent must send SESS_INIT (followed by SESS_TERM BUSY, since
the primary session is up).  This shows the rejections above come from
certificate checks.

All cases must leave the agent alive; the caller re-verifies the
primary session afterward.

Usage: tls_probe.py <host> <port> <pki_dir>
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


def tls_client_probe(host, port, cert=None, key=None):
    """Offer CAN_TLS, then run a TLS handshake presenting cert/key (or
    no certificate).  Returns "rejected" if the agent fails the
    handshake or closes before sending any TCPCL data, "accepted" if
    it sends SESS_INIT, and "inconclusive" otherwise."""
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
        return "inconclusive"

    ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
    ctx.minimum_version = ssl.TLSVersion.TLSv1_2  # don't negotiate down to legacy TLS
    ctx.check_hostname = False
    ctx.verify_mode = ssl.CERT_NONE  # we don't check the server...
    if cert:
        ctx.load_cert_chain(cert, key)
    result = "inconclusive"
    try:
        tls = ctx.wrap_socket(s, server_hostname=host)
        # ...but the server checks us.  Some stacks fail wrap_socket;
        # TLS 1.3 servers may not report the failure until the first
        # read.  On success the agent sends SESS_INIT unprompted.
        tls.settimeout(10)
        got = tls.recv(64)
        if got == b"":
            result = "rejected"  # closed without any TCPCL data
        elif got[0] == 0x07:
            result = "accepted"
        tls.close()
    except (ssl.SSLError, ConnectionResetError, BrokenPipeError):
        result = "rejected"
    except (TimeoutError, OSError):
        pass

    try:
        s.close()
    except OSError:
        pass

    return result


def probe_client_cert(name, host, port, expect, cert=None, key=None):
    result = tls_client_probe(host, port, cert, key)
    check(name, result == expect, f"(expected {expect}, got {result})")


def main():
    host = sys.argv[1]
    port = int(sys.argv[2])
    pki = sys.argv[3]
    probe_no_can_tls(host, port)
    probe_client_cert("no-client-cert", host, port, "rejected")
    probe_client_cert(
        "untrusted-ca",
        host,
        port,
        "rejected",
        f"{pki}/untrusted.crt",
        f"{pki}/probe.key",
    )
    probe_client_cert(
        "expired-cert",
        host,
        port,
        "rejected",
        f"{pki}/expired.crt",
        f"{pki}/probe.key",
    )
    probe_client_cert(
        "valid-cert",
        host,
        port,
        "accepted",
        f"{pki}/node2-chain.pem",
        f"{pki}/node2.key",
    )
    if failures:
        print("FAILED tls probes:", ", ".join(failures))
        return 1
    print("All TLS-negative probes behaved as expected.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
