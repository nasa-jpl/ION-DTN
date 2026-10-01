"""Generate and send a CCS administrative record based on the given arguments."""

import socket
import sys


def authrec_send(
    source: str, dest: str, induct_host: str, induct_port: int, content: dict[int, list]
) -> int:
    try:
        import cbor2
        from bespokebpv7.block_enum import CRCType
        from bespokebpv7.bpv7 import BPv7
    except Exception as exc:  # noqa: BLE001
        sys.stderr.write(f"SKIP: bespokebpv7 unavailable: {exc}\n")
        return 2

    payload = cbor2.dumps([13, content])

    bundle = BPv7()
    bundle.primary_block.route.source_eid = source
    bundle.primary_block.route.dest_eid = dest
    bundle.primary_block.route.report_to = source
    bundle.primary_block.adu_is_admin = True
    bundle.primary_block.set_creation()
    bundle.primary_block.crc_type = CRCType.CRC16
    bundle.primary_block.update_crc()
    bundle.add_payload_block(payload)

    disposition = next(iter(content))
    seq_id = content[disposition][0][0]
    seq_num = content[disposition][0][1]

    wire = bytes(bundle)
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        sock.sendto(wire, (induct_host, induct_port))
    sys.stderr.write(
        f"injected CCS: {len(wire)} bytes src={source} dest={dest} seqId={seq_id} "
        f"seqNum={seq_num} disp={disposition}\n"
    )

    return 0
