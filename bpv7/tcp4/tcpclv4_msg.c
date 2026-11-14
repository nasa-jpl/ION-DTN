/*
	tcpclv4_msg.c:	TCPCLv4 message encode/decode library (RFC 9174).

			Stateless pure functions for parsing and
			serializing TCPCLv4 protocol messages.
			No I/O, no state - just wire format conversion.

			All multi-byte fields are network byte order
			(big-endian) per RFC 9174.

	Author: iondev33

	Copyright (c) 2026, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
								*/

#include "tcpclv4_msg.h"

#include <stdlib.h>
#include <string.h>

/*	==============================================================
 *	BYTE ORDER HELPERS
 *	==============================================================
 */

static uint16_t read_u16(const uint8_t *buf)
{
	return ((uint16_t) buf[0] << 8) | (uint16_t) buf[1];
}

static uint32_t read_u32(const uint8_t *buf)
{
	return ((uint32_t) buf[0] << 24) | ((uint32_t) buf[1] << 16)
		| ((uint32_t) buf[2] << 8) | (uint32_t) buf[3];
}

static uint64_t read_u64(const uint8_t *buf)
{
	return ((uint64_t) read_u32(buf) << 32) | (uint64_t) read_u32(buf + 4);
}

static void write_u16(uint8_t *buf, uint16_t val)
{
	buf[0] = (uint8_t) (val >> 8);
	buf[1] = (uint8_t) val;
}

static void write_u32(uint8_t *buf, uint32_t val)
{
	buf[0] = (uint8_t) (val >> 24);
	buf[1] = (uint8_t) (val >> 16);
	buf[2] = (uint8_t) (val >> 8);
	buf[3] = (uint8_t) val;
}

static void write_u64(uint8_t *buf, uint64_t val)
{
	write_u32(buf, (uint32_t) (val >> 32));
	write_u32(buf + 4, (uint32_t) val);
}

/*	==============================================================
 *	DECODE FUNCTIONS
 *	==============================================================
 */

/*	Contact Header (§4.2): magic(4) + version(1) + flags(1)	*/

int tcpclv4_decode_contact_header(const uint8_t *buf, size_t len,
				   tcpclv4_contact_header_t *out)
{
	if (buf == NULL || out == NULL
	|| len < TCPCLV4_CONTACT_HEADER_LEN)
	{
		return -1;	/* Insufficient data */
	}

	if (read_u32(buf) != TCPCLV4_CONTACT_MAGIC)
	{
		return -1;	/* Invalid magic */
	}

	out->version = buf[4];
	out->flags = buf[5];

	if (out->version != TCPCLV4_PROTOCOL_VERSION)
	{
		return -1;	/* Unsupported protocol version */
	}

	return 0;
}

/*	SESS_INIT (§4.7): type(1) + keepalive(2) + seg_mru(8)
 *	+ xfer_mru(8) + node_id_len(2) + node_id
 *	+ ext_items_len(4) + ext_items				*/

int tcpclv4_decode_sess_init(const uint8_t *buf, size_t len,
			      tcpclv4_sess_init_t *out)
{
	const uint8_t	*p;
	size_t		remaining;
	uint32_t	ext_items_len;

	if (buf == NULL || out == NULL)
	{
		return -1;
	}

	if (len < 25)	/* 1 + 2 + 8 + 8 + 2 + 0 (node_id) + 4 */
	{
		return -1;
	}

	p = buf;
	remaining = len;

	if (*p != TCPCLV4_MSG_SESS_INIT)
	{
		return -1;
	}

	p++;
	remaining--;

	/* Keepalive interval (2 bytes) */
	out->keepalive_interval = read_u16(p);
	p += 2;
	remaining -= 2;

	/* Segment MRU (8 bytes) */
	out->segment_mru = read_u64(p);
	p += 8;
	remaining -= 8;

	/* Transfer MRU (8 bytes) */
	out->transfer_mru = read_u64(p);
	p += 8;
	remaining -= 8;

	/* Node ID length (2 bytes) */
	out->node_id_len = read_u16(p);
	p += 2;
	remaining -= 2;

	/* Node ID string */
	if (remaining < (size_t) out->node_id_len + 4)
	{
		return -1;	/* Insufficient data */
	}

	out->node_id = malloc((size_t) out->node_id_len + 1);
	if (out->node_id == NULL)
	{
		return -1;
	}

	memcpy(out->node_id, p, out->node_id_len);
	out->node_id[out->node_id_len] = '\0';	/* NULL-terminate */
	p += out->node_id_len;
	remaining -= out->node_id_len;

	/* Session extension items length (4 bytes) */
	ext_items_len = read_u32(p);
	p += 4;
	remaining -= 4;

	if (remaining != ext_items_len)
	{
		free(out->node_id);
		out->node_id = NULL;
		return -1;	/* Length mismatch */
	}

	out->extensions_len = ext_items_len;
	if (ext_items_len > 0)
	{
		out->extensions = malloc(ext_items_len);
		if (out->extensions == NULL)
		{
			free(out->node_id);
			out->node_id = NULL;
			return -1;
		}

		memcpy(out->extensions, p, ext_items_len);
	}
	else
	{
		out->extensions = NULL;
	}

	return 0;
}

/*	SESS_TERM (§6.1): type(1) + flags(1) + reason(1)		*/

int tcpclv4_decode_sess_term(const uint8_t *buf, size_t len,
			      tcpclv4_sess_term_t *out)
{
	if (buf == NULL || out == NULL || len < 3)
	{
		return -1;
	}

	if (buf[0] != TCPCLV4_MSG_SESS_TERM)
	{
		return -1;
	}

	out->flags = buf[1];
	out->reason_code = buf[2];

	return 0;
}

/*	XFER_SEGMENT (§5.2.2): type(1) + flags(1) + xfer_id(8)
 *	+ [ext_len(4) + ext, START only] + data_len(8) + data	*/

int tcpclv4_decode_xfer_segment(const uint8_t *buf, size_t len,
				 tcpclv4_xfer_segment_t *out)
{
	const uint8_t	*p;
	size_t		remaining;
	uint32_t	ext_items_len;
	uint64_t	data_len;

	if (buf == NULL || out == NULL)
	{
		return -1;
	}

	if (len < 18)	/* type(1) + flags(1) + xfer_id(8) + data_len(8) */
	{
		return -1;
	}

	p = buf;
	remaining = len;

	if (*p != TCPCLV4_MSG_XFER_SEGMENT)
	{
		return -1;
	}

	p++;
	remaining--;

	/* Flags */
	out->flags = *p;
	p++;
	remaining--;

	/* Transfer ID */
	out->transfer_id = read_u64(p);
	p += 8;
	remaining -= 8;

	/* Transfer extension items (present on START segments only) */
	out->extensions = NULL;
	out->extensions_len = 0;
	if (out->flags & TCPCLV4_XFER_FLAG_START)
	{
		if (remaining < 4)
		{
			return -1;
		}

		ext_items_len = read_u32(p);
		p += 4;
		remaining -= 4;

		if (ext_items_len > 0)
		{
			if (remaining < ext_items_len)
			{
				return -1;
			}

			out->extensions = malloc(ext_items_len);
			if (out->extensions == NULL)
			{
				return -1;
			}

			memcpy(out->extensions, p, ext_items_len);
			out->extensions_len = ext_items_len;
			p += ext_items_len;
			remaining -= ext_items_len;
		}
	}

	/* Data length (8 bytes) */
	if (remaining < 8)
	{
		free(out->extensions);
		out->extensions = NULL;
		return -1;
	}

	data_len = read_u64(p);
	p += 8;
	remaining -= 8;

	if (remaining != data_len)
	{
		free(out->extensions);
		out->extensions = NULL;
		return -1;	/* Length mismatch */
	}

	/* Payload data (points into original buffer) */
	out->data = p;
	out->data_len = data_len;

	return 0;
}

/*	XFER_ACK (§5.2.3): type(1) + flags(1) + xfer_id(8) + ack_len(8)	*/

int tcpclv4_decode_xfer_ack(const uint8_t *buf, size_t len,
			     tcpclv4_xfer_ack_t *out)
{
	if (buf == NULL || out == NULL || len < 18)
	{
		return -1;
	}

	if (buf[0] != TCPCLV4_MSG_XFER_ACK)
	{
		return -1;
	}

	out->flags = buf[1];
	out->transfer_id = read_u64(buf + 2);
	out->ack_len = read_u64(buf + 10);

	return 0;
}

/*	XFER_REFUSE (§5.2.3): type(1) + reason(1) + xfer_id(8)		*/

int tcpclv4_decode_xfer_refuse(const uint8_t *buf, size_t len,
				tcpclv4_xfer_refuse_t *out)
{
	if (buf == NULL || out == NULL || len < 10)
	{
		return -1;
	}

	if (buf[0] != TCPCLV4_MSG_XFER_REFUSE)
	{
		return -1;
	}

	out->reason_code = buf[1];
	out->transfer_id = read_u64(buf + 2);

	return 0;
}

/*	MSG_REJECT (§5.1.1): type(1) + reason(1) + rejected_hdr(1)	*/

int tcpclv4_decode_msg_reject(const uint8_t *buf, size_t len,
			       tcpclv4_msg_reject_t *out)
{
	if (buf == NULL || out == NULL || len < 3)
	{
		return -1;
	}

	if (buf[0] != TCPCLV4_MSG_MSG_REJECT)
	{
		return -1;
	}

	out->reason_code = buf[1];
	out->rejected_msg_header = buf[2];

	return 0;
}

/*	==============================================================
 *	ENCODE FUNCTIONS
 *	==============================================================
 */

/*	Contact Header (§4.2): magic(4) + version(1) + flags(1)	*/

int tcpclv4_encode_contact_header(const tcpclv4_contact_header_t *msg,
				   uint8_t *buf, size_t buf_cap,
				   size_t *out_len)
{
	if (msg == NULL || buf == NULL || out_len == NULL
	|| buf_cap < TCPCLV4_CONTACT_HEADER_LEN)
	{
		return -1;
	}

	write_u32(buf, TCPCLV4_CONTACT_MAGIC);
	buf[4] = TCPCLV4_PROTOCOL_VERSION;
	buf[5] = msg->flags;
	*out_len = TCPCLV4_CONTACT_HEADER_LEN;

	return 0;
}

/*	SESS_INIT (§4.7)						*/

int tcpclv4_encode_sess_init(const tcpclv4_sess_init_t *msg,
			      uint8_t *buf, size_t buf_cap,
			      size_t *out_len)
{
	uint8_t	*p;
	size_t	required;

	if (msg == NULL || buf == NULL || out_len == NULL)
	{
		return -1;
	}

	required = 1 + 2 + 8 + 8 + 2 + msg->node_id_len + 4;
	if (buf_cap < required || msg->extensions_len > buf_cap - required)
	{
		return -1;
	}

	p = buf;

	*p++ = TCPCLV4_MSG_SESS_INIT;
	write_u16(p, msg->keepalive_interval);
	p += 2;
	write_u64(p, msg->segment_mru);
	p += 8;
	write_u64(p, msg->transfer_mru);
	p += 8;
	write_u16(p, msg->node_id_len);
	p += 2;

	memcpy(p, msg->node_id, msg->node_id_len);
	p += msg->node_id_len;

	write_u32(p, msg->extensions_len);
	p += 4;

	if (msg->extensions_len > 0)
	{
		memcpy(p, msg->extensions, msg->extensions_len);
		p += msg->extensions_len;
	}

	*out_len = p - buf;

	return 0;
}

/*	SESS_TERM (§6.1): type(1) + flags(1) + reason(1)		*/

int tcpclv4_encode_sess_term(const tcpclv4_sess_term_t *msg,
			      uint8_t *buf, size_t buf_cap,
			      size_t *out_len)
{
	if (msg == NULL || buf == NULL || out_len == NULL || buf_cap < 3)
	{
		return -1;
	}

	buf[0] = TCPCLV4_MSG_SESS_TERM;
	buf[1] = msg->flags;
	buf[2] = msg->reason_code;

	*out_len = 3;

	return 0;
}

/*	XFER_SEGMENT (§5.2.2)						*/

int tcpclv4_encode_xfer_segment(const tcpclv4_xfer_segment_t *msg,
				 uint8_t *buf, size_t buf_cap,
				 size_t *out_len)
{
	uint8_t	*p;
	size_t	required;

	if (msg == NULL || buf == NULL || out_len == NULL)
	{
		return -1;
	}

	/*	Extension items may only accompany a START segment.	*/

	if (msg->extensions_len > 0
	&& (msg->flags & TCPCLV4_XFER_FLAG_START) == 0)
	{
		return -1;
	}

	required = 1 + 1 + 8 + 8;
	if (msg->flags & TCPCLV4_XFER_FLAG_START)
	{
		required += 4;
	}

	if (buf_cap < required || msg->extensions_len > buf_cap - required)
	{
		return -1;
	}

	required += msg->extensions_len;
	if (msg->data_len > buf_cap - required)
	{
		return -1;
	}

	p = buf;

	*p++ = TCPCLV4_MSG_XFER_SEGMENT;
	*p++ = msg->flags;
	write_u64(p, msg->transfer_id);
	p += 8;

	if (msg->flags & TCPCLV4_XFER_FLAG_START)
	{
		write_u32(p, msg->extensions_len);
		p += 4;

		if (msg->extensions_len > 0)
		{
			memcpy(p, msg->extensions, msg->extensions_len);
			p += msg->extensions_len;
		}
	}

	write_u64(p, msg->data_len);
	p += 8;

	if (msg->data_len > 0)
	{
		memcpy(p, msg->data, msg->data_len);
		p += msg->data_len;
	}

	*out_len = p - buf;

	return 0;
}

/*	XFER_ACK (§5.2.3): type(1) + flags(1) + xfer_id(8) + ack_len(8)	*/

int tcpclv4_encode_xfer_ack(const tcpclv4_xfer_ack_t *msg,
			     uint8_t *buf, size_t buf_cap,
			     size_t *out_len)
{
	if (msg == NULL || buf == NULL || out_len == NULL || buf_cap < 18)
	{
		return -1;
	}

	buf[0] = TCPCLV4_MSG_XFER_ACK;
	buf[1] = msg->flags;
	write_u64(buf + 2, msg->transfer_id);
	write_u64(buf + 10, msg->ack_len);

	*out_len = 18;

	return 0;
}

/*	XFER_REFUSE (§5.2.3): type(1) + reason(1) + xfer_id(8)		*/

int tcpclv4_encode_xfer_refuse(const tcpclv4_xfer_refuse_t *msg,
				uint8_t *buf, size_t buf_cap,
				size_t *out_len)
{
	if (msg == NULL || buf == NULL || out_len == NULL || buf_cap < 10)
	{
		return -1;
	}

	buf[0] = TCPCLV4_MSG_XFER_REFUSE;
	buf[1] = msg->reason_code;
	write_u64(buf + 2, msg->transfer_id);

	*out_len = 10;

	return 0;
}

/*	KEEPALIVE (§5.1.2): type(1) only, no payload			*/

int tcpclv4_encode_keepalive(uint8_t *buf, size_t buf_cap,
			      size_t *out_len)
{
	if (buf == NULL || out_len == NULL || buf_cap < 1)
	{
		return -1;
	}

	buf[0] = TCPCLV4_MSG_KEEPALIVE;
	*out_len = 1;

	return 0;
}

/*	MSG_REJECT (§5.1.1): type(1) + reason(1) + rejected_hdr(1)	*/

int tcpclv4_encode_msg_reject(const tcpclv4_msg_reject_t *msg,
			       uint8_t *buf, size_t buf_cap,
			       size_t *out_len)
{
	if (msg == NULL || buf == NULL || out_len == NULL || buf_cap < 3)
	{
		return -1;
	}

	buf[0] = TCPCLV4_MSG_MSG_REJECT;
	buf[1] = msg->reason_code;
	buf[2] = msg->rejected_msg_header;

	*out_len = 3;

	return 0;
}

/*	==============================================================
 *	STREAM FRAMING
 *	==============================================================
 */

int tcpclv4_msg_frame_len(const uint8_t *buf, size_t len,
			   uint64_t max_msg_len, size_t *msg_len)
{
	uint64_t	total;
	uint64_t	varpart;
	size_t		offset;

	/*	A frame's length fields are bounded by max_msg_len.  When
	 *	that is 0 ("unlimited"), clamp to an absolute 1 TiB ceiling
	 *	so the length arithmetic below cannot overflow on a hostile
	 *	value.							*/

	uint64_t	limit = max_msg_len ? max_msg_len
			: ((uint64_t) 1 << 40);

	if (buf == NULL || msg_len == NULL)
	{
		return -1;
	}

	*msg_len = 0;

	if (len < 1)
	{
		return 0;	/* Need at least the type byte */
	}

	switch (buf[0])
	{
	case TCPCLV4_MSG_KEEPALIVE:
		total = 1;
		break;

	case TCPCLV4_MSG_SESS_TERM:
	case TCPCLV4_MSG_MSG_REJECT:
		total = 3;
		break;

	case TCPCLV4_MSG_XFER_REFUSE:
		total = 10;
		break;

	case TCPCLV4_MSG_XFER_ACK:
		total = 18;
		break;

	case TCPCLV4_MSG_XFER_SEGMENT:

		/*	type(1) + flags(1) + xfer_id(8)
		 *	+ [ext_len(4) + ext, START only]
		 *	+ data_len(8) + data			*/

		if (len < 2)
		{
			return 0;
		}

		offset = 10;
		if (buf[1] & TCPCLV4_XFER_FLAG_START)
		{
			if (len < offset + 4)
			{
				return 0;
			}

			varpart = read_u32(buf + offset);
			if (varpart > limit)
			{
				return -1;
			}

			offset += 4 + (size_t) varpart;
		}

		if (len < offset + 8)
		{
			return 0;
		}

		varpart = read_u64(buf + offset);
		if (varpart > limit)
		{
			return -1;
		}

		total = offset + 8 + varpart;
		break;

	case TCPCLV4_MSG_SESS_INIT:

		/*	type(1) + keepalive(2) + seg_mru(8) + xfer_mru(8)
		 *	+ node_id_len(2) + node_id
		 *	+ ext_len(4) + ext			*/

		if (len < 21)
		{
			return 0;
		}

		varpart = read_u16(buf + 19);	/* node_id_len */
		offset = 21 + (size_t) varpart;
		if (len < offset + 4)
		{
			if (offset + 4 > limit)
			{
				return -1;
			}

			return 0;
		}

		varpart = read_u32(buf + offset);	/* ext_len */
		if (varpart > limit)
		{
			return -1;
		}

		total = offset + 4 + varpart;
		break;

	default:
		return -1;	/* Unknown message type */
	}

	if (total > limit)
	{
		return -1;
	}

	*msg_len = (size_t) total;
	return (len >= total) ? 1 : 0;
}

/*	==============================================================
 *	MEMORY MANAGEMENT
 *	==============================================================
 */

void tcpclv4_sess_init_free(tcpclv4_sess_init_t *msg)
{
	if (msg == NULL)
	{
		return;
	}

	free(msg->node_id);
	msg->node_id = NULL;
	free(msg->extensions);
	msg->extensions = NULL;
}

void tcpclv4_xfer_segment_free(tcpclv4_xfer_segment_t *msg)
{
	if (msg == NULL)
	{
		return;
	}

	free(msg->extensions);
	msg->extensions = NULL;

	/*	msg->data points into the caller's buffer; not freed.	*/
}

/*	==============================================================
 *	UTILITY FUNCTIONS
 *	==============================================================
 */

int tcpclv4_get_msg_type(const uint8_t *buf, size_t len)
{
	if (buf == NULL || len < 1)
	{
		return -1;
	}

	return buf[0];
}

const char* tcpclv4_msg_type_name(uint8_t msg_type)
{
	switch (msg_type)
	{
	case TCPCLV4_MSG_SESS_INIT:	return "SESS_INIT";
	case TCPCLV4_MSG_SESS_TERM:	return "SESS_TERM";
	case TCPCLV4_MSG_XFER_SEGMENT:	return "XFER_SEGMENT";
	case TCPCLV4_MSG_XFER_ACK:	return "XFER_ACK";
	case TCPCLV4_MSG_XFER_REFUSE:	return "XFER_REFUSE";
	case TCPCLV4_MSG_KEEPALIVE:	return "KEEPALIVE";
	case TCPCLV4_MSG_MSG_REJECT:	return "MSG_REJECT";
	default:			return "UNKNOWN";
	}
}
