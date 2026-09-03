"""Relay driver for the LTP superseded-report regression test.

Runs two man-in-the-middle relays, one per direction of the link between
node 2 and node 3, and drives a particular LTP exchange through them:

    node 2 --> :2112 --> node 3      watched; nothing is altered
    node 2 <-- :3112 <-- node 3      the completing report is held, and
                                     reports of gaps are injected

The bug being tested is a sender that abandons a session because of a
reception report that a later report has already superseded.  Reaching it
needs two things true at once: the receiver must already hold the whole
block, and the sender must act on a report of gaps at a moment when it has
no checkpoint left to repair one.

The block is delivered without interference, so the receiver takes
delivery and reports complete reception.  That report is held.  Reports of
gaps are then injected, one at a time: each carries a serial number the
sender has not seen and claims only the first half of the block, so the
sender takes the rest to be missing and spends a checkpoint retransmitting
it.  Injection continues until the sender stops issuing new checkpoint
serial numbers, which is how the budget being spent is *observed* rather
than assumed.  That last injection is the one processed with the budget
gone.  The held report is released afterwards.

The size of the sender's checkpoint budget is deliberately never computed
here.  ION derives it from the block size, the expected segment loss rate
and -- once a repair-round limit lands -- a configured number of rounds,
and a test that hard-codes the answer is one release away from passing
while proving nothing.  Watching for the sender to stop minting
checkpoints costs a few seconds and does not care how the number is
reached.

Because exhaustion is observed rather than assumed, the test can also
insist on it: if the sender never stops issuing checkpoints, no
confirmation is printed and the test calls itself inconclusive instead of
passing.
"""

from __future__ import annotations

import argparse
import os
import sys
import threading
import time

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))

import dtnmitm as M  # noqa: E402
from bespokebpv7.segment_enum import LTPSegmentType  # noqa: E402
from bespokebpv7.segments import ReportSegment  # noqa: E402
from bespokebpv7.utils import bundle_converter  # noqa: E402

DATA_LISTEN_PORT = 2112
DATA_FORWARD_PORT = 3113
RPT_LISTEN_PORT = 3112
RPT_FORWARD_PORT = 2113

HOLD_MS = 40000
"""How long the completing report is held.

Long enough to cover the whole injection sequence; the driver finishes
well inside it, so the exact value does not matter.
"""

SETTLE_S = 1.2
"""How long to wait after an injection for the sender to respond."""

MAX_INJECTIONS = 40
"""Refuse to inject forever if the sender never stops minting."""

EXHAUSTED_MARK = "CHECKPOINT BUDGET OBSERVED EXHAUSTED"
"""Printed once the sender stops issuing new checkpoints.

The test greps for this: without it the sequence never happened, and the
result proves nothing either way.
"""


class Observed:
    """What the relays have seen, shared between them and the driver."""

    lock = threading.Lock()
    last_checkpoint: int | None = None
    completing: dict[str, int] | None = None

    @classmethod
    def note_checkpoint(cls, serial: int) -> None:
        """Record the newest checkpoint serial the sender has issued."""
        with cls.lock:
            cls.last_checkpoint = serial

    @classmethod
    def checkpoint(cls) -> int | None:
        """Read the newest checkpoint serial seen.

        Returns:
            The serial, or None if no checkpoint has been seen.

        """
        with cls.lock:
            return cls.last_checkpoint

    @classmethod
    def note_completing(cls, segment: object) -> None:
        """Record the report claiming the whole block, the first time only."""
        with cls.lock:
            if cls.completing is not None:
                return

            cls.completing = {
                "originator": segment.session_originator,  # type: ignore[attr-defined]
                "session": segment.session_number,  # type: ignore[attr-defined]
                "serial": segment.report_serial_number,  # type: ignore[attr-defined]
                "checkpoint": segment.checkpoint_serial_number,  # type: ignore[attr-defined]
                "lower": segment.lower_bound,  # type: ignore[attr-defined]
                "upper": segment.upper_bound,  # type: ignore[attr-defined]
            }

    @classmethod
    def completing_report(cls) -> dict[str, int] | None:
        """Read the recorded completing report.

        Returns:
            Its fields, or None if it has not been seen yet.

        """
        with cls.lock:
            return cls.completing


def watch_checkpoints(view: M.View) -> M.Verdict:
    """Note the serial of a checkpoint-bearing data segment.

    Returns:
        A verdict that forwards the segment unchanged.

    """
    serial = view.attr("checkpoint_serial_number")
    if serial:
        Observed.note_checkpoint(serial)

    return M.Verdict()


def hold_completing(view: M.View) -> M.Verdict:
    """Hold the report in which the receiver claims the whole block.

    Returns:
        A verdict holding the report until the sequence is done.

    """
    Observed.note_completing(view.segment)
    return M.Verdict(
        act=M.Act.DELAY,
        delay_ms=HOLD_MS,
        note="completing report held while the sender's budget is spent",
    )


def gapped_report(base: dict[str, int], serial: int, checkpoint: int) -> bytes:
    """Build a report of gaps for the session the receiver just completed.

    Claims only the first half of the block, so the sender takes the rest
    to be missing.  Claims accumulate at the sender, so every injection
    claims the same half and their union stays incomplete.

    Returns:
        The serialized report segment.

    """
    span = base["upper"] - base["lower"]
    return bundle_converter.unstructure(
        ReportSegment(
            session_originator=base["originator"],
            session_number=base["session"],
            report_serial_number=serial,
            checkpoint_serial_number=checkpoint,
            lower_bound=base["lower"],
            upper_bound=base["upper"],
            reception_claims=[(0, max(1, span // 2))],
        )
    )


def complete_report(base: dict[str, int], serial: int) -> bytes:
    """Build the report the receiver sent, claiming the whole block.

    Sent once the budget is spent, so that the sender learns the block
    arrived without waiting for the held original.  The original follows
    later carrying this same serial number and is discarded as a report
    already handled, which is exactly right: this is that report.

    Returns:
        The serialized report segment.

    """
    return bundle_converter.unstructure(
        ReportSegment(
            session_originator=base["originator"],
            session_number=base["session"],
            report_serial_number=serial,
            checkpoint_serial_number=base["checkpoint"],
            lower_bound=base["lower"],
            upper_bound=base["upper"],
            reception_claims=[(0, base["upper"] - base["lower"])],
        )
    )


def spend_the_budget(reports: M.Mitm, deadline: float) -> bool:
    """Inject reports of gaps until the sender stops issuing checkpoints.

    Returns:
        True if the sender was observed to stop, meaning its budget is
        spent and the last report of gaps met the condition this test
        exists to exercise.

    """
    base = None
    while base is None and time.monotonic() < deadline:
        time.sleep(0.2)
        base = Observed.completing_report()

    if base is None:
        print("receiver never claimed the whole block; nothing to do", flush=True)
        return False

    print(
        f"receiver holds the block (session {base['session']}, "
        f"report {base['serial']}); spending the sender's checkpoints",
        flush=True,
    )

    # Serial numbers below the completing report's, descending, so each is
    # one the sender has not seen.  A serial it has already handled is
    # discarded as redundant and would spend nothing.
    serial = base["serial"] - 1

    for attempt in range(1, MAX_INJECTIONS + 1):
        if time.monotonic() >= deadline:
            print("ran out of time before the budget was spent", flush=True)
            return False

        before = Observed.checkpoint()
        if before is None:
            print("no checkpoint seen yet; waiting", flush=True)
            time.sleep(SETTLE_S)
            continue

        reports.inject(gapped_report(base, serial, before))
        serial -= 1
        time.sleep(SETTLE_S)
        after = Observed.checkpoint()

        if after == before:
            # The sender answered a report of gaps without issuing a new
            # checkpoint, so it had none left to issue: that report was
            # processed with the budget gone.
            print(
                f"{EXHAUSTED_MARK} after {attempt} injection(s); "
                f"checkpoint serial stayed at {after}",
                flush=True,
            )

            # Tell the sender the block arrived, which is what the held
            # report says.  A sender that kept the session alive finishes
            # it here; one that abandoned it on the report of gaps has
            # nothing left to finish, and BP re-forwards the block.
            reports.inject(complete_report(base, base["serial"]))
            print("completing report delivered to the sender", flush=True)
            return True

        print(
            f"  injection {attempt}: checkpoint {before} -> {after}, "
            "budget not spent yet",
            flush=True,
        )

    print("sender never stopped issuing checkpoints", flush=True)
    return False


def main() -> int:
    """Run the relays and drive the exchange.

    Returns:
        Zero on a clean shutdown.

    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--duration",
        type=int,
        default=90,
        help="seconds to keep relaying before exiting",
    )
    parser.add_argument(
        "--trace-dir",
        default=".",
        help="directory to write relay traces into",
    )
    parser.add_argument(
        "--passthrough",
        action="store_true",
        help="relay faithfully, injecting nothing; the control case",
    )
    args = parser.parse_args()

    data_rules: list[M.Rule] = []
    rpt_rules: list[M.Rule] = []
    if not args.passthrough:
        data_rules = [M.rule(M.seg_type(*M.RED_DATA), watch_checkpoints)]
        rpt_rules = [
            # Every completing report is held, not just the first.  The
            # receiver re-sends one each time it is asked again, and a
            # single one getting through tells the sender the block
            # arrived, ending the session before the sequence is set up.
            M.rule(
                M.all_of(
                    M.seg_type(LTPSegmentType.REPORT),
                    M.report_complete(),
                ),
                hold_completing,
            )
        ]

    data = M.Mitm(
        DATA_LISTEN_PORT,
        DATA_FORWARD_PORT,
        rules=data_rules,
        trace_path=os.path.join(args.trace_dir, "mitm-data.jsonl"),
        label="data",
    )
    reports = M.Mitm(
        RPT_LISTEN_PORT,
        RPT_FORWARD_PORT,
        rules=rpt_rules,
        trace_path=os.path.join(args.trace_dir, "mitm-report.jsonl"),
        label="report",
    )

    with data, reports:
        print(
            f"relaying: data {DATA_LISTEN_PORT}->{DATA_FORWARD_PORT}, "
            f"reports {RPT_LISTEN_PORT}->{RPT_FORWARD_PORT}",
            flush=True,
        )

        started = time.monotonic()
        if not args.passthrough:
            spend_the_budget(reports, started + args.duration - 10)

        remaining = args.duration - (time.monotonic() - started)
        if remaining > 0:
            time.sleep(remaining)

    print(f"data relay: {data.counts}", flush=True)
    print(f"report relay: {reports.counts}", flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
