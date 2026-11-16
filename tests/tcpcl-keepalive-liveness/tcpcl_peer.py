#!/usr/bin/env python3
"""Scripted TCPCLv3 (RFC 7242) peer that checks tcpcli's reception timer.

Node 1's planned session connects to the port this peer listens on; the
peer also connects to node 1's induct.  The peer advertises a keepalive
interval of K = 2 s, so node 1 must close a session only after more than
2K = 4 s without hearing from the peer.

  P1  silent peer: node 1 closes between 4 and 7 s after our header.
      The header goes out half a second after node 1 connects, which is
      half a clock tick, so a timeout one tick early shows up as 3.5 s.
  P2  ACK, LENGTH and REFUSE every 1.5 s, no keepalives: node 1 stays.
  P3  one 20000-byte data segment trickled over 10 s: node 1 stays and
      acknowledges all 20000 bytes.
  P4  accepted session closed after node 1's first keepalive, then
      reopened about K s later, around the tick at which a timer left
      running on the closed session would expire: node 1 serves the new
      session, and afterwards still answers a fresh connection.

Prints a PASS or FAIL line per phase; exits 0 only if every phase passes.
"""

import argparse
import itertools
import os
import socket
import struct
import threading
import time

HOST = "127.0.0.1"
K = 2  # keepalive interval we advertise
TIMEOUT = 2 * K  # silence node 1 must tolerate
DEADLINE = 150.0  # limit on the whole run, in seconds
KEEPALIVE = b"\x40"
NAMES = {
    1: "DATA_SEGMENT",
    2: "ACK_SEGMENT",
    3: "REFUSE_BUNDLE",
    4: "KEEPALIVE",
    5: "SHUTDOWN",
    6: "LENGTH",
}

T0 = time.monotonic()
print_lock = threading.Lock()


def now():
    return time.monotonic()


def log(msg):
    with print_lock:
        print(f"[{now() - T0:7.3f}] {msg}", flush=True)


def sleep_until(t):
    delay = t - now()
    if delay > 0:
        time.sleep(delay)


def sdnv(n):
    out = [n & 0x7F]
    n >>= 7
    while n:
        out.append(0x80 | (n & 0x7F))
        n >>= 7
    return bytes(reversed(out))


def contact_header(eid):
    eid = eid.encode()
    return b"dtn!" + bytes([3, 0x01]) + struct.pack(">H", K) + sdnv(len(eid)) + eid


class Conn:
    """A TCPCL connection to node 1; a thread logs all that node 1 sends."""

    def __init__(self, sock, tag):
        self.sock = sock
        self.tag = tag
        self.start = now()
        self.events = []  # (kind, time, value)
        self.cv = threading.Condition()
        self.buf = b""
        self.closing = False
        self.reader = threading.Thread(target=self._read, daemon=True)
        self.reader.start()

    def _note(self, kind, value=None, text=None):
        t = now()
        with self.cv:
            self.events.append((kind, t, value))
            self.cv.notify_all()
        log(f"{self.tag} rx {text or kind} (+{t - self.start:.3f} s)")

    def _recv(self, n):
        while len(self.buf) < n:
            data = self.sock.recv(65536)
            if not data:
                raise EOFError
            self.buf += data
        out, self.buf = self.buf[:n], self.buf[n:]
        return out

    def _sdnv(self):
        val = 0
        for _ in range(10):
            b = self._recv(1)[0]
            val = (val << 7) | (b & 0x7F)
            if not b & 0x80:
                return val
        raise ValueError("SDNV longer than 10 bytes")

    def _read(self):
        try:
            hdr = self._recv(8)
            eid = self._recv(self._sdnv()).decode(errors="replace")
            ka = struct.unpack(">H", hdr[6:8])[0]
            self._note(
                "header",
                hdr[:4],
                f"contact header {hdr[:4]!r} ver {hdr[4]} flags 0x{hdr[5]:02x} "
                f"keepalive {ka} eid {eid}",
            )
            while True:
                byte = self._recv(1)[0]
                kind = byte >> 4
                name = NAMES.get(kind, "UNKNOWN")
                if kind == 1:
                    length = self._sdnv()
                    self._recv(length)
                    self._note("data", length, f"{name} {length} bytes")
                elif kind in (2, 6):
                    value = self._sdnv()
                    tag = "ack" if kind == 2 else "length"
                    self._note(tag, value, f"{name} {value}")
                elif kind in (3, 4):
                    tag = "refuse" if kind == 3 else "keepalive"
                    self._note(tag, None, name)
                elif kind == 5:
                    reason = self._recv(1)[0] if byte & 0x02 else None
                    delay = self._sdnv() if byte & 0x01 else None
                    self._note(
                        "shutdown",
                        byte,
                        f"SHUTDOWN 0x{byte:02x} reason {reason} delay {delay}",
                    )
                else:
                    self._note("unknown", byte, f"unknown byte 0x{byte:02x}")
                    return
        except (EOFError, OSError, ValueError) as e:
            if self.closing:
                return
            if isinstance(e, EOFError):
                self._note("eof", None, "EOF: node 1 closed the connection")
            else:
                self._note("error", None, f"error {e!r}")

    def send(self, data, what):
        if self.closing:
            return False
        try:
            self.sock.sendall(data)
        except OSError as e:
            log(f"{self.tag} send of {what} failed: {e!r}")
            return False
        if what:
            log(f"{self.tag} tx {what} (+{now() - self.start:.3f} s)")
        return True

    def first(self, kinds):
        with self.cv:
            for e in self.events:
                if e[0] in kinds:
                    return e
        return None

    def wait(self, kinds, until):
        with self.cv:
            while True:
                e = self.first(kinds)
                if e is not None or now() >= until:
                    return e
                self.cv.wait(until - now())

    def values(self, kind):
        with self.cv:
            return [e[2] for e in self.events if e[0] == kind]

    def count(self, kinds, after, before):
        with self.cv:
            return sum(
                1 for e in self.events if e[0] in kinds and after <= e[1] <= before
            )

    def closed_at(self):
        e = self.first(("eof", "error", "shutdown", "unknown"))
        return None if e is None else e[1]

    def hold(self, until):
        """Wait until `until` or until node 1 closes the session."""
        self.wait(("eof", "error", "shutdown", "unknown"), until)

    def pace(self, start, every=1.0):
        """Send a keepalive every `every` s from `start` until closed."""

        def run():
            t = start
            while True:
                t += every
                sleep_until(t)
                if self.closing or self.closed_at() is not None:
                    return
                self.send(KEEPALIVE, "KEEPALIVE")

        threading.Thread(target=run, daemon=True).start()

    def close(self):
        self.closing = True
        try:
            self.sock.shutdown(socket.SHUT_RDWR)
        except OSError:
            pass
        self.sock.close()
        self.reader.join(2)
        log(f"{self.tag} closed by the peer (+{now() - self.start:.3f} s)")


class Peer:
    def __init__(self, args):
        self.args = args
        self.listener = None
        self.last_end = None
        self.results = []

    def result(self, phase, ok, msg):
        line = f"{'PASS' if ok else 'FAIL'} {phase}: {msg}"
        self.results.append(line)
        log(line)
        return ok

    def listen(self):
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        s.bind((HOST, self.args.listen_port))
        s.listen(4)
        self.listener = s
        log(f"listening on {HOST}:{self.args.listen_port}")

    def accept_planned(self, tag, timeout):
        self.listener.settimeout(timeout)
        try:
            sock, addr = self.listener.accept()
        except TimeoutError:
            log(f"{tag} node 1 did not connect within {timeout} s")
            return None
        sock.settimeout(None)
        if self.last_end is None:
            log(f"{tag} node 1 connected from port {addr[1]}")
        else:
            log(
                f"{tag} node 1 reconnected from port {addr[1]}, "
                f"{now() - self.last_end:.3f} s after the previous session ended"
            )
        return Conn(sock, tag)

    def end_planned(self, conn):
        closed = conn.closed_at()
        conn.close()
        self.last_end = closed if closed is not None else now()

    def connect_induct(self, tag):
        try:
            sock = socket.create_connection((HOST, self.args.induct_port), 5)
        except OSError as e:
            log(f"{tag} connect to {HOST}:{self.args.induct_port} failed: {e!r}")
            return None
        sock.settimeout(None)
        log(f"{tag} connected to node 1's induct")
        return Conn(sock, tag)

    def p1_silent(self):
        conn = self.accept_planned("P1", 30)
        if conn is None:
            return self.result("P1", False, "node 1 never connected")
        conn.hold(conn.start + 0.5)  # half a tick after node 1's connect
        conn.send(contact_header("ipn:2.0"), "contact header")
        t_hdr = now()
        end = conn.wait(("eof", "error"), t_hdr + TIMEOUT + 3.5)
        shut = conn.first(("shutdown",))
        self.end_planned(conn)
        if shut is not None:
            log(f"P1 node 1 sent SHUTDOWN {shut[1] - t_hdr:.3f} s after our header")
        if end is None:
            return self.result(
                "P1",
                False,
                f"node 1 kept a silent session open for {TIMEOUT + 3.5:.1f} s",
            )
        dt = end[1] - t_hdr
        ok = TIMEOUT < dt < TIMEOUT + 3
        return self.result(
            "P1",
            ok,
            f"node 1 closed the silent session {dt:.3f} s after our header "
            f"(want {TIMEOUT}..{TIMEOUT + 3} s)",
        )

    def p2_other_messages(self):
        conn = self.accept_planned("P2", 25)
        if conn is None:
            return self.result("P2", False, "node 1 never reconnected")
        conn.send(contact_header("ipn:2.0"), "contact header")
        t_hdr = now()
        messages = itertools.cycle(
            [
                (b"\x20" + sdnv(5), "ACK_SEGMENT 5"),
                (b"\x60" + sdnv(1000), "LENGTH 1000"),
                (b"\x30", "REFUSE_BUNDLE"),
            ]
        )
        for i in range(8):
            conn.hold(t_hdr + 1.5 * (i + 1))
            if conn.closed_at() is not None:
                break
            data, what = next(messages)
            conn.send(data, what)
        conn.hold(t_hdr + 12)
        closed = conn.closed_at()
        kas = conn.count(("keepalive",), t_hdr, t_hdr + 12)
        self.end_planned(conn)
        if closed is not None:
            return self.result(
                "P2",
                False,
                f"node 1 ended the session {closed - t_hdr:.3f} s after our "
                "header although we sent a message every 1.5 s",
            )
        return self.result(
            "P2",
            True,
            "session stayed open for 12 s of ACK/LENGTH/REFUSE without "
            f"keepalives (node 1 sent {kas} keepalives)",
        )

    def p3_slow_segment(self):
        conn = self.accept_planned("P3", 25)
        if conn is None:
            return self.result("P3", False, "node 1 never reconnected")
        conn.send(contact_header("ipn:2.0"), "contact header")
        conn.hold(now() + 0.5)
        length, chunk, step = 20000, 200, 0.1
        conn.send(b"\x13" + sdnv(length), f"DATA_SEGMENT start+end, {length} bytes")
        t_seg = t_end = now()
        sent = 0
        while sent < length and conn.closed_at() is None:
            if not conn.send(bytes(chunk), None):
                break
            t_end = now()
            sent += chunk
            if sent % 4000 == 0:
                log(f"P3 payload sent: {sent} bytes")
            conn.hold(t_seg + step * (sent // chunk))
        log(f"P3 sent {sent} of {length} payload bytes in {t_end - t_seg:.3f} s")
        conn.pace(t_end)
        conn.hold(t_end + 3.2)
        closed = conn.closed_at()
        acks = conn.values("ack")
        ack = conn.first(("ack",))
        self.end_planned(conn)
        if sent < length:
            return self.result(
                "P3",
                False,
                f"node 1 ended the session {(closed or t_end) - t_seg:.3f} s "
                f"into the segment, after {sent} of {length} bytes",
            )
        if closed is not None:
            return self.result(
                "P3",
                False,
                f"node 1 ended the session {closed - t_end:.3f} s after the segment",
            )
        if length not in acks:
            return self.result(
                "P3",
                False,
                f"node 1 acknowledged {acks or 'nothing'}, not {length} bytes",
            )
        return self.result(
            "P3",
            True,
            f"{length} bytes over {t_end - t_seg:.1f} s acknowledged "
            f"{ack[1] - t_end:.3f} s after the last byte; session stayed open",
        )

    def p4_attempt(self, n, offset):
        tag = f"P4.{n}"
        c1 = self.connect_induct(tag + "a")
        if c1 is None:
            return False, "first connection refused"
        c1.send(contact_header("ipn:3.0"), "contact header")
        t1 = now()
        if c1.wait(("header",), t1 + 3) is None:
            c1.close()
            return False, "no contact header on the first connection"
        ka = c1.wait(("keepalive",), t1 + TIMEOUT + 2)
        c1.close()
        if ka is None:
            return False, "no keepalive on the first connection"
        sleep_until(ka[1] + K + offset)
        c2 = self.connect_induct(tag + "b")
        if c2 is None:
            return False, "second connection refused"
        log(
            f"{tag} reconnected {c2.start - ka[1]:.3f} s after node 1's first keepalive"
        )
        c2.send(contact_header("ipn:3.0"), "contact header")
        c2.pace(c2.start)
        hdr = c2.wait(("header",), c2.start + 2)
        if hdr is None:
            c2.close()
            return False, "no contact header within 2 s of reconnecting"
        c2.hold(hdr[1] + 6)
        closed = c2.closed_at()
        kas = c2.count(("keepalive",), hdr[1], hdr[1] + 6)
        c2.close()
        if closed is not None:
            return False, (
                f"node 1 ended the new session {closed - hdr[1]:.3f} s after its header"
            )
        if kas < 2:
            return False, f"node 1 sent {kas} keepalives in 6 s"
        return True, (
            f"header {hdr[1] - c2.start:.3f} s after reconnecting, "
            f"{kas} keepalives in 6 s"
        )

    def p4_chance_reconnect(self):
        ok = True
        for n, offset in enumerate(self.args.offsets, 1):
            good, msg = self.p4_attempt(n, offset)
            ok = self.result(f"P4.{n}", good, f"offset {offset:+.2f} s: {msg}") and ok
            time.sleep(K + 1)
        c = self.connect_induct("P4.final")
        hdr = None if c is None else c.wait(("header",), c.start + 3)
        if c is not None:
            c.close()
        answer = "answered" if hdr else "did not answer"
        ok = (
            self.result(
                "P4.final", hdr is not None, f"node 1 {answer} a fresh connection"
            )
            and ok
        )
        return self.result("P4", ok, "accepted-session reconnects")

    def run(self, phases):
        self.listen()
        if self.args.ready:
            with open(self.args.ready, "w") as f:
                f.write(f"{os.getpid()}\n")
        ok = True
        if "1" in phases:
            ok = self.p1_silent() and ok
        if "2" in phases:
            ok = self.p2_other_messages() and ok
        if "3" in phases:
            ok = self.p3_slow_segment() and ok
        self.listener.close()
        if "4" in phases:
            ok = self.p4_chance_reconnect() and ok
        log("summary:")
        for line in self.results:
            log("  " + line)
        return ok


def watchdog():
    time.sleep(DEADLINE)
    log(f"FAIL: global deadline of {DEADLINE:.0f} s reached")
    os._exit(1)


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--listen-port", type=int, default=4656)
    ap.add_argument("--induct-port", type=int, default=4655)
    ap.add_argument("--phases", default="1234")
    ap.add_argument(
        "--offsets",
        default="-0.5,-0.12,0.12",
        type=lambda v: [float(x) for x in v.split(",")],
        help="P4 reconnect times relative to K s after node "
        "1's first keepalive, comma-separated",
    )
    ap.add_argument("--ready", help="file to write our pid to once listening")
    args = ap.parse_args()
    threading.Thread(target=watchdog, daemon=True).start()
    ok = Peer(args).run(args.phases)
    log(f"peer result: {'PASS' if ok else 'FAIL'}")
    raise SystemExit(0 if ok else 1)


if __name__ == "__main__":
    main()
