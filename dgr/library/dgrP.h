/*
 * dgrP.h -- private declarations internal to libdgr, exposed only so the
 * inbound-segment parser can be exercised without a live UDP peer.
 */

#ifndef DGRP_H
#define DGRP_H

#include "dgr.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Parses a received DGR/LTP segment in buf[0..length-1].  Returns the segment
 * type the caller must act on -- 3 (red client data, EOB) or 8 (report) -- or
 * 0 when the segment is malformed, unsupported, or ignored.
 *
 * For an acted-on segment it sets the engine and session ids and the header
 * length; a type-8 report also sets *rptSerialNbr; a type-3 data segment also
 * sets *ckptSerialNbr and the offset (*contentOffset) and length
 * (*contentLength) of the client service data within buf, bounded so that
 * *contentOffset + *contentLength never exceeds length.
 */
extern int _dgrParseInboundSegment(char *buf, int length, uvast *engineId,
		uvast *sessionNbr, uvast *ckptSerialNbr, uvast *rptSerialNbr,
		int *headerLength, int *contentOffset, uvast *contentLength);

#ifdef __cplusplus
}
#endif

#endif
