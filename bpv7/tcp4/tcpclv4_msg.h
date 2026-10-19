/*
	tcpclv4_msg.h:	TCPCLv4 message encode/decode library (RFC 9174).

			Stateless pure functions for parsing and
			serializing TCPCLv4 protocol messages.
			No I/O, no state - just wire format conversion.

	Author: iondev33

	Copyright (c) 2026, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
								*/

#ifndef _TCPCLV4_MSG_H_
#define _TCPCLV4_MSG_H_

#include "platform.h"

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*	RFC 9174 Message Type Codes (Table 1)				*/

#define TCPCLV4_MSG_SESS_INIT		0x07
#define TCPCLV4_MSG_SESS_TERM		0x05
#define TCPCLV4_MSG_XFER_SEGMENT	0x01
#define TCPCLV4_MSG_XFER_ACK		0x02
#define TCPCLV4_MSG_XFER_REFUSE		0x03
#define TCPCLV4_MSG_KEEPALIVE		0x04
#define TCPCLV4_MSG_MSG_REJECT		0x06

/*	Contact Header Flags (§4.3)					*/

#define TCPCLV4_CONTACT_FLAG_CAN_TLS	0x01

/*	XFER_SEGMENT Flags (§5.2.1)					*/

#define TCPCLV4_XFER_FLAG_END		0x01
#define TCPCLV4_XFER_FLAG_START		0x02

/*	SESS_TERM Reason Codes (§6.1, Table 3)				*/

#define TCPCLV4_TERM_UNKNOWN		0x00
#define TCPCLV4_TERM_IDLE_TIMEOUT	0x01
#define TCPCLV4_TERM_VERSION_MISMATCH	0x02
#define TCPCLV4_TERM_BUSY		0x03
#define TCPCLV4_TERM_CONTACT_FAILURE	0x04
#define TCPCLV4_TERM_RESOURCE_EXHAUSTION 0x05

/*	XFER_REFUSE Reason Codes (§5.2.4, Table 2)			*/

#define TCPCLV4_REFUSE_UNKNOWN		0x00
#define TCPCLV4_REFUSE_COMPLETED	0x01
#define TCPCLV4_REFUSE_NO_RESOURCES	0x02
#define TCPCLV4_REFUSE_RETRANSMIT	0x03
#define TCPCLV4_REFUSE_NOT_ACCEPTABLE	0x04
#define TCPCLV4_REFUSE_EXTENSION_FAILURE 0x05

/*	MSG_REJECT Reason Codes (§5.4, Table 4)			*/

#define TCPCLV4_REJECT_UNKNOWN		0x01
#define TCPCLV4_REJECT_VERSION_MISMATCH	0x02
#define TCPCLV4_REJECT_INCONSISTENT	0x03

/*	Contact Header (§4.3) - Magic "dtn!" + flags			*/

#define TCPCLV4_CONTACT_MAGIC		0x64746E21	/* "dtn!" */
#define TCPCLV4_CONTACT_HEADER_LEN	4

typedef struct
{
	uint32_t	magic;		/* Always TCPCLV4_CONTACT_MAGIC */
	uint8_t		flags;		/* CAN_TLS, etc. */
} tcpclv4_contact_header_t;

/*	SESS_INIT Message (§5.1)					*/

typedef struct
{
	uint16_t	keepalive_interval;	/* Seconds */
	uint64_t	segment_mru;		/* Max bytes per segment */
	uint64_t	transfer_mru;		/* Max bytes per transfer */
	char		*node_id;		/* Peer node ID (caller owns) */
	uint16_t	node_id_len;		/* Length of node_id string */

	/* Session extension items (optional, not yet implemented) */
	uint8_t		*extensions;		/* Raw extension data */
	uint16_t	extensions_len;
} tcpclv4_sess_init_t;

/*	SESS_TERM Message (§6.1)					*/

typedef struct
{
	uint8_t		flags;			/* Reserved, must be 0 */
	uint8_t		reason_code;		/* TCPCLV4_TERM_* */
} tcpclv4_sess_term_t;

/*	XFER_SEGMENT Message (§5.2.1)					*/

typedef struct
{
	uint8_t		flags;			/* START, END bits */
	uint64_t	transfer_id;

	/* For non-START segments: */
	uint64_t	data_offset;		/* 0 for START segments */

	/* Transfer extension items (optional, not yet implemented) */
	uint8_t		*extensions;		/* Raw extension data */
	uint16_t	extensions_len;

	/* Payload data (not copied, points into original buffer) */
	const uint8_t	*data;
	uint64_t	data_len;
} tcpclv4_xfer_segment_t;

/*	XFER_ACK Message (§5.2.2)					*/

typedef struct
{
	uint8_t		flags;			/* Reserved, must be 0 */
	uint64_t	transfer_id;
	uint64_t	ack_len;		/* Total bytes acknowledged */
} tcpclv4_xfer_ack_t;

/*	XFER_REFUSE Message (§5.2.4)					*/

typedef struct
{
	uint8_t		reason_code;		/* TCPCLV4_REFUSE_* */
	uint64_t	transfer_id;
} tcpclv4_xfer_refuse_t;

/*	KEEPALIVE Message (§5.3) - No payload, just message type byte	*/
/*	MSG_REJECT Message (§5.4)					*/

typedef struct
{
	uint8_t		reason_code;		/* TCPCLV4_REJECT_* */
	uint8_t		rejected_msg_header;	/* First byte of bad msg */
} tcpclv4_msg_reject_t;

/*	==============================================================
 *	DECODE FUNCTIONS: Wire bytes → C struct
 *	==============================================================
 *
 *	All decode functions follow this pattern:
 *	  - Input: const uint8_t *buf, size_t len (wire format bytes)
 *	  - Output: pointer to message struct to populate
 *	  - Return: 0 on success, -1 on error
 *
 *	For messages with variable-length fields (node_id, extensions,
 *	data), the struct contains pointers. The caller is responsible
 *	for freeing these using the corresponding _free() function.
 */

int tcpclv4_decode_contact_header(const uint8_t *buf, size_t len,
				   tcpclv4_contact_header_t *out);

int tcpclv4_decode_sess_init(const uint8_t *buf, size_t len,
			      tcpclv4_sess_init_t *out);

int tcpclv4_decode_sess_term(const uint8_t *buf, size_t len,
			      tcpclv4_sess_term_t *out);

int tcpclv4_decode_xfer_segment(const uint8_t *buf, size_t len,
				 tcpclv4_xfer_segment_t *out);

int tcpclv4_decode_xfer_ack(const uint8_t *buf, size_t len,
			     tcpclv4_xfer_ack_t *out);

int tcpclv4_decode_xfer_refuse(const uint8_t *buf, size_t len,
				tcpclv4_xfer_refuse_t *out);

int tcpclv4_decode_msg_reject(const uint8_t *buf, size_t len,
			       tcpclv4_msg_reject_t *out);

/*	==============================================================
 *	ENCODE FUNCTIONS: C struct → wire bytes
 *	==============================================================
 *
 *	All encode functions follow this pattern:
 *	  - Input: pointer to message struct
 *	  - Output: uint8_t *buf (output buffer), size_t buf_cap (capacity)
 *	  - Output: size_t *out_len (actual bytes written)
 *	  - Return: 0 on success, -1 on error (buffer too small, etc.)
 */

int tcpclv4_encode_contact_header(const tcpclv4_contact_header_t *msg,
				   uint8_t *buf, size_t buf_cap,
				   size_t *out_len);

int tcpclv4_encode_sess_init(const tcpclv4_sess_init_t *msg,
			      uint8_t *buf, size_t buf_cap,
			      size_t *out_len);

int tcpclv4_encode_sess_term(const tcpclv4_sess_term_t *msg,
			      uint8_t *buf, size_t buf_cap,
			      size_t *out_len);

int tcpclv4_encode_xfer_segment(const tcpclv4_xfer_segment_t *msg,
				 uint8_t *buf, size_t buf_cap,
				 size_t *out_len);

int tcpclv4_encode_xfer_ack(const tcpclv4_xfer_ack_t *msg,
			     uint8_t *buf, size_t buf_cap,
			     size_t *out_len);

int tcpclv4_encode_xfer_refuse(const tcpclv4_xfer_refuse_t *msg,
				uint8_t *buf, size_t buf_cap,
				size_t *out_len);

int tcpclv4_encode_keepalive(uint8_t *buf, size_t buf_cap,
			      size_t *out_len);

int tcpclv4_encode_msg_reject(const tcpclv4_msg_reject_t *msg,
			       uint8_t *buf, size_t buf_cap,
			       size_t *out_len);

/*	==============================================================
 *	MEMORY MANAGEMENT
 *	==============================================================
 *
 *	For structs with dynamically allocated fields, these functions
 *	free the internal memory. The struct itself is caller-owned.
 */

void tcpclv4_sess_init_free(tcpclv4_sess_init_t *msg);
void tcpclv4_xfer_segment_free(tcpclv4_xfer_segment_t *msg);

/*	==============================================================
 *	UTILITY FUNCTIONS
 *	==============================================================
 */

/*	Get message type from first byte of wire format			*/
int tcpclv4_get_msg_type(const uint8_t *buf, size_t len);

/*	Get message type name string for logging			*/
const char* tcpclv4_msg_type_name(uint8_t msg_type);

#ifdef __cplusplus
}
#endif

#endif  /* _TCPCLV4_MSG_H_ */
