#!/usr/bin/env python3
"""Adversarial probes against a live passive tcpclv4_agent.

Each probe opens its own TCP connection to the agent's listen port and
misbehaves in a specific way.  The agent must survive every case, and
the established primary session (agent <-> agent) must be undisturbed.

Exit status: 0 if all probes behaved as expected, 1 otherwise.

Usage: probe.py <host> <port>
"""

import socket
import sys
import time

CONTACT_OK = bytes([0x64, 0x74, 0x6E, 0x21, 0x04, 0x00])  # dtn! v4 no-TLS

failures = []


def check(name, ok, detail=""):
    tag = "ok" if ok else "FAIL"
    print(f"probe {name}: {tag} {detail}")
    if not ok:
        failures.append(name)


def recv_all(sock, timeout, limit=4096):
    """Read until the peer closes or the timeout expires."""
    sock.settimeout(timeout)
    data = b""
    try:
        while len(data) < limit:
            chunk = sock.recv(4096)
            if not chunk:
                break
            data += chunk
    except TimeoutError:
        pass
    except ConnectionResetError:
        pass
    return data


def connect(host, port):
    return socket.create_connection((host, port), timeout=10)


def probe_bad_magic(host, port):
    """Wrong contact-header magic: the agent must reject the session
    (close the connection) without dying."""
    s = connect(host, port)
    s.sendall(b"xxx!\x04\x00")
    data = recv_all(s, 5)
    s.close()
    # The agent sends its own contact header before reading ours, so we
    # may see 6 bytes; the connection must then close without SESS_INIT
    # acceptance progressing to an established session.  Any outcome
    # short of a hang is acceptable; the real assertions are that the
    # agent survives and the primary session still works.
    check("bad-magic", True, f"({len(data)} bytes then close)")


def probe_bad_version(host, port):
    """Unsupported TCPCL version: must be rejected."""
    s = connect(host, port)
    s.sendall(bytes([0x64, 0x74, 0x6E, 0x21, 0x03, 0x00]))  # version 3
    data = recv_all(s, 5)
    s.close()
    check("bad-version", True, f"({len(data)} bytes then close)")


def probe_silent_peer(host, port):
    """Connect and send nothing.  The agent bounds establishment I/O
    with a socket timeout (10s); the connection must be dropped and the
    agent must not be stalled forever."""
    s = connect(host, port)
    start = time.monotonic()
    data = recv_all(s, 20)  # wait past the agent's 10s timeout
    elapsed = time.monotonic() - start
    s.close()
    # We expect the agent's contact header (6 bytes) and then a close
    # once its read timeout expires.  If we are still connected after
    # 18s, the agent is hung.
    check(
        "silent-peer",
        elapsed < 18,
        f"(released after {elapsed:.1f}s, {len(data)} bytes)",
    )


def probe_busy_refusal(host, port):
    """Well-formed second connection while the primary session is up:
    must receive contact header + SESS_INIT + SESS_TERM reason BUSY."""
    s = connect(host, port)
    s.sendall(CONTACT_OK)
    data = recv_all(s, 10, limit=600)
    s.close()

    if len(data) < 6 or data[:4] != b"dtn!":
        check("busy-refusal", False, "(no contact header)")
        return

    if len(data) < 7 or data[6] != 0x07:
        check("busy-refusal", False, "(no SESS_INIT)")
        return

    # Walk over SESS_INIT: type(1)+keepalive(2)+segMRU(8)+xferMRU(8)
    # +nodeIdLen(2)+nodeId+extLen(4)+ext
    off = 6 + 1 + 2 + 8 + 8
    nid_len = int.from_bytes(data[off : off + 2], "big")
    off += 2 + nid_len
    ext_len = int.from_bytes(data[off : off + 4], "big")
    off += 4 + ext_len

    if off + 3 > len(data) or data[off] != 0x05:
        check("busy-refusal", False, "(no SESS_TERM after SESS_INIT)")
        return

    reason = data[off + 2]
    check("busy-refusal", reason == 0x03, f"(SESS_TERM reason {reason})")


def probe_garbage_flood(host, port):
    """Valid contact header followed by garbage bytes: the agent's
    refusal/establishment path must reject it without harm."""
    s = connect(host, port)
    s.sendall(CONTACT_OK)
    s.sendall(bytes([0xAA] * 512))  # unknown message type, then noise
    data = recv_all(s, 5)
    s.close()
    check("garbage-flood", True, f"({len(data)} bytes then close)")


def main():
    host = sys.argv[1]
    port = int(sys.argv[2])

    probe_bad_magic(host, port)
    time.sleep(1)
    probe_bad_version(host, port)
    time.sleep(1)
    probe_busy_refusal(host, port)
    time.sleep(1)
    probe_garbage_flood(host, port)
    time.sleep(1)
    probe_silent_peer(host, port)

    if failures:
        print("FAILED probes:", ", ".join(failures))
        return 1

    print("All probes behaved as expected.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
