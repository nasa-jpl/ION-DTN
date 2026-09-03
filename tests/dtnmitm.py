"""DTN man-in-the-middle relay for ION tests.

A UDP relay that sits between two ION link services, dissects each
datagram, and decides what to do with it.  It exists because a lossless
loopback link never exercises the paths that matter most: LTP checkpoint
retransmission, report loss, reordering of a report with respect to the
data that supersedes it, and a receiver's response to a duplicated or
altered bundle.

Two layers are dissected, and rules may match on either:

  * the LTP segment -- type, session, checkpoint and report serial
    numbers, reception claims;
  * the BPv7 bundle carried by a data segment, when there is one --
    endpoints, lifetime, flags, extension blocks, payload.

Parsing and serialization are bespokebpv7's; this module only decides.
Because a rule may both match on parsed fields and hand back rewritten
bytes, the same machinery covers monitoring (record everything, change
nothing), loss and reordering, and bundle-level alteration such as
corrupting a CRC to defeat a BIB, stripping or inserting an extension
block, rewriting a lifetime, or delivering a bundle twice.

Every datagram produces one JSON record on the trace, whether or not a
rule touched it, so a test can assert against the trace instead of
scraping daemon logs.

Note on addresses: everything here is IPv4 and explicit.  ION's udplsi
binds the IPv6 loopback when its endpoint is given as "localhost", and a
relay listening on IPv4 then receives nothing at all, silently.  Give
link services 127.0.0.1 addresses in the ltprc when using this module.
"""

from __future__ import annotations

import heapq
import itertools
import json
import socket
import threading
import time
from collections.abc import Callable, Iterable, Sequence
from dataclasses import dataclass
from enum import Enum
from typing import Any

from bespokebpv7.ltp import LTP  # type: ignore[import-untyped]
from bespokebpv7.segment_enum import LTPSegmentType  # type: ignore[import-untyped]

RED_DATA = (
    LTPSegmentType.DATA_RED,
    LTPSegmentType.DATA_RED_CP,
    LTPSegmentType.DATA_RED_CP_EORP,
    LTPSegmentType.DATA_RED_CP_EORP_EOB,
)
"""Every red (reliably transmitted) data segment type."""


class Act(Enum):
    """What is to become of a datagram."""

    PASS = "pass"
    DROP = "drop"
    DELAY = "delay"
    REWRITE = "rewrite"


@dataclass
class Verdict:
    """A rule's decision about one datagram.

    ``delay_ms`` postpones the datagram without holding up anything
    received after it.  ``payload`` replaces the bytes on the wire.
    ``extra`` are additional datagrams to emit, each with its own delay,
    which is how a test injects a segment that was never sent -- a
    duplicate bundle, or a report crafted to look stale.
    """

    act: Act = Act.PASS
    delay_ms: int = 0
    payload: bytes | None = None
    extra: Sequence[tuple[bytes, int]] = ()
    note: str = ""


@dataclass
class View:
    """A dissected datagram: raw bytes plus whatever could be parsed."""

    raw: bytes
    ltp: LTP | None = None

    @property
    def segment(self) -> Any:
        """The typed LTP segment, or None if it would not parse.

        Returns:
            The bespokebpv7 segment object, or None.

        """
        return None if self.ltp is None else self.ltp.segment

    @property
    def segment_type(self) -> LTPSegmentType | None:
        """The LTP segment type.

        Returns:
            The segment type, or None if the datagram would not parse.

        """
        seg = self.segment
        return None if seg is None else seg.segment_type

    @property
    def bundle(self) -> Any:
        """The BPv7 bundle carried by this segment, if any.

        Returns:
            The bespokebpv7 BPv7 object, or None.

        """
        return None if self.ltp is None else self.ltp.bpv7

    def attr(self, name: str, default: Any = None) -> Any:
        """Read a field from the segment without caring which type it is.

        Returns:
            The named segment attribute, or ``default``.

        """
        return getattr(self.segment, name, default)

    def summary(self) -> dict[str, Any]:
        """Describe this datagram for the trace.

        Returns:
            A JSON-serializable dict of the interesting fields.

        """
        out: dict[str, Any] = {"len": len(self.raw)}

        seg_type = self.segment_type
        if seg_type is None:
            out["parsed"] = False
            return out

        out["seg_type"] = seg_type.name
        for name in (
            "session_number",
            "checkpoint_serial_number",
            "report_serial_number",
            "client_offset",
            "client_length",
            "lower_bound",
            "upper_bound",
        ):
            value = self.attr(name)
            if value is not None:
                out[name] = value

        claims = self.attr("reception_claims")
        if claims is not None:
            out["claims"] = [list(claim) for claim in claims]
            complete = self.attr("is_complete_for_scope")
            if callable(complete):
                out["report_complete"] = complete()

        bundle = self.bundle
        if bundle is not None:
            route = bundle.primary_block.route
            out["bundle"] = {
                "src": str(route.source_eid),
                "dst": str(route.dest_eid),
            }

        return out


Matcher = Callable[[View], bool]
Rule = Callable[[View], Verdict | None]


def seg_type(*types: LTPSegmentType) -> Matcher:
    """Match segments of any of the given types.

    Returns:
        A matcher over the segment type.

    """
    wanted = set(types)
    return lambda view: view.segment_type in wanted


def report_gapped() -> Matcher:
    """Match a report whose claims do not cover its whole scope.

    Returns:
        A matcher that selects reports describing gaps.

    """

    def match(view: View) -> bool:
        complete = view.attr("is_complete_for_scope")
        return callable(complete) and not complete()

    return match


def report_complete() -> Matcher:
    """Match a report asserting complete reception of its scope.

    Returns:
        A matcher that selects completing reports.

    """

    def match(view: View) -> bool:
        complete = view.attr("is_complete_for_scope")
        return callable(complete) and bool(complete())

    return match


def field_is(name: str, value: Any) -> Matcher:
    """Match segments whose named field equals a value.

    Returns:
        A matcher over one segment field.

    """
    return lambda view: view.attr(name) == value


def all_of(*matchers: Matcher) -> Matcher:
    """Match when every given matcher matches.

    Returns:
        The conjunction of the matchers.

    """
    return lambda view: all(m(view) for m in matchers)


def rule(
    matcher: Matcher,
    verdict: Verdict | Callable[[View], Verdict],
    *,
    nth: Iterable[int] | None = None,
) -> Rule:
    """Build a rule from a matcher and what to do when it matches.

    ``nth`` restricts the rule to particular occurrences among the
    datagrams the matcher selects, counted from one.  This is how a test
    names "the second red data segment" without knowing anything about
    the traffic around it.

    Returns:
        A rule usable by ``Mitm``.

    """
    wanted = None if nth is None else set(nth)
    seen = itertools.count(1)

    def apply(view: View) -> Verdict | None:
        if not matcher(view):
            return None

        ordinal = next(seen)
        if wanted is not None and ordinal not in wanted:
            return None

        return verdict(view) if callable(verdict) else verdict

    return apply


class Mitm:
    """Relays one direction of a link, applying rules to each datagram."""

    def __init__(
        self,
        listen_port: int,
        forward_port: int,
        *,
        rules: Sequence[Rule] = (),
        trace_path: str | None = None,
        host: str = "127.0.0.1",
        label: str = "mitm",
    ) -> None:
        """Prepare the relay; call ``start`` to run it."""
        self.listen_port = listen_port
        self.forward_port = forward_port
        self.host = host
        self.rules = list(rules)
        self.label = label

        self._trace = open(trace_path, "a", buffering=1) if trace_path else None
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self._sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self._sock.bind((host, listen_port))
        self._sock.settimeout(0.25)

        self._pending: list[tuple[float, int, bytes]] = []
        self._pending_lock = threading.Lock()
        self._seq = 0
        self._running = threading.Event()
        self._threads: list[threading.Thread] = []
        self.counts: dict[str, int] = {}

    # -- lifecycle ----------------------------------------------------

    def start(self) -> None:
        """Begin relaying, in background threads."""
        self._running.set()
        for target in (self._receive_loop, self._release_loop):
            thread = threading.Thread(target=target, daemon=True)
            thread.start()
            self._threads.append(thread)

    def stop(self) -> None:
        """Stop relaying and flush the trace."""
        self._running.clear()
        for thread in self._threads:
            thread.join(timeout=2.0)

        self._sock.close()
        if self._trace is not None:
            self._trace.close()

    def __enter__(self) -> Mitm:
        """Start the relay.

        Returns:
            This relay.

        """
        self.start()
        return self

    def __exit__(self, *_: object) -> None:
        """Stop the relay."""
        self.stop()

    # -- driving from outside ------------------------------------------

    def inject(self, payload: bytes, delay_ms: int = 0) -> None:
        """Emit a datagram of the caller's own making.

        For tests that must do more than react to what passes by: a
        segment that was never sent, at a moment of the test's choosing.
        Safe to call from another thread.
        """
        if delay_ms > 0:
            self._schedule(payload, delay_ms)
        else:
            self._send(payload)

        self.counts["inject"] = self.counts.get("inject", 0) + 1
        if self._trace is not None:
            self._trace.write(
                json.dumps(
                    {
                        "t": round(time.time(), 6),
                        "relay": self.label,
                        "act": "inject",
                        "len": len(payload),
                        "delay_ms": delay_ms,
                    }
                )
                + "\n"
            )

    # -- internals ----------------------------------------------------

    def _dissect(self, raw: bytes) -> View:
        try:
            return View(raw=raw, ltp=LTP(raw))
        except Exception:  # noqa: BLE001 - a test relay must never die on a bad datagram
            return View(raw=raw)

    def _record(self, view: View, verdict: Verdict) -> None:
        self.counts[verdict.act.value] = self.counts.get(verdict.act.value, 0) + 1
        if self._trace is None:
            return

        entry: dict[str, Any] = {
            "t": round(time.time(), 6),
            "relay": self.label,
            "act": verdict.act.value,
        }
        if verdict.delay_ms:
            entry["delay_ms"] = verdict.delay_ms
        if verdict.note:
            entry["note"] = verdict.note

        entry.update(view.summary())
        self._trace.write(json.dumps(entry) + "\n")

    def _schedule(self, payload: bytes, delay_ms: int) -> None:
        due = time.monotonic() + (delay_ms / 1000.0)
        with self._pending_lock:
            self._seq += 1
            heapq.heappush(self._pending, (due, self._seq, payload))

    def _send(self, payload: bytes) -> None:
        self._sock.sendto(payload, (self.host, self.forward_port))

    def _receive_loop(self) -> None:
        while self._running.is_set():
            try:
                raw, _ = self._sock.recvfrom(65535)
            except (TimeoutError, OSError):
                continue

            view = self._dissect(raw)

            verdict = Verdict()
            for candidate in self.rules:
                decided = candidate(view)
                if decided is not None:
                    verdict = decided
                    break

            self._record(view, verdict)

            payload = raw if verdict.payload is None else verdict.payload
            if verdict.act is not Act.DROP:
                if verdict.delay_ms > 0:
                    self._schedule(payload, verdict.delay_ms)
                else:
                    self._send(payload)

            for extra_payload, extra_delay in verdict.extra:
                if extra_delay > 0:
                    self._schedule(extra_payload, extra_delay)
                else:
                    self._send(extra_payload)

    def _release_loop(self) -> None:
        """Send postponed datagrams when due.

        Postponed datagrams are held in a heap keyed on due time, so one
        that is held back does not delay anything received after it.
        Holding them in arrival order instead would preserve the very
        ordering a delay is usually meant to disturb.
        """
        while self._running.is_set() or self._pending:
            now = time.monotonic()
            ready: list[bytes] = []

            with self._pending_lock:
                while self._pending and self._pending[0][0] <= now:
                    ready.append(heapq.heappop(self._pending)[2])

            for payload in ready:
                self._send(payload)

            time.sleep(0.005)


def read_trace(path: str) -> list[dict[str, Any]]:
    """Read a trace file written by ``Mitm``.

    Returns:
        One dict per recorded datagram, in order.

    """
    with open(path) as handle:
        return [json.loads(line) for line in handle if line.strip()]
