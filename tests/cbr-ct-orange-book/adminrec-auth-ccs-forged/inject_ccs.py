#!/usr/bin/env python3
"""Forge a CBR compressed custody signal (CCS) with an arbitrary source EID and
send it to an ION udpcli induct, to test admin-record source authentication.

The CCS administrative record is [13, {disposition: [[seqId, seqNum, length]]}],
matching cbr_encodeCcs()/cbr_decodeBundleSequence(): a 3-element bundle sequence
(no block-source AEID, so the receiver substitutes its own admin EID as the
custody-key source).

Exit 2 (skip) if bespokebpv7 is unavailable.
"""

import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from authrec_send import authrec_send


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--induct-host", default="127.0.0.1")
    ap.add_argument("--induct-port", type=int, required=True)
    ap.add_argument("--source", required=True, help="forged primary-block source EID")
    ap.add_argument(
        "--dest", required=True, help="receiver admin endpoint, e.g. ipn:1.0"
    )
    ap.add_argument("--seq-id", type=int, required=True)
    ap.add_argument("--seq-num", type=int, required=True)
    ap.add_argument("--length", type=int, default=1)
    ap.add_argument(
        "--disposition", type=int, default=1, help="1 = custody accepted, -1 = refused"
    )
    args = ap.parse_args()

    # Admin record: [13 (CCS), {disposition: [[seqId, seqNum, length]]}]
    content = {args.disposition: [[args.seq_id, args.seq_num, args.length]]}
    result = authrec_send(
        args.source, args.dest, args.induct_host, args.induct_port, content
    )

    return result


if __name__ == "__main__":
    sys.exit(main())
