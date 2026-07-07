/*
 * bpsh_proto.h: wire format for bpsh / bpshd.
 *
 * Each bundle payload is a CBOR positional array
 *	[ version, msgType, sessionId, seqNum, payload ]
 * where payload is either a CBOR byte string (REQ, STDIN_CHUNK,
 * STDOUT, STDERR, CWD, ERROR; INIT carries an optional 1-byte
 * stdin-request flag), an empty CBOR byte string (INIT_ACK,
 * STDIN_EOF, EXIT_SESSION), or a 2-element CBOR array
 * [ exitCode, exitCause ] (EXIT).
 */

#ifndef BPSH_PROTO_H
#define BPSH_PROTO_H

#include "bp.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BPSH_VERSION	     1
#define BPSH_DEFAULT_SERVICE 22

/*	Max bundle source EID length bpsh_recv_frame will copy out.	*/
#define BPSH_MAX_EID	     256

typedef enum
{
	BpshMsgInit = 1,	/* C->S, no payload	*/
	BpshMsgInitAck = 2,	/* S->C, no payload	*/
	BpshMsgReq = 3,		/* C->S, command line	*/
	BpshMsgStdinChunk = 4,	/* C->S, bytes		*/
	BpshMsgStdinEof = 5,	/* C->S, no payload	*/
	BpshMsgStdout = 6,	/* S->C, bytes		*/
	BpshMsgStderr = 7,	/* S->C, bytes		*/
	BpshMsgExit = 8,	/* S->C, [code, cause]	*/
	BpshMsgError = 9,	/* S->C, message	*/
	BpshMsgExitSession = 10, /* C->S, no payload	*/
	BpshMsgCwd = 11		/* S->C, shell cwd path	*/
} BpshMsgType;

typedef enum
{
	BpshCauseNormal = 0,
	BpshCauseSignaled = 1,
	BpshCauseTimeout = 2,
	BpshCauseKilled = 3,
	BpshCauseOutputLimit = 4
} BpshExitCause;

typedef struct
{
	int	    version;
	BpshMsgType msgType;
	uvast	    sessionId;
	uvast	    seqNum;

	/*	For byte-string payloads (REQ, STDIN_CHUNK, STDOUT,
	 *	STDERR, ERROR): pointer + length.  After bpsh_decode,
	 *	payload points into the caller-supplied buffer; copy
	 *	the bytes if you need to keep them past the decode
	 *	buffer's lifetime.					*/
	unsigned char *payload;
	uvast	       payloadLen;

	/*	For EXIT only:						*/
	int	      exitCode;
	BpshExitCause exitCause;
} BpshFrame;

/*	Bytes of overhead a maximal frame header consumes (array open,
 *	four uvast ints, byte-string header).  Add this to your payload
 *	length to size an encode buffer.				*/
#define BPSH_FRAME_OVERHEAD 64

/*	bpsh_encode: serialize frame into buf.  Returns number of bytes
 *	written, or -1 if buf is too small or frame is malformed.	*/
extern int bpsh_encode(const BpshFrame *frame, unsigned char *buf, size_t buflen);

/*	bpsh_decode: parse a frame from buf.  Returns number of bytes
 *	consumed on success, -1 on malformed input.  For byte-string
 *	payloads, frame->payload aliases into buf -- do not free buf
 *	until you are done with the frame.				*/
extern int bpsh_decode(unsigned char *buf, size_t buflen, BpshFrame *frame);

/*	The name lookup helpers for debug memo.				*/
extern char *bpsh_msgtype_name(BpshMsgType t);
extern char *bpsh_cause_name(BpshExitCause c);

/*	bpsh_send_frame: encode frame, wrap it in a ZCO, and bp_send it
 *	to destEid over the open endpoint sap.  Returns 0 on success,
 *	-1 on failure.							*/
extern int bpsh_send_frame(BpSAP sap, char *destEid, const BpshFrame *frame);

/*	bpsh_set_send_attendant: register a ReqAttendant so bpsh_send_frame
 *	creates outbound ZCOs in blocking mode -- waiting for ZCO space
 *	instead of failing when the outbound budget is full.  This gives
 *	the sender flow control for large output (a la bpsource).  Pass
 *	NULL (the default) for non-blocking sends.			*/
extern void bpsh_set_send_attendant(ReqAttendant *attendant);

/*	Convenience senders that build a frame of the given kind and ship
 *	it via bpsh_send_frame.  Each returns 0 on success, -1 on error.	*/
extern int bpsh_send_ctl(BpSAP sap, char *destEid, BpshMsgType type,
		uvast sessionId, uvast seq);
extern int bpsh_send_bytes(BpSAP sap, char *destEid, BpshMsgType type,
		uvast sessionId, uvast seq, unsigned char *bytes, size_t len);
extern int bpsh_send_exit(BpSAP sap, char *destEid, uvast sessionId, uvast seq,
		int code, BpshExitCause cause);
extern int bpsh_send_error(BpSAP sap, char *destEid, uvast sessionId, uvast seq,
		char *msg);

/*	Return codes for bpsh_recv_frame.				*/
#define BPSH_RECV_OK	 1   /* frame decoded; *rawBytes owns the bytes	*/
#define BPSH_RECV_NONE	 0   /* no bundle (poll empty / interrupted)	*/
#define BPSH_RECV_SKIP	 2   /* a bundle arrived but was unusable; retry	*/
#define BPSH_RECV_FATAL	 (-1) /* bp_receive error / endpoint stopped	*/

/*	bpsh_recv_frame: receive one bundle from sap (BP_BLOCKING or
 *	BP_POLL) and decode it.  On BPSH_RECV_OK, *rawBytes is set to an
 *	MTAKE'd buffer that owns the bundle bytes (frame->payload aliases
 *	into it) -- the caller must MRELEASE it once done with frame;
 *	*rawLen receives its length, and srcEid (if non-NULL) receives the
 *	bundle source EID.  maxLen rejects oversized bundles (0 = no cap).
 *	Returns one of the BPSH_RECV_* codes above.			*/
extern int bpsh_recv_frame(BpSAP sap, int blocking, size_t maxLen,
		BpshFrame *frame, unsigned char **rawBytes, size_t *rawLen,
		char *srcEid, size_t srcEidLen);

#ifdef __cplusplus
}
#endif

#endif /* BPSH_PROTO_H */
