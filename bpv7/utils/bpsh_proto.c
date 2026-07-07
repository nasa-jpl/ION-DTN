/*
 * bpsh_proto.c: CBOR encode/decode for the bpsh wire format, plus the
 * shared frame send/receive helpers used by bpsh and bpshd.
 */

#include "bpsh_proto.h"
#include "cbor.h"

int bpsh_encode(const BpshFrame *frame, unsigned char *buf, size_t buflen)
{
	unsigned char *cursor = buf;
	unsigned char *end = buf + buflen;
	uvast	       needed;

	if (frame == NULL || buf == NULL)
	{
		return -1;
	}

	/*	Conservative size check: header + (payload | exit pair).
	 *	Worst case for bytestring payload = overhead + payloadLen.
	 *	EXIT carries a 2-element array of small ints, well under
	 *	BPSH_FRAME_OVERHEAD.					*/
	if (frame->msgType == BpshMsgExit)
	{
		needed = BPSH_FRAME_OVERHEAD;
	}
	else
	{
		needed = BPSH_FRAME_OVERHEAD + frame->payloadLen;
	}

	if (buflen < needed)
	{
		return -1;
	}

	cbor_encode_array_open(5, &cursor);
	cbor_encode_integer((uvast) frame->version, &cursor);
	cbor_encode_integer((uvast) frame->msgType, &cursor);
	cbor_encode_integer(frame->sessionId, &cursor);
	cbor_encode_integer(frame->seqNum, &cursor);

	switch (frame->msgType)
	{
	case BpshMsgInitAck:
	case BpshMsgStdinEof:
	case BpshMsgExitSession:
		cbor_encode_byte_string(NULL, 0, &cursor);
		break;

	/*	INIT carries an optional flag byte (0x01 = client will
	 *	send STDIN_CHUNK; absent / 0x00 = REPL mode).		*/
	case BpshMsgInit:
	case BpshMsgReq:
	case BpshMsgStdinChunk:
	case BpshMsgStdout:
	case BpshMsgStderr:
	case BpshMsgError:
	case BpshMsgCwd:
		cbor_encode_byte_string(frame->payload, frame->payloadLen,
				&cursor);
		break;

	case BpshMsgExit:
		cbor_encode_array_open(2, &cursor);
		cbor_encode_signed_int((vast) frame->exitCode, &cursor);
		cbor_encode_integer((uvast) frame->exitCause, &cursor);
		break;

	default:
		return -1;
	}

	if (cursor > end)
	{
		return -1;
	}

	return (int) (cursor - buf);
}

int bpsh_decode(unsigned char *buf, size_t buflen, BpshFrame *frame)
{
	unsigned char *cursor = buf;
	unsigned int   bytesBuffered = (unsigned int) buflen;
	uvast	       arrSize;
	uvast	       val;
	uvast	       bsSize;
	uvast	       exitArrSize;
	vast	       exitCode;

	if (buf == NULL || frame == NULL || buflen == 0)
	{
		return -1;
	}

	arrSize = 5; /* expected outer array size */
	if (cbor_decode_array_open(&arrSize, &cursor, &bytesBuffered) == 0)
	{
		return -1;
	}

	if (cbor_decode_integer(&val, CborAny, &cursor, &bytesBuffered) == 0)
	{
		return -1;
	}

	frame->version = (int) val;

	if (cbor_decode_integer(&val, CborAny, &cursor, &bytesBuffered) == 0)
	{
		return -1;
	}

	frame->msgType = (BpshMsgType) val;

	if (cbor_decode_integer(&frame->sessionId, CborAny, &cursor, &bytesBuffered)
			== 0)
	{
		return -1;
	}

	if (cbor_decode_integer(&frame->seqNum, CborAny, &cursor, &bytesBuffered)
			== 0)
	{
		return -1;
	}

	frame->payload = NULL;
	frame->payloadLen = 0;
	frame->exitCode = 0;
	frame->exitCause = BpshCauseNormal;

	switch (frame->msgType)
	{
	case BpshMsgInitAck:
	case BpshMsgStdinEof:
	case BpshMsgExitSession:
		/*	Input *size is the maximum allowed bytestring
		 *	length; for empty-payload messages we pin it to
		 *	0 so any non-empty bytestring is rejected.	*/
		bsSize = 0;
		if (cbor_decode_byte_string(NULL, &bsSize, &cursor, &bytesBuffered)
				== 0)
		{
			return -1;
		}

		break;

	/*	INIT now carries an optional flag bytestring
	 *	(0 or 1 byte; payload[0] == 0x01 means stdin-forward).	*/
	case BpshMsgInit:
	case BpshMsgReq:
	case BpshMsgStdinChunk:
	case BpshMsgStdout:
	case BpshMsgStderr:
	case BpshMsgError:
	case BpshMsgCwd:
		bsSize = bytesBuffered;
		if (cbor_decode_byte_string(NULL, &bsSize, &cursor, &bytesBuffered)
				== 0)
		{
			return -1;
		}

		/*	cbor_decode_byte_string with NULL value advances
		 *	cursor only to the start of the bytes; advance
		 *	past them manually for correct accounting.	*/
		frame->payload = cursor;
		frame->payloadLen = bsSize;
		cursor += bsSize;
		bytesBuffered -= bsSize;
		break;

	case BpshMsgExit:
		exitArrSize = 2; /* expected EXIT inner array size */
		if (cbor_decode_array_open(&exitArrSize, &cursor, &bytesBuffered)
				== 0)
		{
			return -1;
		}

		if (cbor_decode_signed_int(&exitCode, &cursor, &bytesBuffered)
				== 0)
		{
			return -1;
		}

		frame->exitCode = (int) exitCode;

		if (cbor_decode_integer(&val, CborAny, &cursor, &bytesBuffered)
				== 0)
		{
			return -1;
		}

		frame->exitCause = (BpshExitCause) val;
		break;

	default:
		return -1;
	}

	return (int) (cursor - buf);
}

char *bpsh_msgtype_name(BpshMsgType t)
{
	switch (t)
	{
	case BpshMsgInit:
		return "INIT";
	case BpshMsgInitAck:
		return "INIT_ACK";
	case BpshMsgReq:
		return "REQ";
	case BpshMsgStdinChunk:
		return "STDIN_CHUNK";
	case BpshMsgStdinEof:
		return "STDIN_EOF";
	case BpshMsgStdout:
		return "STDOUT";
	case BpshMsgStderr:
		return "STDERR";
	case BpshMsgExit:
		return "EXIT";
	case BpshMsgError:
		return "ERROR";
	case BpshMsgExitSession:
		return "EXIT_SESSION";
	case BpshMsgCwd:
		return "CWD";
	default:
		return "?";
	}
}

char *bpsh_cause_name(BpshExitCause c)
{
	switch (c)
	{
	case BpshCauseNormal:
		return "normal";
	case BpshCauseSignaled:
		return "signaled";
	case BpshCauseTimeout:
		return "timeout";
	case BpshCauseKilled:
		return "killed";
	case BpshCauseOutputLimit:
		return "output-limit";
	default:
		return "?";
	}
}

static ReqAttendant *sendAttendant = NULL;

void bpsh_set_send_attendant(ReqAttendant *attendant)
{
	sendAttendant = attendant;
}

int bpsh_send_frame(BpSAP sap, char *destEid, const BpshFrame *frame)
{
	Sdr	       sdr = bp_get_sdr();
	unsigned char *buf;
	int	       nbytes;
	Object	       payload;
	Object	       zco;
	Object	       newBundle;
	size_t	       bufsize;

	if (frame == NULL || destEid == NULL)
	{
		return -1;
	}

	bufsize = BPSH_FRAME_OVERHEAD + frame->payloadLen;
	buf = MTAKE(bufsize);
	if (buf == NULL)
	{
		putErrmsg("bpsh: out of memory encoding frame.", NULL);
		return -1;
	}

	nbytes = bpsh_encode(frame, buf, bufsize);
	if (nbytes < 0)
	{
		MRELEASE(buf);
		putErrmsg("bpsh: frame encode failed.", NULL);
		return -1;
	}

	if (sdr_begin_xn(sdr) < 0)
	{
		MRELEASE(buf);
		return -1;
	}

	payload = sdr_malloc(sdr, nbytes);
	if (payload)
	{
		sdr_write(sdr, payload, (char *) buf, nbytes);
	}

	if (sdr_end_xn(sdr) < 0 || payload == 0)
	{
		MRELEASE(buf);
		putErrmsg("bpsh: no SDR space for frame.", NULL);
		return -1;
	}

	MRELEASE(buf);

	/*	With an attendant this blocks until outbound ZCO space is
	 *	available rather than failing -- giving large output flow
	 *	control instead of a dropped (and sequence-gapping) frame.	*/
	zco = ionCreateZco(ZcoSdrSource, payload, 0, nbytes, BP_STD_PRIORITY, 0,
			ZcoOutbound, sendAttendant);
	if (zco == 0 || zco == (Object) ERROR)
	{
		putErrmsg("bpsh: can't create ZCO.", NULL);
		return -1;
	}

	if (bp_send(sap, destEid, NULL, 86400, BP_STD_PRIORITY,
			    NoCustodyRequested, 0, 0, NULL, zco, &newBundle)
			<= 0)
	{
		putErrmsg("bpsh: bp_send failed.", destEid);
		if (sdr_begin_xn(sdr) >= 0)
		{
			zco_destroy(sdr, zco);
			oK(sdr_end_xn(sdr));
		}

		return -1;
	}

	return 0;
}

int bpsh_send_ctl(BpSAP sap, char *destEid, BpshMsgType type, uvast sessionId,
		uvast seq)
{
	BpshFrame out;

	memset(&out, 0, sizeof(out));
	out.version = BPSH_VERSION;
	out.msgType = type;
	out.sessionId = sessionId;
	out.seqNum = seq;
	return bpsh_send_frame(sap, destEid, &out);
}

int bpsh_send_bytes(BpSAP sap, char *destEid, BpshMsgType type, uvast sessionId,
		uvast seq, unsigned char *bytes, size_t len)
{
	BpshFrame out;

	memset(&out, 0, sizeof(out));
	out.version = BPSH_VERSION;
	out.msgType = type;
	out.sessionId = sessionId;
	out.seqNum = seq;
	out.payload = bytes;
	out.payloadLen = len;
	return bpsh_send_frame(sap, destEid, &out);
}

int bpsh_send_exit(BpSAP sap, char *destEid, uvast sessionId, uvast seq,
		int code, BpshExitCause cause)
{
	BpshFrame out;

	memset(&out, 0, sizeof(out));
	out.version = BPSH_VERSION;
	out.msgType = BpshMsgExit;
	out.sessionId = sessionId;
	out.seqNum = seq;
	out.exitCode = code;
	out.exitCause = cause;
	return bpsh_send_frame(sap, destEid, &out);
}

int bpsh_send_error(BpSAP sap, char *destEid, uvast sessionId, uvast seq,
		char *msg)
{
	return bpsh_send_bytes(sap, destEid, BpshMsgError, sessionId, seq,
			(unsigned char *) msg, strlen(msg));
}

int bpsh_recv_frame(BpSAP sap, int blocking, size_t maxLen, BpshFrame *frame,
		unsigned char **rawBytes, size_t *rawLen, char *srcEid,
		size_t srcEidLen)
{
	Sdr	       sdr = bp_get_sdr();
	BpDelivery     dlv;
	vast	       bundleLen;
	ZcoReader      reader;
	unsigned char *bytes;

	*rawBytes = NULL;
	*rawLen = 0;

	if (bp_receive(sap, &dlv, blocking) < 0)
	{
		return BPSH_RECV_FATAL;
	}

	if (dlv.result == BpEndpointStopped)
	{
		bp_release_delivery(&dlv, 1);
		return BPSH_RECV_FATAL;
	}

	if (dlv.result != BpPayloadPresent || dlv.adu == 0)
	{
		bp_release_delivery(&dlv, 1);
		return BPSH_RECV_NONE;
	}

	if (sdr_begin_xn(sdr) < 0)
	{
		bp_release_delivery(&dlv, 1);
		return BPSH_RECV_SKIP;
	}

	bundleLen = zco_source_data_length(sdr, dlv.adu);
	if (bundleLen <= 0 || (maxLen != 0 && (size_t) bundleLen > maxLen))
	{
		sdr_exit_xn(sdr);
		bp_release_delivery(&dlv, 1);
		return BPSH_RECV_SKIP;
	}

	bytes = MTAKE((size_t) bundleLen);
	if (bytes == NULL)
	{
		sdr_exit_xn(sdr);
		bp_release_delivery(&dlv, 1);
		return BPSH_RECV_SKIP;
	}

	zco_start_receiving(dlv.adu, &reader);
	if (zco_receive_source(sdr, &reader, bundleLen, (char *) bytes) < 0)
	{
		MRELEASE(bytes);
		sdr_cancel_xn(sdr);
		bp_release_delivery(&dlv, 1);
		return BPSH_RECV_SKIP;
	}

	if (sdr_end_xn(sdr) < 0)
	{
		MRELEASE(bytes);
		bp_release_delivery(&dlv, 1);
		return BPSH_RECV_SKIP;
	}

	if (srcEid != NULL && srcEidLen > 0)
	{
		istrcpy(srcEid, dlv.bundleSourceEid, srcEidLen);
	}

	bp_release_delivery(&dlv, 1);

	if (bpsh_decode(bytes, (size_t) bundleLen, frame) < 0)
	{
		MRELEASE(bytes);
		return BPSH_RECV_SKIP;
	}

	*rawBytes = bytes;
	*rawLen = (size_t) bundleLen;
	return BPSH_RECV_OK;
}
