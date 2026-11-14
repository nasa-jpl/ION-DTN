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
import struct
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


SESS_INIT = 0x07
SESS_TERM = 0x05
KEEPALIVE = 0x04
TERM_CONTACT_FAILURE = 0x04


def build_sess_init(node_id):
    """RFC 9174 SESS_INIT: type, keepalive(u16), seg_mru(u64),
    xfer_mru(u64), node_id_len(u16), node_id, ext_len(u32).  All
    multi-byte fields are big-endian."""
    nid = node_id.encode()
    return (
        bytes([SESS_INIT])
        + struct.pack(">HQQH", 0, 1 << 20, 1 << 20, len(nid))
        + nid
        + struct.pack(">I", 0)
    )


def parse_messages(data):
    """Walk the concatenated TCPCL messages we understand (SESS_INIT,
    SESS_TERM, KEEPALIVE), returning a list of (name, reason)."""
    msgs = []
    i, n = 0, len(data)
    while i < n:
        t = data[i]
        if t == SESS_INIT:
            if i + 21 > n:
                break
            node_id_len = int.from_bytes(data[i + 19 : i + 21], "big")
            off = i + 21 + node_id_len
            if off + 4 > n:
                break
            ext_len = int.from_bytes(data[off : off + 4], "big")
            msgs.append(("SESS_INIT", None))
            i = off + 4 + ext_len
        elif t == SESS_TERM:
            if i + 3 > n:
                break
            msgs.append(("SESS_TERM", data[i + 2]))
            i += 3
        elif t == KEEPALIVE:
            msgs.append(("KEEPALIVE", None))
            i += 1
        else:
            break
    return msgs


def probe_nodeid_mismatch(host, port, pki):
    """Offer CAN_TLS, present node 2's valid certificate, then send a
    SESS_INIT claiming a Node ID the certificate does not vouch for.
    The agent must refuse with SESS_TERM (Contact Failure).  Run as the
    first/only inbound session so the agent processes our SESS_INIT (a
    second connection would be refused BUSY first)."""
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
    ctx.minimum_version = ssl.TLSVersion.TLSv1_2
    ctx.check_hostname = False
    ctx.verify_mode = ssl.CERT_NONE
    ctx.load_cert_chain(f"{pki}/node2-chain.pem", f"{pki}/node2.key")

    result = "inconclusive"
    try:
        tls = ctx.wrap_socket(s, server_hostname=host)
        tls.settimeout(10)
        tls.sendall(build_sess_init("ipn:9.9"))  # not our cert's EID
        data, _ = read_until_close(tls, 10, limit=256)
        msgs = parse_messages(data)
        if any(m == "SESS_TERM" and r == TERM_CONTACT_FAILURE
               for m, r in msgs):
            result = "rejected"
        elif msgs:
            result = "accepted"  # agent did not refuse the mismatch
        tls.close()
    except (ssl.SSLError, ConnectionResetError, BrokenPipeError):
        result = "handshake-failed"
    except (TimeoutError, OSError):
        pass

    try:
        s.close()
    except OSError:
        pass
    return result


def main():
    host = sys.argv[1]
    port = int(sys.argv[2])
    pki = sys.argv[3]
    mode = sys.argv[4] if len(sys.argv) > 4 else "all"

    if mode == "mismatch":
        # Node-ID authentication: a valid certificate but a SESS_INIT
        # claiming a Node ID the certificate does not carry must be
        # refused.  Must run before the real peer connects.
        result = probe_nodeid_mismatch(host, port, pki)
        check("nodeid-mismatch", result == "rejected", f"(got {result})")
        if failures:
            print("FAILED tls probes:", ", ".join(failures))
            return 1
        print("Node-ID mismatch probe behaved as expected.")
        return 0

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
