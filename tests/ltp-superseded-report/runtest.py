"""Relay driver for the LTP superseded-report regression test.

Runs two man-in-the-middle relays, one per direction of the link between
node 2 and node 3, for as long as the test needs them:

    node 2 --> :2112 --> node 3      red data, one segment discarded
    node 2 <-- :3112 <-- node 3      reports, one crafted stale copy

The scenario being staged is a sender that gives up on a session because
of a reception report that a later report has already superseded.  It
needs two things to be true at the same time, and each relay supplies
one of them.

On the outbound path, one red data segment is discarded.  That opens a
gap, so the receiver's first report describes gaps rather than complete
reception, and the sender spends a second checkpoint repairing it.  With
a red part this small the sender is allowed only two checkpoints, so
after the repair it has none left.

On the return path, the report that finally claims the whole red part is
held briefly, and a copy of it with its claims cut back to the first
extent -- a report that looks exactly like the earlier, superseded view
of reception -- is sent ahead of it.  The sender therefore processes a
report describing a gap at the moment it has no checkpoint left to
repair one, while the report that says everything arrived is right
behind it.

That is the whole bug: cancelling on the stale report discards a
transfer that succeeded, and BP re-forwards, so the receiver is handed
the block a second time.  Crafting the stale report rather than trying
to provoke one by timing makes the sequence deterministic.
"""

from __future__ import annotations

import argparse
import os
import sys
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

DROP_ORDINAL = 2
"""Which red data segment to discard, counting from one."""

HOLD_MS = 400
"""How long to hold the completing report behind its stale copy."""


def stale_copy(view: M.View) -> M.Verdict:
    """Send a superseded view of reception ahead of the real report.

    Returns:
        A verdict holding the real report and injecting the stale copy.

    """
    seg = view.segment

    # A completing report carries exactly one claim spanning its whole
    # scope, so the copy has to shorten that claim rather than drop any:
    # keeping "the first claim" would keep the only claim, and the copy
    # would assert complete reception just like the original.  Claiming
    # half the extent leaves the rest of the scope unaccounted for, which
    # is what makes this read as the superseded view of reception.
    offset, length = seg.reception_claims[0]
    stale_claims = [(offset, max(1, length // 2))]

    stale = ReportSegment(
        session_originator=seg.session_originator,
        session_number=seg.session_number,
        report_serial_number=seg.report_serial_number,
        checkpoint_serial_number=seg.checkpoint_serial_number,
        lower_bound=seg.lower_bound,
        upper_bound=seg.upper_bound,
        reception_claims=stale_claims,
    )

    return M.Verdict(
        act=M.Act.DELAY,
        delay_ms=HOLD_MS,
        note="completing report held behind a crafted stale copy",
        extra=[(bundle_converter.unstructure(stale), 0)],
    )


def main() -> int:
    """Run both relays until asked to stop.

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
        help="relay faithfully, applying no rules; the control case",
    )
    args = parser.parse_args()

    if args.passthrough:
        data_rules: list[M.Rule] = []
        rpt_rules: list[M.Rule] = []
    else:
        data_rules = [
            M.rule(
                M.seg_type(*M.RED_DATA),
                M.Verdict(act=M.Act.DROP, note="opening a reception gap"),
                nth=[DROP_ORDINAL],
            )
        ]
        rpt_rules = [
            M.rule(
                M.all_of(
                    M.seg_type(LTPSegmentType.REPORT),
                    M.report_complete(),
                ),
                stale_copy,
                nth=[1],
            )
        ]

    data_trace = os.path.join(args.trace_dir, "mitm-data.jsonl")
    rpt_trace = os.path.join(args.trace_dir, "mitm-report.jsonl")

    data = M.Mitm(
        DATA_LISTEN_PORT,
        DATA_FORWARD_PORT,
        rules=data_rules,
        trace_path=data_trace,
        label="data",
    )
    report = M.Mitm(
        RPT_LISTEN_PORT,
        RPT_FORWARD_PORT,
        rules=rpt_rules,
        trace_path=rpt_trace,
        label="report",
    )

    with data, report:
        print(
            f"relaying: data {DATA_LISTEN_PORT}->{DATA_FORWARD_PORT}, "
            f"reports {RPT_LISTEN_PORT}->{RPT_FORWARD_PORT}",
            flush=True,
        )
        time.sleep(args.duration)

    print(f"data relay: {data.counts}", flush=True)
    print(f"report relay: {report.counts}", flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
