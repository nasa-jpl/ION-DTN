/*
	tcpclv4_msg.c:	TCPCLv4 message encode/decode implementation.

			Implements RFC 9174 wire format parsing.
			All multi-byte integers use network byte order
			(big-endian) as specified in the RFC.

	Author: iondev33

	Copyright (c) 2026, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
								*/

#include "tcpclv4_msg.h"
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>  /* htons, htonl, ntohs, ntohl */

/*	Network byte order conversion helpers for 64-bit values		*/

/*	Some platforms (Solaris) define htonll/ntohll as macros.
 *	Only define our own if they don't already exist.		*/

#ifndef htonll
static uint64_t htonll(uint64_t hostval)
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
	return ((uint64_t)htonl((uint32_t)(hostval & 0xFFFFFFFF)) << 32)
		| htonl((uint32_t)(hostval >> 32));
#else
	return hostval;
#endif
}
#endif

#ifndef ntohll
static uint64_t ntohll(uint64_t netval)
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
	return ((uint64_t)ntohl((uint32_t)(netval & 0xFFFFFFFF)) << 32)
		| ntohl((uint32_t)(netval >> 32));
#else
	return netval;
#endif
}
#endif

/*	Helper: read big-endian uint16 from buffer			*/

static uint16_t read_u16(const uint8_t *buf)
{
	uint16_t val;

	memcpy(&val, buf, 2);
	return ntohs(val);
}

/*	Helper: read big-endian uint32 from buffer			*/

static uint32_t read_u32(const uint8_t *buf)
{
	uint32_t val;

	memcpy(&val, buf, 4);
	return ntohl(val);
}

/*	Helper: read big-endian uint64 from buffer			*/

static uint64_t read_u64(const uint8_t *buf)
{
	uint64_t val;

	memcpy(&val, buf, 8);
	return ntohll(val);
}

/*	Helper: write big-endian uint16 to buffer			*/

static void write_u16(uint8_t *buf, uint16_t val)
{
	uint16_t netval = htons(val);

	memcpy(buf, &netval, 2);
}

/*	Helper: write big-endian uint32 to buffer			*/

static void write_u32(uint8_t *buf, uint32_t val)
{
	uint32_t netval = htonl(val);

	memcpy(buf, &netval, 4);
}

/*	Helper: write big-endian uint64 to buffer			*/

static void write_u64(uint8_t *buf, uint64_t val)
{
	uint64_t netval = htonll(val);

	memcpy(buf, &netval, 8);
}

/*	==============================================================
 *	DECODE FUNCTIONS
 *	==============================================================
 */

/*	Contact Header: magic(4) [no flags in header, flags elsewhere]	*/

int tcpclv4_decode_contact_header(const uint8_t *buf, size_t len,
				   tcpclv4_contact_header_t *out)
{
	if (len < 4)
	{
		return -1;	/* Insufficient data */
	}

	out->magic = read_u32(buf);
	out->flags = 0;		/* Flags come from subsequent negotiation */

	if (out->magic != TCPCLV4_CONTACT_MAGIC)
	{
		return -1;	/* Invalid magic */
	}

	return 0;
}

/*	SESS_INIT: type(1) + keepalive(2) + seg_mru(8) + xfer_mru(8)
 *		   + node_id_len(2) + node_id + extensions		*/

int tcpclv4_decode_sess_init(const uint8_t *buf, size_t len,
			      tcpclv4_sess_init_t *out)
{
	const uint8_t	*p;
	size_t		remaining;

	if (len < 22)	/* 1 + 2 + 8 + 8 + 2 + 1 (min node_id) */
	{
		return -1;
	}

	p = buf;
	remaining = len;

	/* Skip message type byte (already parsed by caller) */
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
	if (remaining < out->node_id_len)
	{
		return -1;	/* Insufficient data */
	}

	out->node_id = malloc(out->node_id_len + 1);
	if (out->node_id == NULL)
	{
		return -1;
	}

	memcpy(out->node_id, p, out->node_id_len);
	out->node_id[out->node_id_len] = '\0';	/* NULL-terminate */
	p += out->node_id_len;
	remaining -= out->node_id_len;

	/* Session extension items (remaining bytes, optional) */
	out->extensions_len = remaining;
	if (remaining > 0)
	{
		out->extensions = malloc(remaining);
		if (out->extensions == NULL)
		{
			free(out->node_id);
			return -1;
		}

		memcpy(out->extensions, p, remaining);
	}
	else
	{
		out->extensions = NULL;
	}

	return 0;
}

/*	SESS_TERM: type(1) + flags(1) + reason(1)			*/

int tcpclv4_decode_sess_term(const uint8_t *buf, size_t len,
			      tcpclv4_sess_term_t *out)
{
	if (len < 3)
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

/*	XFER_SEGMENT: type(1) + flags(1) + xfer_id(8) + [extensions]
 *		      + [data_offset(8) if !START] + data		*/

int tcpclv4_decode_xfer_segment(const uint8_t *buf, size_t len,
				 tcpclv4_xfer_segment_t *out)
{
	const uint8_t	*p;
	size_t		remaining;
	uint32_t	ext_items_len;

	if (len < 10)	/* Minimum: type(1) + flags(1) + xfer_id(8) */
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

	/* Transfer extension items length (4 bytes) */
	if (remaining < 4)
	{
		return -1;
	}

	ext_items_len = read_u32(p);
	p += 4;
	remaining -= 4;

	/* Transfer extension items (optional) */
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
	else
	{
		out->extensions = NULL;
		out->extensions_len = 0;
	}

	/* Data offset (8 bytes, only if NOT a START segment) */
	if ((out->flags & TCPCLV4_XFER_FLAG_START) == 0)
	{
		if (remaining < 8)
		{
			free(out->extensions);
			return -1;
		}

		out->data_offset = read_u64(p);
		p += 8;
		remaining -= 8;
	}
	else
	{
		out->data_offset = 0;
	}

	/* Payload data (remainder of message) */
	out->data = p;
	out->data_len = remaining;

	return 0;
}

/*	XFER_ACK: type(1) + flags(1) + xfer_id(8) + ack_len(8)		*/

int tcpclv4_decode_xfer_ack(const uint8_t *buf, size_t len,
			     tcpclv4_xfer_ack_t *out)
{
	if (len < 18)	/* 1 + 1 + 8 + 8 */
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

/*	XFER_REFUSE: type(1) + reason(1) + xfer_id(8)			*/

int tcpclv4_decode_xfer_refuse(const uint8_t *buf, size_t len,
				tcpclv4_xfer_refuse_t *out)
{
	if (len < 10)	/* 1 + 1 + 8 */
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

/*	MSG_REJECT: type(1) + reason(1) + rejected_header(1)		*/

int tcpclv4_decode_msg_reject(const uint8_t *buf, size_t len,
			       tcpclv4_msg_reject_t *out)
{
	if (len < 3)
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

/*	Contact Header: magic(4)					*/

int tcpclv4_encode_contact_header(const tcpclv4_contact_header_t *msg,
				   uint8_t *buf, size_t buf_cap,
				   size_t *out_len)
{
	(void)msg;	/* Contact header only contains magic constant */

	if (buf_cap < 4)
	{
		return -1;
	}

	write_u32(buf, TCPCLV4_CONTACT_MAGIC);
	*out_len = 4;

	return 0;
}

/*	SESS_INIT: type(1) + keepalive(2) + seg_mru(8) + xfer_mru(8)
 *		   + node_id_len(2) + node_id + extensions		*/

int tcpclv4_encode_sess_init(const tcpclv4_sess_init_t *msg,
			      uint8_t *buf, size_t buf_cap,
			      size_t *out_len)
{
	uint8_t	*p;
	size_t	required;

	required = 1 + 2 + 8 + 8 + 2 + msg->node_id_len
		   + msg->extensions_len;

	if (buf_cap < required)
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

	if (msg->extensions_len > 0)
	{
		memcpy(p, msg->extensions, msg->extensions_len);
		p += msg->extensions_len;
	}

	*out_len = p - buf;

	return 0;
}

/*	SESS_TERM: type(1) + flags(1) + reason(1)			*/

int tcpclv4_encode_sess_term(const tcpclv4_sess_term_t *msg,
			      uint8_t *buf, size_t buf_cap,
			      size_t *out_len)
{
	if (buf_cap < 3)
	{
		return -1;
	}

	buf[0] = TCPCLV4_MSG_SESS_TERM;
	buf[1] = msg->flags;
	buf[2] = msg->reason_code;

	*out_len = 3;

	return 0;
}

/*	XFER_SEGMENT: type(1) + flags(1) + xfer_id(8) + ext_len(4)
 *		      + [extensions] + [data_offset(8)] + data		*/

int tcpclv4_encode_xfer_segment(const tcpclv4_xfer_segment_t *msg,
				 uint8_t *buf, size_t buf_cap,
				 size_t *out_len)
{
	uint8_t	*p;
	size_t	required;

	required = 1 + 1 + 8 + 4 + msg->extensions_len;

	if ((msg->flags & TCPCLV4_XFER_FLAG_START) == 0)
	{
		required += 8;	/* data_offset */
	}

	required += msg->data_len;

	if (buf_cap < required)
	{
		return -1;
	}

	p = buf;

	*p++ = TCPCLV4_MSG_XFER_SEGMENT;
	*p++ = msg->flags;
	write_u64(p, msg->transfer_id);
	p += 8;

	/* Transfer extension items length */
	write_u32(p, msg->extensions_len);
	p += 4;

	if (msg->extensions_len > 0)
	{
		memcpy(p, msg->extensions, msg->extensions_len);
		p += msg->extensions_len;
	}

	/* Data offset (only if NOT a START segment) */
	if ((msg->flags & TCPCLV4_XFER_FLAG_START) == 0)
	{
		write_u64(p, msg->data_offset);
		p += 8;
	}

	/* Payload data */
	memcpy(p, msg->data, msg->data_len);
	p += msg->data_len;

	*out_len = p - buf;

	return 0;
}

/*	XFER_ACK: type(1) + flags(1) + xfer_id(8) + ack_len(8)		*/

int tcpclv4_encode_xfer_ack(const tcpclv4_xfer_ack_t *msg,
			     uint8_t *buf, size_t buf_cap,
			     size_t *out_len)
{
	if (buf_cap < 18)
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

/*	XFER_REFUSE: type(1) + reason(1) + xfer_id(8)			*/

int tcpclv4_encode_xfer_refuse(const tcpclv4_xfer_refuse_t *msg,
				uint8_t *buf, size_t buf_cap,
				size_t *out_len)
{
	if (buf_cap < 10)
	{
		return -1;
	}

	buf[0] = TCPCLV4_MSG_XFER_REFUSE;
	buf[1] = msg->reason_code;
	write_u64(buf + 2, msg->transfer_id);

	*out_len = 10;

	return 0;
}

/*	KEEPALIVE: type(1) only, no payload				*/

int tcpclv4_encode_keepalive(uint8_t *buf, size_t buf_cap,
			      size_t *out_len)
{
	if (buf_cap < 1)
	{
		return -1;
	}

	buf[0] = TCPCLV4_MSG_KEEPALIVE;
	*out_len = 1;

	return 0;
}

/*	MSG_REJECT: type(1) + reason(1) + rejected_header(1)		*/

int tcpclv4_encode_msg_reject(const tcpclv4_msg_reject_t *msg,
			       uint8_t *buf, size_t buf_cap,
			       size_t *out_len)
{
	if (buf_cap < 3)
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
 *	MEMORY MANAGEMENT
 *	==============================================================
 */

void tcpclv4_sess_init_free(tcpclv4_sess_init_t *msg)
{
	if (msg->node_id != NULL)
	{
		free(msg->node_id);
		msg->node_id = NULL;
	}

	if (msg->extensions != NULL)
	{
		free(msg->extensions);
		msg->extensions = NULL;
	}
}

void tcpclv4_xfer_segment_free(tcpclv4_xfer_segment_t *msg)
{
	if (msg->extensions != NULL)
	{
		free(msg->extensions);
		msg->extensions = NULL;
	}

	/* Note: msg->data is not freed - it points into the original
	 * buffer passed to decode. Caller owns that buffer. */
}

/*	==============================================================
 *	UTILITY FUNCTIONS
 *	==============================================================
 */

int tcpclv4_get_msg_type(const uint8_t *buf, size_t len)
{
	if (len < 1)
	{
		return -1;
	}

	return buf[0];
}

const char* tcpclv4_msg_type_name(uint8_t msg_type)
{
	switch (msg_type)
	{
	case TCPCLV4_MSG_XFER_SEGMENT:
		return "XFER_SEGMENT";
	case TCPCLV4_MSG_XFER_ACK:
		return "XFER_ACK";
	case TCPCLV4_MSG_XFER_REFUSE:
		return "XFER_REFUSE";
	case TCPCLV4_MSG_KEEPALIVE:
		return "KEEPALIVE";
	case TCPCLV4_MSG_SESS_TERM:
		return "SESS_TERM";
	case TCPCLV4_MSG_MSG_REJECT:
		return "MSG_REJECT";
	case TCPCLV4_MSG_SESS_INIT:
		return "SESS_INIT";
	default:
		return "UNKNOWN";
	}
}
